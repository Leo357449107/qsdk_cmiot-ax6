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

#include <linux/of.h>
#include <linux/ioport.h>
#include <linux/qcom_scm.h>
#include "nss_dp_hal.h"

/*
 * nss_dp_hal_tcsr_base_get()
 *	Reads TCSR base address from DTS
 */
static uint32_t nss_dp_hal_tcsr_base_get(void)
{
	uint32_t tcsr_base_addr = 0;
	struct device_node *dp_cmn;

	/*
	 * Get reference to NSS dp common device node
	 */
	dp_cmn = of_find_node_by_name(NULL, "nss-dp-common");
	if (!dp_cmn) {
		pr_info("%s: NSS DP common node not found\n", __func__);
		return 0;
	}

	if (of_property_read_u32(dp_cmn, "qcom,tcsr-base", &tcsr_base_addr)) {
		pr_err("%s: error reading TCSR base\n", __func__);
	}
	of_node_put(dp_cmn);

	return tcsr_base_addr;
}

/*
 * nss_dp_hal_tcsr_set()
 *	Sets the TCSR axi cache override register
 */
static void nss_dp_hal_tcsr_set(void)
{
	void __iomem *tcsr_addr = NULL;
	uint32_t tcsr_base;
	int err;

	tcsr_base = nss_dp_hal_tcsr_base_get();
	if (!tcsr_base) {
		pr_err("%s: Unable to get TCSR base address\n", __func__);
		return;
	}

	/*
	 * Check if Trust Zone is enabled in the system.
	 * If yes, we need to go through SCM API call to program TCSR register.
	 * If TZ is not enabled, we can write to the register directly.
	 */
	if (qcom_scm_is_available()) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
		err = qcom_scm_tcsr_reg_write((tcsr_base + TCSR_GMAC_AXI_CACHE_OVERRIDE_OFFSET),
						TCSR_GMAC_AXI_CACHE_OVERRIDE_VALUE);
#else
		err = qti_scm_tcsr_reg_write((tcsr_base + TCSR_GMAC_AXI_CACHE_OVERRIDE_OFFSET),
						TCSR_GMAC_AXI_CACHE_OVERRIDE_VALUE);
#endif
		if (err) {
			pr_err("%s: SCM TCSR write error: %d\n", __func__, err);
		}
	} else {
		tcsr_addr = ioremap_nocache((tcsr_base + TCSR_GMAC_AXI_CACHE_OVERRIDE_OFFSET),
						TCSR_GMAC_AXI_CACHE_OVERRIDE_REG_SIZE);
		if (!tcsr_addr) {
			pr_err("%s: ioremap failed\n", __func__);
			return;
		}
		writel(TCSR_GMAC_AXI_CACHE_OVERRIDE_VALUE, tcsr_addr);
		iounmap(tcsr_addr);
	}
}

/*
 * nss_dp_hal_get_ndo_stats()
 *	Update netdev statistics from data plane statistics
 */
struct rtnl_link_stats64 *nss_dp_hal_get_ndo_stats(
				struct nss_dp_hal_gmac_stats *gmac_stats,
				struct rtnl_link_stats64 *ndo_stats)
{
	ndo_stats->rx_packets = gmac_stats->rx_stats.rx_packets;
	ndo_stats->rx_bytes = gmac_stats->rx_stats.rx_bytes;
	ndo_stats->rx_errors = gmac_stats->rx_stats.rx_errors;
	ndo_stats->rx_dropped = gmac_stats->rx_stats.rx_errors;
	ndo_stats->rx_length_errors = gmac_stats->rx_stats.rx_length_errors;
	ndo_stats->rx_frame_errors = gmac_stats->rx_stats.rx_dribble_bit_errors;
	ndo_stats->rx_fifo_errors = gmac_stats->rx_stats.rx_fifo_overflows;
	ndo_stats->rx_missed_errors = gmac_stats->rx_stats.rx_missed;
	ndo_stats->collisions = gmac_stats->tx_stats.tx_collisions + gmac_stats->rx_stats.rx_late_collision_errors;
	ndo_stats->tx_packets = gmac_stats->tx_stats.tx_packets;
	ndo_stats->tx_bytes = gmac_stats->tx_stats.tx_bytes;
	ndo_stats->tx_errors = gmac_stats->tx_stats.tx_errors;
	ndo_stats->tx_dropped = gmac_stats->tx_stats.tx_dropped;
	ndo_stats->tx_carrier_errors = gmac_stats->tx_stats.tx_loss_of_carrier_errors + gmac_stats->tx_stats.tx_no_carrier_errors;
	ndo_stats->tx_fifo_errors = gmac_stats->tx_stats.tx_underflow_errors;
	ndo_stats->tx_window_errors = gmac_stats->tx_stats.tx_late_collision_errors;

	return ndo_stats;
}

/*
 * nss_dp_hal_get_data_plane_ops()
 *	Return the data plane ops for registered data plane.
 */
struct nss_dp_data_plane_ops *nss_dp_hal_get_data_plane_ops(void)
{
	return &nss_dp_gmac_ops;
}

/*
 * nss_dp_hal_init()
 *	Sets the gmac ops based on the GMAC type.
 */
bool nss_dp_hal_init(void)
{
	/*
	 * Bail out on not supported platform
	 */
	if (!of_machine_is_compatible("qcom,ipq5018")) {
		return false;
	}

	nss_dp_hal_set_gmac_ops(&syn_gmac_ops, GMAC_HAL_TYPE_SYN_GMAC);

	/*
	 * Program the global GMAC AXI Cache override register
	 * for optimized AXI DMA operation.
	 */
	nss_dp_hal_tcsr_set();
	return true;
}

/*
 * nss_dp_hal_cleanup()
 *	Sets the gmac ops to NULL.
 */
void nss_dp_hal_cleanup(void)
{
	nss_dp_hal_set_gmac_ops(NULL, GMAC_HAL_TYPE_SYN_GMAC);
}
