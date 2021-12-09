// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
 *
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/mhi.h>
#include <linux/module.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/of.h>
#include "internal.h"

#define QRTR_INSTANCE_MASK	0x0000FFFF
#define QRTR_INSTANCE_SHIFT	0

#define MAX_RAMDUMP_TABLE_SIZE 6
#define COREDUMP_DESC	"Q6-COREDUMP"
#define Q6_SFR_DESC	"Q6-SFR"

typedef struct
{
	uint64_t base_address;
	uint64_t actual_phys_address;
	uint64_t size;
	char description[20];
	char file_name[20];
}ramdump_entry;

typedef struct
{
	uint32_t version;
	uint32_t header_size;
	ramdump_entry ramdump_table[MAX_RAMDUMP_TABLE_SIZE];
}ramdump_header_t;

void get_crash_reason(struct mhi_controller *mhi_cntrl)
{
	int i;
	uint64_t coredump_offset = 0;
	struct image_info *rddm_image;
	ramdump_header_t *ramdump_header;
	ramdump_entry *ramdump_table;
	struct mhi_buf *mhi_buf;
	char *msg = ERR_PTR(-EPROBE_DEFER);
	struct device *dev;
	struct pci_dev *pdev;

	rddm_image = mhi_cntrl->rddm_image;
	mhi_buf = rddm_image->mhi_buf;

	dev = &mhi_cntrl->mhi_dev->dev;
	pdev = to_pci_dev(mhi_cntrl->cntrl_dev);
	dev_err(dev, "CRASHED - [DID:DOMAIN:BUS:SLOT] - %x:%04u:%02u:%02u\n",
		pdev->device, pdev->bus->domain_nr, pdev->bus->number,
		PCI_SLOT(pdev->devfn));

	/* Get RDDM header size */
	ramdump_header = (ramdump_header_t *)mhi_buf[0].buf;
	ramdump_table = ramdump_header->ramdump_table;
	coredump_offset += ramdump_header->header_size;

	/* Traverse ramdump table to get coredump offset */
	i = 0;
	while(i < MAX_RAMDUMP_TABLE_SIZE) {
		if (!strncmp(ramdump_table->description, COREDUMP_DESC,
			     sizeof(COREDUMP_DESC)) ||
			!strncmp(ramdump_table->description, Q6_SFR_DESC,
			     sizeof(Q6_SFR_DESC))) {
			break;
		}
		coredump_offset += ramdump_table->size;
		ramdump_table++;
		i++;
	}

	if( i == MAX_RAMDUMP_TABLE_SIZE) {
		dev_err(dev, "Cannot find '%s' entry in ramdump\n",
			COREDUMP_DESC);
		return;
	}

	/* Locate coredump data from the ramdump segments */
	for (i = 0; i < rddm_image->entries; i++)
	{
		if (coredump_offset < mhi_buf[i].len) {
			msg = mhi_buf[i].buf + coredump_offset;
			break;
		} else {
			coredump_offset -= mhi_buf[i].len;
		}
	}

	if (!IS_ERR(msg) && msg && msg[0])
		dev_err(dev, "Fatal error received from wcss software!\n%s\n",
			msg);
}

/* Setup RDDM vector table for RDDM transfer and program RXVEC */
void mhi_rddm_prepare(struct mhi_controller *mhi_cntrl,
		      struct image_info *img_info)
{
	struct mhi_buf *mhi_buf = img_info->mhi_buf;
	struct bhi_vec_entry *bhi_vec = img_info->bhi_vec;
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	u32 sequence_id;
	unsigned int i;

	for (i = 0; i < img_info->entries - 1; i++, mhi_buf++, bhi_vec++) {
		bhi_vec->dma_addr = mhi_buf->dma_addr;
		bhi_vec->size = mhi_buf->len;
	}

	dev_dbg(dev, "BHIe programming for RDDM\n");

	mhi_write_reg(mhi_cntrl, base, BHIE_RXVECADDR_HIGH_OFFS,
		      upper_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_RXVECADDR_LOW_OFFS,
		      lower_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_RXVECSIZE_OFFS, mhi_buf->len);
	sequence_id = MHI_RANDOM_U32_NONZERO(BHIE_RXVECSTATUS_SEQNUM_BMSK);

