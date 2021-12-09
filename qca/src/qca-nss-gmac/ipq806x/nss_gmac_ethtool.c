/*
 * Copyright (c) 2013-2019, 2021 The Linux Foundation. All rights reserved.
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
/*
 * @file
 * This is the network dependent layer to handle ethtool related functionality.
 * This file is tightly coupled to neworking frame work of linux kernel.
 * The functionality carried out in this file should be treated as an
 * example only if the underlying operating system is not Linux.
 *-----------------------------REVISION HISTORY--------------------------------
 * Qualcomm Atheros			01/Mar/2010			Created
 */

#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/phy.h>

#ifdef CONFIG_OF
#include <msm_nss_macsec.h>
#else
#include <mach/msm_nss_macsec.h>
#endif

#include <nss_gmac_dev.h>
#include <nss_gmac_network_interface.h>

struct nss_gmac_ethtool_stats {
	uint8_t stat_string[ETH_GSTRING_LEN];
	uint32_t stat_offset;
};

#define DRVINFO_LEN		32
#define NSS_GMAC_STAT(m)	offsetof(struct nss_gmac_stats, m)
#define NSS_GMAC_HOST_STAT(m)	offsetof(struct nss_gmac_host_stats, m)
#define HW_ERR_SIZE		sizeof(uint64_t)

/**
 * @brief Array of strings describing statistics
 */
