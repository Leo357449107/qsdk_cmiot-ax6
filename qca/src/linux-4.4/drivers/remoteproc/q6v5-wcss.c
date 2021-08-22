/* Copyright (c) 2016, 2019 The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include "remoteproc_internal.h"
#include <linux/soc/qcom/smem.h>
#include <linux/soc/qcom/smem_state.h>
#include <linux/qcom_scm.h>
#include <linux/elf.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <uapi/linux/major.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/timer.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/stringify.h>
#include "rproc_mdt_loader.h"

enum rproc_wcss_state {
	RPROC_Q6V5_STOPPED,
	RPROC_Q6V5_STARTING,
	RPROC_Q6V5_RUNNING,
	RPROC_Q6V5_STOPPING,
	RPROC_Q6V5_INVALID_STATE
};

static int debug_wcss;

/* registers to capture 0x0cd00000 to 0x0cd02028 */
#define Q6_REGISTER_SAVE_SIZE 0x202C

#define DEBUG_WCSS_BREAK_AT_START	1
#define DEBUG_WCSS_NO_RESTART		2
#define DEBUG_DUMP_Q6_REG		4

#define WCSS_CRASH_REASON_SMEM 421
#define WCNSS_PAS_ID		6
#define STOP_ACK_TIMEOUT_MS 5000

#define QDSP6SS_RST_EVB 0x10
#define QDSP6SS_RESET 0x14
#define QDSP6SS_DBG_CFG 0x18
#define QDSP6SS_XO_CBCR 0x38
#define QDSP6SS_MEM_PWR_CTL 0xb0
#define QDSP6SS_SLEEP_CBCR 0x3C
#define QDSP6SS_BHS_STATUS 0x78
#define TCSR_GLOBAL_CFG0 0x0
#define TCSR_GLOBAL_CFG1 0x4

#define QDSP6SS_GFMUX_CTL 0x20
#define QDSP6SS_PWR_CTL 0x30
#define TCSR_HALTREQ 0x0
#define TCSR_HALTACK 0x4
#define TCSR_Q6_HALTREQ 0x0
#define TCSR_Q6_HALTACK 0x4
#define SSCAON_CONFIG 0x8
#define SSCAON_STATUS 0xc
#define GCC_WCSS_BCR 0x0
#define GCC_WCSSAON_RESET 0x10
#define GCC_WCSS_Q6_BCR 0x100
#define GCC_MISC_RESET_ADDR 0x8
#define QDSP6SS_BOOT_CORE_START 0x400
#define HALTACK BIT(0)
#define BHS_EN_REST_ACK BIT(0)

#define EM_QDSP6 164

#define DEFAULT_IMG_ADDR 0x4b000000

struct q6v5_rtable {
	struct resource_table rtable;
	struct fw_rsc_hdr last_hdr;
};

static struct q6v5_rtable q6v5_rtable = {
	.rtable = {
		.ver = 1,
		.num = 0,
	},
	.last_hdr = {
		.type = RSC_LAST,
	},
};

struct q6v5_rproc_pdata {
	void __iomem *q6_base;
	void __iomem *tcsr_q6_base;
	void __iomem *tcsr_base;
	void __iomem *mpm_base;
	void __iomem *gcc_bcr_base;
	void __iomem *gcc_misc_base;
	void __iomem *tcsr_global_base;
	void __iomem *tcsr_q6_boot_trig;
	struct rproc *rproc;
	struct subsys_device *subsys;
	struct subsys_desc subsys_desc;
	struct completion stop_done;
	struct completion err_ready;
	int err_ready_irq;
	struct qcom_smem_state *state;
	unsigned stop_bit;
	unsigned shutdown_bit;
	atomic_t running;
	int emulation;
	int secure;
	int spurios_irqs;
	int stop_retry_count;
	void *reg_save_buffer;
	unsigned int img_addr;
	u32 reset_cmd_id;
};

static struct q6v5_rproc_pdata *q6v5_rproc_pdata;
static struct rproc_ops ipq60xx_q6v5_rproc_ops;
static struct rproc_ops ipq50xx_q6v5_rproc_ops;

#define subsys_to_pdata(d) container_of(d, struct q6v5_rproc_pdata, subsys_desc)

#if defined(CONFIG_IPQ_SUBSYSTEM_DUMP)

#define	OPEN_TIMEOUT	5000
#define	DUMP_TIMEOUT	10000
#define	NUM_WCSS_CLKS	ARRAY_SIZE(wcss_clk_names)

static struct timer_list dump_timeout;
static struct completion dump_complete;

static struct timer_list open_timeout;
static struct completion open_complete;
static atomic_t open_timedout;

static const struct file_operations q6_dump_ops;
static struct class *dump_class;

struct dump_file_private {
	int ehdr_remaining_bytes;
	struct list_head dump_segments;
	Elf32_Ehdr *ehdr;
	struct task_struct *pdesc;
};

struct dump_segment {
	struct list_head node;
	phys_addr_t addr;
	size_t size;
	loff_t offset;
};

struct dumpdev {
	const char *name;
	const struct file_operations *fops;
	fmode_t fmode;
	struct list_head dump_segments;
} q6dump = {"q6mem", &q6_dump_ops, FMODE_UNSIGNED_OFFSET | FMODE_EXCL};

static const char *wcss_clk_names[] = {"wcss_axi_m_clk",
							"sys_noc_wcss_ahb_clk",
							"q6_axim_clk",
							"q6ss_atbm_clk",
							"q6ss_pclkdbg_clk",
							"q6_tsctr_1to2_clk",
							"wcss_core_tbu_clk",
							"wcss_q6_tbu_clk",
							"gcc_q6_ahb_clk"
							};

static struct clk *wcss_clks[NUM_WCSS_CLKS];

static void ipq60xx_wcss_clks_disable(struct device *dev, int count)
{
	int i;

	for (i = 0; i < count; i++) {
		clk_disable_unprepare(wcss_clks[i]);
		devm_clk_put(dev, wcss_clks[i]);
	}
}

static int ipq60xx_wcss_clks_prepare_enable(struct device *dev)
{
	int i, ret;

	for (i = 0; i < NUM_WCSS_CLKS; i++) {
		wcss_clks[i] = devm_clk_get(dev, wcss_clk_names[i]);
		if (IS_ERR(wcss_clks[i])) {
			pr_err("%s unable to get clk %s\n",
					__func__, wcss_clk_names[i]);
			ret = PTR_ERR(wcss_clks[i]);
			goto disable_clk;
		}
		ret = clk_prepare_enable(wcss_clks[i]);
		if (ret) {
			pr_err("%s unable to enable clk %s\n",
					__func__, wcss_clk_names[i]);
			goto put_disable_clk;
		}
	}

	return 0;

put_disable_clk:
	devm_clk_put(dev, wcss_clks[i]);
disable_clk:
	ipq60xx_wcss_clks_disable(dev, i);
	return ret;
}

static void open_timeout_func(unsigned long data)
{
	atomic_set(&open_timedout, 1);
	complete(&open_complete);
	pr_err("open time Out: Q6 crash dump collection failed\n");
}

static void dump_timeout_func(unsigned long data)
{
	struct dump_file_private *dfp = (struct dump_file_private *)data;

	pr_err("Time Out: Q6 crash dump collection failed\n");

	dump_timeout.data = -ETIMEDOUT;
	send_sig(SIGKILL, dfp->pdesc, 0);
}

