// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016-2018 Linaro Ltd.
 * Copyright (C) 2014 Sony Mobile Communications AB
 * Copyright (c) 2012-2018, The Linux Foundation. All rights reserved.
 */
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/soc/qcom/smem.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/soc/qcom/mdt_loader.h>
#include "qcom_q6v5.h"
#include "qcom_common.h"
#include <linux/rpmsg/qcom_glink.h>
#include <linux/interrupt.h>
#include <linux/qcom_scm.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <uapi/linux/major.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_platform.h>

#define WCSS_CRASH_REASON		421

/* Q6SS Register Offsets */
#define Q6SS_RESET_REG			0x014
#define Q6SS_GFMUX_CTL_REG		0x020
#define Q6SS_PWR_CTL_REG		0x030
#define Q6SS_MEM_PWR_CTL		0x0B0
#define Q6SS_AHB_UPPER			0x104
#define Q6SS_AHB_LOWER			0x108

/* AXI Halt Register Offsets */
#define AXI_HALTREQ_REG			0x0
#define AXI_HALTACK_REG			0x4
#define AXI_IDLE_REG			0x8

#define HALT_ACK_TIMEOUT_MS		100

/* Q6SS_RESET */
#define Q6SS_STOP_CORE			BIT(0)
#define Q6SS_CORE_ARES			BIT(1)
#define Q6SS_BUS_ARES_ENABLE		BIT(2)
#define Q6SS_BOOT_CORE_START		0x400
#define Q6SS_BOOT_CMD			0x404
#define Q6SS_BOOT_STATUS		0x408
#define Q6SS_DBG_CFG			0x18

/* Q6SS_GFMUX_CTL */
#define Q6SS_CLK_ENABLE			BIT(1)

/* Q6SS_PWR_CTL */
#define Q6SS_L2DATA_STBY_N		BIT(18)
#define Q6SS_SLP_RET_N			BIT(19)
#define Q6SS_CLAMP_IO			BIT(20)
#define QDSS_BHS_ON			BIT(21)

/* Q6SS parameters */
#define Q6SS_LDO_BYP		BIT(25)
#define Q6SS_BHS_ON		BIT(24)
#define Q6SS_CLAMP_WL		BIT(21)
#define Q6SS_CLAMP_QMC_MEM		BIT(22)
#define Q6SS_TIMEOUT_US		1000
#define Q6SS_XO_CBCR		GENMASK(5, 3)

/* Q6SS config/status registers */
#define TCSR_GLOBAL_CFG0	0x0
#define TCSR_GLOBAL_CFG1	0x4
#define SSCAON_CONFIG		0x8
#define SSCAON_STATUS		0xc
#define Q6SS_BHS_STATUS		0x78
#define Q6SS_RST_EVB		0x10

#define BHS_EN_REST_ACK		BIT(0)
#define WCSS_HM_RET			BIT(1)
#define SSCAON_ENABLE		BIT(13)
#define SSCAON_BUS_EN		BIT(15)
#define SSCAON_BUS_MUX_MASK	GENMASK(18, 16)
#define SSCAON_MASK		GENMASK(17, 15)

#define MEM_BANKS		19
#define TCSR_WCSS_CLK_MASK	0x1F
#define TCSR_WCSS_CLK_ENABLE	0x14

#define WCNSS_PAS_ID		6
#define MPD_WCNSS_PAS_ID	0xD
#define DEFAULT_IMG_ADDR        0x4b000000

#define Q6_BOOT_TRIG_SVC_ID	0x5
#define Q6_BOOT_TRIG_CMD_ID	0x2

#define REMOTE_PID	1
#define VERSION 1
#define Q6_BOOT_ARGS_SMEM_SIZE 4096

struct q6v5_wcss_pd_fw_info {
	struct list_head node;
	phys_addr_t paddr;
	size_t size;
};

struct dump_file_private;

struct dumpdev {
	const struct file_operations *fops;
	char name[BUF_SIZE];
	fmode_t fmode;
	struct list_head dump_segments;
};

struct q6v5_wcss {
	struct device *dev;

	void __iomem *reg_base;
	void __iomem *rmb_base;
	void __iomem *mpm_base;
	void __iomem *tcsr_msip_base;
	void __iomem *tcsr_q6_boot_trig;
	void __iomem *aon_reset;

	struct regmap *halt_map;
	u32 halt_q6;
	u32 halt_wcss;
	u32 halt_nc;
	u32 reset_cmd_id;

	struct reset_control *wcss_aon_reset;
	struct reset_control *wcss_reset;
	struct reset_control *wcss_q6_reset;
	struct reset_control *ce_reset;

	struct qcom_q6v5 q6v5;

	struct qcom_rproc_subdev smd_subdev;
	struct qcom_rproc_glink glink_subdev;
	struct qcom_rproc_ssr ssr_subdev;
	struct qcom_sysmon *sysmon;

	phys_addr_t mem_phys;
	phys_addr_t mem_reloc;
	void *mem_region;
	size_t mem_size;
	const char *m3_fw_name;
	unsigned wcss_aon_seq;
	bool is_int_radio;
	struct dump_file_private *dfp;
	int pd_asid;
	struct dumpdev q6dump;
	struct q6v5_wcss_pd_fw_info *fw_info;
	bool is_radio_stopped;
};

struct q6_platform_data {
	bool nosecure;
	bool is_q6v6;
	bool emulation;
	bool is_mpd_arch;
};

static int debug_wcss;
static int load_pil = 1;
struct rproc *q6_rproc;

#if defined(CONFIG_IPQ_SS_DUMP)

#define	OPEN_TIMEOUT	60000
#define	DUMP_TIMEOUT	10000
#define NUM_WCSS_CLKS   ARRAY_SIZE(wcss_clk_names)

static const struct file_operations q6_dump_ops;
static struct class *dump_class;
static int dump_major;
DEFINE_SPINLOCK(dump_class_lock);
static atomic_t g_class_refcnt;

struct dump_file_private {
	int ehdr_remaining_bytes;
	struct list_head dump_segments;
	Elf32_Ehdr *ehdr;
	struct task_struct *pdesc;
	struct timer_list dump_timeout;
	struct completion dump_complete;

	struct timer_list open_timeout;
	struct completion open_complete;
	atomic_t open_timedout;
};

struct dump_segment {
	struct list_head node;
	phys_addr_t addr;
	size_t size;
	loff_t offset;
};

static const char *wcss_clk_names[] = {"gcc_q6_axis_clk",
					"gcc_wcss_ahb_s_clk",
					"gcc_wcss_ecahb_clk",
					"gcc_wcss_acmt_clk",
					"gcc_wcss_axi_m_clk",
					"gcc_q6_axim_clk",
					"gcc_q6_axim2_clk",
					"gcc_q6_ahb_clk",
					"gcc_q6_ahb_s_clk",
					"gcc_wcss_axi_s_clk"};

static struct clk *g_wcss_clks[NUM_WCSS_CLKS] = {NULL};

static int wcss_clks_prepare_disable(struct device *dev, int clk_sta_id,
							 int clk_cnt)
{
	int temp;

	if (clk_cnt > NUM_WCSS_CLKS)
		return -EINVAL;

	if (clk_sta_id >= clk_cnt)
		clk_cnt = clk_cnt + clk_sta_id;

	for (temp = clk_sta_id; temp < clk_cnt; temp++) {
		if (g_wcss_clks[temp] == NULL)
			continue;
		clk_disable_unprepare(g_wcss_clks[temp]);
		devm_clk_put(dev, g_wcss_clks[temp]);
		g_wcss_clks[temp] = NULL;
	}
	return 0;
}

static int wcss_clks_prepare_enable(struct device *dev, int clk_sta_id,
							int clk_cnt)
{
	int temp, ret;

	if (clk_cnt > NUM_WCSS_CLKS)
		return -EINVAL;

	if (clk_sta_id >= clk_cnt)
		clk_cnt = clk_cnt + clk_sta_id;

	for (temp = clk_sta_id; temp < clk_cnt; temp++) {
		g_wcss_clks[temp] = devm_clk_get(dev, wcss_clk_names[temp]);
		if (IS_ERR(g_wcss_clks[temp])) {
			pr_err("%s unable to get clk %s\n", __func__,
					wcss_clk_names[temp]);
			ret = PTR_ERR(g_wcss_clks[temp]);
			goto disable_clk;
		}
		ret = clk_prepare_enable(g_wcss_clks[temp]);
		if (ret) {
			pr_err("%s unable to enable clk %s\n",
					__func__, wcss_clk_names[temp]);
			goto put_disable_clk;
		}
	}
	return 0;

put_disable_clk:
	devm_clk_put(dev, g_wcss_clks[temp]);
disable_clk:
	wcss_clks_prepare_disable(dev, 0, temp);
	return ret;
}

static void open_timeout_func(unsigned long data)
{
	struct q6v5_wcss *wcss = (struct q6v5_wcss *)data;

	atomic_set(&wcss->dfp->open_timedout, 1);
	complete(&wcss->dfp->open_complete);
	pr_err("open time Out: Q6 crash dump collection failed\n");
}

