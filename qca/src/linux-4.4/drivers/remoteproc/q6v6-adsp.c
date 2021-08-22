/* Copyright (c) 2016, 2018 The Linux Foundation. All rights reserved.
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
#include <linux/timer.h>
#include <linux/stringify.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include "rproc_mdt_loader.h"

enum rproc_adsp_state {
	RPROC_Q6V6_STOPPED,
	RPROC_Q6V6_STARTING,
	RPROC_Q6V6_RUNNING,
	RPROC_Q6V6_STOPPING,
	RPROC_Q6V6_INVALID_STATE
};

static int debug_adsp;

/* registers to capture 0x0cd00000 to 0x0cd02028 */
#define Q6_REGISTER_SAVE_SIZE 0x202C

#define DEBUG_ADSP_BREAK_AT_START	1
#define DEBUG_ADSP_NO_RESTART		2
#define DEBUG_DUMP_Q6_REG		4

#define ADSP_CRASH_REASON_SMEM 423
#define ADSP_PAS_ID		1
#define STOP_ACK_TIMEOUT_MS 2000

#define QDSP6SS_RST_EVB 0x10
#define QDSP6SS_RESET 0x14
#define QDSP6SS_DBG_CFG 0x18
#define QDSP6SS_XO_CBCR 0x38
#define QDSP6SS_MEM_PWR_CTL 0xb0
#define QDSP6SS_SLEEP_CBCR 0x3C
#define QDSP6SS_CLOCK_SPDM_MON 0x48
#define QDSP6SS_BHS_STATUS 0x78
#define QDSP6SS_CP_CLK_CTL 0x508
#define LPASS_QDSP6SS_CORE_CBCR_ADDR 0x20
#define QDSP6SS_PWR_CTL 0x30
#define LPASS_QDSP6SS_CORE_CFG_RCGR 0x2c
#define LPASS_QDSP6SS_CORE_CMD_RCGR 0x28
#define LPASS_QDSP6SS_BOOT_CORE_START 0x400
#define LPASS_QDSP6SS_BOOT_CMD 0x404
#define LPASS_QDSP6SS_BOOT_STATUS 0x408

#define LPASS_AUDIO_CORE_BCR	0x8000
#define LPASS_AUDIO_CORE_BCR_SLP_CBCR	0x8004
#define LPASS_Q6SS_BCR	0xA000
#define LPASS_Q6SS_BCR_SLP_CBCR 0xA004
#define LPASS_AUDIO_CORE_GDSCR	0xB000
#define LPASS_AON_CMD_RCGR	0x14000
#define LPASS_AON_CFG_RCGR	0x14004
#define LPASS_AUDIO_WRAPPER_AON_CBCR	0x14098
#define LPASS_AUDIO_CORE_AUD_SLIMBUS_CORE_CBCR	0x17018
#define LPASS_AUDIO_CORE_QCA_SLIMBUS_CORE_CBCR	0x18018
#define LPASS_AUDIO_CORE_LPM_CORE_CBCR	0x1E000
#define LPASS_AUDIO_CORE_CORE_CBCR	0x1F000
#define LPASS_Q6SS_AHBM_AON_CBCR	0x26000
#define LPASS_Q6SS_ALT_RESET_AON_CBCR	0x2A000
#define LPASS_Q6SS_Q6_AXIM_CBCR		0x2D000
#define LPASS_Q6SS_AHBS_AON_CBCR	0x33000
#define LPASS_AUDIO_CORE_LPAIF_RXTX_WR_MEM_CBCR	0x40000
#define LPASS_AUDIO_CORE_LPAIF_RXTX_RD_MEM_CBCR	0x40004
#define LPASS_Q6SS_RST_EVB_SEL	0x8E000
#define LPASS_Q6SS_RST_EVB	0x8E004

#define TCSR_GLOBAL_CFG0 0x0
#define TCSR_GLOBAL_CFG1 0x4

#define TCSR_LPASS_PWR_ON			0x0
#define TCSR_LPASS2SNOC_HALTREQ			0x4
#define TCSR_LPASS_CORE_AXIM_HALTREQ		0x8
#define TCSR_LPASS2SNOC_HALTACK			0xC
#define TCSR_LPASS_CORE_AXIM_HALTACK		0x10
#define TCSR_LPASS2SNOC_MASTER_IDLE		0x14
#define TCSR_LPASS_CORE_AXIM_MASTER_IDLE	0x18
#define TCSR_LPASS2SNOC_MASTER_REQPEND		0x1C

#define SSCAON_CONFIG 0x8
#define SSCAON_STATUS 0xc
#define GCC_ADSPAON_RESET 0x10
#define GCC_ADSP_Q6_BCR 0x100
#define GCC_MISC_RESET_ADDR 0x8
#define HALTACK BIT(0)
#define BHS_EN_REST_ACK BIT(0)

#define EM_QDSP6 164

#define DEFAULT_IMG_ADDR 0x4b000000

struct q6v6_rtable {
	struct resource_table rtable;
	struct fw_rsc_hdr last_hdr;
};

