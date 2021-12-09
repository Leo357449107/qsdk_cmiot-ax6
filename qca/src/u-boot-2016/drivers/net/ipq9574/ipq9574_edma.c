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
#include <common.h>
#include <net.h>
#include <asm-generic/errno.h>
#include <asm/io.h>
#include <malloc.h>
#include <phy.h>
#include <net.h>
#include <miiphy.h>
#include <asm/arch-ipq9574/edma_regs.h>
#include <asm/global_data.h>
#include <fdtdec.h>
#include "ipq9574_edma.h"
#include "ipq_phy.h"

DECLARE_GLOBAL_DATA_PTR;
#ifdef DEBUG
#define pr_debug(fmt, args...) printf(fmt, ##args);
#else
#define pr_debug(fmt, args...)
#endif

#define pr_info(fmt, args...) printf(fmt, ##args);
#define pr_warn(fmt, args...) printf(fmt, ##args);

#ifndef CONFIG_IPQ9574_BRIDGED_MODE
#define IPQ9574_EDMA_MAC_PORT_NO	3
#endif

static struct ipq9574_eth_dev *ipq9574_edma_dev[IPQ9574_EDMA_DEV];

uchar ipq9574_def_enetaddr[6] = {0x00, 0x03, 0x7F, 0xBA, 0xDB, 0xAD};
phy_info_t *phy_info[IPQ9574_PHY_MAX] = {0};
int sgmii_mode[2] = {0};

extern int ipq_sw_mdio_init(const char *);
extern int ipq_mdio_read(int mii_id, int regnum, ushort *data);
extern void ipq9574_qca8075_phy_map_ops(struct phy_ops **ops);
extern int ipq9574_qca8075_phy_init(struct phy_ops **ops, u32 phy_id);
extern void ipq9574_qca8075_phy_interface_set_mode(uint32_t phy_id, uint32_t mode);
extern int ipq_qca8033_phy_init(struct phy_ops **ops, u32 phy_id);
extern int ipq_qca8081_phy_init(struct phy_ops **ops, u32 phy_id);
extern int ipq_qca_aquantia_phy_init(struct phy_ops **ops, u32 phy_id);
extern int ipq_board_fw_download(unsigned int phy_addr);

static int tftp_acl_our_port;

/*
 * EDMA hardware instance
 */
static u32 ipq9574_edma_hw_addr;

/*
 * ipq9574_edma_reg_read()
 *	Read EDMA register
 */
uint32_t ipq9574_edma_reg_read(uint32_t reg_off)
{
	return (uint32_t)readl(ipq9574_edma_hw_addr + reg_off);
}

/*
 * ipq9574_edma_reg_write()
 *	Write EDMA register
 */
void ipq9574_edma_reg_write(uint32_t reg_off, uint32_t val)
{
	writel(val, (ipq9574_edma_hw_addr + reg_off));
}

/*
 * ipq9574_edma_alloc_rx_buffer()
 *	Alloc Rx buffers for one RxFill ring
 */
int ipq9574_edma_alloc_rx_buffer(struct ipq9574_edma_hw *ehw,
		struct ipq9574_edma_rxfill_ring *rxfill_ring)
{
	uint16_t num_alloc = 0;
	uint16_t cons, next, counter;
	struct ipq9574_edma_rxfill_desc *rxfill_desc;
	uint32_t reg_data;

	/*
	 * Read RXFILL ring producer index
	 */
	reg_data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXFILL_PROD_IDX(
					rxfill_ring->id));

	next = reg_data & IPQ9574_EDMA_RXFILL_PROD_IDX_MASK & (rxfill_ring->count - 1);

	/*
	 * Read RXFILL ring consumer index
	 */
	reg_data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXFILL_CONS_IDX(
					rxfill_ring->id));

	cons = reg_data & IPQ9574_EDMA_RXFILL_CONS_IDX_MASK;

	while (1) {

		counter = next;

		if (++counter == rxfill_ring->count)
			counter = 0;

		if (counter == cons)
			break;

		if (counter >= CONFIG_SYS_RX_ETH_BUFFER) {
			pr_info("%s: counter >= CONFIG_SYS_RX_ETH_BUFFER counter = %d\n",
				__func__, counter);
			break;
		}
		/*
		 * Get RXFILL descriptor
		 */
		rxfill_desc = IPQ9574_EDMA_RXFILL_DESC(rxfill_ring, next);

		/*
		 * Fill the opaque value
		 */
		rxfill_desc->rdes2 = next;

		/*
		 * Save buffer size in RXFILL descriptor
		 */
		rxfill_desc->rdes1 |= (IPQ9574_EDMA_RX_BUFF_SIZE <<
				      IPQ9574_EDMA_RXFILL_BUF_SIZE_SHIFT) &
				      IPQ9574_EDMA_RXFILL_BUF_SIZE_MASK;
		num_alloc++;
		next = counter;
	}

	if (num_alloc) {
		/*
		 * Update RXFILL ring producer index
		 */
		reg_data = next & IPQ9574_EDMA_RXFILL_PROD_IDX_MASK;

		/*
		 * make sure the producer index updated before
		 * updating the hardware
		 */
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXFILL_PROD_IDX(
					rxfill_ring->id), reg_data);

		pr_debug("%s: num_alloc = %d\n", __func__, num_alloc);
	}

	return num_alloc;
}

/*
 * ipq9574_edma_clean_tx()
 *	Reap Tx descriptors
 */
uint32_t ipq9574_edma_clean_tx(struct ipq9574_edma_hw *ehw,
			struct ipq9574_edma_txcmpl_ring *txcmpl_ring)
{
	struct ipq9574_edma_txcmpl_desc *txcmpl_desc;
	uint16_t prod_idx, cons_idx;
	uint32_t data;
	uint32_t txcmpl_consumed = 0;
	uchar *skb;

	/*
	 * Get TXCMPL ring producer index
	 */
	data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXCMPL_PROD_IDX(
					txcmpl_ring->id));
	prod_idx = data & IPQ9574_EDMA_TXCMPL_PROD_IDX_MASK;

	/*
	 * Get TXCMPL ring consumer index
	 */
	data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXCMPL_CONS_IDX(
					txcmpl_ring->id));
	cons_idx = data & IPQ9574_EDMA_TXCMPL_CONS_IDX_MASK;

	while (cons_idx != prod_idx) {

		txcmpl_desc = IPQ9574_EDMA_TXCMPL_DESC(txcmpl_ring, cons_idx);

		skb = (uchar *)txcmpl_desc->tdes0;

		if (unlikely(!skb)) {
			pr_debug("Invalid skb: cons_idx:%u prod_idx:%u\n",
				cons_idx, prod_idx);
		}

		if (++cons_idx == txcmpl_ring->count)
			cons_idx = 0;

		txcmpl_consumed++;
	}

	pr_debug("%s :%u txcmpl_consumed:%u prod_idx:%u cons_idx:%u\n",
		__func__, txcmpl_ring->id, txcmpl_consumed, prod_idx,
		cons_idx);

	if (txcmpl_consumed == 0)
		return 0;

	/*
	 * Update TXCMPL ring consumer index
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXCMPL_CONS_IDX(
				txcmpl_ring->id), cons_idx);

	return txcmpl_consumed;
}

/*
 * ipq9574_edma_clean_rx()
 *	Reap Rx descriptors
 */
uint32_t ipq9574_edma_clean_rx(struct ipq9574_edma_common_info *c_info,
				struct ipq9574_edma_rxdesc_ring *rxdesc_ring)
{
	void *skb;
	struct ipq9574_edma_rxdesc_desc *rxdesc_desc;
	uint16_t prod_idx, cons_idx;
	int src_port_num;
	int pkt_length;
	int rx = CONFIG_SYS_RX_ETH_BUFFER;
	u16 cleaned_count = 0;
	struct ipq9574_edma_hw *ehw = &c_info->hw;

	pr_debug("%s: rxdesc_ring->id = %d\n", __func__, rxdesc_ring->id);
	/*
	 * Read Rx ring consumer index
	 */
	cons_idx = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXDESC_CONS_IDX(
					rxdesc_ring->id)) &
					IPQ9574_EDMA_RXDESC_CONS_IDX_MASK;

	while (rx) {
		/*
		 * Read Rx ring producer index
		 */
		prod_idx = ipq9574_edma_reg_read(
			IPQ9574_EDMA_REG_RXDESC_PROD_IDX(rxdesc_ring->id))
			& IPQ9574_EDMA_RXDESC_PROD_IDX_MASK;

		if (cons_idx == prod_idx) {
			pr_debug("%s: cons = prod \n", __func__);
			break;
		}

		rxdesc_desc = IPQ9574_EDMA_RXDESC_DESC(rxdesc_ring, cons_idx);

		skb = (void *)rxdesc_desc->rdes0;

		rx--;

		/*
		 * Check src_info from Rx Descriptor
		 */
		if (IPQ9574_EDMA_RXPH_SRC_INFO_TYPE_GET(rxdesc_desc->rdes4) ==
				IPQ9574_EDMA_PREHDR_DSTINFO_PORTID_IND) {
			src_port_num = rxdesc_desc->rdes4 &
				IPQ9574_EDMA_PREHDR_PORTNUM_BITS;
		} else {
			pr_warn("WARN: src_info_type:0x%x. Drop skb:%p\n",
				IPQ9574_EDMA_RXPH_SRC_INFO_TYPE_GET(rxdesc_desc->rdes4),
				skb);
			goto next_rx_desc;
		}

		/*
		 * Get packet length
		 */
		pkt_length = (rxdesc_desc->rdes5 &
			      IPQ9574_EDMA_RXDESC_PKT_SIZE_MASK) >>
			      IPQ9574_EDMA_RXDESC_PKT_SIZE_SHIFT;

		if (unlikely((src_port_num < IPQ9574_NSS_DP_START_PHY_PORT)  ||
			(src_port_num > IPQ9574_NSS_DP_MAX_PHY_PORTS))) {
			pr_warn("WARN: Port number error :%d. Drop skb:%p\n",
					src_port_num, skb);
			goto next_rx_desc;
		}

		cleaned_count++;

		pr_debug("%s: received pkt %p with length %d\n",
			__func__, skb, pkt_length);

		net_process_received_packet(skb, pkt_length);
next_rx_desc:
		/*
		 * Update consumer index
		 */
		if (++cons_idx == rxdesc_ring->count)
			cons_idx = 0;
	}

	if (cleaned_count) {
		ipq9574_edma_alloc_rx_buffer(ehw, rxdesc_ring->rxfill);
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_CONS_IDX(
						rxdesc_ring->id), cons_idx);
	}

	return 0;
}

