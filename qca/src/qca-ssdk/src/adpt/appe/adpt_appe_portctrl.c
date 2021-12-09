/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include "sw.h"
#include "appe_global_reg.h"
#include "appe_global.h"
#include "adpt.h"
#include "adpt_appe_portctrl.h"
#include "hppe_portctrl_reg.h"
#include "hppe_portctrl.h"
#include "appe_counter_reg.h"
#include "appe_counter.h"
#include "appe_portvlan_reg.h"
#include "appe_portvlan.h"
#include "appe_l2_vp.h"

sw_error_t
_adpt_appe_port_mux_mac_set(a_uint32_t dev_id, fal_port_t port_id,
	a_uint32_t port_type)
{
	sw_error_t rv = SW_OK;
	a_uint32_t mode0, mode1;
	a_uint32_t mux_mac_type;
	union appe_port_mux_ctrl_u appe_port_mux_ctrl;

	ADPT_DEV_ID_CHECK(dev_id);
	memset(&appe_port_mux_ctrl, 0, sizeof(appe_port_mux_ctrl));

	if (port_type == PORT_GMAC_TYPE) {
		mux_mac_type = APPE_PORT_MUX_MAC_TYPE;
	} else if (port_type == PORT_XGMAC_TYPE) {
		mux_mac_type = APPE_PORT_MUX_XMAC_TYPE;
	} else {
		return SW_BAD_VALUE;
	}

	rv = appe_port_mux_ctrl_get(dev_id, &appe_port_mux_ctrl);
	SW_RTN_ON_ERROR (rv);

	switch (port_id) {
		case SSDK_PHYSICAL_PORT1:
			appe_port_mux_ctrl.bf.port1_mac_sel = mux_mac_type;
			break;
		case SSDK_PHYSICAL_PORT2:
			appe_port_mux_ctrl.bf.port2_mac_sel = mux_mac_type;
			break;
		case SSDK_PHYSICAL_PORT3:
			appe_port_mux_ctrl.bf.port3_mac_sel = mux_mac_type;
			break;
		case SSDK_PHYSICAL_PORT4:
			appe_port_mux_ctrl.bf.port4_mac_sel = mux_mac_type;
			break;
		case SSDK_PHYSICAL_PORT5:
			mode0 = ssdk_dt_global_get_mac_mode(dev_id,
						SSDK_UNIPHY_INSTANCE0);
			mode1 = ssdk_dt_global_get_mac_mode(dev_id,
						SSDK_UNIPHY_INSTANCE1);
			appe_port_mux_ctrl.bf.port5_mac_sel = mux_mac_type;
			if ((((mode0 == PORT_WRAPPER_PSGMII) ||
				(mode0 == PORT_WRAPPER_PSGMII_FIBER)) &&
				(mode1 == PORT_WRAPPER_MAX)) ||
				(((mode0 == PORT_WRAPPER_SGMII4_RGMII4) ||
				(mode0 == PORT_WRAPPER_SGMII_CHANNEL4)) &&
				(mode1 == PORT_WRAPPER_MAX))) {
			    	appe_port_mux_ctrl.bf.port5_pcs_sel =
					APPE_PORT5_MUX_PCS_UNIPHY0;
			}
			if (mode1 != PORT_WRAPPER_MAX) {
				appe_port_mux_ctrl.bf.port5_pcs_sel =
					APPE_PORT5_MUX_PCS_UNIPHY1;
			}
			break;
		case SSDK_PHYSICAL_PORT6:
			appe_port_mux_ctrl.bf.port6_mac_sel = mux_mac_type;
			break;
		default:
			break;
	}

	rv = appe_port_mux_ctrl_set(dev_id, &appe_port_mux_ctrl);
	SW_RTN_ON_ERROR (rv);

	SSDK_INFO("appe port %d mac type is %d\n", port_id, port_type);

	return rv;
}
#ifndef IN_PORTCONTROL_MINI
sw_error_t adpt_appe_port_8023ah_set(a_uint32_t dev_id, fal_port_t port_id,
	fal_port_8023ah_ctrl_t *port_8023ah_ctrl)
{
	sw_error_t rv = SW_OK;

	ADPT_DEV_ID_CHECK(dev_id);

	if(!ADPT_IS_PPORT (port_id))
	{
		SSDK_ERROR ("port 0x%x is not supported", port_id);
		return SW_OUT_OF_RANGE;
	}
	rv = appe_link_oam_ctrl_loopback_state_set (dev_id, port_id,
		port_8023ah_ctrl->loopback_enable);

	return rv;
}

