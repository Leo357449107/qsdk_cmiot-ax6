/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#include "nss_macsec_ops.h"

static struct secy_rx_sc_policy_rule_t
	g_rx_sc_policy[MAX_SECY_ID][SECY_POLICY_IDX_MAX + 1];
static struct secy_tx_sc_policy_rule_t
	g_tx_sc_policy[MAX_SECY_ID][SECY_POLICY_IDX_MAX + 1];
static u32 tx_ssci[MAX_SECY_ID][SECY_POLICY_IDX_MAX + 1];
static u32 rx_ssci[MAX_SECY_ID][SECY_POLICY_IDX_MAX + 1];
struct secy_ctl_filt_t g_secy_tx_filt[MAX_SECY_ID];
struct secy_ctl_filt_t g_secy_rx_filt[MAX_SECY_ID];
static struct secy_pn_table_t *secy_pn_table[MAX_SECY_ID];


#define PHY_REG_FIELD_READ(secy_id, phy_layer, reg_addr, field, pFieldValue) \
	phy_acces_reg_field_read(secy_id, phy_layer, reg_addr, \
	field##_OFFSET, field##_LEN, (u16 *)pFieldValue)

#define PHY_REG_FIELD_WRITE(secy_id, phy_layer, reg_addr, field, fieldValue) \
	phy_acces_reg_field_write(secy_id, phy_layer, reg_addr, \
	field##_OFFSET, field##_LEN, (u16)fieldValue)

static g_error_t phy_mii_reg_write(u32 secy_id, u32  reg_addr, u16 val16)
{
	int ret = 0;

	if (secy_id >= MAX_SECY_ID)
		return ERROR_PARAM;

	ret = nss_macsec_phy_mdio_write(secy_id, reg_addr, val16);
	if (ret)
		return ERROR_NOT_SUPPORT;
	else
		return ERROR_OK;
}
g_error_t phy_mii_reg_read(u32 secy_id, u32 reg_addr, u16 *p_val16)
{
	int ret = 0;
	u16 reg_val = 0;

	ret = nss_macsec_phy_mdio_read(secy_id, reg_addr, &reg_val);

	if (ret)
		return ERROR_NOT_SUPPORT;
	else {
		*p_val16 = reg_val;
		return ERROR_OK;
	}
}
static g_error_t phy_dbg_reg_write(u32 secy_id, u32 reg_addr, u16 val16)
{
	g_error_t ret = 0;

	ret = phy_mii_reg_write(secy_id, PHY_DBG_CTRL_REG, reg_addr);
	ret = phy_mii_reg_write(secy_id, PHY_DBG_DATA_REG, val16);

	return ret;
}
static g_error_t phy_dbg_reg_read(u32 secy_id, u32 reg_addr, u16 *p_val16)
{
	g_error_t ret = 0;

	ret = phy_mii_reg_write(secy_id, PHY_DBG_CTRL_REG, reg_addr);
	ret = phy_mii_reg_read(secy_id, PHY_DBG_DATA_REG, p_val16);

	return ret;
}
static g_error_t phy_mmd_reg_write(u32 secy_id, enum phy_layer_t phy_layer,
	u32 reg_addr, u16 val16)
{
	g_error_t ret  = 0;
	u32 phy_mmd_addr = 0;

	if (phy_layer == PHY_MMD3)
		phy_mmd_addr = MMD_PCS_ADDR;
	else if (phy_layer == PHY_MMD7)
		phy_mmd_addr = MMD_AUTONE_ADDR;
	else
		return ERROR_NOT_SUPPORT;

	reg_addr = (MII_ADDR_C45 | (phy_mmd_addr << 16) | reg_addr);
	ret = phy_mii_reg_write(secy_id, reg_addr, val16);

	return ret;
}

static g_error_t phy_mmd_reg_read(u32 secy_id, enum phy_layer_t phy_layer,
	u32 reg_addr, u16 *p_val16)
{
	u32 phy_mmd_addr = 0;
	u16 reg_val = 0;
	g_error_t ret = 0;

	if (phy_layer == PHY_MMD3)
		phy_mmd_addr = MMD_PCS_ADDR;
	else if (phy_layer == PHY_MMD7)
		phy_mmd_addr = MMD_AUTONE_ADDR;
	else
		return ERROR_NOT_SUPPORT;

	reg_addr = (MII_ADDR_C45 | (phy_mmd_addr << 16) | reg_addr);

	ret = phy_mii_reg_read(secy_id, reg_addr, &reg_val);
	*p_val16 = reg_val;

	return ret;
}

static g_error_t phy_access_reg_write(u32 secy_id, enum phy_layer_t phy_layer,
	u32 reg_addr, u16 val16)
{
	g_error_t ret  = OK;

	if ((phy_layer == PHY_MMD3) || (phy_layer == PHY_MMD7))
		ret = phy_mmd_reg_write(secy_id, phy_layer, reg_addr, val16);
	else if (phy_layer == PHY_DEBUG)
		ret = phy_dbg_reg_write(secy_id, reg_addr, val16);
	else if (phy_layer == PHY_MII)
		ret = phy_mii_reg_write(secy_id, reg_addr, val16);
	else
		return ERROR_NOT_SUPPORT;

	return ret;

}
static g_error_t phy_access_reg_read(u32 secy_id, enum phy_layer_t phy_layer,
	u32 reg_addr, u16 *p_val16)
{
	g_error_t ret  = OK;

	if (p_val16 == NULL)
		return ERROR_PARAM;

	if ((phy_layer == PHY_MMD3) || (phy_layer == PHY_MMD7))
		ret = phy_mmd_reg_read(secy_id, phy_layer, reg_addr, p_val16);
	else if (phy_layer == PHY_DEBUG)
		ret = phy_dbg_reg_read(secy_id, reg_addr, p_val16);
	else if (phy_layer == PHY_MII)
		ret = phy_mii_reg_read(secy_id, reg_addr, p_val16);
	else
		return ERROR_NOT_SUPPORT;

	return ret;
}
g_error_t phy_acces_reg_field_write(u32 secy_id, enum phy_layer_t phy_layer,
	u32 reg_addr, u8 offset, u8 length, u16 val16)
{
	g_error_t ret  = OK;
	u16  data = 0;
	u16  data16 = 0;

	if (length == 0
		|| offset >= PHY_REG_MAX_LEN
		|| length > (PHY_REG_MAX_LEN - offset)) {
		return ERROR_PARAM;
	}

	ret = phy_access_reg_read(secy_id, phy_layer, reg_addr, &data);
	if (ret != OK)
		return ret;

	val16 &= ((0x1UL << length) - 1);
	data16 = (data & ~(((0x1UL << length) - 1) << offset)) |
		(val16 << offset);

	if ((data & 0xffff) == data16)
		return ret;

	ret = phy_access_reg_write(secy_id, phy_layer, reg_addr, data16);
	if (ret != OK)
		return ret;

	return ret;
}
g_error_t phy_acces_reg_field_read(u32 secy_id, enum phy_layer_t phy_layer,
	u32 reg_addr, u8 offset, u8 length, u16 *p_val16)
{
	g_error_t ret = OK;
	u16 data = 0;

	if (length == 0 || offset >= PHY_REG_MAX_LEN
		|| length > (PHY_REG_MAX_LEN - offset)
		|| !p_val16) {

		return ERROR_PARAM;
	}

	ret = phy_access_reg_read(secy_id, phy_layer, reg_addr, &data);
	if (ret != OK)
		return ret;

	if ((offset == 0) && (length == 16))
		*p_val16 = data & 0xffff;
	else
		*p_val16 = (data >> offset) & ((0x1UL << length) - 1);

	return ret;
}

g_error_t phy_access_reg_mask_write(u32 secy_id, enum phy_layer_t   phy_layer,
	u32 reg_addr, u16 mask, u16 val16)
{
	g_error_t ret = OK;
	u16 data = 0;
	u16 data16 = val16;

	if (mask != 0xffff) {
		ret = phy_access_reg_read(secy_id, phy_layer, reg_addr, &data);
		if (ret != OK)
			return ret;

		data &= ~mask;
		data16 = (data & 0xffff) | (val16 & mask);
	}

	ret = phy_access_reg_write(secy_id, phy_layer, reg_addr, data16);
	if (ret != OK)
		return ret;

	return ret;
}
g_error_t qca808x_secy_reg_write(u32 secy_id, u32 secy_reg_addr,
	u16 secy_reg_data)
{
	return phy_access_reg_write(secy_id, PHY_MMD3, secy_reg_addr,
		secy_reg_data);
}

g_error_t qca808x_secy_reg_read(u32 secy_id, u32 secy_reg_addr,
	u16 *secy_reg_data)
{
	return phy_access_reg_read(secy_id, PHY_MMD3, secy_reg_addr,
		secy_reg_data);
}
g_error_t qca808x_secy_use_es_en_set(u32 secy_id, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_CONFIG,
		SYS_USE_ES_EN, enable);
}
g_error_t qca808x_secy_use_scb_en_set(u32 secy_id, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_CONFIG,
		SYS_USE_SCB_EN, enable);
}
g_error_t qca808x_secy_include_sci_en_set(u32 secy_id, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_CONFIG,
		SYS_INCLUDED_SCI_EN, enable);
}
g_error_t qca808x_secy_controlled_port_en_set(u32 secy_id, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_PORT_CTRL,
		SYS_PORT_EN, enable);
}
g_error_t qca808x_secy_controlled_port_en_get(u32 secy_id, bool *enable)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_PORT_CTRL, SYS_PORT_EN, &val));

	if (val)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}
g_error_t qca808x_secy_sc_sa_mapping_mode_set(u32 secy_id,
	enum secy_sc_sa_mapping_mode_t mode)
{
	return ERROR_OK;
}

static g_error_t qca808x_secy_clock_en_set(u32 secy_id, bool enable)
{
	u16 val = 0;

	if (enable == TRUE)
		val = SOFTWARE_EN;
	else if (enable == FALSE)
		val = SOFTWARE_BYPASS;
	else
		return ERROR_PARAM;

	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SOFTWARE_EN_CTRL,
		SYS_SECY_SOFTWARE_EN, val);
}
g_error_t qca808x_secy_loopback_set(u32 secy_id,
	enum secy_loopback_type_t type)
{
	u16 val = 0;

	if (type == MACSEC_NORMAL)
		val = SYS_MACSEC_EN;
	else if (type == MACSEC_SWITCH_LB)
		return ERROR_NOT_SUPPORT;
	else if (type == MACSEC_PHY_LB)
		return ERROR_NOT_SUPPORT;
	else if (type == MACSEC_BYPASS)
		val = SYS_BYPASS;
	else
		return ERROR_PARAM;

	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_PACKET_CTRL,
		SYS_SECY_LPBK, val);
}

g_error_t qca808x_secy_loopback_get(u32 secy_id,
	enum secy_loopback_type_t *type)
{
	u16 lpbk = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_PACKET_CTRL, SYS_SECY_LPBK, &lpbk));

	if (lpbk == SYS_BYPASS)
		*type = MACSEC_BYPASS;
	else if (lpbk == SYS_PHY_LB)
		*type = MACSEC_PHY_LB;
	else if (lpbk == SYS_SWITCH_LB)
		*type = MACSEC_SWITCH_LB;
	else
		*type = MACSEC_NORMAL;

	return ERROR_OK;
}
g_error_t qca808x_secy_mtu_set(u32 secy_id, u32 len)
{
	u16 val = 0;

	val = (u16)len;

	return phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_LEN, val);
}

g_error_t qca808x_secy_mtu_get(u32 secy_id, u32 *len)
{
	u16 val = 0;

	phy_access_reg_read(secy_id, PHY_MMD3, MACSEC_SYS_FRAME_LEN, &val);

	*len = (u32)val;

	return ERROR_OK;
}

static g_error_t qca808x_secy_shadow_register_en(u32 secy_id, bool enable)
{
	if (enable == TRUE) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(
			secy_id, PHY_MMD7, MACSEC_SHADOW_REGISTER,
				MACSEC_SHADOW_DUPLEX_EN, TRUE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(
			secy_id, PHY_MMD7, MACSEC_SHADOW_REGISTER,
				MACSEC_SHADOW_LEGACY_DUPLEX_EN, TRUE));
	} else {
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(
			secy_id, PHY_MMD7, MACSEC_SHADOW_REGISTER,
				MACSEC_SHADOW_DUPLEX_EN, FALSE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(
			secy_id, PHY_MMD7, MACSEC_SHADOW_REGISTER,
				MACSEC_SHADOW_LEGACY_DUPLEX_EN, FALSE));
	}
	return ERROR_OK;
}

g_error_t qca808x_secy_en_set(u32 secy_id, bool enable)
{
	enum secy_loopback_type_t type;

	if (enable == TRUE)
		type = MACSEC_NORMAL;
	else
		type = MACSEC_BYPASS;

	SHR_RET_ON_ERR(qca808x_secy_loopback_set(secy_id, type));

	SHR_RET_ON_ERR(qca808x_secy_shadow_register_en(secy_id, enable));

	return ERROR_OK;
}

g_error_t qca808x_secy_en_get(u32 secy_id, bool *enable)
{
	enum secy_loopback_type_t type;

	qca808x_secy_loopback_get(secy_id, &type);

	if (type == MACSEC_NORMAL)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}
g_error_t qca808x_secy_cipher_suite_set(u32 secy_id,
	enum secy_cipher_suite_t cipher_suite)
{
	g_error_t rv = ERROR_OK;
	bool enable = FALSE;
	bool xpn_enable = FALSE;

	if ((cipher_suite == SECY_CIPHER_SUITE_128) ||
		(cipher_suite == SECY_CIPHER_SUITE_XPN_128))
		enable = FALSE;
	else if ((cipher_suite == SECY_CIPHER_SUITE_256) ||
		(cipher_suite == SECY_CIPHER_SUITE_XPN_256))
		enable = TRUE;
	else
		rv = ERROR_NOT_SUPPORT;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_CTRL, SYS_AES256_ENABLE, enable));

	if ((cipher_suite == SECY_CIPHER_SUITE_128) ||
		(cipher_suite == SECY_CIPHER_SUITE_256))
		xpn_enable = FALSE;
	else if ((cipher_suite == SECY_CIPHER_SUITE_XPN_128) ||
		(cipher_suite == SECY_CIPHER_SUITE_XPN_256))
		xpn_enable = TRUE;
	else
		rv = ERROR_NOT_SUPPORT;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_CTRL, SYS_XPN_ENABLE, xpn_enable));

	return rv;
}
g_error_t qca808x_secy_cipher_suite_get(u32 secy_id,
	enum secy_cipher_suite_t *cipher_suite)
{
	u16 val = 0, val1 = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_CTRL, SYS_AES256_ENABLE, &val));

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_CTRL, SYS_XPN_ENABLE, &val1));

	if ((val == MACSEC_AES256_ENABLE) && (val1 == MACSEC_XPN_ENABLE))
		*cipher_suite = SECY_CIPHER_SUITE_XPN_256;
	else if ((val == MACSEC_AES256_ENABLE) && (val1 != MACSEC_XPN_ENABLE))
		*cipher_suite = SECY_CIPHER_SUITE_256;
	else if ((val != MACSEC_AES256_ENABLE) && (val1 == MACSEC_XPN_ENABLE))
		*cipher_suite = SECY_CIPHER_SUITE_XPN_128;
	else
		*cipher_suite = SECY_CIPHER_SUITE_128;

	return ERROR_OK;
}
g_error_t qca808x_secy_sram_dbg_en_set(u32 secy_id, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_CONFIG,
		SYS_SRAM_DEBUG_EN, enable);
}