/*
 * ip9574_edma_rx_complete()
 */
static int ipq9574_edma_rx_complete(struct ipq9574_edma_common_info *c_info)
{
	struct ipq9574_edma_hw *ehw = &c_info->hw;
	struct ipq9574_edma_txcmpl_ring *txcmpl_ring;
	struct ipq9574_edma_rxdesc_ring *rxdesc_ring;
	struct ipq9574_edma_rxfill_ring *rxfill_ring;
	uint32_t misc_intr_status, reg_data;
	int i;

	for (i = 0; i < ehw->rxdesc_rings; i++) {
		rxdesc_ring = &ehw->rxdesc_ring[i];
		ipq9574_edma_clean_rx(c_info, rxdesc_ring);
	}

	for (i = 0; i < ehw->txcmpl_rings; i++) {
		txcmpl_ring = &ehw->txcmpl_ring[i];
		ipq9574_edma_clean_tx(ehw, txcmpl_ring);
	}

	for (i = 0; i < ehw->rxfill_rings; i++) {
		rxfill_ring = &ehw->rxfill_ring[i];
		ipq9574_edma_alloc_rx_buffer(ehw, rxfill_ring);
	}

	/*
	 * Set RXDESC ring interrupt mask
	 */
	for (i = 0; i < ehw->rxdesc_rings; i++) {
		rxdesc_ring = &ehw->rxdesc_ring[i];
		ipq9574_edma_reg_write(
			IPQ9574_EDMA_REG_RXDESC_INT_MASK(rxdesc_ring->id),
			ehw->rxdesc_intr_mask);
	}

	/*
	 * Set TXCMPL ring interrupt mask
	 */
	for (i = 0; i < ehw->txcmpl_rings; i++) {
		txcmpl_ring = &ehw->txcmpl_ring[i];
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TX_INT_MASK(
					txcmpl_ring->id),
					ehw->txcmpl_intr_mask);
	}

	/*
	 * Set RXFILL ring interrupt mask
	 */
	for (i = 0; i < ehw->rxfill_rings; i++) {
		rxfill_ring = &ehw->rxfill_ring[i];
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXFILL_INT_MASK(
					rxfill_ring->id),
					ehw->rxfill_intr_mask);
	}

	/*
	 * Read Misc intr status
	 */
	reg_data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_MISC_INT_STAT);
	misc_intr_status = reg_data & ehw->misc_intr_mask;

	if (misc_intr_status != 0) {
		pr_info("%s: misc_intr_status = 0x%x\n", __func__,
			misc_intr_status);
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_MISC_INT_MASK,
					IPQ9574_EDMA_MASK_INT_DISABLE);
	}

	return 0;
}

/*
 * ipq9574_eth_snd()
 *	Transmit a packet using an EDMA ring
 */
static int ipq9574_eth_snd(struct eth_device *dev, void *packet, int length)
{
	struct ipq9574_eth_dev *priv = dev->priv;
	struct ipq9574_edma_common_info *c_info = priv->c_info;
	struct ipq9574_edma_hw *ehw = &c_info->hw;
	struct ipq9574_edma_txdesc_desc *txdesc;
	struct ipq9574_edma_txdesc_ring *txdesc_ring;
	uint16_t hw_next_to_use, hw_next_to_clean, chk_idx;
	uint32_t data;
	uchar *skb;

	txdesc_ring = ehw->txdesc_ring;

	if (tftp_acl_our_port != tftp_our_port) {
		/* Allowing tftp packets */
		ipq9574_ppe_acl_set(3, 0x4, 0x1, tftp_our_port, 0xffff, 0, 0);
		tftp_acl_our_port = tftp_our_port;
	}
	/*
	 * Read TXDESC ring producer index
	 */
	data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC_PROD_IDX(
					txdesc_ring->id));

	hw_next_to_use = data & IPQ9574_EDMA_TXDESC_PROD_IDX_MASK;

	pr_debug("%s: txdesc_ring->id = %d\n", __func__, txdesc_ring->id);

	/*
	 * Read TXDESC ring consumer index
	 */
	/*
	 * TODO - read to local variable to optimize uncached access
	 */
	data = ipq9574_edma_reg_read(
			IPQ9574_EDMA_REG_TXDESC_CONS_IDX(txdesc_ring->id));

	hw_next_to_clean = data & IPQ9574_EDMA_TXDESC_CONS_IDX_MASK;

	/*
	 * Check for available Tx descriptor
	 */
	chk_idx = (hw_next_to_use + 1) & (txdesc_ring->count - 1);

	if (chk_idx == hw_next_to_clean) {
		return NETDEV_TX_BUSY;
	}

	/*
	 * Get Tx descriptor
	 */
	txdesc = IPQ9574_EDMA_TXDESC_DESC(txdesc_ring, hw_next_to_use);

	skb = (uchar *)txdesc->tdes0;

	pr_debug("%s: txdesc->tdes0 (buffer addr) = 0x%x length = %d \
			prod_idx = %d cons_idx = %d\n",
			__func__, txdesc->tdes0, length,
			hw_next_to_use, hw_next_to_clean);

#ifdef CONFIG_IPQ9574_BRIDGED_MODE
	/* VP 0x0 share vsi 2 with port 1-4 */
	/* src is 0x2000, dest is 0x0 */
	txdesc->tdes4 = 0x00002000;
#else
	/*
	 * Populate Tx dst info, port id is macid in dp_dev
	 */
	txdesc->tdes4 |= (((IPQ9574_EDMA_PREHDR_DSTINFO_PORTID_IND << 8) |
			(IPQ9574_EDMA_MAC_PORT_NO & 0x0fff)) << 16);
#endif

	/*
	 * Set opaque field
	 */
	txdesc->tdes2 = cpu_to_le32(skb);

	/*
	 * copy the packet
	 */
	memcpy(skb, packet, length);

	/*
	 * Populate Tx descriptor
	 */
	txdesc->tdes6 |= (1 << IPQ9574_EDMA_TXDESC_PREHEADER_SHIFT);

	txdesc->tdes5 |= ((length << IPQ9574_EDMA_TXDESC_DATA_LENGTH_SHIFT) &
			  IPQ9574_EDMA_TXDESC_DATA_LENGTH_MASK);

	/*
	 * Update producer index
	 */
	hw_next_to_use = (hw_next_to_use + 1) & (txdesc_ring->count - 1);

	/*
	 * make sure the hw_next_to_use is updated before the
	 * write to hardware
	 */

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC_PROD_IDX(
				txdesc_ring->id), hw_next_to_use &
				 IPQ9574_EDMA_TXDESC_PROD_IDX_MASK);

	pr_debug("%s: successfull\n", __func__);

	return EDMA_TX_OK;
}

static int ipq9574_eth_recv(struct eth_device *dev)
{
	struct ipq9574_eth_dev *priv = dev->priv;
	struct ipq9574_edma_common_info *c_info = priv->c_info;
	struct ipq9574_edma_rxdesc_ring *rxdesc_ring;
	struct ipq9574_edma_txcmpl_ring *txcmpl_ring;
	struct ipq9574_edma_rxfill_ring *rxfill_ring;
	struct ipq9574_edma_hw *ehw = &c_info->hw;
	volatile u32 reg_data;
	u32 rxdesc_intr_status = 0, txcmpl_intr_status = 0, rxfill_intr_status = 0;
	int i;

	/*
	 * Read RxDesc intr status
	 */
	for (i = 0; i < ehw->rxdesc_rings; i++) {
		rxdesc_ring = &ehw->rxdesc_ring[i];

		reg_data = ipq9574_edma_reg_read(
				IPQ9574_EDMA_REG_RXDESC_INT_STAT(
					rxdesc_ring->id));
		rxdesc_intr_status |= reg_data &
				IPQ9574_EDMA_RXDESC_RING_INT_STATUS_MASK;

		/*
		 * Disable RxDesc intr
		 */
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_INT_MASK(
					rxdesc_ring->id),
					IPQ9574_EDMA_MASK_INT_DISABLE);
	}

	/*
	 * Read TxCmpl intr status
	 */
	for (i = 0; i < ehw->txcmpl_rings; i++) {
		txcmpl_ring = &ehw->txcmpl_ring[i];

		reg_data = ipq9574_edma_reg_read(
				IPQ9574_EDMA_REG_TX_INT_STAT(
					txcmpl_ring->id));
		txcmpl_intr_status |= reg_data &
				IPQ9574_EDMA_TXCMPL_RING_INT_STATUS_MASK;

		/*
		 * Disable TxCmpl intr
		 */
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TX_INT_MASK(
					txcmpl_ring->id),
					IPQ9574_EDMA_MASK_INT_DISABLE);
	}

	/*
	 * Read RxFill intr status
	 */
	for (i = 0; i < ehw->rxfill_rings; i++) {
		rxfill_ring = &ehw->rxfill_ring[i];

		reg_data = ipq9574_edma_reg_read(
				IPQ9574_EDMA_REG_RXFILL_INT_STAT(
					rxfill_ring->id));
		rxfill_intr_status |= reg_data &
				IPQ9574_EDMA_RXFILL_RING_INT_STATUS_MASK;

		/*
		 * Disable RxFill intr
		 */
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXFILL_INT_MASK(
					rxfill_ring->id),
					IPQ9574_EDMA_MASK_INT_DISABLE);
	}

	if ((rxdesc_intr_status != 0) || (txcmpl_intr_status != 0) ||
	    (rxfill_intr_status != 0)) {
		for (i = 0; i < ehw->rxdesc_rings; i++) {
			rxdesc_ring = &ehw->rxdesc_ring[i];
			ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_INT_MASK(
						rxdesc_ring->id),
						IPQ9574_EDMA_MASK_INT_DISABLE);
		}
		ipq9574_edma_rx_complete(c_info);
	}

	return 0;
}