static void dump_timeout_func(unsigned long data)
{
	struct q6v5_wcss *wcss = (struct q6v5_wcss *)data;

	pr_err("Time Out: Q6 crash dump collection failed\n");

	wcss->dfp->dump_timeout.data = -ETIMEDOUT;
	send_sig(SIGKILL, wcss->dfp->pdesc, 0);
}

static int q6_dump_open(struct inode *inode, struct file *file)
{
	struct dump_file_private *dfp = NULL;
	struct dump_segment *segment, *tmp;
	int nsegments = 0;
	size_t elfcore_hdrsize, p_off;
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	struct q6v5_wcss *wcss = q6_rproc->priv;
	int dump_minor =  iminor(inode);

	if (dump_minor) {
		struct rproc_child *rp_child;

		list_for_each_entry(rp_child, &q6_rproc->child, node) {
			struct rproc *c_rproc =
				(struct rproc *)rp_child->handle;

			wcss = c_rproc->priv;
			if (wcss->pd_asid == dump_minor)
				break;
		}
	}

	file->private_data = wcss;
	dfp = wcss->dfp;

	del_timer_sync(&dfp->open_timeout);

	if (atomic_read(&dfp->open_timedout) == 1) {
		pr_err("open timedout\n");
		return -ENODEV;
	}

	if (list_empty(&wcss->q6dump.dump_segments)) {
		pr_err("q6dump segments list is null\n");
		return -ENODEV;
	}

	file->f_mode |= wcss->q6dump.fmode;

	INIT_LIST_HEAD(&dfp->dump_segments);
	list_for_each_entry(segment, &wcss->q6dump.dump_segments, node) {
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
	list_for_each_entry(segment, &wcss->q6dump.dump_segments, node) {
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

	/* This takes care of the user space app stalls during delayed read. */
	init_completion(&wcss->dfp->dump_complete);

	setup_timer(&wcss->dfp->dump_timeout, dump_timeout_func,
						(unsigned long)wcss);
	mod_timer(&wcss->dfp->dump_timeout,
			jiffies + msecs_to_jiffies(DUMP_TIMEOUT));

	complete(&wcss->dfp->open_complete);
	return 0;

err:
	list_for_each_entry_safe(segment, tmp, &dfp->dump_segments, node) {
		list_del(&segment->node);
		kfree(segment);
	}
	return -ENOMEM;
}

static int q6_dump_release(struct inode *inode, struct file *file)
{
	struct dump_segment *segment, *tmp;
	struct q6v5_wcss *wcss = file->private_data;
	struct dump_file_private *dfp = wcss->dfp;

	list_for_each_entry_safe(segment, tmp, &dfp->dump_segments, node) {
		list_del(&segment->node);
		kfree(segment);
	}

	device_destroy(dump_class, MKDEV(dump_major, wcss->pd_asid));
	atomic_dec(&g_class_refcnt);

	if (atomic_read(&g_class_refcnt) == 0) {
		spin_lock(&dump_class_lock);
		class_destroy(dump_class);
		dump_class = NULL;
		unregister_chrdev(dump_major, "dump");
		dump_major = 0;
		spin_unlock(&dump_class_lock);
	}

	complete(&dfp->dump_complete);
	return 0;
}

static ssize_t q6_dump_read(struct file *file, char __user *buf, size_t count,
		loff_t *ppos)
{
	void *buffer = NULL;
	struct q6v5_wcss *wcss = file->private_data;
	struct dump_file_private *dfp = wcss->dfp;

	struct dump_segment *segment, *tmp;
	size_t copied = 0, to_copy = count;
	int segment_num = 0;

	if (dfp->dump_timeout.data == -ETIMEDOUT) {
		pr_err("dump timedout\n");
		return 0;
	}

	mod_timer(&dfp->dump_timeout, jiffies + msecs_to_jiffies(DUMP_TIMEOUT));

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

int crashdump_add_segment(phys_addr_t dump_addr, size_t dump_size,
					struct dumpdev *dumpdev)

{
	struct dump_segment *segment;

	segment = kzalloc(sizeof(*segment), GFP_KERNEL);
	if (!segment)
		return -ENOMEM;

	segment->addr = dump_addr;
	segment->size = dump_size;
	segment->offset = 0;

	list_add_tail(&segment->node, &dumpdev->dump_segments);

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

	ret = crashdump_add_segment(segment.addr, segment.size, dumpdev);

fail:
	return ret;
}

static void crashdump_init(struct rproc *rproc, struct rproc_dump_segment *segment, void *dest)
{
	int ret = 0;
	int index = 0;
	struct device *dump_dev = NULL;
	struct device_node *node = NULL, *np = NULL;
	struct q6v5_wcss *wcss = rproc->priv;
	struct dump_segment *seg, *tmp;

	wcss->dfp = kzalloc(sizeof(struct dump_file_private), GFP_KERNEL);
	if (wcss->dfp == NULL) {
		pr_err("%s:\tCan not allocate memory for private structure\n",
				__func__);
		return;
	}

	init_completion(&wcss->dfp->open_complete);
	atomic_set(&wcss->dfp->open_timedout, 0);

	spin_lock(&dump_class_lock);
	if (!dump_class) {
		dump_major = register_chrdev(UNNAMED_MAJOR, "dump",
								&q6_dump_ops);
		if (dump_major < 0) {
			ret = dump_major;
			pr_err("Unable to allocate a major number err = %d",
									ret);
			spin_unlock(&dump_class_lock);
			goto reg_failed;
		}

		dump_class = class_create(THIS_MODULE, "dump");
		if (IS_ERR(dump_class)) {
			ret = PTR_ERR(dump_class);
			spin_unlock(&dump_class_lock);
			goto class_failed;
		}
	}
	spin_unlock(&dump_class_lock);

	if (rproc->parent) {
		strlcpy(wcss->q6dump.name, rproc->name, BUF_SIZE);
		strlcat(wcss->q6dump.name, "_mem", BUF_SIZE);
	} else
		strlcpy(wcss->q6dump.name, "q6mem", BUF_SIZE);

	wcss->q6dump.fops = &q6_dump_ops;
	wcss->q6dump.fmode = FMODE_UNSIGNED_OFFSET | FMODE_EXCL;

	dump_dev = device_create(dump_class, NULL,
					MKDEV(dump_major, wcss->pd_asid),
					NULL, wcss->q6dump.name);
	if (IS_ERR(dump_dev)) {
		ret = PTR_ERR(dump_dev);
		pr_err("Unable to create a device err = %d", ret);
		goto device_failed;
	}
	atomic_inc(&g_class_refcnt);

	INIT_LIST_HEAD(&wcss->q6dump.dump_segments);

	if (rproc->parent == NULL)
		np = of_find_node_by_name(NULL, "qcom_q6v5_wcss");
	else
		np = of_find_node_by_name(NULL, rproc->name);

	while (1) {
		node = of_parse_phandle(np, "memory-region", index);
		if (node == NULL)
			break;

		ret = add_segment(&wcss->q6dump, node);
		of_node_put(node);
		if (ret != 0)
			break;

		index++;
	}
	of_node_put(np);

	if (wcss->fw_info) {
		ret = crashdump_add_segment(wcss->fw_info->paddr,
				wcss->fw_info->size, &wcss->q6dump);
		if (ret)
			pr_err("unable to add segments\n");
	}

	/* This avoids race condition between the scheduled timer and the opened
	 * file discriptor during delay in user space app execution.
	 */
	setup_timer(&wcss->dfp->open_timeout, open_timeout_func,
						(unsigned long)wcss);

	mod_timer(&wcss->dfp->open_timeout,
				jiffies + msecs_to_jiffies(OPEN_TIMEOUT));

	wait_for_completion(&wcss->dfp->open_complete);
	if (atomic_read(&wcss->dfp->open_timedout) == 1) {
		ret = -ETIMEDOUT;
		goto dump_dev_failed;
	}
	wait_for_completion(&wcss->dfp->dump_complete);
	if (wcss->dfp->dump_timeout.data == -ETIMEDOUT) {
		ret = wcss->dfp->dump_timeout.data;
		wcss->dfp->dump_timeout.data = 0;
	}

	del_timer_sync(&wcss->dfp->dump_timeout);

	list_for_each_entry_safe(seg, tmp, &wcss->q6dump.dump_segments, node) {
		list_del(&seg->node);
		kfree(seg);
	}

	kfree(wcss->dfp->ehdr);
	wcss->dfp->ehdr = NULL;
	kfree(wcss->dfp);
	wcss->dfp = NULL;
	return;

dump_dev_failed:
	device_destroy(dump_class, MKDEV(dump_major, wcss->pd_asid));
	if (atomic_read(&g_class_refcnt) == 0) {
device_failed:
		class_destroy(dump_class);
class_failed:
		unregister_chrdev(dump_major, "dump");
		dump_major = 0;
	}
reg_failed:
	kfree(wcss->dfp);
	return;
}
#else
static void crashdump_init(struct rproc *rproc, struct rproc_dump_segment *segment, void *dest)
{
	return;
}

#endif /* CONFIG_IPQ_SS_DUMP */

int q6v5_wcss_store_pd_fw_info(struct device *dev, phys_addr_t paddr,
							size_t size)
{
	struct rproc *rproc = dev_get_drvdata(dev);
	struct q6v5_wcss *wcss;

	if (rproc == NULL) {
		pr_err("Invalid rproc\n");
		return -EINVAL;
	}

	wcss = rproc->priv;
	wcss->fw_info = devm_kzalloc(wcss->dev,
					sizeof(struct q6v5_wcss_pd_fw_info),
					GFP_KERNEL);
	if (IS_ERR_OR_NULL(wcss->fw_info)) {
		dev_err(wcss->dev,
				"failed to allocate memory to store fw info\n");
		return PTR_ERR(wcss->fw_info);
	}
	wcss->fw_info->paddr = paddr;
	wcss->fw_info->size = size;
	return 0;
}
EXPORT_SYMBOL(q6v5_wcss_store_pd_fw_info);

#ifdef CONFIG_CNSS2
static int crashdump_init_new(int check, const struct subsys_desc *subsys)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);
	struct rproc *rproc = q6v5->rproc;
	struct rproc_dump_segment *segment = NULL;
	void *dest = NULL;

	crashdump_init(rproc, segment, dest);
	return 0;
}

static int start_q6_userpd(const struct subsys_desc *subsys)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);
	struct rproc *rproc = q6v5->rproc;
	int ret;
	struct q6v5_wcss *wcss_p = rproc->parent->priv;
	struct q6_platform_data *pdata = dev_get_platdata(wcss_p->dev);

	if (pdata->emulation && !load_pil) {
		pr_info("%s: Emulation start, PIL loading skipped\n",
							rproc->name);
		rproc->ops->start(rproc);
		return 0;
	}

	ret = rproc_boot(rproc);
	if (ret)
		pr_err("couldn't boot (%s)q6v5: %d\n", rproc->name, ret);
	else
		q6v5->running = true;

	return ret;
}

