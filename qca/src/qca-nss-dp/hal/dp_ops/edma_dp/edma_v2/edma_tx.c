/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
	uint32_t avail, count;
	uint32_t end_idx;
	uint32_t more_bit = 0;

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
		dmac_inv_range_no_dsb((void *)txcmpl, txcmpl + avail);
	} else {
		dmac_inv_range_no_dsb(txcmpl_ring->desc, txcmpl_ring->desc + end_idx);
		dmac_inv_range_no_dsb((void *)txcmpl, txcmpl_ring->desc + EDMA_TX_RING_SIZE);
	}

	dsb(st);

	/*
	 * TODO:
	 * Instead of freeing the skb, it might be better to save and use
	 * for Rxfill.
	 */
	while (likely(avail--)) {

		/*
		 * The last descriptor holds the SKB pointer for scattered frames.
		 * So skip the descriptors with more bit set.
		 */
		more_bit = EDMA_TXCMPL_MORE_BIT_GET(txcmpl);
		if (unlikely(more_bit)) {
			cons_idx = ((cons_idx + 1) & EDMA_TX_RING_SIZE_MASK);
			txcmpl = EDMA_TXCMPL_DESC(txcmpl_ring, cons_idx);
			continue;
		}

		/*
		 * Find and free the skb for Tx completion
		 */
		skb = (struct sk_buff *)EDMA_TXCMPL_OPAQUE_GET(txcmpl);
		if (unlikely(!skb)) {
			edma_warn("Invalid skb: cons_idx:%u prod_idx:%u word2:%x word3:%x\n",
					cons_idx, prod_idx, txcmpl->word2, txcmpl->word3);
		} else {
			edma_debug("skb:%px, skb->len %d, skb->data_len %d, cons_idx:%d prod_idx:%d word2:0x%x word3:0x%x\n",
					skb, skb->len, skb->data_len, cons_idx, prod_idx, txcmpl->word2, txcmpl->word3);

			dev_kfree_skb_any(skb);
		}

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
 * edma_tx_desc_init()
 *	Initializes the Tx descriptor
 */
static inline void edma_tx_desc_init(struct edma_pri_txdesc *txdesc)
{
	memset(txdesc, 0, sizeof(struct edma_pri_txdesc));
}

/*
 * edma_tx_skb_nr_frags()
 *	Process Tx for skb with nr_frags
 */
static uint32_t edma_tx_skb_nr_frags(struct edma_txdesc_ring *txdesc_ring, struct edma_pri_txdesc **txdesc,
		struct sk_buff *skb, uint32_t *hw_next_to_use)
{
	uint8_t i = 0;
	uint32_t nr_frags = 0, buf_len = 0, num_descs = 0, start_idx = 0, end_idx = 0;
	struct edma_pri_txdesc *txd = *txdesc;

	/*
	 * Hold onto the index mapped to *txdesc.
	 * This will be the index previous to that of current *hw_next_to_use
	 */
	start_idx = (((*hw_next_to_use) + EDMA_TX_RING_SIZE_MASK) & EDMA_TX_RING_SIZE_MASK);

	/*
	 * Handle if the skb has nr_frags
	 */
	nr_frags = skb_shinfo(skb)->nr_frags;
	num_descs = nr_frags;
	i = 0;
	while (nr_frags--) {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[i];
		buf_len = skb_frag_size(frag);

		/*
		 * Setting the MORE bit on the previous Tx descriptor.
		 * Note: We will flush this descriptor as well later.
		 */
		EDMA_TXDESC_MORE_BIT_SET(txd, 1);

		txd = EDMA_TXDESC_PRI_DESC(txdesc_ring, *hw_next_to_use);
		edma_tx_desc_init(txd);
		txd->word0 = (dma_addr_t)virt_to_phys(skb_frag_address(frag));
		dmac_clean_range_no_dsb((void *)skb_frag_address(frag),
				(void *)(skb_frag_address(frag) + buf_len));

		EDMA_TXDESC_DATA_LEN_SET(txd, buf_len);

		*hw_next_to_use = ((*hw_next_to_use + 1) & EDMA_TX_RING_SIZE_MASK);
		i++;
	}

	/*
	 * This will be the index previous to that of current *hw_next_to_use
	 */
	end_idx = (((*hw_next_to_use) + EDMA_TX_RING_SIZE_MASK) & EDMA_TX_RING_SIZE_MASK);

	/*
	 * Need to flush from initial *txdesc to accomodate for MORE bit change.
	 * Need to flush all the descriptors but last-one that we filled as well.
	 */
	if (end_idx > start_idx) {
		dmac_clean_range_no_dsb((void *)(*txdesc),
				((*txdesc) + num_descs));
	} else {
		dmac_clean_range_no_dsb(txdesc_ring->pdesc,
				txdesc_ring->pdesc + end_idx);
		dmac_clean_range_no_dsb((void *)(*txdesc),
				txdesc_ring->pdesc + EDMA_TX_RING_SIZE);
	}

	*txdesc = txd;
	return num_descs;
}

/*
 * edma_tx_skb_first_desc()
 *	Process the Tx for the first descriptor required for skb
 */
static struct edma_pri_txdesc *edma_tx_skb_first_desc(struct nss_dp_dev *dp_dev, struct edma_txdesc_ring *txdesc_ring,
					struct sk_buff *skb, uint32_t *hw_next_to_use, struct edma_tx_stats *stats)
{
	uint32_t buf_len = 0, mss = 0;
	struct edma_pri_txdesc *txd = NULL;

	/*
	 * Get the packet length
	 */
	buf_len = skb_headlen(skb);

	txd = EDMA_TXDESC_PRI_DESC(txdesc_ring, *hw_next_to_use);

	edma_tx_desc_init(txd);

	/*
	 * Set the data pointer as the buffer address in the descriptor.
	 */
	txd->word0 = (dma_addr_t)virt_to_phys(skb->data);
	dmac_clean_range_no_dsb((void *)skb->data, (void *)(skb->data + buf_len));

	EDMA_TXDESC_SERVICE_CODE_SET(txd, EDMA_SC_BYPASS);

	/*
	 * Offload L3/L4 checksum computation
	 */
	if (likely(skb->ip_summed == CHECKSUM_PARTIAL)) {
		EDMA_TXDESC_ADV_OFFLOAD_SET(txd);
		EDMA_TXDESC_IP_CSUM_SET(txd);
		EDMA_TXDESC_L4_CSUM_SET(txd);
	}

	/*
	 * Check if the packet needs TSO
	 */
	if (unlikely(skb_is_gso(skb))) {
		if ((skb_shinfo(skb)->gso_type == SKB_GSO_TCPV4) ||
				(skb_shinfo(skb)->gso_type == SKB_GSO_TCPV6)){
			mss = skb_shinfo(skb)->gso_size;
			EDMA_TXDESC_TSO_ENABLE_SET(txd, 1);
			EDMA_TXDESC_MSS_SET(txd, mss);

			/*
			 * Update tso stats
			 */
			u64_stats_update_begin(&stats->syncp);
			stats->tx_tso_pkts++;
			u64_stats_update_end(&stats->syncp);
		}
	}

	/*
	 * Set packet length in the descriptor
	 */
	EDMA_TXDESC_DATA_LEN_SET(txd, buf_len);

	/*
	 * Set destination information in the descriptor
	 */
	EDMA_DST_INFO_SET(txd, dp_dev->macid);

	*hw_next_to_use = (*hw_next_to_use + 1) & EDMA_TX_RING_SIZE_MASK;

	return txd;
}

/*
 * edma_tx_skb_sg_fill_desc()
 * 	API to fill SG skb into Tx descriptor. Handles both nr_frags and fraglist cases.
 */
static uint32_t edma_tx_skb_sg_fill_desc(struct nss_dp_dev *dp_dev, struct edma_txdesc_ring *txdesc_ring,
		struct edma_pri_txdesc **txdesc, struct sk_buff *skb, uint32_t *hw_next_to_use,
		struct edma_tx_stats *stats)
{
	uint32_t buf_len = 0, num_descs = 0;
	struct sk_buff *iter_skb = NULL;
	uint32_t num_sg_frag_list = 0;
	struct edma_pri_txdesc *txd = *txdesc;

	/*
	 * Process head skb
	 */
	txd = edma_tx_skb_first_desc(dp_dev, txdesc_ring, skb, hw_next_to_use, stats);
	num_descs++;

	/*
	 * Process skb with nr_frags
	 */
	if (unlikely(skb_shinfo(skb)->nr_frags)) {
		num_descs += edma_tx_skb_nr_frags(txdesc_ring, &txd, skb, hw_next_to_use);
		u64_stats_update_begin(&stats->syncp);
		stats->tx_nr_frag_pkts++;
		u64_stats_update_end(&stats->syncp);
	}

	if (unlikely(skb_has_frag_list(skb))) {
		struct edma_pri_txdesc *start_desc = NULL;
		uint32_t start_idx = 0, end_idx = 0;

		/*
		 * Hold onto the index mapped to txd.
		 * This will be the index previous to that of current *hw_next_to_use
		 */
		start_idx = (((*hw_next_to_use) + EDMA_TX_RING_SIZE_MASK) & EDMA_TX_RING_SIZE_MASK);
		start_desc = txd;

		/*
		 * Walk through all fraglist skbs
		 */
		skb_walk_frags(skb, iter_skb) {
			uint32_t num_nr_frag = 0;

			buf_len = skb_headlen(iter_skb);

			/*
			 * We make sure to flush this descriptor later
			 */
			EDMA_TXDESC_MORE_BIT_SET(txd, 1);

			txd = EDMA_TXDESC_PRI_DESC(txdesc_ring, *hw_next_to_use);
			edma_tx_desc_init(txd);
			txd->word0 = (dma_addr_t)virt_to_phys(iter_skb->data);
			dmac_clean_range_no_dsb((void *)iter_skb->data,
					(void *)(iter_skb->data + buf_len));

			EDMA_TXDESC_DATA_LEN_SET(txd, buf_len);

			*hw_next_to_use = (*hw_next_to_use + 1) & EDMA_TX_RING_SIZE_MASK;
			num_descs += 1;
			num_sg_frag_list += 1;

			/*
			 * skb fraglist skb can have nr_frags
			 */
			if (unlikely(skb_shinfo(iter_skb)->nr_frags)) {
				num_nr_frag = edma_tx_skb_nr_frags(txdesc_ring, &txd, iter_skb, hw_next_to_use);
				num_descs += num_nr_frag;
				num_sg_frag_list += num_nr_frag;

				/*
				 * Update fraglist with nr_frag stats
				 */
				u64_stats_update_begin(&stats->syncp);
				stats->tx_fraglist_with_nr_frags_pkts++;
				u64_stats_update_end(&stats->syncp);
			}
		}

		/*
		 * This will be the index previous to
		 * that of current *hw_next_to_use
		 */
		end_idx = (((*hw_next_to_use) + EDMA_TX_RING_SIZE_MASK) &
					EDMA_TX_RING_SIZE_MASK);

		/*
		 * Need to flush from initial txd to accomodate for MORE bit change.
		 * Need to flush all the descriptors but last-one that we filled as well.
		 * This may result double flush if fraglist iter_skb has nr_frags which is rare.
		 */
		if (end_idx > start_idx) {
			dmac_clean_range_no_dsb((void *)(start_desc),
					((start_desc) + num_sg_frag_list));
		} else {
			dmac_clean_range_no_dsb(txdesc_ring->pdesc,
					txdesc_ring->pdesc + end_idx);
			dmac_clean_range_no_dsb((void *)(start_desc),
					txdesc_ring->pdesc + EDMA_TX_RING_SIZE);
		}

		/*
		 * Update frag_list stats
		 */
		u64_stats_update_begin(&stats->syncp);
		stats->tx_fraglist_pkts++;
		u64_stats_update_end(&stats->syncp);
	}

	edma_debug("skb:%px num_descs_filled: %u, nr_frags %u frag_list fragments %u\n",
			skb, num_descs, skb_shinfo(skb)->nr_frags, num_sg_frag_list);

	*txdesc = txd;
	return num_descs;
}

/*
 * edma_tx_num_descs_for_sg()
 *	Calculates number of descriptors needed for SG
 */
static uint32_t edma_tx_num_descs_for_sg(struct sk_buff *skb)
{
	uint32_t nr_frags_first = 0, num_tx_desc_needed = 0;

	/*
	 * Check if we have enough Tx descriptors for SG
	 */
	if (unlikely(skb_shinfo(skb)->nr_frags)) {
		nr_frags_first = skb_shinfo(skb)->nr_frags;
		BUG_ON(nr_frags_first > MAX_SKB_FRAGS);
		num_tx_desc_needed += nr_frags_first;
	}

	/*
	 * Walk through fraglist skbs making a note of nr_frags
	 * One Tx desc for fraglist skb. Fraglist skb may have further nr_frags.
	 */
	if (unlikely(skb_has_frag_list(skb))) {
		struct sk_buff *iter_skb;
		skb_walk_frags(skb, iter_skb) {
			uint32_t nr_frags = skb_shinfo(iter_skb)->nr_frags;
			BUG_ON(nr_frags > MAX_SKB_FRAGS);
			num_tx_desc_needed += (1 + nr_frags);
		}
	}

	return num_tx_desc_needed;
}

/*
 * edma_tx_avail_desc()
 *	Get available Tx descriptors
 */
static uint32_t edma_tx_avail_desc(struct edma_txdesc_ring *txdesc_ring, uint32_t hw_next_to_use)
{
	uint32_t data = 0, avail = 0, hw_next_to_clean = 0;

	data = edma_reg_read(EDMA_REG_TXDESC_CONS_IDX(txdesc_ring->id));
	hw_next_to_clean = data & EDMA_TXDESC_CONS_IDX_MASK;

	avail = EDMA_DESC_AVAIL_COUNT(hw_next_to_clean - 1, hw_next_to_use, EDMA_TX_RING_SIZE);

	return avail;
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
	uint32_t hw_next_to_use = 0;
	uint32_t num_tx_desc_needed = 0, num_desc_filled = 0;
	struct edma_pri_txdesc *txdesc = NULL;

	hw_next_to_use = txdesc_ring->prod_idx;

	if (unlikely(!(txdesc_ring->avail_desc)))  {
		txdesc_ring->avail_desc = edma_tx_avail_desc(txdesc_ring, hw_next_to_use);
		if (unlikely(!txdesc_ring->avail_desc)) {
			edma_debug("No available descriptors are present at %d ring\n",
					txdesc_ring->id);
			u64_stats_update_begin(&stats->syncp);
			++stats->tx_no_desc_avail;
			u64_stats_update_end(&stats->syncp);
			return EDMA_TX_FAIL_NO_DESC;
		}
	}

	/*
	 * Process head skb for linear skb
	 * Process head skb + nr_frags + fraglist for non linear skb
	 */
	if (likely(!skb_is_nonlinear(skb))) {
		txdesc = edma_tx_skb_first_desc(dp_dev, txdesc_ring, skb, &hw_next_to_use, stats);
		num_desc_filled++;
	} else {
		num_tx_desc_needed += 1;
		num_tx_desc_needed += edma_tx_num_descs_for_sg(skb);

		if (unlikely(num_tx_desc_needed > txdesc_ring->avail_desc)) {
			txdesc_ring->avail_desc = edma_tx_avail_desc(txdesc_ring, hw_next_to_use);
			if (num_tx_desc_needed > txdesc_ring->avail_desc) {
				u64_stats_update_begin(&stats->syncp);
				++stats->tx_no_desc_avail;
				u64_stats_update_end(&stats->syncp);
				edma_debug("Not enough available descriptors are present at %d ring for SG packet. Needed %d, currently available %d\n",
					txdesc_ring->id, num_tx_desc_needed, txdesc_ring->avail_desc);
				return EDMA_TX_FAIL_NO_DESC;
			}
		}

		num_desc_filled = edma_tx_skb_sg_fill_desc(dp_dev, txdesc_ring, &txdesc, skb, &hw_next_to_use, stats);
	}

	/*
	 * Set the skb pointer to the descriptor's opaque field/s
	 * on the last descriptor of the packet/SG packet.
	 */
	EDMA_TXDESC_OPAQUE_SET(txdesc, skb);

	/*
	 * Flush the last descriptor.
	 */
	dmac_clean_range_no_dsb(txdesc, txdesc + 1);

	/*
	 * Update producer index
	 */
	txdesc_ring->prod_idx = hw_next_to_use & EDMA_TXDESC_PROD_IDX_MASK;
	txdesc_ring->avail_desc -= num_desc_filled;

	edma_debug("%s: skb:%px tx_ring:%u proto:0x%x skb->len:%d\n"
			" port:%u prod_idx:%u ip_summed:0x%x\n",
			netdev->name, skb, txdesc_ring->id, ntohs(skb->protocol),
			skb->len, dp_dev->macid, hw_next_to_use, skb->ip_summed);

	/*
	 * Make sure the information written to the descriptors
	 * is updated before writing to the hardware.
	 */
	dsb(st);

	edma_reg_write(EDMA_REG_TXDESC_PROD_IDX(txdesc_ring->id),
			txdesc_ring->prod_idx);

	u64_stats_update_begin(&stats->syncp);
	stats->tx_pkts += 1;
	stats->tx_bytes += skb->len;
	u64_stats_update_end(&stats->syncp);
	return EDMA_TX_OK;
}
