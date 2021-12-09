/*
 * Copyright (c) 2013-2016, 2019-2020, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/* Generic skb recycler */
#include "skbuff_recycle.h"
#include <linux/proc_fs.h>
#include <linux/string.h>

#include "skbuff_debug.h"

static struct proc_dir_entry *proc_net_skbrecycler;

static DEFINE_PER_CPU(struct sk_buff_head, recycle_list);
static int skb_recycle_max_skbs = SKB_RECYCLE_MAX_SKBS;

#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
static DEFINE_PER_CPU(struct sk_buff_head, recycle_spare_list);
static struct global_recycler glob_recycler;
static int skb_recycle_spare_max_skbs = SKB_RECYCLE_SPARE_MAX_SKBS;
#endif

inline struct sk_buff *skb_recycler_alloc(struct net_device *dev,
					  unsigned int length)
{
	unsigned long flags;
	struct sk_buff_head *h;
	struct sk_buff *skb = NULL;
	struct sk_buff *ln = NULL;

	if (unlikely(length > SKB_RECYCLE_SIZE))
		return NULL;

	h = &get_cpu_var(recycle_list);
	local_irq_save(flags);
	skb = skb_peek(h);
	if (skb) {
		ln = skb_peek_next(skb, h);
		skbuff_debugobj_activate(skb);
		/* Recalculate the sum for skb->next as next and prev pointers
		 * of skb->next will be updated in __skb_unlink
		 */
		skbuff_debugobj_sum_validate(ln);
		__skb_unlink(skb, h);
		skbuff_debugobj_sum_update(ln);
	}
#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	if (unlikely(!skb)) {
		u8 head;

		spin_lock(&glob_recycler.lock);
		/* If global recycle list is not empty, use global buffers */
		head = glob_recycler.head;
		if (unlikely(head == glob_recycler.tail)) {
			spin_unlock(&glob_recycler.lock);
		} else {
			struct sk_buff *gn = glob_recycler.pool[head].next;
			struct sk_buff *gp = glob_recycler.pool[head].prev;

			/* Move SKBs from global list to CPU pool */
			skbuff_debugobj_sum_validate(gn);
			skbuff_debugobj_sum_validate(gp);
			skb_queue_splice_init(&glob_recycler.pool[head], h);
			skbuff_debugobj_sum_update(gn);
			skbuff_debugobj_sum_update(gp);

			head = (head + 1) & SKB_RECYCLE_MAX_SHARED_POOLS_MASK;
			glob_recycler.head = head;
			spin_unlock(&glob_recycler.lock);
			/* We have refilled the CPU pool - dequeue */
			skb = skb_peek(h);
			if (skb) {
				/* Recalculate the sum for skb->next as next and
				 * prev pointers of skb->next will be updated
				 * in __skb_unlink
				 */
				ln = skb_peek_next(skb, h);
				skbuff_debugobj_activate(skb);
				skbuff_debugobj_sum_validate(ln);
				__skb_unlink(skb, h);
				skbuff_debugobj_sum_update(ln);
			}
		}
	}
#endif
	local_irq_restore(flags);
	put_cpu_var(recycle_list);

	if (likely(skb)) {
		struct skb_shared_info *shinfo;

		/* We're about to write a large amount to the skb to
		 * zero most of the structure so prefetch the start
		 * of the shinfo region now so it's in the D-cache
		 * before we start to write that.
		 */
		shinfo = skb_shinfo(skb);
		prefetchw(shinfo);

		zero_struct(skb, offsetof(struct sk_buff, tail));
		refcount_set(&skb->users, 1);
		skb->mac_header = (typeof(skb->mac_header))~0U;
		skb->transport_header = (typeof(skb->transport_header))~0U;
		zero_struct(shinfo, offsetof(struct skb_shared_info, dataref));
		atomic_set(&shinfo->dataref, 1);

		skb->data = skb->head + NET_SKB_PAD;
		skb_reset_tail_pointer(skb);

		skb->dev = dev;
	}

	return skb;
}

