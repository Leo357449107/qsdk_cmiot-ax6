/*
 * Copyright (c) 2014, 2018, The Linux Foundation. All rights reserved.
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

#include "nss_macsec_types.h"
#include "nss_macsec_register_api.h"
#include "nss_macsec.h"
#include "nss_macsec_secy.h"
#include "nss_macsec_utility.h"
#include "nss_macsec_secy_rx.h"
#include "nss_macsec_ops.h"

u32 nss_macsec_secy_rx_reg_get(u32 secy_id, u32 addr, u32 *pvalue)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_reg_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_reg_get(secy_id, addr,
				(u16 *)pvalue);
		return rv;
	}

	return ingress_reg_get(secy_id, addr, pvalue);
}

u32 nss_macsec_secy_rx_reg_set(u32 secy_id, u32 addr, u32 value)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_reg_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_reg_set(secy_id, addr,
				(u16)value);
		return rv;
	}

	return ingress_reg_set(secy_id, addr, value);
}

u32 nss_macsec_secy_rx_ctl_filt_get(u32 secy_id, u32 filt_id,
				    fal_rx_ctl_filt_t *pfilt)
{
	struct ig_ctl_filt ctl_entry;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_ctl_filt_t secy_ctl_filt;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_CTL_FILT_NUM) && (pfilt != NULL));

	memset(&secy_ctl_filt, 0, sizeof(secy_ctl_filt));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_ctl_filt_get(secy_id, filt_id,
				&secy_ctl_filt);
			pfilt->bypass = secy_ctl_filt.bypass;
			pfilt->match_type =
			(fal_ig_ctl_match_type_e)secy_ctl_filt.match_type;
			pfilt->match_mask = secy_ctl_filt.match_mask;
			pfilt->ether_type_da_range =
				secy_ctl_filt.ether_type_da_range;
			memcpy(pfilt->sa_da_addr, secy_ctl_filt.sa_da_addr,
				sizeof(secy_ctl_filt.sa_da_addr));
		}
		return rv;
	}

	SHR_RET_ON_ERR(ig_ctl_filt_table_get
		       (secy_id, filt_id, (u32 *) (&ctl_entry)));

	memset(pfilt, 0, sizeof(fal_rx_ctl_filt_t));
	pfilt->bypass = !(ctl_entry.action);
	pfilt->match_type = ctl_entry.match_type;
	pfilt->match_mask = ctl_entry.match_mask;
	pfilt->ether_type_da_range = ctl_entry.ethertype_da_range;

	pfilt->sa_da_addr[5] = (u8)(ctl_entry.sa_da_0);
	pfilt->sa_da_addr[4] = (u8)(ctl_entry.sa_da_0 >> 8);
	pfilt->sa_da_addr[3] = (u8)(ctl_entry.sa_da_0 >> 16);
	pfilt->sa_da_addr[2] = (u8)(ctl_entry.sa_da_0 >> 24);
	pfilt->sa_da_addr[1] = (u8)(ctl_entry.sa_da_1);
	pfilt->sa_da_addr[0] = (u8)(ctl_entry.sa_da_1 >> 8);

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_ctl_filt_set(u32 secy_id, u32 filt_id,
				    fal_rx_ctl_filt_t *pfilt)
{
	struct ig_ctl_filt ctl_entry;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_ctl_filt_t secy_ctl_filt;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_CTL_FILT_NUM) && (pfilt != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_ctl_filt.bypass = pfilt->bypass;
			secy_ctl_filt.match_type =
				(enum secy_ctl_match_type_t)pfilt->match_type;
			secy_ctl_filt.match_mask = pfilt->match_mask;
			secy_ctl_filt.ether_type_da_range =
				pfilt->ether_type_da_range;
			memcpy(secy_ctl_filt.sa_da_addr, pfilt->sa_da_addr,
				sizeof(pfilt->sa_da_addr));
			rv = secy_drv->secy_rx_ctl_filt_set(secy_id, filt_id,
				&secy_ctl_filt);
		}
		return rv;
	}

	memset(&ctl_entry, 0, sizeof(ctl_entry));

	//tiger: other fields

	ctl_entry.action = !(pfilt->bypass);
	ctl_entry.match_type = pfilt->match_type;
	ctl_entry.match_mask = pfilt->match_mask;
	ctl_entry.ethertype_da_range = pfilt->ether_type_da_range;
	ctl_entry.sa_da_0 = (pfilt->sa_da_addr[2]<<24) |
		(pfilt->sa_da_addr[3]<<16) |
		(pfilt->sa_da_addr[4]<<8) |
		pfilt->sa_da_addr[5];
	ctl_entry.sa_da_1 = (pfilt->sa_da_addr[0] << 8) | pfilt->sa_da_addr[1];

	SHR_RET_ON_ERR(ig_ctl_filt_table_set
		       (secy_id, filt_id, (u32 *) (&ctl_entry)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_ctl_filt_clear(u32 secy_id, u32 filt_id)
{
	struct ig_ctl_filt ctl_entry;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM)
			&& (filt_id < FAL_RX_CTL_FILT_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_clear == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_ctl_filt_clear(secy_id,
				filt_id);
		}
		return rv;
	}

	memset(&ctl_entry, 0, sizeof(union ig_ctl_filt_u));

	SHR_RET_ON_ERR(ig_ctl_filt_table_set
		       (secy_id, filt_id, (u32 *) (&ctl_entry)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_ctl_filt_clear_all(u32 secy_id)
{
	int i;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK(secy_id < FAL_SECY_ID_NUM);

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_ctl_filt_clear_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_ctl_filt_clear_all(secy_id);
		return rv;
	}

	for (i = 0; i < FAL_RX_CTL_FILT_NUM; i++) {
		nss_macsec_secy_rx_ctl_filt_clear(secy_id, i);
	}

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_prc_lut_get(u32 secy_id, u32 index,
				   fal_rx_prc_lut_t *pentry)
{
	struct ig_prc_lut igc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_rx_prc_lut_t secy_rx_prc_lut;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(index < FAL_RX_CLASS_LUT_NUM) && (pentry != NULL));

	memset(pentry, 0, sizeof(fal_rx_prc_lut_t));
	memset(&secy_rx_prc_lut, 0, sizeof(secy_rx_prc_lut));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_prc_lut_get(secy_id, index,
				&secy_rx_prc_lut);

			*pentry = *(fal_rx_prc_lut_t *)&secy_rx_prc_lut;
		}
		return rv;
	}

	SHR_RET_ON_ERR(ig_prc_lut_table_get(secy_id, index, (u32 *)(&igc)));

	//tiger: more fields

	pentry->valid = igc.valid;
	pentry->action = igc.action;
	pentry->channel = fal_rx_sc_id_2_channel(secy_id, igc.sc_index);

	pentry->sci_mask = igc.sci_mask;
	pentry->sci[0] = igc.sci_1 >> 24;
	pentry->sci[1] = igc.sci_1 >> 16;
	pentry->sci[2] = igc.sci_1 >> 8;
	pentry->sci[3] = igc.sci_1;
	pentry->sci[4] = igc.sci_0 >> 24;
	pentry->sci[5] = igc.sci_0 >> 16;
	pentry->sci[6] = igc.sci_0 >> 8;
	pentry->sci[7] = igc.sci_0;

	pentry->tci_mask = igc.tci_7_2_mask;
	pentry->tci = igc.tci;

	pentry->da_mask = igc.da_mask;
	pentry->da[5] = (u8)(igc.da_0);
	pentry->da[4] = (u8)(igc.da_0 >> 8);
	pentry->da[3] = (u8)(igc.da_1);
	pentry->da[2] = (u8)(igc.da_1 >> 8);
	pentry->da[1] = (u8)(igc.da_1 >> 16);
	pentry->da[0] = (u8)(igc.da_1 >> 24);

	pentry->sa_mask = (igc.sa_mask_0 << 2) + igc.sa_mask_1;
	pentry->sa[5] = (u8)(igc.sa_0);
	pentry->sa[4] = (u8)(igc.sa_0 >> 8);
	pentry->sa[3] = (u8)(igc.sa_0 >> 16);
	pentry->sa[2] = (u8)(igc.sa_0 >> 24);
	pentry->sa[1] = (u8)(igc.sa_1);
	pentry->sa[0] = (u8)(igc.sa_1 >> 8);

	pentry->ether_type_mask = igc.ethertype_mask;
	pentry->ether_type = igc.ethertype;
	pentry->uncontrolled_port = igc.uncontrolled_port;
	pentry->offset = igc.encryption_offset;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_prc_lut_set(u32 secy_id, u32 index,
				   fal_rx_prc_lut_t *pentry)
{
	struct ig_prc_lut igc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_rx_prc_lut_t *secy_rx_prc_lut = NULL;

	memset(&igc, 0, sizeof(igc));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(index < FAL_RX_CLASS_LUT_NUM) && (pentry != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_rx_prc_lut = (struct secy_rx_prc_lut_t *)pentry;
			rv = secy_drv->secy_rx_prc_lut_set(secy_id, index,
				secy_rx_prc_lut);
		}
		return rv;
	}

	//tiger: other fields

	igc.valid = (pentry->valid) ? 1 : 0;
	igc.action = pentry->action;
	igc.sc_index = fal_rx_channel_2_sc_id(secy_id, pentry->channel);

	igc.sci_mask = pentry->sci_mask;
	igc.sci_0 = (pentry->sci[4] << 24) |
		(pentry->sci[5] << 16) |
		(pentry->sci[6] << 8) |
		(pentry->sci[7]);
	igc.sci_1 = (pentry->sci[0] << 24) |
		(pentry->sci[1] << 16) |
		(pentry->sci[2] << 8) |
		(pentry->sci[3]);

	igc.tci_7_2_mask = pentry->tci_mask & 0x3f;
	igc.tci = pentry->tci;

	igc.da_mask = pentry->da_mask & 0x3f;
	igc.da_0 = (pentry->da[4] << 8)| pentry->da[5];
	igc.da_1 = (pentry->da[0]<<24) | (pentry->da[1]<<16) |
		(pentry->da[2]<<8) | pentry->da[3];

	igc.sa_mask_0 = (pentry->sa_mask >> 2) & 0xf;
	igc.sa_mask_1 = pentry->sa_mask & 0x3;
	igc.sa_0 = (pentry->sa[2]<<24) | (pentry->sa[3]<<16) |
		(pentry->sa[4]<<8) | pentry->sa[5];
	igc.sa_1 = (pentry->sa[0] << 8) | pentry->sa[1];

	igc.ethertype_mask = pentry->ether_type_mask & 0x3;
	igc.ethertype = pentry->ether_type;

	igc.uncontrolled_port = pentry->uncontrolled_port;
	igc.encryption_offset = pentry->offset;

	if (FAL_SECY_SC_SA_MODE(secy_id) == FAL_SC_SA_MAP_1_1)
		igc.an_mask= 0x3;
	else if (FAL_SECY_SC_SA_MODE(secy_id) == FAL_SC_SA_MAP_1_2)
		igc.an_mask = 0x2;
	else if (FAL_SECY_SC_SA_MODE(secy_id) == FAL_SC_SA_MAP_1_4)
		igc.an_mask = 0x0;

	SHR_RET_ON_ERR(ig_prc_lut_table_set(secy_id, index, (u32 *) (&igc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_prc_lut_clear(u32 secy_id, u32 index)
{
	struct ig_prc_lut igc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(index < FAL_RX_CLASS_LUT_NUM));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_clear == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_prc_lut_clear(secy_id, index);
		return rv;
	}

	memset(&igc, 0, sizeof(igc));

	SHR_RET_ON_ERR(ig_prc_lut_table_set(secy_id, index, (u32 *) (&igc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_prc_lut_clear_all(u32 secy_id)
{
	int i;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_prc_lut_clear_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_prc_lut_clear_all(secy_id);
		return rv;
	}

	for (i = 0; i < FAL_RX_CLASS_LUT_NUM; i++) {
		nss_macsec_secy_rx_prc_lut_clear(secy_id, i);
	}

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_create(u32 secy_id, u32 channel)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_create == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_create(secy_id, channel);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	memset(&sc, 0, sizeof(sc));
	sc.valid = 1;
	//tiger: other fields

	SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *) (&sc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_en_get(u32 secy_id, u32 channel, bool *penable)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_en_get(secy_id, channel,
				penable);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *) (&sc)));

	*penable = sc.valid;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_en_set(u32 secy_id, u32 channel, bool enable)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_en_set(secy_id, channel,
				enable);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *) (&sc)));

	if (sc.valid != (enable ? 1 : 0)) {
		sc.valid = (enable ? 1 : 0);
		SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *) (&sc)));
	}

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_del(u32 secy_id, u32 channel)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_del == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_del(secy_id, channel);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	memset(&sc, 0, sizeof(sc));
	SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *) (&sc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_del_all(u32 secy_id)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	int ret = 0;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK(secy_id < FAL_SECY_ID_NUM);

	memset(&sc, 0, sizeof(sc));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_del_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_del_all(secy_id);
		return rv;
	}

	for (sc_id = IG_SC_INDEX_MIN; sc_id <= IG_SC_INDEX_MAX; sc_id++) {
		ret += ig_sc_table_set(secy_id, sc_id, (u32 *) (&sc));
	}

	return ret;
}

u32 nss_macsec_secy_rx_sc_validate_frame_get(u32 secy_id, u32 channel,
					     fal_rx_sc_validate_frame_e *pmode)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(pmode != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_validate_frame_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_sc_validate_frame_get(secy_id,
				channel, &secy_vf_mode);
			if (secy_vf_mode == STRICT)
				*pmode = FAL_RX_SC_VALIDATE_FRAME_STRICT;
			else if (secy_vf_mode == CHECKED)
				*pmode = FAL_RX_SC_VALIDATE_FRAME_CHECK;
			else if (secy_vf_mode == DISABLED)
				*pmode = FAL_RX_SC_VALIDATE_FRAME_DISABLED;
			else
				rv = ERROR_NOT_SUPPORT;
		}
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *) (&sc)));

	*pmode = sc.validate_frames;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_validate_frame_set(u32 secy_id, u32 channel,
					     fal_rx_sc_validate_frame_e mode)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode = STRICT;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(mode < FAL_RX_SC_VALIDATE_FRAME_MAX));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_validate_frame_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (mode == FAL_RX_SC_VALIDATE_FRAME_STRICT)
				secy_vf_mode = STRICT;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_CHECK)
				secy_vf_mode = CHECKED;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_DISABLED)
				secy_vf_mode = DISABLED;
			else
				return ERROR_NOT_SUPPORT;
			rv = secy_drv->secy_rx_sc_validate_frame_set(secy_id,
				channel, secy_vf_mode);
		}
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	sc.validate_frames = mode;
	SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *)(&sc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_replay_protect_get(u32 secy_id, u32 channel,
					     bool *penable)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_replay_protect_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_replay_protect_get(secy_id,
				channel, penable);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *) (&sc)));

	*penable = sc.enable_reply_protect;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_replay_protect_set(u32 secy_id, u32 channel,
					     bool enable)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_replay_protect_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_replay_protect_set(secy_id,
				channel, enable);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	sc.enable_reply_protect = enable;
	SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *)(&sc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_anti_replay_window_get(u32 secy_id, u32 channel,
						 u32 *pwindow)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(pwindow != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_anti_replay_window_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_anti_replay_window_get(
				secy_id, channel, pwindow);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	*pwindow = (sc.anti_replay_window_0 & 0x1fffffff) +
		((sc.anti_replay_window_1 & 0x7) << 29);

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_anti_replay_window_set(u32 secy_id, u32 channel,
						 u32 window)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_anti_replay_window_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_anti_replay_window_set(
				secy_id, channel, window);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	sc.anti_replay_window_0 = window & 0x1fffffff;
	sc.anti_replay_window_1 = (window >> 29) & 0x7;

	SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *)(&sc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_in_used_get(u32 secy_id, u32 channel,
				      bool *p_in_used)
{
	u32 sc_id = 0;
	struct ig_sc sc;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(p_in_used != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_in_used_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_in_used_get(secy_id,
				channel, p_in_used);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	*p_in_used = sc.receiving;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_an_roll_over_get(u32 secy_id, u32 channel,
					   bool *penable)
{
	u32 sc_id = 0;
	struct ig_sc sc;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(penable != NULL));

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	*penable = sc.an_roll_over;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_an_roll_over_set(u32 secy_id, u32 channel,
					   bool enable)
{
	u32 sc_id = 0;
	struct ig_sc sc;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	sc.an_roll_over = enable;
	SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *)(&sc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_start_stop_time_get(u32 secy_id, u32 channel,
					      u32 *p_start_time,
					      u32 *p_stop_time)
{
	u32 sc_id = 0;
	struct ig_sc sc;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(p_start_time != NULL) && (p_stop_time != NULL));

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);

	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *) (&sc)));

	*p_start_time = sc.start_time;
	*p_stop_time = sc.stop_time;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sc_start_stop_time_set(u32 secy_id, u32 channel,
					      u32 start_time, u32 stop_time)
{
	u32 sc_id = 0;
	struct ig_sc sc;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	SHR_RET_ON_ERR(ig_sc_table_get(secy_id, sc_id, (u32 *)(&sc)));

	sc.start_time = start_time;
	sc.stop_time = stop_time;
	SHR_RET_ON_ERR(ig_sc_table_set(secy_id, sc_id, (u32 *)(&sc)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_create(u32 secy_id, u32 channel, u32 an)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_create == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_create(secy_id, channel, an);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	memset(&sa, 0, sizeof(sa));
	sa.valid = 1;

	SHR_RET_ON_ERR(ig_sa_table_set(secy_id, sa_id, (u32 *)(&sa)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_en_get(u32 secy_id, u32 channel, u32 an,
				 bool *penable)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(penable != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_en_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_en_get(secy_id,
				channel, an, penable);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	SHR_RET_ON_ERR(ig_sa_table_get(secy_id, sa_id, (u32 *)(&sa)));
	*penable = sa.valid;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_en_set(u32 secy_id, u32 channel, u32 an, bool enable)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_en_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_en_set(secy_id,
				channel, an, enable);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	SHR_RET_ON_ERR(ig_sa_table_get(secy_id, sa_id, (u32 *)(&sa)));

	sa.valid = enable;
	SHR_RET_ON_ERR(ig_sa_table_set(secy_id, sa_id, (u32 *)(&sa)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_next_pn_get(u32 secy_id, u32 channel,
				      u32 an, u32 *pnpn)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pnpn != NULL));
	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_pn_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_pn_get(secy_id,
				channel, an, pnpn);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	SHR_RET_ON_ERR(ig_sa_table_get(secy_id, sa_id, (u32 *)(&sa)));
	*pnpn = sa.next_pn;

	return ERROR_OK;
}
u32 nss_macsec_secy_rx_sa_next_pn_set(u32 secy_id, u32 channel,
				      u32 an, u32 next_pn)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_pn_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_pn_set(secy_id,
				channel, an, next_pn);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_del(u32 secy_id, u32 channel, u32 an)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_del == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_del(secy_id, channel, an);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	memset(&sa, 0, sizeof(sa));
	SHR_RET_ON_ERR(ig_sa_table_set(secy_id, sa_id, (u32 *)(&sa)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_del_all(u32 secy_id)
{
	int i = 0;
	struct ig_sa sa;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_del_all == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_del_all(secy_id);
		return rv;
	}

	memset(&sa, 0, sizeof(sa));

	for (i = IG_SA_INDEX_MIN; i <= IG_SA_INDEX_MAX; i++) {
		SHR_RET_ON_ERR(ig_sa_table_set(secy_id, i, (u32 *)(&sa)));
	}

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sak_get(u32 secy_id, u32 channel,
			       u32 an, fal_rx_sak_t *pkey)
{
	u32 sc_id = 0;
	u32 sak_id = 0;
	struct ig_sak sak;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sak_t secy_key;

	memset(&secy_key, 0, sizeof(secy_key));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pkey != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sak_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_sak_get(secy_id, channel,
				an, &secy_key);
			*pkey = *(fal_rx_sak_t *)&secy_key;
		}
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sak_id = IG_SAK_INDEX_MIN + sc_id + an;

	SHR_RET_ON_ERR(ig_sak_table_get(secy_id, sak_id, (u32 *)(&sak)));

	memcpy(pkey->sak, &sak, FAL_SAK_DEFAULT_LEN);

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sak_set(u32 secy_id, u32 channel,
			       u32 an, fal_rx_sak_t *pkey)
{
	u32 sc_id = 0;
	u32 sak_id = 0;
	struct ig_sak sak;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sak_t *secy_key;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(pkey != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sak_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_key = (struct secy_sak_t *)pkey;
			rv = secy_drv->secy_rx_sak_set(secy_id,
				channel, an, secy_key);
		}
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sak_id = IG_SAK_INDEX_MIN + sc_id + an;

	memcpy(&sak, pkey->sak, FAL_SAK_DEFAULT_LEN);
	SHR_RET_ON_ERR(ig_sak_table_set(secy_id, sak_id, (u32 *)(&sak)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_in_used_get(u32 secy_id, u32 channel,
				      u32 an, bool *p_in_used)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(p_in_used != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_in_used_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_in_used_get(secy_id, channel,
				an, p_in_used);
		return rv;
	}

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	SHR_RET_ON_ERR(ig_sa_table_get(secy_id, sa_id, (u32 *)(&sa)));
	*p_in_used = sa.inuse;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_start_stop_time_get(u32 secy_id, u32 channel, u32 an,
					      u32 *p_start_time,
					      u32 *p_stop_time)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(p_start_time != NULL) && (p_stop_time != NULL));

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	SHR_RET_ON_ERR(ig_sa_table_get(secy_id, sa_id, (u32 *)(&sa)));

	*p_start_time = sa.start_time;
	*p_stop_time = sa.stop_time;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_sa_start_stop_time_set(u32 secy_id, u32 channel, u32 an,
					      u32 start_time, u32 stop_time)
{
	u32 sc_id = 0;
	u32 sa_id = 0;
	struct ig_sa sa;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	sc_id = fal_rx_channel_2_sc_id(secy_id, channel);
	sa_id = IG_SA_INDEX_MIN + sc_id + an;

	SHR_RET_ON_ERR(ig_sa_table_get(secy_id, sa_id, (u32 *)(&sa)));

	sa.start_time = start_time;
	sa.stop_time = stop_time;
	SHR_RET_ON_ERR(ig_sa_table_set(secy_id, sa_id, (u32 *)(&sa)));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_pn_threshold_get(u32 secy_id, u32 *p_pn_threshold)
{
	union csr_insy_sa_thr_u value;

	value.val = 0;
	SHR_RET_ON_ERR(csr_insy_sa_thr_get(secy_id, &value));

	*p_pn_threshold = value.bf.csr_insy_sa_thr;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_pn_threshold_set(u32 secy_id, u32 pn_threshold)
{
	union csr_insy_sa_thr_u value;

	value.val = 0;
	value.bf.csr_insy_sa_thr = pn_threshold;

	SHR_RET_ON_ERR(csr_insy_sa_thr_set(secy_id, &value));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_replay_protect_get(u32 secy_id, u32 *enable)
{
	union insy_ectrl_en_u value;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_replay_protect_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_replay_protect_get(secy_id,
				(bool *)enable);
		return rv;
	}

	value.val = 0;
	SHR_RET_ON_ERR(insy_ectrl_en_get(secy_id, &value));

	*enable = (value.bf.insy_ectrl_en>>5)&0x1;

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_replay_protect_set(u32 secy_id, u32 enable)
{
	union insy_ectrl_en_u value;
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_replay_protect_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_replay_protect_set(secy_id,
				(bool)enable);
		return rv;
	}

	value.val = 0;
	SHR_RET_ON_ERR(insy_ectrl_en_get(secy_id, &value));

	if (enable)
		value.bf.insy_ectrl_en |= 0x1<<5;
	else
		value.bf.insy_ectrl_en &= ~(0x1<<5);

	SHR_RET_ON_ERR(insy_ectrl_en_set(secy_id, &value));

	return ERROR_OK;
}

u32 nss_macsec_secy_rx_validate_frame_get(u32 secy_id, u32 *mode)
{
	union csr_igsy_ctrl_u value;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_validate_frame_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_validate_frame_get(secy_id,
				&secy_vf_mode);
			if (secy_vf_mode == STRICT)
				*mode = FAL_RX_SC_VALIDATE_FRAME_STRICT;
			else if (secy_vf_mode == CHECKED)
				*mode = FAL_RX_SC_VALIDATE_FRAME_CHECK;
			else if (secy_vf_mode == DISABLED)
				*mode = FAL_RX_SC_VALIDATE_FRAME_DISABLED;
			else
				rv = ERROR_NOT_SUPPORT;
		}
		return rv;
	}

	value.val = 0;
	SHR_RET_ON_ERR(csr_igsy_ctrl_get(secy_id, &value));

	*mode = value.bf.csr_insy_global_validate_frames;
	return ERROR_OK;
}

u32 nss_macsec_secy_rx_validate_frame_set(u32 secy_id, u32 mode)
{
	union csr_igsy_ctrl_u value;
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	enum secy_vf_t secy_vf_mode = STRICT;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_validate_frame_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			if (mode == FAL_RX_SC_VALIDATE_FRAME_STRICT)
				secy_vf_mode = STRICT;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_CHECK)
				secy_vf_mode = CHECKED;
			else if (mode == FAL_RX_SC_VALIDATE_FRAME_DISABLED)
				secy_vf_mode = DISABLED;
			else
				rv = ERROR_NOT_SUPPORT;

			rv = secy_drv->secy_rx_validate_frame_set(secy_id,
				secy_vf_mode);
		}
		return rv;
	}

	value.val = 0;
	SHR_RET_ON_ERR(csr_igsy_ctrl_get(secy_id, &value));
	value.bf.csr_insy_global_validate_frames = mode;
	SHR_RET_ON_ERR(csr_igsy_ctrl_set(secy_id, &value));

	return ERROR_OK;
}
u32 nss_macsec_secy_rx_sa_next_xpn_get(u32 secy_id, u32 channel,
	u32 an, u32 *p_next_xpn)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(p_next_xpn != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_xpn_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_xpn_get(secy_id,
				channel, an, p_next_xpn);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_next_xpn_set(u32 secy_id, u32 channel,
	u32 an, u32 next_xpn)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_next_xpn_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sa_next_xpn_set(secy_id,
				channel, an, next_xpn);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_sc_ssci_get(u32 secy_id, u32 channel, u32 *pssci)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(pssci != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_ssci_get == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_ssci_get(secy_id, channel,
				pssci);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sc_ssci_set(u32 secy_id, u32 channel, u32 ssci)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sc_ssci_set == NULL)
			rv = ERROR_NOT_SUPPORT;
		else
			rv = secy_drv->secy_rx_sc_ssci_set(secy_id, channel,
				ssci);
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_sa_ki_get(u32 secy_id, u32 channel,
	u32 an, struct fal_rx_sa_ki_t *ki)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sa_ki_t secy_sa_ki;

	memset(&secy_sa_ki, 0, sizeof(secy_sa_ki));

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)) &&
			(ki != NULL));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_ki_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_sa_ki_get(secy_id, channel,
					an, &secy_sa_ki);
			*ki = *(struct fal_rx_sa_ki_t *)&secy_sa_ki;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;

}
u32 nss_macsec_secy_rx_sa_ki_set(u32 secy_id, u32 channel,
	u32 an, struct fal_rx_sa_ki_t *ki)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_sa_ki_t *secy_sa_ki = NULL;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(channel < FAL_SECY_CHANNEL_NUM(secy_id)) &&
			(an < FAL_SECY_SA_NUM_PER_SC(secy_id)));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_sa_ki_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_sa_ki = (struct secy_sa_ki_t *)ki;
			rv = secy_drv->secy_rx_sa_ki_set(secy_id, channel,
					an, secy_sa_ki);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

u32 nss_macsec_secy_rx_udf_filt_set(u32 secy_id, u32 filt_id,
	struct fal_rx_udf_filt_t *pfilt)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_t filter;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_UDF_FILT_MAX_ID) && (pfilt != NULL));

	memset(&filter, 0, sizeof(filter));
	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_filt_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			filter.udf_field0 = pfilt->udf_field0;
			filter.udf_field1 = pfilt->udf_field1;
			filter.udf_field2 = pfilt->udf_field2;
			filter.type = (enum secy_udf_filt_type_t)pfilt->type;
			filter.operator =
				(enum secy_udf_filt_op_t)pfilt->operator;
			filter.offset = pfilt->offset;
			filter.mask = pfilt->mask;
			rv = secy_drv->secy_rx_udf_filt_set(secy_id, filt_id,
				&filter);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_udf_filt_get(u32 secy_id, u32 filt_id,
	struct fal_rx_udf_filt_t *pfilt)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_t filter;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(filt_id < FAL_RX_UDF_FILT_MAX_ID) && (pfilt != NULL));

	memset(&filter, 0, sizeof(filter));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_filt_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_udf_filt_get(secy_id, filt_id,
				&filter);
			pfilt->udf_field0 = filter.udf_field0;
			pfilt->udf_field1 = filter.udf_field1;
			pfilt->udf_field2 = filter.udf_field2;
			pfilt->type = (enum fal_rx_udf_filt_type_e)filter.type;
			pfilt->operator =
				(enum fal_rx_udf_filt_op_e)filter.operator;
			pfilt->offset = filter.offset;
			pfilt->mask = filter.mask;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_udf_ufilt_cfg_set(u32 secy_id,
	struct fal_rx_udf_filt_cfg_t *cfg)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_cfg_t secy_cfg;

	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(cfg != NULL));

	memset(&secy_cfg, 0, sizeof(secy_cfg));
	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_ufilt_cfg_set == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			secy_cfg.enable = cfg->enable;
			secy_cfg.priority = cfg->priority;
			secy_cfg.inverse = cfg->inverse;
			secy_cfg.pattern0 =
			(enum secy_udf_filt_cfg_pattern_t)cfg->pattern0;
			secy_cfg.pattern1 =
			(enum secy_udf_filt_cfg_pattern_t)cfg->pattern1;
			secy_cfg.pattern2 =
			(enum secy_udf_filt_cfg_pattern_t)cfg->pattern2;
			rv = secy_drv->secy_rx_udf_ufilt_cfg_set(secy_id,
				&secy_cfg);
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}
u32 nss_macsec_secy_rx_udf_ufilt_cfg_get(u32 secy_id,
	struct fal_rx_udf_filt_cfg_t *cfg)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	struct secy_udf_filt_cfg_t secy_cfg;


	SHR_PARAM_CHECK((secy_id < FAL_SECY_ID_NUM) &&
			(cfg != NULL));

	memset(&secy_cfg, 0, sizeof(secy_cfg));

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_rx_udf_ufilt_cfg_get == NULL) {
			rv = ERROR_NOT_SUPPORT;
		} else {
			rv = secy_drv->secy_rx_udf_ufilt_cfg_get(secy_id,
				&secy_cfg);
			cfg->enable = secy_cfg.enable;
			cfg->priority = secy_cfg.priority;
			cfg->inverse = secy_cfg.inverse;
			cfg->pattern0 =
			(enum fal_rx_udf_filt_cfg_pattern_e)secy_cfg.pattern0;
			cfg->pattern1 =
			(enum fal_rx_udf_filt_cfg_pattern_e)secy_cfg.pattern1;
			cfg->pattern2 =
			(enum fal_rx_udf_filt_cfg_pattern_e)secy_cfg.pattern2;
		}
		return rv;
	}

	return ERROR_NOT_SUPPORT;
}

