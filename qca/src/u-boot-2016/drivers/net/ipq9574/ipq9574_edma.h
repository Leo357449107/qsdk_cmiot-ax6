/*
 **************************************************************************
 * Copyright (c) 2016-2019, 2021, The Linux Foundation. All rights reserved.
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
 **************************************************************************
*/
#ifndef __IPQ9574_EDMA__
#define __IPQ9574_EDMA__

#define IPQ9574_NSS_DP_START_PHY_PORT   1
#define IPQ9574_NSS_DP_MAX_PHY_PORTS    6

#define IPQ9574_EDMA_DEVICE_NODE_NAME	"edma"

/* Number of descriptors in each ring is defined with below macro */
#define EDMA_RING_SIZE			128

/* Number of byte in a descriptor is defined with below macros for each of
 * the rings respectively */
#define IPQ9574_EDMA_TXDESC_DESC_SIZE	(sizeof(struct ipq9574_edma_txdesc_desc))
#define IPQ9574_EDMA_TXCMPL_DESC_SIZE	(sizeof(struct ipq9574_edma_txcmpl_desc))
#define IPQ9574_EDMA_RXDESC_DESC_SIZE	(sizeof(struct ipq9574_edma_rxdesc_desc))
#define IPQ9574_EDMA_RXFILL_DESC_SIZE	(sizeof(struct ipq9574_edma_rxfill_desc))
#define IPQ9574_EDMA_RX_SEC_DESC_SIZE	(sizeof(struct ipq9574_edma_rx_sec_desc))
#define IPQ9574_EDMA_TX_SEC_DESC_SIZE	(sizeof(struct ipq9574_edma_tx_sec_desc))

#define IPQ9574_EDMA_START_GMACS	IPQ9574_NSS_DP_START_PHY_PORT
#define IPQ9574_EDMA_MAX_GMACS		IPQ9574_NSS_DP_MAX_PHY_PORTS

#define IPQ9574_EDMA_TX_BUFF_SIZE	1572
#define IPQ9574_EDMA_RX_BUFF_SIZE	2048

/* Max number of rings of each type is defined with below macro */
#define IPQ9574_EDMA_MAX_TXCMPL_RINGS	32	/* Max TxCmpl rings */
#define IPQ9574_EDMA_MAX_RXDESC_RINGS	24	/* Max RxDesc rings */
#define IPQ9574_EDMA_MAX_RXFILL_RINGS	8	/* Max RxFill rings */
#define IPQ9574_EDMA_MAX_TXDESC_RINGS	32	/* Max TxDesc rings */

#define IPQ9574_EDMA_GET_DESC(R, i, type) (&(((type *)((R)->desc))[i]))
#define IPQ9574_EDMA_RXFILL_DESC(R, i) 	IPQ9574_EDMA_GET_DESC(R, i, struct ipq9574_edma_rxfill_desc)
#define IPQ9574_EDMA_RXDESC_DESC(R, i) 	IPQ9574_EDMA_GET_DESC(R, i, struct ipq9574_edma_rxdesc_desc)
#define IPQ9574_EDMA_TXDESC_DESC(R, i) 	IPQ9574_EDMA_GET_DESC(R, i, struct ipq9574_edma_txdesc_desc)
#define IPQ9574_EDMA_TXCMPL_DESC(R, i) 	IPQ9574_EDMA_GET_DESC(R, i, struct ipq9574_edma_txcmpl_desc)
#define IPQ9574_EDMA_RXPH_SRC_INFO_TYPE_GET(rxph)	(((rxph & 0xffff) >> 8) & 0xf0)

#define IPQ9574_EDMA_DEV		1
#define IPQ9574_EDMA_TX_QUEUE		1
#define IPQ9574_EDMA_RX_QUEUE		1

/* Only 1 ring of each type will be used in U-Boot which is defined with
 * below macros */
#define IPQ9574_EDMA_TX_DESC_RING_START	23
#define IPQ9574_EDMA_TX_DESC_RING_NOS	1
#define IPQ9574_EDMA_TX_DESC_RING_SIZE	\
(IPQ9574_EDMA_TX_DESC_RING_START + IPQ9574_EDMA_TX_DESC_RING_NOS)

#define IPQ9574_EDMA_SEC_TX_DESC_RING_START	31
#define IPQ9574_EDMA_SEC_TX_DESC_RING_NOS	1
#define IPQ9574_EDMA_SEC_TX_DESC_RING_SIZE	\
(IPQ9574_EDMA_SEC_TX_DESC_RING_START + IPQ9574_EDMA_SEC_TX_DESC_RING_NOS)