sw_error_t adpt_appe_port_8023ah_get(a_uint32_t dev_id, a_uint32_t port_id,
	fal_port_8023ah_ctrl_t *port_8023ah_ctrl)
{
	sw_error_t rv = SW_OK;

	ADPT_DEV_ID_CHECK(dev_id);

	if(!ADPT_IS_PPORT (port_id))
	{
		SSDK_ERROR ("port 0x%x is not supported", port_id);
		return SW_OUT_OF_RANGE;
	}
	rv = appe_link_oam_ctrl_loopback_state_get (dev_id, port_id,
		&(port_8023ah_ctrl->loopback_enable));

	return rv;
}
#endif

sw_error_t
adpt_appe_port_mtu_cfg_set(a_uint32_t dev_id, fal_port_t port_id,
	fal_mtu_cfg_t *mtu_cfg)
{
	a_uint32_t port_type = 0, port_value = 0;
	union l2_vp_port_tbl_u l2_vp_port_tbl = {0};

	ADPT_DEV_ID_CHECK(dev_id);
	ADPT_NULL_POINT_CHECK(mtu_cfg);

	port_type = FAL_PORT_ID_TYPE(port_id);
	port_value = FAL_PORT_ID_VALUE(port_id);

	SW_RTN_ON_ERROR(appe_l2_vp_port_tbl_get(dev_id, port_value, &l2_vp_port_tbl));
	l2_vp_port_tbl.bf.mtu_check_type = mtu_cfg->mtu_type;
	l2_vp_port_tbl.bf.extra_header_len = mtu_cfg->extra_header_len;

	l2_vp_port_tbl.bf.eg_vlan_fmt_valid = A_FALSE;
	l2_vp_port_tbl.bf.eg_ctag_fmt = A_FALSE;
	l2_vp_port_tbl.bf.eg_stag_fmt = A_FALSE;
	if(mtu_cfg->eg_vlan_tag_flag != 0)
	{
		l2_vp_port_tbl.bf.eg_vlan_fmt_valid = A_TRUE;
	}
	if(mtu_cfg->eg_vlan_tag_flag & BIT(0))
	{
		l2_vp_port_tbl.bf.eg_ctag_fmt = A_TRUE;
	}
	if(mtu_cfg->eg_vlan_tag_flag & BIT(1))
	{
		l2_vp_port_tbl.bf.eg_stag_fmt = A_TRUE;
	}
	/*physical_port_mtu_check_en is only for VP port*/
	if(port_type == FAL_PORT_TYPE_VPORT)
	{
		if(!mtu_cfg->mtu_enable)
		{
			l2_vp_port_tbl.bf.physical_port_mtu_check_en = A_TRUE;
		}
		else
		{
			l2_vp_port_tbl.bf.physical_port_mtu_check_en = A_FALSE;
		}
	}
	else
	{
		if(!mtu_cfg->mtu_enable)
		{
			SSDK_ERROR("physical port %d does not support mtu disable\n",
				port_id);
			return SW_NOT_SUPPORTED;
		}
	}

	SW_RTN_ON_ERROR(appe_l2_vp_port_tbl_set (dev_id, port_value, &l2_vp_port_tbl));

	return SW_OK;
}

sw_error_t
adpt_appe_port_mtu_cfg_get(a_uint32_t dev_id, fal_port_t port_id,
		fal_mtu_cfg_t *mtu_cfg)
{
	a_uint32_t port_type, port_value;
	union l2_vp_port_tbl_u l2_vp_port_tbl = {0};

	ADPT_DEV_ID_CHECK(dev_id);
	ADPT_NULL_POINT_CHECK(mtu_cfg);

	port_type = FAL_PORT_ID_TYPE(port_id);
	port_value = FAL_PORT_ID_VALUE(port_id);

	SW_RTN_ON_ERROR(appe_l2_vp_port_tbl_get(dev_id, port_value, &l2_vp_port_tbl));
	mtu_cfg->mtu_type = l2_vp_port_tbl.bf.mtu_check_type;
	mtu_cfg->extra_header_len = l2_vp_port_tbl.bf.extra_header_len;
	mtu_cfg->eg_vlan_tag_flag = 0;
	if(l2_vp_port_tbl.bf.eg_vlan_fmt_valid == A_TRUE)
	{
		if(l2_vp_port_tbl.bf.eg_ctag_fmt)
		{
			mtu_cfg->eg_vlan_tag_flag |= BIT(0);
		}
		if(l2_vp_port_tbl.bf.eg_stag_fmt)
		{
			mtu_cfg->eg_vlan_tag_flag |= BIT(1);
		}
	}
	mtu_cfg->mtu_enable = A_TRUE;
	if(port_type == FAL_PORT_TYPE_VPORT)
	{
		if(l2_vp_port_tbl.bf.physical_port_mtu_check_en)
		{
			mtu_cfg->mtu_enable = A_FALSE;
		}
	}

	return SW_OK;
}