static int start_q6(const struct subsys_desc *subsys)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);
	struct rproc *rproc = q6v5->rproc;
	int ret = 0;
	struct q6_platform_data *pdata =
		dev_get_platdata(((struct q6v5_wcss *)rproc->priv)->dev);

	if (pdata->emulation && !load_pil) {
		pr_info("q6v5: Emulation start, PIL loading skipped\n");
		rproc->bootaddr = DEFAULT_IMG_ADDR;
		rproc->ops->start(rproc);
		rproc_start_subdevices(rproc);
		return 0;
	}

	ret = rproc_boot(rproc);
	if (ret)
		pr_err("couldn't boot q6v5: %d\n", ret);
	else
		q6v5->running = true;

	return ret;
}

static int stop_q6_userpd(const struct subsys_desc *subsys, bool force_stop)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);
	struct rproc *rproc = q6v5->rproc;
	struct q6v5_wcss *wcss = rproc->priv, *wcss_p = rproc->parent->priv;
	int ret = 0;
	struct q6_platform_data *pdata = dev_get_platdata(wcss_p->dev);

	if (!subsys_get_crash_status(q6v5->subsys) && force_stop) {
		ret = qcom_q6v5_request_stop(&wcss->q6v5);
		if (ret == -ETIMEDOUT) {
			dev_err(wcss->dev, "timed out on wait\n");
			return ret;
		}
	}

	if (pdata->emulation && !load_pil) {
		pr_info("%s: Emulation stop\n", rproc->name);
		rproc->ops->stop(rproc);
		goto stop_flag;
	}

	rproc_shutdown(rproc);
stop_flag:
	q6v5->running = false;
	return ret;
}

static int stop_q6(const struct subsys_desc *subsys, bool force_stop)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);
	struct rproc *rproc = q6v5->rproc;
	struct q6v5_wcss *wcss = rproc->priv;
	int ret = 0;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);

	if (!subsys_get_crash_status(q6v5->subsys) && force_stop) {
		ret = qcom_q6v5_request_stop(&wcss->q6v5);
		if (ret == -ETIMEDOUT) {
			dev_err(wcss->dev, "timed out on wait\n");
			return ret;
		}
	}

	if (pdata->emulation && !load_pil) {
		pr_info("q6v5: Emulation stop\n");
		rproc_stop_subdevices(rproc, false);
		rproc->ops->stop(rproc);
		goto stop_flag;
	}

	rproc_shutdown(rproc);

stop_flag:
	q6v5->running = false;
	return ret;
}
#endif

static int q6v5_wcss_reset(struct q6v5_wcss *wcss)
{
	int ret;
	u32 val;
	int i;

	if (wcss->wcss_aon_seq) {
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val |= BIT(0);
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
		mdelay(1);

		/*set CFG[18:15]=1* and clear CFG[1]=0*/
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~(SSCAON_BUS_MUX_MASK | WCSS_HM_RET);
		val |= SSCAON_BUS_EN;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
		mdelay(1);
	}

	/* Assert resets, stop core */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val |= Q6SS_CORE_ARES | Q6SS_BUS_ARES_ENABLE | Q6SS_STOP_CORE;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);
	/* BHS require xo cbcr to be enabled */
	val = readl(wcss->reg_base + Q6SS_XO_CBCR);
	val |= 0x1;
	writel(val, wcss->reg_base + Q6SS_XO_CBCR);

	/* Read CLKOFF bit to go low indicating CLK is enabled */
	ret = readl_poll_timeout(wcss->reg_base + Q6SS_XO_CBCR,
				 val, !(val & BIT(31)), 1,
				 Q6SS_TIMEOUT_US);
	if (ret) {
		dev_err(wcss->dev,
			"xo cbcr enabling timed out (rc:%d)\n", ret);
		return ret;
	}
	/* Enable power block headswitch and wait for it to stabilize */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_BHS_ON;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);
	udelay(1);

	/* Put LDO in bypass mode */
	val |= Q6SS_LDO_BYP;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Deassert Q6 compiler memory clamp */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_CLAMP_QMC_MEM;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Deassert memory peripheral sleep and L2 memory standby */
	val |= Q6SS_L2DATA_STBY_N | Q6SS_SLP_RET_N;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Turn on L1, L2, ETB and JU memories 1 at a time */
	val = readl(wcss->reg_base + Q6SS_MEM_PWR_CTL);
	for (i = MEM_BANKS; i >= 0; i--) {
		val |= BIT(i);
		writel(val, wcss->reg_base + Q6SS_MEM_PWR_CTL);
		/*
		 * Read back value to ensure the write is done then
		 * wait for 1us for both memory peripheral and data
		 * array to turn on.
		 */
		val |= readl(wcss->reg_base + Q6SS_MEM_PWR_CTL);
		udelay(1);
	}
	/* Remove word line clamp */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val &= ~Q6SS_CLAMP_WL;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Remove IO clamp */
	val &= ~Q6SS_CLAMP_IO;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* Bring core out of reset */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val &= ~Q6SS_CORE_ARES;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	/* Turn on core clock */
	val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	val |= Q6SS_CLK_ENABLE;
	writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);

	/* Start core execution */
	val = readl(wcss->reg_base + Q6SS_RESET_REG);
	val &= ~Q6SS_STOP_CORE;
	writel(val, wcss->reg_base + Q6SS_RESET_REG);

	if (wcss->wcss_aon_seq) {
		/* Wait for SSCAON_STATUS */
		val = readl(wcss->rmb_base + SSCAON_STATUS);
		ret = readl_poll_timeout(wcss->rmb_base + SSCAON_STATUS,
					 val, (val & 0xffff) == 0x10, 1000,
					 Q6SS_TIMEOUT_US * 1000);
		if (ret) {
			dev_err(wcss->dev, " Boot Error, SSCAON=0x%08X\n", val);
			return ret;
		}
	}

	return 0;
}

static int q6v5_wcss_powerup(struct q6v5_wcss *wcss)
{
	int ret;
	u32 val;

	if (wcss->wcss_aon_seq) {
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~SSCAON_MASK;
		val |= SSCAON_BUS_EN;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);

		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~(1<<1);
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
		mdelay(2);

		/* 5 - wait for SSCAON_STATUS */
		ret = readl_poll_timeout(wcss->rmb_base + SSCAON_STATUS,
				val, (val & 0xffff) == 0x10, 1000,
				Q6SS_TIMEOUT_US * 10);
		if (ret) {
			dev_err(wcss->dev,
				"can't get SSCAON_STATUS rc:%d)\n", ret);
			return ret;
		}
	}

	return 0;
}