static int q6_dump_open(struct inode *inode, struct file *file)
{
	struct dump_file_private *dfp = NULL;
	struct dump_segment *segment, *tmp;
	int nsegments = 0;
	size_t elfcore_hdrsize, p_off;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;

	del_timer_sync(&open_timeout);

	if (atomic_read(&open_timedout) == 1)
		return -ENODEV;

	if (list_empty(&q6dump.dump_segments))
		return -ENODEV;

	file->f_mode |= q6dump.fmode;

	dfp = kzalloc(sizeof(struct dump_file_private), GFP_KERNEL);
	if (dfp == NULL) {
		pr_err("%s:\tCan not allocate memory for private structure\n",
				__func__);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&dfp->dump_segments);
	list_for_each_entry(segment, &q6dump.dump_segments, node) {
		struct dump_segment *s;

		s = kzalloc(sizeof(*s), GFP_KERNEL);
		if (!s)
			goto err;
		s->addr = segment->addr;
		s->size = segment->size;
		s->offset = segment->offset;
		list_add_tail(&s->node, &dfp->dump_segments);

		nsegments++;
	}

	elfcore_hdrsize = sizeof(*ehdr) + sizeof(*phdr) * nsegments;
	ehdr = kzalloc(elfcore_hdrsize, GFP_KERNEL);
	if (ehdr == NULL)
		goto err;

	memcpy(ehdr->e_ident, ELFMAG, SELFMAG);
	ehdr->e_ident[EI_CLASS] = ELFCLASS32;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
	ehdr->e_type = ET_CORE;
	ehdr->e_machine = EM_QDSP6;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_phoff = sizeof(*ehdr);
	ehdr->e_ehsize = sizeof(*ehdr);
	ehdr->e_phentsize = sizeof(*phdr);
	ehdr->e_phnum = nsegments;

	/* There are 'nsegments' of phdr in the elf header */
	phdr = (void *)ehdr + ehdr->e_phoff;
	memset(phdr, 0, sizeof(*phdr) * nsegments);

	p_off = elfcore_hdrsize;
	list_for_each_entry(segment, &q6dump.dump_segments, node) {
		phdr->p_type = PT_LOAD;
		phdr->p_offset = p_off;
		phdr->p_vaddr = phdr->p_paddr = segment->addr;
		phdr->p_filesz = phdr->p_memsz = segment->size;
		phdr->p_flags = PF_R | PF_W | PF_X;

		p_off += phdr->p_filesz;

		phdr++;
	}

	dfp->ehdr = ehdr;
	dfp->ehdr_remaining_bytes = elfcore_hdrsize;
	dfp->pdesc = current;

	file->private_data = dfp;

	dump_timeout.data = (unsigned long)dfp;

	/* This takes care of the user space app stalls during delayed read. */
	init_completion(&dump_complete);

	setup_timer(&dump_timeout, dump_timeout_func, (unsigned long)dfp);
	mod_timer(&dump_timeout, jiffies + msecs_to_jiffies(DUMP_TIMEOUT));

	complete(&open_complete);

	return 0;

err:
	list_for_each_entry_safe(segment, tmp, &dfp->dump_segments, node) {
		list_del(&segment->node);
		kfree(segment);
	}

	kfree(dfp);

	return -ENOMEM;
}

static int q6_dump_release(struct inode *inode, struct file *file)
{
	int dump_minor =  iminor(inode);
	int dump_major = imajor(inode);

	struct dump_segment *segment, *tmp;

	struct dump_file_private *dfp = (struct dump_file_private *)
		file->private_data;

	list_for_each_entry_safe(segment, tmp, &dfp->dump_segments, node) {
		list_del(&segment->node);
		kfree(segment);
	}

	kfree(dfp->ehdr);

	kfree(dfp);

	device_destroy(dump_class, MKDEV(dump_major, dump_minor));

	class_destroy(dump_class);

	complete(&dump_complete);

	return 0;
}

static ssize_t q6_dump_read(struct file *file, char __user *buf, size_t count,
		loff_t *ppos)
{
	void *buffer = NULL;
	struct dump_file_private *dfp = (struct dump_file_private *)
		file->private_data;
	struct dump_segment *segment, *tmp;
	size_t copied = 0, to_copy = count;
	int segment_num = 0;

	if (dump_timeout.data == -ETIMEDOUT)
		return 0;

	mod_timer(&dump_timeout, jiffies + msecs_to_jiffies(DUMP_TIMEOUT));

	if (list_empty(&dfp->dump_segments))
		return 0;

	if (dfp->ehdr_remaining_bytes) {
		if (to_copy > dfp->ehdr_remaining_bytes)
			to_copy = dfp->ehdr_remaining_bytes;

		copy_to_user(buf, (char *)dfp->ehdr + *ppos, to_copy);
		buf += to_copy;
		dfp->ehdr_remaining_bytes -= to_copy;
		copied += to_copy;

		if (copied == to_copy) {
			*ppos += to_copy;
			return copied;
		}
	}

	list_for_each_entry_safe(segment, tmp, &dfp->dump_segments, node) {
		size_t pending = 0;

		segment_num++;
		pending = segment->size - segment->offset;
		if (pending > to_copy)
			pending = to_copy;

		buffer = ioremap(segment->addr + segment->offset, pending);
		if (!buffer) {
			pr_err("ioremap failed for segment %d, offset 0x%llx of size 0x%zx\n",
			       segment_num, segment->offset, pending);
			return -ENOMEM;
		}
		copy_to_user(buf, buffer, pending);
		iounmap(buffer);

		segment->offset += pending;
		buf += pending;
		copied += pending;
		to_copy -= pending;

		if (segment->offset == segment->size) {
			list_del(&segment->node);
			kfree(segment);
		}

		if (to_copy == 0)
			break;
	}

	*ppos += copied;
	return copied;
}

static ssize_t q6_dump_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	return 0;
}

static const struct file_operations q6_dump_ops = {
	.open		=	q6_dump_open,
	.read		=	q6_dump_read,
	.write		=	q6_dump_write,
	.release	=       q6_dump_release,
};

int crashdump_add_segment(phys_addr_t dump_addr, size_t dump_size)
{
	struct dump_segment *segment;

	segment = kzalloc(sizeof(*segment), GFP_KERNEL);
	if (!segment)
		return -ENOMEM;

	segment->addr = dump_addr;
	segment->size = dump_size;
	segment->offset = 0;

	list_add_tail(&segment->node, &q6dump.dump_segments);

	return 0;
}
EXPORT_SYMBOL(crashdump_add_segment);

static int add_segment(struct dumpdev *dumpdev, struct device_node *node)
{
	struct dump_segment segment = {0};
	int ret;

	ret = of_property_read_u32_index(node, "reg", 1, (u32 *)&segment.addr);
	if (ret) {
		pr_err("Could not retrieve reg property: %d\n", ret);
		goto fail;
	}

	ret = of_property_read_u32_index(node, "reg", 3, (u32 *)&segment.size);
	if (ret) {
		pr_err("Could not retrieve reg property: %d\n", ret);
		goto fail;
	}

	ret = crashdump_add_segment(segment.addr, segment.size);

fail:
	return ret;
}

