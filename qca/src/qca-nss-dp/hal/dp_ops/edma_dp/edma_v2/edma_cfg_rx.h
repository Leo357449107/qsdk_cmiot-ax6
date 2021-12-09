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

#ifndef __EDMA_CFG_RX_H__
#define __EDMA_CFG_RX_H__

#define EDMA_RX_NAPI_WORK		32

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

#endif	/* __EDMA_CFG_RX_H__ */
