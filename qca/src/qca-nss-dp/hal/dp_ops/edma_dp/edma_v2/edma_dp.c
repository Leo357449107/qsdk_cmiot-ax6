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

#include <fal/fal_vsi.h>
#include "edma.h"
#include "edma_cfg_tx.h"
#include "edma_cfg_rx.h"
#include "edma_debug.h"
#include "nss_dp_api_if.h"
#include "nss_dp_dev.h"

/*
 * edma_dp_open()
 *	Do slow path data plane open
 */
static int edma_dp_open(struct nss_dp_data_plane_ctx *dpc,
			uint32_t tx_desc_ring, uint32_t rx_desc_ring,
			uint32_t mode)
{
	if (!dpc->dev) {
		return NSS_DP_FAILURE;
	}

	/*
	 * Enable NAPI
	 */
	if (atomic_read(&edma_gbl_ctx.active_port_count) != 0) {
		atomic_inc(&edma_gbl_ctx.active_port_count);
		return NSS_DP_SUCCESS;
	}
	atomic_inc(&edma_gbl_ctx.active_port_count);

	/*
	 * TODO:
	 * Enable interrupts and NAPI only for the Tx-cmpl rings mapped to this port
	 */
	edma_cfg_tx_napi_enable(&edma_gbl_ctx);
	edma_cfg_rx_napi_enable(&edma_gbl_ctx);

	/*
	 * Enable the interrupt masks.
	 */
	edma_enable_interrupts(&edma_gbl_ctx);

	return NSS_DP_SUCCESS;
}

/*
 * edma_dp_close()
 *	Do slow path data plane close
 */
static int edma_dp_close(struct nss_dp_data_plane_ctx *dpc)
{
	if (!atomic_dec_and_test(&edma_gbl_ctx.active_port_count)) {
		return NSS_DP_SUCCESS;
	}

	/*
	 * Disable the interrupt masks.
	 */
	edma_disable_interrupts(&edma_gbl_ctx);

	/*
	 * TODO:
	 *  Disable interrupts and NAPI only for the Tx-cmpl rings mapped to this port
	 */
	edma_cfg_rx_napi_disable(&edma_gbl_ctx);
	edma_cfg_tx_napi_disable(&edma_gbl_ctx);

	return NSS_DP_SUCCESS;
}

/*
 * edma_dp_link_state()
 *	EDMA data plane link state API
 */
static int edma_dp_link_state(struct nss_dp_data_plane_ctx *dpc,
						uint32_t link_state)
{
	return NSS_DP_SUCCESS;
}

/*
 * edma_dp_mac_addr()
 *	EDMA data plane MAC address change API
 */
static int edma_dp_mac_addr(struct nss_dp_data_plane_ctx *dpc, uint8_t *addr)
{
	return NSS_DP_SUCCESS;
}

/*
 * edma_dp_change_mtu()
 *	EDMA data plane MTU change API
 */
static int edma_dp_change_mtu(struct nss_dp_data_plane_ctx *dpc, uint32_t mtu)
{
	return NSS_DP_SUCCESS;
}

/*
 * edma_dp_xmit()
 *	Transmit a packet using EDMA
 */
static netdev_tx_t edma_dp_xmit(struct nss_dp_data_plane_ctx *dpc,
				struct sk_buff *skb)
{
	struct net_device *netdev = dpc->dev;
	struct edma_txdesc_ring *txdesc_ring;
	struct edma_pcpu_stats *pcpu_stats;
	struct edma_tx_stats *stats;
	struct nss_dp_dev *dp_dev;
	uint32_t skbq;
	int ret;

	/*
	 * Select a TX ring
	 */
	skbq = (skb_get_queue_mapping(skb) & (NR_CPUS - 1));

	dp_dev = (struct nss_dp_dev *)netdev_priv(netdev);
	txdesc_ring = (struct edma_txdesc_ring *)dp_dev->dp_info.txr_map[0][skbq];

	pcpu_stats = &dp_dev->dp_info.pcpu_stats;
	stats = this_cpu_ptr(pcpu_stats->tx_stats);

	/*
	 * Check for non-linear skb
	 */
	if (unlikely(skb_is_nonlinear(skb))) {
		netdev_dbg(netdev, "cannot Tx non-linear skb:%px\n", skb);
		u64_stats_update_begin(&stats->syncp);
		++stats->tx_non_linear_pkts;
		u64_stats_update_end(&stats->syncp);
		goto drop;
	}