static struct q6v6_rtable q6v6_rtable = {
	.rtable = {
		.ver = 1,
		.num = 0,
	},
	.last_hdr = {
		.type = RSC_LAST,
	},
};

struct q6v6_rproc_pdata {
	void __iomem *q6_base;
	void __iomem *gcc_bcr_base;
	void __iomem *q6ss_cc_base;
	void __iomem *tcsr_base;
	struct rproc *rproc;
	struct subsys_device *subsys;
	struct subsys_desc subsys_desc;
	struct completion stop_done;
	struct completion err_ready;
	unsigned int err_ready_irq;
	struct qcom_smem_state *state;
	unsigned stop_bit;
	unsigned shutdown_bit;
	atomic_t running;
	int emulation;
	int secure;
	int spurios_irqs;
	void *reg_save_buffer;
	unsigned int img_addr;
	u32 reset_cmd_id;
};

static struct q6v6_rproc_pdata *q6v6_rproc_pdata;

#define subsys_to_pdata(d) container_of(d, struct q6v6_rproc_pdata, subsys_desc)

#if defined(CONFIG_IPQ_SUBSYSTEM_DUMP)

#define	OPEN_TIMEOUT	5000
#define	DUMP_TIMEOUT	10000
#define	NUM_LPASS_CLKS	11

static struct timer_list dump_timeout;
static struct completion dump_complete;

static struct timer_list open_timeout;
static struct completion open_complete;
static atomic_t open_timedout;

static const struct file_operations q6_dump_ops;
static struct class *dump_class;

struct dump_file_private {
	int remaining_bytes;
	int rel_addr_off;
	int ehdr_remaining_bytes;
	char *ehdr;
	struct task_struct *pdesc;
};

static struct dumpdev {
	const char *name;
	const struct file_operations *fops;
	fmode_t fmode;
	char ss_name[8];
	u32 dump_phy_addr;
	u32 dump_size;
} q6dump = {"adsp_q6mem", &q6_dump_ops,
		FMODE_UNSIGNED_OFFSET | FMODE_EXCL, "lpass"};


static int lpass_clks_enable(struct device *dev)
{
	int ret, i;
	const char *lpass_clk_names[NUM_LPASS_CLKS] = {
						"mem_noc_lpass_clk",
						"snoc_lpass_cfg_clk",
						"lpass_core_axim_clk",
						"lpass_snoc_cfg_clk",
						"lpass_q6_axim_clk",
						"lpass_q6_atbm_at_clk",
						"lpass_q6_pclkdbg_clk",
						"lpass_q6ss_tsctr_1to2_clk",
						"lpass_q6ss_trig_clk",
						"lpass_tbu_clk",
						"pcnoc_lpass_clk"
					};
	struct clk *lpass_clks[NUM_LPASS_CLKS];

	for (i = 0; i < NUM_LPASS_CLKS; i++) {
		lpass_clks[i] = devm_clk_get(dev, lpass_clk_names[i]);
		if (IS_ERR(lpass_clks[i])) {
			pr_err("unable to get clock %s\n", lpass_clk_names[i]);
			return PTR_ERR(lpass_clks[i]);
		}

		ret = clk_prepare_enable(lpass_clks[i]);
		if (ret) {
			pr_err("unable to enable clock %s\n",
						lpass_clk_names[i]);
			return ret;
		}
	}

	return 0;
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

	del_timer_sync(&open_timeout);

	if (atomic_read(&open_timedout) == 1)
		return -ENODEV;

	file->f_mode |= q6dump.fmode;

	dfp = kzalloc(sizeof(struct dump_file_private), GFP_KERNEL);
	if (dfp == NULL) {
		pr_err("%s:\tCan not allocate memory for private structure\n",
				__func__);
		return -ENOMEM;
	}

	dfp->remaining_bytes = q6dump.dump_size;
	dfp->rel_addr_off = 0;
	dfp->pdesc = current;

	file->private_data = dfp;

	dump_timeout.data = (unsigned long)dfp;

	/* This takes care of the user space app stalls during delayed read. */
	init_completion(&dump_complete);

	setup_timer(&dump_timeout, dump_timeout_func, (unsigned long)dfp);
	mod_timer(&dump_timeout, jiffies + msecs_to_jiffies(DUMP_TIMEOUT));

	complete(&open_complete);

	return 0;
}

static int q6_dump_release(struct inode *inode, struct file *file)
{
	int dump_minor =  iminor(inode);
	int dump_major = imajor(inode);

	kfree(file->private_data);

	device_destroy(dump_class, MKDEV(dump_major, dump_minor));

	class_destroy(dump_class);

	complete(&dump_complete);

	return 0;
}