/*
 * ipq9574_edma_setup_ring_resources()
 *	Allocate/setup resources for EDMA rings
 */
static int ipq9574_edma_setup_ring_resources(struct ipq9574_edma_hw *ehw)
{
	struct ipq9574_edma_txcmpl_ring *txcmpl_ring;
	struct ipq9574_edma_txdesc_ring *txdesc_ring;
	struct ipq9574_edma_sec_txdesc_ring *sec_txdesc_ring;
	struct ipq9574_edma_rxfill_ring *rxfill_ring;
	struct ipq9574_edma_rxdesc_ring *rxdesc_ring;
	struct ipq9574_edma_sec_rxdesc_ring *sec_rxdesc_ring;
	struct ipq9574_edma_txdesc_desc *tx_desc;
	struct ipq9574_edma_rxfill_desc *rxfill_desc;
	int i, j, index;
	void *tx_buf;
	void *rx_buf;

	/*
	 * Allocate Rx fill ring descriptors
	 */
	for (i = 0; i < ehw->rxfill_rings; i++) {
		rxfill_ring = &ehw->rxfill_ring[i];
		rxfill_ring->count = EDMA_RING_SIZE;
		rxfill_ring->id = ehw->rxfill_ring_start + i;
		rxfill_ring->desc = (void *)noncached_alloc(
				IPQ9574_EDMA_RXFILL_DESC_SIZE * rxfill_ring->count,
				CONFIG_SYS_CACHELINE_SIZE);

		if (rxfill_ring->desc == NULL) {
			pr_info("%s: rxfill_ring->desc alloc error\n", __func__);
			return -ENOMEM;
		}
		rxfill_ring->dma = virt_to_phys(rxfill_ring->desc);
		pr_debug("rxfill ring id = %d, rxfill ring ptr = %p, rxfill ring dma = %u\n",
			rxfill_ring->id, rxfill_ring->desc, (unsigned int)
			rxfill_ring->dma);

		rx_buf = (void *)noncached_alloc(IPQ9574_EDMA_RX_BUFF_SIZE *
					rxfill_ring->count,
					CONFIG_SYS_CACHELINE_SIZE);

		if (rx_buf == NULL) {
			pr_info("%s: rxfill_ring->desc buffer alloc error\n",
				 __func__);
			return -ENOMEM;
		}

		for (j = 0; j < rxfill_ring->count; j++) {
			rxfill_desc = IPQ9574_EDMA_RXFILL_DESC(rxfill_ring, j);
			rxfill_desc->rdes0 = virt_to_phys(rx_buf);
			rxfill_desc->rdes1 = 0;
			rxfill_desc->rdes2 = 0;
			rxfill_desc->rdes3 = 0;
			rx_buf += IPQ9574_EDMA_RX_BUFF_SIZE;
			pr_debug("Ring %d: rxfill ring dis0 ptr = %p, rxfill ring dis0 dma = %u\n",
				j, rxfill_desc, (unsigned int)rxfill_desc->rdes0);
		}
	}

	/*
	 * Allocate secondary RxDesc ring descriptors
	 */
	for (i = 0; i < ehw->sec_rxdesc_rings; i++) {
		sec_rxdesc_ring = &ehw->sec_rxdesc_ring[i];
		sec_rxdesc_ring->count = EDMA_RING_SIZE;
		sec_rxdesc_ring->id = ehw->sec_rxdesc_ring_start + i;
		sec_rxdesc_ring->desc = (void *)noncached_alloc(
				IPQ9574_EDMA_RX_SEC_DESC_SIZE * sec_rxdesc_ring->count,
				CONFIG_SYS_CACHELINE_SIZE);
		if (sec_rxdesc_ring->desc == NULL) {
			pr_info("%s: sec_rxdesc_ring->desc alloc error\n", __func__);
			return -ENOMEM;
		}
		sec_rxdesc_ring->dma = virt_to_phys(sec_rxdesc_ring->desc);
		pr_debug("sec rxdesc ring id = %d, sec rxdesc ring ptr = %p, sec rxdesc ring dma = %u\n",
			sec_rxdesc_ring->id, sec_rxdesc_ring->desc, (unsigned int)
			sec_rxdesc_ring->dma);
	}

	/*
	 * Allocate RxDesc ring descriptors
	 */
	for (i = 0; i < ehw->rxdesc_rings; i++) {
		rxdesc_ring = &ehw->rxdesc_ring[i];
		rxdesc_ring->count = EDMA_RING_SIZE;
		rxdesc_ring->id = ehw->rxdesc_ring_start + i;

		/*
		 * Create a mapping between RX Desc ring and Rx fill ring.
		 * Number of fill rings are lesser than the descriptor rings
		 * Share the fill rings across descriptor rings.
		 */

		index = ehw->rxfill_ring_start + (i % ehw->rxfill_rings);
		rxdesc_ring->rxfill =
			&ehw->rxfill_ring[index - ehw->rxfill_ring_start];
		rxdesc_ring->rxfill = ehw->rxfill_ring;

		rxdesc_ring->desc = (void *)noncached_alloc(
				IPQ9574_EDMA_RXDESC_DESC_SIZE * rxdesc_ring->count,
				CONFIG_SYS_CACHELINE_SIZE);

		if (rxdesc_ring->desc == NULL) {
			pr_info("%s: rxdesc_ring->desc alloc error\n", __func__);
			return -ENOMEM;
		}
		rxdesc_ring->dma = virt_to_phys(rxdesc_ring->desc);
		pr_debug("rxdesc ring id = %d, rxdesc ring ptr = %p, rxdesc ring dma = %u\n",
			rxdesc_ring->id, rxdesc_ring->desc, (unsigned int)
			rxdesc_ring->dma);
	}

	/*
	 * Allocate TxCmpl ring descriptors
	 */
	for (i = 0; i < ehw->txcmpl_rings; i++) {
		txcmpl_ring = &ehw->txcmpl_ring[i];
		txcmpl_ring->count = EDMA_RING_SIZE;
		txcmpl_ring->id = ehw->txcmpl_ring_start + i;
		txcmpl_ring->desc = (void *)noncached_alloc(
				IPQ9574_EDMA_TXCMPL_DESC_SIZE * txcmpl_ring->count,
				CONFIG_SYS_CACHELINE_SIZE);

		if (txcmpl_ring->desc == NULL) {
			pr_info("%s: txcmpl_ring->desc alloc error\n", __func__);
			return -ENOMEM;
		}
		txcmpl_ring->dma = virt_to_phys(txcmpl_ring->desc);
		pr_debug("txcmpl ring id = %d, txcmpl ring ptr = %p, txcmpl ring dma = %u\n",
			txcmpl_ring->id, txcmpl_ring->desc, (unsigned int)
			txcmpl_ring->dma);
	}

	/*
	 * Allocate secondary TxDesc ring descriptors
	 */
	for (i = 0; i < ehw->sec_txdesc_rings; i++) {
		sec_txdesc_ring = &ehw->sec_txdesc_ring[i];
		sec_txdesc_ring->count = EDMA_RING_SIZE;
		sec_txdesc_ring->id = ehw->sec_txdesc_ring_start + i;
		sec_txdesc_ring->desc = (void *)noncached_alloc(
				IPQ9574_EDMA_TX_SEC_DESC_SIZE * sec_txdesc_ring->count,
				CONFIG_SYS_CACHELINE_SIZE);
		if (sec_txdesc_ring->desc == NULL) {
			pr_info("%s: sec_txdesc_ring->desc alloc error\n", __func__);
			return -ENOMEM;
		}
		sec_txdesc_ring->dma = virt_to_phys(sec_txdesc_ring->desc);
		pr_debug("sec txdesc ring id = %d, sec txdesc ring ptr = %p, sec txdesc ring dma = %u\n",
			sec_txdesc_ring->id, sec_txdesc_ring->desc, (unsigned int)
			sec_txdesc_ring->dma);
	}

	/*
	 * Allocate TxDesc ring descriptors
	 */
	for (i = 0; i < ehw->txdesc_rings; i++) {
		txdesc_ring = &ehw->txdesc_ring[i];
		txdesc_ring->count = EDMA_RING_SIZE;
		txdesc_ring->id = ehw->txdesc_ring_start + i;
		txdesc_ring->desc = (void *)noncached_alloc(
				IPQ9574_EDMA_TXDESC_DESC_SIZE * txdesc_ring->count,
				CONFIG_SYS_CACHELINE_SIZE);
		if (txdesc_ring->desc == NULL) {
			pr_info("%s: txdesc_ring->desc alloc error\n", __func__);
			return -ENOMEM;
		}
		txdesc_ring->dma = virt_to_phys(txdesc_ring->desc);
		pr_debug("txdesc ring id = %d, txdesc ring ptr = %p, txdesc ring dma = %u\n",
			txdesc_ring->id, txdesc_ring->desc, (unsigned int)
			txdesc_ring->dma);

		tx_buf = (void *)noncached_alloc(IPQ9574_EDMA_TX_BUFF_SIZE *
					txdesc_ring->count,
					CONFIG_SYS_CACHELINE_SIZE);
		if (tx_buf == NULL) {
			pr_info("%s: txdesc_ring->desc buffer alloc error\n",
				 __func__);
			return -ENOMEM;
		}

		for (j = 0; j < txdesc_ring->count; j++) {
			tx_desc = IPQ9574_EDMA_TXDESC_DESC(txdesc_ring, j);
			tx_desc->tdes0 = virt_to_phys(tx_buf);
			tx_desc->tdes1 = 0;
			tx_desc->tdes2 = 0;
			tx_desc->tdes3 = 0;
			tx_desc->tdes4 = 0;
			tx_desc->tdes5 = 0;
			tx_desc->tdes6 = 0;
			tx_desc->tdes7 = 0;
			tx_buf += IPQ9574_EDMA_TX_BUFF_SIZE;
			pr_debug("Ring %d: txdesc ring dis0 ptr = %p, txdesc ring dis0 dma = %u\n",
				j, tx_desc, (unsigned int)tx_desc->tdes0);

		}
	}

	pr_info("%s: successfull\n", __func__);

	return 0;
}

