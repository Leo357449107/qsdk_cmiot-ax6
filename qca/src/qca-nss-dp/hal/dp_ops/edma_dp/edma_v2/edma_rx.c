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

#include <asm/cacheflush.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include "edma.h"
#include "edma_debug.h"
#include "edma_regs.h"
#include "nss_dp_dev.h"

/*
 * edma_rx_alloc_buffer()
 *	Alloc Rx buffers for one RxFill ring
 */
int edma_rx_alloc_buffer(struct edma_rxfill_ring *rxfill_ring, int alloc_count)
{
	struct edma_rxfill_desc *rxfill_desc;
	uint16_t prod_idx, start_idx;
	uint16_t num_alloc = 0;

	/*
	 * Get RXFILL ring producer index
	 */
	prod_idx = rxfill_ring->prod_idx;
	start_idx = prod_idx;

	while (likely(alloc_count--)) {
		struct sk_buff *skb;
		dma_addr_t buff_addr;

		/*
		 * Allocate buffer
		 */
		skb = dev_alloc_skb(EDMA_BUF_SIZE + NET_IP_ALIGN);
		if (unlikely(!skb)) {
			break;
		}

		/*
		 * Reserve headroom
		 */
		skb_reserve(skb, EDMA_RX_SKB_HEADROOM + NET_IP_ALIGN);

		/*
		 * Get RXFILL descriptor
		 */
		rxfill_desc = EDMA_RXFILL_DESC(rxfill_ring, prod_idx);

		/*
		 * Map Rx buffer for DMA
		 */
		buff_addr = (dma_addr_t)virt_to_phys(skb->data);
		EDMA_RXFILL_BUFFER_ADDR_SET(rxfill_desc, buff_addr);

		/*
		 * Store skb in opaque
		 */
		EDMA_RXFILL_OPAQUE_LO_SET(rxfill_desc, skb);
#ifdef __LP64__
		EDMA_RXFILL_OPAQUE_HI_SET(rxfill_desc, skb);
#endif

		/*
		 * Save buffer size in RXFILL descriptor
		 */
		EDMA_RXFILL_PACKET_LEN_SET(
			rxfill_desc,
			cpu_to_le32((uint32_t)
			(EDMA_BUF_SIZE - EDMA_RX_SKB_HEADROOM - NET_IP_ALIGN)
			& EDMA_RXFILL_BUF_SIZE_MASK));

		/*
		 * Invalidate skb->data
		 */
		dmac_inv_range((void *)skb->data,
				(void *)(skb->data + EDMA_BUF_SIZE -
					EDMA_RX_SKB_HEADROOM -
					NET_IP_ALIGN));
		prod_idx = (prod_idx + 1) & EDMA_RX_RING_SIZE_MASK;
		num_alloc++;
	}

	if (likely(num_alloc)) {
		uint16_t end_idx =
			(start_idx + num_alloc) & EDMA_RX_RING_SIZE_MASK;

		rxfill_desc = EDMA_RXFILL_DESC(rxfill_ring, start_idx);

		/*
		 * Write-back all the cached descriptors
		 * that are processed.
		 */
		if (end_idx > start_idx) {
			dmac_clean_range((void *)rxfill_desc,
					(void *)(rxfill_desc + num_alloc));
		} else {
			dmac_clean_range((void *)rxfill_ring->desc,
					(void *)(rxfill_ring->desc + end_idx));
			dmac_clean_range((void *)rxfill_desc,
					(void *)(rxfill_ring->desc +
							EDMA_RX_RING_SIZE));
		}

		edma_reg_write(EDMA_REG_RXFILL_PROD_IDX(rxfill_ring->ring_id),
								prod_idx);
		rxfill_ring->prod_idx = prod_idx;
	}

	return num_alloc;
}

/*
 * edma_rx_reap()
 *	Reap Rx descriptors
 */
static uint32_t edma_rx_reap(struct edma_gbl_ctx *egc, int budget,
				struct edma_rxdesc_ring *rxdesc_ring)
{
	struct edma_rxdesc_desc *rxdesc_desc;
	uint32_t work_to_do, work_done = 0;
	uint32_t work_leftover;
	uint16_t prod_idx, cons_idx, end_idx;

	/*
	 * Get Rx ring producer and consumer indices
	 */
	cons_idx = rxdesc_ring->cons_idx;