static void q6v6_wcss_reset(struct q6v5_wcss *wcss)
{
	u32 val;
	int ret;
	int temp = 0;
	unsigned int cookie;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);

	/*Assert Q6 BLK Reset*/
	ret = reset_control_assert(wcss->wcss_q6_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_q6_reset failed\n");
		return;
	}

	/* AON Reset */
	ret = reset_control_deassert(wcss->wcss_aon_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_aon_reset failed\n");
		return;
	}

	/*Enable Q6 AXIS CLOCK RESET*/
	ret = wcss_clks_prepare_enable(wcss->dev, 0, 1);
	if (ret) {
		dev_err(wcss->dev, "wcss clk(s) enable failed");
		return;
	}

	/*Disable Q6 AXIS CLOCK RESET*/
	ret = wcss_clks_prepare_disable(wcss->dev, 0, 1);
	if (ret) {
		dev_err(wcss->dev, "wcss clk(s) enable failed");
		return;
	}

	/*De assert Q6 BLK reset*/
	ret = reset_control_deassert(wcss->wcss_q6_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_q6_reset failed\n");
		return;
	}

	/*Prepare Q6 clocks*/
	if (!pdata->is_mpd_arch) {
		ret = wcss_clks_prepare_enable(wcss->dev, 1, NUM_WCSS_CLKS);
		if (ret) {
			dev_err(wcss->dev, "wcss clk(s) enable failed");
			return;
		}
	} else {
		ret = wcss_clks_prepare_enable(wcss->dev, 2, 1);
		if (ret) {
			dev_err(wcss->dev, "wcss clk(s) enable failed");
			return;
		}

		ret = wcss_clks_prepare_enable(wcss->dev, 5, NUM_WCSS_CLKS);
		if (ret) {
			dev_err(wcss->dev, "wcss clk(s) enable failed");
			return;
		}
	}

	/*Secure access to WIFI phy register*/
	regmap_update_bits(wcss->halt_map,
			wcss->halt_nc + TCSR_GLOBAL_CFG1,
			TCSR_WCSS_CLK_MASK,
			0x18);

	/*Disable Q6 AXI2 select*/
	regmap_update_bits(wcss->halt_map,
			wcss->halt_nc + TCSR_GLOBAL_CFG0, 0x40, 0xF0);

	/*wcss axib ib status*/
	regmap_update_bits(wcss->halt_map,
			wcss->halt_nc + TCSR_GLOBAL_CFG0, 0x100, 0x100);

	/*Enable global counter for qtimer*/
	if (wcss->mpm_base && !qcom_scm_is_available())
		writel(0x1, wcss->mpm_base + 0x00);

	/*Q6 AHB upper & lower address*/
	writel(0x00cdc000, wcss->reg_base + Q6SS_AHB_UPPER);
	writel(0x00ca0000, wcss->reg_base + Q6SS_AHB_LOWER);

	/*Configure MSIP*/
	if (wcss->tcsr_msip_base)
		writel(0x1, wcss->tcsr_msip_base + 0x00);

	if (!pdata->is_mpd_arch) {
		/*WCSS powerup*/
		ret = q6v5_wcss_powerup(wcss);
		if (ret) {
			dev_err(wcss->dev, "failed to power up wcss\n");
			return;
		}

		/*Deassert ce reset*/
		ret = reset_control_deassert(wcss->ce_reset);
		if (ret) {
			dev_err(wcss->dev, "ce_reset failed\n");
			return;
		}
	}

	if (debug_wcss)
		writel(0x20000001, wcss->reg_base + Q6SS_DBG_CFG);
	else
		writel(0x0, wcss->reg_base + Q6SS_DBG_CFG);

	if (qcom_scm_is_available()) {
		cookie = 1;
		ret = qcom_scm_wcss_boot(Q6_BOOT_TRIG_SVC_ID,
						Q6_BOOT_TRIG_CMD_ID, &cookie);
		if (ret) {
			dev_err(wcss->dev, "q6-boot trigger scm failed\n");
			return;
		}
	} else
		/* Trigger Boot FSM, to bring core out of rst */
		writel(0x1, wcss->tcsr_q6_boot_trig + 0x0);

	/* Boot core start */
	writel(0x1, wcss->reg_base + Q6SS_BOOT_CORE_START);

	while (temp < 20) {
		val = readl(wcss->reg_base + Q6SS_BOOT_STATUS);
		if (val & 0x01)
			break;
		mdelay(1);
		temp += 1;
	}

	pr_err("%s: start %s\n", wcss->q6v5.rproc->name,
					val == 1 ? "successful" : "failed");
	wcss->q6v5.running = val == 1 ? true : false;
}

static int q6v5_wcss_userpd_powerup(struct rproc *rproc)
{
	int ret;
	struct q6v5_wcss *wcss = rproc->priv;

	if (wcss->is_int_radio) {
		struct q6v5_wcss *wcss_p = rproc->parent->priv;
		struct q6_platform_data *pdata = dev_get_platdata(wcss_p->dev);

		if (pdata->nosecure)
			goto skip_secure;

		ret = qcom_scm_int_radio_powerup(MPD_WCNSS_PAS_ID);
		if (ret) {
			dev_err(wcss->dev, "failed to power up int radio\n");
			return ret;
		}
		goto success_msg;
skip_secure:
		/* AON Reset */
		ret = reset_control_deassert(wcss_p->wcss_aon_reset);
		if (ret) {
			dev_err(wcss_p->dev, "wcss_aon_reset failed\n");
			return ret;
		}

		ret = wcss_clks_prepare_enable(wcss_p->dev, 1, 1);
		if (ret) {
			dev_err(wcss_p->dev,
					"failed to enable %s clock %d\n",
					wcss_clk_names[1], ret);
			return ret;
		}

		ret = wcss_clks_prepare_enable(wcss_p->dev, 3, 2);
		if (ret) {
			dev_err(wcss_p->dev,
					"failed to enable clock %d\n",
					ret);
			return ret;
		}

		ret = q6v5_wcss_powerup(wcss_p);
		if (ret)
			return ret;

		/*Deassert ce reset*/
		ret = reset_control_deassert(wcss_p->ce_reset);
		if (ret) {
			dev_err(wcss->dev, "ce_reset failed\n");
			return ret;
		}
success_msg:
		pr_info("%s wcss powered up successfully\n", rproc->name);
	}
	return 0;
}

static int q6v5_wcss_userpd_start(struct rproc *rproc)
{
	int ret = 0;
	struct q6v5_wcss *wcss = rproc->priv;

	/*power up userpd wcss*/
	ret = q6v5_wcss_userpd_powerup(rproc);
	if (ret) {
		dev_err(wcss->dev, "Failed to power up %s\n", rproc->name);
		return ret;
	}

	ret = qcom_q6v5_request_spawn(&wcss->q6v5);
	if (ret == -ETIMEDOUT) {
		pr_err("%s: %s spawn timedout\n", __func__, rproc->name);
		return ret;
	}

	ret = qcom_q6v5_wait_for_start(&wcss->q6v5, 5 * HZ);
	if (ret == -ETIMEDOUT) {
		pr_err("%s start timedout\n", rproc->name);
		wcss->q6v5.running = false;
		return ret;
	}
	wcss->q6v5.running = true;
	return ret;
}

static int q6v5_wcss_start(struct rproc *rproc)
{
	struct q6v5_wcss *wcss = rproc->priv;
	struct qcom_q6v5 *q6v5 = &wcss->q6v5;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);
	int ret;
	u32 peripheral = pdata->is_mpd_arch ? MPD_WCNSS_PAS_ID : WCNSS_PAS_ID;

	if (pdata->nosecure)
		goto skip_secure;

	qcom_q6v5_prepare(&wcss->q6v5);
	ret = qcom_scm_pas_auth_and_reset(peripheral, debug_wcss,
						wcss->reset_cmd_id);
	if (ret) {
		dev_err(wcss->dev, "q6-wcss reset failed\n");
		qcom_q6v5_unprepare(&wcss->q6v5);
		return ret;
	} else {
		/* q6-wcss reset done. wait for ready interrupt */
		goto skip_reset;
	}

skip_secure:
	/* Release Q6 and WCSS reset */
	ret = reset_control_deassert(wcss->wcss_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_reset failed\n");
		return ret;
	}

	ret = reset_control_deassert(wcss->wcss_q6_reset);
	if (ret) {
		dev_err(wcss->dev, "wcss_q6_reset failed\n");
		goto wcss_reset;
	}

	/* Lithium configuration - clock gating and bus arbitration */
	ret = regmap_update_bits(wcss->halt_map,
				 wcss->halt_nc + TCSR_GLOBAL_CFG0,
				 TCSR_WCSS_CLK_MASK,
				 TCSR_WCSS_CLK_ENABLE);
	if (ret)
		goto wcss_q6_reset;

	ret = regmap_update_bits(wcss->halt_map,
				 wcss->halt_nc + TCSR_GLOBAL_CFG1,
				 1, 0);
	if (ret)
		goto wcss_q6_reset;

	if (debug_wcss && !pdata->is_q6v6)
		writel(0x20000001, wcss->reg_base + Q6SS_DBG_CFG);

	/* Write bootaddr to EVB so that Q6WCSS will jump there after reset */
	writel(rproc->bootaddr >> 4, wcss->reg_base + Q6SS_RST_EVB);

	if (pdata->is_q6v6) {
		q6v6_wcss_reset(wcss);
	} else {
		ret = q6v5_wcss_reset(wcss);
		if (ret)
			goto wcss_q6_reset;
	}

	if (debug_wcss && !pdata->is_q6v6)
		writel(0x0, wcss->reg_base + Q6SS_DBG_CFG);