/*
 * ipq9574_edma_free_desc()
 *	Free EDMA desc memory
 */
static void ipq9574_edma_free_desc(struct ipq9574_edma_common_info *c_info)
{
	struct ipq9574_edma_hw *ehw = &c_info->hw;
	struct ipq9574_edma_txcmpl_ring *txcmpl_ring;
	struct ipq9574_edma_txdesc_ring *txdesc_ring;
	struct ipq9574_edma_rxfill_ring *rxfill_ring;
	struct ipq9574_edma_rxdesc_ring *rxdesc_ring;
	struct ipq9574_edma_txdesc_desc *tx_desc;
	int i;

	for (i = 0; i < ehw->rxfill_rings; i++) {
		rxfill_ring = &ehw->rxfill_ring[i];
		if (rxfill_ring->desc)
			ipq9574_free_mem(rxfill_ring->desc);
	}

	for (i = 0; i < ehw->rxdesc_rings; i++) {
		rxdesc_ring = &ehw->rxdesc_ring[i];
		if (rxdesc_ring->desc)
			ipq9574_free_mem(rxdesc_ring->desc);
	}

	for (i = 0; i < ehw->txcmpl_rings; i++) {
		txcmpl_ring = &ehw->txcmpl_ring[i];
		if (txcmpl_ring->desc) {
			ipq9574_free_mem(txcmpl_ring->desc);
		}
	}

	for (i = 0; i < ehw->txdesc_rings; i++) {
		txdesc_ring = &ehw->txdesc_ring[i];
		if (txdesc_ring->desc) {
			tx_desc = IPQ9574_EDMA_TXDESC_DESC(txdesc_ring, 0);
			if (tx_desc->tdes0)
				ipq9574_free_mem((void *)tx_desc->tdes0);
			ipq9574_free_mem(txdesc_ring->desc);
		}
	}
}

/*
 * ipq9574_edma_free_rings()
 *	Free EDMA software rings
 */
static void ipq9574_edma_free_rings(struct ipq9574_edma_common_info *c_info)
{
	struct ipq9574_edma_hw *ehw = &c_info->hw;
	ipq9574_free_mem(ehw->rxfill_ring);
	ipq9574_free_mem(ehw->rxdesc_ring);
	ipq9574_free_mem(ehw->txdesc_ring);
	ipq9574_free_mem(ehw->txcmpl_ring);
}

static void ipq9574_edma_disable_rings(struct ipq9574_edma_hw *edma_hw)
{
	int i, desc_index;
	u32 data;

	/*
	 * Disable Rx rings
	 */
	for (i = 0; i < IPQ9574_EDMA_MAX_RXDESC_RINGS; i++) {
		data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXDESC_CTRL(i));
		data &= ~IPQ9574_EDMA_RXDESC_RX_EN;
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_CTRL(i), data);
	}

	/*
	 * Disable RxFill Rings
	 */
	for (i = 0; i < IPQ9574_EDMA_MAX_RXFILL_RINGS; i++) {
		data = ipq9574_edma_reg_read(
				IPQ9574_EDMA_REG_RXFILL_RING_EN(i));
		data &= ~IPQ9574_EDMA_RXFILL_RING_EN;
		ipq9574_edma_reg_write(
				IPQ9574_EDMA_REG_RXFILL_RING_EN(i), data);
	}

	/*
	 * Disable Tx rings
	 */
	for (desc_index = 0; desc_index <
			 IPQ9574_EDMA_MAX_TXDESC_RINGS; desc_index++) {
		data = ipq9574_edma_reg_read(
				IPQ9574_EDMA_REG_TXDESC_CTRL(desc_index));
		data &= ~IPQ9574_EDMA_TXDESC_TX_EN;
		ipq9574_edma_reg_write(
				IPQ9574_EDMA_REG_TXDESC_CTRL(desc_index), data);
	}
}

static void ipq9574_edma_disable_intr(struct ipq9574_edma_hw *ehw)
{
	int i;

	/*
	 * Disable interrupts
	 */
	for (i = 0; i < IPQ9574_EDMA_MAX_TXCMPL_RINGS; i++)
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TX_INT_MASK(i), 0);

	for (i = 0; i < IPQ9574_EDMA_MAX_RXFILL_RINGS; i++)
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXFILL_INT_MASK(i), 0);

	for (i = 0; i < IPQ9574_EDMA_MAX_RXDESC_RINGS; i++)
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RX_INT_CTRL(i), 0);

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_MISC_INT_MASK,
				IPQ9574_EDMA_MASK_INT_DISABLE);
}

static void set_sgmii_mode(int port_id, int sg_mode)
{
	if (port_id == 4)
		sgmii_mode[0] = sg_mode;
	else if (port_id == 5)
		sgmii_mode[1] = sg_mode;
}

static int get_sgmii_mode(int port_id)
{
	if (port_id == 4)
		return sgmii_mode[0];
	else if (port_id == 5)
		return sgmii_mode[1];
	else
		return -1;
}