static ssize_t q6_dump_read(struct file *file, char __user *buf, size_t count,
		loff_t *ppos)
{
	void *buffer = NULL;
	size_t elfcore_hdrsize;
	Elf32_Phdr *phdr;
	Elf32_Ehdr *ehdr;
	int nsegments = 1;
	size_t count2 = 0;
	struct dump_file_private *dfp = (struct dump_file_private *)
		file->private_data;

	if (dump_timeout.data == -ETIMEDOUT)
		return 0;

	mod_timer(&dump_timeout, jiffies + msecs_to_jiffies(DUMP_TIMEOUT));

	if (dfp->ehdr == NULL) {
		elfcore_hdrsize = sizeof(*ehdr) + sizeof(*phdr) * nsegments;

		ehdr = kzalloc(elfcore_hdrsize, GFP_KERNEL);
		if (ehdr == NULL)
			return -ENOMEM;

		dfp->ehdr = (char *)ehdr;
		phdr = (Elf32_Phdr *)(ehdr + 1);

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

		phdr->p_type = PT_LOAD;
		phdr->p_offset = elfcore_hdrsize;
		phdr->p_vaddr = phdr->p_paddr = q6dump.dump_phy_addr;
		phdr->p_filesz = phdr->p_memsz = q6dump.dump_size;
		phdr->p_flags = PF_R | PF_W | PF_X;

		dfp->ehdr_remaining_bytes = elfcore_hdrsize;
	}

	if (dfp->ehdr_remaining_bytes) {
		if (count > dfp->ehdr_remaining_bytes) {
			count2 = dfp->ehdr_remaining_bytes;
			copy_to_user(buf, dfp->ehdr + *ppos, count2);
			buf += count2;
			dfp->ehdr_remaining_bytes -= count2;
			count -= count2;
			kfree(dfp->ehdr);
		} else {
			copy_to_user(buf, dfp->ehdr + *ppos, count);
			dfp->ehdr_remaining_bytes -= count;
			if (!dfp->ehdr_remaining_bytes)
				kfree(dfp->ehdr);
			*ppos = *ppos + count;
			return count;
		}
	}

	if (count > dfp->remaining_bytes)
		count = dfp->remaining_bytes;

	if (dfp->rel_addr_off < q6dump.dump_size) {
		buffer = ioremap(q6dump.dump_phy_addr + dfp->rel_addr_off,
				count);
		if (!buffer) {
			pr_err("can not map physical address %x : %zd\n",
					(unsigned int)q6dump.dump_phy_addr +
					dfp->rel_addr_off, count);
			return -ENOMEM;
		}
		dfp->rel_addr_off = dfp->rel_addr_off + count;
		copy_to_user(buf, buffer, count);
	} else
		return 0;

	dfp->remaining_bytes = dfp->remaining_bytes - count;

	iounmap(buffer);

	*ppos = *ppos + count + count2;
	return count + count2;
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

static int crashdump_init(int check, const struct subsys_desc *desc)
{
	int ret = 0;
	int dump_major = 0;
	struct device *dump_dev = NULL;
	struct device_node *node = NULL;

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

	node = of_find_node_by_name(NULL, q6dump.ss_name);
	if (node == NULL) {
		ret = -ENODEV;
		goto dump_dev_failed;
	}

	ret = of_property_read_u32_index(node, "reg", 1, &q6dump.dump_phy_addr);
	if (ret) {
		pr_err("could not retrieve reg property: %d\n", ret);
		goto dump_dev_failed;
	}

	ret = of_property_read_u32_index(node, "reg", 3, &q6dump.dump_size);
	if (ret) {
		pr_err("could not retrieve reg property: %d\n",
				ret);
		goto dump_dev_failed;
	}

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

static struct resource_table *q6v6_find_loaded_rsc_table(struct rproc *rproc,
	const struct firmware *fw)
{
	q6v6_rtable.rtable.offset[0] = sizeof(struct resource_table);
	return &(q6v6_rtable.rtable);
}

static void adsp_powerdown(struct q6v6_rproc_pdata *pdata)
{
	unsigned long val = 0;

	/* 1. Enable the LPASS core clocks for the LPASS buses*/
	val = readl(pdata->q6ss_cc_base + LPASS_AUDIO_CORE_CORE_CBCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_AUDIO_CORE_CORE_CBCR);

	/* 2. The DSP must vote to keep the LPASS core in a
	powered on state while applying the BCR reset*/
	val = readl(pdata->q6ss_cc_base + LPASS_AUDIO_CORE_GDSCR);
	val &= ~(BIT(0));
	writel(val, pdata->q6ss_cc_base + LPASS_AUDIO_CORE_GDSCR);

	/* 3. Set the LPASS core BCR to 1 */
	val = readl(pdata->q6ss_cc_base + LPASS_AUDIO_CORE_BCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_AUDIO_CORE_BCR);

	mdelay(10);
	/* 4. wait till LPASS_AUDIO_CORE_BCR reg value becomes 1 */
	do {
		mdelay(10);
		val = readl(pdata->q6ss_cc_base + LPASS_AUDIO_CORE_BCR);
		val &= 0x1;
	} while(val != 1);

	/* 5. clears the BCR register*/
	val = readl(pdata->q6ss_cc_base + LPASS_AUDIO_CORE_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->q6ss_cc_base + LPASS_AUDIO_CORE_BCR);

	mdelay(10);
	return;
}

static void q6_powerdown(struct q6v6_rproc_pdata *pdata)
{
	unsigned int nretry = 0;
	unsigned long val = 0;

	/* To retain power domain after q6 powerdown */
	writel(0x1, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* Halt Q6 bus interface */
	writel(0x1, pdata->tcsr_base + TCSR_LPASS2SNOC_HALTREQ);

	nretry = 0;
	while (1) {
		val = readl(pdata->tcsr_base + TCSR_LPASS2SNOC_HALTACK);
		if (val & HALTACK)
			break;
		mdelay(1);
		nretry++;
		if (nretry >= 10) {
			pr_err("%s: can't get TCSR Q6 haltACK\n", __func__);
			break;
		}
	}

	/*HVX*/
	val = readl(pdata->q6_base + QDSP6SS_CP_CLK_CTL);
	val &= ~(BIT(0));
	writel(val, pdata->q6_base + QDSP6SS_CP_CLK_CTL);

	/*  enable the sleep clock branch*/
	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_BCR_SLP_CBCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_BCR_SLP_CBCR);

	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_BCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_BCR);

	/* Wait till BCR Reset is done */
	nretry = 0;
	while (1) {
		val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_BCR);
		if (val != 0)
			break;
		mdelay(1);
		nretry++;
		if (nretry >= 10) {
			pr_err("BHS_STATUS not OFF\n");
			break;
		}
	}
	mdelay(50);

	/*remove master port halting by setting HALTREQ to 0*/
	writel(0x0, pdata->tcsr_base + TCSR_LPASS2SNOC_HALTREQ);

	/*clear the BCR.*/
	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_BCR);
	val &= ~(BIT(0));
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_BCR);
	return;
}