skip_reset:
	ret = qcom_q6v5_wait_for_start(&wcss->q6v5, 5 * HZ);
	if (ret == -ETIMEDOUT) {
		if (debug_wcss)
			goto skip_reset;

		if (!pdata->nosecure)
			qcom_scm_pas_shutdown(peripheral);

		if (q6v5->running) {
			ret = 0;
			pr_err("%s up without err ready\n", q6v5->rproc->name);
		} else {
			dev_err(wcss->dev, "start timed out\n");
			q6v5->running = false;
			goto wcss_q6_reset;
		}
	}

	q6v5->running = true;
	return ret;

wcss_q6_reset:
	reset_control_assert(wcss->wcss_q6_reset);

wcss_reset:
	reset_control_assert(wcss->wcss_reset);

	return ret;
}

static void q6v6_wcss_halt_axi_port(struct q6v5_wcss *wcss,
				    struct regmap *halt_map,
				    u32 offset)
{
	unsigned long timeout;
	unsigned int val;
	int ret;

	/* Assert halt request */
	regmap_write(halt_map, offset + AXI_HALTREQ_REG, 1);

	/* Wait for halt */
	timeout = jiffies + msecs_to_jiffies(HALT_ACK_TIMEOUT_MS);
	for (;;) {
		ret = regmap_read(halt_map, offset + AXI_HALTACK_REG, &val);
		if (ret || val || time_after(jiffies, timeout))
			break;

		msleep(1);
	}

	if (offset == wcss->halt_q6) {
		ret = regmap_read(halt_map, offset + AXI_IDLE_REG, &val);
		if (ret || !val)
			dev_err(wcss->dev, "port failed halt\n");
	}

	/* Clear halt request (port will remain halted until reset) */
	regmap_write(halt_map, offset + AXI_HALTREQ_REG, 0);
}

static void q6v5_wcss_halt_axi_port(struct q6v5_wcss *wcss,
				    struct regmap *halt_map,
				    u32 offset)
{
	unsigned long timeout;
	unsigned int val;
	int ret;

	/* Check if we're already idle */
	ret = regmap_read(halt_map, offset + AXI_IDLE_REG, &val);
	if (!ret && val)
		return;

	/* Assert halt request */
	regmap_write(halt_map, offset + AXI_HALTREQ_REG, 1);

	/* Wait for halt */
	timeout = jiffies + msecs_to_jiffies(HALT_ACK_TIMEOUT_MS);
	for (;;) {
		ret = regmap_read(halt_map, offset + AXI_HALTACK_REG, &val);
		if (ret || val || time_after(jiffies, timeout))
			break;

		msleep(1);
	}

	ret = regmap_read(halt_map, offset + AXI_IDLE_REG, &val);
	if (ret || !val)
		dev_err(wcss->dev, "port failed halt\n");

	/* Clear halt request (port will remain halted until reset) */
	regmap_write(halt_map, offset + AXI_HALTREQ_REG, 0);
}

static int q6v5_wcss_powerdown(struct q6v5_wcss *wcss)
{
	int ret;
	u32 val;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);

	/* 1 - Assert WCSS/Q6 HALTREQ */
	if (!pdata->is_q6v6)
		q6v5_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_wcss);
	else
		q6v6_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_wcss);

	if (!pdata->is_q6v6) {
		/* 2 - Enable WCSSAON_CONFIG */
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val |= SSCAON_ENABLE;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);

		/* 3 - Set SSCAON_CONFIG */
		val |= SSCAON_BUS_EN;
		val &= ~SSCAON_BUS_MUX_MASK;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	} else {
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~SSCAON_MASK;
		val |= SSCAON_BUS_EN;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	}

	/* 4 - SSCAON_CONFIG 1 */
	val |= BIT(1);
	writel(val, wcss->rmb_base + SSCAON_CONFIG);
	if (pdata->is_q6v6)
		mdelay(2);

	/* 5 - wait for SSCAON_STATUS */
	ret = readl_poll_timeout(wcss->rmb_base + SSCAON_STATUS,
				 val, (val & 0xffff) == 0x400, 1000,
				 Q6SS_TIMEOUT_US * 10);
	if (ret) {
		dev_err(wcss->dev,
			"can't get SSCAON_STATUS rc:%d)\n", ret);
		return ret;
	}

	/* 6 - De-assert WCSS_AON reset */
	reset_control_assert(wcss->wcss_aon_reset);

	if (!pdata->is_q6v6) {
		/* 7 - Disable WCSSAON_CONFIG 13 */
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~SSCAON_ENABLE;
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	} else {
		val = readl(wcss->rmb_base + SSCAON_CONFIG);
		val &= ~(1<<1);
		writel(val, wcss->rmb_base + SSCAON_CONFIG);
	}

	/* 8 - De-assert WCSS/Q6 HALTREQ */
	reset_control_assert(wcss->wcss_reset);

	return 0;
}

static void q6v6_q6_powerdown(struct q6v5_wcss *wcss)
{
	int ret;
	unsigned int cookie;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);

	if (!pdata->is_mpd_arch) {
		/*Assert ce reset*/
		reset_control_assert(wcss->ce_reset);
		mdelay(2);
	}

	/*Disable clocks*/
	ret = wcss_clks_prepare_disable(wcss->dev, 1, NUM_WCSS_CLKS);
	if (ret) {
		dev_err(wcss->dev, "wcss clk(s) disable failed");
		return;
	}

	if (qcom_scm_is_available()) {
		cookie = 0;
		ret = qcom_scm_wcss_boot(Q6_BOOT_TRIG_SVC_ID,
						Q6_BOOT_TRIG_CMD_ID, &cookie);
		if (ret) {
			dev_err(wcss->dev, "q6-stop trigger scm failed\n");
			return;
		}
	} else
		/* Disbale Boot FSM */
		writel(0x0, wcss->tcsr_q6_boot_trig + 0x0);
}

static int q6v5_q6_powerdown(struct q6v5_wcss *wcss)
{
	int ret;
	u32 val;
	int i;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);

	/* 1 - Halt Q6 bus interface */
	if (!pdata->is_q6v6)
		q6v5_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_q6);
	else
		q6v6_wcss_halt_axi_port(wcss, wcss->halt_map, wcss->halt_q6);

	/* 2 - Disable Q6 Core clock */
	val = readl(wcss->reg_base + Q6SS_GFMUX_CTL_REG);
	val &= ~Q6SS_CLK_ENABLE;
	writel(val, wcss->reg_base + Q6SS_GFMUX_CTL_REG);

	if (pdata->is_q6v6) {
		q6v6_q6_powerdown(wcss);
		goto assert;
	}

	/* 3 - Clamp I/O */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_CLAMP_IO;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 4 - Clamp WL */
	val |= QDSS_BHS_ON;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 5 - Clear Erase standby */
	val &= ~Q6SS_L2DATA_STBY_N;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 6 - Clear Sleep RTN */
	val &= ~Q6SS_SLP_RET_N;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 7 - turn off Q6 memory foot/head switch one bank at a time */
	for (i = 0; i < 20; i++) {
		val = readl(wcss->reg_base + Q6SS_MEM_PWR_CTL);
		val &= ~BIT(i);
		writel(val, wcss->reg_base + Q6SS_MEM_PWR_CTL);
		mdelay(1);
	}

	/* 8 - Assert QMC memory RTN */
	val = readl(wcss->reg_base + Q6SS_PWR_CTL_REG);
	val |= Q6SS_CLAMP_QMC_MEM;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);

	/* 9 - Turn off BHS */
	val &= ~Q6SS_BHS_ON;
	writel(val, wcss->reg_base + Q6SS_PWR_CTL_REG);
	udelay(1);

	/* 10 - Wait till BHS Reset is done */
	ret = readl_poll_timeout(wcss->reg_base + Q6SS_BHS_STATUS,
				 val, !(val & BHS_EN_REST_ACK), 1000,
				 Q6SS_TIMEOUT_US * 10);
	if (ret) {
		dev_err(wcss->dev, "BHS_STATUS not OFF (rc:%d)\n", ret);
		return ret;
	}

assert:
	/* 11 -  Assert WCSS reset */
	reset_control_assert(wcss->wcss_reset);

	/* 12 - Assert Q6 reset */
	reset_control_assert(wcss->wcss_q6_reset);

	return 0;
}

static int q6v5_wcss_userpd_powerdown(struct rproc *rproc)
{
	int ret;
	struct q6v5_wcss *wcss_p = rproc->parent->priv, *wcss = rproc->priv;

	if (wcss->is_int_radio) {
		struct q6_platform_data *pdata = dev_get_platdata(wcss_p->dev);

		if (pdata->nosecure)
			goto skip_secure;

		ret = qcom_scm_int_radio_powerdown(MPD_WCNSS_PAS_ID);
		if (ret) {
			dev_err(wcss->dev, "failed to power down int radio\n");
			return ret;
		}
		goto success_msg;
skip_secure:
		/* WCSS powerdown */
		ret = q6v5_wcss_powerdown(wcss_p);
		if (ret) {
			dev_err(wcss_p->dev, "failed to power down wcss %d\n",
					ret);
			return ret;
		}

		/*Assert ce reset*/
		reset_control_assert(wcss_p->ce_reset);
		mdelay(2);

		ret = wcss_clks_prepare_disable(wcss_p->dev, 1, 1);
		if (ret) {
			dev_err(wcss_p->dev, "Failed to disable %s clk %d\n",
					wcss_clk_names[1], ret);
			return ret;
		}

		ret = wcss_clks_prepare_disable(wcss_p->dev, 3, 2);
		if (ret) {
			dev_err(wcss_p->dev, "Failed to disable clk's %d\n",
					ret);
			return ret;
		}

		/* Deassert WCSS reset */
		reset_control_deassert(wcss_p->wcss_reset);
success_msg:
		dev_info(&rproc->dev, "%s wcss powered down successfully\n",
								rproc->name);
	}
	return 0;
}