static const struct nss_gmac_ethtool_stats gmac_gstrings_stats[] = {
	{"rx_bytes", NSS_GMAC_STAT(rx_bytes)},
	{"rx_packets", NSS_GMAC_STAT(rx_packets)},
	{"rx_errors", NSS_GMAC_STAT(rx_errors)},
	{"rx_receive_errors", NSS_GMAC_STAT(rx_receive_errors)},
	{"rx_descriptor_errors", NSS_GMAC_STAT(rx_descriptor_errors)},
	{"rx_late_collision_errors", NSS_GMAC_STAT(rx_late_collision_errors)},
	{"rx_dribble_bit_errors", NSS_GMAC_STAT(rx_dribble_bit_errors)},
	{"rx_length_errors", NSS_GMAC_STAT(rx_length_errors)},
	{"rx_ip_header_errors", NSS_GMAC_STAT(rx_ip_header_errors)},
	{"rx_ip_payload_errors", NSS_GMAC_STAT(rx_ip_payload_errors)},
	{"rx_no_buffer_errors", NSS_GMAC_STAT(rx_no_buffer_errors)},
	{"rx_transport_csum_bypassed", NSS_GMAC_STAT(rx_transport_csum_bypassed)},
	{"tx_bytes", NSS_GMAC_STAT(tx_bytes)},
	{"tx_packets", NSS_GMAC_STAT(tx_packets)},
	{"tx_collisions", NSS_GMAC_STAT(tx_collisions)},
	{"tx_errors", NSS_GMAC_STAT(tx_errors)},
	{"tx_jabber_timeout_errors", NSS_GMAC_STAT(tx_jabber_timeout_errors)},
	{"tx_frame_flushed_errors", NSS_GMAC_STAT(tx_frame_flushed_errors)},
	{"tx_loss_of_carrier_errors", NSS_GMAC_STAT(tx_loss_of_carrier_errors)},
	{"tx_no_carrier_errors", NSS_GMAC_STAT(tx_no_carrier_errors)},
	{"tx_late_collision_errors", NSS_GMAC_STAT(tx_late_collision_errors)},
	{"tx_excessive_collision_errors", NSS_GMAC_STAT(tx_excessive_collision_errors)},
	{"tx_excessive_deferral_errors", NSS_GMAC_STAT(tx_excessive_deferral_errors)},
	{"tx_underflow_errors", NSS_GMAC_STAT(tx_underflow_errors)},
	{"tx_ip_header_errors", NSS_GMAC_STAT(tx_ip_header_errors)},
	{"tx_ip_payload_errors", NSS_GMAC_STAT(tx_ip_payload_errors)},
	{"tx_dropped", NSS_GMAC_STAT(tx_dropped)},
	{"rx_missed", NSS_GMAC_STAT(rx_missed)},
	{"fifo_overflows", NSS_GMAC_STAT(fifo_overflows)},
	{"rx_scatter_errors", NSS_GMAC_STAT(rx_scatter_errors)},
	{"tx_ts_create_errors", NSS_GMAC_STAT(tx_ts_create_errors)},
	{"pmt_interrupts", NSS_GMAC_STAT(hw_errs[0])},
	{"mmc_interrupts", NSS_GMAC_STAT(hw_errs[0]) + (1 * HW_ERR_SIZE)},
	{"line_interface_interrupts", NSS_GMAC_STAT(hw_errs[0]) + (2 * HW_ERR_SIZE)},
	{"fatal_bus_error_interrupts", NSS_GMAC_STAT(hw_errs[0]) + (3 * HW_ERR_SIZE)},
	{"rx_buffer_unavailable_interrupts", NSS_GMAC_STAT(hw_errs[0]) + (4 * HW_ERR_SIZE)},
	{"rx_process_stopped_interrupts", NSS_GMAC_STAT(hw_errs[0]) + (5 * HW_ERR_SIZE)},
	{"tx_underflow_interrupts", NSS_GMAC_STAT(hw_errs[0]) + (6 * HW_ERR_SIZE)},
	{"rx_overflow_interrupts", NSS_GMAC_STAT(hw_errs[0]) + (7 * HW_ERR_SIZE)},
	{"tx_jabber_timeout_interrutps", NSS_GMAC_STAT(hw_errs[0]) + (8 * HW_ERR_SIZE)},
	{"tx_process_stopped_interrutps", NSS_GMAC_STAT(hw_errs[0]) + (9 * HW_ERR_SIZE)},
	{"gmac_total_ticks", NSS_GMAC_STAT(gmac_total_ticks)},
	{"gmac_worst_case_ticks", NSS_GMAC_STAT(gmac_worst_case_ticks)},
	{"gmac_iterations", NSS_GMAC_STAT(gmac_iterations)},
	{"tx_pause_frames", NSS_GMAC_STAT(tx_pause_frames)},
	{"mmc_rx_overflow_errors", NSS_GMAC_STAT(mmc_rx_overflow_errors)},
	{"mmc_rx_watchdog_timeout_errors", NSS_GMAC_STAT(mmc_rx_watchdog_timeout_errors)},
	{"mmc_rx_crc_errors", NSS_GMAC_STAT(mmc_rx_crc_errors)},
	{"mmc_rx_ip_header_errors", NSS_GMAC_STAT(mmc_rx_ip_header_errors)},
	{"mmc_rx_octets_g", NSS_GMAC_STAT(mmc_rx_octets_g)},
	{"mmc_rx_ucast_frames", NSS_GMAC_STAT(mmc_rx_ucast_frames)},
	{"mmc_rx_bcast_frames", NSS_GMAC_STAT(mmc_rx_bcast_frames)},
	{"mmc_rx_mcast_frames", NSS_GMAC_STAT(mmc_rx_mcast_frames)},
	{"mmc_rx_undersize", NSS_GMAC_STAT(mmc_rx_undersize)},
	{"mmc_rx_oversize", NSS_GMAC_STAT(mmc_rx_oversize)},
	{"mmc_rx_jabber", NSS_GMAC_STAT(mmc_rx_jabber)},
	{"mmc_rx_octets_gb", NSS_GMAC_STAT(mmc_rx_octets_gb)},
	{"mmc_rx_frag_frames_g", NSS_GMAC_STAT(mmc_rx_frag_frames_g)},
	{"mmc_tx_octets_g", NSS_GMAC_STAT(mmc_tx_octets_g)},
	{"mmc_tx_ucast_frames", NSS_GMAC_STAT(mmc_tx_ucast_frames)},
	{"mmc_tx_bcast_frames", NSS_GMAC_STAT(mmc_tx_bcast_frames)},
	{"mmc_tx_mcast_frames", NSS_GMAC_STAT(mmc_tx_mcast_frames)},
	{"mmc_tx_deferred", NSS_GMAC_STAT(mmc_tx_deferred)},
	{"mmc_tx_single_col", NSS_GMAC_STAT(mmc_tx_single_col)},
	{"mmc_tx_multiple_col", NSS_GMAC_STAT(mmc_tx_multiple_col)},
	{"mmc_tx_octets_gb", NSS_GMAC_STAT(mmc_tx_octets_gb)},
};

