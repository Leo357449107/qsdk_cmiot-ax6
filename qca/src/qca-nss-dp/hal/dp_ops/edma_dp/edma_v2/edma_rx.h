/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#ifndef __EDMA_RX_H__
#define __EDMA_RX_H__

#define EDMA_RXFILL_RING_PER_CORE_MAX	1
#define EDMA_RXDESC_RING_PER_CORE_MAX	1

#define EDMA_RX_MAX_PROCESS		32	/* Max Rx processing without
						   replenishing RxFill ring */
#define EDMA_RX_SKB_HEADROOM		128

#define EDMA_GET_DESC(R, i, type)	(&(((type *)((R)->desc))[(i)]))
#define EDMA_GET_PDESC(R, i, type)	(&(((type *)((R)->pdesc))[(i)]))
#define EDMA_GET_SDESC(R, i, type)	(&(((type *)((R)->sdesc))[(i)]))
#define EDMA_RXFILL_DESC(R, i)		EDMA_GET_DESC(R, i, struct edma_rxfill_desc)
#define EDMA_RXDESC_PRI_DESC(R, i)	EDMA_GET_PDESC(R, i, struct edma_rxdesc_desc)
#define EDMA_RXDESC_SEC_DESC(R, i)	EDMA_GET_SDESC(R, i, struct edma_rxdesc_sec_desc)

#define EDMA_RX_RING_SIZE		256
#define EDMA_RX_RING_SIZE_MASK		(EDMA_RX_RING_SIZE - 1)
#define EDMA_RX_RING_ID_MASK		0x1F
#define EDMA_MAX_RXDESC_RINGS		24	/* Max RxDesc rings */
#define EDMA_MAX_RXFILL_RINGS		8	/* Max RxFill rings */
#define EDMA_RX_MAX_PRIORITY_LEVEL	1

#define EDMA_RXDESC_BUFFER_ADDR_GET(desc)	((uint32_t)((desc)->word0))
#define EDMA_RXDESC_OPAQUE_GET(desc)		((uintptr_t)((uint64_t)((desc)->word2) | \
						((uint64_t)((desc)->word3) << 0x20)))
#define EDMA_RXDESC_SRCINFO_TYPE_PORTID		0x2000
#define EDMA_RXDESC_SRCINFO_TYPE_SHIFT		8
#define EDMA_RXDESC_SRCINFO_TYPE_MASK		0xF000
#define EDMA_RXDESC_L3CSUM_STATUS_MASK		0x2000
#define EDMA_RXDESC_L4CSUM_STATUS_MASK		0x1000
#define EDMA_RXDESC_PORTNUM_BITS		0x0FFF

#define EDMA_RXDESC_PACKET_LEN_MASK		0x3FFFF
#define EDMA_RXDESC_PACKET_LEN_GET(desc)	(((desc)->word5) & \
						EDMA_RXDESC_PACKET_LEN_MASK)
#define EDMA_RXDESC_SRC_INFO_GET(desc)		(((desc)->word4) & 0xFFFF)
#define EDMA_RXDESC_L3CSUM_STATUS_GET(desc)	(((desc)->word6) & \
						EDMA_RXDESC_L3CSUM_STATUS_MASK)
#define EDMA_RXDESC_L4CSUM_STATUS_GET(desc)	(((desc)->word6) & \
						EDMA_RXDESC_L4CSUM_STATUS_MASK)
#define EDMA_RXDESC_SERVICE_CODE_GET(desc)	(((desc)->word7) & 0x1FF)

#define EDMA_RXFILL_BUF_SIZE_MASK		0xFFFF
#define EDMA_RXFILL_BUF_SIZE_SHIFT		16
#define EDMA_RXFILL_OPAQUE_LO_SET(desc, ptr)	(((desc)->word2) = (uint32_t)(uintptr_t)(ptr))
#define EDMA_RXFILL_OPAQUE_HI_SET(desc, ptr)	(((desc)->word3) = (uint32_t)((uint64_t)(ptr) >> 0x20))
#define EDMA_RXFILL_OPAQUE_GET(desc)		((uintptr_t)((uint64_t)((desc)->word2) | \
						((uint64_t)((desc)->word3) << 0x20)))