	if (likely(rxdesc_ring->work_leftover > EDMA_RX_MAX_PROCESS)) {
		work_to_do = rxdesc_ring->work_leftover;
	} else {
		prod_idx =
		edma_reg_read(EDMA_REG_RXDESC_PROD_IDX(rxdesc_ring->ring_id)) &
				EDMA_RXDESC_PROD_IDX_MASK;
		work_to_do = EDMA_DESC_AVAIL_COUNT(prod_idx,
				cons_idx, EDMA_RX_RING_SIZE);
		rxdesc_ring->work_leftover = work_to_do;
	}

	if (work_to_do > budget) {
		work_to_do = budget;
	}
	rxdesc_ring->work_leftover -= work_to_do;

	end_idx = (cons_idx + work_to_do) & EDMA_RX_RING_SIZE_MASK;

	rxdesc_desc = EDMA_RXDESC_PRI_DESC(rxdesc_ring, cons_idx);

	/*
	 * Invalidate all the cached descriptors
	 * that'll be processed.
	 */
	if (end_idx > cons_idx) {
		dmac_inv_range((void *)rxdesc_desc,
			(void *)(rxdesc_desc + work_to_do));
	} else {
		dmac_inv_range((void *)rxdesc_ring->pdesc,
			(void *)(rxdesc_ring->pdesc + end_idx));
		dmac_inv_range((void *)rxdesc_desc,
			(void *)(rxdesc_ring->pdesc + EDMA_RX_RING_SIZE));
	}

	work_leftover = work_to_do & (EDMA_RX_MAX_PROCESS - 1);
	while (likely(work_to_do--)) {
		struct net_device *ndev;
		struct sk_buff *skb;
		struct nss_dp_dev *dp_dev;
		struct edma_pcpu_stats *pcpu_stats;
		struct edma_rx_stats *rx_stats;
		uint32_t src_port_num;
		uint32_t pkt_length;

		/*
		 * Get opaque from RXDESC
		 */
		skb = (struct sk_buff *)EDMA_RXDESC_OPAQUE_GET(rxdesc_desc);

		/*
		 * Check src_info
		 */
		src_port_num = EDMA_RXDESC_SRC_INFO_GET(rxdesc_desc);
		if (likely((src_port_num & EDMA_RXDESC_SRCINFO_TYPE_MASK) ==
					EDMA_RXDESC_SRCINFO_TYPE_PORTID)) {
			src_port_num &= EDMA_RXDESC_PORTNUM_BITS;
		} else {
			edma_warn("Src_info_type:0x%x. Drop skb:%px\n",
			  (src_port_num & EDMA_RXDESC_SRCINFO_TYPE_MASK), skb);
			dev_kfree_skb_any(skb);
			goto next_rx_desc;
		}

		if (unlikely((src_port_num < NSS_DP_START_IFNUM)  ||
			(src_port_num > NSS_DP_HAL_MAX_PORTS))) {
			edma_warn("Port number error :%d. Drop skb:%px\n",
					src_port_num, skb);
			dev_kfree_skb_any(skb);
			goto next_rx_desc;
		}

		/*
		 * Get netdev for this port using the source port
		 * number as index into the netdev array. We need to
		 * subtract one since the indices start form '0' and
		 * port numbers start from '1'.
		 */
		ndev = egc->netdev_arr[src_port_num - 1];
		if (unlikely(!ndev)) {
			edma_warn("Netdev Null src_info_type:0x%x. \
					Drop skb:%px\n", src_port_num, skb);
			dev_kfree_skb_any(skb);
			goto next_rx_desc;
		}

		/*
		 * Get packet length
		 */
		pkt_length = EDMA_RXDESC_PACKET_LEN_GET(rxdesc_desc);

		/*
		 * Invalidate the buffer received from the HW
		 */
		dmac_inv_range((void *)skb->data,
			(void *)(skb->data + pkt_length));

		/*
		 * Update skb fields and indicate packet to stack
		 */
		skb->skb_iif = ndev->ifindex;
		skb_put(skb, pkt_length);
		skb->protocol = eth_type_trans(skb, ndev);

		/*
		 * TODO: Enable Rx checksum offload.
		 */
		skb->ip_summed = CHECKSUM_NONE;

#ifdef CONFIG_NET_SWITCHDEV
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0))
		skb->offload_fwd_mark = ndev->offload_fwd_mark;
#else
		/*
		 * TODO: Implement ndo_get_devlink_port()
		 */
		skb->offload_fwd_mark = 0;