g_error_t qca808x_secy_sram_dbg_en_get(u32 secy_id, bool *enable)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_SRAM_DEBUG_EN, &val));

	if (val)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_sc_policy_set(u32 secy_id, u32 rule_index,
	struct secy_rx_sc_policy_rule_t *rule)
{
	u16 val = 0, msk = 0;
	u32 i = 0;

	if ((rule_index > SECY_POLICY_IDX_MAX) || (rule == NULL))
		return ERROR_PARAM;

	if (rule->rule_valid)
		msk |= SC_BIND_MASK_VALID;

	if (rule->rule_mask & SC_BIND_MASK_DA) {
		for (i = 0; i < 3; i++) {
			val = (rule->mac_da.addr[i * 2] << 8) |
				(rule->mac_da.addr[i * 2 + 1]);
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXDA_BASE(rule_index) + 2 - i, val));
		}
		msk |= SC_BIND_MASK_DA;
	}

	if (rule->rule_mask & SC_BIND_MASK_SA) {
		for (i = 0; i < 3; i++) {
			val = (rule->mac_sa.addr[i * 2] << 8) |
				(rule->mac_sa.addr[i * 2 + 1]);
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXSA_BASE(rule_index) + 2 - i, val));
		}
		msk |= SC_BIND_MASK_SA;
	}

	if (rule->rule_mask & SC_BIND_MASK_ETHERTYPE) {
		val = rule->ethtype;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXETHERTYPE(rule_index), val));
		msk |= SC_BIND_MASK_ETHERTYPE;
	}

	if (rule->rule_mask & SC_BIND_MASK_OUTER_VLAN) {
		val = rule->outer_vlanid & 0xfff;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXOUTVTAG(rule_index), val));
		msk |= SC_BIND_MASK_OUTER_VLAN;
	}

	if (rule->rule_mask & SC_BIND_MASK_INNER_VLAN) {
		val = rule->inner_vlanid & 0xfff;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXINVTAG(rule_index), val));
		msk |= SC_BIND_MASK_INNER_VLAN;
	}

	val = 0;
	if (rule->rule_mask & SC_BIND_MASK_BCAST) {
		val |= ((rule->bc_flag == TRUE)?1:0)<<SC_BIND_RX_IFBC_OFFSET;
		msk |= SC_BIND_MASK_BCAST;
	}
	val |= (rule->action.rx_sc_index & 0xf) << SC_BIND_RXCTX_OFFSET;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_RXCTX(rule_index), val));

	val = 0;
	if (rule->rule_mask & SC_BIND_MASK_TCI) {
		val |= (rule->rx_tci & 0xff) << SC_BIND_RXTCI_OFFSET;
		msk |= SC_BIND_MASK_TCI;
	}
	val |= (rule->action.decryption_offset & 0x3f) <<
		SC_BIND_RXOFFSET_OFFSET;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_RXTCI(rule_index), val));

	val = rule->rx_sci.port;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_RXSCI_BASE(rule_index), val));

	for (i = 0; i < 3; i++) {
		val = (rule->rx_sci.addr[i * 2] << 8) |
			rule->rx_sci.addr[i * 2 + 1];
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXSCI_BASE(rule_index) + 3 - i, val));
	}
	msk |= SC_BIND_MASK_SCI;

	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_RXMASK(rule_index), msk));

	memcpy(&g_rx_sc_policy[secy_id][rule_index], rule,
		sizeof(struct secy_rx_sc_policy_rule_t));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_policy_get(u32 secy_id, u32 rule_index,
	struct secy_rx_sc_policy_rule_t *rule)
{
	u16 val = 0, msk = 0;
	u32 i = 0;
	u16 enable = 0;

	if ((rule_index > SECY_POLICY_IDX_MAX) || (rule == NULL))
		return ERROR_PARAM;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_SRAM_DEBUG_EN, &enable));

	if (enable) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXMASK(rule_index), &msk));
		rule->rule_valid = (msk & SC_BIND_MASK_VALID)?TRUE:FALSE;
		rule->rule_mask  = msk & 0xff;

		for (i = 0; i < 3; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXDA_BASE(rule_index) + 2 - i, &val));
			rule->mac_da.addr[i * 2]   = (val >> 8) & 0xff;
			rule->mac_da.addr[i * 2 + 1] =  val & 0xff;
		}

		for (i = 0; i < 3; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXSA_BASE(rule_index) + 2 - i, &val));
			rule->mac_sa.addr[i * 2]   = (val >> 8) & 0xff;
			rule->mac_sa.addr[i * 2 + 1] =  val & 0xff;
		}

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXETHERTYPE(rule_index), &val));
		rule->ethtype = val;

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXOUTVTAG(rule_index), &val));
		rule->outer_vlanid = val & 0xfff;

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXINVTAG(rule_index), &val));
		rule->inner_vlanid = val & 0xfff;

		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_RXCTX(rule_index), SC_BIND_RX_IFBC, &val));
		rule->bc_flag = (val)?TRUE:FALSE;

		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_RXTCI(rule_index), SC_BIND_RXTCI, &val));
		rule->rx_tci = val;

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXSCI_BASE(rule_index), &val));
		rule->rx_sci.port = val;

		for (i = 0; i < 3; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXSCI_BASE(rule_index) + 3 - i, &val));
			rule->rx_sci.addr[i * 2]     = (val >> 8) & 0xff;
			rule->rx_sci.addr[i * 2 + 1] =  val & 0xff;
		}

		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_RXCTX(rule_index), SC_BIND_RXCTX, &val));
		rule->action.rx_sc_index = val;
		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_RXTCI(rule_index), SC_BIND_RXOFFSET, &val));
		rule->action.decryption_offset = val;
	} else {
		 memcpy(rule, &g_rx_sc_policy[secy_id][rule_index],
			sizeof(struct secy_rx_sc_policy_rule_t));
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_npn_set(u32 secy_id, u32 sc_index,
	u32 an, u32 next_pn)
{
	u16 val = 0;
	u16 channel = 0;
	u32 sa_index = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);
	val = (u16)(next_pn & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_RX_NPN(channel), val));
	val = (u16)((next_pn >> 16) & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_RX_NPN(channel) + 1, val));

	secy_pn_table[secy_id][pn_index].rx_sa_pn = next_pn;

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_sa_npn_get(u32 secy_id, u32 sc_index,
	u32 an, u32 *next_pn)
{
	u16 val0 = 0, val1 = 0, channel = 0;
	u32 sa_index = 0, i = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);

	for (i = 0; i < SECY_PN_QUERY_TIMES; i++) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_RX_NPN(channel), &val0));
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_RX_NPN(channel) + 1, &val1));
		*next_pn = (((u32)val1 << 16) | val0);
		if (*next_pn >= secy_pn_table[secy_id][pn_index].rx_sa_pn) {
			secy_pn_table[secy_id][pn_index].rx_sa_pn = *next_pn;
			break;
		}
	}
	if (i >= SECY_PN_QUERY_TIMES)
		*next_pn = secy_pn_table[secy_id][pn_index].rx_sa_pn;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_replay_protect_en_set(u32 secy_id,
	u32 sc_index, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_REPLAY_PROTECT_EN, enable);
}
g_error_t qca808x_secy_rx_sc_replay_protect_en_get(u32 secy_id,
	u32 sc_index, bool *enable)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_REPLAY_PROTECT_EN, &val));

	if (val)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_replay_window_set(u32 secy_id,
	u32 sc_index, u32 window)
{
	u16 val = 0;

	val = (u16)(window & 0xffff);

	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SYS_REPLAY_WIN_BASE, val));
	val = (u16)((window >> 16)  & 0xffff);

	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SYS_REPLAY_WIN_BASE + 1, val));
	return OK;
}

g_error_t qca808x_secy_rx_sc_replay_window_get(u32 secy_id,
	u32 sc_index, u32 *window)
{
	u16 val = 0;
	u32 win = 0;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_SYS_REPLAY_WIN_BASE, &val));
	win = val;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_SYS_REPLAY_WIN_BASE+1, &val));
	win |= val << 16;

	*window = win;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_in_used_get(u32 secy_id, u32 sc_index,
	bool *enable)
{
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));
	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_get(secy_id, sc_index, &rule));

	*enable = rule.rule_valid;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_prc_lut_set(u32 secy_id, u32 sc_index,
	struct secy_rx_prc_lut_t *pentry)
{
	struct secy_rx_sc_policy_rule_t rule;
	u32 i =  0;

	memset(&rule, 0, sizeof(rule));
	if ((sc_index > SECY_SC_IDX_MAX) || (pentry == NULL))
		return ERROR_PARAM;

	rule.rule_valid = pentry->valid;
	rule.action.rx_sc_index = sc_index;

	for (i = 0; i < 6; i++) {
		rule.mac_da.addr[i] = pentry->da[i];
		rule.mac_sa.addr[i] = pentry->sa[i];
		rule.rx_sci.addr[i] = pentry->sci[i];
	}
	rule.ethtype = pentry->ether_type;
	rule.inner_vlanid = pentry->inner_vlanid;
	rule.outer_vlanid = pentry->outer_vlanid;
	rule.bc_flag = pentry->bc_flag;
	rule.action.decryption_offset = pentry->offset;
	rule.action.rx_sc_index = pentry->channel;
	rule.rx_tci = pentry->tci;
	rule.rx_sci.port = (((u16)(pentry->sci[6])) << 8) |
			(u16)(pentry->sci[7]);
	rule.rule_mask = pentry->rule_mask;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_prc_lut_get(u32 secy_id, u32 sc_index,
	struct secy_rx_prc_lut_t *pentry)
{
	struct secy_rx_sc_policy_rule_t rule;
	u32 i = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (pentry == NULL))
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));
	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_get(secy_id, sc_index, &rule));

	pentry->valid = rule.rule_valid;
	for (i = 0; i < 6; i++) {
		pentry->da[i] = rule.mac_da.addr[i];
		pentry->sa[i] = rule.mac_sa.addr[i];
		pentry->sci[i] = rule.rx_sci.addr[i];
	}
	pentry->ether_type = rule.ethtype;
	pentry->inner_vlanid = rule.inner_vlanid;
	pentry->outer_vlanid = rule.outer_vlanid;
	pentry->bc_flag = rule.bc_flag;
	pentry->offset = rule.action.decryption_offset;
	pentry->channel = rule.action.rx_sc_index;
	pentry->tci = rule.rx_tci;
	pentry->sci[6] = (u8)(rule.rx_sci.port >> 8);
	pentry->sci[7] = (u8)(rule.rx_sci.port & 0xff);
	pentry->rule_mask = rule.rule_mask;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_create(u32 secy_id, u32 sc_index)
{
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_get(secy_id, sc_index, &rule));

	rule.rule_valid = TRUE;
	rule.action.rx_sc_index = sc_index;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_validate_frames_set(u32 secy_id,
	u32 sc_index, enum secy_vf_t value)
{
	u16 val = 0;

	if (value == STRICT)
		val = SYS_FRAME_VALIDATE_STRICT;
	else if (value == CHECKED)
		val = SYS_FRAME_VALIDATE_CHECK;
	else if (value == DISABLED)
		val = SYS_FRAME_VALIDATE_DIS;
	else
		return ERROR_PARAM;

	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_CTRL, SYS_FRAME_VALIDATE, val);
}

g_error_t qca808x_secy_rx_sc_validate_frames_get(u32 secy_id,
	u32 sc_index, enum secy_vf_t *value)
{
	u16 validate = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
			MACSEC_SYS_FRAME_CTRL, SYS_FRAME_VALIDATE, &validate));

	if (validate == SYS_FRAME_VALIDATE_STRICT)
		*value = STRICT;
	else if (validate == SYS_FRAME_VALIDATE_CHECK)
		*value = CHECKED;
	else
		*value = DISABLED;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_create(u32 secy_id, u32 sc_index, u32 an)
{
	u16 val = 0, chan_shift;
	u32 reg, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	SHR_RET_ON_ERR(qca808x_secy_rx_sa_npn_set(secy_id, sc_index, an, 1));

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/4;
	chan_shift = 2*((sc_index%4) * 2 + sa_index);

	reg += MACSEC_RX_AN_BASE;

	val  = (((u16)an&AN_MASK) << chan_shift);

	return phy_access_reg_mask_write(secy_id, PHY_MMD3, reg,
		(AN_MASK << chan_shift), val);
}
g_error_t qca808x_secy_rx_sak_set(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sak_t *key)
{
	u16 val = 0;
	u16 reg;
	int i, j, reg_offset;
	u32 channel, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) || (an > SECY_AN_IDX_MAX) ||
			(key == NULL))
		return ERROR_PARAM;

	if ((key->len != SAK_LEN_128BITS) && (key->len != SAK_LEN_256BITS) &&
			(key->len != SAK_LEN_LEGACY))
		return ERROR_PARAM;

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;

	if (key->len == SAK_LEN_256BITS) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				val = (key->sak[i * 2 + 1] << 8) |
					key->sak[i * 2];
				reg = MACSEC_RX_SAK_KEY0(channel);
				reg_offset = i;
			} else {
				j = i - 8;
				val = (key->sak1[j * 2 + 1] << 8) |
					key->sak1[j * 2];
				reg = MACSEC_RX_EXTENDED_SAK_KEY0(channel);
				reg_offset = j;
			}
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
				PHY_MMD3, reg + reg_offset, val));
		}
	} else {
		for (i = 0; i < 8; i++) {
			reg = MACSEC_RX_SAK_KEY0(channel);
			val = (key->sak[i * 2 + 1] << 8) | key->sak[i * 2];
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
				PHY_MMD3, reg + i, val));
		}
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sak_get(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sak_t *key)
{
	u16 val = 0;
	u16 reg;
	int i, j, reg_offset;
	u32 channel, sa_index;
	enum secy_cipher_suite_t cipher_suite;

	if ((sc_index > SECY_SC_IDX_MAX) || (an > SECY_AN_IDX_MAX) ||
			(key == NULL)) {
		return ERROR_PARAM;
	}
	SHR_RET_ON_ERR(qca808x_secy_cipher_suite_get(secy_id, &cipher_suite));

	if ((cipher_suite == SECY_CIPHER_SUITE_128) ||
			(cipher_suite == SECY_CIPHER_SUITE_XPN_128))
		key->len = 16;
	else
		key->len = 32;

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;
	if (key->len == SAK_LEN_256BITS) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				reg = MACSEC_RX_SAK_KEY0(channel);
				reg_offset = i;
				SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
					PHY_MMD3, reg + reg_offset, &val));
				key->sak[i * 2] = val & 0xff;
				key->sak[i * 2 + 1] = (val >> 0x8) & 0xff;
			} else {
				reg = MACSEC_RX_EXTENDED_SAK_KEY0(channel);
				reg_offset = i - 8;
				SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
					PHY_MMD3, reg + reg_offset, &val));
				j = i - 8;
				key->sak1[j * 2] = val & 0xff;
				key->sak1[j * 2 + 1] = (val >> 0x8) & 0xff;
			}
		}
	} else {
		for (i = 0; i < 8; i++) {
			reg = MACSEC_RX_SAK_KEY0(channel);
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
				PHY_MMD3, reg + i, &val));
			key->sak[i * 2] = val & 0xff;
			key->sak[i * 2 + 1] = (val >> 0x8) & 0xff;
		}
	}

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_sa_en_set(u32 secy_id, u32 sc_index,
	u32 an, bool enable)
{
	u16 val = 0;
	u16 reg, chan_shift;
	u32 sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/8;
	chan_shift = ((sc_index%8) * 2 + sa_index);

	reg += MACSEC_RX_SA_CONTROL;

	if (enable)
		val  = (1 << chan_shift);
	else
		val &= ~(1 << chan_shift);

	return phy_access_reg_mask_write(secy_id, PHY_MMD3, reg,
		(1<<chan_shift), val);
}