	mhi_write_reg_field(mhi_cntrl, base, BHIE_RXVECDB_OFFS,
			    BHIE_RXVECDB_SEQNUM_BMSK, BHIE_RXVECDB_SEQNUM_SHFT,
			    sequence_id);

	dev_dbg(dev, "Address: %p and len: 0x%zx sequence: %u\n",
		&mhi_buf->dma_addr, mhi_buf->len, sequence_id);
}

/* Collect RDDM buffer during kernel panic */
static int __mhi_download_rddm_in_panic(struct mhi_controller *mhi_cntrl)
{
	int ret;
	u32 rx_status;
	enum mhi_ee_type ee;
	const u32 delayus = 2000;
	u32 retry = (mhi_cntrl->timeout_ms * 1000) / delayus;
	const u32 rddm_timeout_us = 200000;
	int rddm_retry = rddm_timeout_us / delayus;
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	u32 val, i;
	struct {
		char *name;
		u32 offset;
	} error_reg[] = {
		{ "ERROR_CODE", BHI_ERRCODE },
		{ "ERROR_DBG1", BHI_ERRDBG1 },
		{ "ERROR_DBG2", BHI_ERRDBG2 },
		{ "ERROR_DBG3", BHI_ERRDBG3 },
		{ NULL },
	};

	dev_dbg(dev, "Entered with pm_state:%s dev_state:%s ee:%s\n",
		to_mhi_pm_state_str(mhi_cntrl->pm_state),
		TO_MHI_STATE_STR(mhi_cntrl->dev_state),
		TO_MHI_EXEC_STR(mhi_cntrl->ee));

	/*
	 * This should only be executing during a kernel panic, we expect all
	 * other cores to shutdown while we're collecting RDDM buffer. After
	 * returning from this function, we expect the device to reset.
	 *
	 * Normaly, we read/write pm_state only after grabbing the
	 * pm_lock, since we're in a panic, skipping it. Also there is no
	 * gurantee that this state change would take effect since
	 * we're setting it w/o grabbing pm_lock
	 */
	mhi_cntrl->pm_state = MHI_PM_LD_ERR_FATAL_DETECT;
	/* update should take the effect immediately */
	smp_wmb();

	/*
	 * Make sure device is not already in RDDM. In case the device asserts
	 * and a kernel panic follows, device will already be in RDDM.
	 * Do not trigger SYS ERR again and proceed with waiting for
	 * image download completion.
	 */
	ee = mhi_get_exec_env(mhi_cntrl);
	if (ee == MHI_EE_MAX)
		goto error_exit_rddm;

	if (ee != MHI_EE_RDDM) {
		dev_dbg(dev, "Trigger device into RDDM mode using SYS ERR\n");
		mhi_set_mhi_state(mhi_cntrl, MHI_STATE_SYS_ERR);

		dev_dbg(dev, "Waiting for device to enter RDDM\n");
		while (rddm_retry--) {
			ee = mhi_get_exec_env(mhi_cntrl);
			if (ee == MHI_EE_RDDM)
				break;

			udelay(delayus);
		}

		if (rddm_retry <= 0) {
			/* Hardware reset so force device to enter RDDM */
			dev_dbg(dev,
				"Did not enter RDDM, do a host req reset\n");
			mhi_write_reg(mhi_cntrl, mhi_cntrl->regs,
				      MHI_SOC_RESET_REQ_OFFSET,
				      MHI_SOC_RESET_REQ);
			udelay(delayus);
		}

		ee = mhi_get_exec_env(mhi_cntrl);
	}

	dev_dbg(dev,
		"Waiting for RDDM image download via BHIe, current EE:%s\n",
		TO_MHI_EXEC_STR(ee));

	while (retry--) {
		ret = mhi_read_reg_field(mhi_cntrl, base, BHIE_RXVECSTATUS_OFFS,
					 BHIE_RXVECSTATUS_STATUS_BMSK,
					 BHIE_RXVECSTATUS_STATUS_SHFT,
					 &rx_status);
		if (ret)
			return -EIO;

		if (rx_status == BHIE_RXVECSTATUS_STATUS_XFER_COMPL) {
			get_crash_reason(mhi_cntrl);
			return 0;
		}

		udelay(delayus);
	}

	ee = mhi_get_exec_env(mhi_cntrl);
	ret = mhi_read_reg(mhi_cntrl, base, BHIE_RXVECSTATUS_OFFS, &rx_status);

	dev_err(dev, "RXVEC_STATUS: 0x%x\n", rx_status);
	for (i = 0; error_reg[i].name; i++) {
		ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi,
				   error_reg[i].offset, &val);
		if (ret)
			break;
		dev_err(dev, "reg:%s value:0x%x\n",
			error_reg[i].name, val);
	}