static int q6v5_wcss_userpd_stop(struct rproc *rproc)
{
	int ret = 0;
	struct q6v5_wcss *wcss = rproc->priv;

	/*send stop request, if it is not crashed*/
#ifdef CONFIG_CNSS2
	struct subsys_desc *desc = &wcss->q6v5.subsys_desc;
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(desc);

	if (!subsys_get_crash_status(q6v5->subsys)) {
#else
	if (rproc->state != RPROC_CRASHED) {
#endif
		ret = qcom_q6v5_request_stop(&wcss->q6v5);
		if (ret) {
			dev_err(&rproc->dev, "%s not stopped\n", rproc->name);
			BUG_ON(ret);
		}
	}

	ret = q6v5_wcss_userpd_powerdown(rproc);
	if (ret) {
		dev_err(&rproc->dev, "failed to powerdown %s\n", rproc->name);
		return ret;
	}
	wcss->is_radio_stopped = true;
	return ret;
}

static int q6v5_wcss_stop(struct rproc *rproc)
{
	struct q6v5_wcss *wcss = rproc->priv;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);
	int ret;
	u32 peripheral = pdata->is_mpd_arch ? MPD_WCNSS_PAS_ID : WCNSS_PAS_ID;

	if (pdata->nosecure)
		goto skip_secure;

	ret = qcom_scm_pas_shutdown(peripheral);
	if (ret) {
		dev_err(wcss->dev, "not able to shutdown\n");
		return ret;
	} else {
		qcom_q6v5_unprepare(&wcss->q6v5);
		return ret;
	}

skip_secure:
	/* WCSS powerdown */
	if (!pdata->is_mpd_arch) {
		ret = q6v5_wcss_powerdown(wcss);
		if (ret)
			return ret;
	}
	/* Q6 Power down */
	ret = q6v5_q6_powerdown(wcss);
	if (ret)
		return ret;

	qcom_q6v5_unprepare(&wcss->q6v5);

	return 0;
}

static int q6v5_register_dump_segments(struct rproc *rproc, const struct firmware *fw)
{
	/*
	 * Registering custom coredump function with a dummy dump segment as the
	 * dump regions are taken care by the dump function itself
	 */
	return rproc_coredump_add_custom_segment(rproc, 0, 0, crashdump_init, NULL);
}

static void *q6v5_wcss_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct q6v5_wcss *wcss = rproc->priv;
	int offset;

	offset = da - wcss->mem_reloc;
	if (offset < 0 || offset + len > wcss->mem_size)
		return NULL;

	return wcss->mem_region + offset;
}

static int share_bootargs_to_q6(struct device *dev)
{
	int ret;
	u32 smem_id, rd_val;
	const char *key = "qcom,bootargs_smem";
	size_t size;
	u16 cnt, tmp, version;
	void *ptr;
	u8 *bootargs_arr;
	struct device_node *np = dev->of_node;

	if (!device_property_read_bool(dev, "qcom,share_bootargs"))
		return 0;

	ret = of_property_read_u32(np, key, &smem_id);
	if (ret) {
		pr_err("failed to get smem id\n");
		return ret;
	}

	ret = qcom_smem_alloc(REMOTE_PID, smem_id,
					Q6_BOOT_ARGS_SMEM_SIZE);
	if (ret && ret != -EEXIST) {
		pr_err("failed to allocate q6 bootargs smem segment\n");
		return ret;
	}

	ptr = qcom_smem_get(1, smem_id, &size);
	if (IS_ERR(ptr)) {
		pr_err("Unable to acquire smp2p item(%d) ret:%ld\n",
					smem_id, PTR_ERR(ptr));
		return PTR_ERR(ptr);
	}

	/*Version*/
	version = VERSION;
	memcpy_toio(ptr, &version, sizeof(version));
	ptr += sizeof(version);

	cnt = ret = of_property_count_u32_elems(np, "boot-args");
	if (ret < 0) {
		pr_err("failed to read boot args ret:%d\n", ret);
		return ret;
	}

	/* No of elements */
	memcpy_toio(ptr, &cnt, sizeof(u16));
	ptr += sizeof(u16);

	bootargs_arr = kzalloc(cnt, GFP_KERNEL);
	if (!bootargs_arr) {
		pr_err("failed to allocate memory\n");
		return PTR_ERR(bootargs_arr);
	}

	for (tmp = 0; tmp < cnt; tmp++) {
		ret = of_property_read_u32_index(np, "boot-args", tmp, &rd_val);
		if (ret) {
			pr_err("failed to read boot args\n");
			kfree(bootargs_arr);
			return ret;
		}
		bootargs_arr[tmp] = (u8)rd_val;
	}

	/* Copy bootargs */
	memcpy_toio(ptr, bootargs_arr, cnt);
	ptr += (cnt);

	of_node_put(np);
	kfree(bootargs_arr);
	return ret;
}

static int q6v5_wcss_userpd_m3_load(struct rproc *p_rproc)
{
	struct q6v5_wcss *p_wcss = p_rproc->priv;
	struct rproc_child *rp_child;
	const struct firmware *m3_fw;
	int ret;

	list_for_each_entry(rp_child, &p_rproc->child, node) {
		struct rproc *child = (struct rproc *)rp_child->handle;
		struct q6v5_wcss *c_wcss = child->priv;

		if (!c_wcss->m3_fw_name)
			continue;

		ret = request_firmware(&m3_fw, c_wcss->m3_fw_name, p_wcss->dev);
		if (ret) {
			dev_info(c_wcss->dev, "skipping firmware %s\n",
							c_wcss->m3_fw_name);
			continue;
		}

		ret = qcom_mdt_load_no_init(p_wcss->dev, m3_fw,
				c_wcss->m3_fw_name, 0,
				p_wcss->mem_region, p_wcss->mem_phys,
				p_wcss->mem_size, &p_wcss->mem_reloc);

		release_firmware(m3_fw);

		if (ret) {
			dev_err(c_wcss->dev, "can't load %s\n",
						c_wcss->m3_fw_name);
			return ret;
		}
	}

	return 0;
}

static int q6v5_wcss_load(struct rproc *rproc, const struct firmware *fw)
{
	struct q6v5_wcss *wcss = rproc->priv;
	struct q6_platform_data *pdata = dev_get_platdata(wcss->dev);
	const struct firmware *m3_fw;
	int ret;
	u32 peripheral = pdata->is_mpd_arch ? MPD_WCNSS_PAS_ID : WCNSS_PAS_ID;
	struct rproc_child *rp_child;

	ret = share_bootargs_to_q6(wcss->dev);
	if (ret) {
		dev_err(wcss->dev,
			"boot args sharing with q6 failed %d\n",
			ret);
		return ret;
	}

	if (pdata->is_mpd_arch) {
		ret = q6v5_wcss_userpd_m3_load(rproc);
		if (ret) {
			dev_err(wcss->dev, "can't load userpd m3 firmware\n");
			return ret;
		}
		goto skip_m3;
	}

	if (!wcss->m3_fw_name) {
		dev_info(wcss->dev, "skipping firmware %s\n", "m3_fw.mdt");
		goto skip_m3;
	}

	ret = request_firmware(&m3_fw, wcss->m3_fw_name, wcss->dev);
	if (ret) {
		dev_info(wcss->dev, "skipping firmware %s\n", "m3_fw.mdt");
		goto skip_m3;
	}

	ret = qcom_mdt_load_no_init(wcss->dev, m3_fw, wcss->m3_fw_name, 0,
				    wcss->mem_region, wcss->mem_phys,
				    wcss->mem_size, &wcss->mem_reloc);

	release_firmware(m3_fw);

	if (ret) {
		dev_err(wcss->dev, "can't load %s\n", "m3_fw.bXX");
		return ret;
	}

skip_m3:
	if (pdata->nosecure)
		ret = qcom_mdt_load_no_init(wcss->dev, fw, rproc->firmware,
			     peripheral, wcss->mem_region, wcss->mem_phys,
			     wcss->mem_size, &wcss->mem_reloc);
	else
		ret = qcom_mdt_load(wcss->dev, fw, rproc->firmware,
			     peripheral, wcss->mem_region, wcss->mem_phys,
			     wcss->mem_size, &wcss->mem_reloc);

	list_for_each_entry(rp_child, &rproc->child, node) {
		struct rproc *child = (struct rproc *)rp_child->handle;
		struct q6v5_wcss *c_wcss = child->priv;

		ret = qcom_get_pd_segment_info(c_wcss->dev, fw,
			wcss->mem_phys, wcss->mem_size, c_wcss->pd_asid);
		c_wcss->is_radio_stopped = false;
	}

	return ret;
}