inline bool skb_recycler_consume(struct sk_buff *skb)
{
	unsigned long flags;
	struct sk_buff_head *h;
	struct sk_buff *ln = NULL;
	/* Can we recycle this skb?  If not, simply return that we cannot */
	if (unlikely(!consume_skb_can_recycle(skb, SKB_RECYCLE_MIN_SIZE,
					      SKB_RECYCLE_MAX_SIZE)))
		return false;

	/* If we can, then it will be much faster for us to recycle this one
	 * later than to allocate a new one from scratch.
	 */
	h = &get_cpu_var(recycle_list);
	local_irq_save(flags);
	/* Attempt to enqueue the CPU hot recycle list first */
	if (likely(skb_queue_len(h) < skb_recycle_max_skbs)) {
		ln = skb_peek(h);
		/* Recalculate the sum for peek of list as next and prev
		 * pointers of skb->next will be updated in __skb_queue_head
		 */
		skbuff_debugobj_sum_validate(ln);
		__skb_queue_head(h, skb);
		skbuff_debugobj_deactivate(skb);
		skbuff_debugobj_sum_update(ln);
		local_irq_restore(flags);
		preempt_enable();
		return true;
	}
#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	h = this_cpu_ptr(&recycle_spare_list);

	/* The CPU hot recycle list was full; if the spare list is also full,
	 * attempt to move the spare list to the global list for other CPUs to
	 * use.
	 */
	if (unlikely(skb_queue_len(h) >= skb_recycle_spare_max_skbs)) {
		u8 cur_tail, next_tail;

		spin_lock(&glob_recycler.lock);
		cur_tail = glob_recycler.tail;
		next_tail = (cur_tail + 1) & SKB_RECYCLE_MAX_SHARED_POOLS_MASK;
		if (next_tail != glob_recycler.head) {
			struct sk_buff_head *p = &glob_recycler.pool[cur_tail];
			struct sk_buff *hn = h->next, *hp = h->prev;

			/* Move SKBs from CPU pool to Global pool*/
			skbuff_debugobj_sum_validate(hp);
			skbuff_debugobj_sum_validate(hn);
			skb_queue_splice_init(h, p);
			skbuff_debugobj_sum_update(hp);
			skbuff_debugobj_sum_update(hn);

			/* Done with global list init */
			glob_recycler.tail = next_tail;
			spin_unlock(&glob_recycler.lock);

			/* Recalculate the sum for peek of list as next and prev
			 * pointers of skb->next will be updated in
			 * __skb_queue_head
			 */
			ln = skb_peek(h);
			skbuff_debugobj_sum_validate(ln);
			/* We have now cleared room in the spare;
			 * Initialize and enqueue skb into spare
			 */
			__skb_queue_head(h, skb);
			skbuff_debugobj_sum_update(ln);
			skbuff_debugobj_deactivate(skb);

			local_irq_restore(flags);
			preempt_enable();
			return true;
		}
		/* We still have a full spare because the global is also full */
		spin_unlock(&glob_recycler.lock);
	} else {
		/* We have room in the spare list; enqueue to spare list */
		ln = skb_peek(h);
		/* Recalculate the sum for peek of list as next and prev
		 * pointers of skb->next will be updated in __skb_queue_head
		 */
		skbuff_debugobj_sum_validate(ln);
		__skb_queue_head(h, skb);
		skbuff_debugobj_deactivate(skb);
		skbuff_debugobj_sum_update(ln);
		local_irq_restore(flags);
		preempt_enable();
		return true;
	}
#endif

	local_irq_restore(flags);
	preempt_enable();

	return false;
}

static void skb_recycler_free_skb(struct sk_buff_head *list)
{
	struct sk_buff *skb = NULL;
	unsigned long flags;

	spin_lock_irqsave(&list->lock, flags);
	while ((skb = skb_peek(list)) != NULL) {
		skbuff_debugobj_activate(skb);
		__skb_unlink(skb, list);
		skb_release_data(skb);
		kfree_skbmem(skb);
	}
	spin_unlock_irqrestore(&list->lock, flags);
}

static int skb_cpu_callback(unsigned int ocpu)
{
	unsigned long oldcpu = (unsigned long)ocpu;

	skb_recycler_free_skb(&per_cpu(recycle_list, oldcpu));
#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	spin_lock(&glob_recycler.lock);
	skb_recycler_free_skb(&per_cpu(recycle_spare_list, oldcpu));
	spin_unlock(&glob_recycler.lock);
#endif

	return NOTIFY_DONE;
}

#ifdef CONFIG_SKB_RECYCLER_PREALLOC
static int __init skb_prealloc_init_list(void)
{
	int i;
	struct sk_buff *skb;

	for (i = 0; i < SKB_RECYCLE_MAX_PREALLOC_SKBS; i++) {
		skb = __alloc_skb(SKB_RECYCLE_MAX_SIZE + NET_SKB_PAD,
				  GFP_KERNEL, 0, NUMA_NO_NODE);
		if (unlikely(!skb))
			return -ENOMEM;

		skb_reserve(skb, NET_SKB_PAD);

		skb_recycler_consume(skb);
	}
	return 0;
}
#endif