	/*
	 * Transmit the packet
	 */
	ret = edma_tx_ring_xmit(netdev, skb, txdesc_ring, stats);
	if (likely(ret == EDMA_TX_OK)) {
		return NETDEV_TX_OK;
	}

drop:
	dev_kfree_skb_any(skb);
	u64_stats_update_begin(&stats->syncp);
	++stats->tx_drops;
	u64_stats_update_end(&stats->syncp);

	return NETDEV_TX_OK;
}

/*
 * edma_dp_set_features()
 *	Set the supported net_device features
 */
static void edma_dp_set_features(struct nss_dp_data_plane_ctx *dpc)
{
	/*
	 * TODO:
	 * Add flags to support HIGHMEM/cksum offload.
	 */
}

/* TODO - check if this is needed */
/*
 * edma_dp_pause_on_off()
 *	Set pause frames on or off
 *
 * No need to send a message if we defaulted to slow path.
 */
static int edma_dp_pause_on_off(struct nss_dp_data_plane_ctx *dpc,
				uint32_t pause_on)
{
	return NSS_DP_SUCCESS;
}

/*
 * edma_dp_get_ndo_stats
 *	Get EDMA data plane stats
 */
static void edma_dp_get_ndo_stats(struct nss_dp_data_plane_ctx *dpc,
					struct nss_dp_gmac_stats *stats)
{
	struct nss_dp_dev *dp_dev = netdev_priv(dpc->dev);
	struct nss_dp_hal_info *dp_info = &dp_dev->dp_info;
	struct edma_rx_stats *pcpu_rx_stats;
	struct edma_tx_stats *pcpu_tx_stats;
	int i;

	memset(&stats->stats, 0, sizeof(struct nss_dp_hal_gmac_stats));

	for_each_possible_cpu(i) {
		struct edma_rx_stats rxp;
		struct edma_tx_stats txp;
		unsigned int start;
		pcpu_rx_stats = per_cpu_ptr(dp_info->pcpu_stats.rx_stats, i);

		do {
			start = u64_stats_fetch_begin_irq(&pcpu_rx_stats->syncp);
			memcpy(&rxp, pcpu_rx_stats, sizeof(*pcpu_rx_stats));
		} while (u64_stats_fetch_retry_irq(&pcpu_rx_stats->syncp, start));

		stats->stats.rx_packets += rxp.rx_pkts;
		stats->stats.rx_bytes += rxp.rx_bytes;
		stats->stats.rx_dropped += rxp.rx_drops;

		pcpu_tx_stats = per_cpu_ptr(dp_info->pcpu_stats.tx_stats, i);

		do {
			start = u64_stats_fetch_begin_irq(&pcpu_tx_stats->syncp);
			memcpy(&txp, pcpu_tx_stats, sizeof(*pcpu_tx_stats));
		} while (u64_stats_fetch_retry_irq(&pcpu_tx_stats->syncp, start));

		stats->stats.tx_packets += txp.tx_pkts;
		stats->stats.tx_bytes += txp.tx_bytes;
		stats->stats.tx_dropped += txp.tx_drops;
		stats->stats.tx_no_desc_avail += txp.tx_no_desc_avail;
		stats->stats.tx_non_linear_packets += txp.tx_non_linear_pkts;
	}
}

#ifdef CONFIG_RFS_ACCEL
/*
 * edma_dp_rx_flow_steer()
 *	Flow steer of the data plane
 *
 * Initial receive flow steering function for data plane operation.
 */
static int edma_dp_rx_flow_steer(struct nss_dp_data_plane_ctx *dpc, struct sk_buff *skb,
					uint32_t cpu, bool is_add)
{
	return NSS_DP_SUCCESS;
}
#endif

/*
 * edma_dp_deinit()
 *	Free edma resources
 */
static int edma_dp_deinit(struct nss_dp_data_plane_ctx *dpc)
{
	struct net_device *netdev = dpc->dev;
	struct nss_dp_dev *dp_dev = (struct nss_dp_dev *)netdev_priv(netdev);

	free_percpu(dp_dev->dp_info.pcpu_stats.rx_stats);
	free_percpu(dp_dev->dp_info.pcpu_stats.tx_stats);

	/*
	 * Free up resources used by EDMA if all the
	 * interfaces have been overridden
	 * */
	if (edma_gbl_ctx.dp_override_cnt == EDMA_MAX_GMACS - 1) {
		edma_cleanup(true);
	} else {
		edma_gbl_ctx.dp_override_cnt++;
	}

	return NSS_DP_SUCCESS;
}

