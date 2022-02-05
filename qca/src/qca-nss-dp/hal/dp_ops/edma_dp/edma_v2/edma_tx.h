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

#ifndef __EDMA_TX_H__
#define __EDMA_TX_H__

#define EDMA_GET_DESC(R, i, type)	(&(((type *)((R)->desc))[(i)]))
#define EDMA_GET_PDESC(R, i, type)	(&(((type *)((R)->pdesc))[(i)]))
#define EDMA_GET_SDESC(R, i, type)	(&(((type *)((R)->sdesc))[(i)]))
#define EDMA_TXCMPL_DESC(R, i)		EDMA_GET_DESC(R, i, struct edma_txcmpl_desc)
#define EDMA_TXDESC_PRI_DESC(R, i)	EDMA_GET_PDESC(R, i, struct edma_pri_txdesc)
#define EDMA_TXDESC_SEC_DESC(R, i)	EDMA_GET_SDESC(R, i, struct edma_sec_txdesc)

#define EDMA_MAX_TXCMPL_RINGS		32	/* Max TxCmpl rings */
#define EDMA_MAX_TXDESC_RINGS		32	/* Max TxDesc rings */
#define EDMA_TXCMPL_RING_PER_CORE_MAX	6
#define EDMA_TX_MAX_PRIORITY_LEVEL	1

#define EDMA_TX_RING_SIZE		256
#define EDMA_TX_RING_SIZE_MASK		(EDMA_TX_RING_SIZE - 1)

#define EDMA_TX_RING_PER_CORE_MAX	(EDMA_TX_MAX_PRIORITY_LEVEL * EDMA_MAX_GMACS)

#define EDMA_DST_PORT_TYPE		2
#define EDMA_DST_PORT_TYPE_SHIFT	28
#define EDMA_DST_PORT_TYPE_MASK		(0xf << EDMA_DST_PORT_TYPE_SHIFT)
#define EDMA_DST_PORT_ID_SHIFT		16
#define EDMA_DST_PORT_ID_MASK		(0xfff << EDMA_DST_PORT_ID_SHIFT)

#define EDMA_DST_PORT_TYPE_SET(x)	(((x) << EDMA_DST_PORT_TYPE_SHIFT) & EDMA_DST_PORT_TYPE_MASK)
#define EDMA_DST_PORT_ID_SET(x)		(((x) << EDMA_DST_PORT_ID_SHIFT) & EDMA_DST_PORT_ID_MASK)
#define EDMA_DST_INFO_SET(desc, x)	((desc)->word4 |= (EDMA_DST_PORT_TYPE_SET(EDMA_DST_PORT_TYPE) | EDMA_DST_PORT_ID_SET(x)))
#define EDMA_TXDESC_TSO_ENABLE_SHIFT		24
#define EDMA_TXDESC_TSO_ENABLE_MASK		0x1000000
#define EDMA_TXDESC_TSO_ENABLE_SET(desc, x)	((desc)->word5 |= (((x) << EDMA_TXDESC_TSO_ENABLE_SHIFT) & EDMA_TXDESC_TSO_ENABLE_MASK))
#define EDMA_TXDESC_MSS_SHIFT			16
#define EDMA_TXDESC_MSS_MASK			0xFFFF0000
#define EDMA_TXDESC_MSS_SET(desc, x)		((desc)->word6 |= (((x) << EDMA_TXDESC_MSS_SHIFT) & EDMA_TXDESC_MSS_MASK))
#define EDMA_TXDESC_MORE_BIT_MASK	0x40000000
#define EDMA_TXDESC_MORE_BIT_SHIFT	30
#define EDMA_TXDESC_MORE_BIT_SET(desc, x)	((desc)->word1 |= (((x) << EDMA_TXDESC_MORE_BIT_SHIFT) & EDMA_TXDESC_MORE_BIT_MASK))

