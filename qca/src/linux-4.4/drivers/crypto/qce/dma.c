/*
 * Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/dmaengine.h>
#include <crypto/scatterwalk.h>

#include "dma.h"
#include "core.h"

#define QCE_REG_BUF_DMA_ADDR(qce, vaddr) \
	((qce)->reg_read_buf_phys + \
	 ((uint8_t *)(vaddr) - (uint8_t *)(qce)->reg_read_buf))

void qce_clear_bam_transaction(struct qce_device *qce)
{
	struct qce_bam_transaction *qce_bam_txn = qce->dma.qce_bam_txn;
	qce_bam_txn->qce_bam_ce_index = 0;
	qce_bam_txn->qce_write_sgl_cnt = 0;
	qce_bam_txn->qce_read_sgl_cnt = 0;
	qce_bam_txn->qce_bam_ce_index = 0;
	qce_bam_txn->qce_pre_bam_ce_index = 0;
}

static int qce_dma_prep_cmd_sg(struct qce_device *qce, struct dma_chan *chan,
				struct qcom_bam_sgl *qce_bam_sgl,
			   int qce_sgl_cnt, unsigned long flags,
			   enum dma_transfer_direction dir,
			   dma_async_tx_callback cb, void *cb_param)
{
	struct dma_async_tx_descriptor *dma_desc;
	dma_cookie_t cookie;
	struct qce_desc_info *desc;

	desc = qce->dma.qce_bam_txn->qce_desc;

	if (!qce_bam_sgl || !qce_sgl_cnt)
		return -EINVAL;

	if (!qcom_bam_map_sg(qce->dev, qce_bam_sgl,
				qce_sgl_cnt, dir)) {
		dev_err(qce->dev, "failure in mapping sgl for cmd desc\n");
		return -ENOMEM;
	}

	desc->bam_desc_data.dir = dir;
	desc->bam_desc_data.sgl_cnt = qce_sgl_cnt;
	desc->bam_desc_data.bam_sgl = qce_bam_sgl;

	dma_desc = dmaengine_prep_dma_custom_mapping(chan, &desc->bam_desc_data, flags);
	if (!dma_desc) {
		pr_err("%s:failure in prep cmd desc\n",__func__);
		qcom_bam_unmap_sg(qce->dev, qce_bam_sgl, qce_sgl_cnt, dir);
		kfree(desc);
		return -EINVAL;
	}

	desc->dma_desc = dma_desc;

	desc->dma_desc->callback = cb;
	desc->dma_desc->callback_param = cb_param;

	cookie = dmaengine_submit(desc->dma_desc);

	return dma_submit_error(cookie);
}

int qce_submit_cmd_desc(struct qce_device *qce)
{
	struct qce_bam_transaction *qce_bam_txn = qce->dma.qce_bam_txn;
	struct dma_chan *rxchan = qce->dma.rxchan;
	int ret = 0;

	/* For command descriptor always use consumer pipe
	 * it recomended as per HPG
	 */
	if (qce_bam_txn->qce_read_sgl_cnt) {
		ret = qce_dma_prep_cmd_sg(qce, rxchan, qce_bam_txn->qce_reg_read_sgl,
				qce_bam_txn->qce_read_sgl_cnt, 0, DMA_DEV_TO_MEM, NULL, NULL);
	}

	if (qce_bam_txn->qce_write_sgl_cnt) {
		ret = qce_dma_prep_cmd_sg(qce, rxchan, qce_bam_txn->qce_reg_write_sgl,
				qce_bam_txn->qce_write_sgl_cnt, 0, DMA_MEM_TO_DEV, NULL, NULL);
	}

	if (ret) {
		pr_err("%s: Error while submiting cmd desc\n",__func__);
		return ret;
	}

	qce_dma_issue_pending(&qce->dma);

	return ret;
}