/* procfs: count
 * Show skb counts
 */
static int proc_skb_count_show(struct seq_file *seq, void *v)
{
	int cpu;
	int len;
	int total;
#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	unsigned int i;
	unsigned long flags;
#endif

	total = 0;

	for_each_online_cpu(cpu) {
		len = skb_queue_len(&per_cpu(recycle_list, cpu));
		seq_printf(seq, "recycle_list[%d]: %d\n", cpu, len);
		total += len;
	}

#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	for_each_online_cpu(cpu) {
		len = skb_queue_len(&per_cpu(recycle_spare_list, cpu));
		seq_printf(seq, "recycle_spare_list[%d]: %d\n", cpu, len);
		total += len;
	}

	for (i = 0; i < SKB_RECYCLE_MAX_SHARED_POOLS; i++) {
		spin_lock_irqsave(&glob_recycler.lock, flags);
		len = skb_queue_len(&glob_recycler.pool[i]);
		spin_unlock_irqrestore(&glob_recycler.lock, flags);
		seq_printf(seq, "global_list[%d]: %d\n", i, len);
		total += len;
	}
#endif

	seq_printf(seq, "total: %d\n", total);
	return 0;
}

static int proc_skb_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_skb_count_show, PDE_DATA(inode));
}

static const struct file_operations proc_skb_count_fops = {
	.owner   = THIS_MODULE,
	.open    = proc_skb_count_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
};

/* procfs: flush
 * Flush skbs
 */
static void skb_recycler_flush_task(struct work_struct *work)
{
	unsigned long flags;
	struct sk_buff_head *h;
	struct sk_buff_head tmp;

	skb_queue_head_init(&tmp);

	h = &get_cpu_var(recycle_list);
	local_irq_save(flags);
	skb_queue_splice_init(h, &tmp);
	local_irq_restore(flags);
	put_cpu_var(recycle_list);
	skb_recycler_free_skb(&tmp);

#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	h = &get_cpu_var(recycle_spare_list);
	local_irq_save(flags);
	skb_queue_splice_init(h, &tmp);
	local_irq_restore(flags);
	put_cpu_var(recycle_spare_list);
	skb_recycler_free_skb(&tmp);
#endif
}

static ssize_t proc_skb_flush_write(struct file *file,
				    const char __user *buf,
				    size_t count,
				    loff_t *ppos)
{
#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	unsigned int i;
	unsigned long flags;
#endif
	schedule_on_each_cpu(&skb_recycler_flush_task);

#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	spin_lock_irqsave(&glob_recycler.lock, flags);
	for (i = 0; i < SKB_RECYCLE_MAX_SHARED_POOLS; i++)
		skb_recycler_free_skb(&glob_recycler.pool[i]);
	glob_recycler.head = 0;
	glob_recycler.tail = 0;
	spin_unlock_irqrestore(&glob_recycler.lock, flags);
#endif
	return count;
}

static const struct file_operations proc_skb_flush_fops = {
	.owner   = THIS_MODULE,
	.write   = proc_skb_flush_write,
	.open    = simple_open,
	.llseek  = noop_llseek,
};

/* procfs: max_skbs
 * Show max skbs
 */
static int proc_skb_max_skbs_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, "%d\n", skb_recycle_max_skbs);
	return 0;
}

static int proc_skb_max_skbs_open(struct inode *inode, struct file *file)
{
	return single_open(file, proc_skb_max_skbs_show, PDE_DATA(inode));
}

static ssize_t proc_skb_max_skbs_write(struct file *file,
				       const char __user *buf,
				       size_t count,
				       loff_t *ppos)
{
	int ret;
	int max;
	char buffer[13];

	memset(buffer, 0, sizeof(buffer));
	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;
	if (copy_from_user(buffer, buf, count) != 0)
		return -EFAULT;
	ret = kstrtoint(strstrip(buffer), 10, &max);
	if (ret == 0 && max >= 0)
		skb_recycle_max_skbs = max;

	return count;
}

static const struct file_operations proc_skb_max_skbs_fops = {
	.owner   = THIS_MODULE,
	.open    = proc_skb_max_skbs_open,
	.read    = seq_read,
	.write   = proc_skb_max_skbs_write,
	.release = single_release,
};

#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
/* procfs: max_spare_skbs
 * Show max spare skbs
 */
static int proc_skb_max_spare_skbs_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, "%d\n", skb_recycle_spare_max_skbs);
	return 0;
}