static int crashdump_init(int check, const struct subsys_desc *desc)
{
	int ret = 0;
	int index = 0;
	int dump_major = 0;
	struct device *dump_dev = NULL;
	struct device_node *node = NULL, *np = NULL;

	init_completion(&open_complete);
	atomic_set(&open_timedout, 0);

	dump_major = register_chrdev(UNNAMED_MAJOR, "dump", &q6_dump_ops);
	if (dump_major < 0) {
		ret = dump_major;
		pr_err("Unable to allocate a major number err = %d", ret);
		goto reg_failed;
	}

	dump_class = class_create(THIS_MODULE, "dump");
	if (IS_ERR(dump_class)) {
		ret = PTR_ERR(dump_class);
		goto class_failed;
	}

	dump_dev = device_create(dump_class, NULL, MKDEV(dump_major, 0), NULL,
			q6dump.name);
	if (IS_ERR(dump_dev)) {
		ret = PTR_ERR(dump_dev);
		pr_err("Unable to create a device err = %d", ret);
		goto device_failed;
	}

	INIT_LIST_HEAD(&q6dump.dump_segments);

	np = of_find_node_by_name(NULL, "q6v5_wcss");
	while (1) {
		node = of_parse_phandle(np, "memory-region", index);
		if (node == NULL)
			break;

		ret = add_segment(&q6dump, node);
		of_node_put(node);
		if (ret != 0)
			break;

		index++;
	}
	of_node_put(np);

	/* This avoids race condition between the scheduled timer and the opened
	 * file discriptor during delay in user space app execution.
	 */
	setup_timer(&open_timeout, open_timeout_func, 0);

	mod_timer(&open_timeout, jiffies + msecs_to_jiffies(OPEN_TIMEOUT));

	wait_for_completion(&open_complete);

	if (atomic_read(&open_timedout) == 1) {
		ret = -ETIMEDOUT;
		goto dump_dev_failed;
	}

	wait_for_completion(&dump_complete);

	if (dump_timeout.data == -ETIMEDOUT) {
		ret = dump_timeout.data;
		dump_timeout.data = 0;
	}

	del_timer_sync(&dump_timeout);
	return ret;

dump_dev_failed:
	device_destroy(dump_class, MKDEV(dump_major, 0));
device_failed:
	class_destroy(dump_class);
class_failed:
	unregister_chrdev(dump_major, "dump");
reg_failed:
	return ret;
}
#else
static inline int crashdump_init(int check, const struct subsys_desc *desc)
{
	return 0;
}
#endif /* CONFIG_IPQ_SUBSYSTEM_DUMP */

static struct resource_table *q6v5_find_loaded_rsc_table(struct rproc *rproc,
	const struct firmware *fw)
{
	q6v5_rtable.rtable.offset[0] = sizeof(struct resource_table);
	return &(q6v5_rtable.rtable);
}

static void halt_q6(struct q6v5_rproc_pdata *pdata)
{
	unsigned int nretry = 0;
	unsigned long val = 0;

	/* Halt Q6 bus interface - 9*/
	val = readl(pdata->tcsr_q6_base + TCSR_Q6_HALTREQ);
	val |= BIT(0);
	writel(val, pdata->tcsr_q6_base + TCSR_Q6_HALTREQ);

	nretry = 0;
	while (1) {
		val = readl(pdata->tcsr_q6_base + TCSR_Q6_HALTACK);
		if (val & HALTACK)
			break;
		mdelay(1);
		nretry++;
		if (nretry >= pdata->stop_retry_count) {
			pr_err("can't get TCSR Q6 haltACK\n");
			break;
		}
	}
}

static void halt_clr(struct q6v5_rproc_pdata *pdata)
{
	unsigned long val = 0;

	/* HALT CLEAR - 18 */
	val = readl(pdata->tcsr_q6_base + TCSR_Q6_HALTREQ);
	val &= ~(BIT(0));
	writel(val, pdata->tcsr_q6_base + TCSR_Q6_HALTREQ);
}

static void wcss_powerdown(struct q6v5_rproc_pdata *pdata)
{
	unsigned int nretry = 0;
	unsigned long val = 0;

	/* Assert WCSS/Q6 HALTREQ - 1 */
	nretry = 0;
	val = readl(pdata->tcsr_base + TCSR_HALTREQ);
	val |= BIT(0);
	writel(val, pdata->tcsr_base + TCSR_HALTREQ);
	while (1) {
		val = readl(pdata->tcsr_base + TCSR_HALTACK);
		if (val & HALTACK)
			break;
		mdelay(1);
		nretry++;
		if (nretry >= pdata->stop_retry_count) {
			pr_warn("can't get TCSR haltACK\n");
			break;
		}
	}

	/* Check HALTACK */
	/* Set MPM_SSCAON_CONFIG 13 - 2 */
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val |= BIT(13);
	writel(val, pdata->mpm_base + SSCAON_CONFIG);

	/* Set MPM_SSCAON_CONFIG 15 - 3 */
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val |= BIT(15);
	val &= ~(BIT(16));
	val &= ~(BIT(17));
	val &= ~(BIT(18));
	writel(val, pdata->mpm_base + SSCAON_CONFIG);

	/* Set MPM_SSCAON_CONFIG 1 - 4 */
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val |= BIT(1);
	writel(val, pdata->mpm_base + SSCAON_CONFIG);

	/* wait for SSCAON_STATUS to be 0x400 - 5 */
	nretry = 0;
	while (1) {
		val = readl(pdata->mpm_base + SSCAON_STATUS);
		/* ignore bits 16 to 31 */
		val &= 0xffff;
		if (val == BIT(10))
			break;
		nretry++;
		mdelay(1);
		if (nretry == pdata->stop_retry_count) {
			pr_warn("can't get SSCAON_STATUS\n");
			break;
		}
	}

	/* Enable Q6/WCSS BLOCK ARES - 6 */
	val = readl(pdata->gcc_misc_base + GCC_WCSSAON_RESET);
	val |= BIT(0);
	writel(val, pdata->gcc_misc_base + GCC_WCSSAON_RESET);
	mdelay(1);

	/* Enable MPM_WCSSAON_CONFIG 13 - 7 */
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val &= (~(BIT(13)));
	writel(val, pdata->mpm_base + SSCAON_CONFIG);

	/* Enable A2AB/ACMT/ECHAB ARES - 8 */
	val = readl(pdata->tcsr_base + TCSR_HALTREQ);
	val &= ~(BIT(0));
	writel(val, pdata->tcsr_base + TCSR_HALTREQ);

	return;
}

static void ipq50xx_q6_powerdown(struct q6v5_rproc_pdata *pdata)
{
	unsigned long val = 0;

	/*halt q6 bus*/
	halt_q6(pdata);

	/* Enable Q6 Block reset - 19 */
	val = readl(pdata->gcc_bcr_base + 0x4);
	val |= BIT(0);
	writel(val, pdata->gcc_bcr_base + 0x4);
	mdelay(2);

	/* Enable Q6 Axix Ares */
	val = readl(pdata->gcc_misc_base + 0x158);
	val |= BIT(0);
	writel(val, pdata->gcc_misc_base + 0x158);

	/* Disable Q6 Core clock - 10 */
	val = readl(pdata->q6_base + QDSP6SS_GFMUX_CTL);
	val &= (~(BIT(0)));
	writel(val, pdata->q6_base + QDSP6SS_GFMUX_CTL);

	/* Disable Q6 Block reset - 19 */
	val = readl(pdata->gcc_bcr_base + 0x4);
	val &= (~(BIT(0)));
	writel(val, pdata->gcc_bcr_base + 0x4);
	mdelay(2);

	/* Disbale Boot FSM */
	writel(0x0, pdata->tcsr_q6_boot_trig);

	/*halt clear*/
	halt_clr(pdata);
}

