/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#ifndef __NSS_DP_HAL_H__
#define __NSS_DP_HAL_H__

#include "nss_dp_dev.h"

/*
 * nss_dp_hal_get_gmac_ops()
 *	Returns gmac hal ops based on the GMAC type.
 */
static inline struct nss_gmac_hal_ops *nss_dp_hal_get_gmac_ops(uint32_t gmac_type)
{
	return dp_global_ctx.gmac_hal_ops[gmac_type];
}

/*
 * nss_dp_hal_set_gmac_ops()
 *	Sets dp global gmac hal ops based on the GMAC type.
 */
static inline void nss_dp_hal_set_gmac_ops(struct nss_gmac_hal_ops *hal_ops, uint32_t gmac_type)
{
	dp_global_ctx.gmac_hal_ops[gmac_type] = hal_ops;
}

/*
 * HAL functions implemented by SoC specific source files.
 */
extern struct rtnl_link_stats64 *nss_dp_hal_get_ndo_stats(
				struct nss_dp_hal_gmac_stats *gmac_stats,
				struct rtnl_link_stats64 *ndo_stats);
extern struct nss_dp_data_plane_ops *nss_dp_hal_get_data_plane_ops(void);
extern bool nss_dp_hal_init(void);
extern void nss_dp_hal_cleanup(void);

#endif	/* __NSS_DP_HAL_H__ */