static void qce_prep_dma_command_desc(struct qce_device *qce, struct qce_dma_data *dma,
		bool read, unsigned int addr, void *buff, int size, int flags)
{
	int qce_bam_ce_size;
	struct bam_cmd_element *qce_bam_ce_buffer;
	struct qce_bam_transaction *qce_bam_txn = dma->qce_bam_txn;

	qce_bam_ce_buffer = &qce_bam_txn->qce_bam_ce[qce_bam_txn->qce_bam_ce_index];
	if (read) {
		qcom_prep_bam_ce(qce_bam_ce_buffer, addr, BAM_READ_COMMAND,
				QCE_REG_BUF_DMA_ADDR(qce, (unsigned int *)buff));

	} else {
		qcom_prep_bam_ce(qce_bam_ce_buffer, addr, BAM_WRITE_COMMAND,
				*(unsigned int *)buff);
	}

	if (read) {
		qce_bam_ce_buffer = &qce_bam_txn->qce_bam_ce
			[qce_bam_txn->qce_pre_bam_ce_index];
		qce_bam_txn->qce_bam_ce_index += size;
		qce_bam_ce_size = (qce_bam_txn->qce_bam_ce_index -
				qce_bam_txn->qce_pre_bam_ce_index) *
			sizeof(struct bam_cmd_element);

		sg_set_buf(&qce_bam_txn->qce_reg_read_sgl[qce_bam_txn->qce_read_sgl_cnt].sgl,
				qce_bam_ce_buffer,
				qce_bam_ce_size);
		if (flags & QCE_DMA_DESC_FLAG_LOCK) {
			qce_bam_txn->qce_reg_read_sgl[qce_bam_txn->qce_read_sgl_cnt].dma_flags =
				DESC_FLAG_CMD | DESC_FLAG_LOCK;
		} else if (flags & QCE_DMA_DESC_FLAG_UNLOCK) {
			qce_bam_txn->qce_reg_read_sgl[qce_bam_txn->qce_read_sgl_cnt].dma_flags =
				DESC_FLAG_CMD | DESC_FLAG_UNLOCK;
		} else {
			qce_bam_txn->qce_reg_read_sgl[qce_bam_txn->qce_read_sgl_cnt].dma_flags =
				DESC_FLAG_CMD;
		}

		qce_bam_txn->qce_read_sgl_cnt++;
		qce_bam_txn->qce_pre_bam_ce_index = qce_bam_txn->qce_bam_ce_index;
	} else {
		qce_bam_ce_buffer = &qce_bam_txn->qce_bam_ce
			[qce_bam_txn->qce_pre_bam_ce_index];
		qce_bam_txn->qce_bam_ce_index += size;
		qce_bam_ce_size = (qce_bam_txn->qce_bam_ce_index -
				qce_bam_txn->qce_pre_bam_ce_index) *
			sizeof(struct bam_cmd_element);

		sg_set_buf(&qce_bam_txn->qce_reg_write_sgl[qce_bam_txn->qce_write_sgl_cnt].sgl,
				qce_bam_ce_buffer,
				qce_bam_ce_size);
		if (flags & QCE_DMA_DESC_FLAG_LOCK) {
			qce_bam_txn->qce_reg_write_sgl[qce_bam_txn->qce_write_sgl_cnt].dma_flags =
				DESC_FLAG_CMD | DESC_FLAG_LOCK;
		} else if (flags & QCE_DMA_DESC_FLAG_UNLOCK) {
			qce_bam_txn->qce_reg_write_sgl[qce_bam_txn->qce_write_sgl_cnt].dma_flags =
				DESC_FLAG_CMD | DESC_FLAG_UNLOCK;
		} else {
			qce_bam_txn->qce_reg_write_sgl[qce_bam_txn->qce_write_sgl_cnt].dma_flags =
				DESC_FLAG_CMD;
		}

		qce_bam_txn->qce_write_sgl_cnt++;
		qce_bam_txn->qce_pre_bam_ce_index = qce_bam_txn->qce_bam_ce_index;
	}
}

int qce_write_reg_dma(struct qce_device *qce,
		unsigned int offset, u32 val, int cnt, int flag)
{
	void *buff = &val;
	unsigned int reg_addr = ((unsigned int)(qce->base_dma) + offset);

	qce_prep_dma_command_desc(qce, &qce->dma, false, reg_addr, buff, cnt, flag);

	return 0;
}

int qce_read_reg_dma(struct qce_device *qce,
		unsigned int offset, void *buff, int cnt, int flag)
{
	unsigned int reg_addr = ((unsigned int)(qce->base_dma) + offset);
	void *vaddr = qce->reg_read_buf;

	qce_prep_dma_command_desc(qce, &qce->dma, true, reg_addr, vaddr, cnt, flag);

	memcpy(buff, vaddr, 4);

	return 0;
}

struct qce_bam_transaction *qce_alloc_bam_txn(struct qce_dma_data *dma)
{
	struct qce_bam_transaction *qce_bam_txn;

	dma->qce_bam_txn = kmalloc(sizeof(*qce_bam_txn), GFP_KERNEL);
	if (!dma->qce_bam_txn)
		return NULL;

	dma->qce_bam_txn->qce_desc = kzalloc(sizeof(struct qce_desc_info), GFP_KERNEL);
	if (!dma->qce_bam_txn->qce_desc)
		return NULL;

	qcom_bam_sg_init_table(dma->qce_bam_txn->qce_reg_write_sgl, QCE_BAM_CMD_SGL_SIZE);
	qcom_bam_sg_init_table(dma->qce_bam_txn->qce_reg_read_sgl, QCE_BAM_CMD_SGL_SIZE);

	qce_bam_txn = dma->qce_bam_txn;

	return qce_bam_txn;
}