error_exit_rddm:
	dev_err(dev, "RDDM transfer failed. Current EE: %s\n",
		TO_MHI_EXEC_STR(ee));

	return -EIO;
}

/* Download RDDM image from device */
int mhi_download_rddm_image(struct mhi_controller *mhi_cntrl, bool in_panic)
{
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	u32 rx_status;
	rwlock_t *pm_lock = &mhi_cntrl->pm_lock;
	u32 val, ret, i;
	struct {
		char *name;
		u32 offset;
	} error_reg[] = {
		{ "ERROR_CODE", BHI_ERRCODE },
		{ "ERROR_DBG1", BHI_ERRDBG1 },
		{ "ERROR_DBG2", BHI_ERRDBG2 },
		{ "ERROR_DBG3", BHI_ERRDBG3 },
		{ NULL },
	};

	if (in_panic)
		return __mhi_download_rddm_in_panic(mhi_cntrl);

	dev_dbg(dev, "Waiting for RDDM image download via BHIe\n");

	/* Wait for the image download to complete */
	wait_event_timeout(mhi_cntrl->state_event,
			   mhi_read_reg_field(mhi_cntrl, base,
					      BHIE_RXVECSTATUS_OFFS,
					      BHIE_RXVECSTATUS_STATUS_BMSK,
					      BHIE_RXVECSTATUS_STATUS_SHFT,
					      &rx_status) || rx_status,
			   msecs_to_jiffies(mhi_cntrl->timeout_ms));

	if (rx_status == BHIE_RXVECSTATUS_STATUS_XFER_COMPL) {
		get_crash_reason(mhi_cntrl);
		return 0;
	}
	dev_err(dev, "Image download completion timed out, rx_status = %d\n",
		rx_status);
	read_lock_bh(pm_lock);
	if (MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
		for (i = 0; error_reg[i].name; i++) {
			ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi,
					   error_reg[i].offset, &val);
			if (ret)
				break;
			dev_err(dev, "reg:%s value:0x%x\n",
				error_reg[i].name, val);
		}
	}
	read_unlock_bh(pm_lock);

	return -EIO;
}
EXPORT_SYMBOL_GPL(mhi_download_rddm_image);