#define EDMA_TXDESC_ADV_OFFSET_BIT	31
#define EDMA_TXDESC_ADV_OFFLOAD_SET(desc)	((desc)->word5 |= (1 << EDMA_TXDESC_ADV_OFFSET_BIT))
#define EDMA_TXDESC_IP_CSUM_BIT		25
#define EDMA_TXDESC_IP_CSUM_SET(desc)		((desc)->word5 |= (1 << EDMA_TXDESC_IP_CSUM_BIT))

#define EDMA_TXDESC_L4_CSUM_SET_SHIFT	26
#define EDMA_TXDESC_L4_CSUM_SET_MASK	(0x3 << EDMA_TXDESC_L4_CSUM_SET_SHIFT)
#define EDMA_TXDESC_L4_CSUM_SET(desc)	((desc)->word5 |= ((1 << EDMA_TXDESC_L4_CSUM_SET_SHIFT) & EDMA_TXDESC_L4_CSUM_SET_MASK))

#define EDMA_TXDESC_DATA_LEN_SET(desc, x)	((desc)->word5 |= ((x) & 0x1ffff))
#define EDMA_TXDESC_SERVICE_CODE_SHIFT	16
#define EDMA_TXDESC_SERVICE_CODE_MASK	(0x1FF << EDMA_TXDESC_SERVICE_CODE_SHIFT)
#define EDMA_TXDESC_SERVICE_CODE_SET(desc, x)	((desc)->word1 |= (((x) << EDMA_TXDESC_SERVICE_CODE_SHIFT) & EDMA_TXDESC_SERVICE_CODE_MASK))
#ifdef __LP64__
#define EDMA_TXDESC_OPAQUE_GET(desc)		(((uint64_t)(desc)->word3 << 32) | (desc)->word2)
#define EDMA_TXCMPL_OPAQUE_GET(desc)		(((uint64_t)(desc)->word1 << 32) | (desc)->word0)
#define EDMA_TXDESC_OPAQUE_LO_SET(desc, ptr)	((desc)->word2 = (uint32_t)(uintptr_t)ptr)
#define EDMA_TXDESC_OPAQUE_HI_SET(desc, ptr)	((desc)->word3 = (uint32_t)((uint64_t)ptr >> 32))
#define EDMA_TXDESC_OPAQUE_SET(desc, ptr)	do {	\
	EDMA_TXDESC_OPAQUE_LO_SET(desc, ptr);		\
	EDMA_TXDESC_OPAQUE_HI_SET(desc, ptr);		\
} while (0)
#else
#define EDMA_TXCMPL_OPAQUE_GET(desc)		((desc)->word0)
#define EDMA_TXDESC_OPAQUE_GET(desc)		((desc)->word2)
#define EDMA_TXDESC_OPAQUE_LO_SET(desc, ptr)	((desc)->word2 = (uint32_t)(uintptr_t)ptr)
#define EDMA_TXDESC_OPAQUE_SET(desc, ptr)	EDMA_TXDESC_OPAQUE_LO_SET(desc, ptr)
#endif
#define EDMA_TXCMPL_MORE_BIT_MASK		0x40000000
#define EDMA_TXCMPL_MORE_BIT_GET(desc)		((desc)->word2 & EDMA_TXCMPL_MORE_BIT_MASK)

/*
 * edma_tx
 *	List of return values of the TX API.
 */
enum edma_tx {
	EDMA_TX_OK = 0,			/* Tx success */
	EDMA_TX_FAIL_NO_DESC = 1,	/* Not enough descriptors */
	EDMA_TX_FAIL = 2,		/* Tx failure */
};

/*
 * edma_tx_stats
 *	EDMA TX per cpu stats
 */
struct edma_tx_stats {
	uint64_t tx_pkts;
	uint64_t tx_bytes;
	uint64_t tx_drops;
	uint64_t tx_no_desc_avail;
	uint64_t tx_nr_frag_pkts;
	uint64_t tx_fraglist_pkts;
	uint64_t tx_fraglist_with_nr_frags_pkts;
	uint64_t tx_tso_pkts;
	struct u64_stats_sync syncp;
};