static void save_adsp_regs(struct q6v6_rproc_pdata *pdata)
{
	int i = 0;
	unsigned int *buffer = NULL;

	if (!(debug_adsp & DEBUG_DUMP_Q6_REG))
		return;

	buffer = (unsigned int *)(q6v6_rproc_pdata->reg_save_buffer);

	for (i = 0; i < Q6_REGISTER_SAVE_SIZE; i += 0x4) {
		*buffer = readl(q6v6_rproc_pdata->q6_base + i);
		buffer++;
	}
}

static int q6_rproc_stop(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v6_rproc_pdata *pdata = platform_get_drvdata(pdev);
	int ret = 0;
	unsigned int state;

	state = atomic_read(&pdata->running);

	if ((state == RPROC_Q6V6_RUNNING) || (state == RPROC_Q6V6_STARTING)) {
		atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_STOPPING);
		if (pdata->secure) {
			ret = qcom_scm_pas_shutdown(ADSP_PAS_ID);
			if (ret)
				dev_err(dev, "failed to shutdown %d\n", ret);
			atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_STOPPED);
			return ret;
		}

		/* Non secure */
		/* Print registers for debug of powerdown issue */
		save_adsp_regs(pdata);
		/* ADSP powerdown */
		adsp_powerdown(pdata);

		/* Q6 Power down */
		q6_powerdown(pdata);
		atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_STOPPED);
	}

	return ret;
}

static int wait_for_err_ready(struct q6v6_rproc_pdata *pdata)
{
	int ret;
	pr_err("%s entry\n", __func__);
wait_again:
	ret = wait_for_completion_timeout(&pdata->err_ready,
					  msecs_to_jiffies(10000));
	if (!ret) {
		if ((debug_adsp & DEBUG_ADSP_BREAK_AT_START))
			goto wait_again;
		pr_err("[%s]: Error ready timed out\n",
				pdata->subsys_desc.name);
		return -ETIMEDOUT;
	}

	return 0;
}