static int mhi_fw_load_bhie(struct mhi_controller *mhi_cntrl,
			    const struct mhi_buf *mhi_buf)
{
	void __iomem *base = mhi_cntrl->bhie;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	rwlock_t *pm_lock = &mhi_cntrl->pm_lock;
	u32 tx_status, sequence_id;
	int ret;
	u32 val, i;
	struct {
		char *name;
		u32 offset;
	} error_reg[] = {
		{ "ERROR_CODE", BHI_ERRCODE },
		{ "ERROR_DBG1", BHI_ERRDBG1 },
		{ "ERROR_DBG2", BHI_ERRDBG2 },
		{ "ERROR_DBG3", BHI_ERRDBG3 },
		{ NULL },
	};


	read_lock_bh(pm_lock);
	if (!MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
		read_unlock_bh(pm_lock);
		return -EIO;
	}

	sequence_id = MHI_RANDOM_U32_NONZERO(BHIE_TXVECSTATUS_SEQNUM_BMSK);
	dev_dbg(dev, "Starting image download via BHIe. Sequence ID: %u\n",
		sequence_id);
	mhi_write_reg(mhi_cntrl, base, BHIE_TXVECADDR_HIGH_OFFS,
		      upper_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_TXVECADDR_LOW_OFFS,
		      lower_32_bits(mhi_buf->dma_addr));

	mhi_write_reg(mhi_cntrl, base, BHIE_TXVECSIZE_OFFS, mhi_buf->len);

	mhi_write_reg_field(mhi_cntrl, base, BHIE_TXVECDB_OFFS,
			    BHIE_TXVECDB_SEQNUM_BMSK, BHIE_TXVECDB_SEQNUM_SHFT,
			    sequence_id);
	read_unlock_bh(pm_lock);

	/* Wait for the image download to complete */
	ret = wait_event_timeout(mhi_cntrl->state_event,
				 MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state) ||
				 mhi_read_reg_field(mhi_cntrl, base,
						   BHIE_TXVECSTATUS_OFFS,
						   BHIE_TXVECSTATUS_STATUS_BMSK,
						   BHIE_TXVECSTATUS_STATUS_SHFT,
						   &tx_status) || tx_status,
				 msecs_to_jiffies(mhi_cntrl->timeout_ms));
	if (MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state) ||
	    tx_status != BHIE_TXVECSTATUS_STATUS_XFER_COMPL) {
		pr_err("Upper:0x%x Lower:0x%x len:0x%zx sequence:%u\n",
			upper_32_bits(mhi_buf->dma_addr),
			lower_32_bits(mhi_buf->dma_addr),
			mhi_buf->len, sequence_id);

		pr_err("MHI pm_state: %s tx_status: %d ee: %s\n",
			to_mhi_pm_state_str(mhi_cntrl->pm_state), tx_status,
			TO_MHI_EXEC_STR(mhi_get_exec_env(mhi_cntrl)));

		read_lock_bh(pm_lock);
		if (MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
			for (i = 0; error_reg[i].name; i++) {
				ret = mhi_read_reg(mhi_cntrl, base,
						   error_reg[i].offset, &val);
				if (ret)
					break;
				dev_err(dev, "Reg: %s value: 0x%x\n",
					error_reg[i].name, val);
			}
		}
		read_unlock_bh(pm_lock);
		return -EIO;
	}

	return (!ret) ? -ETIMEDOUT : 0;
}

static int mhi_fw_load_bhi(struct mhi_controller *mhi_cntrl,
			   dma_addr_t dma_addr,
			   size_t size)
{
	u32 tx_status, val, session_id;
	int i, ret;
	void __iomem *base = mhi_cntrl->bhi;
	rwlock_t *pm_lock = &mhi_cntrl->pm_lock;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	struct {
		char *name;
		u32 offset;
	} error_reg[] = {
		{ "ERROR_CODE", BHI_ERRCODE },
		{ "ERROR_DBG1", BHI_ERRDBG1 },
		{ "ERROR_DBG2", BHI_ERRDBG2 },
		{ "ERROR_DBG3", BHI_ERRDBG3 },
		{ NULL },
	};

	read_lock_bh(pm_lock);
	if (!MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
		read_unlock_bh(pm_lock);
		goto invalid_pm_state;
	}

	session_id = MHI_RANDOM_U32_NONZERO(BHI_TXDB_SEQNUM_BMSK);
	dev_dbg(dev, "Starting image download via BHI. Session ID: %u\n",
		session_id);
	mhi_write_reg(mhi_cntrl, base, BHI_STATUS, 0);
	mhi_write_reg(mhi_cntrl, base, BHI_IMGADDR_HIGH,
		      upper_32_bits(dma_addr));
	mhi_write_reg(mhi_cntrl, base, BHI_IMGADDR_LOW,
		      lower_32_bits(dma_addr));
	mhi_write_reg(mhi_cntrl, base, BHI_IMGSIZE, size);
	mhi_write_reg(mhi_cntrl, base, BHI_IMGTXDB, session_id);
	read_unlock_bh(pm_lock);

	/* Wait for the image download to complete */
	ret = wait_event_timeout(mhi_cntrl->state_event,
			   MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state) ||
			   mhi_read_reg_field(mhi_cntrl, base, BHI_STATUS,
					      BHI_STATUS_MASK, BHI_STATUS_SHIFT,
					      &tx_status) || tx_status,
			   msecs_to_jiffies(mhi_cntrl->timeout_ms));
	if (MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state))
		goto invalid_pm_state;

	if (tx_status == BHI_STATUS_ERROR) {
		dev_err(dev, "Image transfer failed\n");
		read_lock_bh(pm_lock);
		if (MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
			for (i = 0; error_reg[i].name; i++) {
				ret = mhi_read_reg(mhi_cntrl, base,
						   error_reg[i].offset, &val);
				if (ret)
					break;
				dev_err(dev, "Reg: %s value: 0x%x\n",
					error_reg[i].name, val);
			}
		}
		read_unlock_bh(pm_lock);
		goto invalid_pm_state;
	}

	return (!ret) ? -ETIMEDOUT : 0;