static void q6_powerdown(struct q6v5_rproc_pdata *pdata)
{
	int i = 0;
	unsigned int nretry = 0;
	unsigned long val = 0;

	/* To retain power domain after q6 powerdown */
	writel(0x1, pdata->q6_base + QDSP6SS_DBG_CFG);

	/*halt q6 bus*/
	halt_q6(pdata);

	/* Disable Q6 Core clock - 10 */
	val = readl(pdata->q6_base + QDSP6SS_GFMUX_CTL);
	val &= (~(BIT(1)));
	writel(val, pdata->q6_base + QDSP6SS_GFMUX_CTL);

	/* Clamp I/O - 11 */
	val = readl(pdata->q6_base + QDSP6SS_PWR_CTL);
	val |= BIT(20);
	writel(val, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* Clamp WL - 12 */
	val = readl(pdata->q6_base + QDSP6SS_PWR_CTL);
	val |= BIT(21);
	writel(val, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* Clear Erase standby - 13 */
	val = readl(pdata->q6_base + QDSP6SS_PWR_CTL);
	val &= (~(BIT(18)));
	writel(val, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* Clear Sleep RTN - 14 */
	val = readl(pdata->q6_base + QDSP6SS_PWR_CTL);
	val &= (~(BIT(19)));
	writel(val, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* turn off QDSP6 memory foot/head switch one
	 * bank at a time - 15
	 */
	for (i = 0; i < 20; i++) {
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val &= (~(BIT(i)));
		writel(val, pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		mdelay(1);
	}

	/* Assert QMC memory RTN - 16 */
	val = readl(pdata->q6_base + QDSP6SS_PWR_CTL);
	val |= BIT(22);
	writel(val, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* Turn off BHS - 17 */
	val = readl(pdata->q6_base + QDSP6SS_PWR_CTL);
	val &= (~(BIT(24)));
	writel(val, pdata->q6_base + QDSP6SS_PWR_CTL);
	udelay(1);

	/* Wait till BHS Reset is done */
	nretry = 0;
	while (1) {
		val = readl(pdata->q6_base + QDSP6SS_BHS_STATUS);
		if (!(val & BHS_EN_REST_ACK))
			break;
		mdelay(1);
		nretry++;
		if (nretry >= pdata->stop_retry_count) {
			pr_err("BHS_STATUS not OFF\n");
			break;
		}
	}

	/* De-assert WCSS/Q6 HALTREQ - 8 */
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val |= BIT(0);
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_BCR);
	mdelay(1);

	/* Enable Q6 Block reset - 19 */
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);
	val |= BIT(0);
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);
	mdelay(2);

	/*halt clear*/
	halt_clr(pdata);

	return;
}

static void save_wcss_regs(struct q6v5_rproc_pdata *pdata)
{
	int i = 0;
	unsigned int *buffer = NULL;

	if (!(debug_wcss & DEBUG_DUMP_Q6_REG))
		return;

	buffer = (unsigned int *)(q6v5_rproc_pdata->reg_save_buffer);

	for (i = 0; i < Q6_REGISTER_SAVE_SIZE; i += 0x4) {
		*buffer = readl(q6v5_rproc_pdata->q6_base + i);
		buffer++;
	}
}

static int q6_rproc_stop(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int ret = 0;
	unsigned int state;

	state = atomic_read(&pdata->running);

	if ((state == RPROC_Q6V5_RUNNING) || (state == RPROC_Q6V5_STARTING)) {
		atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STOPPING);
		if (pdata->secure) {
			ret = qcom_scm_pas_shutdown(WCNSS_PAS_ID);
			if (ret)
				dev_err(dev, "failed to shutdown %d\n", ret);
			atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STOPPED);
			return ret;
		}

		/* Non secure */
		/* Print registers for debug of powerdown issue */
		save_wcss_regs(pdata);

		if (of_device_get_match_data(&pdev->dev) ==
					&ipq50xx_q6v5_rproc_ops) {
			ipq50xx_q6_powerdown(pdata);
		} else {
			/* WCSS powerdown */
			wcss_powerdown(pdata);

			/* Q6 Power down */
			q6_powerdown(pdata);
		}

		/*Disable clocks*/
		if (of_device_get_match_data(&pdev->dev) ==
					&ipq60xx_q6v5_rproc_ops)
			ipq60xx_wcss_clks_disable(dev, NUM_WCSS_CLKS);

		atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STOPPED);
	}

	return ret;
}

static int wait_for_err_ready(struct q6v5_rproc_pdata *pdata)
{
	int ret;

wait_again:
	ret = wait_for_completion_timeout(&pdata->err_ready,
					  msecs_to_jiffies(10000));
	if (!ret) {
		if ((debug_wcss & DEBUG_WCSS_BREAK_AT_START))
			goto wait_again;
		pr_err("[%s]: Error ready timed out\n",
				pdata->subsys_desc.name);
		return -ETIMEDOUT;
	}

	return 0;
}

