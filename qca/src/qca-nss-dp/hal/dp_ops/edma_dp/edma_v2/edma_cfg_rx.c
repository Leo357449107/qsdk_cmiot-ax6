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

#include <linux/debugfs.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/reset.h>
#include "edma.h"
#include "edma_cfg_rx.h"
#include "edma_regs.h"
#include "edma_debug.h"

/*
 * edma_cfg_rx_fill_ring_cleanup()
 *	Cleanup resources for one RxFill ring
 *
 * API expects ring to be disabled by caller
 */
static void edma_cfg_rx_fill_ring_cleanup(struct edma_gbl_ctx *egc,
				struct edma_rxfill_ring *rxfill_ring)
{
	uint16_t cons_idx, curr_idx;
	uint32_t reg_data;

	/*
	 * Get RXFILL ring producer index
	 */
	curr_idx = rxfill_ring->prod_idx & EDMA_RXFILL_PROD_IDX_MASK;

	/*
	 * Get RXFILL ring consumer index
	 */
	reg_data = edma_reg_read(EDMA_REG_RXFILL_CONS_IDX(rxfill_ring->ring_id));
	cons_idx = reg_data & EDMA_RXFILL_CONS_IDX_MASK;

	while (curr_idx != cons_idx) {
		struct sk_buff *skb;
		struct edma_rxfill_desc *rxfill_desc;

		/*
		 * Get RXFILL descriptor
		 */
		rxfill_desc = EDMA_RXFILL_DESC(rxfill_ring, cons_idx);

		cons_idx = (cons_idx + 1) & EDMA_RX_RING_SIZE_MASK;

		/*
		 * Get skb from opaque
		 */
		skb = (struct sk_buff *)EDMA_RXFILL_OPAQUE_GET(rxfill_desc);
		if (unlikely(!skb)) {
			edma_warn("Empty skb reference at index:%d\n",
					cons_idx);
			continue;
		}
		dev_kfree_skb_any(skb);
	}

	/*
	 * Free RXFILL ring descriptors
	 */
	kfree(rxfill_ring->desc);
	rxfill_ring->desc = NULL;
	rxfill_ring->dma = (dma_addr_t)0;
}

/*
 * edma_cfg_rx_fill_ring_setup()
 *	Setup resources for one RxFill ring
 */
static int edma_cfg_rx_fill_ring_setup(struct edma_rxfill_ring *rxfill_ring)
{
	/*
	 * Allocate RxFill ring descriptors
	 */
	rxfill_ring->desc = kmalloc((sizeof(struct edma_rxfill_desc) * rxfill_ring->count) +
				SMP_CACHE_BYTES,  GFP_KERNEL | __GFP_ZERO);
	if (!rxfill_ring->desc) {
		edma_err("Descriptor alloc for RXFILL ring %u failed\n",
							rxfill_ring->ring_id);
		return -ENOMEM;
	}

	rxfill_ring->dma = (dma_addr_t)virt_to_phys(rxfill_ring->desc);

	return 0;
}

/*
 * edma_cfg_rx_desc_ring_setup()
 *	Setup resources for one RxDesc ring
 */