static int ipq9574_eth_init(struct eth_device *eth_dev, bd_t *this)
{
	int i;
	u8 status = 0;
	int mac_speed = 0x0;
	struct ipq9574_eth_dev *priv = eth_dev->priv;
	struct phy_ops *phy_get_ops;
	static fal_port_speed_t old_speed[IPQ9574_PHY_MAX] = {[0 ... IPQ9574_PHY_MAX-1] = FAL_SPEED_BUTT};
	static fal_port_speed_t curr_speed[IPQ9574_PHY_MAX];
	fal_port_duplex_t duplex;
	char *lstatus[] = {"up", "Down"};
	char *dp[] = {"Half", "Full"};
	int linkup = 0;
	int clk[4] = {0};
	int phy_addr = -1, node = -1;
	int aquantia_port[2] = {-1, -1}, aquantia_port_cnt = -1;
	int sfp_port[2] = {-1, -1}, sfp_port_cnt = -1;
	int sgmii_mode = -1, sfp_mode = -1, sgmii_fiber = -1;
	int phy_node = -1, res = -1;

	node = fdt_path_offset(gd->fdt_blob, "/ess-switch");

	if (node >= 0) {
		aquantia_port_cnt = fdtdec_get_uint(gd->fdt_blob, node, "aquantia_port_cnt", -1);

		if (aquantia_port_cnt >= 1) {
			res = fdtdec_get_int_array(gd->fdt_blob, node, "aquantia_port",
					  (u32 *)aquantia_port, aquantia_port_cnt);
			if (res < 0)
				printf("Error: Aquantia port details not provided in DT\n");
		}

		sfp_port_cnt = fdtdec_get_uint(gd->fdt_blob, node, "sfp_port_cnt", -1);

		if (sfp_port_cnt >= 1) {
			res = fdtdec_get_int_array(gd->fdt_blob, node, "sfp_port",
					(u32 *)sfp_port, sfp_port_cnt);
			if (res < 0)
				printf("Error: SFP port details not provided in DT\n");
		}
	}

	phy_node = fdt_path_offset(gd->fdt_blob, "/ess-switch/port_phyinfo");
	/*
	 * Check PHY link, speed, Duplex on all phys.
	 * we will proceed even if single link is up
	 * else we will return with -1;
	 */
	for (i =  0; i < IPQ9574_PHY_MAX; i++) {
		if (i == sfp_port[0] || i == sfp_port[1]) {
			status = phy_status_get_from_ppe(i);
			duplex = FAL_FULL_DUPLEX;
			/* SFP Port can be enabled in USXGMII0 or USXGMII1 i.e
			 * SFP Port can be port5 or port6 (with port id - 4 or 5).
			 * Port5 (port id - 4) -> Serdes1
			 * Port6 (port id - 5) -> Serdes2
			 */
			if (i == 4) {
				sfp_mode = fdtdec_get_uint(gd->fdt_blob, node, "switch_mac_mode1", -1);
				if (sfp_mode < 0) {
					printf("Error: switch_mac_mode1 not specified in dts\n");
					return sfp_mode;
				}
			} else if (i == 5) {
				sfp_mode = fdtdec_get_uint(gd->fdt_blob, node, "switch_mac_mode2", -1);
				if (sfp_mode < 0) {
					printf("Error: switch_mac_mode2 not specified in dts\n");
					return sfp_mode;
				}
			} else {
				printf("Error: SFP Port can be enabled in USXGMII0 or USXGMII1 (Port5 or Port6) only in ipq9574 platform\n");
			}
			if (sfp_mode == EPORT_WRAPPER_SGMII_FIBER) {
				sgmii_fiber = 1;
				curr_speed[i] = FAL_SPEED_1000;
			} else if (sfp_mode == EPORT_WRAPPER_10GBASE_R) {
				sgmii_fiber = 0;
				curr_speed[i] = FAL_SPEED_10000;
			} else if (sfp_mode == EPORT_WRAPPER_SGMII_PLUS) {
				sgmii_fiber = 0;
				curr_speed[i] = FAL_SPEED_2500;
			} else {
				printf("Error: Wrong mode specified for SFP Port in DT\n");
				return sfp_mode;
			}
		} else {
			if (!priv->ops[i]) {
				printf("Phy ops not mapped\n");
				continue;
			}
			phy_get_ops = priv->ops[i];

			if (!phy_get_ops->phy_get_link_status ||
					!phy_get_ops->phy_get_speed ||
					!phy_get_ops->phy_get_duplex) {
				printf("Error:Link status/Get speed/Get duplex not mapped\n");
				return -1;
			}

			if (phy_node >= 0) {
				/*
				 * For each ethernet port, one node should be added
				 * inside port_phyinfo with appropriate phy address
				 */
				phy_addr = phy_info[i]->phy_address;
			} else {
				printf("Error:Phy addresses not configured in DT\n");
				return -1;
			}

			status = phy_get_ops->phy_get_link_status(priv->mac_unit, phy_addr);
			phy_get_ops->phy_get_speed(priv->mac_unit, phy_addr, &curr_speed[i]);
			phy_get_ops->phy_get_duplex(priv->mac_unit, phy_addr, &duplex);
		}

		if (status == 0) {
			linkup++;
			if (old_speed[i] == curr_speed[i]) {
				printf("eth%d PHY%d %s Speed :%d %s duplex\n",
					priv->mac_unit, i, lstatus[status], curr_speed[i],
					dp[duplex]);
				continue;
			} else {
				old_speed[i] = curr_speed[i];
			}
		} else {
			printf("eth%d PHY%d %s Speed :%d %s duplex\n",
				priv->mac_unit, i, lstatus[status], curr_speed[i],
				dp[duplex]);
			continue;
		}

		/*
		 * Note: If the current port link is up and its speed is
		 * different from its initially configured speed, only then
		 * below re-configuration is done.
		 *
		 * These conditions are checked above and if any of it
		 * fails, then no config is done for that eth port.
		 */
		switch (curr_speed[i]) {
			case FAL_SPEED_10:
				mac_speed = 0x0;
				clk[0] = 0x209;
				clk[1] = 0x9;
				clk[2] = 0x309;
				clk[3] = 0x9;
				if (i == aquantia_port[0] || i == aquantia_port[1]) {
					clk[1] = 0x18;
					clk[3] = 0x18;
					if (i == 4) {
						clk[0] = 0x413;
						clk[2] = 0x513;
					} else {
						clk[0] = 0x213;
						clk[2] = 0x313;
					}
				}
				if (phy_node >= 0) {
					if (phy_info[i]->phy_type == QCA8081_PHY_TYPE) {
						set_sgmii_mode(i, 1);
						if (i == 4) {
							clk[0] = 0x409;
							clk[2] = 0x509;
						}
					}
				}
				printf("eth%d PHY%d %s Speed :%d %s duplex\n",
						priv->mac_unit, i, lstatus[status], curr_speed[i],
						dp[duplex]);
				break;
			case FAL_SPEED_100:
				mac_speed = 0x1;
				clk[0] = 0x209;
				clk[1] = 0x0;
				clk[2] = 0x309;
				clk[3] = 0x0;
				if (i == aquantia_port[0] || i == aquantia_port[1]) {
					clk[1] = 0x4;
					clk[3] = 0x4;
					if (i == 4) {
						clk[0] = 0x409;
						clk[2] = 0x509;
					}
				}
				if (phy_node >= 0) {
					if (phy_info[i]->phy_type == QCA8081_PHY_TYPE) {
						set_sgmii_mode(i, 1);
						if (i == 4) {
							clk[0] = 0x409;
							clk[2] = 0x509;
						}
					}
				}
				printf("eth%d PHY%d %s Speed :%d %s duplex\n",
						priv->mac_unit, i, lstatus[status], curr_speed[i],
						dp[duplex]);
				break;
			case FAL_SPEED_1000:
				mac_speed = 0x2;
				clk[0] = 0x201;
				clk[1] = 0x0;
				clk[2] = 0x301;
				clk[3] = 0x0;
				if (i == aquantia_port[0] || i == aquantia_port[1]) {
					if (i == 4) {
						clk[0] = 0x404;
						clk[2] = 0x504;
					} else {
						clk[0] = 0x204;
						clk[2] = 0x304;
					}
				} else if (i == sfp_port[0] || i == sfp_port[1]) {
					if (i == 4) {
						clk[0] = 0x401;
						clk[2] = 0x501;
					}
				}
				if (phy_node >= 0) {
					if (phy_info[i]->phy_type == QCA8081_PHY_TYPE) {
						set_sgmii_mode(i, 1);
						if (i == 4) {
							clk[0] = 0x401;
							clk[2] = 0x501;
						}
					}
				}
				printf("eth%d PHY%d %s Speed :%d %s duplex\n",
						priv->mac_unit, i, lstatus[status], curr_speed[i],
						dp[duplex]);
				break;
			case FAL_SPEED_2500:
				clk[1] = 0x0;
				clk[3] = 0x0;
				if (i == aquantia_port[0] || i == aquantia_port[1]) {
					mac_speed = 0x4;
					if (i == 4) {
						clk[0] = 0x407;
						clk[2] = 0x507;
					} else {
						clk[0] = 0x207;
						clk[2] = 0x307;
					}
				} else if (i == sfp_port[0] || i == sfp_port[1]) {
					mac_speed = 0x2;
					if (i == 4) {
						clk[0] = 0x401;
						clk[2] = 0x501;
					} else {
						clk[0] = 0x201;
						clk[2] = 0x301;
					}
				}
				if (phy_node >= 0) {
					if (phy_info[i]->phy_type == QCA8081_PHY_TYPE) {
						mac_speed = 0x2;
						set_sgmii_mode(i, 0);
						if (i == 4) {
							clk[0] = 0x401;
							clk[2] = 0x501;
						} else {
							clk[0] = 0x201;
							clk[2] = 0x301;
						}
					}
				}
				printf("eth%d PHY%d %s Speed :%d %s duplex\n",
						priv->mac_unit, i, lstatus[status], curr_speed[i],
						dp[duplex]);
				break;
			case FAL_SPEED_5000:
				mac_speed = 0x5;
				clk[1] = 0x0;
				clk[3] = 0x0;
				if (i == 4) {
					clk[0] = 0x403;
					clk[2] = 0x503;
				} else {
					clk[0] = 0x203;
					clk[2] = 0x303;
				}
				printf("eth%d PHY%d %s Speed :%d %s duplex\n",
						priv->mac_unit, i, lstatus[status], curr_speed[i],
						dp[duplex]);
				break;
			case FAL_SPEED_10000:
				mac_speed = 0x3;
				clk[1] = 0x0;
				clk[3] = 0x0;
				if (i == 4) {
					clk[0] = 0x401;
					clk[2] = 0x501;
				} else {
					clk[0] = 0x201;
					clk[2] = 0x301;
				}
				printf("eth%d PHY%d %s Speed :%d %s duplex\n",
						priv->mac_unit, i, lstatus[status], curr_speed[i],
						dp[duplex]);
				break;
			default:
				printf("Unknown speed\n");
				break;
		}

		if (phy_node >= 0) {
			if (phy_info[i]->phy_type == QCA8081_PHY_TYPE) {
				sgmii_mode = get_sgmii_mode(i);
				ppe_port_bridge_txmac_set(i + 1, 1);
				if (sgmii_mode == 1) { /* SGMII Mode */
					if (i == 4)
						ppe_uniphy_mode_set(0x1, EPORT_WRAPPER_SGMII0_RGMII4);
					else if (i == 5)
						ppe_uniphy_mode_set(0x2, EPORT_WRAPPER_SGMII0_RGMII4);

				} else if (sgmii_mode == 0) { /* SGMII Plus Mode */
					if (i == 4)
						ppe_uniphy_mode_set(0x1, EPORT_WRAPPER_SGMII_PLUS);
					else if (i == 5)
						ppe_uniphy_mode_set(0x2, EPORT_WRAPPER_SGMII_PLUS);
				}
			}
		}

		if (i == sfp_port[0] || i == sfp_port[1]) {
			if (sgmii_fiber) { /* SGMII Fiber mode */
				ppe_port_bridge_txmac_set(i + 1, 1);
				if (i == 4)
					ppe_uniphy_mode_set(0x1, EPORT_WRAPPER_SGMII_FIBER);
				else
					ppe_uniphy_mode_set(0x2, EPORT_WRAPPER_SGMII_FIBER);
				ppe_port_mux_mac_type_set(i + 1, EPORT_WRAPPER_SGMII_FIBER);
			} else if (sfp_mode == EPORT_WRAPPER_10GBASE_R) { /* 10GBASE_R mode */
				if (i == 4)
					ppe_uniphy_mode_set(0x1, EPORT_WRAPPER_10GBASE_R);
				else
					ppe_uniphy_mode_set(0x2, EPORT_WRAPPER_10GBASE_R);
				ppe_port_mux_mac_type_set(i + 1, EPORT_WRAPPER_10GBASE_R);
			} else { /* SGMII Plus Mode */
				ppe_port_bridge_txmac_set(i + 1, 1);
				if (i == 4)
					ppe_uniphy_mode_set(0x1, EPORT_WRAPPER_SGMII_PLUS);
				else if (i == 5)
					ppe_uniphy_mode_set(0x2, EPORT_WRAPPER_SGMII_PLUS);
			}
		}

		ipq9574_speed_clock_set(i, clk);

		ipq9574_port_mac_clock_reset(i);

		if (i == aquantia_port[0] || i == aquantia_port[1])
			ipq9574_uxsgmii_speed_set(i, mac_speed, duplex, status);
		else if ((i == sfp_port[0] || i == sfp_port[1]) && sgmii_fiber == 0)
			ipq9574_10g_r_speed_set(i, status);
		else
			ipq9574_pqsgmii_speed_set(i, mac_speed, status);
	}

	if (linkup <= 0) {
		/* No PHY link is alive */
		return -1;
	}

	pr_info("%s: done\n", __func__);

	return 0;
}

static int ipq9574_edma_wr_macaddr(struct eth_device *dev)
{
	return 0;
}

static void ipq9574_eth_halt(struct eth_device *dev)
{
	pr_info("%s: done\n", __func__);
}