static int ipq50xx_q6_rproc_emu_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int ret = 0;

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STARTING);

	/*Debugging*/
	if (debug_wcss & DEBUG_WCSS_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Boot adderss */
	writel(pdata->img_addr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);

	/* Trigger Boot FSM, to bring core out of rst */
	writel(0x1, pdata->tcsr_q6_boot_trig);

	/* Retain debugger state during next QDSP6 reset */
	if (!(debug_wcss & DEBUG_WCSS_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Boot core start */
	writel(0x1, pdata->q6_base + QDSP6SS_BOOT_CORE_START);

	do {
		ret = wait_for_err_ready(pdata);
	} while (ret);

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_RUNNING);
	pr_emerg("Q6 Emulation reset out is done\n");

	return ret;
}

static int ipq60xx_q6_rproc_emu_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int temp = 19;
	unsigned long val = 0;
	int ret = 0;
	unsigned int nretry = 0;

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STARTING);
	/* Release Q6 and WCSS reset */
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);

	/* Last but two step in script */
	val = readl(pdata->gcc_misc_base + 0x10);

	/*set CFG[18:15]=1* and clear CFG[1]=0*/
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val &= 0xfff8fffd;
	val |= (1<<15);
	writel(val, pdata->mpm_base + SSCAON_CONFIG);

	/* Set MPM_SSCAON_CONFIG 13 - 2 */
	/* Last but one step in script */
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val |= 0x1;
	writel(val, pdata->mpm_base + SSCAON_CONFIG);
	mdelay(100);

	/* This is for Lithium configuration - clock gating */
	/* Last step in script */
	val = readl(pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);
	val |= 0x14;
	writel(val, pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);

	/* This is for Lithium configuration - bus arbitration */
	val = readl(pdata->tcsr_global_base + TCSR_GLOBAL_CFG1);
	val = 0;
	writel(val, pdata->tcsr_global_base + TCSR_GLOBAL_CFG1);

	writel(pdata->img_addr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);
	writel(0x1, pdata->q6_base + QDSP6SS_XO_CBCR);
	writel(0x1700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	mdelay(10);
	/* Put LDO in bypass mode */
	writel(0x3700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert QDSP6 complier memory clamp */
	writel(0x3300000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert memory peripheral sleep and L2 memory standby */
	writel(0x33c0000, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* turn on QDSP6 memory foot/head switch one bank at a time */
	while  (temp >= 0) {
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = val | 1 << temp;
		writel(val, pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		mdelay(10);
		temp -= 1;
	}

	/* Remove the QDSP6 core memory word line clamp */
	writel(0x31FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* Remove QDSP6 I/O clamp */
	writel(0x30FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);

	if (debug_wcss & DEBUG_WCSS_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Bring Q6 out of reset and stop the core */
	writel(0x5, pdata->q6_base + QDSP6SS_RESET);
	mdelay(100);
	/* Retain debugger state during next QDSP6 reset */
	if (!(debug_wcss & DEBUG_WCSS_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Turn on the QDSP6 core clock */
	writel(0x102, pdata->q6_base + QDSP6SS_GFMUX_CTL);
	/* Enable the core to run */
	writel(0x4, pdata->q6_base + QDSP6SS_RESET);

	nretry = 0;
	while (1) {
		val = readl(pdata->mpm_base + SSCAON_STATUS);
		if (0x10 == (val & 0xffff))
			break;
		mdelay(1);
		nretry++;
		if (nretry >= 10000) {
			pr_err("[%s]: Boot Error, SSCAON=0x%08lX\n",
					pdata->subsys_desc.name, val);
			break;
		}
	}

	do {
		ret = wait_for_err_ready(pdata);
	} while (ret);

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_RUNNING);
	pr_emerg("Q6 Emulation reset out is done\n");

	return ret;
}

static int ipq807x_q6_rproc_emu_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int temp = 19;
	unsigned long val = 0;
	int ret = 0;

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STARTING);
	/* Release Q6 and WCSS reset */
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);

	/* Last but two step in script */
	val = readl(pdata->gcc_misc_base + 0x10);

	/* Set MPM_SSCAON_CONFIG 13 - 2 */
	/* Last but one step in script */
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val |= 0x1;
	writel(val, pdata->mpm_base + SSCAON_CONFIG);
	mdelay(100);

	/* This is for Lithium configuration - clock gating */
	/* Last step in script */
	val = readl(pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);
	val |= 0x14;
	writel(val, pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);

	writel(pdata->img_addr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);
	writel(0x1, pdata->q6_base + QDSP6SS_XO_CBCR);
	writel(0x1700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	mdelay(10);
	/* Put LDO in bypass mode */
	writel(0x3700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert QDSP6 complier memory clamp */
	writel(0x3300000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert memory peripheral sleep and L2 memory standby */
	writel(0x33c0000, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* turn on QDSP6 memory foot/head switch one bank at a time */
	while  (temp >= 0) {
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = val | 1 << temp;
		writel(val, pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		mdelay(10);
		temp -= 1;
	}

	/* Remove the QDSP6 core memory word line clamp */
	writel(0x31FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* Remove QDSP6 I/O clamp */
	writel(0x30FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);

	if (debug_wcss & DEBUG_WCSS_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Bring Q6 out of reset and stop the core */
	writel(0x5, pdata->q6_base + QDSP6SS_RESET);
	mdelay(100);
	/* Retain debugger state during next QDSP6 reset */
	if (!(debug_wcss & DEBUG_WCSS_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Turn on the QDSP6 core clock */
	writel(0x102, pdata->q6_base + QDSP6SS_GFMUX_CTL);
	/* Enable the core to run */
	writel(0x4, pdata->q6_base + QDSP6SS_RESET);

	do {
		ret = wait_for_err_ready(pdata);
	} while (ret);

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_RUNNING);
	pr_emerg("Q6 Emulation reset out is done\n");

	return ret;
}

static int ipq60xx_q6_rproc_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int temp = 19;
	unsigned long val = 0;
	unsigned int nretry = 0;
	int ret = 0;

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STARTING);
	if (pdata->secure) {
		ret = qcom_scm_pas_auth_and_reset(WCNSS_PAS_ID,
			(debug_wcss & DEBUG_WCSS_BREAK_AT_START),
						pdata->reset_cmd_id);
		if (ret) {
			dev_err(dev, "q6-wcss reset failed\n");
			return ret;
		}
		goto skip_reset;
	}

	/* Release Q6 and WCSS reset */
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_BCR);

	val = readl(pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);

	val = readl(pdata->gcc_misc_base + GCC_WCSSAON_RESET);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_misc_base + GCC_WCSSAON_RESET);
	mdelay(1);

	if(ipq60xx_wcss_clks_prepare_enable(&pdev->dev))
		goto skip_reset;

	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val |= 0x1;
	writel(val, pdata->mpm_base + SSCAON_CONFIG);
	mdelay(1);
	/*set CFG[18:15]=1* and clear CFG[1]=0*/
	val = readl(pdata->mpm_base + SSCAON_CONFIG);
	val &= 0xfff8fffd;
	val |= (1<<15);
	writel(val, pdata->mpm_base + SSCAON_CONFIG);
	mdelay(1);

	/* This is for Lithium configuration - clock gating */
	val = readl(pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);
	val |= 0x14;
	writel(val, pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);

	/* This is for Lithium configuration - bus arbitration */
	val = readl(pdata->tcsr_global_base + TCSR_GLOBAL_CFG1);
	val = 0;
	writel(val, pdata->tcsr_global_base + TCSR_GLOBAL_CFG1);

	/* Write bootaddr to EVB so that Q6WCSS will jump there after reset */
	writel(rproc->bootaddr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);
	/* Turn on XO clock. It is required for BHS and memory operation */
	writel(0x1, pdata->q6_base + QDSP6SS_XO_CBCR);
	/* Turn on BHS */
	writel(0x1700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	udelay(1);

	/* Wait till BHS Reset is done */
	while (1) {
		val = readl(pdata->q6_base + QDSP6SS_BHS_STATUS);
		if (val & BHS_EN_REST_ACK)
			break;
		mdelay(1);
		nretry++;
		if (nretry >= 10) {
			pr_err("BHS_STATUS not ON\n");
			break;
		}
	}

	/* Put LDO in bypass mode */
	writel(0x3700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert QDSP6 complier memory clamp */
	writel(0x3300000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert memory peripheral sleep and L2 memory standby */
	writel(0x33c0000, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* turn on QDSP6 memory foot/head switch one bank at a time */
	while  (temp >= 0) {
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = val | 1 << temp;
		writel(val, pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		mdelay(10);
		temp -= 1;
	}
	/* Remove the QDSP6 core memory word line clamp */
	writel(0x31FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* Remove QDSP6 I/O clamp */
	writel(0x30FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);

	if (debug_wcss & DEBUG_WCSS_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Bring Q6 out of reset and stop the core */
	writel(0x5, pdata->q6_base + QDSP6SS_RESET);

	/* Retain debugger state during next QDSP6 reset */
	if (!(debug_wcss & DEBUG_WCSS_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Turn on the QDSP6 core clock */
	writel(0x102, pdata->q6_base + QDSP6SS_GFMUX_CTL);
	/* Enable the core to run */
	writel(0x4, pdata->q6_base + QDSP6SS_RESET);

	nretry = 0;
	while (1) {
		val = readl(pdata->mpm_base + SSCAON_STATUS);
		if (0x10 == (val & 0xffff))
			break;
		mdelay(1);
		nretry++;
		if (nretry >= 1000) {
			pr_err("[%s]: Boot Error, SSCAON=0x%08lX\n",
					pdata->subsys_desc.name, val);
			break;
		}
	}
skip_reset:

	ret = wait_for_err_ready(pdata);
	if (!ret)
		atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_RUNNING);

	return ret;
}

static int ipq50xx_q6_rproc_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int ret = 0;

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STARTING);
	if (pdata->secure) {
		ret = qcom_scm_pas_auth_and_reset(WCNSS_PAS_ID,
				(debug_wcss & DEBUG_WCSS_BREAK_AT_START),
				pdata->reset_cmd_id);
		if (ret) {
			dev_err(dev, "q6-wcss reset failed\n");
			return ret;
		}
		goto skip_reset;
	}

	if (debug_wcss & DEBUG_WCSS_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/*Boot adderss */
	writel(pdata->img_addr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);

	/* Trigger Boot FSM, to bring core out of rst */
	writel(0x1, pdata->tcsr_q6_boot_trig);

	/* Retain debugger state during next QDSP6 reset */
	if (!(debug_wcss & DEBUG_WCSS_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Boot core start */
	writel(0x1, pdata->q6_base + QDSP6SS_BOOT_CORE_START);

skip_reset:
	ret = wait_for_err_ready(pdata);
	if (!ret)
		atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_RUNNING);

	return ret;
}

static int ipq807x_q6_rproc_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int temp = 19;
	unsigned long val = 0;
	unsigned int nretry = 0;
	int ret = 0;

	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STARTING);
	if (pdata->secure) {
		ret = qcom_scm_pas_auth_and_reset(WCNSS_PAS_ID,
			(debug_wcss & DEBUG_WCSS_BREAK_AT_START),
						pdata->reset_cmd_id);
		if (ret) {
			dev_err(dev, "q6-wcss reset failed\n");
			return ret;
		}
		goto skip_reset;
	}

	/* Release Q6 and WCSS reset */
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_BCR);
	val = readl(pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_bcr_base + GCC_WCSS_Q6_BCR);

	val = readl(pdata->gcc_misc_base + GCC_WCSSAON_RESET);
	val &= ~(BIT(0));
	writel(val, pdata->gcc_misc_base + GCC_WCSSAON_RESET);
	mdelay(1);

	/* This is for Lithium configuration - clock gating */
	val = readl(pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);
	val |= 0x14;
	writel(val, pdata->tcsr_global_base + TCSR_GLOBAL_CFG0);

	/* This is for Lithium configuration - bus arbitration */
	val = readl(pdata->tcsr_global_base + TCSR_GLOBAL_CFG1);
	val = 0;
	writel(val, pdata->tcsr_global_base + TCSR_GLOBAL_CFG1);

	/* Write bootaddr to EVB so that Q6WCSS will jump there after reset */
	writel(rproc->bootaddr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);
	/* Turn on XO clock. It is required for BHS and memory operation */
	writel(0x1, pdata->q6_base + QDSP6SS_XO_CBCR);
	/* Turn on BHS */
	writel(0x1700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	udelay(1);

	/* Wait till BHS Reset is done */
	while (1) {
		val = readl(pdata->q6_base + QDSP6SS_BHS_STATUS);
		if (val & BHS_EN_REST_ACK)
			break;
		mdelay(1);
		nretry++;
		if (nretry >= 10) {
			pr_err("BHS_STATUS not ON\n");
			break;
		}
	}

	/* Put LDO in bypass mode */
	writel(0x3700000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert QDSP6 complier memory clamp */
	writel(0x3300000, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* De-assert memory peripheral sleep and L2 memory standby */
	writel(0x33c0000, pdata->q6_base + QDSP6SS_PWR_CTL);

	/* turn on QDSP6 memory foot/head switch one bank at a time */
	while  (temp >= 0) {
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = val | 1 << temp;
		writel(val, pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		val = readl(pdata->q6_base + QDSP6SS_MEM_PWR_CTL);
		mdelay(10);
		temp -= 1;
	}
	/* Remove the QDSP6 core memory word line clamp */
	writel(0x31FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);
	/* Remove QDSP6 I/O clamp */
	writel(0x30FFFFF, pdata->q6_base + QDSP6SS_PWR_CTL);

	if (debug_wcss & DEBUG_WCSS_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Bring Q6 out of reset and stop the core */
	writel(0x5, pdata->q6_base + QDSP6SS_RESET);

	/* Retain debugger state during next QDSP6 reset */
	if (!(debug_wcss & DEBUG_WCSS_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Turn on the QDSP6 core clock */
	writel(0x102, pdata->q6_base + QDSP6SS_GFMUX_CTL);
	/* Enable the core to run */
	writel(0x4, pdata->q6_base + QDSP6SS_RESET);

skip_reset:

	ret = wait_for_err_ready(pdata);
	if (!ret)
		atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_RUNNING);

	return ret;
}

static void *q6_da_to_va(struct rproc *rproc, u64 da, int len)
{
	unsigned long addr = (__force unsigned long)(da & 0xFFFFFFFF);

	return ioremap(addr, len);
}

static char *rproc_q6_state(enum rproc_wcss_state state)
{
	switch (state) {
		case RPROC_Q6V5_STOPPED:
			return __stringify(RPROC_Q6V5_STOPPED);
		case RPROC_Q6V5_STARTING:
			return __stringify(RPROC_Q6V5_STARTING);
		case RPROC_Q6V5_RUNNING:
			return __stringify(RPROC_Q6V5_RUNNING);
		case RPROC_Q6V5_STOPPING:
			return __stringify(RPROC_Q6V5_STOPPING);
		default:
			return __stringify(RPROC_Q6V5_INVALID_STATE);
	}
}

static irqreturn_t wcss_err_ready_intr_handler(int irq, void *dev_id)
{
	struct q6v5_rproc_pdata *pdata = dev_id;
	unsigned int state;

	state = atomic_read(&pdata->running);

	if (state != RPROC_Q6V5_STARTING) {
		pdata->spurios_irqs++;
		pr_emerg("ERROR: smp2p %s in wrong state %s (%d)\n",
				__func__,
				rproc_q6_state(state),
				pdata->spurios_irqs);
		return IRQ_HANDLED;
	}

	pr_info("Subsystem error monitoring/handling services are up\n");

	complete(&pdata->err_ready);
	return IRQ_HANDLED;
}

static irqreturn_t wcss_err_fatal_intr_handler(int irq, void *dev_id)
{
	struct q6v5_rproc_pdata *pdata = subsys_to_pdata(dev_id);
	char *msg;
	size_t len;
	unsigned int state;

	state = atomic_read(&pdata->running);
	if ((state != RPROC_Q6V5_RUNNING) && (state != RPROC_Q6V5_STOPPING)) {
		pdata->spurios_irqs++;
		pr_emerg("ERROR: smp2p %s in wrong state %s (%d)\n",
				__func__,
				rproc_q6_state(state),
				pdata->spurios_irqs);
		return IRQ_HANDLED;
	}

	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, WCSS_CRASH_REASON_SMEM, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		pr_err("Fatal error received from wcss software!: %s\n", msg);
	else
		pr_err("Fatal error received no message!\n");

	if (debug_wcss & DEBUG_WCSS_NO_RESTART) {
		pr_info("WCSS Restart is disabled, Ignoring fatal error.\n");
		return IRQ_HANDLED;
	}

	subsys_set_crash_status(pdata->subsys, CRASH_STATUS_ERR_FATAL);
	subsystem_restart_dev(pdata->subsys);
	return IRQ_HANDLED;
}

static irqreturn_t wcss_stop_ack_intr_handler(int irq, void *dev_id)
{
	struct q6v5_rproc_pdata *pdata = subsys_to_pdata(dev_id);
	unsigned int state;

	state = atomic_read(&pdata->running);
	if (state != RPROC_Q6V5_RUNNING) {
		pdata->spurios_irqs++;
		pr_emerg("ERROR: smp2p %s in wrong state %s (%d)\n",
				__func__,
				rproc_q6_state(state),
				pdata->spurios_irqs);
		return IRQ_HANDLED;
	}

	pr_info("Received stop ack interrupt from wcss\n");
	complete(&pdata->stop_done);
	return IRQ_HANDLED;
}

static irqreturn_t wcss_wdog_bite_intr_handler(int irq, void *dev_id)
{
	struct q6v5_rproc_pdata *pdata = subsys_to_pdata(dev_id);
	char *msg;
	size_t len;
	unsigned int state;

	state = atomic_read(&pdata->running);
	if (state != RPROC_Q6V5_RUNNING) {
		pdata->spurios_irqs++;
		pr_emerg("ERROR: smp2p %s in wrong state %s (%d)\n",
				__func__,
				rproc_q6_state(state),
				pdata->spurios_irqs);
		complete(&pdata->stop_done);
		return IRQ_HANDLED;
	}

	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, WCSS_CRASH_REASON_SMEM, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		pr_err("Watchdog bite received from wcss software!: %s\n", msg);
	else
		pr_err("Watchdog bit received no message!\n");

	if (debug_wcss & DEBUG_WCSS_NO_RESTART) {
		pr_info("WCSS Restart is disabled, Ignoring WDOG Bite.\n");
		return IRQ_HANDLED;
	}

	subsys_set_crash_status(pdata->subsys, CRASH_STATUS_WDOG_BITE);
	subsystem_restart_dev(pdata->subsys);

	return IRQ_HANDLED;
}


static int start_q6(const struct subsys_desc *subsys)
{
	struct q6v5_rproc_pdata *pdata = subsys_to_pdata(subsys);
	struct rproc *rproc = pdata->rproc;
	int ret = 0;

	reinit_completion(&pdata->stop_done);
	reinit_completion(&pdata->err_ready);
	pdata->subsys_desc.ramdump_disable = 0;

	if (pdata->emulation) {
		pr_emerg("q6v5: Emulation start, no smp2p messages\n");
		rproc->ops->start(rproc);
		return 0;
	}

	ret = rproc_boot(rproc);
	if (ret) {
		pr_err("couldn't boot q6v5: %d\n", ret);
	}

	return ret;
}

static int stop_q6(const struct subsys_desc *subsys, bool force_stop)
{
	struct q6v5_rproc_pdata *pdata = subsys_to_pdata(subsys);
	struct rproc *rproc = pdata->rproc;
	int ret = 0;

	if (!subsys_get_crash_status(pdata->subsys) && force_stop) {
		qcom_smem_state_update_bits(pdata->state,
			BIT(pdata->stop_bit), BIT(pdata->stop_bit));

		ret = wait_for_completion_timeout(&pdata->stop_done,
			msecs_to_jiffies(STOP_ACK_TIMEOUT_MS));

		if (ret == 0)
			pr_err("Timedout waiting for stop-ack\n");

		qcom_smem_state_update_bits(pdata->state,
			BIT(pdata->stop_bit), 0);
	}

	if (pdata->emulation) {
		pr_emerg("q6v5: Emulation stop, no smp2p messages\n");
		q6_rproc_stop(rproc);
		return 0;
	}

	rproc_shutdown(rproc);

	return 0;
}

static void wcss_panic_handler(const struct subsys_desc *subsys)
{
	struct q6v5_rproc_pdata *pdata = subsys_to_pdata(subsys);

	atomic_set(&pdata->running, RPROC_Q6V5_STOPPING);

	if (!subsys_get_crash_status(pdata->subsys)) {
		qcom_smem_state_update_bits(pdata->state,
			BIT(pdata->shutdown_bit), BIT(pdata->shutdown_bit));
		mdelay(STOP_ACK_TIMEOUT_MS);
	}
	return;
}

static int q6v5_load(struct rproc *rproc, const struct firmware *fw)
{
	int ret = 0;
	struct device *dev_rproc = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev_rproc);
	struct q6v5_rproc_pdata *pdata = platform_get_drvdata(pdev);

	if (pdata->secure) {
		ret = qcom_scm_pas_init_image(WCNSS_PAS_ID, fw->data, fw->size);
		if (ret) {
			dev_err(dev_rproc, "image authentication failed\n");
			return ret;
		}
	}
	pr_info("Sanity check passed for the image\n");

	return mdt_load(rproc, fw);
}

static struct rproc_ops ipq807x_q6v5_rproc_ops = {
	.start          = ipq807x_q6_rproc_start,
	.da_to_va       = q6_da_to_va,
	.stop           = q6_rproc_stop,
	.find_loaded_rsc_table = q6v5_find_loaded_rsc_table,
	.load = q6v5_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
};

static struct rproc_ops ipq60xx_q6v5_rproc_ops = {
	.start          = ipq60xx_q6_rproc_start,
	.da_to_va       = q6_da_to_va,
	.stop           = q6_rproc_stop,
	.find_loaded_rsc_table = q6v5_find_loaded_rsc_table,
	.load = q6v5_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
};

static struct rproc_ops ipq50xx_q6v5_rproc_ops = {
	.start          = ipq50xx_q6_rproc_start,
	.da_to_va       = q6_da_to_va,
	.stop           = q6_rproc_stop,
	.find_loaded_rsc_table = q6v5_find_loaded_rsc_table,
	.load = q6v5_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
};

static int q6_rproc_probe(struct platform_device *pdev)
{
	const char *firmware_name;
	struct rproc *rproc;
	int ret, emulation;
	unsigned int img_addr;
	struct resource *resource;
	struct qcom_smem_state *state;
	unsigned stop_bit;
	struct rproc_ops *ops;

	pr_emerg("DEV CI test message\n");
	state = qcom_smem_state_get(&pdev->dev, "stop",
					&stop_bit);
	if (IS_ERR(state)) {
		/* Wait till SMP2P is registered and up */
		return -EPROBE_DEFER;
	}

	ret = dma_set_coherent_mask(&pdev->dev,
			DMA_BIT_MASK(sizeof(dma_addr_t) * 8));
	if (ret) {
		dev_err(&pdev->dev, "dma_set_coherent_mask: %d\n", ret);
		return ret;
	}

	ret = of_property_read_string(pdev->dev.of_node, "firmware",
			&firmware_name);
	if (ret) {
		dev_err(&pdev->dev, "couldn't read firmware name: %d\n", ret);
		return ret;
	}

	ops = (void *)of_device_get_match_data(&pdev->dev);
	if (!ops) {
		dev_err(&pdev->dev, "chipset not supported\n");
		return -EIO;
	}

	emulation = of_property_read_bool(pdev->dev.of_node, "qca,emulation");
	img_addr = DEFAULT_IMG_ADDR;
	if (emulation) {
		ret = of_property_read_u32(pdev->dev.of_node, "img-addr",
							&img_addr);
		if (ops == &ipq807x_q6v5_rproc_ops)
			ops->start = ipq807x_q6_rproc_emu_start;
		else if (ops == &ipq60xx_q6v5_rproc_ops)
			ops->start = ipq60xx_q6_rproc_emu_start;
		else if (ops == &ipq50xx_q6v5_rproc_ops)
			ops->start = ipq50xx_q6_rproc_emu_start;
	}

	rproc = rproc_alloc(&pdev->dev, "q6v5-wcss", ops, firmware_name,
						sizeof(*q6v5_rproc_pdata));
	if (unlikely(!rproc))
		return -ENOMEM;

	q6v5_rproc_pdata = rproc->priv;
	q6v5_rproc_pdata->rproc = rproc;
	q6v5_rproc_pdata->stop_retry_count = 10;
	q6v5_rproc_pdata->emulation = emulation;
	q6v5_rproc_pdata->img_addr = img_addr;

	q6v5_rproc_pdata->secure = of_property_read_bool(pdev->dev.of_node,
					"qca,secure");
	if(of_property_read_bool(pdev->dev.of_node, "qca,dump-q6-reg"))
		debug_wcss |= DEBUG_DUMP_Q6_REG;

	q6v5_rproc_pdata->spurios_irqs = 0;
	rproc->has_iommu = false;
	/* We will record the values before q6 and wcss powerdown */
	q6v5_rproc_pdata->reg_save_buffer = kzalloc((Q6_REGISTER_SAVE_SIZE * 4),
							GFP_KERNEL);
	if (q6v5_rproc_pdata->reg_save_buffer == NULL) {
		ret = -ENOMEM;
		goto free_rproc;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "qca,sec-reset-cmd",
					&q6v5_rproc_pdata->reset_cmd_id);
	if (ret)
		q6v5_rproc_pdata->reset_cmd_id = QCOM_SCM_PAS_AUTH_DEBUG_RESET_CMD;

	if (!q6v5_rproc_pdata->secure) {
		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"wcss-base");
		if (unlikely(!resource)) {
			ret = -EINVAL;
			goto free_rproc;
		}

		q6v5_rproc_pdata->q6_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v5_rproc_pdata->q6_base) {
			ret = -ENOMEM;
			goto free_rproc;
		}

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"tcsr-q6-base");
		if (unlikely(!resource)) {
			ret = -EINVAL;
			goto free_rproc;
		}

		q6v5_rproc_pdata->tcsr_q6_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v5_rproc_pdata->tcsr_q6_base) {
			ret = -ENOMEM;
			goto free_rproc;
		}

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"tcsr-base");
		if (unlikely(!resource)) {
			ret = -EINVAL;
			goto free_rproc;
		}

		q6v5_rproc_pdata->tcsr_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v5_rproc_pdata->tcsr_base) {
			ret = -ENOMEM;
			goto free_rproc;
		}

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"mpm-base");
		if (unlikely(!resource)) {
			ret = -EINVAL;
			goto free_rproc;
		}

		q6v5_rproc_pdata->mpm_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v5_rproc_pdata->mpm_base) {
			ret = -ENOMEM;
			goto free_rproc;
		}

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"gcc-wcss-bcr-base");
		if (unlikely(!resource)) {
			ret = -EINVAL;
			goto free_rproc;
		}

		q6v5_rproc_pdata->gcc_bcr_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v5_rproc_pdata->gcc_bcr_base) {
			ret = -ENOMEM;
			goto free_rproc;
		}

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"gcc-wcss-misc-base");
		if (unlikely(!resource)) {
			ret = -EINVAL;
			goto free_rproc;
		}

		q6v5_rproc_pdata->gcc_misc_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v5_rproc_pdata->gcc_misc_base) {
			ret = -ENOMEM;
			goto free_rproc;
		}

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"tcsr-global");
		if (unlikely(!resource)) {
			ret = -EINVAL;
			goto free_rproc;
		}

		q6v5_rproc_pdata->tcsr_global_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v5_rproc_pdata->tcsr_global_base) {
			ret = -ENOMEM;
			goto free_rproc;
		}

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						 "tcsr-q6-boot-trig");
		if (resource != NULL) {
			q6v5_rproc_pdata->tcsr_q6_boot_trig =
				ioremap(resource->start,
						resource_size(resource));
			if (!q6v5_rproc_pdata->tcsr_q6_boot_trig) {
				ret = -ENOMEM;
				goto free_rproc;
			}
		}
	}

	platform_set_drvdata(pdev, q6v5_rproc_pdata);

	q6v5_rproc_pdata->err_ready_irq = platform_get_irq_byname(pdev,
						"qcom,gpio-err-ready");
	if (q6v5_rproc_pdata->err_ready_irq < 0) {
		pr_err("Can't get err-ready irq number %d\t deffered\n",
			q6v5_rproc_pdata->err_ready_irq);
		ret = q6v5_rproc_pdata->err_ready_irq;
		goto free_rproc;
	}
	ret = devm_request_threaded_irq(&pdev->dev,
				q6v5_rproc_pdata->err_ready_irq,
				NULL, wcss_err_ready_intr_handler,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"error_ready_interrupt", q6v5_rproc_pdata);
	if (ret < 0) {
		pr_err("Can't register err ready handler irq = %d ret = %d\n",
			q6v5_rproc_pdata->err_ready_irq, ret);
		goto free_rproc;
	}

	q6v5_rproc_pdata->state = qcom_smem_state_get(&pdev->dev, "stop",
			&q6v5_rproc_pdata->stop_bit);
	if (IS_ERR(q6v5_rproc_pdata->state)) {
		pr_err("Can't get stop bit status fro SMP2P\n");
		ret = PTR_ERR(q6v5_rproc_pdata->state);
		goto free_rproc;
	}

	q6v5_rproc_pdata->state = qcom_smem_state_get(&pdev->dev, "shutdown",
			&q6v5_rproc_pdata->shutdown_bit);
	if (IS_ERR(q6v5_rproc_pdata->state)) {
		pr_err("Can't get shutdown bit status fro SMP2P\n");
		ret = PTR_ERR(q6v5_rproc_pdata->state);
		goto free_rproc;
	}

	q6v5_rproc_pdata->subsys_desc.is_not_loadable = 0;
	q6v5_rproc_pdata->subsys_desc.name = "q6v5-wcss";
	q6v5_rproc_pdata->subsys_desc.dev = &pdev->dev;
	q6v5_rproc_pdata->subsys_desc.owner = THIS_MODULE;
	q6v5_rproc_pdata->subsys_desc.shutdown = stop_q6;
	q6v5_rproc_pdata->subsys_desc.powerup = start_q6;
	q6v5_rproc_pdata->subsys_desc.ramdump = crashdump_init;
	q6v5_rproc_pdata->subsys_desc.crash_shutdown = wcss_panic_handler;
	q6v5_rproc_pdata->subsys_desc.err_fatal_handler =
				wcss_err_fatal_intr_handler;
	q6v5_rproc_pdata->subsys_desc.stop_ack_handler =
				wcss_stop_ack_intr_handler;
	q6v5_rproc_pdata->subsys_desc.wdog_bite_handler =
				wcss_wdog_bite_intr_handler;
	q6v5_rproc_pdata->subsys_desc.depends_on = "q6v5-m3";

	q6v5_rproc_pdata->subsys = subsys_register(
					&q6v5_rproc_pdata->subsys_desc);
	if (IS_ERR(q6v5_rproc_pdata->subsys)) {
		ret = PTR_ERR(q6v5_rproc_pdata->subsys);
		goto free_rproc;
	}

	init_completion(&q6v5_rproc_pdata->stop_done);
	init_completion(&q6v5_rproc_pdata->err_ready);
	atomic_set(&q6v5_rproc_pdata->running, RPROC_Q6V5_STOPPED);

	rproc->auto_boot = false;
	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	return 0;

free_rproc:
	if (q6v5_rproc_pdata->gcc_misc_base)
		iounmap(q6v5_rproc_pdata->gcc_misc_base);

	if (q6v5_rproc_pdata->gcc_bcr_base)
		iounmap(q6v5_rproc_pdata->gcc_bcr_base);

	if (q6v5_rproc_pdata->mpm_base)
		iounmap(q6v5_rproc_pdata->mpm_base);

	if (q6v5_rproc_pdata->tcsr_base)
		iounmap(q6v5_rproc_pdata->tcsr_base);

	if (q6v5_rproc_pdata->tcsr_q6_base)
		iounmap(q6v5_rproc_pdata->tcsr_q6_base);

	if (q6v5_rproc_pdata->q6_base)
		iounmap(q6v5_rproc_pdata->q6_base);

	if (q6v5_rproc_pdata->tcsr_global_base)
		iounmap(q6v5_rproc_pdata->tcsr_global_base);

	if (q6v5_rproc_pdata->tcsr_q6_boot_trig)
		iounmap(q6v5_rproc_pdata->tcsr_q6_boot_trig);

	rproc_free(rproc);

	return ret;
}

static int q6_rproc_remove(struct platform_device *pdev)
{
	struct q6v5_rproc_pdata *pdata;
	struct rproc *rproc;

	pdata = platform_get_drvdata(pdev);
	rproc = q6v5_rproc_pdata->rproc;

	rproc_del(rproc);
	rproc_free(rproc);

	subsys_unregister(pdata->subsys);

	return 0;
}

static const struct of_device_id q6_match_table[] = {
	{ .compatible = "qca,q6v5-wcss-rproc-ipq807x", .data = &ipq807x_q6v5_rproc_ops },
	{ .compatible = "qca,q6v5-wcss-rproc-ipq60xx", .data = &ipq60xx_q6v5_rproc_ops },
	{ .compatible = "qca,q6v5-wcss-rproc-ipq50xx",
	  .data = &ipq50xx_q6v5_rproc_ops },
	{}
};

static struct platform_driver q6_rproc_driver = {
	.probe = q6_rproc_probe,
	.remove = q6_rproc_remove,
	.driver = {
		.name = "q6v5-wcss",
		.of_match_table = q6_match_table,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(q6_rproc_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("QCA Remote Processor control driver");

module_param(debug_wcss, int, 0644);