static int edma_cfg_rx_desc_ring_setup(struct edma_rxdesc_ring *rxdesc_ring)
{
	/*
	 * Allocate RxDesc ring descriptors
	 */
	rxdesc_ring->pdesc = kmalloc((sizeof(struct edma_rxdesc_desc) *  rxdesc_ring->count) +
				SMP_CACHE_BYTES,  GFP_KERNEL | __GFP_ZERO);
	if (!rxdesc_ring->pdesc) {
		edma_err("Descriptor alloc for RXDESC ring %u failed\n",
							rxdesc_ring->ring_id);
		return -ENOMEM;
	}

	rxdesc_ring->pdma = (dma_addr_t)virt_to_phys(rxdesc_ring->pdesc);

	/*
	 * Allocate secondary RxDesc ring descriptors
	 */
	rxdesc_ring->sdesc = kmalloc((sizeof(struct edma_rxdesc_sec_desc) *  rxdesc_ring->count) +
				SMP_CACHE_BYTES,  GFP_KERNEL | __GFP_ZERO);
	if (!rxdesc_ring->sdesc) {
		edma_err("Descriptor alloc for secondary RX ring %u failed\n",
							rxdesc_ring->ring_id);
		kfree(rxdesc_ring->pdesc);
		rxdesc_ring->pdesc = NULL;
		rxdesc_ring->pdma = (dma_addr_t)0;
		return -ENOMEM;
	}

	rxdesc_ring->sdma = (dma_addr_t)virt_to_phys(rxdesc_ring->sdesc);

	return 0;
}

/*
 * edma_cfg_rx_desc_ring_cleanup()
 *	Cleanup resources for RxDesc ring
 *
 * API expects ring to be disabled by caller
 */
static void edma_cfg_rx_desc_ring_cleanup(struct edma_gbl_ctx *egc,
				struct edma_rxdesc_ring *rxdesc_ring)
{
	uint16_t prod_idx, cons_idx;

	/*
	 * Get Rxdesc consumer & producer indices
	 */
	cons_idx = rxdesc_ring->cons_idx & EDMA_RXDESC_CONS_IDX_MASK;

	prod_idx = edma_reg_read(EDMA_REG_RXDESC_PROD_IDX(rxdesc_ring->ring_id))
					& EDMA_RXDESC_PROD_IDX_MASK;

	/*
	 * Free any buffers assigned to any descriptors
	 */
	while (cons_idx != prod_idx) {
		struct sk_buff *skb;
		struct edma_rxdesc_desc *rxdesc_desc =
			EDMA_RXDESC_PRI_DESC(rxdesc_ring, cons_idx);

		/*
		 * Update consumer index
		 */
		cons_idx = (cons_idx + 1) & EDMA_RX_RING_SIZE_MASK;

		/*
		 * Get opaque from RXDESC
		 */
		skb = (struct sk_buff *)EDMA_RXDESC_OPAQUE_GET(rxdesc_desc);
		if (unlikely(!skb)) {
			edma_warn("Empty skb reference at index:%d\n",
								cons_idx);
			continue;
		}
		dev_kfree_skb_any(skb);
	}

	/*
	 * Free RXDESC ring descriptors
	 */
	kfree(rxdesc_ring->pdesc);
	rxdesc_ring->pdesc = NULL;
	rxdesc_ring->pdma = (dma_addr_t)0;

	/*
	 * TODO:
	 * Free any buffers assigned to any secondary ring descriptors
	 */
	kfree(rxdesc_ring->sdesc);
	rxdesc_ring->sdesc = NULL;
	rxdesc_ring->sdma = (dma_addr_t)0;
}

/*
 * edma_cfg_rx_desc_ring_configure()
 *	Configure one RxDesc ring in EDMA HW
 */
static void edma_cfg_rx_desc_ring_configure(struct edma_gbl_ctx *egc,
					struct edma_rxdesc_ring *rxdesc_ring)
{
	uint32_t data;

	edma_reg_write(EDMA_REG_RXDESC_BA(rxdesc_ring->ring_id),
			(uint32_t)(rxdesc_ring->pdma & EDMA_RXDESC_BA_MASK));

	edma_reg_write(EDMA_REG_RXDESC_PREHEADER_BA(rxdesc_ring->ring_id),
			(uint32_t)(rxdesc_ring->sdma & EDMA_RXDESC_PREHEADER_BA_MASK));

	data = rxdesc_ring->count & EDMA_RXDESC_RING_SIZE_MASK;
	data |= (EDMA_RXDESC_PL_DEFAULT_VALUE & EDMA_RXDESC_PL_OFFSET_MASK)
		 << EDMA_RXDESC_PL_OFFSET_SHIFT;
	edma_reg_write(EDMA_REG_RXDESC_RING_SIZE(rxdesc_ring->ring_id), data);