int qce_dma_request(struct device *dev, struct qce_dma_data *dma)
{
	int ret;
	struct qce_device *qce = container_of(dma, struct qce_device, dma);

	dma->txchan = dma_request_slave_channel_reason(dev, "tx");
	if (IS_ERR(dma->txchan))
		return PTR_ERR(dma->txchan);

	dma->rxchan = dma_request_slave_channel_reason(dev, "rx");
	if (IS_ERR(dma->rxchan)) {
		ret = PTR_ERR(dma->rxchan);
		goto error_rx;
	}

	dma->result_buf = kmalloc(QCE_RESULT_BUF_SZ + QCE_IGNORE_BUF_SZ,
				  GFP_KERNEL);
	if (!dma->result_buf) {
		ret = -ENOMEM;
		goto error_nomem;
	}

	dma->ignore_buf = dma->result_buf + QCE_RESULT_BUF_SZ;

	if (qce->qce_cmd_desc_enable) {
		dma->qce_bam_txn = qce_alloc_bam_txn(dma);
		if (!dma->qce_bam_txn) {
			pr_err("%s Failed to allocate bam transaction\n",__func__);
			return -ENOMEM;
		}

		qce->reg_read_buf = dmam_alloc_coherent(qce->dev, QCE_MAX_REG_READ *
					sizeof(*qce->reg_read_buf),
					&qce->reg_read_buf_phys, GFP_KERNEL);
		if (!qce->reg_read_buf) {
			pr_err("%s: Failed to allocate reg_read_buf\n",__func__);
			return -ENOMEM;
		}
	}

	return 0;
error_nomem:
	dma_release_channel(dma->rxchan);
error_rx:
	dma_release_channel(dma->txchan);
	return ret;
}

void qce_dma_release(struct qce_dma_data *dma)
{
	struct qce_device *qce = container_of(dma,
			struct qce_device, dma);

	dma_release_channel(dma->txchan);
	dma_release_channel(dma->rxchan);
	kfree(dma->result_buf);
	if (qce->qce_cmd_desc_enable) {
		if (qce->reg_read_buf)
			dmam_free_coherent(qce->dev, QCE_MAX_REG_READ *
				sizeof(*qce->reg_read_buf),
				qce->reg_read_buf,
				qce->reg_read_buf_phys);
		kfree(dma->qce_bam_txn->qce_desc);
		kfree(dma->qce_bam_txn);
	}
}

struct scatterlist *
qce_sgtable_add(struct sg_table *sgt, struct scatterlist *new_sgl)
{
	struct scatterlist *sg = sgt->sgl, *sg_last = NULL;

	while (sg) {
		if (!sg_page(sg))
			break;
		sg = sg_next(sg);
	}

	if (!sg)
		return ERR_PTR(-EINVAL);

	while (new_sgl && sg) {
		sg_set_page(sg, sg_page(new_sgl), new_sgl->length,
			    new_sgl->offset);
		sg_last = sg;
		sg = sg_next(sg);
		new_sgl = sg_next(new_sgl);
	}

	return sg_last;
}

static int qce_dma_prep_sg(struct dma_chan *chan, struct scatterlist *sg,
		int nents, unsigned long flags,
		enum dma_transfer_direction dir,
		dma_async_tx_callback cb, void *cb_param)
{
	struct dma_async_tx_descriptor *desc;
	dma_cookie_t cookie;
	if (!sg || !nents)
		return -EINVAL;
	desc = dmaengine_prep_slave_sg(chan, sg, nents, dir, flags);
	if (!desc)
		return -EINVAL;

	desc->callback = cb;
	desc->callback_param = cb_param;
	cookie = dmaengine_submit(desc);

	return dma_submit_error(cookie);
}

int qce_dma_prep_sgs(struct qce_dma_data *dma, struct scatterlist *rx_sg,
		     int rx_nents, struct scatterlist *tx_sg, int tx_nents,
		     dma_async_tx_callback cb, void *cb_param)
{
	struct dma_chan *rxchan = dma->rxchan;
	struct dma_chan *txchan = dma->txchan;
	unsigned long flags = DMA_PREP_INTERRUPT | DMA_CTRL_ACK;
	int ret;

	ret = qce_dma_prep_sg(rxchan, rx_sg, rx_nents, flags, DMA_MEM_TO_DEV,
			NULL, NULL);
	if (ret)
		return ret;

	return qce_dma_prep_sg(txchan, tx_sg, tx_nents, flags, DMA_DEV_TO_MEM,
			cb, cb_param);
}

void qce_dma_issue_pending(struct qce_dma_data *dma)
{
	dma_async_issue_pending(dma->rxchan);
	dma_async_issue_pending(dma->txchan);
}

int qce_dma_terminate_all(struct qce_dma_data *dma)
{
	int ret;

	ret = dmaengine_terminate_all(dma->rxchan);
	return ret ?: dmaengine_terminate_all(dma->txchan);
}