static int q6_rproc_emu_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v6_rproc_pdata *pdata = platform_get_drvdata(pdev);
	unsigned long val = 0;

	atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_STARTING);

	/* 1. enable LPASS XO clock */
	val = readl(pdata->q6_base + QDSP6SS_XO_CBCR);
	val |= 0x1;
	writel(val, pdata->q6_base + QDSP6SS_XO_CBCR);

	/* 2. enable LPASS core clock */
	writel(0x1, pdata->q6_base + LPASS_QDSP6SS_CORE_CBCR_ADDR);

	/* 3. enable LPASS sleep clock */
	val = readl(pdata->q6_base + QDSP6SS_SLEEP_CBCR);
	val |= 0x1;
	writel(val, pdata->q6_base + QDSP6SS_SLEEP_CBCR);

	/* 4. configure root clock generator(RCG) configuration register */
	val = readl(pdata->q6_base + LPASS_QDSP6SS_CORE_CFG_RCGR);
	val &= 0xFFFFF8E0;
	writel(val, pdata->q6_base + LPASS_QDSP6SS_CORE_CFG_RCGR);

	/* 5. update the RCG */
	val = readl(pdata->q6_base + LPASS_QDSP6SS_CORE_CMD_RCGR);
	val |= 0x1;
	writel(val, pdata->q6_base + LPASS_QDSP6SS_CORE_CMD_RCGR);

	pr_debug("Enable_Talos_Turing And GCC_clks complete\n");

	/* 6. set boot address in EVB so that Q6LPASS will jump there after reset */
	writel(pdata->img_addr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);

	/* debug: break at start */
	if (debug_adsp & DEBUG_ADSP_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* debug: Retain debugger state during next QDSP6 reset */
	if (!(debug_adsp & DEBUG_ADSP_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* 7. start LPASS QDSP */
	writel(0x1, pdata->q6_base + LPASS_QDSP6SS_BOOT_CORE_START);

	/* 8. start boot FSM */
	writel(0x1, pdata->q6_base + LPASS_QDSP6SS_BOOT_CMD);

	/* 9. wait till boot FSM completes */
	do {
		mdelay(10);
		val = readl(pdata->q6_base + LPASS_QDSP6SS_BOOT_STATUS);
	} while(val != 1);

	atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_RUNNING);
	pr_emerg("LPASS Q6 Emulation reset out is done\n");

	return 0;
}

static int q6_rproc_start(struct rproc *rproc)
{
	struct device *dev = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev);
	struct q6v6_rproc_pdata *pdata = platform_get_drvdata(pdev);
	unsigned long val = 0;
	int ret = 0;

	atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_STARTING);
	if (pdata->secure) {
		ret = qcom_scm_pas_auth_and_reset(ADSP_PAS_ID,
			(debug_adsp & DEBUG_ADSP_BREAK_AT_START),
				pdata->reset_cmd_id);
		if (ret) {
			dev_err(dev, "q6-adsp reset failed\n");
			return ret;
		}
		goto skip_reset;
	}
	/*  configure root clock generator(RCG) configuration register */
	val = readl(pdata->q6_base + LPASS_QDSP6SS_CORE_CFG_RCGR);
	val &= 0xFFFFF8E0;
	writel(val, pdata->q6_base + LPASS_QDSP6SS_CORE_CFG_RCGR);

	/*  update the RCG */
	val = readl(pdata->q6_base + LPASS_QDSP6SS_CORE_CMD_RCGR);
	val |= 0x1;
	writel(val, pdata->q6_base + LPASS_QDSP6SS_CORE_CMD_RCGR);

	/*  enable LPASS SLP clock */
	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_BCR_SLP_CBCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_BCR_SLP_CBCR);

	/*Configure AON bus to use XO*/
	val = readl(pdata->q6ss_cc_base + LPASS_AON_CFG_RCGR);
	val &= ~(GENMASK(10, 8));
	writel(0x0, pdata->q6ss_cc_base + LPASS_AON_CFG_RCGR);

	val = readl(pdata->q6ss_cc_base + LPASS_AON_CMD_RCGR);
	val |= 1;
	writel(val, pdata->q6ss_cc_base + LPASS_AON_CMD_RCGR);

	/*AHBS*/
	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_AHBS_AON_CBCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_AHBS_AON_CBCR);

	/*AHBM*/
	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_AHBM_AON_CBCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_AHBM_AON_CBCR);

	/*AXIM*/
	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_Q6_AXIM_CBCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_Q6_AXIM_CBCR);

	/*Enable core CBCR*/
	val = readl(pdata->q6_base + QDSP6SS_XO_CBCR);
	val |= 0x1;
	writel(val, pdata->q6_base + QDSP6SS_XO_CBCR);

	/*  enable LPASS sleep clock */
	val = readl(pdata->q6_base + QDSP6SS_SLEEP_CBCR);
	val |= 0x1;
	writel(val, pdata->q6_base + QDSP6SS_SLEEP_CBCR);

	/*  enable LPASS core clock */
	writel(0x1, pdata->q6_base + LPASS_QDSP6SS_CORE_CBCR_ADDR);

	pr_debug("Enable_Talos_Turing And GCC_clks complete\n");

	/*Turn on aon wrapper clock and alt reset*/
	val = readl(pdata->q6ss_cc_base + LPASS_AUDIO_WRAPPER_AON_CBCR);
	val |= 1;
	writel(val, pdata->q6ss_cc_base + LPASS_AUDIO_WRAPPER_AON_CBCR);

	val = readl(pdata->q6ss_cc_base + LPASS_Q6SS_ALT_RESET_AON_CBCR);
	val |= 0x1;
	writel(val, pdata->q6ss_cc_base + LPASS_Q6SS_ALT_RESET_AON_CBCR);

	/*HVX clock*/
	val = readl(pdata->q6_base + QDSP6SS_CP_CLK_CTL);
	val |= 1;
	writel(val, pdata->q6_base + QDSP6SS_CP_CLK_CTL);

	/*  set boot address in EVB so that Q6LPASS will jump there after reset */
	writel(rproc->bootaddr >> 4, pdata->q6_base + QDSP6SS_RST_EVB);

	/* debug: break at start */
	if (debug_adsp & DEBUG_ADSP_BREAK_AT_START)
		writel(0x20000001, pdata->q6_base + QDSP6SS_DBG_CFG);

	/* debug: Retain debugger state during next QDSP6 reset */
	if (!(debug_adsp & DEBUG_ADSP_BREAK_AT_START))
		writel(0x0, pdata->q6_base + QDSP6SS_DBG_CFG);

	/*  start LPASS QDSP */
	writel(0x1, pdata->q6_base + LPASS_QDSP6SS_BOOT_CORE_START);

	/*  start boot FSM */
	writel(0x1, pdata->q6_base + LPASS_QDSP6SS_BOOT_CMD);

	/*  wait till boot FSM completes */
	do {
		mdelay(10);
		val = readl(pdata->q6_base + LPASS_QDSP6SS_BOOT_STATUS);
	} while(val != 1);