static void ipq9574_edma_set_ring_values(struct ipq9574_edma_hw *edma_hw)
{
	edma_hw->txdesc_ring_start = IPQ9574_EDMA_TX_DESC_RING_START;
	edma_hw->txdesc_rings = IPQ9574_EDMA_TX_DESC_RING_NOS;
	edma_hw->txdesc_ring_end = IPQ9574_EDMA_TX_DESC_RING_SIZE;

	edma_hw->sec_txdesc_ring_start = IPQ9574_EDMA_SEC_TX_DESC_RING_START;
	edma_hw->sec_txdesc_rings = IPQ9574_EDMA_SEC_TX_DESC_RING_NOS;
	edma_hw->sec_txdesc_ring_end = IPQ9574_EDMA_SEC_TX_DESC_RING_SIZE;

	edma_hw->txcmpl_ring_start = IPQ9574_EDMA_TX_CMPL_RING_START;
	edma_hw->txcmpl_rings = IPQ9574_EDMA_RX_FILL_RING_NOS;
	edma_hw->txcmpl_ring_end = IPQ9574_EDMA_TX_CMPL_RING_SIZE;

	edma_hw->rxfill_ring_start = IPQ9574_EDMA_RX_FILL_RING_START;
	edma_hw->rxfill_rings = IPQ9574_EDMA_RX_FILL_RING_NOS;
	edma_hw->rxfill_ring_end = IPQ9574_EDMA_RX_FILL_RING_SIZE;

	edma_hw->rxdesc_ring_start = IPQ9574_EDMA_RX_DESC_RING_START;
	edma_hw->rxdesc_rings = IPQ9574_EDMA_RX_DESC_RING_NOS;
	edma_hw->rxdesc_ring_end = IPQ9574_EDMA_RX_DESC_RING_SIZE;

	edma_hw->sec_rxdesc_ring_start = IPQ9574_EDMA_SEC_RX_DESC_RING_START;
	edma_hw->sec_rxdesc_rings = IPQ9574_EDMA_SEC_RX_DESC_RING_NOS;
	edma_hw->sec_rxdesc_ring_end = IPQ9574_EDMA_SEC_RX_DESC_RING_SIZE;

	pr_info("Num rings - TxDesc:%u (%u-%u) TxCmpl:%u (%u-%u)\n",
		edma_hw->txdesc_rings, edma_hw->txdesc_ring_start,
		(edma_hw->txdesc_ring_start + edma_hw->txdesc_rings - 1),
		edma_hw->txcmpl_rings, edma_hw->txcmpl_ring_start,
		(edma_hw->txcmpl_ring_start + edma_hw->txcmpl_rings - 1));

	pr_info("RxDesc:%u (%u-%u) RxFill:%u (%u-%u)\n",
		edma_hw->rxdesc_rings, edma_hw->rxdesc_ring_start,
		(edma_hw->rxdesc_ring_start + edma_hw->rxdesc_rings - 1),
		edma_hw->rxfill_rings, edma_hw->rxfill_ring_start,
		(edma_hw->rxfill_ring_start + edma_hw->rxfill_rings - 1));
}

/*
 * ipq9574_edma_alloc_rings()
 *	Allocate EDMA software rings
 */
static int ipq9574_edma_alloc_rings(struct ipq9574_edma_hw *ehw)
{
	ehw->rxfill_ring = (void *)noncached_alloc((sizeof(
				struct ipq9574_edma_rxfill_ring) *
				ehw->rxfill_rings),
				CONFIG_SYS_CACHELINE_SIZE);
	if (!ehw->rxfill_ring) {
		pr_info("%s: rxfill_ring alloc error\n", __func__);
		return -ENOMEM;
	}

	ehw->rxdesc_ring = (void *)noncached_alloc((sizeof(
				struct ipq9574_edma_rxdesc_ring) *
				ehw->rxdesc_rings),
				CONFIG_SYS_CACHELINE_SIZE);
	if (!ehw->rxdesc_ring) {
		pr_info("%s: rxdesc_ring alloc error\n", __func__);
		return -ENOMEM;
	}

	ehw->sec_rxdesc_ring = (void *)noncached_alloc((sizeof(
				struct ipq9574_edma_sec_rxdesc_ring) *
				ehw->sec_rxdesc_rings),
				CONFIG_SYS_CACHELINE_SIZE);
	if (!ehw->sec_rxdesc_ring) {
		pr_info("%s: sec_rxdesc_ring alloc error\n", __func__);
		return -ENOMEM;
	}

	ehw->txdesc_ring = (void *)noncached_alloc((sizeof(
				struct ipq9574_edma_txdesc_ring) *
				ehw->txdesc_rings),
				CONFIG_SYS_CACHELINE_SIZE);
	if (!ehw->txdesc_ring) {
		pr_info("%s: txdesc_ring alloc error\n", __func__);
		return -ENOMEM;
	}

	ehw->sec_txdesc_ring = (void *)noncached_alloc((sizeof(
				struct ipq9574_edma_sec_txdesc_ring) *
				ehw->sec_txdesc_rings),
				CONFIG_SYS_CACHELINE_SIZE);
	if (!ehw->sec_txdesc_ring) {
		pr_info("%s: txdesc_ring alloc error\n", __func__);
		return -ENOMEM;
	}

	ehw->txcmpl_ring = (void *)noncached_alloc((sizeof(
				struct ipq9574_edma_txcmpl_ring) *
				ehw->txcmpl_rings),
				CONFIG_SYS_CACHELINE_SIZE);
	if (!ehw->txcmpl_ring) {
		pr_info("%s: txcmpl_ring alloc error\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s: successfull\n", __func__);

	return 0;

}


/*
 * ipq9574_edma_init_rings()
 *	Initialize EDMA rings
 */
static int ipq9574_edma_init_rings(struct ipq9574_edma_hw *ehw)
{
	int ret;

	/*
	 * Setup ring values
	 */
	ipq9574_edma_set_ring_values(ehw);

	/*
	 * Allocate desc rings
	 */
	ret = ipq9574_edma_alloc_rings(ehw);
	if (ret)
		return ret;

	/*
	 * Setup ring resources
	 */
	ret = ipq9574_edma_setup_ring_resources(ehw);
	if (ret)
		return ret;

	return 0;
}

/*
 * ipq9574_edma_configure_sec_txdesc_ring()
 *	Configure one secondary TxDesc ring
 */
static void ipq9574_edma_configure_sec_txdesc_ring(struct ipq9574_edma_hw *ehw,
			  struct ipq9574_edma_sec_txdesc_ring *sec_txdesc_ring)
{
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC_BA2(sec_txdesc_ring->id),
			(uint32_t)(sec_txdesc_ring->dma & 0xffffffff));
}

/*
 * ipq9574_edma_configure_txdesc_ring()
 *	Configure one TxDesc ring
 */
static void ipq9574_edma_configure_txdesc_ring(struct ipq9574_edma_hw *ehw,
					struct ipq9574_edma_txdesc_ring *txdesc_ring)
{
	uint32_t data;
	uint16_t hw_cons_idx;

	/*
	 * Configure TXDESC ring
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC_BA(txdesc_ring->id),
			(uint32_t)(txdesc_ring->dma &
			IPQ9574_EDMA_RING_DMA_MASK));

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC_RING_SIZE(
			txdesc_ring->id), (uint32_t)(txdesc_ring->count &
			IPQ9574_EDMA_TXDESC_RING_SIZE_MASK));

	data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC_CONS_IDX(
				txdesc_ring->id));

	data &= ~(IPQ9574_EDMA_TXDESC_CONS_IDX_MASK);
	hw_cons_idx = data;

	data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC_PROD_IDX(
					txdesc_ring->id));

	data &= ~(IPQ9574_EDMA_TXDESC_PROD_IDX_MASK);
	data |= hw_cons_idx & IPQ9574_EDMA_TXDESC_PROD_IDX_MASK;

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC_PROD_IDX(
					txdesc_ring->id), data);
}

/*
 * ipq9574_edma_configure_txcmpl_ring()
 *	Configure one TxCmpl ring
 */
static void ipq9574_edma_configure_txcmpl_ring(struct ipq9574_edma_hw *ehw,
					struct ipq9574_edma_txcmpl_ring *txcmpl_ring)
{
	/*
	 * Configure TxCmpl ring base address
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXCMPL_BA(txcmpl_ring->id),
			(uint32_t)(txcmpl_ring->dma &
			IPQ9574_EDMA_RING_DMA_MASK));

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXCMPL_RING_SIZE(
			txcmpl_ring->id), (uint32_t)(txcmpl_ring->count &
			IPQ9574_EDMA_TXDESC_RING_SIZE_MASK));

	/*
	 * Set TxCmpl ret mode to opaque
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXCMPL_CTRL(txcmpl_ring->id),
			IPQ9574_EDMA_TXCMPL_RETMODE_OPAQUE);

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TX_INT_CTRL(txcmpl_ring->id),
			0x2);
}


/*
 * ipq9574_edma_configure_sec_rxdesc_ring()
 *	Configure one secondary RxDesc ring
 */
static void ipq9574_edma_configure_sec_rxdesc_ring(struct ipq9574_edma_hw *ehw,
			  struct ipq9574_edma_sec_rxdesc_ring *sec_rxdesc_ring)
{
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_BA2(sec_rxdesc_ring->id),
			(uint32_t)(sec_rxdesc_ring->dma & 0xffffffff));
}

/*
 * ipq9574_edma_configure_rxdesc_ring()
 *	Configure one RxDesc ring
 */
static void ipq9574_edma_configure_rxdesc_ring(struct ipq9574_edma_hw *ehw,
					struct ipq9574_edma_rxdesc_ring *rxdesc_ring)
{
	uint32_t data;

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_BA(rxdesc_ring->id),
			(uint32_t)(rxdesc_ring->dma & 0xffffffff));

	data = rxdesc_ring->count & IPQ9574_EDMA_RXDESC_RING_SIZE_MASK;
	data |= (ehw->rx_payload_offset & IPQ9574_EDMA_RXDESC_PL_OFFSET_MASK) <<
		IPQ9574_EDMA_RXDESC_PL_OFFSET_SHIFT;

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_RING_SIZE(
			rxdesc_ring->id), data);

	/*
	 * Enable ring. Set ret mode to 'opaque'.
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RX_INT_CTRL(rxdesc_ring->id),
			0x2);
}

/*
 * ipq9574_edma_configure_rxfill_ring()
 *	Configure one RxFill ring
 */
static void ipq9574_edma_configure_rxfill_ring(struct ipq9574_edma_hw *ehw,
					struct ipq9574_edma_rxfill_ring *rxfill_ring)
{
	uint32_t data;

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXFILL_BA(rxfill_ring->id),
			(uint32_t)(rxfill_ring->dma & IPQ9574_EDMA_RING_DMA_MASK));

	data = rxfill_ring->count & IPQ9574_EDMA_RXFILL_RING_SIZE_MASK;

	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXFILL_RING_SIZE(rxfill_ring->id), data);
}


/*
 * ipq9574_edma_configure_rings()
 *	Configure EDMA rings
 */