g_error_t qca808x_secy_rx_sa_en_get(u32 secy_id, u32 sc_index,
	u32 an, bool *enable)
{
	u16 val = 0;
	u16 reg, chan_shift;
	u32 sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/8;
	chan_shift = ((sc_index%8) * 2 + sa_index);

	reg += MACSEC_RX_SA_CONTROL;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3, reg, &val));

	if (val&(1 << chan_shift))
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_del(u32 secy_id, u32 sc_index)
{
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));


	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_get(secy_id, sc_index, &rule));

	rule.rule_valid = FALSE;
	rule.action.rx_sc_index = sc_index;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_del_all(u32 secy_id)
{
	u32 sc_index = 0;
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));


	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	for (sc_index = 0; sc_index <= SECY_SC_IDX_MAX; sc_index++) {
		SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_get(secy_id,
			sc_index, &rule));
		rule.rule_valid = FALSE;
		rule.action.rx_sc_index = sc_index;

		SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_set(secy_id,
			sc_index, &rule));
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_prc_lut_clear(u32 secy_id, u32 sc_index)
{
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));


	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_prc_lut_clear_all(u32 secy_id)
{
	u32 sc_index = 0;
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	for (sc_index = 0; sc_index <= SECY_SC_IDX_MAX; sc_index++) {
		SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_set(secy_id,
			sc_index, &rule));
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_validate_frames_set(u32 secy_id,
	enum secy_vf_t value)
{
	u32 sc_index = 0;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_validate_frames_set(secy_id,
		sc_index, value));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_validate_frames_get(u32 secy_id,
	enum secy_vf_t *value)
{
	u32 sc_index = 0;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_validate_frames_get(secy_id,
		sc_index, value));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_replay_protect_en_set(u32 secy_id,
	bool enable)
{
	u32 sc_index = 0;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_replay_protect_en_set(secy_id,
		sc_index, enable));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_replay_protect_en_get(u32 secy_id,
	bool *enable)
{
	u32 sc_index = 0;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_replay_protect_en_get(secy_id,
		sc_index, enable));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_del(u32 secy_id, u32 sc_index, u32 an)
{
	u16 val = 0, chan_shift;
	u32 reg, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/4;
	chan_shift = 2*((sc_index%4) * 2 + sa_index);

	reg += MACSEC_RX_AN_BASE;

	val  = (0x0 << chan_shift);

	return phy_access_reg_mask_write(secy_id, PHY_MMD3, reg,
		(AN_MASK<<chan_shift), val);
}
g_error_t qca808x_secy_rx_sa_del_all(u32 secy_id)
{
	u32 sc_index = 0, an_index = 0;

	for (sc_index = 0; sc_index <= SECY_SC_IDX_MAX; sc_index++) {
		for (an_index = 0; an_index <= SECY_AN_IDX_MAX; an_index++)
			qca808x_secy_rx_sa_del(secy_id, sc_index, an_index);
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_in_used_get(u32 secy_id, u32 sc_index,
	u32 an, bool *p_in_used)
{
	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	SHR_RET_ON_ERR(qca808x_secy_rx_sa_en_get(secy_id, sc_index,
		an, p_in_used));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_en_set(u32 secy_id, u32 sc_index,
	bool enable)
{
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_get(secy_id, sc_index, &rule));

	rule.action.rx_sc_index = sc_index;

	if (enable == TRUE)
		rule.rule_valid = TRUE;
	else
		rule.rule_valid = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;

}
g_error_t qca808x_secy_rx_sc_en_get(u32 secy_id, u32 sc_index, bool *enable)
{
	struct secy_rx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_rx_sc_policy_get(secy_id, sc_index, &rule));

	if (rule.rule_valid == TRUE)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;

}

g_error_t qca808x_secy_tx_sc_policy_set(u32 secy_id, u32 rule_index,
	struct secy_tx_sc_policy_rule_t *rule)
{
	u16 val = 0, msk = 0;
	u32 i = 0;

	if ((rule_index > SECY_POLICY_IDX_MAX) || (rule == NULL))
		return ERROR_PARAM;

	if (rule->rule_valid)
		msk |= SC_BIND_MASK_VALID;

	if (rule->rule_mask & SC_BIND_MASK_DA) {
		for (i = 0; i < 3; i++) {
			val = (rule->mac_da.addr[i * 2] << 8) |
				(rule->mac_da.addr[i * 2 + 1]);
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXDA_BASE(rule_index) + 2 - i, val));
		}
		msk |= SC_BIND_MASK_DA;
	}

	if (rule->rule_mask & SC_BIND_MASK_SA) {
		for (i = 0; i < 3; i++) {
			val = (rule->mac_sa.addr[i * 2] << 8) |
				(rule->mac_sa.addr[i * 2 + 1]);
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXSA_BASE(rule_index) + 2 - i, val));
		}
		msk |= SC_BIND_MASK_SA;
	}

	if (rule->rule_mask & SC_BIND_MASK_ETHERTYPE) {
		val = rule->ethtype;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXETHERTYPE(rule_index), val));
		msk |= SC_BIND_MASK_ETHERTYPE;
	}

	if (rule->rule_mask & SC_BIND_MASK_OUTER_VLAN) {
		val = rule->outer_vlanid & 0xfff;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXOUTVTAG(rule_index), val));
		msk |= SC_BIND_MASK_OUTER_VLAN;
	}

	if (rule->rule_mask & SC_BIND_MASK_INNER_VLAN) {
		val = rule->inner_vlanid & 0xfff;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXINVTAG(rule_index), val));
		msk |= SC_BIND_MASK_INNER_VLAN;
	}

	val = 0;
	if (rule->rule_mask & SC_BIND_MASK_BCAST) {
		val |= ((rule->bc_flag == TRUE)?1:0) << SC_BIND_TX_IFBC_OFFSET;
		msk |= SC_BIND_MASK_BCAST;
	}
	val |= (rule->action.tx_sc_index & 0xf) << SC_BIND_TXCTX_OFFSET;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_TXCTX(rule_index), val));

	val = 0;
	val |= (rule->action.tx_tci & 0xff) << SC_BIND_TXTCI_OFFSET;
	val |= (rule->action.encryption_offset & 0x3f) <<
		SC_BIND_TXOFFSET_OFFSET;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_TXTCI(rule_index), val));

	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
	MACSEC_SC_BIND_TXSCI_BASE(rule_index), rule->action.tx_sci.port));
	for (i = 0; i < 3; i++) {
		val = (rule->action.tx_sci.addr[i * 2] << 8) |
			rule->action.tx_sci.addr[i * 2 + 1];
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXSCI_BASE(rule_index) + 3 - i, val));
	}

	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_TXMASK(rule_index), msk));

	memcpy(&g_tx_sc_policy[secy_id][rule_index], rule,
		sizeof(struct secy_tx_sc_policy_rule_t));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sc_policy_get(u32 secy_id, u32 rule_index,
	struct secy_tx_sc_policy_rule_t *rule)
{
	u16 val = 0, msk = 0;
	u32 i = 0;
	u16 enable = 0;

	if ((rule_index > SECY_POLICY_IDX_MAX) || (rule == NULL))
		return ERROR_PARAM;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_SRAM_DEBUG_EN, &enable));

	if (enable) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXMASK(rule_index), &msk));
		rule->rule_valid = (msk & SC_BIND_MASK_VALID)?TRUE:FALSE;
		rule->rule_mask  = msk & 0x3F;

		for (i = 0; i < 3; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXDA_BASE(rule_index) + 2 - i, &val));
			rule->mac_da.addr[i * 2]   = (val >> 8) & 0xff;
			rule->mac_da.addr[i * 2+1] =  val & 0xff;
		}

		for (i = 0; i < 3; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXSA_BASE(rule_index) + 2 - i, &val));
			rule->mac_sa.addr[i * 2]   = (val >> 8) & 0xff;
			rule->mac_sa.addr[i * 2+1] =  val & 0xff;
		}

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXETHERTYPE(rule_index), &val));
		rule->ethtype = val;

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXOUTVTAG(rule_index), &val));
		rule->outer_vlanid = val & 0xfff;

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXINVTAG(rule_index), &val));
		rule->inner_vlanid = val & 0xfff;

		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_TXCTX(rule_index), SC_BIND_TX_IFBC, &val));
		rule->bc_flag = (val)?TRUE:FALSE;

		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_TXCTX(rule_index), SC_BIND_TXCTX, &val));
		rule->action.tx_sc_index = val;

		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_TXTCI(rule_index), SC_BIND_TXTCI, &val));
		rule->action.tx_tci = val;

		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SC_BIND_TXTCI(rule_index), SC_BIND_TXOFFSET, &val));
		rule->action.encryption_offset = val;

		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXSCI_BASE(rule_index), &val));
		rule->action.tx_sci.port = val;

		for (i = 0; i < 3; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_SC_BIND_TXSCI_BASE(rule_index) + 3 - i, &val));
			rule->action.tx_sci.addr[i * 2]     = (val >> 8) & 0xff;
			rule->action.tx_sci.addr[i * 2 + 1] =  val & 0xff;
		}
	} else {
		memcpy(rule, &g_tx_sc_policy[secy_id][rule_index],
			sizeof(struct secy_tx_sc_policy_rule_t));
	}
	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sa_npn_set(u32 secy_id, u32 sc_index,
	u32 an, u32 next_pn)
{
	u16 val = 0;
	u16 channel = 0;
	u32 sa_index = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);
	val = (u16)(next_pn & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_TX_NPN(channel), val));
	val = (u16)((next_pn >> 16) & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_TX_NPN(channel) + 1, val));

	secy_pn_table[secy_id][pn_index].tx_sa_pn = next_pn;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sa_npn_get(u32 secy_id, u32 sc_index,
	u32 an, u32 *next_pn)
{
	u16 val0 = 0, val1 = 0, channel = 0;
	u32 sa_index = 0, i = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);

	for (i = 0; i < SECY_PN_QUERY_TIMES; i++) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_TX_NPN(channel), &val0));
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_TX_NPN(channel) + 1, &val1));
		*next_pn = (((u32)val1 << 16) | val0);
		if (*next_pn >= secy_pn_table[secy_id][pn_index].tx_sa_pn) {
			secy_pn_table[secy_id][pn_index].tx_sa_pn = *next_pn;
			break;
		}
	}
	if (i >= SECY_PN_QUERY_TIMES)
		*next_pn = secy_pn_table[secy_id][pn_index].tx_sa_pn;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sc_in_used_get(u32 secy_id, u32 sc_index,
	bool *enable)
{
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));
	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));

	*enable = rule.rule_valid;

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_class_lut_set(u32 secy_id, u32 sc_index,
	struct secy_tx_class_lut_t *pentry)
{
	struct secy_tx_sc_policy_rule_t rule;
	u32 i = 0;
	bool enable;

	memset(&rule, 0, sizeof(rule));
	if ((sc_index > SECY_SC_IDX_MAX) || (pentry == NULL))
		return ERROR_PARAM;

	rule.rule_valid = pentry->valid;
	rule.action.tx_sc_index = sc_index;

	for (i = 0; i < 6; i++) {
		rule.mac_da.addr[i] = pentry->da[i];
		rule.mac_sa.addr[i] = pentry->sa[i];
		rule.action.tx_sci.addr[i] = pentry->sci[i];
	}
	rule.action.tx_sci.port = (((u16)(pentry->sci[6])) << 8) |
		(u16)(pentry->sci[7]);
	rule.ethtype = pentry->ether_type;
	rule.inner_vlanid = pentry->vlan_id;
	rule.outer_vlanid = pentry->outer_vlanid;
	rule.bc_flag = pentry->bc_flag;
	rule.action.tx_sc_index = pentry->channel;
	rule.action.tx_tci = pentry->tci;
	rule.action.encryption_offset = pentry->offset;
	rule.rule_mask = pentry->rule_mask;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id,
		sc_index, &rule));

	if (rule.action.tx_tci & MACSEC_SCB_EN)
		enable = TRUE;
	else
		enable = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_use_scb_en_set(secy_id, enable));

	if (rule.action.tx_tci & MACSEC_INCLUDE_SCI_EN)
		enable = TRUE;
	else
		enable = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_include_sci_en_set(secy_id, enable));

	if (rule.action.tx_tci & MACSEC_ES_EN)
		enable = TRUE;
	else
		enable = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_use_es_en_set(secy_id, enable));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_class_lut_get(u32 secy_id, u32 sc_index,
	struct secy_tx_class_lut_t *pentry)
{
	struct secy_tx_sc_policy_rule_t rule;
	u32 i = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (pentry == NULL))
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));
	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id,
		sc_index, &rule));

	pentry->valid = rule.rule_valid;

	for (i = 0; i < 6; i++) {
		pentry->da[i] = rule.mac_da.addr[i];
		pentry->sa[i] = rule.mac_sa.addr[i];
		pentry->sci[i] = rule.action.tx_sci.addr[i];
	}
	pentry->sci[6] = (u8)(rule.action.tx_sci.port >> 8);
	pentry->sci[7] = (u8)(rule.action.tx_sci.port & 0xff);

	pentry->ether_type = rule.ethtype;
	pentry->vlan_id = rule.inner_vlanid;
	pentry->outer_vlanid = rule.outer_vlanid;
	pentry->bc_flag = rule.bc_flag;
	pentry->channel = rule.action.tx_sc_index;
	pentry->tci = rule.action.tx_tci;
	pentry->offset = rule.action.encryption_offset;
	pentry->rule_mask = rule.rule_mask;

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sci_set(u32 secy_id, u32 sc_index, u8 *sci, u32 len)
{
	struct secy_tx_sc_policy_rule_t rule;
	u32 i = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (sci == NULL))
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));

	rule.action.tx_sc_index = sc_index;
	rule.rule_valid = TRUE;

	for (i = 0; i < 6; i++)
		rule.action.tx_sci.addr[i] = sci[i];

	rule.action.tx_sci.port = (((u16)sci[6]) << 8) | ((u16)sci[7]);

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sci_get(u32 secy_id, u32 sc_index, u8 *sci, u32 len)
{
	struct secy_tx_sc_policy_rule_t rule;
	u32 i = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (sci == NULL))
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));
	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id,
		sc_index, &rule));

	for (i = 0; i < 6; i++)
		sci[i] = rule.action.tx_sci.addr[i];

	sci[6] = (u8)(rule.action.tx_sci.port >> 8);
	sci[7] = (u8)(rule.action.tx_sci.port & 0xff);

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_protect_frames_en_set(u32 secy_id,
	u32 sc_index, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_FRAME_CTRL,
		SYS_FRAME_PROTECT_EN, enable);
}

