/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/version.h>
#include <linux/interrupt.h>
#include <linux/phy.h>
#include <linux/netdevice.h>
#include <linux/debugfs.h>
#include <asm/cacheflush.h>

#include "nss_dp_dev.h"
#include "edma_regs.h"
#include "edma_debug.h"
#include "edma.h"

/*
 * edma_tx_complete()
 *	Reap Tx descriptors
 */
uint32_t edma_tx_complete(uint32_t work_to_do, struct edma_txcmpl_ring *txcmpl_ring)
{
	struct edma_txcmpl_desc *txcmpl;
	uint32_t prod_idx;
	uint32_t cons_idx;
	uint32_t data;
	struct sk_buff *skb;
	uint32_t len;
	uint32_t avail, count;
	uint32_t end_idx;

	cons_idx = txcmpl_ring->cons_idx;

	if (likely(txcmpl_ring->avail_pkt >= work_to_do)) {
		avail = work_to_do;
	} else {

		/*
		 * Get TXCMPL ring producer index
		 */
		data = edma_reg_read(EDMA_REG_TXCMPL_PROD_IDX(txcmpl_ring->id));
		prod_idx = data & EDMA_TXCMPL_PROD_IDX_MASK;

		avail = EDMA_DESC_AVAIL_COUNT(prod_idx, cons_idx, EDMA_TX_RING_SIZE);
		txcmpl_ring->avail_pkt = avail;

		if (unlikely(!avail)) {
			edma_debug("No available descriptors are pending for %d txcmpl ring\n",
					txcmpl_ring->id);
			return 0;
		}

		avail = min(avail, work_to_do);
	}

	count = avail;

	end_idx = (cons_idx + avail) & EDMA_TX_RING_SIZE_MASK;
	txcmpl = EDMA_TXCMPL_DESC(txcmpl_ring, cons_idx);

	if (end_idx > cons_idx) {
		dmac_inv_range((void *)txcmpl, txcmpl + avail);
	} else {
		dmac_inv_range(txcmpl_ring->desc, txcmpl_ring->desc + end_idx);
		dmac_inv_range((void *)txcmpl, txcmpl_ring->desc + EDMA_TX_RING_SIZE);
	}

	/*
	 * TODO:
	 * Instead of freeing the skb, it might be better to save and use
	 * for Rxfill.
	 */
	while (likely(avail--)) {
		skb = (struct sk_buff *)EDMA_TXCMPL_OPAQUE_GET(txcmpl);
		if (unlikely(!skb)) {
			edma_warn("Invalid skb: cons_idx:%u prod_idx:%u status %x\n",
				  cons_idx, prod_idx, txcmpl->word2);

			cons_idx = ((cons_idx + 1) & EDMA_TX_RING_SIZE_MASK);
			txcmpl = EDMA_TXCMPL_DESC(txcmpl_ring, cons_idx);
			continue;
		}

		len = skb_headlen(skb);
		edma_debug("skb:%px cons_idx:%d prod_idx:%d word1:0x%x\n",
			   skb, cons_idx, prod_idx, txcmpl->word2);
		dev_kfree_skb_any(skb);

		cons_idx = ((cons_idx + 1) & EDMA_TX_RING_SIZE_MASK);
		txcmpl = EDMA_TXCMPL_DESC(txcmpl_ring, cons_idx);
	}

	txcmpl_ring->cons_idx = cons_idx;
	txcmpl_ring->avail_pkt -= count;

	edma_debug("TXCMPL:%u count:%u prod_idx:%u cons_idx:%u\n",
			txcmpl_ring->id, count, prod_idx, cons_idx);
	edma_reg_write(EDMA_REG_TXCMPL_CONS_IDX(txcmpl_ring->id), cons_idx);

	return count;
}

/*
 * edma_tx_napi_poll()
 *	EDMA TX NAPI handler
 */
int edma_tx_napi_poll(struct napi_struct *napi, int budget)
{
	struct edma_txcmpl_ring *txcmpl_ring = (struct edma_txcmpl_ring *)napi;
	struct edma_gbl_ctx *egc = &edma_gbl_ctx;
	uint32_t txcmpl_intr_status;
	int work_done = 0;
	uint32_t reg_data;

	do {
		work_done += edma_tx_complete(budget - work_done, txcmpl_ring);
		if (work_done >= budget) {
			return work_done;
		}

		reg_data = edma_reg_read(EDMA_REG_TX_INT_STAT(txcmpl_ring->id));
		txcmpl_intr_status = reg_data & EDMA_TXCMPL_RING_INT_STATUS_MASK;
	} while (txcmpl_intr_status);

	/*
	 * No more packets to process. Finish NAPI processing.
	 */
	napi_complete(napi);

	/*
	 * Set TXCMPL ring interrupt mask
	 */
	edma_reg_write(EDMA_REG_TX_INT_MASK(txcmpl_ring->id),
			egc->txcmpl_intr_mask);

	return work_done;
}