sw_error_t
adpt_appe_port_cnt_mode_set(a_uint32_t dev_id, fal_port_t port_id, fal_port_cnt_cfg_t *cnt_cfg)
{
	sw_error_t rv = SW_OK;
	union port_vp_rx_cnt_mode_tbl_u port_vp_rx_cnt_mode_tbl;
	union eg_vp_tbl_u eg_vp_tbl;
	a_uint32_t port_value = FAL_PORT_ID_VALUE(port_id);

	ADPT_DEV_ID_CHECK(dev_id);
	ADPT_NULL_POINT_CHECK(cnt_cfg);

	aos_mem_zero(&port_vp_rx_cnt_mode_tbl, sizeof(union port_vp_rx_cnt_mode_tbl_u));
	aos_mem_zero(&eg_vp_tbl, sizeof(union eg_vp_tbl_u));

	/* set RX counter configs */
	rv = appe_port_vp_rx_cnt_mode_tbl_get(dev_id, port_value/32, &port_vp_rx_cnt_mode_tbl);
	SW_RTN_ON_ERROR(rv);

	switch (cnt_cfg->rx_cnt_mode) {
		case FAL_PORT_CNT_MODE_IP_PKT:
			port_vp_rx_cnt_mode_tbl.bf.cnt_mode &= ~BIT(port_value%32);
			break;
		case FAL_PORT_CNT_MODE_FULL_PKT:
			port_vp_rx_cnt_mode_tbl.bf.cnt_mode |= BIT(port_value%32);
			break;
		default:
			SSDK_ERROR("Unsupported rx_cnt_mode: %d\n", cnt_cfg->rx_cnt_mode);
			return SW_BAD_PARAM;
	}

	rv = appe_port_vp_rx_cnt_mode_tbl_set(dev_id, port_value/32, &port_vp_rx_cnt_mode_tbl);
	SW_RTN_ON_ERROR(rv);

	/* set TX counter configs */
	rv = appe_egress_vp_tbl_get(dev_id, port_value, &eg_vp_tbl);
	SW_RTN_ON_ERROR(rv);

	eg_vp_tbl.bf.cnt_mode = cnt_cfg->tx_cnt_mode;

	rv = appe_egress_vp_tbl_set(dev_id, port_value, &eg_vp_tbl);
	SW_RTN_ON_ERROR(rv);

	return rv;
}

sw_error_t
adpt_appe_port_cnt_mode_get(a_uint32_t dev_id, fal_port_t port_id, fal_port_cnt_cfg_t *cnt_cfg)
{
	sw_error_t rv = SW_OK;
	union port_vp_rx_cnt_mode_tbl_u port_vp_rx_cnt_mode_tbl;
	union eg_vp_tbl_u eg_vp_tbl;
	a_uint32_t port_value = FAL_PORT_ID_VALUE(port_id);

	ADPT_DEV_ID_CHECK(dev_id);
	ADPT_NULL_POINT_CHECK(cnt_cfg);

	aos_mem_zero(&port_vp_rx_cnt_mode_tbl, sizeof(union port_vp_rx_cnt_mode_tbl_u));
	aos_mem_zero(&eg_vp_tbl, sizeof(union eg_vp_tbl_u));

	/* get RX counter configs */
	rv = appe_port_vp_rx_cnt_mode_tbl_get(dev_id, port_value/32, &port_vp_rx_cnt_mode_tbl);
	SW_RTN_ON_ERROR(rv);

	if (port_vp_rx_cnt_mode_tbl.bf.cnt_mode & BIT(port_value%32))
		cnt_cfg->rx_cnt_mode = FAL_PORT_CNT_MODE_FULL_PKT;
	else
		cnt_cfg->rx_cnt_mode = FAL_PORT_CNT_MODE_IP_PKT;

	/* get TX counter configs */
	rv = appe_egress_vp_tbl_get(dev_id, port_value, &eg_vp_tbl);
	SW_RTN_ON_ERROR(rv);

	cnt_cfg->tx_cnt_mode = eg_vp_tbl.bf.cnt_mode;

	return rv;
}