invalid_pm_state:

	return -EIO;
}

void mhi_free_bhie_table(struct mhi_controller *mhi_cntrl,
			 struct image_info *image_info, bool is_fbc)
{
	int i;
	struct mhi_buf *mhi_buf = image_info->mhi_buf;

	for (i = 0; i < image_info->entries; i++, mhi_buf++) {
		/* For FBC image, element mhi_buf[img_info->entries - 2] points
		 * to Dynamic paging region and it should not be freed.
		 */
		if (is_fbc && i == (image_info->entries - 2))
			continue;

		mhi_fw_free_coherent(mhi_cntrl, mhi_buf->len, mhi_buf->buf,
				  mhi_buf->dma_addr);
	}

	kfree(image_info->mhi_buf);
	kfree(image_info);
}

int mhi_update_bhie_table_for_dyn_paging(struct mhi_controller *mhi_cntrl,
					 void *va, phys_addr_t pa,
					 size_t size)
{
	struct image_info *image_info = mhi_cntrl->fbc_image;
	int i, segments;
	struct mhi_buf *mhi_buf;
	struct bhi_vec_entry *bhi_vec;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;

	if (!image_info) {
		dev_err(dev, "FBC Image is NULL\n");
		return -EINVAL;
	}

	segments = image_info->entries;

	/* Find the free entry in bhi_vec table for dynamic paging region */
	bhi_vec = &image_info->bhi_vec[0];
	for (i = 0; (i < segments - 1); i++) {
		if (!bhi_vec->dma_addr)
			break;

		bhi_vec++;
	}
	if (i == (segments - 1)) {
		dev_err(dev, "No space in Vector Table\n");
		return -ENOMEM;
	}

	bhi_vec->dma_addr = pa;
	bhi_vec->size = size;

	/* mhi_buf[segments - 2] is reserved Dynamic Paging region */
	mhi_buf = &image_info->mhi_buf[segments - 2];
	mhi_buf->buf = va;
	mhi_buf->dma_addr = pa;
	mhi_buf->len = size;

	return 0;
}
EXPORT_SYMBOL(mhi_update_bhie_table_for_dyn_paging);

int mhi_alloc_bhie_table(struct mhi_controller *mhi_cntrl,
			 struct image_info **image_info,
			 size_t alloc_size, bool is_fbc)
{
	size_t seg_size = mhi_cntrl->seg_len;
	int segments = DIV_ROUND_UP(alloc_size, seg_size) + 1;
	int i;
	struct image_info *img_info;
	struct mhi_buf *mhi_buf;

	/* Allocate one extra entry for Dynamic Pageable in FBC */
	if (is_fbc)
		segments++;

	img_info = kzalloc(sizeof(*img_info), GFP_KERNEL);
	if (!img_info)
		return -ENOMEM;

	/* Allocate memory for entries */
	img_info->mhi_buf = kcalloc(segments, sizeof(*img_info->mhi_buf),
				    GFP_KERNEL);
	if (!img_info->mhi_buf)
		goto error_alloc_mhi_buf;

	/* Allocate and populate vector table */
	mhi_buf = img_info->mhi_buf;
	for (i = 0; i < segments; i++, mhi_buf++) {
		size_t vec_size = seg_size;

		if (is_fbc && (i == segments - 2)) {
			/* Initialize an entry for Dynamic paging region which
			 * would be updated later in
			 * mhi_update_bhie_table_for_dyn_paging
			 */
			vec_size = 0;
			mhi_buf->buf = NULL;
			mhi_buf->dma_addr = 0;
		} else if (i == segments - 1) {
			/* last entry is for vector table */

			vec_size = sizeof(struct bhi_vec_entry) * i;
			mhi_buf->buf = mhi_alloc_coherent(mhi_cntrl, vec_size,
							  &mhi_buf->dma_addr,
							  GFP_KERNEL);
			if (!mhi_buf->buf)
				goto error_alloc_segment;
		} else {
			mhi_buf->buf = mhi_fw_alloc_coherent(mhi_cntrl, vec_size,
							     &mhi_buf->dma_addr,
							     GFP_KERNEL);
			if (!mhi_buf->buf)
				goto error_alloc_segment;
		}

		mhi_buf->len = vec_size;
	}

	img_info->bhi_vec = img_info->mhi_buf[segments - 1].buf;
	img_info->entries = segments;
	*image_info = img_info;

	return 0;

error_alloc_segment:
	for (--i, --mhi_buf; i >= 0; i--, mhi_buf--)
		mhi_fw_free_coherent(mhi_cntrl, mhi_buf->len, mhi_buf->buf,
				  mhi_buf->dma_addr);

error_alloc_mhi_buf:
	kfree(img_info);

	return -ENOMEM;
}