	/*
	 * Configure the default timer mitigation value
	 */
	data = (EDMA_RX_MOD_TIMER_INIT & EDMA_RX_MOD_TIMER_INIT_MASK)
			<< EDMA_RX_MOD_TIMER_INIT_SHIFT;
	edma_reg_write(EDMA_REG_RX_MOD_TIMER(rxdesc_ring->ring_id), data);

	/*
	 * Enable ring. Set ret mode to 'opaque'.
	 */
	edma_reg_write(EDMA_REG_RX_INT_CTRL(rxdesc_ring->ring_id), EDMA_RX_NE_INT_EN);
}

/*
 * edma_cfg_rx_fill_ring_configure()
 *	Configure one RxFill ring in EDMA HW
 */
static void edma_cfg_rx_fill_ring_configure(struct edma_gbl_ctx *egc,
					struct edma_rxfill_ring *rxfill_ring)
{
	uint32_t ring_sz;

	edma_reg_write(EDMA_REG_RXFILL_BA(rxfill_ring->ring_id),
			(uint32_t)(rxfill_ring->dma & EDMA_RING_DMA_MASK));

	ring_sz = rxfill_ring->count & EDMA_RXFILL_RING_SIZE_MASK;
	edma_reg_write(EDMA_REG_RXFILL_RING_SIZE(rxfill_ring->ring_id), ring_sz);

	/*
	 * Alloc Rx buffers
	 */
	edma_rx_alloc_buffer(rxfill_ring, rxfill_ring->count - 1);
}

/*
 * edma_cfg_rx_ring_to_qid_mapping()
 *	Configure rx ring to PPE queue id mapping
 */
static void edma_cfg_rx_ring_to_qid_mapping(struct edma_gbl_ctx *egc)
{
	uint32_t desc_index, i;

	/*
	 * Set PPE QID to EDMA Rx ring mapping.
	 * Each entry can hold mapping for 4 PPE queues and
	 * entry size is 4 bytes.
	 */
	desc_index = (egc->rxdesc_ring_start & EDMA_RX_RING_ID_MASK);

	for (i = EDMA_PORT_QUEUE_START;
		i <= EDMA_PORT_QUEUE_END;
			i += EDMA_QID2RID_NUM_PER_REG) {
		uint32_t reg_index, data;
		reg_index = i/EDMA_QID2RID_NUM_PER_REG;
		data = EDMA_RX_RING_ID_QUEUE0_SET(desc_index) |
			EDMA_RX_RING_ID_QUEUE1_SET(desc_index + 1) |
			EDMA_RX_RING_ID_QUEUE2_SET(desc_index + 2) |
			EDMA_RX_RING_ID_QUEUE3_SET(desc_index + 3);

		edma_reg_write(EDMA_QID2RID_TABLE_MEM(reg_index), data);

		desc_index += EDMA_QID2RID_NUM_PER_REG;
		edma_debug("Configure QID2RID(%d) reg:0x%x to 0x%x\n",
				i, EDMA_QID2RID_TABLE_MEM(reg_index), data);
	}
}

/*
 * edma_cfg_rx_rings_to_rx_fill_mapping()
 *	Configure Rx rings to Rx fill mapping
 */