/*
 * edma_pri_tx_desc
 *	EDMA primary TX descriptor.
 */
struct edma_pri_txdesc {
	uint32_t word0;		/* Low 32-bit of buffer address */
	uint32_t word1;		/* Buffer recycling, PTP tag flag, PRI valid flag */
	uint32_t word2;		/* Low 32-bit of opaque value */
	uint32_t word3;		/* High 32-bit of opaque value */
	uint32_t word4;		/* Source/Destination port info */
	uint32_t word5;		/* VLAN offload, csum_mode, ip_csum_en, tso_en, data length */
	uint32_t word6;		/* MSS/hash_value/PTP tag, data offset */
	uint32_t word7;		/* L4/L3 offset, PROT type, L2 type, CVLAN/SVLAN tag, service code */
};

/*
 * edma_sec_txdesc
 *	EDMA secondary TX descriptor.
 */
struct edma_sec_txdesc {
	uint32_t word0;		/* Reserved */
	uint32_t word1;		/* Custom csum offset, payload offset, TTL/NAT action */
	uint32_t word2;		/* NAPT translated port, DSCP value, TTL value */
	uint32_t word3;		/* Flow index value and valid flag */
	uint32_t word4;		/* Reserved */
	uint32_t word5;		/* Reserved */
	uint32_t word6;		/* CVLAN/SVLAN command */
	uint32_t word7;		/* CVLAN/SVLAN tag value */
};

/*
 * edma_txcmpl_desc
 *	EDMA TX complete descriptor.
 */
struct edma_txcmpl_desc {
	uint32_t word0;		/* Low 32-bit opaque value */
	uint32_t word1;		/* High 32-bit opaque value */
	uint32_t word2;		/* More fragment, transmit ring id, pool id */
	uint32_t word3;		/* Error indications */
};

/*
 * edma_txdesc_ring
 *	EDMA TX descriptor ring.
 */
struct edma_txdesc_ring {
	uint32_t prod_idx;		/* Producer index */
	uint32_t avail_desc;		/* Number of available descriptor to process */
	uint32_t id;			/* TXDESC ring number */
	struct edma_pri_txdesc *pdesc;	/* Primary descriptor ring virtual address */
	dma_addr_t pdma;		/* Primary descriptor ring physical address */
	struct edma_sec_txdesc *sdesc;	/* Secondary descriptor ring virtual address */
	dma_addr_t sdma;		/* Secondary descriptor ring physical address */
	uint32_t count;			/* Number of descriptors */
};

/*
 * edma_txcmpl_ring
 *	EDMA TX complete ring.
 */
struct edma_txcmpl_ring {
	struct napi_struct napi;	/* NAPI structure */
	uint32_t cons_idx;		/* Consumer index */
	uint32_t avail_pkt;		/* Number of available packets to process */
	struct edma_txcmpl_desc *desc;	/* Descriptor ring virtual address */
	uint32_t id;			/* TXCMPL ring number */
	dma_addr_t dma;			/* Descriptor ring physical address */
	uint32_t count;			/* Number of descriptors in the ring */
	bool napi_added;		/* Flag to indicate NAPI add status */
};

enum edma_tx edma_tx_ring_xmit(struct net_device *netdev, struct sk_buff *skb,
				struct edma_txdesc_ring *txdesc_ring,
				struct edma_tx_stats *stats);
uint32_t edma_tx_complete(uint32_t work_to_do,
				struct edma_txcmpl_ring *txcmpl_ring);
irqreturn_t edma_tx_handle_irq(int irq, void *ctx);
int edma_tx_napi_poll(struct napi_struct *napi, int budget);

#endif	/* __EDMA_TX_H__ */