static void mhi_firmware_copy(struct mhi_controller *mhi_cntrl,
			      const struct firmware *firmware,
			      struct image_info *img_info)
{
	size_t remainder = firmware->size;
	size_t to_cpy;
	const u8 *buf = firmware->data;
	struct mhi_buf *mhi_buf = img_info->mhi_buf;
	struct bhi_vec_entry *bhi_vec = img_info->bhi_vec;

	while (remainder) {
		to_cpy = min(remainder, mhi_buf->len);
		memcpy(mhi_buf->buf, buf, to_cpy);
		bhi_vec->dma_addr = mhi_buf->dma_addr;
		bhi_vec->size = cpu_to_le64(mhi_buf->len);

		buf += to_cpy;
		remainder -= to_cpy;
		bhi_vec++;
		mhi_buf++;
	}
}

void mhi_fw_load_handler(struct mhi_controller *mhi_cntrl)
{
	const struct firmware *firmware = NULL;
	struct image_info *image_info;
	struct device *dev = &mhi_cntrl->mhi_dev->dev;
	const char *fw_name;
	void *buf;
	dma_addr_t dma_addr;
	size_t size;
	int i, ret;
	u32 instance;
	rwlock_t *pm_lock = &mhi_cntrl->pm_lock;
	u32 val;
	struct {
		char *name;
		u32 offset;
	} error_reg[] = {
		{ "ERROR_CODE", BHI_ERRCODE },
		{ "ERROR_DBG1", BHI_ERRDBG1 },
		{ "ERROR_DBG2", BHI_ERRDBG2 },
		{ "ERROR_DBG3", BHI_ERRDBG3 },
		{ NULL },
	};

	if (MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state)) {
		dev_err(dev, "Device MHI is not in valid state\n");
		return;
	}

	/* save hardware info from BHI */
	ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi, BHI_SERIALNU,
			   &mhi_cntrl->serial_number);
	if (ret)
		dev_err(dev, "Could not capture serial number via BHI\n");

	for (i = 0; i < ARRAY_SIZE(mhi_cntrl->oem_pk_hash); i++) {
		ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi, BHI_OEMPKHASH(i),
				   &mhi_cntrl->oem_pk_hash[i]);
		if (ret) {
			dev_err(dev, "Could not capture OEM PK HASH via BHI\n");
			break;
		}
	}

	/* If device is in pass through, do reset to ready state transition */
	if (mhi_cntrl->ee == MHI_EE_PTHRU)
		goto fw_load_ee_pthru;

	fw_name = (mhi_cntrl->ee == MHI_EE_EDL) ?
		mhi_cntrl->edl_image : mhi_cntrl->fw_image;

	if (!fw_name || (mhi_cntrl->fbc_download && (!mhi_cntrl->sbl_size ||
						     !mhi_cntrl->seg_len))) {
		dev_err(dev,
			"No firmware image defined or !sbl_size || !seg_len\n");
		goto error_fw_load;
	}

	ret = request_firmware(&firmware, fw_name, dev);
	if (ret) {
		dev_err(dev, "Error loading firmware: %d\n", ret);
		goto error_fw_load;
	}

	size = (mhi_cntrl->fbc_download) ? mhi_cntrl->sbl_size : firmware->size;

	/* SBL size provided is maximum size, not necessarily the image size */
	if (size > firmware->size)
		size = firmware->size;

	buf = mhi_fw_alloc_coherent(mhi_cntrl, size, &dma_addr, GFP_KERNEL);
	if (!buf) {
		release_firmware(firmware);
		goto error_fw_load;
	}

	/* Download image using BHI */
	memcpy(buf, firmware->data, size);
	ret = mhi_fw_load_bhi(mhi_cntrl, dma_addr, size);
	mhi_fw_free_coherent(mhi_cntrl, size, buf, dma_addr);

	/* Error or in EDL mode, we're done */
	if (ret) {
		dev_err(dev, "MHI did not load image over BHI, ret: %d\n", ret);
		release_firmware(firmware);
		goto error_fw_load;
	}

	if (mhi_cntrl->ee == MHI_EE_EDL) {
		release_firmware(firmware);
		return;
	}

	if (!ret && mhi_cntrl->cntrl_dev->of_node) {
		ret = of_property_read_u32(mhi_cntrl->cntrl_dev->of_node,
					   "qrtr_instance_id", &instance);
		if (!ret) {
			instance &= QRTR_INSTANCE_MASK;
			mhi_write_reg_field(mhi_cntrl, mhi_cntrl->bhi,
					    BHI_ERRDBG2, QRTR_INSTANCE_MASK,
					    QRTR_INSTANCE_SHIFT, instance);
		} else {
			dev_err(dev,
				"qrtr_instance_id not defined in DT, ret:%d\n",
				ret);
		}
	}

	write_lock_irq(&mhi_cntrl->pm_lock);
	mhi_cntrl->dev_state = MHI_STATE_RESET;
	write_unlock_irq(&mhi_cntrl->pm_lock);

	/*
	 * If we're doing fbc, populate vector tables while
	 * device transitioning into MHI READY state
	 */
	if (mhi_cntrl->fbc_download) {
		ret = mhi_alloc_bhie_table(mhi_cntrl, &mhi_cntrl->fbc_image,
					   firmware->size, true);
		if (ret) {
			release_firmware(firmware);
			goto error_fw_load;
		}

		/* Load the firmware into BHIE vec table */
		mhi_firmware_copy(mhi_cntrl, firmware, mhi_cntrl->fbc_image);
	}

	release_firmware(firmware);