#define IPQ9574_EDMA_TX_CMPL_RING_START	31
#define IPQ9574_EDMA_TX_CMPL_RING_NOS	1
#define IPQ9574_EDMA_TX_CMPL_RING_SIZE	\
(IPQ9574_EDMA_TX_CMPL_RING_START + IPQ9574_EDMA_TX_CMPL_RING_NOS)

#define IPQ9574_EDMA_RX_DESC_RING_START	15
#define IPQ9574_EDMA_RX_DESC_RING_NOS	1
#define IPQ9574_EDMA_RX_DESC_RING_SIZE	\
(IPQ9574_EDMA_RX_DESC_RING_START + IPQ9574_EDMA_RX_DESC_RING_NOS)

#define IPQ9574_EDMA_SEC_RX_DESC_RING_START	15
#define IPQ9574_EDMA_SEC_RX_DESC_RING_NOS	1
#define IPQ9574_EDMA_SEC_RX_DESC_RING_SIZE	\
(IPQ9574_EDMA_SEC_RX_DESC_RING_START + IPQ9574_EDMA_SEC_RX_DESC_RING_NOS)

#define IPQ9574_EDMA_RX_FILL_RING_START	7
#define IPQ9574_EDMA_RX_FILL_RING_NOS	1
#define IPQ9574_EDMA_RX_FILL_RING_SIZE	\
(IPQ9574_EDMA_RX_FILL_RING_START + IPQ9574_EDMA_RX_FILL_RING_NOS)

#define NETDEV_TX_BUSY	1

/*
 * RxDesc descriptor
 */
struct ipq9574_edma_rxdesc_desc {
	uint32_t rdes0;
		/* buffer_address_lo */
	uint32_t rdes1;
		/* valid toggle, more, int_pri, drop_prec, reserved x 3,
		 * tunnel_type, tunnel_term_ind, cpu_code_valid, known_ind,
		 * wifi_qos_flag, wifi_qos, buffer_address_hi */
	uint32_t rdes2;
		/* opaque_lo */
	uint32_t rdes3;
		/* opaque_hi */
	uint32_t rdes4;
		/* dst_info, src_info */
	uint32_t rdes5;
		/* dspcp, pool_id, data_lengh */
	uint32_t rdes6;
		/* hash_value, hash_flag, l3_csum_status, l4_csum_status,
		 * data_offset */
	uint32_t rdes7;
		/* l4_offset, l3_offset, pid, CVLAN flag, SVLAN flag, PPPOE flag
		 * service_code */
};

/*
 * RxFill descriptor
 */
struct ipq9574_edma_rxfill_desc {
	uint32_t rdes0;
		/* buffer_address_lo */
	uint32_t rdes1;
		/* buffer_size, reserved x 1, buffer_address_hi */
	uint32_t rdes2;
		/* opaque_lo */
	uint32_t rdes3;
		/* opaque_hu */
};

/*
 * TxDesc descriptor
 */
struct ipq9574_edma_txdesc_desc {
	uint32_t tdes0;
		/* buffer_address_lo */
	uint32_t tdes1;
		/* reserved x 1, more, int_pri, drop_prec, reserved x 4,
		 * buff_recycling, fake_mac_header,ptp_tag_flag, pri_valid,
		 * buffer_address_high_bits_tbi, buffer_address_hi */
	uint32_t tdes2;
		/* opaque_lo */
	uint32_t tdes3;
		/* opaque_hi */
	uint32_t tdes4;
		/* dst_info, src_info */
	uint32_t tdes5;
		/* adv_offload_en, vlan_offload_en, frm_fmt_indication_en,
		 * edit_offload_en, csum_mode, ip_csum_en, tso_en,  pool_id,
		 * data_lengh */
	uint32_t tdes6;
		/* mss/hash_value/pip_tag, hash_flag, reserved x 2,
		 * data_offset */
	uint32_t tdes7;
		/* l4_offset, l3_offset, reserved, prot_type, l2_type,
		 * CVLAN flag, SVLAN flag, PPPOE flag, service_code */
};

/*
 * TxCmpl descriptor
 */
struct ipq9574_edma_txcmpl_desc {
	uint32_t tdes0;
		/* buffer_address_lo */
	uint32_t tdes1;
		/* buffer_size, reserved x 1, buffer_address_hi */
	uint32_t tdes2;
		/* opaque_lo */
	uint32_t tdes3;
		/* opaque_hu */
};

/*
 * EDMA Rx Secondary Descriptor
 */
struct ipq9574_edma_rx_sec_desc {
	uint32_t rx_sec0;
	uint32_t rx_sec1;
	uint32_t rx_sec2;
	uint32_t rx_sec3;
	uint32_t rx_sec4;
	uint32_t rx_sec5;
	uint32_t rx_sec6;
	uint32_t rx_sec7;
};

