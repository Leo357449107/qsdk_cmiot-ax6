/*
 * Copyright 2014 Broadcom Corporation.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _BCM_SF2_ETH_H_
#define _BCM_SF2_ETH_H_

#include <phy.h>

#define RX_BUF_SIZE	2048
/* RX_BUF_NUM must be power of 2 */
#define RX_BUF_NUM	32

#define TX_BUF_SIZE	2048
/* TX_BUF_NUM must be power of 2 */
#define TX_BUF_NUM	2

/* Support 2 Ethernet ports now */
#define BCM_ETH_MAX_PORT_NUM	2

#define CONFIG_BCM_SF2_ETH_DEFAULT_PORT	0

enum {
	MAC_DMA_TX = 1,
	MAC_DMA_RX = 2
};

struct eth_dma {
	void *tx_desc_aligned;
	void *rx_desc_aligned;
	void *tx_desc;
	void *rx_desc;

	uint8_t *tx_buf;
	uint8_t *rx_buf;

	int cur_tx_index;
	int cur_rx_index;

	int (*tx_packet)(struct eth_dma *dma, void *packet, int length);
	bool (*check_tx_done)(struct eth_dma *dma);

	int (*check_rx_done)(struct eth_dma *dma, uint8_t *buf);

	int (*enable_dma)(struct eth_dma *dma, int dir);
	int (*disable_dma)(struct eth_dma *dma, int dir);
};

struct eth_info {
	struct eth_dma dma;
	phy_interface_t phy_interface;
	struct phy_device *port[BCM_ETH_MAX_PORT_NUM];
	int port_num;

	int (*miiphy_read)(const char *devname, unsigned char phyaddr,
			   unsigned char reg, unsigned short *value);
	int (*miiphy_write)(const char *devname, unsigned char phyaddr,
			    unsigned char reg, unsigned short value);

	int (*mac_init)(struct eth_device *dev);
	int (*enable_mac)(void);
	int (*disable_mac)(void);
	int (*set_mac_addr)(unsigned char *mac);
	int (*set_mac_speed)(int speed, int duplex);

};

#endif /* _BCM_SF2_ETH_H_ */
