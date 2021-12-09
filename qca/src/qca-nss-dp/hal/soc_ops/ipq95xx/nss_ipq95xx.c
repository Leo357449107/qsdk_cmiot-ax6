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

#include <linux/of.h>
#include <nss_dp_arch.h>
#include "nss_dp_hal.h"

/*
 * nss_dp_hal_get_ndo_stats()
 *	Update netdev statistics from data plane statistics
 */
struct rtnl_link_stats64 *nss_dp_hal_get_ndo_stats(
				struct nss_dp_hal_gmac_stats *gmac_stats,
				struct rtnl_link_stats64 *ndo_stats)
{
	ndo_stats->rx_packets = gmac_stats->rx_packets;
	ndo_stats->rx_bytes = gmac_stats->rx_bytes;
	ndo_stats->rx_dropped = gmac_stats->rx_dropped;
	ndo_stats->tx_packets = gmac_stats->tx_packets;
	ndo_stats->tx_bytes = gmac_stats->tx_bytes;
	ndo_stats->tx_dropped = gmac_stats->tx_dropped;
	ndo_stats->tx_errors = gmac_stats->tx_no_desc_avail +
				gmac_stats->tx_non_linear_packets;

	return ndo_stats;
}

/*
 * nss_dp_hal_get_data_plane_ops()
 *	Return the data plane ops for registered data plane.
 */
struct nss_dp_data_plane_ops *nss_dp_hal_get_data_plane_ops(void)
{
	return &nss_dp_edma_ops;
}

/*
 * nss_dp_hal_init()
 *	Initialize EDMA and set gmac ops.
 */
bool nss_dp_hal_init(void)
{
	/*
	 * Bail out on not supported platform
	 */
	if (!of_machine_is_compatible("qcom,ipq9574")) {
		return false;
	}

	if (edma_init()) {
		return false;
	}

	nss_dp_hal_set_gmac_ops(&qcom_gmac_ops, GMAC_HAL_TYPE_QCOM);
	nss_dp_hal_set_gmac_ops(&syn_gmac_ops, GMAC_HAL_TYPE_SYN_XGMAC);

	return true;
}

/*
 * nss_dp_hal_cleanup()
 *	Cleanup EDMA and set gmac ops to NULL.
 */
void nss_dp_hal_cleanup(void)
{
	nss_dp_hal_set_gmac_ops(NULL, GMAC_HAL_TYPE_QCOM);
	nss_dp_hal_set_gmac_ops(NULL, GMAC_HAL_TYPE_SYN_XGMAC);
	edma_cleanup(false);
}