static void edma_cfg_rx_rings_to_rx_fill_mapping(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	/*
	 * Set RXDESC2FILL_MAP_xx reg.
	 * 3 registers hold the Rxfill mapping for total 24 Rxdesc rings.
	 * 3 bits holds the rx fill ring mapping for each of the
	 * rx descriptor ring.
	 */
	edma_reg_write(EDMA_REG_RXDESC2FILL_MAP_0, 0);
	edma_reg_write(EDMA_REG_RXDESC2FILL_MAP_1, 0);
	edma_reg_write(EDMA_REG_RXDESC2FILL_MAP_2, 0);

	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		uint32_t data, reg, ring_id;
		struct edma_rxdesc_ring *rxdesc_ring = &egc->rxdesc_rings[i];

		ring_id = rxdesc_ring->ring_id;
		if ((ring_id >= 0) && (ring_id <= 9)) {
			reg = EDMA_REG_RXDESC2FILL_MAP_0;
		} else if ((ring_id >= 10) && (ring_id <= 19)) {
			reg = EDMA_REG_RXDESC2FILL_MAP_1;
		} else {
			reg = EDMA_REG_RXDESC2FILL_MAP_2;
		}

		edma_debug("Configure RXDESC:%u to use RXFILL:%u\n",
						ring_id,
						rxdesc_ring->rxfill->ring_id);

		/*
		 * Set the Rx fill ring number in the
		 * mapping register.
		 */
		data = edma_reg_read(reg);
		data |= (rxdesc_ring->rxfill->ring_id &
				EDMA_RXDESC2FILL_MAP_RXDESC_MASK) <<
				((ring_id % 10) * 3);
		edma_reg_write(reg, data);
	}

	edma_debug("EDMA_REG_RXDESC2FILL_MAP_0: 0x%x\n", edma_reg_read(EDMA_REG_RXDESC2FILL_MAP_0));
	edma_debug("EDMA_REG_RXDESC2FILL_MAP_1: 0x%x\n", edma_reg_read(EDMA_REG_RXDESC2FILL_MAP_1));
	edma_debug("EDMA_REG_RXDESC2FILL_MAP_2: 0x%x\n", edma_reg_read(EDMA_REG_RXDESC2FILL_MAP_2));
}

/*
 * edma_cfg_rx_rings_enable()
 *	API to enable Rx and Rxfill rings
 */
void edma_cfg_rx_rings_enable(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	/*
	 * Enable Rx rings
	 */
	for (i = egc->rxdesc_ring_start; i < egc->rxdesc_ring_end; i++) {
		uint32_t data;

		data = edma_reg_read(EDMA_REG_RXDESC_CTRL(i));
		data |= EDMA_RXDESC_RX_EN;
		edma_reg_write(EDMA_REG_RXDESC_CTRL(i), data);
	}

	for (i = egc->rxfill_ring_start; i < egc->rxfill_ring_end; i++) {
		uint32_t data;

		data = edma_reg_read(EDMA_REG_RXFILL_RING_EN(i));
		data |= EDMA_RXFILL_RING_EN;
		edma_reg_write(EDMA_REG_RXFILL_RING_EN(i), data);
	}
}

/*
 * edma_cfg_rx_rings_disable()
 *	API to disable Rx and Rxfill rings
 */
void edma_cfg_rx_rings_disable(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	/*
	 * Disable Rx rings
	 */
	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		uint32_t data;
		struct edma_rxdesc_ring *rxdesc_ring = NULL;

		rxdesc_ring = &egc->rxdesc_rings[i];
		data = edma_reg_read(EDMA_REG_RXDESC_CTRL(rxdesc_ring->ring_id));
		data &= ~EDMA_RXDESC_RX_EN;
		edma_reg_write(EDMA_REG_RXDESC_CTRL(rxdesc_ring->ring_id), data);
	}

	/*
	 * Disable RxFill Rings
	 */
	for (i = 0; i < egc->num_rxfill_rings; i++) {
		uint32_t data;
		struct edma_rxfill_ring *rxfill_ring = NULL;

		rxfill_ring = &egc->rxfill_rings[i];
		data = edma_reg_read(EDMA_REG_RXFILL_RING_EN(rxfill_ring->ring_id));
		data &= ~EDMA_RXFILL_RING_EN;
		edma_reg_write(EDMA_REG_RXFILL_RING_EN(rxfill_ring->ring_id), data);
	}
}