/*
 * EDMA Tx Secondary Descriptor
 */
struct ipq9574_edma_tx_sec_desc {
	uint32_t tx_sec0;
	uint32_t tx_sec1;
	uint32_t rx_sec2;
	uint32_t rx_sec3;
	uint32_t rx_sec4;
	uint32_t rx_sec5;
	uint32_t rx_sec6;
	uint32_t rx_sec7;
};

/*
 * secondary Tx descriptor ring
 */
struct ipq9574_edma_sec_txdesc_ring {
	uint32_t id;			/* TXDESC ring number */
	void *desc;			/* descriptor ring virtual address */
	dma_addr_t dma;			/* descriptor ring physical address */
	uint16_t count;			/* number of descriptors */
};

/*
 * Tx descriptor ring
 */
struct ipq9574_edma_txdesc_ring {
	uint32_t id;			/* TXDESC ring number */
	void *desc;			/* descriptor ring virtual address */
	dma_addr_t dma;			/* descriptor ring physical address */
	uint16_t count;			/* number of descriptors */
};

/*
 * TxCmpl ring
 */
struct ipq9574_edma_txcmpl_ring {
	uint32_t id;			/* TXCMPL ring number */
	void *desc;			/* descriptor ring virtual address */
	dma_addr_t dma;			/* descriptor ring physical address */
	uint16_t count;			/* number of descriptors in the ring */
};

/*
 * RxFill ring
 */
struct ipq9574_edma_rxfill_ring {
	uint32_t id;			/* RXFILL ring number */
	void *desc;			/* descriptor ring virtual address */
	dma_addr_t dma;			/* descriptor ring physical address */
	uint16_t count;			/* number of descriptors in the ring */
};

/*
 * secondary RxDesc ring
 */
struct ipq9574_edma_sec_rxdesc_ring {
	uint32_t id;			/* RXDESC ring number */
	void *desc;			/* descriptor ring virtual address */
	dma_addr_t dma;			/* descriptor ring physical address */
	uint16_t count;			/* number of descriptors in the ring */
};

/*
 * RxDesc ring
 */
struct ipq9574_edma_rxdesc_ring {
	uint32_t id;			/* RXDESC ring number */
	struct ipq9574_edma_rxfill_ring *rxfill;	/* RXFILL ring used */
	void *desc;			/* descriptor ring virtual address */
	dma_addr_t dma;			/* descriptor ring physical address */
	uint16_t count;			/* number of descriptors in the ring */
};

enum ipq9574_edma_tx {
	EDMA_TX_OK = 0,		/* Tx success */
	EDMA_TX_DESC = 1,	/* Not enough descriptors */
	EDMA_TX_FAIL = 2,	/* Tx failure */
};


/* per core queue related information */
struct queue_per_cpu_info {
	u32 tx_mask; /* tx interrupt mask */
	u32 rx_mask; /* rx interrupt mask */
	u32 tx_status; /* tx interrupt status */
	u32 rx_status; /* rx interrupt status */
	u32 tx_start; /* tx queue start */
	u32 rx_start; /* rx queue start */
	struct ipq9574_edma_common_info *c_info; /* edma common info */
};