skip_reset:

	ret = wait_for_err_ready(pdata);
	if (!ret)
		atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_RUNNING);

	return 0;

}

static void *q6_da_to_va(struct rproc *rproc, u64 da, int len)
{
	unsigned long addr = (__force unsigned long)(da & 0xFFFFFFFF);

	return ioremap(addr, len);
}

static char *rproc_q6_state(enum rproc_adsp_state state)
{
	switch (state) {
		case RPROC_Q6V6_STOPPED:
			return __stringify(RPROC_Q6V6_STOPPED);
		case RPROC_Q6V6_STARTING:
			return __stringify(RPROC_Q6V6_STARTING);
		case RPROC_Q6V6_RUNNING:
			return __stringify(RPROC_Q6V6_RUNNING);
		case RPROC_Q6V6_STOPPING:
			return __stringify(RPROC_Q6V6_STOPPING);
		default:
			return __stringify(RPROC_Q6V6_INVALID_STATE);
	}
}

static irqreturn_t adsp_err_ready_intr_handler(int irq, void *dev_id)
{
	struct q6v6_rproc_pdata *pdata = dev_id;
	unsigned int state;

	state = atomic_read(&pdata->running);

	if (state != RPROC_Q6V6_STARTING) {
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

static irqreturn_t adsp_err_fatal_intr_handler(int irq, void *dev_id)
{
	struct q6v6_rproc_pdata *pdata = subsys_to_pdata(dev_id);
	char *msg;
	size_t len;
	unsigned int state;

	state = atomic_read(&pdata->running);
	if ((state != RPROC_Q6V6_RUNNING) && (state != RPROC_Q6V6_STOPPING)) {
		pdata->spurios_irqs++;
		pr_emerg("ERROR: smp2p %s in wrong state %s (%d)\n",
				__func__,
				rproc_q6_state(state),
				pdata->spurios_irqs);
		return IRQ_HANDLED;
	}

	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, ADSP_CRASH_REASON_SMEM, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		pr_err("Fatal error received from adsp software!: %s\n", msg);
	else
		pr_err("Fatal error received no message!\n");

	if (debug_adsp & DEBUG_ADSP_NO_RESTART) {
		pr_info("ADSP Restart is disabled, Ignoring fatal error.\n");
		return IRQ_HANDLED;
	}

	subsys_set_crash_status(pdata->subsys, CRASH_STATUS_ERR_FATAL);
	subsystem_restart_dev(pdata->subsys);
	return IRQ_HANDLED;
}

static irqreturn_t adsp_stop_ack_intr_handler(int irq, void *dev_id)
{
	struct q6v6_rproc_pdata *pdata = subsys_to_pdata(dev_id);
	unsigned int state;

	state = atomic_read(&pdata->running);
	if (state != RPROC_Q6V6_RUNNING) {
		pdata->spurios_irqs++;
		pr_emerg("ERROR: smp2p %s in wrong state %s (%d)\n",
				__func__,
				rproc_q6_state(state),
				pdata->spurios_irqs);
		return IRQ_HANDLED;
	}

	pr_info("Received stop ack interrupt from adsp\n");
	complete(&pdata->stop_done);
	return IRQ_HANDLED;
}

static irqreturn_t adsp_wdog_bite_intr_handler(int irq, void *dev_id)
{
	struct q6v6_rproc_pdata *pdata = subsys_to_pdata(dev_id);
	char *msg;
	size_t len;
	unsigned int state;

	state = atomic_read(&pdata->running);
	if (state != RPROC_Q6V6_RUNNING) {
		pdata->spurios_irqs++;
		pr_emerg("ERROR: smp2p %s in wrong state %s (%d)\n",
				__func__,
				rproc_q6_state(state),
				pdata->spurios_irqs);
		complete(&pdata->stop_done);
		return IRQ_HANDLED;
	}

	msg = qcom_smem_get(QCOM_SMEM_HOST_ANY, ADSP_CRASH_REASON_SMEM, &len);
	if (!IS_ERR(msg) && len > 0 && msg[0])
		pr_err("Watchdog bite received from adsp software!: %s\n", msg);
	else
		pr_err("Watchdog bit received no message!\n");

	if (debug_adsp & DEBUG_ADSP_NO_RESTART) {
		pr_info("ADSP Restart is disabled, Ignoring WDOG Bite.\n");
		return IRQ_HANDLED;
	}

	subsys_set_crash_status(pdata->subsys, CRASH_STATUS_WDOG_BITE);
	subsystem_restart_dev(pdata->subsys);

	return IRQ_HANDLED;
}


static int start_q6(const struct subsys_desc *subsys)
{
	struct q6v6_rproc_pdata *pdata = subsys_to_pdata(subsys);
	struct rproc *rproc = pdata->rproc;
	int ret = 0;

	reinit_completion(&pdata->stop_done);
	reinit_completion(&pdata->err_ready);
	pdata->subsys_desc.ramdump_disable = 1;

	if (pdata->emulation) {
		pr_emerg("q6v6: Emulation start, no smp2p messages\n");
		q6_rproc_emu_start(rproc);
		return 0;
	}

	ret = rproc_boot(rproc);
	if (ret) {
		pr_err("couldn't boot q6v6: %d\n", ret);
	}

	return ret;
}

static int stop_q6(const struct subsys_desc *subsys, bool force_stop)
{
	struct q6v6_rproc_pdata *pdata = subsys_to_pdata(subsys);
	struct rproc *rproc = pdata->rproc;
	int ret = 0;

	if (!subsys_get_crash_status(pdata->subsys) && !force_stop) {
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
		pr_emerg("q6v6: Emulation stop, no smp2p messages\n");
		return 0;
	}

	rproc_shutdown(rproc);

	return 0;
}

static void adsp_panic_handler(const struct subsys_desc *subsys)
{
	struct q6v6_rproc_pdata *pdata = subsys_to_pdata(subsys);

	atomic_set(&pdata->running, RPROC_Q6V6_STOPPING);

	if (!subsys_get_crash_status(pdata->subsys)) {
		qcom_smem_state_update_bits(pdata->state,
			BIT(pdata->shutdown_bit), BIT(pdata->shutdown_bit));
		mdelay(STOP_ACK_TIMEOUT_MS);
	}
	return;
}

static int q6v6_load(struct rproc *rproc, const struct firmware *fw)
{
	int ret = 0;
	struct device *dev_rproc = rproc->dev.parent;
	struct platform_device *pdev = to_platform_device(dev_rproc);
	struct q6v6_rproc_pdata *pdata = platform_get_drvdata(pdev);

	if (pdata->secure) {
		ret = qcom_scm_pas_init_image(ADSP_PAS_ID, fw->data, fw->size);
		if (ret) {
			dev_err(dev_rproc, "image authentication failed\n");
			return ret;
		}
	}
	pr_info("Sanity check passed for the image\n");

	return mdt_load(rproc, fw);
}

static struct rproc_ops q6v6_rproc_ops = {
	.start          = q6_rproc_start,
	.da_to_va       = q6_da_to_va,
	.stop           = q6_rproc_stop,
	.find_loaded_rsc_table = q6v6_find_loaded_rsc_table,
	.load = q6v6_load,
	.get_boot_addr = rproc_elf_get_boot_addr,
};

static int q6_rproc_probe(struct platform_device *pdev)
{
	const char *firmware_name;
	struct rproc *rproc;
	int ret;
	struct resource *resource;
	struct qcom_smem_state *state;
	unsigned stop_bit;

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

	rproc = rproc_alloc(&pdev->dev, "q6v6-adsp", &q6v6_rproc_ops,
				firmware_name,
				sizeof(*q6v6_rproc_pdata));
	if (unlikely(!rproc))
		return -ENOMEM;

	q6v6_rproc_pdata = rproc->priv;
	q6v6_rproc_pdata->rproc = rproc;
	q6v6_rproc_pdata->emulation = of_property_read_bool(pdev->dev.of_node,
					"qca,emulation");
	if (q6v6_rproc_pdata->emulation) {
		q6v6_rproc_pdata->img_addr = DEFAULT_IMG_ADDR;
		ret = of_property_read_u32(pdev->dev.of_node, "img-addr",
						&q6v6_rproc_pdata->img_addr);
	}

	q6v6_rproc_pdata->secure = of_property_read_bool(pdev->dev.of_node,
					"qca,secure");
	if(of_property_read_bool(pdev->dev.of_node, "qca,dump-q6-reg"))
		debug_adsp |= DEBUG_DUMP_Q6_REG;

	q6v6_rproc_pdata->spurios_irqs = 0;
	rproc->has_iommu = false;
	/* We will record the values before q6 and adsp powerdown */
	q6v6_rproc_pdata->reg_save_buffer = kzalloc((Q6_REGISTER_SAVE_SIZE * 4),
							GFP_KERNEL);
	if (q6v6_rproc_pdata->reg_save_buffer == NULL)
		goto free_rproc;

	if (!q6v6_rproc_pdata->secure && lpass_clks_enable(&pdev->dev))
		goto free_rproc;

	ret = of_property_read_u32(pdev->dev.of_node, "qca,sec-reset-cmd",
					&q6v6_rproc_pdata->reset_cmd_id);
	if (ret)
		q6v6_rproc_pdata->reset_cmd_id = QCOM_SCM_PAS_AUTH_DEBUG_RESET_CMD;

	if (!q6v6_rproc_pdata->secure) {
		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"adsp-base");
		if (unlikely(!resource))
			goto free_rproc;

		q6v6_rproc_pdata->q6_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v6_rproc_pdata->q6_base)
			goto free_rproc;

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"gcc-adsp-bcr-base");
		if (unlikely(!resource))
			goto free_rproc;

		q6v6_rproc_pdata->gcc_bcr_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v6_rproc_pdata->gcc_bcr_base)
			goto free_rproc;

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"adsp-cc-base");
		if (unlikely(!resource))
			goto free_rproc;

		q6v6_rproc_pdata->q6ss_cc_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v6_rproc_pdata->q6ss_cc_base)
			goto free_rproc;

		resource = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						"tcsr-base");
		if (unlikely(!resource))
			goto free_rproc;

		q6v6_rproc_pdata->tcsr_base = ioremap(resource->start,
				resource_size(resource));
		if (!q6v6_rproc_pdata->tcsr_base)
			goto free_rproc;

	}

	platform_set_drvdata(pdev, q6v6_rproc_pdata);

	q6v6_rproc_pdata->err_ready_irq = platform_get_irq_byname(pdev,
						"qcom,gpio-err-ready");
	if (q6v6_rproc_pdata->err_ready_irq < 0) {
		pr_err("Can't get err-ready irq number %d\n",
			q6v6_rproc_pdata->err_ready_irq);
		goto free_rproc;
	}
	ret = devm_request_threaded_irq(&pdev->dev,
				q6v6_rproc_pdata->err_ready_irq,
				NULL, adsp_err_ready_intr_handler,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"error_ready_interrupt", q6v6_rproc_pdata);
	if (ret < 0) {
		pr_err("Can't register err ready handler\n");
		goto free_rproc;
	}

	q6v6_rproc_pdata->state = qcom_smem_state_get(&pdev->dev, "stop",
			&q6v6_rproc_pdata->stop_bit);
	if (IS_ERR(q6v6_rproc_pdata->state)) {
		pr_err("Can't get stop bit status fro SMP2P\n");
		goto free_rproc;
	}

	q6v6_rproc_pdata->state = qcom_smem_state_get(&pdev->dev, "shutdown",
			&q6v6_rproc_pdata->shutdown_bit);
	if (IS_ERR(q6v6_rproc_pdata->state)) {
		pr_err("Can't get shutdown bit status fro SMP2P\n");
		goto free_rproc;
	}

	q6v6_rproc_pdata->subsys_desc.is_not_loadable = 0;
	q6v6_rproc_pdata->subsys_desc.name = "q6v6-adsp";
	q6v6_rproc_pdata->subsys_desc.dev = &pdev->dev;
	q6v6_rproc_pdata->subsys_desc.owner = THIS_MODULE;
	q6v6_rproc_pdata->subsys_desc.shutdown = stop_q6;
	q6v6_rproc_pdata->subsys_desc.powerup = start_q6;
	q6v6_rproc_pdata->subsys_desc.ramdump = crashdump_init;
	q6v6_rproc_pdata->subsys_desc.crash_shutdown = adsp_panic_handler;
	q6v6_rproc_pdata->subsys_desc.err_fatal_handler =
				adsp_err_fatal_intr_handler;
	q6v6_rproc_pdata->subsys_desc.stop_ack_handler =
				adsp_stop_ack_intr_handler;
	q6v6_rproc_pdata->subsys_desc.wdog_bite_handler =
				adsp_wdog_bite_intr_handler;

	q6v6_rproc_pdata->subsys = subsys_register(
					&q6v6_rproc_pdata->subsys_desc);
	if (IS_ERR(q6v6_rproc_pdata->subsys)) {
		ret = PTR_ERR(q6v6_rproc_pdata->subsys);
		goto free_rproc;
	}

	init_completion(&q6v6_rproc_pdata->stop_done);
	init_completion(&q6v6_rproc_pdata->err_ready);
	atomic_set(&q6v6_rproc_pdata->running, RPROC_Q6V6_STOPPED);

	rproc->auto_boot = false;
	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	return 0;