g_error_t qca808x_secy_tx_protect_frames_en_get(u32 secy_id,
	u32 sc_index, bool *enable)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_CTRL, SYS_FRAME_PROTECT_EN, &val));

	if (val)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_confidentiality_offset_set(u32 secy_id, u32 sc_index,
	enum secy_cofidentiality_offset_t offset)
{
	struct secy_tx_sc_policy_rule_t rule;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));
	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id,
		sc_index, &rule));

	rule.action.tx_sc_index = sc_index;

	if (offset == SECY_CONFIDENTIALITY_OFFSET_50)
		rule.action.encryption_offset = 50;
	else if (offset == SECY_CONFIDENTIALITY_OFFSET_30)
		rule.action.encryption_offset = 30;
	else
		rule.action.encryption_offset = 0;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id,
		sc_index, &rule));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_confidentiality_offset_get(u32 secy_id, u32 sc_index,
	enum secy_cofidentiality_offset_t *offset)
{
	struct secy_tx_sc_policy_rule_t rule;


	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));
	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));

	if (rule.action.encryption_offset == 50)
		*offset = SECY_CONFIDENTIALITY_OFFSET_50;
	else if (rule.action.encryption_offset == 30)
		*offset = SECY_CONFIDENTIALITY_OFFSET_30;
	else
		*offset = SECY_CONFIDENTIALITY_OFFSET_00;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sak_set(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sak_t *key)
{
	u16 val = 0;
	u16 reg;
	int i, j, reg_offset;
	u32 channel, sa_index;


	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX) ||
			(key == NULL)) {
		return ERROR_PARAM;
	}

	if ((key->len != SAK_LEN_128BITS) && (key->len != SAK_LEN_256BITS) &&
			(key->len != SAK_LEN_LEGACY))
		return ERROR_PARAM;

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;

	if (key->len == SAK_LEN_256BITS) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				val = (key->sak[i * 2 + 1] << 8) |
					key->sak[i * 2];
				reg = MACSEC_TX_SAK_KEY0(channel);
				reg_offset = i;
			} else {
				j = i - 8;
				val = (key->sak1[j * 2 + 1] << 8) |
					key->sak1[j * 2];
				reg = MACSEC_TX_EXTENDED_SAK_KEY0(channel);
				reg_offset = j;
			}
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
				PHY_MMD3, reg + reg_offset, val));
		}
	} else {
		reg = MACSEC_TX_SAK_KEY0(channel);
		for (i = 0; i < 8; i++) {
			val = (key->sak[i * 2 + 1] << 8) | key->sak[i * 2];
			reg_offset = i;
			SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
				PHY_MMD3, reg + reg_offset, val));
		}
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sak_get(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sak_t *key)
{
	u16 val = 0;
	u16 reg;
	int i, j, reg_offset;
	u32 channel, sa_index;
	enum secy_cipher_suite_t cipher_suite;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX) ||
			(key == NULL)) {
		return ERROR_PARAM;
	}
	SHR_RET_ON_ERR(qca808x_secy_cipher_suite_get(secy_id, &cipher_suite));

	if ((cipher_suite == SECY_CIPHER_SUITE_128) ||
			(cipher_suite == SECY_CIPHER_SUITE_XPN_128))
		key->len = 16;
	else
		key->len = 32;

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;
	if (key->len == SAK_LEN_256BITS) {
		for (i = 0; i < 16; i++) {
			if (i < 8) {
				reg = MACSEC_TX_SAK_KEY0(channel);
				reg_offset = i;
				SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
					PHY_MMD3, reg + reg_offset, &val));
				key->sak[i * 2] = val & 0xff;
				key->sak[i * 2 + 1] = (val >> 0x8) & 0xff;
			} else {
				reg = MACSEC_TX_EXTENDED_SAK_KEY0(channel);
				reg_offset = i - 8;
				SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
					PHY_MMD3, reg + reg_offset, &val));
				j = i - 8;
				key->sak1[j * 2] = val & 0xff;
				key->sak1[j * 2 + 1] = (val >> 0x8) & 0xff;
			}
		}
	} else {
		reg = MACSEC_TX_SAK_KEY0(channel);
		for (i = 0; i < 8; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
				PHY_MMD3, reg + i, &val));
			key->sak[i * 2] = val & 0xff;
			key->sak[i * 2 + 1] = (val >> 0x8) & 0xff;
		}
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_tci_7_2_set(u32 secy_id, u32 sc_index, u8 tci)
{
	struct secy_tx_sc_policy_rule_t rule;
	bool enable;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));
	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id,
		sc_index, &rule));

	rule.action.tx_sc_index = sc_index;
	rule.action.tx_tci = (tci << 0x2);
	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id,
		sc_index, &rule));

	if (rule.action.tx_tci & MACSEC_SCB_EN)
		enable = TRUE;
	else
		enable = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_use_scb_en_set(secy_id, enable));

	if (rule.action.tx_tci & MACSEC_INCLUDE_SCI_EN)
		enable = TRUE;
	else
		enable = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_include_sci_en_set(secy_id, enable));

	if (rule.action.tx_tci & MACSEC_ES_EN)
		enable = TRUE;
	else
		enable = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_use_es_en_set(secy_id, enable));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_tci_7_2_get(u32 secy_id, u32 sc_index, u8 *tci)
{
	struct secy_tx_sc_policy_rule_t rule;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));
	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id,
		sc_index, &rule));

	*tci = (rule.action.tx_tci >> 0x2);

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sa_en_set(u32 secy_id, u32 sc_index, u32 an,
	bool enable)
{
	u16 val = 0;
	u16 reg, chan_shift;
	u32 sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/8;
	chan_shift = ((sc_index%8) * 2 + sa_index);

	reg += MACSEC_TX_SA_CONTROL;

	if (enable)
		val = (1 << chan_shift);
	else
		val &= ~(1 << chan_shift);

	return phy_access_reg_mask_write(secy_id, PHY_MMD3,
		reg, (1<<chan_shift), val);
}

g_error_t qca808x_secy_tx_sa_en_get(u32 secy_id, u32 sc_index, u32 an,
	bool *enable)
{
	u16 val = 0;
	u16 reg, chan_shift;
	u32 sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/8;
	chan_shift = ((sc_index%8) * 2 + sa_index);

	reg += MACSEC_TX_SA_CONTROL;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3, reg, &val));

	if (val&(1 << chan_shift))
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sc_an_set(u32 secy_id, u32 sc_index, u32 an)
{
	u16 val = 0;
	u16 reg, chan_shift;
	u32 sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/4;
	chan_shift = 2*((sc_index%4) * 2 + sa_index);

	reg += MACSEC_TX_AN_BASE;

	val = (((u16)an&AN_MASK) << chan_shift);

	return phy_access_reg_mask_write(secy_id, PHY_MMD3, reg,
		(AN_MASK<<chan_shift), val);
}

g_error_t qca808x_secy_tx_sc_an_get(u32 secy_id, u32 sc_index, u32 *an)
{
	u16 val = 0;
	u16 reg, chan_shift;
	bool enable;
	u32 i, an_index = 0, sa_index = 0;

	for (i = 0; i < 4; i++) {
		SHR_RET_ON_ERR(qca808x_secy_tx_sa_en_get(secy_id,
			sc_index, i, &enable));
		if (enable == TRUE)
			an_index = i;
	}

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an_index > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an_index);
	reg = sc_index/4;
	chan_shift = 2*((sc_index%4) * 2 + sa_index);

	reg += MACSEC_TX_AN_BASE;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3, reg, &val));

	*an = (val>>chan_shift)&AN_MASK;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sc_create(u32 secy_id, u32 sc_index)
{
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));

	rule.rule_valid = TRUE;
	rule.action.tx_sc_index = sc_index;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sc_del(u32 secy_id, u32 sc_index)
{
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));


	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));
	rule.rule_valid = FALSE;
	rule.action.tx_sc_index = sc_index;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sc_del_all(u32 secy_id)
{
	u32 sc_index = 0;
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));


	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	for (sc_index = 0; sc_index <= SECY_SC_IDX_MAX; sc_index++) {
		SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id,
			sc_index, &rule));
		rule.rule_valid = FALSE;
		rule.action.tx_sc_index = sc_index;

		SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id,
			sc_index, &rule));
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_class_lut_clear(u32 secy_id, u32 sc_index)
{
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));


	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_class_lut_clear_all(u32 secy_id)
{
	u32 sc_index = 0;
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	for (sc_index = 0; sc_index <= SECY_SC_IDX_MAX; sc_index++)
		SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id,
			sc_index, &rule));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sa_create(u32 secy_id, u32 sc_index, u32 an)
{
	u16 val = 0;
	u16 reg, chan_shift;
	u32 sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/4;
	chan_shift = 2*((sc_index%4) * 2 + sa_index);

	reg += MACSEC_TX_AN_BASE;

	val = (((u16)an&AN_MASK) << chan_shift);

	return phy_access_reg_mask_write(secy_id, PHY_MMD3, reg,
		(AN_MASK << chan_shift), val);
}

g_error_t qca808x_secy_tx_sa_del(u32 secy_id, u32 sc_index, u32 an)
{
	u16 val = 0, chan_shift;
	u32 reg, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	reg = sc_index/4;
	chan_shift = 2*((sc_index%4) * 2 + sa_index);

	reg += MACSEC_TX_AN_BASE;

	val = (0x0 << chan_shift);

	return phy_access_reg_mask_write(secy_id, PHY_MMD3, reg,
		(AN_MASK << chan_shift), val);

}
g_error_t qca808x_secy_tx_sa_del_all(u32 secy_id)
{
	u32 sc_index = 0, an_index = 0;

	for (sc_index = 0; sc_index <= SECY_SC_IDX_MAX; sc_index++) {
		for (an_index = 0; an_index <= SECY_AN_IDX_MAX; an_index++)
			qca808x_secy_tx_sa_del(secy_id, sc_index, an_index);
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sa_in_used_get(u32 secy_id, u32 sc_index,
	u32 an, bool *p_in_used)
{
	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	SHR_RET_ON_ERR(qca808x_secy_tx_sa_en_get(secy_id, sc_index,
		an, p_in_used));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sc_en_set(u32 secy_id, u32 sc_index, bool enable)
{
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));

	rule.action.tx_sc_index = sc_index;

	if (enable == TRUE)
		rule.rule_valid = TRUE;
	else
		rule.rule_valid = FALSE;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;

}
g_error_t qca808x_secy_tx_sc_en_get(u32 secy_id, u32 sc_index, bool *enable)
{
	struct secy_tx_sc_policy_rule_t rule;

	memset(&rule, 0, sizeof(rule));

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));


	if (rule.rule_valid == TRUE)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;

}
g_error_t qca808x_secy_tx_class_lut_sci_update(u32 secy_id, u32 sc_index,
	u8 *sci)
{
	struct secy_tx_sc_policy_rule_t rule;
	u32 i = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (sci == NULL))
		return ERROR_PARAM;

	memset(&rule, 0, sizeof(rule));

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_get(secy_id, sc_index, &rule));

	rule.action.tx_sc_index = sc_index;

	for (i = 0; i < 6; i++)
		rule.action.tx_sci.addr[i] = sci[i];

	rule.action.tx_sci.port = (((u16)sci[6]) << 8) | ((u16)sci[7]);

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_policy_set(secy_id, sc_index, &rule));

	return ERROR_OK;
}

static u32 qca808x_addr(u32 sc_index, u32 sa_index, u32 addr)
{
	u32 reg = 0;

	switch (addr) {
	case RX_SA_UNUSED_PKTS_BASE:
	case RX_SA_NOUSING_PKTS_BASE:
	case RX_SA_NOTVALID_PKTS_BASE:
	case RX_SA_INVALID_PKTS_BASE:
	case RX_SA_OK_PKTS_BASE:
	case TX_SA_PROTECTED_PKTS_BASE:
	case TX_SA_ENCRYPTED_PKTS_BASE:
	case TX_SA_PROTECTED_PKTS_HIGH_BASE:
	case TX_SA_ENCRYPTED_PKTS_HIGH_BASE:
	case RX_SA_UNUSED_PKTS_HIGH_BASE:
	case RX_SA_NOUSING_PKTS_HIGH_BASE:
	case RX_SA_NOTVALID_PKTS_HIGH_BASE:
	case RX_SA_INVALID_PKTS_HIGH_BASE:
	case RX_SA_OK_PKTS_HIGH_BASE:
		reg = addr + (sc_index * 2 + sa_index) * 2;
		break;
	case RX_SC_LATE_PKTS_BASE:
	case RX_SC_DELAYED_PKTS_BASE:
	case RX_SC_UNCHECKED_PKTS_BASE:
	case RX_SC_VALIDATED_PKTS_BASE:
	case RX_SC_DECRYPTED_PKTS_BASE:
	case TX_SC_PROTECTED_OCTETS_BASE:
	case TX_SC_ENCRYPTED_OCTETS_BASE:
		reg = addr + sc_index * 2;
		break;
	default:
		reg = addr;
		break;
	}

	return reg;
}