/**
 * @brief Array of strings describing NSS Host statistics
 */
static const struct nss_gmac_ethtool_stats gmac_gstrings_host_stats[] = {
	{"rx_prec0", NSS_GMAC_HOST_STAT(rx_per_prec[0])},
	{"rx_prec1", NSS_GMAC_HOST_STAT(rx_per_prec[1])},
	{"rx_prec2", NSS_GMAC_HOST_STAT(rx_per_prec[2])},
	{"rx_prec3", NSS_GMAC_HOST_STAT(rx_per_prec[3])},
	{"rx_prec4", NSS_GMAC_HOST_STAT(rx_per_prec[4])},
	{"rx_prec5", NSS_GMAC_HOST_STAT(rx_per_prec[5])},
	{"rx_prec6", NSS_GMAC_HOST_STAT(rx_per_prec[6])},
	{"rx_prec7", NSS_GMAC_HOST_STAT(rx_per_prec[7])},
	{"tx_prec0", NSS_GMAC_HOST_STAT(tx_per_prec[0])},
	{"tx_prec1", NSS_GMAC_HOST_STAT(tx_per_prec[1])},
	{"tx_prec2", NSS_GMAC_HOST_STAT(tx_per_prec[2])},
	{"tx_prec3", NSS_GMAC_HOST_STAT(tx_per_prec[3])},
	{"tx_prec4", NSS_GMAC_HOST_STAT(tx_per_prec[4])},
	{"tx_prec5", NSS_GMAC_HOST_STAT(tx_per_prec[5])},
	{"tx_prec6", NSS_GMAC_HOST_STAT(tx_per_prec[6])},
	{"tx_prec7", NSS_GMAC_HOST_STAT(tx_per_prec[7])},
};

/**
 * @brief Array of strings describing private flag names
 */
static const char *gmac_strings_priv_flags[] = {
	"linkpoll",
	"tstamp",
	"ignore_rx_csum_err",
};

#define NSS_GMAC_STATS_LEN	ARRAY_SIZE(gmac_gstrings_stats)
#define NSS_GMAC_HOST_STATS_LEN	ARRAY_SIZE(gmac_gstrings_host_stats)
#define NSS_GMAC_PRIV_FLAGS_LEN	ARRAY_SIZE(gmac_strings_priv_flags)

/**
 * @brief Get number of strings that describe requested objects.
 * @param[in] pointer to struct net_device.
 * @param[in] string set to get
 */
static int32_t nss_gmac_get_strset_count(struct net_device *netdev,
					 int32_t sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return NSS_GMAC_STATS_LEN + NSS_GMAC_HOST_STATS_LEN;

	case ETH_SS_PRIV_FLAGS:
		return NSS_GMAC_PRIV_FLAGS_LEN;
		break;

	default:
		netdev_dbg(netdev, "%s: Invalid string set\n", __func__);
		return -EOPNOTSUPP;
	}
}

/**
 * @brief Get strings that describe requested objects
 * @param[in] pointer to struct net_device.
 * @param[in] string set to get
 * @param[out] pointer to buffer
 */
