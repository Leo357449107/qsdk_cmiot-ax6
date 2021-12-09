/*
 * Copyright (c) 2016-2017, 2019-2020, The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/**
 * @defgroup hppe_init _HPPE_INIT_H_
 * @{
 */
#ifndef _HPPE_INIT_H_
#define _HPPE_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "init/ssdk_init.h"

#define HPPE_GCC_UNIPHY_REG_INC 0x100
#ifdef APPE
#define HPPE_TO_XGMAC_PORT_ID(port_id) (port_id -1)
#else
#define HPPE_TO_XGMAC_PORT_ID(port_id)  (port_id - 5)
#endif
#define HPPE_TO_GMAC_PORT_ID(port_id) (port_id -1)
#define HPPE_FCS_LEN  4

#define HPPE_MUX_PORT1  5
#define HPPE_MUX_PORT2  6

#define HPPE_GCC_UNIPHY_PSGMII_SOFT_RESET 0x3ff2
#define HPPE_GCC_UNIPHY_USXGMII_SOFT_RESET 0x36
#define HPPE_MAX_PORT_NUM 6
#define HPPE_GCC_UNIPHY_USXGMII_XPCS_RESET 0x4
#define HPPE_GCC_UNIPHY_USXGMII_XPCS_RELEASE_RESET 0x0


#define HPPE_UNIPHY_BASE1	0x10000
#define HPPE_UNIPHY_BASE2	0x20000
#define HPPE_UNIPHY_MAX_DIRECT_ACCESS_REG	0x7fff
#define HPPE_UNIPHY_INDIRECT_REG_ADDR   0x83fc
#define HPPE_UNIPHY_INDIRECT_HIGH_ADDR  0x1fff00
#define HPPE_UNIPHY_INDIRECT_LOW_ADDR   0xff
#define HPPE_UNIPHY_INDIRECT_DATA       0x20
#define UNIPHY_CALIBRATION_DONE         0x1
#define UNIPHY_10GR_LINKUP              0x1
#define UNIPHY_10GR_LINK_LOSS           0x7
#define UNIPHY_ATHEROS_NEGOTIATION      0x0
#define UNIPHY_STANDARD_NEGOTIATION     0x1
#define UNIPHY_CH0_QSGMII_SGMII_MODE    0x0
#define UNIPHY_CH0_PSGMII_MODE          0x1
#define UNIPHY_CH0_SGMII_MODE           0x0
#define UNIPHY_CH0_QSGMII_MODE          0x1
#define UNIPHY_SGMII_MODE_ENABLE        0x1
#define UNIPHY_SGMII_MODE_DISABLE       0x0
#define UNIPHY_SGMIIPLUS_MODE_ENABLE    0x1
#define UNIPHY_SGMIIPLUS_MODE_DISABLE   0x0
#define UNIPHY_XPCS_MODE_ENABLE         0x1
#define UNIPHY_XPCS_MODE_DISABLE        0x0
#define UNIPHY_PHY_SGMII_MODE           0x3
#define UNIPHY_PHY_SGMIIPLUS_MODE       0x5
#define UNIPHY_SGMII_CHANNEL1_DISABLE   0x0
#define UNIPHY_SGMII_CHANNEL1_ENABLE    0x1
#define UNIPHY_SGMII_CHANNEL4_DISABLE   0x0
#define UNIPHY_SGMII_CHANNEL4_ENABLE    0x1
#define UNIPHY_FORCE_SPEED_ENABLE       0x1

#define SGMII_1000M_SOURCE1_CLOCK1 0x101
#define SGMII_100M_SOURCE1_CLOCK1 0x109
#define SGMII_10M_SOURCE1_CLOCK1 0x109
#define SGMII_1000M_SOURCE2_CLOCK1 0x301
#define SGMII_100M_SOURCE2_CLOCK1 0x309
#define SGMII_10M_SOURCE2_CLOCK1 0x309
#define SGMII_1000M_CLOCK2 0x0
#define SGMII_100M_CLOCK2 0x0
#define SGMII_10M_CLOCK2 0x9
#define UNIPHY_MISC2_REG_OFFSET 0x218
#define UNIPHY_PLL_RESET_REG_OFFSET 0x780
#define UNIPHY_MISC2_REG_VALUE 0x70
#define UNIPHY_MISC2_REG_SGMII_PLUS_MODE 0x50
#define UNIPHY_PLL_RESET_REG_VALUE 0x02bf
#define UNIPHY_PLL_RESET_REG_DEFAULT_VALUE 0x02ff
#define UNIPHY_MISC2_REG_SGMII_MODE 0x30
#define UNIPHY_FORCE_SPEED_MODE_ENABLE       0x1

#define AQ_PHY_AUTO_STATUS_REG 0x70001
#define AQ_PHY_LINK_STATUS_REG 0x7c800
#define AQ_PHY_FLOWCTRL_STATUS_REG 0x7c810
#define PHY_MII_STATUS_REG 0x11


#define MALIBU_PHY_QSGMII 0x8504
#define MALIBU_PHY_MODE_REG 0x1f
#define MALIBU_PSGMII_PHY_ADDR 0x5
#define MALIBU_MODE_CHANAGE_RESET 0x0
#define MALIBU_MODE_RESET_DEFAULT_VALUE 0x5f
#define MALIBU_MODE_RESET_REG 0x0

sw_error_t hppe_init(a_uint32_t dev_id, ssdk_init_cfg *cfg);
a_bool_t hppe_mac_port_valid_check(a_uint32_t dev_id, fal_port_t port_id);
a_bool_t hppe_xgmac_port_check(a_uint32_t dev_id, fal_port_t port_id);
sw_error_t hppe_cleanup(a_uint32_t dev_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HPPE_INIT_H_ */
/**
 * @}
 */