static g_error_t qca808x_secy_mib_read(u32 secy_id, u32 reg_addr, u32 *mib_val)
{
	u16 val = 0;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		reg_addr + 1, &val));
	*mib_val = val << 16;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		reg_addr, &val));
	*mib_val += val;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sa_mib_get(u32 secy_id, u32 sc_index,
	u32 an, struct secy_tx_sa_mib_t *mib)
{
	u32 mib_low = 0, mib_high = 0;
	u32 sa_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX) || (mib == NULL)) {
		return ERROR_PARAM;
	}

	memset(mib, 0, sizeof(*mib));

	sa_index = SECY_AN_TO_SA_MAPPING(an);

	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_SA_PROTECTED_PKTS_HIGH_BASE), &mib_high));
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_SA_PROTECTED_PKTS_BASE), &mib_low));
	mib->protected_pkts = ((u64)mib_high << 32) | (u64)mib_low;

	mib_low = 0; mib_high = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_SA_ENCRYPTED_PKTS_HIGH_BASE), &mib_high));
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_SA_ENCRYPTED_PKTS_BASE), &mib_low));
	mib->encrypted_pkts = ((u64)mib_high << 32) | (u64)mib_low;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sc_mib_get(u32 secy_id, u32 sc_index,
	struct secy_tx_sc_mib_t *mib)
{
	u32 val = 0;
	u32 sa_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (mib == NULL))
		return ERROR_PARAM;

	memset(mib, 0, sizeof(*mib));

	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_SC_PROTECTED_OCTETS_BASE), &val));
	mib->protected_octets = (u64)val;

	val = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_SC_ENCRYPTED_OCTETS_BASE), &val));
	mib->encrypted_octets = (u64)val;

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_mib_get(u32 secy_id, struct secy_tx_mib_t *mib)
{
	u32 val = 0;
	u32 sc_index = 0, sa_index = 0;

	if (mib == NULL)
		return ERROR_PARAM;

	memset(mib, 0, sizeof(*mib));

	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_UNTAGGED_PKTS), &val));
	mib->untagged_pkts = (u64)val;
	val = 0;

	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			TX_TOO_LONG_PKTS), &val));
	mib->too_long = (u64)val;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_mib_get(u32 secy_id, u32 sc_index,
	u32 an, struct secy_rx_sa_mib_t *mib)
{
	u32 mib_low = 0, mib_high = 0;
	u32 sa_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX) || (mib == NULL)) {
		return ERROR_PARAM;
	}

	memset(mib, 0, sizeof(*mib));

	sa_index = SECY_AN_TO_SA_MAPPING(an);

	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_UNUSED_PKTS_HIGH_BASE), &mib_high));
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_UNUSED_PKTS_BASE), &mib_low));
	mib->unused_sa = ((u64)mib_high << 32) | (u64)mib_low;

	mib_low = 0;
	mib_high = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_NOUSING_PKTS_HIGH_BASE), &mib_high));
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_NOUSING_PKTS_BASE), &mib_low));
	mib->not_using_sa = ((u64)mib_high << 32) | (u64)mib_low;

	mib_low = 0;
	mib_high = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_NOTVALID_PKTS_HIGH_BASE), &mib_high));
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_NOTVALID_PKTS_BASE), &mib_low));
	mib->not_valid_pkts = ((u64)mib_high << 32) | (u64)mib_low;

	mib_low = 0;
	mib_high = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_INVALID_PKTS_HIGH_BASE), &mib_high));
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_INVALID_PKTS_BASE), &mib_low));
	mib->invalid_pkts = ((u64)mib_high << 32) | (u64)mib_low;

	mib_low = 0;
	mib_high = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_OK_PKTS_HIGH_BASE), &mib_high));
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SA_OK_PKTS_BASE), &mib_low));
	mib->ok_pkts = ((u64)mib_high << 32) | (u64)mib_low;

	mib_low = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SC_LATE_PKTS_BASE), &mib_low));
	mib->late_pkts = (u64)mib_low;

	mib_low = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SC_DELAYED_PKTS_BASE), &mib_low));
	mib->delayed_pkts = (u64)mib_low;

	mib_low = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SC_UNCHECKED_PKTS_BASE), &mib_low));
	mib->unchecked_pkts = (u64)mib_low;

	mib_low = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SC_VALIDATED_PKTS_BASE), &mib_low));
	mib->validated_octets = (u64)mib_low;

	mib_low = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_SC_DECRYPTED_PKTS_BASE), &mib_low));
	mib->decrypted_octets = (u64)mib_low;

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_mib_get(u32 secy_id, struct secy_rx_mib_t *mib)
{
	u32 sc_index = 0, sa_index = 0;
	u32 val = 0;

	if (mib == NULL)
		return ERROR_PARAM;

	memset(mib, 0, sizeof(*mib));

	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_UNTAGGED_PKTS), &val));
	mib->untagged_pkts = (u64)val;

	val = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_NO_TAG_PKTS), &val));
	mib->notag_pkts = (u64)val;

	val = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_BAD_TAG_PKTS), &val));
	mib->bad_tag_pkts = (u64)val;

	val = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_UNKNOWN_SCI_PKTS), &val));
	mib->unknown_sci_pkts = (u64)val;

	val = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_NO_SCI_PKTS), &val));
	mib->no_sci_pkts = (u64)val;

	val = 0;
	SHR_RET_ON_ERR(qca808x_secy_mib_read(secy_id,
		qca808x_addr(sc_index, sa_index,
			RX_OVERRUN_PKTS), &val));
	mib->too_long_packets = (u64)val;

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sa_mib_clear(u32 secy_id, u32 sc_index,
	u32 an)
{
	struct secy_tx_sa_mib_t mib;

	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	memset(&mib, 0, sizeof(mib));

	SHR_RET_ON_ERR(qca808x_secy_tx_sa_mib_get(secy_id, sc_index,
		an, &mib));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sc_mib_clear(u32 secy_id, u32 sc_index)
{
	struct secy_tx_sc_mib_t mib;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	memset(&mib, 0, sizeof(mib));

	SHR_RET_ON_ERR(qca808x_secy_tx_sc_mib_get(secy_id, sc_index, &mib));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_mib_clear(u32 secy_id)
{
	struct secy_tx_mib_t mib;

	memset(&mib, 0, sizeof(mib));

	SHR_RET_ON_ERR(qca808x_secy_tx_mib_get(secy_id, &mib));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_mib_clear(u32 secy_id, u32 sc_index,
	u32 an)
{
	struct secy_rx_sa_mib_t mib;


	if ((sc_index > SECY_SC_IDX_MAX) ||
			(an > SECY_AN_IDX_MAX)) {
		return ERROR_PARAM;
	}

	memset(&mib, 0, sizeof(mib));

	SHR_RET_ON_ERR(qca808x_secy_rx_sa_mib_get(secy_id, sc_index,
		an, &mib));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_mib_clear(u32 secy_id)
{
	struct secy_rx_mib_t mib;

	memset(&mib, 0, sizeof(mib));

	SHR_RET_ON_ERR(qca808x_secy_rx_mib_get(secy_id, &mib));

	return ERROR_OK;
}
g_error_t qca808x_secy_xpn_en_set(u32 secy_id, bool enable)
{
	return PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3, MACSEC_SYS_FRAME_CTRL,
		SYS_XPN_ENABLE, enable);
}
g_error_t qca808x_secy_xpn_en_get(u32 secy_id, bool *enable)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_FRAME_CTRL, SYS_XPN_ENABLE, &val));

	if (val)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_sa_xpn_set(u32 secy_id, u32 sc_index,
	u32 an, u32 next_pn)
{
	u16 val = 0;
	u16 channel = 0;
	u32 sa_index = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (an > SECY_AN_IDX_MAX))
		return ERROR_PARAM;

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);
	val = (u16)(next_pn & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_RX_XPN(channel), val));
	val = (u16)((next_pn >> 16) & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_RX_XPN(channel) + 1, val));

	secy_pn_table[secy_id][pn_index].rx_sa_xpn = next_pn;

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_sa_xpn_get(u32 secy_id, u32 sc_index,
	u32 an, u32 *next_pn)
{
	u16 val0 = 0, val1 = 0, channel = 0;
	u32 sa_index = 0, i = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (an > SECY_AN_IDX_MAX))
		return ERROR_PARAM;

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);

	for (i = 0; i < SECY_PN_QUERY_TIMES; i++) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_RX_XPN(channel), &val0));
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_RX_XPN(channel) + 1, &val1));
		*next_pn = (((u32)val1 << 16) | val0);
		if (*next_pn >= secy_pn_table[secy_id][pn_index].rx_sa_xpn) {
			secy_pn_table[secy_id][pn_index].rx_sa_xpn = *next_pn;
			break;
		}
	}
	if (i >= SECY_PN_QUERY_TIMES)
		*next_pn = secy_pn_table[secy_id][pn_index].rx_sa_xpn;

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sa_xpn_set(u32 secy_id, u32 sc_index,
	u32 an, u32 next_pn)
{
	u16 val = 0;
	u16 channel = 0;
	u32 sa_index = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (an > SECY_AN_IDX_MAX))
		return ERROR_PARAM;

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);
	val = (u16)(next_pn & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_TX_XPN(channel), val));
	val = (u16)((next_pn >> 16) & 0xffff);
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_TX_XPN(channel) + 1, val));

	secy_pn_table[secy_id][pn_index].tx_sa_xpn = next_pn;

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sa_xpn_get(u32 secy_id, u32 sc_index,
	u32 an, u32 *next_pn)
{
	u16 val0 = 0, val1 = 0, channel = 0;
	u32 sa_index = 0, i = 0, pn_index = 0;

	if ((sc_index > SECY_SC_IDX_MAX) || (an > SECY_AN_IDX_MAX))
		return ERROR_PARAM;

	pn_index = (sc_index * 4 + an);
	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = (sc_index * 2 + sa_index);

	for (i = 0; i < SECY_PN_QUERY_TIMES; i++) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_TX_XPN(channel), &val0));
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
			MACSEC_TX_XPN(channel) + 1, &val1));
		*next_pn = (((u32)val1 << 16) | val0);
		if (*next_pn >= secy_pn_table[secy_id][pn_index].tx_sa_xpn) {
			secy_pn_table[secy_id][pn_index].tx_sa_xpn = *next_pn;
			break;
		}
	}
	if (i >= SECY_PN_QUERY_TIMES)
		*next_pn = secy_pn_table[secy_id][pn_index].tx_sa_xpn;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_ssci_set(u32 secy_id, u32 sc_index, u32 ssci)
{
	u16 val = 0, reg = 0;
	u32 i;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	reg = MACSEC_SC_RX_SSCI0(sc_index);

	for (i = 0; i < 2; i++) {
		val = (ssci >> (i * 16)) & 0xffff;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
			PHY_MMD3, reg + i, val));
	}

	rx_ssci[secy_id][sc_index] = ssci;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sc_ssci_get(u32 secy_id, u32 sc_index, u32 *ssci)
{
	u16 val = 0, reg = 0, enable;
	u32 i;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_SRAM_DEBUG_EN, &enable));

	if (enable) {
		reg = MACSEC_SC_RX_SSCI0(sc_index);

		*ssci = 0;
		for (i = 0; i < 2; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
				PHY_MMD3, reg + i, &val));
			*ssci |= (((u32)val) << (i * 16));
		}
	} else {
		*ssci = rx_ssci[secy_id][sc_index];
	}
	return ERROR_OK;
}

g_error_t qca808x_secy_tx_sc_ssci_set(u32 secy_id, u32 sc_index, u32 ssci)
{
	u16 val = 0, reg = 0;
	u32 i;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	reg = MACSEC_SC_TX_SSCI0(sc_index);

	for (i = 0; i < 2; i++) {
		val = (ssci >> (i * 16)) & 0xffff;
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
			PHY_MMD3, reg + i, val));
	}

	tx_ssci[secy_id][sc_index] = ssci;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sc_ssci_get(u32 secy_id, u32 sc_index, u32 *ssci)
{
	u16 val = 0, reg = 0, enable;
	u32 i;

	if (sc_index > SECY_SC_IDX_MAX)
		return ERROR_PARAM;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_SRAM_DEBUG_EN, &enable));

	if (enable) {
		reg = MACSEC_SC_TX_SSCI0(sc_index);

		*ssci = 0;
		for (i = 0; i < 2; i++) {
			SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
				PHY_MMD3, reg + i, &val));
			*ssci |= (((u32)val) << (i * 16));

		}
	} else {
		*ssci = tx_ssci[secy_id][sc_index];
	}
	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_ki_set(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sa_ki_t *key_idendifier)
{
	u16 val = 0;
	u16 reg;
	int i;
	u32 channel, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
		(an > SECY_AN_IDX_MAX) ||
		(key_idendifier == NULL)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;
	reg = MACSEC_RX_KI_KEY0(channel);

	for (i = 0; i < 8; i++) {
		val = (key_idendifier->ki[i * 2 + 1] << 8) |
				key_idendifier->ki[i*2];
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
			PHY_MMD3, reg + i, val));
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_sa_ki_get(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sa_ki_t *key_idendifier)
{
	u16 val = 0;
	u16 reg;
	int i;
	u32 channel, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
		(an > SECY_AN_IDX_MAX) ||
		(key_idendifier == NULL)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;
	reg = MACSEC_RX_KI_KEY0(channel);

	for (i = 0; i < 8; i++) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
			PHY_MMD3, reg + i, &val));
		key_idendifier->ki[i*2] = val & 0xff;
		key_idendifier->ki[i * 2 + 1] = (val >> 0x8) & 0xff;
	}

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_sa_ki_set(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sa_ki_t *key_idendifier)
{
	u16 val = 0;
	u16 reg;
	int i;
	u32 channel, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
		(an > SECY_AN_IDX_MAX) ||
		(key_idendifier == NULL)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;
	reg = MACSEC_TX_KI_KEY0(channel);

	for (i = 0; i < 8; i++) {
		val = (key_idendifier->ki[i * 2 + 1] << 8) |
				key_idendifier->ki[i*2];
		SHR_RET_ON_ERR(phy_access_reg_write(secy_id,
			PHY_MMD3, reg + i, val));
	}

	return ERROR_OK;
}


g_error_t qca808x_secy_tx_sa_ki_get(u32 secy_id, u32 sc_index,
	u32 an, struct secy_sa_ki_t *key_idendifier)
{
	u16 val = 0;
	u16 reg;
	int i;
	u32 channel, sa_index;

	if ((sc_index > SECY_SC_IDX_MAX) ||
		(an > SECY_AN_IDX_MAX) ||
		(key_idendifier == NULL)) {
		return ERROR_PARAM;
	}

	sa_index = SECY_AN_TO_SA_MAPPING(an);
	channel = sc_index * 2 + sa_index;
	reg = MACSEC_TX_KI_KEY0(channel);

	for (i = 0; i < 8; i++) {
		SHR_RET_ON_ERR(phy_access_reg_read(secy_id,
			PHY_MMD3, reg + i, &val));
		key_idendifier->ki[i*2] = val & 0xff;
		key_idendifier->ki[i * 2 + 1] = (val >> 0x8) & 0xff;

	}

	return ERROR_OK;
}
g_error_t qca808x_secy_flow_control_en_set(u32 secy_id, bool enable)
{
	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_PAUSE_CTRL_BASE, SYS_E_PAUSE_EN, enable));
	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_PAUSE_CTRL_BASE, SYS_I_PAUSE_EN, enable));

	return ERROR_OK;
}

g_error_t qca808x_secy_flow_control_en_get(u32 secy_id, bool *enable)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_PAUSE_CTRL_BASE, SYS_E_PAUSE_EN, &val));

	if (val)
		*enable = TRUE;
	else
		*enable = FALSE;

	return ERROR_OK;
}
g_error_t qca808x_secy_special_pkt_ctrl_set(u32 secy_id,
	enum secy_packet_type_t packet_type,
	enum secy_packet_action_t action)
{
	u16 val  = 0;

