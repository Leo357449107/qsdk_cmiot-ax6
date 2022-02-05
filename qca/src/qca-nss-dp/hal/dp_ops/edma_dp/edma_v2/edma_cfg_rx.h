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

#ifndef __EDMA_CFG_RX_H__
#define __EDMA_CFG_RX_H__

#define EDMA_RX_NAPI_WORK		32
#define EDMA_RX_DEFAULT_QUEUE_PRI	0
#define EDMA_RX_FC_ENABLE		0	/* RX flow control default state */
#define EDMA_RX_FC_XOFF_THRE_MIN	0	/* Rx flow control minimum X-OFF value */
#define EDMA_RX_FC_XON_THRE_MIN		0	/* Rx flow control mininum X-ON value */
#define EDMA_RX_AC_FC_THRE_ORIG		0x190	/* Rx AC flow control original threshold */
#define EDMA_RX_AC_FC_THRE_MIN		0	/* Rx AC flow control minimum threshold */
#define EDMA_RX_AC_FC_THRE_MAX		0x7ff	/* Rx AC flow control maximum threshold.
						   AC FC threshold value is 11 bits long */

extern uint32_t edma_cfg_rx_fc_enable;

void edma_cfg_rx_rings(struct edma_gbl_ctx *egc);
int32_t edma_cfg_rx_rings_alloc(struct edma_gbl_ctx *egc);
void edma_cfg_rx_rings_cleanup(struct edma_gbl_ctx *egc);
void edma_cfg_rx_napi_disable(struct edma_gbl_ctx *egc);
void edma_cfg_rx_napi_enable(struct edma_gbl_ctx *egc);
void edma_cfg_rx_napi_delete(struct edma_gbl_ctx *egc);
void edma_cfg_rx_napi_add(struct edma_gbl_ctx *egc, struct net_device *netdev);
void edma_cfg_rx_mapping(struct edma_gbl_ctx *egc);
void edma_cfg_rx_rings_enable(struct edma_gbl_ctx *egc);
void edma_cfg_rx_rings_disable(struct edma_gbl_ctx *egc);
int edma_cfg_rx_fc_enable_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos);
#endif	/* __EDMA_CFG_RX_H__ */