/*
 * edma_tx_handle_irq()
 *	Process TX IRQ and schedule napi
 */
irqreturn_t edma_tx_handle_irq(int irq, void *ctx)
{
	struct edma_txcmpl_ring *txcmpl_ring = (struct edma_txcmpl_ring *)ctx;

	edma_debug("irq: irq=%d txcmpl_ring_id=%u\n", irq, txcmpl_ring->id);
	if (likely(napi_schedule_prep(&txcmpl_ring->napi))) {
		/*
		 * Disable TxCmpl intr
		 */
		edma_reg_write(EDMA_REG_TX_INT_MASK(txcmpl_ring->id),
				EDMA_MASK_INT_DISABLE);
		__napi_schedule(&txcmpl_ring->napi);
	}

	return IRQ_HANDLED;
}

/*
 * edma_tx_ring_xmit()
 *	API to transmit a packet.
 */
enum edma_tx edma_tx_ring_xmit(struct net_device *netdev, struct sk_buff *skb,
				struct edma_txdesc_ring *txdesc_ring,
				struct edma_tx_stats *stats)
{
	struct nss_dp_dev *dp_dev = netdev_priv(netdev);
	struct edma_pri_txdesc *txdesc;
	uint32_t hw_next_to_use, hw_next_to_clean, data, avail, buf_len;

	hw_next_to_use = txdesc_ring->prod_idx;

	if (unlikely(!(txdesc_ring->avail_desc)))  {
		data = edma_reg_read(EDMA_REG_TXDESC_CONS_IDX(txdesc_ring->id));
		hw_next_to_clean = data & EDMA_TXDESC_CONS_IDX_MASK;

		avail = EDMA_DESC_AVAIL_COUNT(hw_next_to_clean - 1, hw_next_to_use, EDMA_TX_RING_SIZE);
		if (unlikely(!avail)) {
			edma_debug("No available descriptors are present at %d ring\n",
					txdesc_ring->id);
			u64_stats_update_begin(&stats->syncp);
			++stats->tx_no_desc_avail;
			u64_stats_update_end(&stats->syncp);
			return EDMA_TX_FAIL_NO_DESC;
		}
		txdesc_ring->avail_desc = avail;
	}

	/*
	 * Get the packet length
	 */
	buf_len = skb_headlen(skb);

	txdesc = EDMA_TXDESC_PRI_DESC(txdesc_ring, hw_next_to_use);

	/*
	 * Set the skb pointer to the descriptor's opaque field/s
	 */
	EDMA_TXDESC_OPAQUE_SET(txdesc, skb);

	/*
	 * Set the data pointer as the buffer address in the descriptor.
	 */
	txdesc->word0 = (dma_addr_t)virt_to_phys(skb->data);
	EDMA_TXDESC_SERVICE_CODE_SET(txdesc, EDMA_SC_BYPASS);

	dmac_clean_range((void *)skb->data, (void *)(skb->data + buf_len));

	/*
	 * Set packet length in the descriptor
	 */
	EDMA_TXDESC_DATA_LEN_SET(txdesc, buf_len);

	/*
	 * Set destination information in the descriptor
	 */
	EDMA_DST_INFO_SET(txdesc, dp_dev->macid);

	edma_debug("%s: skb:%px tx_ring:%u proto:0x%x skb->len:%d\n"
			" port:%u prod_idx:%u cons_idx:%u\n",
			netdev->name, skb, txdesc_ring->id, ntohs(skb->protocol),
			skb->len, dp_dev->macid, hw_next_to_use, hw_next_to_clean);

	/*
	 * Update producer index
	 */
	hw_next_to_use = (hw_next_to_use + 1) & EDMA_TX_RING_SIZE_MASK;
	txdesc_ring->prod_idx = hw_next_to_use & EDMA_TXDESC_PROD_IDX_MASK;
	txdesc_ring->avail_desc--;

	dmac_clean_range(txdesc, txdesc + 1);

	edma_reg_write(EDMA_REG_TXDESC_PROD_IDX(txdesc_ring->id),
			hw_next_to_use & EDMA_TXDESC_PROD_IDX_MASK);

	u64_stats_update_begin(&stats->syncp);
	++stats->tx_pkts;
	stats->tx_bytes += skb->len;
	u64_stats_update_end(&stats->syncp);
	return EDMA_TX_OK;
}