	if (action == PACKET_PLAIN_OR_UPLOAD)
		val = MSB_PLAIN_LSB_UPLOAD;
	else if (action == PACKET_PLAIN_OR_DISCARD)
		val = MSB_PLAIN_LSB_DISCARD;
	else if (action == PACKET_CRYPTO_OR_UPLOAD)
		val = MSB_CRYPTO_LSB_UPLOAD;
	else if (action == PACKET_CRYPTO_OR_DISCARD)
		val = MSB_CRYPTO_LSB_DISCARD;
	else
		return ERROR_PARAM;

	if (packet_type == PACKET_STP) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
			MACSEC_SYS_PACKET_CTRL, SYS_STP_SW, val));
	} else if (packet_type == PACKET_CDP) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
			MACSEC_SYS_PACKET_CTRL, SYS_CDP_SW, val));
	} else if (packet_type == PACKET_LLDP) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
			MACSEC_SYS_PACKET_CTRL, SYS_LLDP_SW, val));
	} else  {
		return ERROR_NOT_SUPPORT;
	}

	return ERROR_OK;
}

g_error_t qca808x_secy_special_pkt_ctrl_get(u32 secy_id,
	enum secy_packet_type_t packet_type,
	enum secy_packet_action_t *action)
{
	u16 val  = 0;

	if (packet_type == PACKET_STP) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
			MACSEC_SYS_PACKET_CTRL, SYS_STP_SW, &val));
	} else if (packet_type == PACKET_CDP) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
			MACSEC_SYS_PACKET_CTRL, SYS_CDP_SW, &val));
	} else if (packet_type == PACKET_LLDP) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
			MACSEC_SYS_PACKET_CTRL, SYS_LLDP_SW, &val));
	} else  {
		return ERROR_NOT_SUPPORT;
	}

	if (val == MSB_PLAIN_LSB_UPLOAD)
		*action = PACKET_PLAIN_OR_UPLOAD;
	else if (val == MSB_PLAIN_LSB_DISCARD)
		*action = PACKET_PLAIN_OR_DISCARD;
	else if (val == MSB_CRYPTO_LSB_UPLOAD)
		*action = PACKET_CRYPTO_OR_UPLOAD;
	else
		*action = PACKET_CRYPTO_OR_DISCARD;

	return ERROR_OK;
}

g_error_t qca808x_secy_udf_ethtype_set(u32 secy_id, bool enable, u16 type)
{
	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_RX_UDEF_ETH_TYPE_EN, enable));
	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_TX_UDEF_ETH_TYPE_EN, enable));
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SYS_RX_ETHTYPE, type));
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_SYS_TX_ETHTYPE, type));

	return ERROR_OK;
}

g_error_t qca808x_secy_udf_ethtype_get(u32 secy_id, bool *enable, u16 *type)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_RX_UDEF_ETH_TYPE_EN, &val));

	if (val)
		*enable = TRUE;
	else
		*enable = FALSE;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_SYS_RX_ETHTYPE, type));

	return ERROR_OK;
}
g_error_t qca808x_secy_forward_az_pattern_en_set(u32 secy_id, bool enable)
{
	if (enable == TRUE) {
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_FORWARD_AZ_PATTERN_EN_CTRL,
			LPI_RCOVER_EN, FALSE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_FORWARD_AZ_PATTERN_EN_CTRL,
			E_FORWARD_PATTERN_EN, TRUE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_FORWARD_AZ_PATTERN_EN_CTRL,
			I_FORWARD_PATTERN_EN, TRUE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_LOCAL_AZ_PATTERN_EN_CTRL,
			E_MAC_LPI_EN, FALSE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_LOCAL_AZ_PATTERN_EN_CTRL,
			I_MAC_LPI_EN, FALSE));
	} else {
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_FORWARD_AZ_PATTERN_EN_CTRL,
			LPI_RCOVER_EN, TRUE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_FORWARD_AZ_PATTERN_EN_CTRL,
			E_FORWARD_PATTERN_EN, FALSE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_FORWARD_AZ_PATTERN_EN_CTRL,
			I_FORWARD_PATTERN_EN, FALSE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_LOCAL_AZ_PATTERN_EN_CTRL,
			E_MAC_LPI_EN, TRUE));
		SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE
			(secy_id, PHY_MMD3, MACSEC_LOCAL_AZ_PATTERN_EN_CTRL,
			I_MAC_LPI_EN, TRUE));
	}
	return ERROR_OK;
}

static g_error_t _qca808x_secy_tx_epr_property_set(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	u16 val = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_E_EPR_CONFIG, E_EPR_PRIORITY, cfg->priority));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_E_EPR_CONFIG, E_EPR_REVERSE, cfg->inverse));

	if (cfg->pattern0 == SECY_FILTER_PATTERN_AND)
		val = SECY_EPR_PATTERN0;
	else if (cfg->pattern0 == SECY_FILTER_PATTERN_OR)
		val = SECY_EPR_PATTERN1;
	else
		val = SECY_EPR_PATTERN2;
	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_E_EPR_CONFIG, E_EPR_PATTERN0, val));

	if (cfg->pattern1 == SECY_FILTER_PATTERN_AND)
		val = SECY_EPR_PATTERN0;
	else if (cfg->pattern1 == SECY_FILTER_PATTERN_OR)
		val = SECY_EPR_PATTERN1;
	else
		val = SECY_EPR_PATTERN2;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_E_EPR_CONFIG, E_EPR_PATTERN1, val));

	if (cfg->pattern2 == SECY_FILTER_PATTERN_AND)
		val = SECY_EPR_PATTERN0;
	else if (cfg->pattern2 == SECY_FILTER_PATTERN_OR)
		val = SECY_EPR_PATTERN1;
	else
		val = SECY_EPR_PATTERN2;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_E_EPR_CONFIG, E_EPR_PATTERN2, val));

	return ERROR_OK;
}

static g_error_t _qca808x_secy_tx_epr_property_get(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	u16 val = 0;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_E_EPR_CONFIG, &val));

	cfg->inverse = (val >> E_EPR_REVERSE_OFFSET) & 0xf;
	cfg->priority = (val >> E_EPR_PRIORITY_OFFSET) & 0x7;
	if (((val >> E_EPR_PATTERN0_OFFSET) & 0x3) == SECY_EPR_PATTERN0)
		cfg->pattern0 = SECY_FILTER_PATTERN_AND;
	else if (((val >> E_EPR_PATTERN0_OFFSET) & 0x3) == SECY_EPR_PATTERN1)
		cfg->pattern0 = SECY_FILTER_PATTERN_OR;
	else
		cfg->pattern0 = SECY_FILTER_PATTERN_XOR;

	if (((val >> E_EPR_PATTERN1_OFFSET) & 0x3) == SECY_EPR_PATTERN0)
		cfg->pattern1 = SECY_FILTER_PATTERN_AND;
	else if (((val >> E_EPR_PATTERN1_OFFSET) & 0x3) == SECY_EPR_PATTERN1)
		cfg->pattern1 = SECY_FILTER_PATTERN_OR;
	else
		cfg->pattern1 = SECY_FILTER_PATTERN_XOR;

	if ((val & 0x3) == SECY_EPR_PATTERN0)
		cfg->pattern2 = SECY_FILTER_PATTERN_AND;
	else if ((val & 0x3) == SECY_EPR_PATTERN1)
		cfg->pattern2 = SECY_FILTER_PATTERN_OR;
	else
		cfg->pattern2 = SECY_FILTER_PATTERN_XOR;

	return ERROR_OK;

}

g_error_t qca808x_secy_tx_udf_ufilt_cfg_set(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	u16 val = FALSE, val1 = FALSE;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_CEPR_EN, &val1));

	if (cfg->enable) {
		val = TRUE;
		val1 = FALSE;
	} else
		val = FALSE;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UEPR_EN, val));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_CEPR_EN, val1));

	SHR_RET_ON_ERR(_qca808x_secy_tx_epr_property_set(secy_id, cfg));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_udf_ufilt_cfg_get(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	u16 val = 0;

	memset(cfg, 0, sizeof(*cfg));

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UEPR_EN, &val));

	if (val)
		cfg->enable = TRUE;
	else
		cfg->enable = FALSE;

	SHR_RET_ON_ERR(_qca808x_secy_tx_epr_property_get(secy_id, cfg));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_udf_cfilt_cfg_set(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	u16 val = 0, val1 = 0;

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UEPR_EN, &val));

	if (cfg->enable) {
		val = FALSE;
		val1 = TRUE;
	} else
		val1 = FALSE;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UEPR_EN, val));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_CEPR_EN, val1));

	SHR_RET_ON_ERR(_qca808x_secy_tx_epr_property_set(secy_id, cfg));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_udf_cfilt_cfg_get(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	u16 val = 0;

	memset(cfg, 0, sizeof(*cfg));

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_CEPR_EN, &val));

	if (val)
		cfg->enable = TRUE;
	else
		cfg->enable = FALSE;

	SHR_RET_ON_ERR(_qca808x_secy_tx_epr_property_get(secy_id, cfg));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_udf_ufilt_cfg_set(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	bool val = FALSE;

	if (cfg->enable)
		val = TRUE;
	else
		val = FALSE;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UIER_EN, val));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_I_EPR_CONFIG, I_EPR_PRIORITY, cfg->priority));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_I_EPR_CONFIG, I_EPR_REVERSE, cfg->inverse));

	if (cfg->pattern0 == SECY_FILTER_PATTERN_AND)
		val = SECY_EPR_PATTERN0;
	else if (cfg->pattern0 == SECY_FILTER_PATTERN_OR)
		val = SECY_EPR_PATTERN1;
	else
		val = SECY_EPR_PATTERN2;
	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_I_EPR_CONFIG, I_EPR_PATTERN0, val));

	if (cfg->pattern1 == SECY_FILTER_PATTERN_AND)
		val = SECY_EPR_PATTERN0;
	else if (cfg->pattern1 == SECY_FILTER_PATTERN_OR)
		val = SECY_EPR_PATTERN1;
	else
		val = SECY_EPR_PATTERN2;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_I_EPR_CONFIG, I_EPR_PATTERN1, val));

	if (cfg->pattern2 == SECY_FILTER_PATTERN_AND)
		val = SECY_EPR_PATTERN0;
	else if (cfg->pattern2 == SECY_FILTER_PATTERN_OR)
		val = SECY_EPR_PATTERN1;
	else
		val = SECY_EPR_PATTERN2;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_I_EPR_CONFIG, I_EPR_PATTERN2, val));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_udf_ufilt_cfg_get(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg)
{
	u16 val = 0;

	memset(cfg, 0, sizeof(*cfg));

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UIER_EN, &val));

	if (val)
		cfg->enable = TRUE;
	else
		cfg->enable = FALSE;

	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_I_EPR_CONFIG, &val));

	cfg->inverse = (val >> I_EPR_REVERSE_OFFSET) & 0xf;
	cfg->priority = (val >> I_EPR_PRIORITY_OFFSET) & 0x7;
	if (((val >> I_EPR_PATTERN0_OFFSET) & 0x3) == SECY_EPR_PATTERN0)
		cfg->pattern0 = SECY_FILTER_PATTERN_AND;
	else if (((val >> I_EPR_PATTERN0_OFFSET) & 0x3) == SECY_EPR_PATTERN1)
		cfg->pattern0 = SECY_FILTER_PATTERN_OR;
	else
		cfg->pattern0 = SECY_FILTER_PATTERN_XOR;

	if (((val >> I_EPR_PATTERN1_OFFSET) & 0x3) == SECY_EPR_PATTERN0)
		cfg->pattern1 = SECY_FILTER_PATTERN_AND;
	else if (((val >> I_EPR_PATTERN1_OFFSET) & 0x3) == SECY_EPR_PATTERN1)
		cfg->pattern1 = SECY_FILTER_PATTERN_OR;
	else
		cfg->pattern1 = SECY_FILTER_PATTERN_XOR;

	if ((val & 0x3) == SECY_EPR_PATTERN0)
		cfg->pattern2 = SECY_FILTER_PATTERN_AND;
	else if ((val & 0x3) == SECY_EPR_PATTERN1)
		cfg->pattern2 = SECY_FILTER_PATTERN_OR;
	else
		cfg->pattern2 = SECY_FILTER_PATTERN_XOR;

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_udf_filt_set(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter)
{
	u16 val = 0;
	u16 val_type = 0, val_offset;

	if (index > SECY_FILTER_MAX_INDEX)
		return ERROR_PARAM;

	val = filter->udf_field0;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_E_EPR_FIELD0(index), val));

	val = filter->udf_field1;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_E_EPR_FIELD1(index), val));

	val = filter->udf_field2;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_E_EPR_FIELD2(index), val));

	val = 0;
	val |= (filter->mask & 0x3f) << E_EPR_MASK_OFFSET;

	if ((filter->type & 0x3) == SECY_FILTER_IP_PACKET)
		val_type = SECY_EPR_IP_PACKET;
	else if ((filter->type & 0x3) == SECY_FILTER_TCP_PACKET)
		val_type = SECY_EPR_TCP_PACKET;
	else
		val_type = SECY_EPR_ANY_PACKET;

	val |= val_type << E_EPR_TYPE_OFFSET;

	val_offset = filter->offset & 0x3f;
	val |= val_offset << E_EPR_OFFSET_OFFSET;

	val |= (filter->operator & 0x1) << E_EPR_OPERATOR_OFFSET;

	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_E_EPR_MSK_TYPE_OP(index), val));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_udf_filt_get(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter)
{
	u16 val = 0;

	memset(filter, 0, sizeof(*filter));

	if (index > SECY_FILTER_MAX_INDEX)
		return ERROR_PARAM;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_E_EPR_FIELD0(index), &val));
	filter->udf_field0 = val;

	val = 0;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_E_EPR_FIELD1(index), &val));
	filter->udf_field1 = val;

	val = 0;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_E_EPR_FIELD2(index), &val));
	filter->udf_field2 = val;

	val = 0;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_E_EPR_MSK_TYPE_OP(index), &val));

	filter->mask = val & 0x3f;

	if (((val >> E_EPR_TYPE_OFFSET) & 0x3) == SECY_EPR_IP_PACKET)
		filter->type = SECY_FILTER_IP_PACKET;
	else if (((val >> E_EPR_TYPE_OFFSET) & 0x3) == SECY_EPR_TCP_PACKET)
		filter->type = SECY_FILTER_TCP_PACKET;
	else
		filter->type = SECY_FILTER_ANY_PACKET;

	filter->offset = (val >> E_EPR_OFFSET_OFFSET) & 0x3f;

	if ((val >> E_EPR_OPERATOR_OFFSET) & 0x1)
		filter->operator = SECY_FILTER_OPERATOR_LESS;
	else
		filter->operator = SECY_FILTER_OPERATOR_EQUAL;

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_udf_filt_set(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter)
{
	u16 val = 0;
	u16 val_type = 0, val_offset;

	if (index > SECY_FILTER_MAX_INDEX)
		return ERROR_PARAM;

	val = filter->udf_field0;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_I_EPR_FIELD0(index), val));

	val = filter->udf_field1;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_I_EPR_FIELD1(index), val));

	val = filter->udf_field2;
	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_I_EPR_FIELD2(index), val));

	val = 0;
	val |= (filter->mask & 0x3f) << I_EPR_MASK_OFFSET;

	if ((filter->type & 0x3) == SECY_FILTER_IP_PACKET)
		val_type = SECY_EPR_IP_PACKET;
	else if ((filter->type & 0x3) == SECY_FILTER_TCP_PACKET)
		val_type = SECY_EPR_TCP_PACKET;
	else
		val_type = SECY_EPR_ANY_PACKET;

	val |= val_type << I_EPR_TYPE_OFFSET;

	val_offset = filter->offset & 0x3f;
	val |= val_offset << I_EPR_OFFSET_OFFSET;

	val |= (filter->operator & 0x1) << I_EPR_OPERATOR_OFFSET;

	SHR_RET_ON_ERR(phy_access_reg_write(secy_id, PHY_MMD3,
		MACSEC_I_EPR_MSK_TYPE_OP(index), val));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_udf_filt_get(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter)
{
	u16 val = 0;

	memset(filter, 0, sizeof(*filter));

	if (index > SECY_FILTER_MAX_INDEX)
		return ERROR_PARAM;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_I_EPR_FIELD0(index), &val));
	filter->udf_field0 = val;

	val = 0;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_I_EPR_FIELD1(index), &val));
	filter->udf_field1 = val;

	val = 0;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_I_EPR_FIELD2(index), &val));
	filter->udf_field2 = val;

	val = 0;
	SHR_RET_ON_ERR(phy_access_reg_read(secy_id, PHY_MMD3,
		MACSEC_I_EPR_MSK_TYPE_OP(index), &val));

	filter->mask = val & 0x3f;

	if (((val >> I_EPR_TYPE_OFFSET) & 0x3) == SECY_EPR_IP_PACKET)
		filter->type = SECY_FILTER_IP_PACKET;
	else if (((val >> I_EPR_TYPE_OFFSET) & 0x3) == SECY_EPR_TCP_PACKET)
		filter->type = SECY_FILTER_TCP_PACKET;
	else
		filter->type = SECY_FILTER_ANY_PACKET;

	filter->offset = (val >> I_EPR_OFFSET_OFFSET) & 0x3f;

	if ((val >> I_EPR_OPERATOR_OFFSET) & 0x1)
		filter->operator = SECY_FILTER_OPERATOR_LESS;
	else
		filter->operator = SECY_FILTER_OPERATOR_EQUAL;

	return ERROR_OK;
}