/*
 * edma_cfg_rx_mapping()
 *	API to setup RX ring mapping
 */
void edma_cfg_rx_mapping(struct edma_gbl_ctx *egc)
{
	edma_cfg_rx_ring_to_qid_mapping(egc);
	edma_cfg_rx_rings_to_rx_fill_mapping(egc);
}

/*
 * edma_cfg_rx_rings_setup()
 *	Allocate/setup resources for EDMA rings
 */
static int edma_cfg_rx_rings_setup(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	/*
	 * Allocate Rx fill ring descriptors
	 */
	for (i = 0; i < egc->num_rxfill_rings; i++) {
		int32_t ret;
		struct edma_rxfill_ring *rxfill_ring = NULL;

		rxfill_ring = &egc->rxfill_rings[i];
		rxfill_ring->count = EDMA_RX_RING_SIZE;
		rxfill_ring->ring_id = egc->rxfill_ring_start + i;

		ret = edma_cfg_rx_fill_ring_setup(rxfill_ring);
		if (ret != 0) {
			edma_err("Error in setting up %d rxfill ring. ret: %d",
					 rxfill_ring->ring_id, ret);
			while (--i >= 0) {
				edma_cfg_rx_fill_ring_cleanup(egc,
					&egc->rxfill_rings[i]);
			}

			return -ENOMEM;
		}
	}

	/*
	 * Allocate RxDesc ring descriptors
	 */
	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		uint32_t index;
		int32_t ret;
		struct edma_rxdesc_ring *rxdesc_ring = NULL;

		rxdesc_ring = &egc->rxdesc_rings[i];
		rxdesc_ring->count = EDMA_RX_RING_SIZE;
		rxdesc_ring->ring_id = egc->rxdesc_ring_start + i;

		/*
		 * Create a mapping between RX Desc ring and Rx fill ring.
		 * Number of fill rings are lesser than the descriptor rings
		 * Share the fill rings across descriptor rings.
		 */
		index = egc->rxfill_ring_start + (i % egc->num_rxfill_rings);
		rxdesc_ring->rxfill = &egc->rxfill_rings[index - egc->rxfill_ring_start];

		ret = edma_cfg_rx_desc_ring_setup(rxdesc_ring);
		if (ret != 0) {
			edma_err("Error in setting up %d rxdesc ring. ret: %d",
					 rxdesc_ring->ring_id, ret);
			while (--i >= 0) {
				edma_cfg_rx_desc_ring_cleanup(egc, &egc->rxdesc_rings[i]);
			}

			goto rxdesc_mem_alloc_fail;
		}
	}

	return 0;

rxdesc_mem_alloc_fail:
	for (i = 0; i < egc->num_rxfill_rings; i++) {
		edma_cfg_rx_fill_ring_cleanup(egc, &egc->rxfill_rings[i]);
	}

	return -ENOMEM;
}

/*
 * edma_cfg_rx_rings_alloc()
 *	Allocate EDMA Rx rings
 */
int32_t edma_cfg_rx_rings_alloc(struct edma_gbl_ctx *egc)
{
	egc->rxfill_rings = kzalloc((sizeof(struct edma_rxfill_ring) *
				egc->num_rxfill_rings), GFP_KERNEL);
	if (!egc->rxfill_rings) {
		edma_err("Error in allocating rxfill ring\n");
		return -ENOMEM;
	}

	egc->rxdesc_rings = kzalloc((sizeof(struct edma_rxdesc_ring) *
				egc->num_rxdesc_rings), GFP_KERNEL);
	if (!egc->rxdesc_rings) {
		edma_err("Error in allocating rxdesc ring\n");
		goto rxdesc_ring_alloc_fail;
	}

	edma_info("RxDesc:%u (%u-%u) RxFill:%u (%u-%u)\n",
		egc->num_rxdesc_rings, egc->rxdesc_ring_start,
		(egc->rxdesc_ring_start + egc->num_rxdesc_rings - 1),
		egc->num_rxfill_rings, egc->rxfill_ring_start,
		(egc->rxfill_ring_start + egc->num_rxfill_rings - 1));

	if (edma_cfg_rx_rings_setup(egc)) {
		edma_err("Error in setting up rx rings\n");
		goto rx_rings_setup_fail;
	}

	return 0;

rx_rings_setup_fail:
	kfree(egc->rxdesc_rings);
	egc->rxdesc_rings = NULL;
rxdesc_ring_alloc_fail:
	kfree(egc->rxfill_rings);
	egc->rxfill_rings = NULL;
	return -ENOMEM;
}