fw_load_ee_pthru:
	/* Transitioning into MHI RESET->READY state */
	ret = mhi_ready_state_transition(mhi_cntrl);

	if (!mhi_cntrl->fbc_download)
		return;

	if (ret) {
		dev_err(dev, "MHI did not enter READY state\n");
		goto error_ready_state;
	}

	/* Wait for the SBL event */
	ret = wait_event_timeout(mhi_cntrl->state_event,
				 mhi_cntrl->ee == MHI_EE_SBL ||
				 MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state),
				 msecs_to_jiffies(mhi_cntrl->timeout_ms));

	if (!ret || MHI_PM_IN_ERROR_STATE(mhi_cntrl->pm_state)) {
		dev_err(dev, "MHI did not enter SBL\n");
		goto error_ready_state;
	}

	/* Start full firmware image download */
	image_info = mhi_cntrl->fbc_image;
	ret = mhi_fw_load_bhie(mhi_cntrl,
			       /* Vector table is the last entry */
			       &image_info->mhi_buf[image_info->entries - 1]);
	if (ret) {
		dev_err(dev, "MHI did not load image over BHIe, ret: %d\n",
			ret);
		goto error_fw_load;
	}

	return;

error_ready_state:
	read_lock_bh(pm_lock);
	if (MHI_REG_ACCESS_VALID(mhi_cntrl->pm_state)) {
		for (i = 0; error_reg[i].name; i++) {
			ret = mhi_read_reg(mhi_cntrl, mhi_cntrl->bhi,
					   error_reg[i].offset, &val);
			if (ret)
				break;
			dev_err(dev, "reg:%s value:0x%x\n",
				error_reg[i].name, val);
		}
	}
	read_unlock_bh(pm_lock);
	mhi_free_bhie_table(mhi_cntrl, mhi_cntrl->fbc_image, true);
	mhi_cntrl->fbc_image = NULL;

error_fw_load:
	mhi_cntrl->pm_state = MHI_PM_FW_DL_ERR;
	wake_up_all(&mhi_cntrl->state_event);
}