free_rproc:
	if (q6v6_rproc_pdata->q6ss_cc_base)
		iounmap(q6v6_rproc_pdata->q6ss_cc_base);

	if (q6v6_rproc_pdata->gcc_bcr_base)
		iounmap(q6v6_rproc_pdata->gcc_bcr_base);

	if (q6v6_rproc_pdata->q6_base)
		iounmap(q6v6_rproc_pdata->q6_base);

	rproc_free(rproc);
	return -EIO;
}

static int q6_rproc_remove(struct platform_device *pdev)
{
	struct q6v6_rproc_pdata *pdata;
	struct rproc *rproc;

	pdata = platform_get_drvdata(pdev);
	rproc = q6v6_rproc_pdata->rproc;

	rproc_del(rproc);
	rproc_free(rproc);

	subsys_unregister(pdata->subsys);

	return 0;
}

static const struct of_device_id q6_match_table[] = {
	{ .compatible = "qca,q6v6-adsp-rproc" },
	{}
};

static struct platform_driver q6_rproc_driver = {
	.probe = q6_rproc_probe,
	.remove = q6_rproc_remove,
	.driver = {
		.name = "q6v6-adsp",
		.of_match_table = q6_match_table,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(q6_rproc_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("QCA Remote Processor control driver");

module_param(debug_adsp, int, 0644);