/*
 * edma_cfg_rx_rings_cleanup()
 *	Cleanup EDMA rings
 */
void edma_cfg_rx_rings_cleanup(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	/*
	 * Free Rx fill ring descriptors
	 */
	for (i = 0; i < egc->num_rxfill_rings; i++) {
		edma_cfg_rx_fill_ring_cleanup(egc, &egc->rxfill_rings[i]);
	}

	/*
	 * Free Rx completion ring descriptors
	 */
	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		edma_cfg_rx_desc_ring_cleanup(egc, &egc->rxdesc_rings[i]);
	}

	kfree(egc->rxfill_rings);
	kfree(egc->rxdesc_rings);
	egc->rxfill_rings = NULL;
	egc->rxdesc_rings = NULL;
}

/*
 * edma_cfg_rx_rings()
 *	Configure EDMA rings
 */
void edma_cfg_rx_rings(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	/*
	 * Configure RXFILL rings
	 */
	for (i = 0; i < egc->num_rxfill_rings; i++) {
		edma_cfg_rx_fill_ring_configure(egc, &egc->rxfill_rings[i]);
	}

	/*
	 * Configure RXDESC ring
	 */
	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		edma_cfg_rx_desc_ring_configure(egc, &egc->rxdesc_rings[i]);
	}
}

/*
 * edma_cfg_rx_napi_disable()
 *	Disable RX NAPI
 */
void edma_cfg_rx_napi_disable(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		struct edma_rxdesc_ring *rxdesc_ring;

		rxdesc_ring = &egc->rxdesc_rings[i];

		if (!rxdesc_ring->napi_added) {
			continue;
		}

		napi_disable(&rxdesc_ring->napi);
	}
}

/*
 * edma_cfg_rx_napi_enable()
 *	Enable RX NAPI
 */
void edma_cfg_rx_napi_enable(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		struct edma_rxdesc_ring *rxdesc_ring;

		rxdesc_ring = &egc->rxdesc_rings[i];

		if (!rxdesc_ring->napi_added) {
			continue;
		}

		napi_enable(&rxdesc_ring->napi);
	}
}

/*
 * edma_cfg_rx_napi_delete()
 *	Delete RX NAPI
 */
void edma_cfg_rx_napi_delete(struct edma_gbl_ctx *egc)
{
	uint32_t i;

	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		struct edma_rxdesc_ring *rxdesc_ring;

		rxdesc_ring = &egc->rxdesc_rings[i];

		if (!rxdesc_ring->napi_added) {
			continue;
		}

		netif_napi_del(&rxdesc_ring->napi);
		rxdesc_ring->napi_added = false;
	}
}

/*
 * edma_cfg_rx_napi_add()
 *	RX NAPI add API
 */
void edma_cfg_rx_napi_add(struct edma_gbl_ctx *egc, struct net_device *netdev)
{
	uint32_t i;

	for (i = 0; i < egc->num_rxdesc_rings; i++) {
		struct edma_rxdesc_ring *rxdesc_ring = &egc->rxdesc_rings[i];
		netif_napi_add(netdev, &rxdesc_ring->napi,
				edma_rx_napi_poll, EDMA_RX_NAPI_WORK);
		rxdesc_ring->napi_added = true;
	}
}