static void ipq9574_edma_configure_rings(struct ipq9574_edma_hw *ehw)
{
	int i;

	/*
	 * Configure TXDESC ring
	 */
	for (i = 0; i < ehw->txdesc_rings; i++)
		ipq9574_edma_configure_txdesc_ring(ehw, &ehw->txdesc_ring[i]);

	/*
	 * Configure secondary TXDESC ring
	 */
	for (i = 0; i < ehw->sec_txdesc_rings; i++)
		ipq9574_edma_configure_sec_txdesc_ring(ehw, &ehw->sec_txdesc_ring[i]);

	/*
	 * Configure TXCMPL ring
	 */
	for (i = 0; i < ehw->txcmpl_rings; i++)
		ipq9574_edma_configure_txcmpl_ring(ehw, &ehw->txcmpl_ring[i]);

	/*
	 * Configure RXFILL rings
	 */
	for (i = 0; i < ehw->rxfill_rings; i++)
		ipq9574_edma_configure_rxfill_ring(ehw, &ehw->rxfill_ring[i]);

	/*
	 * Configure RXDESC ring
	 */
	for (i = 0; i < ehw->rxdesc_rings; i++)
		ipq9574_edma_configure_rxdesc_ring(ehw, &ehw->rxdesc_ring[i]);

	/*
	 * Configure secondary RXDESC ring
	 */
	for (i = 0; i < ehw->rxdesc_rings; i++)
		ipq9574_edma_configure_sec_rxdesc_ring(ehw, &ehw->sec_rxdesc_ring[i]);

	pr_info("%s: successfull\n", __func__);
}

/*
 * ipq9574_edma_hw_reset()
 *	EDMA hw reset
 */
void ipq9574_edma_hw_reset(void)
{
	writel(NSS_CC_EDMA_HW_RESET_ASSERT, NSS_CC_PPE_RESET_ADDR);
	mdelay(500);
	writel(NSS_CC_EDMA_HW_RESET_DEASSERT, NSS_CC_PPE_RESET_ADDR);
	mdelay(100);
}

/*
 * ipq9574_edma_hw_init()
 *	EDMA hw init
 */
int ipq9574_edma_hw_init(struct ipq9574_edma_hw *ehw)
{
	int ret, desc_index;
	uint32_t i, reg;
	volatile uint32_t data;

	struct ipq9574_edma_rxdesc_ring *rxdesc_ring = NULL;

	ipq9574_ppe_provision_init();

	data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_MAS_CTRL);
	printf("EDMA ver %d hw init\n", data);

	/*
	 * Setup private data structure
	 */
	ehw->rxfill_intr_mask = IPQ9574_EDMA_RXFILL_INT_MASK;
	ehw->rxdesc_intr_mask = IPQ9574_EDMA_RXDESC_INT_MASK_PKT_INT;
	ehw->txcmpl_intr_mask = IPQ9574_EDMA_TX_INT_MASK_PKT_INT |
				IPQ9574_EDMA_TX_INT_MASK_UGT_INT;
	ehw->misc_intr_mask = 0xff;
	ehw->rx_payload_offset = 0x0;

	/*
	 * Reset EDMA
	 */
	ipq9574_edma_hw_reset();

	/*
	 * Disable interrupts
	 */
	ipq9574_edma_disable_intr(ehw);

	/*
	 * Disable rings
	 */
	ipq9574_edma_disable_rings(ehw);

	ret = ipq9574_edma_init_rings(ehw);
	if (ret)
		return ret;

	ipq9574_edma_configure_rings(ehw);

	/*
	 * Clear the TXDESC2CMPL_MAP_xx reg before setting up
	 * the mapping. This register holds TXDESC to TXFILL ring
	 * mapping.
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_0, 0);
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_1, 0);
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_2, 0);
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_3, 0);
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_4, 0);
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_5, 0);
	desc_index = ehw->txcmpl_ring_start;

	/*
	 * 6 registers to hold the completion mapping for total 32
	 * TX desc rings (0-5, 6-11, 12-17, 18-23, 24-29 & rest).
	 * In each entry 5 bits hold the mapping for a particular TX desc ring.
	 */
	for (i = ehw->txdesc_ring_start;
		i < ehw->txdesc_ring_end; i++) {
		if ((i >= 0) && (i <= 5))
			reg = IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_0;
		else if ((i >= 6) && (i <= 11))
			reg = IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_1;
		else if ((i >= 12) && (i <= 17))
			reg = IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_2;
		else if ((i >= 18) && (i <= 23))
			reg = IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_3;
		else if ((i >= 24) && (i <= 29))
			reg = IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_4;
		else
			reg = IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_5;

		pr_debug("Configure TXDESC:%u to use TXCMPL:%u\n",
			 i, desc_index);

		/*
		 * Set the Tx complete descriptor ring number in the mapping
		 * register.
		 * E.g. If (txcmpl ring)desc_index = 31, (txdesc ring)i = 28.
		 * 	reg = IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_4
		 * 	data |= (desc_index & 0x1F) << ((i % 6) * 5);
		 * 	data |= (0x1F << 20); - This sets 11111 at 20th bit of
		 * 	register IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_4
		 */

		data = ipq9574_edma_reg_read(reg);
		data |= (desc_index & 0x1F) << ((i % 6) * 5);
		ipq9574_edma_reg_write(reg, data);

		desc_index++;
		if (desc_index == ehw->txcmpl_ring_end)
			desc_index = ehw->txcmpl_ring_start;
	}

	pr_debug("EDMA_REG_TXDESC2CMPL_MAP_0: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_0));
	pr_debug("EDMA_REG_TXDESC2CMPL_MAP_1: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_1));
	pr_debug("EDMA_REG_TXDESC2CMPL_MAP_2: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_2));
	pr_debug("EDMA_REG_TXDESC2CMPL_MAP_3: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_3));
	pr_debug("EDMA_REG_TXDESC2CMPL_MAP_4: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_4));
	pr_debug("EDMA_REG_TXDESC2CMPL_MAP_5: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC2CMPL_MAP_5));

	/*
	 * Set PPE QID to EDMA Rx ring mapping.
	 * When coming up use only queue 0.
	 * HOST EDMA rings.
	 * Each entry can hold mapping for 8 PPE queues and entry size is
	 * 4 bytes
	 */
	desc_index = ehw->rxdesc_ring_start;
	reg = IPQ9574_EDMA_QID2RID_TABLE_MEM(0);
	data = 0;
	data |= (desc_index & 0xF);
	ipq9574_edma_reg_write(reg, data);
	pr_debug("Configure QID2RID reg:0x%x to 0x%x\n", reg, data);

	/*
	 * Set RXDESC2FILL_MAP_xx reg.
	 * There are 3 registers RXDESC2FILL_0, RXDESC2FILL_1 and RXDESC2FILL_2
	 * 3 bits holds the rx fill ring mapping for each of the
	 * rx descriptor ring.
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC2FILL_MAP_0, 0);
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC2FILL_MAP_1, 0);
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC2FILL_MAP_2, 0);

	for (i = ehw->rxdesc_ring_start;
			i < ehw->rxdesc_ring_end; i++) {
		if ((i >= 0) && (i <= 9))
			reg = IPQ9574_EDMA_REG_RXDESC2FILL_MAP_0;
		else if ((i >= 10) && (i <= 19))
			reg = IPQ9574_EDMA_REG_RXDESC2FILL_MAP_1;
		else
			reg = IPQ9574_EDMA_REG_RXDESC2FILL_MAP_2;

		rxdesc_ring = &ehw->rxdesc_ring[i - ehw->rxdesc_ring_start];

		pr_debug("Configure RXDESC:%u to use RXFILL:%u\n",
				rxdesc_ring->id, rxdesc_ring->rxfill->id);

		/*
		 * Set the Rx fill descriptor ring number in the mapping
		 * register.
		 * E.g. If (rxfill ring)rxdesc_ring->rxfill->id = 7, (rxdesc ring)i = 13.
		 * 	reg = IPQ9574_EDMA_REG_RXDESC2FILL_MAP_1
		 * 	data |= (rxdesc_ring->rxfill->id & 0x7) << ((i % 10) * 3);
		 * 	data |= (0x7 << 9); - This sets 111 at 9th bit of
		 * 	register IPQ9574_EDMA_REG_RXDESC2FILL_MAP_1
		 */
		data = ipq9574_edma_reg_read(reg);
		data |= (rxdesc_ring->rxfill->id & 0x7) << ((i % 10) * 3);
		ipq9574_edma_reg_write(reg, data);
	}

	pr_debug("EDMA_REG_RXDESC2FILL_MAP_0: 0x%x\n",
		ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXDESC2FILL_MAP_0));
	pr_debug("EDMA_REG_RXDESC2FILL_MAP_1: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXDESC2FILL_MAP_1));
	pr_debug("EDMA_REG_RXDESC2FILL_MAP_2: 0x%x\n",
		 ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXDESC2FILL_MAP_2));

	/*
	 * Enable EDMA
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_PORT_CTRL,
				 IPQ9574_EDMA_PORT_CTRL_EN);

	/*
	 * Enable Rx rings
	 */
	for (i = ehw->rxdesc_ring_start; i < ehw->rxdesc_ring_end; i++) {
		data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXDESC_CTRL(i));
		data |= IPQ9574_EDMA_RXDESC_RX_EN;
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXDESC_CTRL(i), data);
	}

	for (i = ehw->rxfill_ring_start; i < ehw->rxfill_ring_end; i++) {
		data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_RXFILL_RING_EN(i));
		data |= IPQ9574_EDMA_RXFILL_RING_EN;
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_RXFILL_RING_EN(i), data);
	}

	/*
	 * Enable Tx rings
	 */
	for (i = ehw->txdesc_ring_start; i < ehw->txdesc_ring_end; i++) {
		data = ipq9574_edma_reg_read(IPQ9574_EDMA_REG_TXDESC_CTRL(i));
		data |= IPQ9574_EDMA_TXDESC_TX_EN;
		ipq9574_edma_reg_write(IPQ9574_EDMA_REG_TXDESC_CTRL(i), data);
	}

	/*
	 * Enable MISC interrupt
	 */
	ipq9574_edma_reg_write(IPQ9574_EDMA_REG_MISC_INT_MASK,
				ehw->misc_intr_mask);

	pr_info("%s: successfull\n", __func__);
	return 0;
}