#define EDMA_RXFILL_PACKET_LEN_SET(desc, len)	(((desc)->word1) = (uint32_t)\
						((((uint32_t)len) << \
						EDMA_RXFILL_BUF_SIZE_SHIFT) & 0xFFFF0000))
#define EDMA_RXFILL_BUFFER_ADDR_SET(desc, addr)	(((desc)->word0) = (uint32_t)(addr))

/*
 * edma_rx_stats
 *	EDMA RX per cpu stats
 */
struct edma_rx_stats {
	uint64_t rx_pkts;
	uint64_t rx_bytes;
	uint64_t rx_drops;
	struct u64_stats_sync syncp;
};

/*
 * Rx descriptor
 */
struct edma_rxdesc_desc {
	uint32_t word0;		/* Contains buffer address */
	uint32_t word1;		/* Contains more bit, priority bit, service code */
	uint32_t word2;		/* Contains opaque */
	uint32_t word3;		/* Contains opaque high bits */
	uint32_t word4;		/* Contains destination and source information */
	uint32_t word5;		/* Contains WiFi QoS, data length */
	uint32_t word6;		/* Contains hash value, check sum status */
	uint32_t word7;		/* Contains DSCP, packet offsets */
};

/*
 * Rx secondary descriptor
 */
struct edma_rxdesc_sec_desc {
	uint32_t word0;		/* Contains timestamp */
	uint32_t word1;		/* Contains secondary checksum status */
	uint32_t word2;		/* Contains QoS tag */
	uint32_t word3;		/* Contains flow index details */
	uint32_t word4;		/* Contains secondary packet offsets */
	uint32_t word5;		/* Contains multicast bit, checksum */
	uint32_t word6;		/* Contains SVLAN, CVLAN */
	uint32_t word7;		/* Contains secondary SVLAN, CVLAN */
};

/*
 * RxFill descriptor
 */
struct edma_rxfill_desc {
	uint32_t word0;		/* Contains buffer address */
	uint32_t word1;		/* Contains buffer size */
	uint32_t word2;		/* Contains opaque */
	uint32_t word3;		/* Contains opaque high bits */
};

/*
 * RxFill ring
 */
struct edma_rxfill_ring {
	uint32_t ring_id;		/* RXFILL ring number */
	uint32_t count;			/* number of descriptors in the ring */
	uint32_t prod_idx;		/* Ring producer index */
	struct edma_rxfill_desc *desc;	/* descriptor ring virtual address */
	dma_addr_t dma;			/* descriptor ring physical address */
};

/*
 * RxDesc ring
 */
struct edma_rxdesc_ring {
	struct napi_struct napi;	/* Napi structure */
	uint32_t ring_id;		/* RXDESC ring number */
	uint32_t count;			/* number of descriptors in the ring */
	uint32_t work_leftover;		/* Leftover descriptors to be processed */
	uint32_t cons_idx;		/* Ring consumer index */
	struct edma_rxdesc_desc *pdesc;
					/* Primary descriptor ring virtual address */
	struct edma_rxdesc_sec_desc *sdesc;
					/* Secondary descriptor ring virtual address */
	struct edma_rxfill_ring *rxfill;
					/* RXFILL ring used */
	bool napi_added;		/* Flag to indicate NAPI add status */
	dma_addr_t pdma;		/* Primary descriptor ring physical address */
	dma_addr_t sdma;		/* Secondary descriptor ring physical address */
};

irqreturn_t edma_rx_handle_irq(int irq, void *ctx);
int edma_rx_alloc_buffer(struct edma_rxfill_ring *rxfill_ring, int alloc_count);
int edma_rx_napi_poll(struct napi_struct *napi, int budget);

#endif	/* __EDMA_RX_H__ */