g_error_t qca808x_secy_ctl_filt_mac_convert(struct secy_ctl_filt_t *secy_filt,
	struct secy_udf_filt_t *filter)
{
	memset(filter, 0, sizeof(*filter));

	filter->udf_field0 = secy_filt->sa_da_addr[0] << 0x8 |
		secy_filt->sa_da_addr[1];
	filter->udf_field1 = secy_filt->sa_da_addr[2] << 0x8 |
		secy_filt->sa_da_addr[3];
	filter->udf_field2 = secy_filt->sa_da_addr[4] << 0x8 |
		secy_filt->sa_da_addr[5];

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_ctl_filt_da_mac_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x0;
	filter.mask = secy_filt->match_mask;

	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_ctl_filt_sa_mac_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x6;
	filter.mask = secy_filt->match_mask;

	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_ctl_filt_ether_type_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter, 0, sizeof(filter));
	memset(&filter1, 0, sizeof(filter1));

	filter.udf_field0 = secy_filt->ether_type_da_range;
	filter.offset = 0xc;
	filter.mask = secy_filt->match_mask & 0x30;

	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_ctl_filt_da_ether_type_set(u32 secy_id,
	u32 index, struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x0;
	filter.mask = secy_filt->match_mask;

	filter1.udf_field0 = secy_filt->ether_type_da_range;
	filter1.offset = 0xc;
	filter1.mask = (secy_filt->match_mask >> 0x8) & 0x30;

	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_ctl_filt_sa_ether_type_set(u32 secy_id,
	u32 index, struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x6;
	filter.mask = secy_filt->match_mask;

	filter1.udf_field0 = secy_filt->ether_type_da_range;
	filter1.offset = 0xc;
	filter1.mask = (secy_filt->match_mask >> 0x8) & 0x30;

	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_ctl_filt_half_da_sa_set(u32 secy_id,
	u32 index, struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter, 0, sizeof(filter));
	memset(&filter1, 0, sizeof(filter1));


	filter.udf_field1 = secy_filt->sa_da_addr[0];
	filter.udf_field2 = secy_filt->sa_da_addr[1] << 0x8 |
		secy_filt->sa_da_addr[2];
	filter.offset = 0x0;
	filter.mask = (secy_filt->match_mask) & 0x7;

	filter1.udf_field1 = secy_filt->sa_da_addr[3];
	filter1.udf_field2 = secy_filt->sa_da_addr[4] << 0x8 |
		secy_filt->sa_da_addr[5];
	filter1.offset = 0x6;
	filter1.mask = (secy_filt->match_mask >> 0x8) & 0x7;

	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_ctl_filt_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	u16 val = 0, val1 = 0;
	g_error_t rv;

	if (index > 0)
		return ERROR_PARAM;

	switch (secy_filt->match_type) {
	case SECY_CTL_COMPARE_DA:
		rv = qca808x_secy_tx_ctl_filt_da_mac_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_SA:
		rv = qca808x_secy_tx_ctl_filt_sa_mac_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_ETHER_TYPE:
		rv = qca808x_secy_tx_ctl_filt_ether_type_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_HALF_DA_SA:
		rv = qca808x_secy_tx_ctl_filt_half_da_sa_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_DA_ETHER_TYPE:
		rv = qca808x_secy_tx_ctl_filt_da_ether_type_set(
			secy_id, index, secy_filt);
		break;
	case SECY_CTL_COMPARE_SA_ETHER_TYPE:
		rv = qca808x_secy_tx_ctl_filt_sa_ether_type_set(
			secy_id, index, secy_filt);
		break;
	default:
		return ERROR_PARAM;
	}

	SHR_RET_ON_ERR(PHY_REG_FIELD_READ(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_CEPR_EN, &val1));

	if (secy_filt->bypass) {
		val = TRUE;
		val1 = FALSE;
	} else
		val = FALSE;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UEPR_EN, val));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_CEPR_EN, val1));

	memcpy(&g_secy_tx_filt[secy_id], secy_filt,
		sizeof(struct secy_ctl_filt_t));

	return ERROR_OK;
}

g_error_t qca808x_secy_tx_ctl_filt_get(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	if (index > 0)
		return ERROR_PARAM;

	memcpy(secy_filt, &g_secy_tx_filt[secy_id],
		sizeof(struct secy_ctl_filt_t));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_ctl_filt_clear(u32 secy_id, u32 index)
{
	u32 i = 0;
	struct secy_udf_filt_t filter;

	if (index > 0)
		return ERROR_PARAM;

	memset(&filter, 0, sizeof(filter));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UEPR_EN, FALSE));

	for (i = 0; i < 4; i++)
		SHR_RET_ON_ERR(qca808x_secy_tx_udf_filt_set(secy_id, i,
			&filter));

	memset(&g_secy_tx_filt[secy_id], 0, sizeof(struct secy_ctl_filt_t));

	return ERROR_OK;
}
g_error_t qca808x_secy_tx_ctl_filt_clear_all(u32 secy_id)
{
	u32 index = 0;

	SHR_RET_ON_ERR(qca808x_secy_tx_ctl_filt_clear(secy_id, index));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_ctl_filt_da_mac_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x0;
	filter.mask = secy_filt->match_mask;

	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_ctl_filt_sa_mac_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x6;
	filter.mask = secy_filt->match_mask;

	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_ctl_filt_ether_type_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter, 0, sizeof(filter));
	memset(&filter1, 0, sizeof(filter1));

	filter.udf_field0 = secy_filt->ether_type_da_range;
	filter.offset = 0xc;
	filter.mask = secy_filt->match_mask & 0x30;

	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_ctl_filt_da_ether_type_set(u32 secy_id,
	u32 index, struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x0;
	filter.mask = secy_filt->match_mask;

	filter1.udf_field0 = secy_filt->ether_type_da_range;
	filter1.offset = 0xc;
	filter1.mask = (secy_filt->match_mask >> 0x8) & 0x30;

	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_ctl_filt_sa_ether_type_set(u32 secy_id,
	u32 index, struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter1, 0, sizeof(filter1));

	SHR_RET_ON_ERR(qca808x_secy_ctl_filt_mac_convert(secy_filt, &filter));

	filter.offset = 0x6;
	filter.mask = secy_filt->match_mask;

	filter1.udf_field0 = secy_filt->ether_type_da_range;
	filter1.offset = 0xc;
	filter1.mask = (secy_filt->match_mask >> 0x8) & 0x30;

	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_ctl_filt_half_da_sa_set(u32 secy_id,
	u32 index, struct secy_ctl_filt_t *secy_filt)
{
	struct secy_udf_filt_t filter, filter1;

	memset(&filter, 0, sizeof(filter));
	memset(&filter1, 0, sizeof(filter1));


	filter.udf_field1 = secy_filt->sa_da_addr[0];
	filter.udf_field2 = secy_filt->sa_da_addr[1] << 0x8 |
		secy_filt->sa_da_addr[2];
	filter.offset = 0x0;
	filter.mask = (secy_filt->match_mask) & 0x7;

	filter1.udf_field1 = secy_filt->sa_da_addr[3];
	filter1.udf_field2 = secy_filt->sa_da_addr[4] << 0x8 |
		secy_filt->sa_da_addr[5];
	filter1.offset = 0x6;
	filter1.mask = (secy_filt->match_mask >> 0x8) & 0x7;

	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 0, &filter));
	SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, 1, &filter1));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_ctl_filt_set(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	bool val = 0;
	g_error_t rv;

	if (index > 0)
		return ERROR_PARAM;

	switch (secy_filt->match_type) {
	case SECY_CTL_COMPARE_DA:
		rv = qca808x_secy_rx_ctl_filt_da_mac_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_SA:
		rv = qca808x_secy_rx_ctl_filt_sa_mac_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_ETHER_TYPE:
		rv = qca808x_secy_rx_ctl_filt_ether_type_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_HALF_DA_SA:
		rv = qca808x_secy_rx_ctl_filt_half_da_sa_set(secy_id,
			index, secy_filt);
		break;
	case SECY_CTL_COMPARE_DA_ETHER_TYPE:
		rv = qca808x_secy_rx_ctl_filt_da_ether_type_set(
			secy_id, index, secy_filt);
		break;
	case SECY_CTL_COMPARE_SA_ETHER_TYPE:
		rv = qca808x_secy_rx_ctl_filt_sa_ether_type_set(
			secy_id, index, secy_filt);
		break;
	default:
		return ERROR_PARAM;
	}

	if (secy_filt->bypass)
		val = TRUE;
	else
		val = FALSE;

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UIER_EN, val));

	memcpy(&g_secy_rx_filt[secy_id], secy_filt,
		sizeof(struct secy_ctl_filt_t));

	return ERROR_OK;
}

g_error_t qca808x_secy_rx_ctl_filt_get(u32 secy_id, u32 index,
	struct secy_ctl_filt_t *secy_filt)
{
	if (index > 0)
		return ERROR_PARAM;