/*
 * edma_dp_configure()
 *	API to configure data plane
 */
static int edma_dp_configure(struct net_device *netdev, uint32_t macid)
{
	if (!netdev) {
		edma_err("nss_dp_edma: Invalid netdev pointer %px\n", netdev);
		return -EINVAL;
	}

	if ((macid < EDMA_START_GMACS) || (macid > EDMA_MAX_GMACS)) {
		edma_err("nss_dp_edma: Invalid macid(%d) for %s\n",
			macid, netdev->name);
		return -EINVAL;
	}

	edma_info("nss_dp_edma: Registering netdev %s(qcom-id:%d) with EDMA\n",
		netdev->name, macid);

	/*
	 * We expect 'macid' to correspond to ports numbers on
	 * IPQ95xx. These begin from '1' and hence we subtract
	 * one when using it as an array index.
	 */
	edma_gbl_ctx.netdev_arr[macid - 1] = netdev;

	edma_cfg_tx_fill_per_port_tx_map(netdev, macid);

	if (edma_gbl_ctx.napi_added) {
		return 0;
	}

	/*
	 * TX/RX NAPI addition
	 */
	edma_cfg_rx_napi_add(&edma_gbl_ctx, netdev);
	edma_cfg_tx_napi_add(&edma_gbl_ctx, netdev);

	/*
	 * Register the interrupt handlers
	 */
	if (edma_irq_init() < 0) {
		edma_cfg_rx_napi_delete(&edma_gbl_ctx);
		edma_cfg_tx_napi_delete(&edma_gbl_ctx);
		return -EINVAL;
	}

	edma_gbl_ctx.napi_added = true;
	return 0;
}

/*
 * edma_dp_init()
 *	EDMA data plane init function
 */
static int edma_dp_init(struct nss_dp_data_plane_ctx *dpc)
{
	struct net_device *netdev = dpc->dev;
	struct nss_dp_dev *dp_dev = (struct nss_dp_dev *)netdev_priv(netdev);
	int ret = 0;

	/*
	 * Allocate per-cpu stats memory
	 */
	dp_dev->dp_info.pcpu_stats.rx_stats =
		netdev_alloc_pcpu_stats(struct edma_rx_stats);
	if (!dp_dev->dp_info.pcpu_stats.rx_stats) {
		netdev_err(netdev, "Percpu EDMA Rx stats alloc failed for %s\n",
				netdev->name);
		return NSS_DP_FAILURE;
	}

	dp_dev->dp_info.pcpu_stats.tx_stats =
		netdev_alloc_pcpu_stats(struct edma_tx_stats);
	if (!dp_dev->dp_info.pcpu_stats.tx_stats) {
		netdev_err(netdev, "Percpu EDMA Tx stats alloc failed for %s\n",
				netdev->name);
		free_percpu(dp_dev->dp_info.pcpu_stats.rx_stats);
		return NSS_DP_FAILURE;
	}

	/*
	 * Configure the data plane
	 */
	ret = edma_dp_configure(netdev, dp_dev->macid);
	if (ret) {
		netdev_dbg(netdev, "Error configuring the data plane %s\n",
				netdev->name);
		free_percpu(dp_dev->dp_info.pcpu_stats.rx_stats);
		free_percpu(dp_dev->dp_info.pcpu_stats.tx_stats);
		return NSS_DP_FAILURE;
	}

	return NSS_DP_SUCCESS;
}

/*
 * nss_dp_edma_ops
 *	EDMA data plane operations
 */
struct nss_dp_data_plane_ops nss_dp_edma_ops = {
	.init		= edma_dp_init,
	.open		= edma_dp_open,
	.close		= edma_dp_close,
	.link_state	= edma_dp_link_state,
	.mac_addr	= edma_dp_mac_addr,
	.change_mtu	= edma_dp_change_mtu,
	.xmit		= edma_dp_xmit,
	.set_features	= edma_dp_set_features,
	.pause_on_off	= edma_dp_pause_on_off,
	.get_stats	= edma_dp_get_ndo_stats,
#ifdef CONFIG_RFS_ACCEL
	.rx_flow_steer	= edma_dp_rx_flow_steer,
#endif
	.deinit		= edma_dp_deinit,
};