void get_phy_address(int offset)
{
	int phy_type;
	int phy_address;
	int i;

	for (i = 0; i < IPQ9574_PHY_MAX; i++)
		phy_info[i] = ipq9574_alloc_mem(sizeof(phy_info_t));
	i = 0;
	for (offset = fdt_first_subnode(gd->fdt_blob, offset); offset > 0;
	     offset = fdt_next_subnode(gd->fdt_blob, offset)) {
		phy_address = fdtdec_get_uint(gd->fdt_blob,
					      offset, "phy_address", 0);
		phy_type = fdtdec_get_uint(gd->fdt_blob,
					   offset, "phy_type", 0);
		phy_info[i]->phy_address = phy_address;
		phy_info[i++]->phy_type = phy_type;
	}
}

int ipq9574_edma_init(void *edma_board_cfg)
{
	struct eth_device *dev[IPQ9574_EDMA_DEV];
	struct ipq9574_edma_common_info *c_info[IPQ9574_EDMA_DEV];
	struct ipq9574_edma_hw *hw[IPQ9574_EDMA_DEV];
	uchar enet_addr[IPQ9574_EDMA_DEV * 6];
	int i;
	int ret = -1;
	ipq9574_edma_board_cfg_t ledma_cfg, *edma_cfg;
	int phy_id;
	uint32_t phy_chip_id, phy_chip_id1, phy_chip_id2;
	static int sw_init_done = 0;
	int node, phy_addr, aquantia_port[2] = {-1, -1}, aquantia_port_cnt = -1;
	int mode, phy_node = -1, res = -1;

	node = fdt_path_offset(gd->fdt_blob, "/ess-switch");

	if (node >= 0) {
		aquantia_port_cnt = fdtdec_get_uint(gd->fdt_blob, node, "aquantia_port_cnt", -1);

		if (aquantia_port_cnt >= 1) {
			res = fdtdec_get_int_array(gd->fdt_blob, node, "aquantia_port",
					  (u32 *)aquantia_port, aquantia_port_cnt);
			if (res < 0)
				printf("Error: Aquantia port details not provided in DT");
		}
	}

	phy_node = fdt_path_offset(gd->fdt_blob, "/ess-switch/port_phyinfo");
	if (phy_node >= 0)
		get_phy_address(phy_node);

	mode = fdtdec_get_uint(gd->fdt_blob, node, "switch_mac_mode0", -1);
	if (mode < 0) {
		printf("Error:switch_mac_mode0 not specified in dts");
		return mode;
	}

	memset(c_info, 0, (sizeof(c_info) * IPQ9574_EDMA_DEV));
	memset(enet_addr, 0, sizeof(enet_addr));
	memset(&ledma_cfg, 0, sizeof(ledma_cfg));
	edma_cfg = &ledma_cfg;
	strlcpy(edma_cfg->phy_name, "IPQ MDIO0", sizeof(edma_cfg->phy_name));

	/* Getting the MAC address from ART partition */
	ret = get_eth_mac_address(enet_addr, IPQ9574_EDMA_DEV);

	/*
	 * Register EDMA as single ethernet
	 * interface.
	 */
	for (i = 0; i < IPQ9574_EDMA_DEV; edma_cfg++, i++) {
		dev[i] = ipq9574_alloc_mem(sizeof(struct eth_device));

		if (!dev[i])
			goto init_failed;

		memset(dev[i], 0, sizeof(struct eth_device));

		c_info[i] = ipq9574_alloc_mem(
			sizeof(struct ipq9574_edma_common_info));

		if (!c_info[i])
			goto init_failed;

		memset(c_info[i], 0,
			sizeof(struct ipq9574_edma_common_info));

		hw[i] = &c_info[i]->hw;

		c_info[i]->hw.hw_addr = (unsigned long  __iomem *)
						IPQ9574_EDMA_CFG_BASE;

		ipq9574_edma_dev[i] = ipq9574_alloc_mem(
				sizeof(struct ipq9574_eth_dev));

		if (!ipq9574_edma_dev[i])
			goto init_failed;

		memset (ipq9574_edma_dev[i], 0,
			sizeof(struct ipq9574_eth_dev));

		dev[i]->iobase = IPQ9574_EDMA_CFG_BASE;
		dev[i]->init = ipq9574_eth_init;
		dev[i]->halt = ipq9574_eth_halt;
		dev[i]->recv = ipq9574_eth_recv;
		dev[i]->send = ipq9574_eth_snd;
		dev[i]->write_hwaddr = ipq9574_edma_wr_macaddr;
		dev[i]->priv = (void *)ipq9574_edma_dev[i];

		if ((ret < 0) ||
			(!is_valid_ethaddr(&enet_addr[edma_cfg->unit * 6]))) {
			memcpy(&dev[i]->enetaddr[0], ipq9574_def_enetaddr, 6);
		} else {
			memcpy(&dev[i]->enetaddr[0],
				&enet_addr[edma_cfg->unit * 6], 6);
		}

		printf("MAC%x addr:%x:%x:%x:%x:%x:%x\n",
			edma_cfg->unit, dev[i]->enetaddr[0],
			dev[i]->enetaddr[1],
			dev[i]->enetaddr[2],
			dev[i]->enetaddr[3],
			dev[i]->enetaddr[4],
			dev[i]->enetaddr[5]);

		snprintf(dev[i]->name, sizeof(dev[i]->name), "eth%d", i);

		ipq9574_edma_dev[i]->dev  = dev[i];
		ipq9574_edma_dev[i]->mac_unit = edma_cfg->unit;
		ipq9574_edma_dev[i]->c_info = c_info[i];
		ipq9574_edma_hw_addr = IPQ9574_EDMA_CFG_BASE;

		ret = ipq_sw_mdio_init(edma_cfg->phy_name);
		if (ret)
			goto init_failed;

		for (phy_id =  0; phy_id < IPQ9574_PHY_MAX; phy_id++) {
			if (phy_node >= 0) {
				phy_addr = phy_info[phy_id]->phy_address;
			} else {
				printf("Error:Phy addresses not configured in DT\n");
				goto init_failed;
			}

			phy_chip_id1 = ipq_mdio_read(phy_addr, QCA_PHY_ID1, NULL);
			phy_chip_id2 = ipq_mdio_read(phy_addr, QCA_PHY_ID2, NULL);
			phy_chip_id = (phy_chip_id1 << 16) | phy_chip_id2;
			if (phy_id == aquantia_port[0] || phy_id == aquantia_port[1]) {
				phy_chip_id1 = ipq_mdio_read(phy_addr, (1<<30) |(1<<16) | QCA_PHY_ID1, NULL);
				phy_chip_id2 = ipq_mdio_read(phy_addr, (1<<30) |(1<<16) | QCA_PHY_ID2, NULL);
				phy_chip_id = (phy_chip_id1 << 16) | phy_chip_id2;
			}
			pr_debug("phy_id is: 0x%x, phy_addr = 0x%x, phy_chip_id1 = 0x%x, phy_chip_id2 = 0x%x, phy_chip_id = 0x%x\n",
				 phy_id, phy_addr, phy_chip_id1, phy_chip_id2, phy_chip_id);
			switch(phy_chip_id) {
				case QCA8075_PHY_V1_0_5P:
				case QCA8075_PHY_V1_1_5P:
				case QCA8075_PHY_V1_1_2P:
					if (!sw_init_done) {
						if (ipq9574_qca8075_phy_init(&ipq9574_edma_dev[i]->ops[phy_id], phy_addr) == 0) {
							sw_init_done = 1;
						}
					} else {
						ipq9574_qca8075_phy_map_ops(&ipq9574_edma_dev[i]->ops[phy_id]);
					}

					if (mode == EPORT_WRAPPER_PSGMII)
						ipq9574_qca8075_phy_interface_set_mode(phy_addr, 0x0);
					else if (mode == EPORT_WRAPPER_QSGMII)
						ipq9574_qca8075_phy_interface_set_mode(phy_addr, 0x4);
					break;
#ifdef CONFIG_QCA8033_PHY
				case QCA8033_PHY:
					ipq_qca8033_phy_init(&ipq9574_edma_dev[i]->ops[phy_id], phy_addr);
					break;
#endif
#ifdef CONFIG_QCA8081_PHY
				case QCA8081_PHY:
				case QCA8081_1_1_PHY:
					ipq_qca8081_phy_init(&ipq9574_edma_dev[i]->ops[phy_id], phy_addr);
					break;
#endif
#ifdef CONFIG_IPQ9574_QCA_AQUANTIA_PHY
				case AQUANTIA_PHY_107:
				case AQUANTIA_PHY_109:
				case AQUANTIA_PHY_111:
				case AQUANTIA_PHY_111B0:
				case AQUANTIA_PHY_112:
				case AQUANTIA_PHY_112C:
				case AQUANTIA_PHY_113C_A0:
				case AQUANTIA_PHY_113C_A1:
				case AQUANTIA_PHY_113C_B0:
				case AQUANTIA_PHY_113C_B1:
					ipq_board_fw_download(phy_addr);
					mdelay(100);
					ipq_qca_aquantia_phy_init(&ipq9574_edma_dev[i]->ops[phy_id], phy_addr);
					break;
#endif
				default:
					if (phy_info[phy_id]->phy_type != SFP_PHY_TYPE)
						printf("\nphy chip id: 0x%x id not matching for phy id: 0x%x with phy_type: 0x%x and phy address: 0x%x",
							phy_chip_id, phy_id, phy_info[phy_id]->phy_type, phy_info[phy_id]->phy_address);
					break;
			}
		}

		ret = ipq9574_edma_hw_init(hw[i]);

		if (ret)
			goto init_failed;

		eth_register(dev[i]);
	}

	return 0;

init_failed:
	printf("Error in allocating Mem\n");

	for (i = 0; i < IPQ9574_EDMA_DEV; i++) {
		if (dev[i]) {
			eth_unregister(dev[i]);
			ipq9574_free_mem(dev[i]);
		}
		if (c_info[i]) {
			ipq9574_edma_free_desc(c_info[i]);
			ipq9574_edma_free_rings(c_info[i]);
			ipq9574_free_mem(c_info[i]);
		}
		if (ipq9574_edma_dev[i]) {
			ipq9574_free_mem(ipq9574_edma_dev[i]);
		}
	}

	return -1;
}
