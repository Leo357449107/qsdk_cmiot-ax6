/*
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2021-2022, Qualcomm Innovation Center, Inc. All rights reserved
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __NSS_DP_DEV_H__
#define __NSS_DP_DEV_H__

#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/version.h>

#include "nss_dp_api_if.h"
#include "nss_dp_hal_if.h"
#include "nss_dp_hal_info.h"

#define NSS_DP_ACL_DEV_ID 0

/*
 * Rx buffer allocation size as per memory profile
 */
#if (defined(NSS_DP_MEM_PROFILE_LOW) || defined(NSS_DP_MEM_PROFILE_MEDIUM)) && !defined(__LP64__)
#define NSS_DP_RX_BUFFER_SIZE		1856
#else
#define NSS_DP_RX_BUFFER_SIZE		1984
#endif

#if defined(NSS_DP_IPQ95XX)
/*
 * Rx rings flow control threshold values
 *
 * The Rx flow control has the X-OFF and the X-ON threshold values.
 * Whenever the free Rx ring descriptor count falls below the X-OFF value, the
 * ring level flow control will kick in and the mapped PPE queues will be backpressured.
 * Similarly, whenever the free Rx ring descriptor count crosses the X-ON value,
 * the ring level flow control will be disabled.
 */
#define NSS_DP_RX_FC_XOFF_DEF		16
#define NSS_DP_RX_FC_XON_DEF		32

/*
 * Rx ring's mapped AC FC threshold value.
 *
 * This value is picked by running four uni-UDPv4 traffic which are mapped
 * to 4 different rings (and hence processed by 4 different cores) and then
 * increase one flow's rate to more than what the core can process and observe
 * whether this congestion in one flow is influencing other flows or not.
 * Below is the maximum threshold value which is invoking the congestion's queue
 * tail drop and hence not trigerring the Rx port's back-pressure.
 */
#define NSS_DP_RX_AC_FC_THRES_DEF	0x104
#endif

struct nss_dp_global_ctx;

/*
 * nss data plane device structure
 */
struct nss_dp_dev {
	uint32_t macid;			/* Sequence# of Mac on the platform */
	uint32_t vsi;			/* vsi number */
	unsigned long flags;		/* Status flags */
	unsigned long drv_flags;	/* Driver specific feature flags */

	/* Phy related stuff */
	struct phy_device *phydev;	/* Phy device */
	struct mii_bus *miibus;		/* MII bus */
	uint32_t phy_mii_type;		/* RGMII/SGMII/QSGMII */
	uint32_t phy_mdio_addr;		/* Mdio address */
	bool link_poll;			/* Link polling enable? */
	uint32_t forced_speed;		/* Forced speed? */
	uint32_t forced_duplex;		/* Forced duplex? */
	uint32_t link_state;		/* Current link state */
	uint32_t pause;			/* Current flow control settings */

	struct net_device *netdev;
	struct platform_device *pdev;
	struct napi_struct napi;

	struct nss_dp_data_plane_ctx *dpc;
					/* context when NSS owns GMACs */
	struct nss_dp_data_plane_ops *data_plane_ops;
					/* ops for each data plane */
	struct nss_dp_global_ctx *ctx;	/* Global NSS DP context */
	struct nss_gmac_hal_dev *gmac_hal_ctx;
					/* context of gmac hal */
	struct nss_gmac_hal_ops *gmac_hal_ops;
					/* GMAC HAL OPS */
	struct nss_dp_hal_info dp_info;
					/* SoC specific data plane information */

	/* switchdev related attributes */
#ifdef CONFIG_NET_SWITCHDEV
	u8 stp_state;			/* STP state of this physical port */
	unsigned long brport_flags;	/* bridge port flags */
#endif
	uint32_t rx_page_mode;		/* page mode for Rx processing */
	uint32_t rx_jumbo_mru;		/* Jumbo mru value for Rx processing */
};

/*
 * nss data plane global context
 */
struct nss_dp_global_ctx {
	struct nss_dp_dev *nss_dp[NSS_DP_HAL_MAX_PORTS];
	struct nss_gmac_hal_ops *gmac_hal_ops[GMAC_HAL_TYPE_MAX];
					/* GMAC HAL OPS */
	bool common_init_done;		/* Flag to hold common init state */
	uint8_t slowproto_acl_bm;	/* Port bitmap to allow slow protocol packets */
	uint32_t rx_buf_size;		/* Buffer size to allocate */
};

/* Global data */
extern struct nss_dp_global_ctx dp_global_ctx;
extern struct nss_dp_data_plane_ctx dp_global_data_plane_ctx[NSS_DP_HAL_MAX_PORTS];
#if defined(NSS_DP_IPQ95XX)
extern int nss_dp_rx_fc_xon;
extern int nss_dp_rx_fc_xoff;
extern int nss_dp_rx_ac_fc_threshold;
#endif

/*
 * nss data plane link state
 */
enum nss_dp_link_state {
	__NSS_DP_LINK_UP,	/* Indicate link is UP */
	__NSS_DP_LINK_DOWN	/* Indicate link is down */
};

/*
 * nss data plane status
 */
enum nss_dp_state {
	__NSS_DP_UP,		/* set to indicate the interface is UP	*/
	__NSS_DP_RXCSUM,	/* Rx checksum enabled			*/
	__NSS_DP_AUTONEG,	/* Autonegotiation Enabled		*/
	__NSS_DP_LINKPOLL,	/* Poll link status			*/
};

/*
 * nss data plane private flags
 */
enum nss_dp_priv_flags {
	__NSS_DP_PRIV_FLAG_INIT_DONE,
	__NSS_DP_PRIV_FLAG_IRQ_REQUESTED,
	__NSS_DP_PRIV_FLAG_INIT_OVERRIDE,
	__NSS_DP_PRIV_FLAG_MAX,
};
#define NSS_DP_PRIV_FLAG(x)	(1 << __NSS_DP_PRIV_FLAG_ ## x)

/*
 * nss_dp_set_ethtool_ops()
 */
void nss_dp_set_ethtool_ops(struct net_device *netdev);

/*
 * nss data plane switchdev helpers
 */
#ifdef CONFIG_NET_SWITCHDEV
void nss_dp_switchdev_setup(struct net_device *dev);
bool nss_dp_is_phy_dev(struct net_device *dev);
#endif

#endif	/* __NSS_DP_DEV_H__ */