#endif
		edma_debug("skb:%px ring_idx:%u pktlen:%d proto:0x%x mark:%u\n",
			   skb, cons_idx, pkt_length, skb->protocol,
			   skb->offload_fwd_mark);
#else
		edma_debug("skb:%px ring_idx:%u pktlen:%d proto:0x%x\n",
			   skb, cons_idx, pkt_length, skb->protocol);
#endif

		dp_dev = netdev_priv(ndev);
		pcpu_stats = &dp_dev->dp_info.pcpu_stats;
		rx_stats = this_cpu_ptr(pcpu_stats->rx_stats);

		/*
		 * TODO: Do a batched update of the stats per netdevice.
		 */
		u64_stats_update_begin(&rx_stats->syncp);
		rx_stats->rx_pkts++;
		rx_stats->rx_bytes += pkt_length;
		u64_stats_update_end(&rx_stats->syncp);

		/*
		 * Send packet upto network stack
		 */
		netif_receive_skb(skb);

next_rx_desc:
		/*
		 * Update consumer index
		 */
		cons_idx = (cons_idx + 1) & EDMA_RX_RING_SIZE_MASK;

		rxdesc_desc = EDMA_RXDESC_PRI_DESC(rxdesc_ring, cons_idx);

		/*
		 * Update work done
		 */
		work_done++;

		/*
		 * Check if we can refill EDMA_RX_MAX_PROCESS worth buffers,
		 * if yes, refill and update index before continuing.
		 */
		if (unlikely(!(work_done & (EDMA_RX_MAX_PROCESS - 1)))) {
			edma_reg_write(
				EDMA_REG_RXDESC_CONS_IDX(rxdesc_ring->ring_id),
				cons_idx);
			rxdesc_ring->cons_idx = cons_idx;
			edma_rx_alloc_buffer(rxdesc_ring->rxfill,
				EDMA_RX_MAX_PROCESS);
		}
	}

	/*
	 * Check if we need to refill and update
	 * index for any buffers before exit.
	 */
	if (unlikely(work_leftover)) {
		edma_reg_write(EDMA_REG_RXDESC_CONS_IDX(rxdesc_ring->ring_id),
					cons_idx);
		rxdesc_ring->cons_idx = cons_idx;
		edma_rx_alloc_buffer(rxdesc_ring->rxfill, work_leftover);
	}

	return work_done;
}

/*
 * edma_rx_napi_poll()
 *	EDMA RX NAPI handler
 */
int edma_rx_napi_poll(struct napi_struct *napi, int budget)
{
	struct edma_rxdesc_ring *rxdesc_ring = (struct edma_rxdesc_ring *)napi;
	struct edma_gbl_ctx *egc = &edma_gbl_ctx;
	int32_t work_done = 0;
	uint32_t status;

	do {
		work_done += edma_rx_reap(egc, budget - work_done, rxdesc_ring);
		if (likely(work_done >= budget)) {
			return work_done;
		}

		/*
		 * Check if there are more packets to process
		 */
		status = EDMA_RXDESC_RING_INT_STATUS_MASK &
			edma_reg_read(
				EDMA_REG_RXDESC_INT_STAT(rxdesc_ring->ring_id));
	} while (likely(status));

	/*
	 * No more packets to process. Finish NAPI processing.
	 */
	napi_complete(napi);

	/*
	 * Set RXDESC ring interrupt mask
	 */
	edma_reg_write(EDMA_REG_RXDESC_INT_MASK(rxdesc_ring->ring_id),
						egc->rxdesc_intr_mask);

	return work_done;
}

/*
 * edma_rx_handle_irq()
 *	Process RX IRQ and schedule napi
 */
irqreturn_t edma_rx_handle_irq(int irq, void *ctx)
{
	struct edma_rxdesc_ring *rxdesc_ring = (struct edma_rxdesc_ring *)ctx;

	edma_debug("irq: irq=%d rxdesc_ring_id=%u\n", irq, rxdesc_ring->ring_id);

	if (likely(napi_schedule_prep(&rxdesc_ring->napi))) {

		/*
		 * Disable RxDesc interrupt
		 */
		edma_reg_write(EDMA_REG_RXDESC_INT_MASK(rxdesc_ring->ring_id),
							EDMA_MASK_INT_DISABLE);
		__napi_schedule(&rxdesc_ring->napi);
	}

	return IRQ_HANDLED;
}