static void nss_gmac_get_strings(struct net_device *netdev, uint32_t stringset,
			  uint8_t *data)
{
	uint8_t *p = data;
	uint32_t i;

	switch (stringset) {
	case ETH_SS_STATS:
		for (i = 0; i < NSS_GMAC_STATS_LEN; i++) {
			memcpy(p, gmac_gstrings_stats[i].stat_string,
				ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (i = 0; i < NSS_GMAC_HOST_STATS_LEN; i++) {
			memcpy(p, gmac_gstrings_host_stats[i].stat_string,
				ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		break;

	case ETH_SS_PRIV_FLAGS:
		for (i = 0; i < NSS_GMAC_PRIV_FLAGS_LEN; i++) {
			memcpy(p, gmac_strings_priv_flags[i],
				ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}

		break;
	}
}

/**
 * @brief Get statistics
 * @param[in] pointer to struct net_device.
 * @param[in] string set to get
 * @param[out] pointer to buffer
 */
static void nss_gmac_get_ethtool_stats(struct net_device *netdev,
				struct ethtool_stats *stats, uint64_t *data)
{
	struct nss_gmac_dev *gmacdev = netdev_priv(netdev);
	int32_t i, j;
	uint8_t *p = NULL;

	if (!gmacdev->data_plane_ops)
		return;

	spin_lock_bh(&gmacdev->stats_lock);
	gmacdev->data_plane_ops->get_stats(gmacdev->data_plane_ctx, &gmacdev->nss_stats);

	/* Update NSS FW stats */
	for (i = 0; i < NSS_GMAC_STATS_LEN; i++) {
		p = (uint8_t *)&(gmacdev->nss_stats) +
					gmac_gstrings_stats[i].stat_offset;
		data[i] = *(uint64_t *)p;
	}

	/* Update NSS GMAC Host stats */
	for (i = NSS_GMAC_STATS_LEN, j = 0; i < NSS_GMAC_STATS_LEN + NSS_GMAC_HOST_STATS_LEN; i++) {
		p = (uint8_t *)&(gmacdev->nss_host_stats) +
					gmac_gstrings_host_stats[j++].stat_offset;
		data[i] = *(uint64_t *)p;
	}
	spin_unlock_bh(&gmacdev->stats_lock);
}

/**
 * @brief Return driver information.
 *	  Note: Fields are 32 bytes in length.
 * @param[in] pointer to struct net_device.
 * @param[out] pointer to struct ethtool_drvinfo
 */
static void nss_gmac_get_drvinfo(struct net_device *dev,
						struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, nss_gmac_driver_string, sizeof(info->driver));
	strlcpy(info->version, nss_gmac_driver_version, sizeof(info->version));
	strlcpy(info->bus_info, "NSS", ETHTOOL_BUSINFO_LEN);
	info->n_priv_flags = __NSS_GMAC_PRIV_FLAG_MAX;
}

/**
 * @brief Return pause parameters.
 * @param[in] pointer to struct net_device.
 * @param[in] pointer to ethtool_pauseparam structure.
 */
static void nss_gmac_get_pauseparam(struct net_device *netdev,
			     struct ethtool_pauseparam *pause)
{
	struct nss_gmac_dev *gmacdev = (struct nss_gmac_dev *)netdev_priv(netdev);
	BUG_ON(gmacdev == NULL);
	BUG_ON(gmacdev->netdev != netdev);

	pause->rx_pause = gmacdev->pause & FLOW_CTRL_RX ? 1 : 0;
	pause->tx_pause = gmacdev->pause & FLOW_CTRL_TX ? 1 : 0;

	pause->autoneg = AUTONEG_ENABLE;
}

/**
 * @brief Return pause parameters.
 * @param[in] pointer to struct net_device.
 * @param[in] pointer to ethtool_pauseparam structure.
 */
static int nss_gmac_set_pauseparam(struct net_device *netdev,
			    struct ethtool_pauseparam *pause)
{
	struct nss_gmac_dev *gmacdev = (struct nss_gmac_dev *)netdev_priv(netdev);
	struct phy_device *phydev;

	BUG_ON(gmacdev == NULL);
	BUG_ON(gmacdev->netdev != netdev);

	/* set flow control settings */
	gmacdev->pause = 0;
	if (pause->rx_pause)
		gmacdev->pause |= FLOW_CTRL_RX;

	if (pause->tx_pause)
		gmacdev->pause |= FLOW_CTRL_TX;

	/*
	 * If the link polling for this GMAC is disabled, we do not
	 * attempt to make changes to the PHY settings.
	 */
	if (!test_bit(__NSS_GMAC_LINKPOLL, &gmacdev->flags)) {
		if (gmacdev->forced_speed != SPEED_UNKNOWN) {
			if (gmacdev->pause & FLOW_CTRL_TX)
				nss_gmac_tx_pause_enable(gmacdev);
			else
				nss_gmac_tx_pause_disable(gmacdev);

			if (gmacdev->pause & FLOW_CTRL_RX)
				nss_gmac_rx_pause_enable(gmacdev);
			else
				nss_gmac_rx_pause_disable(gmacdev);
		}

		return 0;
	}

	phydev = gmacdev->phydev;

	/* Update flow control advertisment */
	phydev->advertising &= ~(ADVERTISED_Pause | ADVERTISED_Asym_Pause);

	if (gmacdev->pause & FLOW_CTRL_RX)
		phydev->advertising |=
				(ADVERTISED_Pause | ADVERTISED_Asym_Pause);

	if (gmacdev->pause & FLOW_CTRL_TX)
		phydev->advertising |= ADVERTISED_Asym_Pause;

	genphy_config_aneg(gmacdev->phydev);

	return 0;
}

/**
 * @brief Restart autonegotiation
 * @param[in] pointer to struct net_device.
 */
static int nss_gmac_nway_reset(struct net_device *netdev)
{
	struct nss_gmac_dev *gmacdev = (struct nss_gmac_dev *)netdev_priv(netdev);
	BUG_ON(gmacdev == NULL);

	if (!netif_running(netdev))
		return -EAGAIN;

	/*
	 * If the link polling for this GMAC is disabled, we probably
	 * do not have a PHY attached.
	 */
	if (!test_bit(__NSS_GMAC_LINKPOLL, &gmacdev->flags))
		return -EINVAL;

	if (!test_bit(__NSS_GMAC_AUTONEG, &gmacdev->flags))
		return -EINVAL;

	genphy_restart_aneg(gmacdev->phydev);

	return 0;
}

/**
 * @brief Get Wake On Lan settings
 * @param[in] pointer to struct net_device.
 * @param[in] pointer to struct ethtool_wolinfo.
 */
static void nss_gmac_get_wol(struct net_device *netdev,
			     struct ethtool_wolinfo *wol)
{
	wol->supported = 0;
	wol->wolopts = 0;
}

/**
 * @brief Get message level
 * @param[in] pointer to struct net_device.
 */
static uint32_t nss_gmac_get_msglevel(struct net_device *netdev)
{
	return 0;
}

/**
 * @brief Get Settings
 * @param[in] pointer to struct net_device.
 * @param[in] pointer to struct ethtool_cmd.
 */
static int32_t nss_gmac_get_settings(struct net_device *netdev,
			      struct ethtool_cmd *ecmd)
{
	struct nss_gmac_dev *gmacdev = (struct nss_gmac_dev *)netdev_priv(netdev);
	struct phy_device *phydev = NULL;
	uint16_t phyreg;

	BUG_ON(gmacdev == NULL);

	/*
	 * If the speed/duplex for this GMAC is forced and we are not
	 * polling for link state changes, return the values as specified by
	 * platform. This will be true for GMACs connected to switch, and
	 * interfaces that do not use a PHY.
	 */
	if (!test_bit(__NSS_GMAC_LINKPOLL, &gmacdev->flags)) {
		if (gmacdev->forced_speed != SPEED_UNKNOWN) {
			ethtool_cmd_speed_set(ecmd, gmacdev->forced_speed);
			ecmd->duplex = gmacdev->forced_duplex;
			ecmd->mdio_support = 0;
			ecmd->lp_advertising = 0;
			return 0;
		} else {
			/* Non-link polled interfaced must have a forced
			 * speed/duplex
			 */
			return -EIO;
		}
	}

	phydev = gmacdev->phydev;

	/* update PHY status */
	if (phydev->is_c45 == true) {
		ecmd->mdio_support = ETH_MDIO_SUPPORTS_C45;
	} else {
		if (genphy_read_status(phydev) != 0) {
			return -EIO;
		}
		ecmd->mdio_support = ETH_MDIO_SUPPORTS_C22;
	}

	/* Populate capabilities advertised by self */
	ecmd->advertising = phydev->advertising;

	ecmd->autoneg = phydev->autoneg;
	ethtool_cmd_speed_set(ecmd, phydev->speed);
	ecmd->duplex = phydev->duplex;
	ecmd->port = PORT_TP;
	ecmd->phy_address = gmacdev->phy_base;
	ecmd->transceiver = XCVR_EXTERNAL;

	/* Populate supported capabilities */
	ecmd->supported = phydev->supported;

	if (phydev->is_c45 == true)
		return 0;

	/* Populate capabilities advertised by link partner */
	phyreg = nss_gmac_mii_rd_reg(gmacdev, gmacdev->phy_base, MII_LPA);
	if (phyreg & LPA_10HALF)
		ecmd->lp_advertising |= ADVERTISED_10baseT_Half;

	if (phyreg & LPA_10FULL)
		ecmd->lp_advertising |= ADVERTISED_10baseT_Full;

	if (phyreg & LPA_100HALF)
		ecmd->lp_advertising |= ADVERTISED_100baseT_Half;

	if (phyreg & LPA_100FULL)
		ecmd->lp_advertising |= ADVERTISED_100baseT_Full;

	if (phyreg & LPA_PAUSE_CAP)
		ecmd->lp_advertising |= ADVERTISED_Pause;

	if (phyreg & LPA_PAUSE_ASYM)
		ecmd->lp_advertising |= ADVERTISED_Asym_Pause;

	phyreg = nss_gmac_mii_rd_reg(gmacdev, gmacdev->phy_base, MII_STAT1000);
	if (phyreg & LPA_1000HALF)
		ecmd->lp_advertising |= ADVERTISED_1000baseT_Half;

	if (phyreg & LPA_1000FULL)
		ecmd->lp_advertising |= ADVERTISED_1000baseT_Full;

	return 0;
}

/**
 * @brief Set Settings
 * @param[in] pointer to struct net_device.
 * @param[in] pointer to struct ethtool_cmd.
 */
static int32_t nss_gmac_set_settings(struct net_device *netdev,
			      struct ethtool_cmd *ecmd)
{
	struct nss_gmac_dev *gmacdev = (struct nss_gmac_dev *)netdev_priv(netdev);
	struct phy_device *phydev = NULL;

	BUG_ON(gmacdev == NULL);

	/*
	 * If the speed for this GMAC is forced, and link polling
	 * is disabled by platform, do not proceed with the
	 * changes below. This would be true for GMACs connected
	 * to switch and interfaces that do not use a PHY.
	 */
	if ((gmacdev->forced_speed != SPEED_UNKNOWN)
	    && (!test_bit(__NSS_GMAC_LINKPOLL, &gmacdev->flags)))
		return -EPERM;

	phydev = gmacdev->phydev;
	if (!phydev) {
		return -EPERM;
	}

	if (ecmd->autoneg == AUTONEG_ENABLE) {
		set_bit(__NSS_GMAC_AUTONEG, &gmacdev->flags);
	} else {
		clear_bit(__NSS_GMAC_AUTONEG, &gmacdev->flags);
	}

	return phy_ethtool_sset(phydev, ecmd);
}

/**
 * @brief Set driver specific flags
 * @param[in] pointer to struct net_device.
 * @param[in] flags to set.
 */
static int32_t nss_gmac_set_priv_flags(struct net_device *netdev, u32 flags)
{
	struct nss_gmac_dev *gmacdev = (struct nss_gmac_dev *)netdev_priv(netdev);
	struct phy_device *phydev = gmacdev->phydev;
	uint32_t changed = flags ^ gmacdev->drv_flags;

	if (changed & NSS_GMAC_PRIV_FLAG(LINKPOLL)) {

		if (!test_bit(__NSS_GMAC_LINKPOLL, &gmacdev->flags)) {
			/*
			 * Platform has disabled Link polling. Do not enable
			 * link polling via driver specific flags. This conditon
			 * is typically true for GMACs connected to a switch.
			 */
			return -EOPNOTSUPP;
		}

		if (IS_ERR(phydev))
			return -EINVAL;

		if (flags & NSS_GMAC_PRIV_FLAG(LINKPOLL)) {
			gmacdev->drv_flags |= NSS_GMAC_PRIV_FLAG(LINKPOLL);
			if (phydev->autoneg == AUTONEG_ENABLE)
				genphy_restart_aneg(phydev);
		} else {
			gmacdev->drv_flags &= ~NSS_GMAC_PRIV_FLAG(LINKPOLL);
		}
	}

	/*
	 * Set ignore rx csum flag
	 */
	if (changed & NSS_GMAC_PRIV_FLAG(IGNORE_RX_CSUM_ERR)) {

		if (flags & NSS_GMAC_PRIV_FLAG(IGNORE_RX_CSUM_ERR)) {
			nss_gmac_rx_tcpip_chksum_drop_disable(gmacdev);
			gmacdev->drv_flags |= NSS_GMAC_PRIV_FLAG(IGNORE_RX_CSUM_ERR);
			netdev_dbg(netdev, "%s: Enabled 'ignore Rx csum error' flag", __func__);
		} else {
			nss_gmac_rx_tcpip_chksum_drop_enable(gmacdev);
			gmacdev->drv_flags &= ~NSS_GMAC_PRIV_FLAG(IGNORE_RX_CSUM_ERR);
			netdev_dbg(netdev, "%s: Disabled 'ignore Rx csum error' flag", __func__);
		}
	}

	return 0;
}

/**
 * @brief Get driver specific flags
 * @param[in] pointer to struct net_device.
 */
static uint32_t nss_gmac_get_priv_flags(struct net_device *netdev)
{
	struct nss_gmac_dev *gmacdev = (struct nss_gmac_dev *)netdev_priv(netdev);

	return (uint32_t)gmacdev->drv_flags;
}

/**
 * Ethtool operations
 */
struct ethtool_ops nss_gmac_ethtool_ops = {
	.get_drvinfo = &nss_gmac_get_drvinfo,
	.get_link = &ethtool_op_get_link,
	.get_msglevel = &nss_gmac_get_msglevel,
	.get_pauseparam = &nss_gmac_get_pauseparam,
	.set_pauseparam = &nss_gmac_set_pauseparam,
	.nway_reset = &nss_gmac_nway_reset,
	.get_wol = &nss_gmac_get_wol,
	.get_settings = &nss_gmac_get_settings,
	.set_settings = &nss_gmac_set_settings,
	.get_strings = &nss_gmac_get_strings,
	.get_sset_count = &nss_gmac_get_strset_count,
	.get_ethtool_stats = &nss_gmac_get_ethtool_stats,
	.get_priv_flags = nss_gmac_get_priv_flags,
	.set_priv_flags = nss_gmac_set_priv_flags,
};

/**
 * @brief Register ethtool_ops
 * @param[in] pointer to struct net_device
 */
void nss_gmac_ethtool_register(struct net_device *netdev)
{
	netdev->ethtool_ops = &nss_gmac_ethtool_ops;
}