	memcpy(secy_filt, &g_secy_rx_filt[secy_id],
		sizeof(struct secy_ctl_filt_t));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_ctl_filt_clear(u32 secy_id, u32 index)
{
	u32 i = 0;
	struct secy_udf_filt_t filter;

	if (index > 0)
		return ERROR_PARAM;

	memset(&filter, 0, sizeof(filter));

	SHR_RET_ON_ERR(PHY_REG_FIELD_WRITE(secy_id, PHY_MMD3,
		MACSEC_SYS_CONFIG, SYS_UIER_EN, FALSE));

	for (i = 0; i < 4; i++)
		SHR_RET_ON_ERR(qca808x_secy_rx_udf_filt_set(secy_id, i,
			&filter));

	memset(&g_secy_rx_filt[secy_id], 0,
		sizeof(struct secy_ctl_filt_t));

	return ERROR_OK;
}
g_error_t qca808x_secy_rx_ctl_filt_clear_all(u32 secy_id)
{
	u32 index = 0;

	SHR_RET_ON_ERR(qca808x_secy_rx_ctl_filt_clear(secy_id, index));

	return ERROR_OK;
}

g_error_t qca808x_secy_channel_number_get(u32 secy_id, u32 *number)
{
	*number = SECY_SC_IDX_MAX + 1;

	return ERROR_OK;
}

g_error_t qca808x_secy_init(u32 secy_id)
{
	u32 sc, sa;
	u32 pn = 1;

	secy_pn_table[secy_id] = kzalloc(
		SECY_PN_CHANNEL_MAX * sizeof(struct secy_pn_table_t),
			GFP_KERNEL);
	if (secy_pn_table[secy_id] == NULL)
		return -ENOMEM;

	qca808x_secy_clock_en_set(secy_id, TRUE);
	qca808x_secy_tx_mib_clear(secy_id);
	qca808x_secy_rx_mib_clear(secy_id);
	qca808x_secy_forward_az_pattern_en_set(secy_id, FALSE);
	for (sc = 0; sc < SECY_SC_IDX_MAX + 1; sc++) {
		qca808x_secy_tx_sc_mib_clear(secy_id, sc);
		for (sa = 0; sa < SECY_SA_IDX_MAX + 1; sa++) {
			qca808x_secy_tx_sa_mib_clear(secy_id,
				sc, sa);
			qca808x_secy_rx_sa_mib_clear(secy_id,
				sc, sa);
			/*init next pn*/
			qca808x_secy_tx_sa_npn_set(secy_id, sc,
				sa, pn);
			qca808x_secy_rx_sa_npn_set(secy_id, sc,
				sa, pn);
			qca808x_secy_tx_sa_xpn_set(secy_id, sc,
				sa, pn);
			qca808x_secy_rx_sa_xpn_set(secy_id, sc,
				sa, pn);
		}
	}
	return ERROR_OK;
}

static struct secy_drv_t *qca808x_secy_ops;
static int qca808x_secy_ops_init(void)
{

	g_error_t ret;

	qca808x_secy_ops = kzalloc(sizeof(*qca808x_secy_ops), GFP_KERNEL);
	if (qca808x_secy_ops == NULL)
		return -ENOMEM;

	secy_ops_init(NAPA_PHY_CHIP);

	qca808x_secy_ops->secy_init = qca808x_secy_init;
	qca808x_secy_ops->secy_en_set = qca808x_secy_en_set;
	qca808x_secy_ops->secy_en_get = qca808x_secy_en_get;
	qca808x_secy_ops->secy_mtu_set = qca808x_secy_mtu_set;
	qca808x_secy_ops->secy_mtu_get = qca808x_secy_mtu_get;
	qca808x_secy_ops->secy_cipher_suite_set =
		qca808x_secy_cipher_suite_set;
	qca808x_secy_ops->secy_cipher_suite_get =
		qca808x_secy_cipher_suite_get;
	qca808x_secy_ops->secy_controlled_port_en_set =
		qca808x_secy_controlled_port_en_set;
	qca808x_secy_ops->secy_controlled_port_en_get =
		qca808x_secy_controlled_port_en_get;
	qca808x_secy_ops->secy_sc_sa_mapping_mode_set =
		qca808x_secy_sc_sa_mapping_mode_set;
	qca808x_secy_ops->secy_xpn_en_set =
		qca808x_secy_xpn_en_set;
	qca808x_secy_ops->secy_xpn_en_get =
		qca808x_secy_xpn_en_get;
	qca808x_secy_ops->secy_flow_control_en_set =
		qca808x_secy_flow_control_en_set;
	qca808x_secy_ops->secy_flow_control_en_get =
		qca808x_secy_flow_control_en_get;
	qca808x_secy_ops->secy_special_pkt_ctrl_set =
		qca808x_secy_special_pkt_ctrl_set;
	qca808x_secy_ops->secy_special_pkt_ctrl_get =
		qca808x_secy_special_pkt_ctrl_get;
	qca808x_secy_ops->secy_udf_ethtype_set =
		qca808x_secy_udf_ethtype_set;
	qca808x_secy_ops->secy_udf_ethtype_get =
		qca808x_secy_udf_ethtype_get;
	qca808x_secy_ops->secy_loopback_set =
		qca808x_secy_loopback_set;
	qca808x_secy_ops->secy_loopback_get =
		qca808x_secy_loopback_get;
	qca808x_secy_ops->secy_channel_number_get =
		qca808x_secy_channel_number_get;

	/*rx*/
	qca808x_secy_ops->secy_rx_prc_lut_set = qca808x_secy_rx_prc_lut_set;
	qca808x_secy_ops->secy_rx_prc_lut_get = qca808x_secy_rx_prc_lut_get;
	qca808x_secy_ops->secy_rx_sc_create = qca808x_secy_rx_sc_create;
	qca808x_secy_ops->secy_rx_sc_del = qca808x_secy_rx_sc_del;
	qca808x_secy_ops->secy_rx_sc_del_all = qca808x_secy_rx_sc_del_all;
	qca808x_secy_ops->secy_rx_sc_validate_frame_set =
		qca808x_secy_rx_sc_validate_frames_set;
	qca808x_secy_ops->secy_rx_sc_validate_frame_get =
		qca808x_secy_rx_sc_validate_frames_get;
	qca808x_secy_ops->secy_rx_sc_replay_protect_set =
		qca808x_secy_rx_sc_replay_protect_en_set;
	qca808x_secy_ops->secy_rx_sc_replay_protect_get =
		qca808x_secy_rx_sc_replay_protect_en_get;
	qca808x_secy_ops->secy_rx_sc_anti_replay_window_set =
		qca808x_secy_rx_sc_replay_window_set;
	qca808x_secy_ops->secy_rx_sc_anti_replay_window_get =
		qca808x_secy_rx_sc_replay_window_get;
	qca808x_secy_ops->secy_rx_sa_create = qca808x_secy_rx_sa_create;
	qca808x_secy_ops->secy_rx_sak_set = qca808x_secy_rx_sak_set;
	qca808x_secy_ops->secy_rx_sak_get = qca808x_secy_rx_sak_get;
	qca808x_secy_ops->secy_rx_sa_en_set = qca808x_secy_rx_sa_en_set;
	qca808x_secy_ops->secy_rx_sa_en_get = qca808x_secy_rx_sa_en_get;
	qca808x_secy_ops->secy_rx_sc_in_used_get =
		qca808x_secy_rx_sc_in_used_get;
	qca808x_secy_ops->secy_rx_sa_next_pn_get =
		qca808x_secy_rx_sa_npn_get;
	qca808x_secy_ops->secy_rx_reg_set = phy_mii_reg_write;
	qca808x_secy_ops->secy_rx_reg_get = phy_mii_reg_read;
	qca808x_secy_ops->secy_rx_sa_next_xpn_set = qca808x_secy_rx_sa_xpn_set;
	qca808x_secy_ops->secy_rx_sa_next_xpn_get = qca808x_secy_rx_sa_xpn_get;
	qca808x_secy_ops->secy_rx_sc_ssci_set = qca808x_secy_rx_sc_ssci_set;
	qca808x_secy_ops->secy_rx_sc_ssci_get = qca808x_secy_rx_sc_ssci_get;
	qca808x_secy_ops->secy_rx_sa_ki_set = qca808x_secy_rx_sa_ki_set;
	qca808x_secy_ops->secy_rx_sa_ki_get = qca808x_secy_rx_sa_ki_get;
	qca808x_secy_ops->secy_rx_prc_lut_clear = qca808x_secy_rx_prc_lut_clear;
	qca808x_secy_ops->secy_rx_prc_lut_clear_all =
		qca808x_secy_rx_prc_lut_clear_all;
	qca808x_secy_ops->secy_rx_validate_frame_set =
		qca808x_secy_rx_validate_frames_set;
	qca808x_secy_ops->secy_rx_validate_frame_get =
		qca808x_secy_rx_validate_frames_get;
	qca808x_secy_ops->secy_rx_replay_protect_set =
		qca808x_secy_rx_replay_protect_en_set;
	qca808x_secy_ops->secy_rx_replay_protect_get =
		qca808x_secy_rx_replay_protect_en_get;
	qca808x_secy_ops->secy_rx_sa_del = qca808x_secy_rx_sa_del;
	qca808x_secy_ops->secy_rx_sa_del_all = qca808x_secy_rx_sa_del_all;
	qca808x_secy_ops->secy_rx_sa_in_used_get =
		qca808x_secy_rx_sa_in_used_get;
	qca808x_secy_ops->secy_rx_sc_en_set = qca808x_secy_rx_sc_en_set;
	qca808x_secy_ops->secy_rx_sc_en_get = qca808x_secy_rx_sc_en_get;
	qca808x_secy_ops->secy_rx_sa_next_pn_set = qca808x_secy_rx_sa_npn_set;

	/*tx */
	qca808x_secy_ops->secy_tx_sc_in_used_get =
	qca808x_secy_tx_sc_in_used_get;
	qca808x_secy_ops->secy_tx_sa_next_pn_set = qca808x_secy_tx_sa_npn_set;
	qca808x_secy_ops->secy_tx_sa_next_pn_get = qca808x_secy_tx_sa_npn_get;
	qca808x_secy_ops->secy_tx_sc_tci_7_2_set = qca808x_secy_tx_tci_7_2_set;
	qca808x_secy_ops->secy_tx_sc_tci_7_2_get = qca808x_secy_tx_tci_7_2_get;
	qca808x_secy_ops->secy_tx_sc_an_set = qca808x_secy_tx_sc_an_set;
	qca808x_secy_ops->secy_tx_sc_an_get = qca808x_secy_tx_sc_an_get;
	qca808x_secy_ops->secy_tx_sa_en_set = qca808x_secy_tx_sa_en_set;
	qca808x_secy_ops->secy_tx_sa_en_get = qca808x_secy_tx_sa_en_get;
	qca808x_secy_ops->secy_tx_sc_create = qca808x_secy_tx_sci_set;
	qca808x_secy_ops->secy_tx_sc_sci_get = qca808x_secy_tx_sci_get;
	qca808x_secy_ops->secy_tx_sc_del = qca808x_secy_tx_sc_del;
	qca808x_secy_ops->secy_tx_sc_del_all = qca808x_secy_tx_sc_del_all;
	qca808x_secy_ops->secy_tx_sc_confidentiality_offset_set =
		qca808x_secy_tx_confidentiality_offset_set;
	qca808x_secy_ops->secy_tx_sc_confidentiality_offset_get =
		qca808x_secy_tx_confidentiality_offset_get;
	qca808x_secy_ops->secy_tx_class_lut_set =
		qca808x_secy_tx_class_lut_set;
	qca808x_secy_ops->secy_tx_class_lut_get =
		qca808x_secy_tx_class_lut_get;
	qca808x_secy_ops->secy_tx_sak_set = qca808x_secy_tx_sak_set;
	qca808x_secy_ops->secy_tx_sak_get = qca808x_secy_tx_sak_get;
	qca808x_secy_ops->secy_tx_sc_protect_set =
		qca808x_secy_tx_protect_frames_en_set;
	qca808x_secy_ops->secy_tx_sc_protect_get =
		qca808x_secy_tx_protect_frames_en_get;
	qca808x_secy_ops->secy_tx_reg_set = phy_mii_reg_write;
	qca808x_secy_ops->secy_tx_reg_get = phy_mii_reg_read;
	qca808x_secy_ops->secy_tx_sa_next_xpn_set = qca808x_secy_tx_sa_xpn_set;
	qca808x_secy_ops->secy_tx_sa_next_xpn_get = qca808x_secy_tx_sa_xpn_get;
	qca808x_secy_ops->secy_tx_sc_ssci_set = qca808x_secy_tx_sc_ssci_set;
	qca808x_secy_ops->secy_tx_sc_ssci_get = qca808x_secy_tx_sc_ssci_get;
	qca808x_secy_ops->secy_tx_sa_ki_set = qca808x_secy_tx_sa_ki_set;
	qca808x_secy_ops->secy_tx_sa_ki_get = qca808x_secy_tx_sa_ki_get;
	qca808x_secy_ops->secy_tx_class_lut_clear =
		qca808x_secy_tx_class_lut_clear;
	qca808x_secy_ops->secy_tx_class_lut_clear_all =
		qca808x_secy_tx_class_lut_clear_all;
	qca808x_secy_ops->secy_tx_sa_create = qca808x_secy_tx_sa_create;
	qca808x_secy_ops->secy_tx_sa_del = qca808x_secy_tx_sa_del;
	qca808x_secy_ops->secy_tx_sa_del_all = qca808x_secy_tx_sa_del_all;
	qca808x_secy_ops->secy_tx_sa_in_used_get =
		qca808x_secy_tx_sa_in_used_get;
	qca808x_secy_ops->secy_tx_sc_en_set = qca808x_secy_tx_sc_en_set;
	qca808x_secy_ops->secy_tx_sc_en_get = qca808x_secy_tx_sc_en_get;
	qca808x_secy_ops->secy_tx_class_lut_sci_update =
		qca808x_secy_tx_class_lut_sci_update;


	/*mib*/
	qca808x_secy_ops->secy_tx_sa_mib_get = qca808x_secy_tx_sa_mib_get;
	qca808x_secy_ops->secy_tx_sc_mib_get = qca808x_secy_tx_sc_mib_get;
	qca808x_secy_ops->secy_tx_mib_get = qca808x_secy_tx_mib_get;
	qca808x_secy_ops->secy_rx_sa_mib_get = qca808x_secy_rx_sa_mib_get;
	qca808x_secy_ops->secy_rx_mib_get = qca808x_secy_rx_mib_get;
	qca808x_secy_ops->secy_tx_sa_mib_clear = qca808x_secy_tx_sa_mib_clear;
	qca808x_secy_ops->secy_tx_sc_mib_clear = qca808x_secy_tx_sc_mib_clear;
	qca808x_secy_ops->secy_tx_mib_clear = qca808x_secy_tx_mib_clear;
	qca808x_secy_ops->secy_rx_sa_mib_clear = qca808x_secy_rx_sa_mib_clear;
	qca808x_secy_ops->secy_rx_mib_clear = qca808x_secy_rx_mib_clear;

	/*filter */
	qca808x_secy_ops->secy_tx_udf_ufilt_cfg_set =
		qca808x_secy_tx_udf_ufilt_cfg_set;
	qca808x_secy_ops->secy_tx_udf_ufilt_cfg_get =
		qca808x_secy_tx_udf_ufilt_cfg_get;
	qca808x_secy_ops->secy_tx_udf_cfilt_cfg_set =
		qca808x_secy_tx_udf_cfilt_cfg_set;
	qca808x_secy_ops->secy_tx_udf_cfilt_cfg_get =
		qca808x_secy_tx_udf_cfilt_cfg_get;
	qca808x_secy_ops->secy_rx_udf_ufilt_cfg_set =
		qca808x_secy_rx_udf_ufilt_cfg_set;
	qca808x_secy_ops->secy_rx_udf_ufilt_cfg_get =
		qca808x_secy_rx_udf_ufilt_cfg_get;
	qca808x_secy_ops->secy_tx_udf_filt_set =
		qca808x_secy_tx_udf_filt_set;
	qca808x_secy_ops->secy_tx_udf_filt_get =
		qca808x_secy_tx_udf_filt_get;
	qca808x_secy_ops->secy_rx_udf_filt_set =
		qca808x_secy_rx_udf_filt_set;
	qca808x_secy_ops->secy_rx_udf_filt_get =
		qca808x_secy_rx_udf_filt_get;

	qca808x_secy_ops->secy_tx_ctl_filt_set =
		qca808x_secy_tx_ctl_filt_set;
	qca808x_secy_ops->secy_tx_ctl_filt_get =
		qca808x_secy_tx_ctl_filt_get;
	qca808x_secy_ops->secy_tx_ctl_filt_clear =
		qca808x_secy_tx_ctl_filt_clear;
	qca808x_secy_ops->secy_tx_ctl_filt_clear_all =
		qca808x_secy_tx_ctl_filt_clear_all;
	qca808x_secy_ops->secy_rx_ctl_filt_set =
		qca808x_secy_rx_ctl_filt_set;
	qca808x_secy_ops->secy_rx_ctl_filt_get =
		qca808x_secy_rx_ctl_filt_get;
	qca808x_secy_ops->secy_rx_ctl_filt_clear =
		qca808x_secy_rx_ctl_filt_clear;
	qca808x_secy_ops->secy_rx_ctl_filt_clear_all =
		qca808x_secy_rx_ctl_filt_clear_all;

	ret = secy_ops_register(NAPA_PHY_CHIP, qca808x_secy_ops);

	return ret;
}

int qca808x_secy_driver_init(u32 secy_id)
{
	static u32 secy_ops_flag;

	if (secy_ops_flag == 0) {
		qca808x_secy_ops_init();
		secy_ops_flag = 1;
	}

	qca808x_secy_init(secy_id);

	return 0;
}
int qca808x_secy_driver_exit(u32 secy_id)
{

	secy_ops_unregister(NAPA_PHY_CHIP);

	if (qca808x_secy_ops != NULL) {
		kfree(qca808x_secy_ops);
		qca808x_secy_ops = NULL;
	}

	if (secy_pn_table[secy_id] != NULL) {
		kfree(secy_pn_table[secy_id]);
		secy_pn_table[secy_id] = NULL;
	}

	return 0;
}