static int q6v5_wcss_userpd_load(struct rproc *rproc, const struct firmware *fw)
{
	struct rproc *p = rproc->parent;
	const struct firmware *f;
	struct q6v5_wcss *wcss_p = p->priv, *wcss = rproc->priv;
	struct q6_platform_data *pdata = dev_get_platdata(wcss_p->dev);
	int ret;
	u32 peripheral = pdata->is_mpd_arch ? MPD_WCNSS_PAS_ID : WCNSS_PAS_ID;

	dev_dbg(&rproc->dev, "%s:p:%p-p->name:%s-c:%p-c->name:%s-\n", __func__,
				p, p->name, rproc, rproc->name);

	if (wcss->is_radio_stopped == false)
		return 0;
	ret = request_firmware(&f, p->firmware, &p->dev);
	if (ret < 0) {
		dev_err(&p->dev, "%s: request_firmware failed: %d\n",
				__func__, ret);
		return ret;
	}

	if (pdata->nosecure)
		ret = qcom_mdt_load_no_init(wcss->dev, f, p->firmware,
			     peripheral, wcss_p->mem_region, wcss_p->mem_phys,
			     wcss_p->mem_size, &wcss_p->mem_reloc);
	else
		ret = qcom_mdt_load(wcss->dev, f, p->firmware,
			     peripheral, wcss_p->mem_region, wcss_p->mem_phys,
			     wcss_p->mem_size, &wcss_p->mem_reloc);
	return ret;
}

static const struct rproc_ops q6v5_wcss_userpd_ops = {
	.start = q6v5_wcss_userpd_start,
	.stop = q6v5_wcss_userpd_stop,
	.parse_fw = q6v5_register_dump_segments,
	.load = q6v5_wcss_userpd_load,
};

static const struct rproc_ops q6v5_wcss_ops = {
	.start = q6v5_wcss_start,
	.stop = q6v5_wcss_stop,
	.parse_fw = q6v5_register_dump_segments,
	.da_to_va = q6v5_wcss_da_to_va,
	.load = q6v5_wcss_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
};

static int q6v5_wcss_init_reset(struct q6v5_wcss *wcss)
{
	struct device *dev = wcss->dev;

	wcss->wcss_aon_reset = devm_reset_control_get(dev, "wcss_aon_reset");
	if (IS_ERR(wcss->wcss_aon_reset)) {
		dev_err(wcss->dev, "unable to acquire wcss_aon_reset\n");
		return PTR_ERR(wcss->wcss_aon_reset);
	}

	wcss->wcss_reset = devm_reset_control_get(dev, "wcss_reset");
	if (IS_ERR(wcss->wcss_reset)) {
		dev_err(wcss->dev, "unable to acquire wcss_reset\n");
		return PTR_ERR(wcss->wcss_reset);
	}

	wcss->wcss_q6_reset = devm_reset_control_get(dev, "wcss_q6_reset");
	if (IS_ERR(wcss->wcss_q6_reset)) {
		dev_err(wcss->dev, "unable to acquire wcss_q6_reset\n");
		return PTR_ERR(wcss->wcss_q6_reset);
	}

	if (of_property_read_bool(dev->of_node, "qcom,q6v6")) {
		wcss->ce_reset = devm_reset_control_get(dev, "ce_reset");
		if (IS_ERR(wcss->ce_reset)) {
			dev_err(wcss->dev, "unable to acquire ce_reset\n");
			return PTR_ERR(wcss->ce_reset);
		}
	}

	return 0;
}

static int q6v5_wcss_init_mmio(struct q6v5_wcss *wcss,
			       struct platform_device *pdev)
{
	struct of_phandle_args args;
	struct resource *res;
	int ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "qdsp6");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(&pdev->dev, "qdsp6 resource not available\n");
		return -EINVAL;
	}

	wcss->reg_base = ioremap(res->start, resource_size(res));
	if (IS_ERR(wcss->reg_base))
		return PTR_ERR(wcss->reg_base);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rmb");
	wcss->rmb_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(wcss->rmb_base))
		return PTR_ERR(wcss->rmb_base);

	if (of_property_read_bool(pdev->dev.of_node, "qcom,q6v6")) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mpm");
		wcss->mpm_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(wcss->mpm_base))
			return PTR_ERR(wcss->mpm_base);

		res = platform_get_resource_byname(pdev,
				IORESOURCE_MEM, "tcsr-msip");
		wcss->tcsr_msip_base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(wcss->tcsr_msip_base))
			return PTR_ERR(wcss->tcsr_msip_base);

		res = platform_get_resource_byname(pdev,
				IORESOURCE_MEM, "tcsr-q6-boot-trig");
		wcss->tcsr_q6_boot_trig = devm_ioremap_resource(&pdev->dev,
									res);
		if (IS_ERR(wcss->tcsr_q6_boot_trig))
			return PTR_ERR(wcss->tcsr_q6_boot_trig);
	}

	ret = of_parse_phandle_with_fixed_args(pdev->dev.of_node,
					       "qcom,halt-regs", 3, 0, &args);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to parse qcom,halt-regs\n");
		return -EINVAL;
	}

	wcss->halt_map = syscon_node_to_regmap(args.np);
	of_node_put(args.np);
	if (IS_ERR(wcss->halt_map))
		return PTR_ERR(wcss->halt_map);

	wcss->halt_q6 = args.args[0];
	wcss->halt_wcss = args.args[1];
	wcss->halt_nc = args.args[2];

	return 0;
}

static int q6v5_alloc_memory_region(struct q6v5_wcss *wcss)
{
	struct reserved_mem *rmem = NULL;
	struct device_node *node;
	struct device *dev = wcss->dev;

	node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (node)
		rmem = of_reserved_mem_lookup(node);
	of_node_put(node);

	if (!rmem) {
		dev_err(dev, "unable to acquire memory-region\n");
		return -EINVAL;
	}

	wcss->mem_phys = rmem->base;
	wcss->mem_reloc = rmem->base;
	wcss->mem_size = rmem->size;
	wcss->mem_region = devm_ioremap_wc(dev, wcss->mem_phys, wcss->mem_size);
	if (!wcss->mem_region) {
		dev_err(dev, "unable to map memory region: %pa+%pa\n",
			&rmem->base, &rmem->size);
		return -EBUSY;
	}

	return 0;
}

static void q6v5_release_resources(struct platform_device *pdev)
{
	struct rproc *rproc;
	struct q6v5_wcss *wcss;
	struct qcom_q6v5 *q6v5;
	struct rproc_child *rp_child, *tmp_child;

	rproc = platform_get_drvdata(pdev);
	wcss = rproc->priv;
	q6v5 = &wcss->q6v5;
	list_for_each_entry_safe(rp_child, tmp_child, &rproc->child, node) {
		struct rproc *child = (struct rproc *)rp_child->handle;
#ifdef CONFIG_CNSS2
		struct q6v5_wcss *c_wcss = child->priv;
		struct qcom_q6v5 *c_q6v5 =
			subsys_to_pdata(&c_wcss->q6v5.subsys_desc);
		subsys_unregister(c_q6v5->subsys);
#endif
		qcom_remove_ssr_subdev(child, &wcss->ssr_subdev);
		rproc_del(child);
		rproc_free(child);
	}

#ifdef CONFIG_CNSS2
	subsys_unregister(q6v5->subsys);
#endif
	rproc_del(rproc);
	qcom_remove_glink_subdev(rproc, &wcss->glink_subdev);
	qcom_remove_ssr_subdev(rproc, &wcss->ssr_subdev);
	rproc_free(rproc);
}