/* edma hw specific data */
struct ipq9574_edma_hw {
	unsigned long  __iomem *hw_addr; /* inner register address */
	u8 intr_clear_type; /* interrupt clear */
	u8 intr_sw_idx_w; /* To do chk type interrupt software index */
	u16 rx_buff_size; /* To do chk type Rx buffer size */
	u8 rss_type; /* rss protocol type */
	uint16_t rx_payload_offset; /* start of the payload offset */
	uint32_t flags; /* internal flags */
	int active; /* status */
	struct ipq9574_edma_txdesc_ring *txdesc_ring; /* Tx Descriptor Ring, SW is producer */
	struct ipq9574_edma_sec_txdesc_ring *sec_txdesc_ring; /* secondary Tx Descriptor Ring, SW is producer */
	struct ipq9574_edma_txcmpl_ring *txcmpl_ring; /* Tx Completion Ring, SW is consumer */
	struct ipq9574_edma_rxdesc_ring *rxdesc_ring; /* Rx Descriptor Ring, SW is consumer */
	struct ipq9574_edma_sec_rxdesc_ring *sec_rxdesc_ring; /* secondary Rx Descriptor Ring, SW is consumer */
	struct ipq9574_edma_rxfill_ring *rxfill_ring; /* Rx Fill Ring, SW is producer */
	uint32_t txdesc_rings; /* Number of TxDesc rings */
	uint32_t txdesc_ring_start; /* Id of first TXDESC ring */
	uint32_t txdesc_ring_end; /* Id of the last TXDESC ring */
	uint32_t sec_txdesc_rings; /* Number of secondary TxDesc rings */
	uint32_t sec_txdesc_ring_start; /* Id of first secondary TxDesc ring */
	uint32_t sec_txdesc_ring_end; /* Id of last secondary TxDesc ring */
	uint32_t txcmpl_rings; /* Number of TxCmpl rings */
	uint32_t txcmpl_ring_start; /* Id of first TXCMPL ring */
	uint32_t txcmpl_ring_end; /* Id of last TXCMPL ring */
	uint32_t rxfill_rings; /* Number of RxFill rings */
	uint32_t rxfill_ring_start; /* Id of first RxFill ring */
	uint32_t rxfill_ring_end; /* Id of last RxFill ring */
	uint32_t rxdesc_rings; /* Number of RxDesc rings */
	uint32_t rxdesc_ring_start; /* Id of first RxDesc ring */
	uint32_t rxdesc_ring_end; /* Id of last RxDesc ring */
	uint32_t sec_rxdesc_rings; /* Number of secondary RxDesc rings */
	uint32_t sec_rxdesc_ring_start; /* Id of first secondary RxDesc ring */
	uint32_t sec_rxdesc_ring_end; /* Id of last secondary RxDesc ring */
	uint32_t tx_intr_mask; /* tx interrupt mask */
	uint32_t rx_intr_mask; /* rx interrupt mask */
	uint32_t rxfill_intr_mask; /* Rx fill ring interrupt mask */
	uint32_t rxdesc_intr_mask; /* Rx Desc ring interrupt mask */
	uint32_t txcmpl_intr_mask; /* Tx Cmpl ring interrupt mask */
	uint32_t misc_intr_mask; /* misc interrupt interrupt mask */
};

struct ipq9574_edma_common_info {
	struct ipq9574_edma_hw hw;
};

#define MAX_PHY 6
struct ipq9574_eth_dev {
	u8 *phy_address;
	uint no_of_phys;
	uint interface;
	uint speed;
	uint duplex;
	uint sw_configured;
	uint mac_unit;
	uint mac_ps;
	int link_printed;
	u32 padding;
	struct eth_device *dev;
	struct ipq9574_edma_common_info *c_info;
	struct phy_ops *ops[MAX_PHY];
	const char phy_name[MDIO_NAME_LEN];
} __attribute__ ((aligned(8)));

static inline void* ipq9574_alloc_mem(u32 size)
{
	void *p = malloc(size);
	if (p != NULL)
		memset(p, 0, size);
	return p;
}

static inline void* ipq9574_alloc_memalign(u32 size)
{
	void *p = memalign(CONFIG_SYS_CACHELINE_SIZE, size);
	if (p != NULL)
		memset(p, 0, size);
	return p;
}

static inline void ipq9574_free_mem(void *ptr)
{
	if (ptr)
		free(ptr);
}

uint32_t ipq9574_edma_reg_read(uint32_t reg_off);
void ipq9574_edma_reg_write(uint32_t reg_off, uint32_t val);


extern int get_eth_mac_address(uchar *enetaddr, uint no_of_macs);

typedef struct {
	uint count;
	u8 addr[7];
} ipq9574_edma_phy_addr_t;

/* ipq9574 edma Paramaters */
typedef struct {
	uint base;
	int unit;
	uint mac_conn_to_phy;
	phy_interface_t phy;
	ipq9574_edma_phy_addr_t phy_addr;
	char phy_name[MDIO_NAME_LEN];
} ipq9574_edma_board_cfg_t;

extern void ipq9574_ppe_provision_init(void);
extern void ipq9574_port_mac_clock_reset(int port);
extern void ipq9574_speed_clock_set(int port, int clk[4]);
extern void ipq9574_pqsgmii_speed_set(int port, int speed, int status);
extern void ipq9574_uxsgmii_speed_set(int port, int speed, int duplex, int status);
extern void ppe_port_mux_mac_type_set(int port_id, int mode);
extern void ppe_port_bridge_txmac_set(int port, int status);
extern void ipq9574_10g_r_speed_set(int port, int status);
extern int phy_status_get_from_ppe(int port_id);

extern void ipq9574_ppe_acl_set(int rule_id, int rule_type, int pkt_type, int l4_port_no, int l4_port_mask, int permit, int deny);
extern void ppe_uniphy_mode_set(uint32_t uniphy_index, uint32_t mode);
#endif /* ___IPQ9574_EDMA__ */