static int proc_skb_max_spare_skbs_open(struct inode *inode, struct file *file)
{
	return single_open(file,
			   proc_skb_max_spare_skbs_show,
			   PDE_DATA(inode));
}

static ssize_t
proc_skb_max_spare_skbs_write(struct file *file,
			      const char __user *buf,
			      size_t count,
			      loff_t *ppos)
{
	int ret;
	int max;
	char buffer[13];

	memset(buffer, 0, sizeof(buffer));
	if (count > sizeof(buffer) - 1)
		count = sizeof(buffer) - 1;
	if (copy_from_user(buffer, buf, count) != 0)
		return -EFAULT;
	ret = kstrtoint(strstrip(buffer), 10, &max);
	if (ret == 0 && max >= 0)
		skb_recycle_spare_max_skbs = max;

	return count;
}

static const struct file_operations proc_skb_max_spare_skbs_fops = {
	.owner   = THIS_MODULE,
	.open    = proc_skb_max_spare_skbs_open,
	.read    = seq_read,
	.write   = proc_skb_max_spare_skbs_write,
	.release = single_release,
};
#endif /* CONFIG_SKB_RECYCLER_MULTI_CPU */

static void skb_recycler_init_procfs(void)
{
	proc_net_skbrecycler = proc_mkdir("skb_recycler", init_net.proc_net);
	if (!proc_net_skbrecycler) {
		pr_err("cannot create skb_recycle proc dir");
		return;
	}

	if (!proc_create("count",
			 S_IRUGO,
			 proc_net_skbrecycler,
			 &proc_skb_count_fops))
		pr_err("cannot create proc net skb_recycle held\n");

	if (!proc_create("flush",
			 S_IWUGO,
			 proc_net_skbrecycler,
			 &proc_skb_flush_fops))
		pr_err("cannot create proc net skb_recycle flush\n");

	if (!proc_create("max_skbs",
			 S_IRUGO | S_IWUGO,
			 proc_net_skbrecycler,
			 &proc_skb_max_skbs_fops))
		pr_err("cannot create proc net skb_recycle max_skbs\n");

#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	if (!proc_create("max_spare_skbs",
			 S_IRUGO | S_IWUGO,
			 proc_net_skbrecycler,
			 &proc_skb_max_spare_skbs_fops))
		pr_err("cannot create proc net skb_recycle max_spare_skbs\n");
#endif
}

void __init skb_recycler_init(void)
{
	int cpu;
#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	unsigned int i;
#endif

	for_each_possible_cpu(cpu) {
		skb_queue_head_init(&per_cpu(recycle_list, cpu));
	}

#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	for_each_possible_cpu(cpu) {
		skb_queue_head_init(&per_cpu(recycle_spare_list, cpu));
	}

	spin_lock_init(&glob_recycler.lock);

	for (i = 0; i < SKB_RECYCLE_MAX_SHARED_POOLS; i++)
		skb_queue_head_init(&glob_recycler.pool[i]);
	glob_recycler.head = 0;
	glob_recycler.tail = 0;
#endif

#ifdef CONFIG_SKB_RECYCLER_PREALLOC
	if (skb_prealloc_init_list())
		pr_err("Failed to preallocate SKBs for recycle list\n");
#endif
	cpuhp_setup_state_nocalls(CPUHP_SKB_RECYCLER_DEAD, "net/skbuff_recycler:dead:",NULL, skb_cpu_callback);
	skbuff_debugobj_register_callback();
	skb_recycler_init_procfs();
}

void skb_recycler_print_all_lists(void)
{

	unsigned long flags;
	int cpu;
#ifdef CONFIG_SKB_RECYCLER_MULTI_CPU
	int i;
	struct sk_buff_head *h;

	cpu = get_cpu();
	spin_lock_irqsave(&glob_recycler.lock, flags);
	for (i = 0; i < SKB_RECYCLE_MAX_SHARED_POOLS; i++)
		skbuff_debugobj_print_skb_list((&glob_recycler.pool[i])->next,
					       "Global Pool", -1);
	spin_unlock_irqrestore(&glob_recycler.lock, flags);

	preempt_disable();
	local_irq_save(flags);

	h = &per_cpu(recycle_spare_list, cpu);
	skbuff_debugobj_print_skb_list(h->next, "Recycle Spare", cpu);

	local_irq_restore(flags);
	preempt_enable();
#endif

	preempt_disable();
	local_irq_save(flags);
	h = &per_cpu(recycle_list, cpu);
	skbuff_debugobj_print_skb_list(h->next, "Recycle List", cpu);

	local_irq_restore(flags);
	preempt_enable();

}