sw_error_t
adpt_appe_port_rx_cnt_get(a_uint32_t dev_id, fal_port_t port_id, fal_port_cnt_t *port_cnt)
{
	sw_error_t rv = SW_OK;
	union phy_port_rx_cnt_tbl_u phy_port_rx_cnt_tbl;
	union port_rx_cnt_tbl_u vport_rx_cnt_tbl;
	a_uint32_t port_value = FAL_PORT_ID_VALUE(port_id);

	ADPT_DEV_ID_CHECK(dev_id);
	ADPT_NULL_POINT_CHECK(port_cnt);

	aos_mem_zero(&phy_port_rx_cnt_tbl, sizeof(union phy_port_rx_cnt_tbl_u));
	aos_mem_zero(&vport_rx_cnt_tbl, sizeof(union port_rx_cnt_tbl_u));

	if(ADPT_IS_PPORT(port_id))
	{
		rv = appe_phy_port_rx_cnt_tbl_get(dev_id, port_value, &phy_port_rx_cnt_tbl);
		SW_RTN_ON_ERROR(rv);

		port_cnt->rx_pkt_cnt = phy_port_rx_cnt_tbl.bf.rx_pkt_cnt;
		port_cnt->rx_byte_cnt = ((a_uint64_t)phy_port_rx_cnt_tbl.bf.rx_byte_cnt_1 <<
			SW_FIELD_OFFSET_IN_WORD(PHY_PORT_RX_CNT_TBL_RX_BYTE_CNT_OFFSET)) |
			phy_port_rx_cnt_tbl.bf.rx_byte_cnt_0;
		port_cnt->rx_drop_pkt_cnt = ((a_uint32_t)phy_port_rx_cnt_tbl.bf.rx_drop_pkt_cnt_1 <<
			SW_FIELD_OFFSET_IN_WORD(PHY_PORT_RX_CNT_TBL_RX_DROP_PKT_CNT_OFFSET)) |
			phy_port_rx_cnt_tbl.bf.rx_drop_pkt_cnt_0;
		port_cnt->rx_drop_byte_cnt = ((a_uint64_t)phy_port_rx_cnt_tbl.bf.rx_drop_byte_cnt_1 <<
			SW_FIELD_OFFSET_IN_WORD(PHY_PORT_RX_CNT_TBL_RX_DROP_BYTE_CNT_OFFSET)) |
			phy_port_rx_cnt_tbl.bf.rx_drop_byte_cnt_0;
	}
	else
	{
		rv = appe_port_rx_cnt_tbl_get(dev_id, port_value, &vport_rx_cnt_tbl);
		SW_RTN_ON_ERROR(rv);

		port_cnt->rx_pkt_cnt = vport_rx_cnt_tbl.bf.rx_pkt_cnt;
		port_cnt->rx_byte_cnt = ((a_uint64_t)vport_rx_cnt_tbl.bf.rx_byte_cnt_1 <<
			SW_FIELD_OFFSET_IN_WORD(PORT_RX_CNT_TBL_RX_BYTE_CNT_OFFSET)) |
			vport_rx_cnt_tbl.bf.rx_byte_cnt_0;
		port_cnt->rx_drop_pkt_cnt = ((a_uint32_t)vport_rx_cnt_tbl.bf.rx_drop_pkt_cnt_1 <<
			SW_FIELD_OFFSET_IN_WORD(PORT_RX_CNT_TBL_RX_DROP_PKT_CNT_OFFSET)) |
			vport_rx_cnt_tbl.bf.rx_drop_pkt_cnt_0;
		port_cnt->rx_drop_byte_cnt = ((a_uint64_t)vport_rx_cnt_tbl.bf.rx_drop_byte_cnt_1 <<
			SW_FIELD_OFFSET_IN_WORD(PORT_RX_CNT_TBL_RX_DROP_BYTE_CNT_OFFSET)) |
			vport_rx_cnt_tbl.bf.rx_drop_byte_cnt_0;
	}

	return rv;
}

sw_error_t
adpt_appe_port_rx_cnt_flush(a_uint32_t dev_id, fal_port_t port_id)
{
	sw_error_t rv = SW_OK;
	union phy_port_rx_cnt_tbl_u phy_port_rx_cnt_tbl;
	union port_rx_cnt_tbl_u vport_rx_cnt_tbl;
	a_uint32_t port_value = FAL_PORT_ID_VALUE(port_id);

	ADPT_DEV_ID_CHECK(dev_id);

	aos_mem_zero(&phy_port_rx_cnt_tbl, sizeof(union phy_port_rx_cnt_tbl_u));
	aos_mem_zero(&vport_rx_cnt_tbl, sizeof(union port_rx_cnt_tbl_u));

	if(ADPT_IS_PPORT(port_id))
	{
		rv = appe_phy_port_rx_cnt_tbl_set(dev_id, port_value, &phy_port_rx_cnt_tbl);
		SW_RTN_ON_ERROR(rv);
	}

	//rx_vp could counter corresponding pp cnt(rx_vp[1] could recored the pport 1 cnt)
	rv = appe_port_rx_cnt_tbl_set(dev_id, port_value, &vport_rx_cnt_tbl);
	SW_RTN_ON_ERROR(rv);

	return rv;
}