static int q6v5_register_userpd(struct platform_device *pdev)
{
	struct q6v5_wcss *wcss;
	struct rproc *rproc = NULL;
	int ret;
	struct qcom_q6v5 *q6v5;
	struct device_node *userpd_np;
	struct platform_device *userpd_pdev;
	struct rproc_child *rp_child;
#ifdef CONFIG_CNSS2
	struct q6v5_wcss *wcss_p;
	struct subsys_child *sub_child;
#endif
	for_each_available_child_of_node(pdev->dev.of_node, userpd_np) {
		if (strstr(userpd_np->name, "userpd") == NULL)
			continue;

		pr_info("%s(%p) node found\n", userpd_np->name, userpd_np);

		userpd_pdev = of_platform_device_create(userpd_np,
				userpd_np->name, &pdev->dev);
		if (!userpd_pdev) {
			pr_err("failed to create %s platform device\n",
							userpd_np->name);
			ret = -ENODEV;
			q6v5_release_resources(pdev);
			return ret;
		}

		rproc = rproc_alloc(&userpd_pdev->dev, userpd_pdev->name,
				&q6v5_wcss_userpd_ops, NULL, sizeof(*wcss));
		if (!rproc) {
			dev_err(&userpd_pdev->dev,
				"failed to allocate rproc\n");
			q6v5_release_resources(pdev);
			platform_device_unregister(userpd_pdev);
			return -ENOMEM;
		}
		kfree(rproc->firmware);
		rproc->firmware = NULL;

		wcss = rproc->priv;
		wcss->dev = &userpd_pdev->dev;
		q6v5 = &wcss->q6v5;
		rproc->parent = platform_get_drvdata(pdev);
		ret = qcom_q6v5_init(q6v5, userpd_pdev, rproc,
				WCSS_CRASH_REASON, NULL);
		if (ret)
			goto clear_resources;

#ifdef CONFIG_CNSS2
		wcss_p = rproc->parent->priv;
		q6v5->subsys_desc.parent = &wcss_p->q6v5.subsys_desc;
		/*
		 * subsys-register
		 */
		q6v5->subsys_desc.is_not_loadable = 0;
		q6v5->subsys_desc.name = userpd_pdev->dev.of_node->name;
		q6v5->subsys_desc.dev = &userpd_pdev->dev;
		q6v5->subsys_desc.owner = THIS_MODULE;
		q6v5->subsys_desc.shutdown = stop_q6_userpd;
		q6v5->subsys_desc.powerup = start_q6_userpd;
		q6v5->subsys_desc.ramdump = crashdump_init_new;
		q6v5->subsys_desc.err_fatal_handler = q6v5_fatal_interrupt;
		q6v5->subsys_desc.stop_ack_handler = q6v5_stop_interrupt;

		q6v5->subsys = subsys_register(&q6v5->subsys_desc);
		if (IS_ERR(q6v5->subsys)) {
			dev_err(&userpd_pdev->dev,
				"failed to register with ssr\n");
			ret = PTR_ERR(q6v5->subsys);
			goto clear_resources;
		}
		dev_info(wcss->dev, "ssr registeration success %s\n",
				q6v5->subsys_desc.name);
#endif
		rproc->auto_boot = false;
		ret = rproc_add(rproc);
		if (ret)
			goto clear_resources;

#ifdef CONFIG_CNSS2

		sub_child = devm_kzalloc(&userpd_pdev->dev,
				sizeof(*sub_child), GFP_KERNEL);
		if (IS_ERR_OR_NULL(sub_child)) {
			ret = PTR_ERR(sub_child);
			goto clear_resources;
		}
		sub_child->handle = &q6v5->subsys_desc;
		subsys_add_child(q6v5->subsys_desc.parent, sub_child);
#else
		rproc->is_parent_dependent = true;
#endif
		rp_child = devm_kzalloc(&userpd_pdev->dev,
				sizeof(*rp_child), GFP_KERNEL);
		if (IS_ERR_OR_NULL(rp_child)) {
			ret = PTR_ERR(rp_child);
			goto clear_resources;
		}
		rp_child->handle = rproc;
		rproc_add_child(rproc->parent, rp_child);
		wcss->is_int_radio = of_property_read_bool(userpd_np,
							"qca,int_radio");

		ret = of_property_read_u32(userpd_np, "qca,asid",
						&wcss->pd_asid);
		if (ret) {
			dev_err(wcss->dev, "failed to get asid\n");
			q6v5_release_resources(pdev);
			platform_device_unregister(userpd_pdev);
			return ret;
		}
		platform_set_drvdata(userpd_pdev, rproc);
		ret = of_property_read_string(userpd_np, "m3_firmware",
							&wcss->m3_fw_name);
		if (ret)
			wcss->m3_fw_name = NULL;
		qcom_add_ssr_subdev(rproc, &wcss->ssr_subdev, rproc->name);
	}

	return 0;
clear_resources:
	q6v5_release_resources(pdev);
	rproc_free(rproc);
	platform_device_unregister(userpd_pdev);
	return ret;
}

static int q6v5_wcss_probe(struct platform_device *pdev)
{
	struct q6v5_wcss *wcss;
	struct rproc *rproc;
	int ret;
	const char *firmware_name;
	struct q6_platform_data *pdata;
	struct qcom_q6v5 *q6v5;

	ret = of_property_read_string(pdev->dev.of_node, "firmware",
		&firmware_name);
	if (ret) {
		dev_err(&pdev->dev, "couldn't read firmware name: %d\n", ret);
		return ret;
	}

	rproc = rproc_alloc(&pdev->dev, pdev->name, &q6v5_wcss_ops,
			    firmware_name, sizeof(*wcss));
	if (!rproc) {
		dev_err(&pdev->dev, "failed to allocate rproc\n");
		return -ENOMEM;
	}

	wcss = rproc->priv;
	wcss->dev = &pdev->dev;

	ret = q6v5_wcss_init_mmio(wcss, pdev);
	if (ret)
		goto free_rproc;

	ret = q6v5_alloc_memory_region(wcss);
	if (ret)
		goto free_rproc;

	ret = q6v5_wcss_init_reset(wcss);
	if (ret)
		goto free_rproc;

	q6v5 = &wcss->q6v5;
	ret = qcom_q6v5_init(q6v5, pdev, rproc, WCSS_CRASH_REASON, NULL);
	if (ret)
		goto free_rproc;

#ifdef CONFIG_CNSS2
	/*
	 * subsys-register
	 */
	q6v5->subsys_desc.is_not_loadable = 0;
	q6v5->subsys_desc.name = pdev->dev.of_node->name;
	q6v5->subsys_desc.dev = &pdev->dev;
	q6v5->subsys_desc.owner = THIS_MODULE;
	q6v5->subsys_desc.shutdown = stop_q6;
	q6v5->subsys_desc.powerup = start_q6;
	q6v5->subsys_desc.ramdump = crashdump_init_new;
	q6v5->subsys_desc.crash_shutdown = q6v5_panic_handler;
	q6v5->subsys_desc.err_fatal_handler = q6v5_fatal_interrupt;
	q6v5->subsys_desc.stop_ack_handler = q6v5_ready_interrupt;
	q6v5->subsys_desc.wdog_bite_handler = q6v5_wdog_interrupt;

	q6v5->subsys = subsys_register(&q6v5->subsys_desc);
	if (IS_ERR(q6v5->subsys)) {
		dev_err(&pdev->dev, "failed to register with ssr\n");
		ret = PTR_ERR(q6v5->subsys);
		goto free_rproc;
	}
	q6v5->subsys_desc.parent = NULL;
	dev_info(wcss->dev, "ssr registeration success %s\n",
					q6v5->subsys_desc.name);
#endif
	rproc->auto_boot = false;
	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	ret = of_property_read_u32(pdev->dev.of_node, "qca,sec-reset-cmd",
				   &wcss->reset_cmd_id);
	if (ret)
		wcss->reset_cmd_id = QCOM_SCM_PAS_AUTH_DEBUG_RESET_CMD;

	wcss->wcss_aon_seq = of_property_read_bool(pdev->dev.of_node,
							"qca,wcss-aon-reset-seq");

	ret = of_property_read_string(pdev->dev.of_node, "m3_firmware",
					&wcss->m3_fw_name);
	if (ret)
		wcss->m3_fw_name = NULL;

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (!pdata) {
		ret = PTR_ERR(pdata);
		goto skip_pdata;
	}

	pdata->is_q6v6 = of_property_read_bool(pdev->dev.of_node, "qcom,q6v6");
	pdata->nosecure = of_property_read_bool(pdev->dev.of_node,
							"qcom,nosecure");
	pdata->emulation = of_property_read_bool(pdev->dev.of_node,
							"qcom,emulation");
	pdata->is_mpd_arch = of_property_read_bool(pdev->dev.of_node,
							"qcom,multipd_arch");

	platform_device_add_data(pdev, pdata, sizeof(*pdata));

skip_pdata:
	qcom_add_glink_subdev(rproc, &wcss->glink_subdev);
	qcom_add_ssr_subdev(rproc, &wcss->ssr_subdev, "mpss");
	platform_set_drvdata(pdev, rproc);
	rproc->parent = NULL;

	if (pdata && pdata->is_mpd_arch) {
		ret = q6v5_register_userpd(pdev);
		if (ret) {
			pr_err("Failed to register userpd\n");
			kfree(pdata);
			return ret;
		}
		kfree(pdata);
	}
	q6_rproc = rproc;
	return 0;

free_rproc:
	rproc_free(rproc);

	return ret;
}

static int q6v5_wcss_remove(struct platform_device *pdev)
{
	q6v5_release_resources(pdev);
	return 0;
}

static const struct of_device_id q6v5_wcss_of_match[] = {
	{ .compatible = "qcom,ipq8074-wcss-pil" },
	{ .compatible = "qcom,ipq60xx-wcss-pil" },
	{ .compatible = "qcom,ipq5018-wcss-pil" },
	{ },
};
MODULE_DEVICE_TABLE(of, q6v5_wcss_of_match);

static struct platform_driver q6v5_wcss_driver = {
	.probe = q6v5_wcss_probe,
	.remove = q6v5_wcss_remove,
	.driver = {
		.name = "qcom-q6v5-wcss-pil",
		.of_match_table = q6v5_wcss_of_match,
	},
};
module_platform_driver(q6v5_wcss_driver);
module_param(debug_wcss, int, 0644);
module_param(load_pil, int, 0644);

MODULE_DESCRIPTION("Hexagon WCSS Peripheral Image Loader");
MODULE_LICENSE("GPL v2");
