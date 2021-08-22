/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
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

/**
 * DOC: reg_channel.c
 * This file defines the API to access/update/modify regulatory current channel
 * list by WIN host umac components.
 */

#include <wlan_cmn.h>
#include <reg_services_public_struct.h>
#include <wlan_objmgr_psoc_obj.h>
#include <reg_priv_objs.h>
#include <reg_services_common.h>
#include "reg_channel.h"
#include <wlan_reg_channel_api.h>
#include <wlan_reg_services_api.h>

#ifdef CONFIG_HOST_FIND_CHAN

#ifdef WLAN_FEATURE_11BE
static inline int is_11be_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return (WIRELESS_11BE_MODES & wireless_modes) &&
		!(phybitmap & REGULATORY_PHYMODE_NO11BE);
}
#else
static inline int is_11be_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return false;
}
#endif

static inline int is_11ax_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return (WIRELESS_11AX_MODES & wireless_modes) &&
		!(phybitmap & REGULATORY_PHYMODE_NO11AX);
}

static inline int is_11ac_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return (WIRELESS_11AC_MODES & wireless_modes) &&
		!(phybitmap & REGULATORY_PHYMODE_NO11AC);
}

static inline int is_11n_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return (WIRELESS_11N_MODES & wireless_modes) &&
		!(phybitmap & REGULATORY_CHAN_NO11N);
}

static inline int is_11g_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return (WIRELESS_11G_MODES & wireless_modes) &&
		!(phybitmap & REGULATORY_PHYMODE_NO11G);
}

static inline int is_11b_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return (WIRELESS_11B_MODES & wireless_modes) &&
		!(phybitmap & REGULATORY_PHYMODE_NO11B);
}

static inline int is_11a_supported(uint64_t wireless_modes, uint32_t phybitmap)
{
	return (WIRELESS_11A_MODES & wireless_modes) &&
		!(phybitmap & REGULATORY_PHYMODE_NO11A);
}

#ifdef WLAN_FEATURE_11BE
static void fill_11be_max_phymode_chwidth(uint64_t wireless_modes,
					  uint32_t phybitmap,
					  enum phy_ch_width *max_chwidth,
					  enum reg_phymode *max_phymode)
{
	*max_phymode = REG_PHYMODE_11BE;
	if (wireless_modes & WIRELESS_320_MODES)
		*max_chwidth = CH_WIDTH_320MHZ;
	else if (wireless_modes & WIRELESS_160_MODES)
		*max_chwidth = CH_WIDTH_160MHZ;
	else if (wireless_modes & WIRELESS_80_MODES)
		*max_chwidth = CH_WIDTH_80MHZ;
	else if (wireless_modes & WIRELESS_40_MODES)
		*max_chwidth = CH_WIDTH_40MHZ;
}
#else
static inline void
fill_11be_max_phymode_chwidth(uint64_t wireless_modes,
			      uint32_t phybitmap,
			      enum phy_ch_width *max_chwidth,
			      enum reg_phymode *max_phymode)
{
}
#endif

void reg_update_max_phymode_chwidth_for_pdev(struct wlan_objmgr_pdev *pdev)
{
	uint64_t wireless_modes;
	uint32_t phybitmap;
	enum phy_ch_width max_chwidth = CH_WIDTH_20MHZ;
	enum reg_phymode max_phymode = REG_PHYMODE_MAX;
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	wireless_modes = pdev_priv_obj->wireless_modes;
	phybitmap = pdev_priv_obj->phybitmap;

	if (is_11be_supported(wireless_modes, phybitmap)) {
		fill_11be_max_phymode_chwidth(wireless_modes, phybitmap,
					      &max_chwidth, &max_phymode);
	} else if (is_11ax_supported(wireless_modes, phybitmap)) {
		max_phymode = REG_PHYMODE_11AX;
		if (wireless_modes & WIRELESS_160_MODES)
			max_chwidth = CH_WIDTH_160MHZ;
		else if (wireless_modes & WIRELESS_80_MODES)
			max_chwidth = CH_WIDTH_80MHZ;
		else if (wireless_modes & WIRELESS_40_MODES)
			max_chwidth = CH_WIDTH_40MHZ;
	} else if (is_11ac_supported(wireless_modes, phybitmap)) {
		max_phymode = REG_PHYMODE_11AC;
		if (wireless_modes & WIRELESS_160_MODES)
			max_chwidth = CH_WIDTH_160MHZ;
		else if (wireless_modes & WIRELESS_80_MODES)
			max_chwidth = CH_WIDTH_80MHZ;
		else if (wireless_modes & WIRELESS_40_MODES)
			max_chwidth = CH_WIDTH_40MHZ;
	} else if (is_11n_supported(wireless_modes, phybitmap)) {
		max_phymode = REG_PHYMODE_11N;
		if (wireless_modes & WIRELESS_40_MODES)
			max_chwidth = CH_WIDTH_40MHZ;
	} else if (is_11g_supported(wireless_modes, phybitmap)) {
		max_phymode = REG_PHYMODE_11G;
	} else if (is_11b_supported(wireless_modes, phybitmap)) {
		max_phymode = REG_PHYMODE_11B;
	} else if (is_11a_supported(wireless_modes, phybitmap)) {
		max_phymode = REG_PHYMODE_11A;
	} else {
		reg_err("Device does not support any wireless_mode! %0llx",
			wireless_modes);
	}

	pdev_priv_obj->max_phymode = max_phymode;
	pdev_priv_obj->max_chwidth = max_chwidth;
}

/**
 * chwd_2_contbw_lst - Conversion array from channel width enum to value.
 * Array index of type phy_ch_width, return of type uint16_t.
 */
static uint16_t chwd_2_contbw_lst[CH_WIDTH_MAX + 1] = {
	BW_20_MHZ,   /* CH_WIDTH_20MHZ */
	BW_40_MHZ,   /* CH_WIDTH_40MHZ */
	BW_80_MHZ,   /* CH_WIDTH_80MHZ */
	BW_160_MHZ,  /* CH_WIDTH_160MHZ */
	BW_80_MHZ,   /* CH_WIDTH_80P80MHZ */
	BW_5_MHZ,    /* CH_WIDTH_5MHZ */
	BW_10_MHZ,   /* CH_WIDTH_10MHZ */
#ifdef WLAN_FEATURE_11BE
	BW_320_MHZ,  /* CH_WIDTH_320MHZ */
#endif
	0,           /* CH_WIDTH_INVALID */
#ifdef WLAN_FEATURE_11BE
	BW_320_MHZ,  /* CH_WIDTH_MAX */
#else
	BW_160_MHZ,  /* CH_WIDTH_MAX */
#endif

};

/**
 * reg_get_max_channel_width() - Get the maximum channel width supported
 * given a frequency and a global maximum channel width.
 * @pdev: Pointer to PDEV object.
 * @freq: Input frequency.
 * @g_max_width: Global maximum channel width.
 *
 * Return: Maximum channel width of type phy_ch_width.
 */
#ifdef WLAN_FEATURE_11BE
static enum phy_ch_width
reg_get_max_channel_width(struct wlan_objmgr_pdev *pdev,
			  qdf_freq_t freq,
			  enum phy_ch_width g_max_width)
{
	struct reg_channel_list chan_list;
	uint16_t i, max_bw = 0;
	enum phy_ch_width output_width = CH_WIDTH_INVALID;

	wlan_reg_fill_channel_list(pdev, freq, 0,
				   g_max_width, 0,
				   &chan_list);

	for (i = 0; i < chan_list.num_ch_params; i++) {
		struct ch_params *ch_param = &chan_list.chan_param[i];
		uint16_t cont_bw = chwd_2_contbw_lst[ch_param->ch_width];

		if (max_bw < cont_bw) {
			output_width = ch_param->ch_width;
			max_bw = cont_bw;
		}
	}
	return output_width;
}
#else
static enum phy_ch_width
reg_get_max_channel_width(struct wlan_objmgr_pdev *pdev,
			  qdf_freq_t freq,
			  enum phy_ch_width g_max_width)
{
	struct ch_params chan_params;

	chan_params.ch_width = g_max_width;
	reg_get_channel_params(pdev, freq, 0, &chan_params);
	return chan_params.ch_width;
}
#endif

void reg_modify_chan_list_for_max_chwidth(
		struct wlan_objmgr_pdev *pdev,
		struct regulatory_channel *cur_chan_list)
{
	int i;
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	for (i = 0; i < NUM_CHANNELS; i++) {
		enum phy_ch_width g_max_width = pdev_priv_obj->max_chwidth;
		enum phy_ch_width output_width = CH_WIDTH_INVALID;
		qdf_freq_t freq = cur_chan_list[i].center_freq;

		if (cur_chan_list[i].chan_flags & REGULATORY_CHAN_DISABLED)
			continue;

		/*
		 * Correct the max bandwidths if they were not taken care of
		 * while parsing the reg rules.
		 */
		output_width = reg_get_max_channel_width(pdev, freq,
							 g_max_width);

		if (output_width != CH_WIDTH_INVALID)
			cur_chan_list[i].max_bw =
				qdf_min(cur_chan_list[i].max_bw,
					chwd_2_contbw_lst[output_width]);
	}
}

static uint64_t convregphymode2wirelessmodes[REG_PHYMODE_MAX] = {
	0xFFFFFFFF,                  /* REG_PHYMODE_INVALID */
	WIRELESS_11B_MODES,          /* REG_PHYMODE_11B     */
	WIRELESS_11G_MODES,          /* REG_PHYMODE_11G     */
	WIRELESS_11A_MODES,          /* REG_PHYMODE_11A     */
	WIRELESS_11N_MODES,          /* REG_PHYMODE_11N     */
	WIRELESS_11AC_MODES,         /* REG_PHYMODE_11AC    */
	WIRELESS_11AX_MODES,         /* REG_PHYMODE_11AX    */
#ifdef WLAN_FEATURE_11BE
	WIRELESS_11BE_MODES,         /* REG_PHYMODE_11BE    */
#endif
};

static int reg_is_phymode_in_wireless_modes(enum reg_phymode phy_in,
					    uint64_t wireless_modes)
{
	uint64_t sup_wireless_modes = convregphymode2wirelessmodes[phy_in];

	return sup_wireless_modes & wireless_modes;
}

bool reg_is_phymode_chwidth_allowed(
		struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj,
		enum reg_phymode phy_in,
		enum phy_ch_width ch_width,
		qdf_freq_t freq)
{
	uint32_t phymode_bitmap;
	uint64_t wireless_modes;
	enum phy_ch_width output_width = CH_WIDTH_INVALID;

	if (ch_width == CH_WIDTH_INVALID)
		return false;

	phymode_bitmap = pdev_priv_obj->phybitmap;
	wireless_modes = pdev_priv_obj->wireless_modes;

	if (reg_is_phymode_unallowed(phy_in, phymode_bitmap) ||
	    !reg_is_phymode_in_wireless_modes(phy_in, wireless_modes))
		return false;

	output_width = reg_get_max_channel_width(pdev_priv_obj->pdev_ptr,
						 freq,
						 ch_width);

	if (output_width != ch_width)
		return false;

	return true;
}

void reg_set_chan_blocked(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	int i;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (cur_chan_list[i].center_freq == freq) {
			cur_chan_list[i].is_chan_hop_blocked = true;
			break;
		}
	}
}

bool reg_is_chan_blocked(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	int i;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return false;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++)
		if (cur_chan_list[i].center_freq == freq)
			return cur_chan_list[i].is_chan_hop_blocked;

	return false;
}

void reg_clear_allchan_blocked(struct wlan_objmgr_pdev *pdev)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	int i;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++)
		cur_chan_list[i].is_chan_hop_blocked = false;
}

void reg_set_chan_ht40intol(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq,
			    enum ht40_intol ht40intol_flags)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	int i;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (cur_chan_list[i].center_freq == freq)
			cur_chan_list[i].ht40intol_flags |=
					BIT(ht40intol_flags);
	}
}

void reg_clear_chan_ht40intol(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq,
			      enum ht40_intol ht40intol_flags)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	int i;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (cur_chan_list[i].center_freq == freq)
			cur_chan_list[i].ht40intol_flags &=
				~(BIT(ht40intol_flags));
	}
}

bool reg_is_chan_ht40intol(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq,
			   enum ht40_intol ht40intol_flags)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	int i;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return false;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++)
		if (cur_chan_list[i].center_freq == freq)
			return (cur_chan_list[i].ht40intol_flags &
				BIT(ht40intol_flags));

	return false;
}

void reg_clear_allchan_ht40intol(struct wlan_objmgr_pdev *pdev)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	int i;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++)
		cur_chan_list[i].ht40intol_flags = 0;
}

/*
 * reg_is_band_found_internal - Check if a band channel is found in the
 * current channel list.
 *
 * @start_idx - Start index.
 * @end_idx - End index.
 * @cur_chan_list - Pointer to cur_chan_list.
 */
static bool reg_is_band_found_internal(enum channel_enum start_idx,
				       enum channel_enum end_idx,
				       struct regulatory_channel *cur_chan_list)
{
	uint8_t i;

	for (i = start_idx; i <= end_idx; i++)
		if (!(reg_is_chan_disabled(&cur_chan_list[i])))
			return true;

	return false;
}

bool reg_is_band_present(struct wlan_objmgr_pdev *pdev,
			 enum reg_wifi_band reg_band)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	struct regulatory_channel *cur_chan_list;
	enum channel_enum min_chan_idx, max_chan_idx;

	switch (reg_band) {
	case REG_BAND_2G:
		min_chan_idx = MIN_24GHZ_CHANNEL;
		max_chan_idx = MAX_24GHZ_CHANNEL;
		break;
	case REG_BAND_5G:
		min_chan_idx = MIN_49GHZ_CHANNEL;
		max_chan_idx = MAX_5GHZ_CHANNEL;
		break;
	case REG_BAND_6G:
		min_chan_idx = MIN_6GHZ_CHANNEL;
		max_chan_idx = MAX_6GHZ_CHANNEL;
		break;
	default:
		return false;
	}

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return false;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	return reg_is_band_found_internal(min_chan_idx, max_chan_idx,
					  cur_chan_list);
}

bool reg_is_chan_disabled(struct regulatory_channel *chan)
{
	return ((chan->chan_flags & REGULATORY_CHAN_DISABLED) &&
		(chan->state == CHANNEL_STATE_DISABLE) &&
		(!chan->nol_chan) && (!chan->nol_history));
}

#endif /* CONFIG_HOST_FIND_CHAN */

bool reg_is_nol_for_freq(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq)
{
	enum channel_enum chan_enum;
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;

	chan_enum = reg_get_chan_enum_for_freq(freq);
	if (chan_enum == INVALID_CHANNEL) {
		reg_err("chan freq is not valid");
		return false;
	}

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("pdev reg obj is NULL");
		return false;
	}

	return pdev_priv_obj->cur_chan_list[chan_enum].nol_chan;
}

bool reg_is_nol_hist_for_freq(struct wlan_objmgr_pdev *pdev, qdf_freq_t freq)
{
	enum channel_enum chan_enum;
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;

	chan_enum = reg_get_chan_enum_for_freq(freq);
	if (chan_enum == INVALID_CHANNEL) {
		reg_err("chan freq is not valid");
		return false;
	}

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("pdev reg obj is NULL");
		return false;
	}

	return pdev_priv_obj->cur_chan_list[chan_enum].nol_history;
}

/**
 * reg_is_freq_band_dfs() - Find the bonded pair for the given frequency
 * and check if any of the sub frequencies in the bonded pair is DFS.
 * @pdev: Pointer to the pdev object.
 * @freq: Input frequency.
 * @bonded_chan_ptr: Frequency range of the given channel and width.
 *
 * Return: True if any of the channels in the bonded_chan_ar that contains
 * the input frequency is dfs, else false.
 */
static bool
reg_is_freq_band_dfs(struct wlan_objmgr_pdev *pdev,
		     qdf_freq_t freq,
		     const struct bonded_channel_freq *bonded_chan_ptr)
{
	qdf_freq_t chan_cfreq;
	bool is_dfs = false;

	chan_cfreq =  bonded_chan_ptr->start_freq;
	while (chan_cfreq <= bonded_chan_ptr->end_freq) {
		/* If any of the channel is disabled by regulatory, return. */
		if (reg_is_disable_for_freq(pdev, chan_cfreq) &&
		    !reg_is_nol_for_freq(pdev, chan_cfreq))
			return false;
		if (reg_is_dfs_for_freq(pdev, chan_cfreq))
			is_dfs = true;
		chan_cfreq = chan_cfreq + NEXT_20_CH_OFFSET;
	}

	return is_dfs;
}

bool reg_is_freq_width_dfs(struct wlan_objmgr_pdev *pdev,
			   qdf_freq_t freq,
			   enum phy_ch_width ch_width)
{
	const struct bonded_channel_freq *bonded_chan_ptr;

	if (ch_width == CH_WIDTH_20MHZ)
		return reg_is_dfs_for_freq(pdev, freq);

	bonded_chan_ptr = reg_get_bonded_chan_entry(freq, ch_width);

	if (!bonded_chan_ptr)
		return false;

	return reg_is_freq_band_dfs(pdev, freq, bonded_chan_ptr);
}

/**
 * reg_get_nol_channel_state () - Get channel state from regulatory
 * and treat NOL channels as enabled channels
 * @pdev: Pointer to pdev
 * @freq: channel center frequency.
 *
 * Return: channel state
 */
static enum channel_state
reg_get_nol_channel_state(struct wlan_objmgr_pdev *pdev,
			  qdf_freq_t freq)
{
	enum channel_enum ch_idx;
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	enum channel_state chan_state;

	ch_idx = reg_get_chan_enum_for_freq(freq);

	if (ch_idx == INVALID_CHANNEL)
		return CHANNEL_STATE_INVALID;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("pdev reg obj is NULL");
		return CHANNEL_STATE_INVALID;
	}

	chan_state = pdev_priv_obj->cur_chan_list[ch_idx].state;

	if ((pdev_priv_obj->cur_chan_list[ch_idx].nol_chan ||
	   pdev_priv_obj->cur_chan_list[ch_idx].nol_history) &&
	  chan_state == CHANNEL_STATE_DISABLE)
		chan_state = CHANNEL_STATE_DFS;

	return chan_state;
}

/**
 * reg_get_5g_bonded_chan_state()- Return the channel state for a
 * 5G or 6G channel frequency based on the bonded channel.
 * @pdev: Pointer to pdev.
 * @freq: Channel center frequency.
 * @bonded_chan_ptr: Pointer to bonded_channel_freq.
 *
 * Return: Channel State
 */
static enum channel_state
reg_get_5g_bonded_chan_state(struct wlan_objmgr_pdev *pdev,
			     uint16_t freq,
			     const struct bonded_channel_freq *
			     bonded_chan_ptr)
{
	uint16_t chan_cfreq;
	enum channel_state chan_state = CHANNEL_STATE_INVALID;
	enum channel_state temp_chan_state;

	chan_cfreq =  bonded_chan_ptr->start_freq;
	while (chan_cfreq <= bonded_chan_ptr->end_freq) {
		temp_chan_state = reg_get_nol_channel_state(pdev, chan_cfreq);
		if (temp_chan_state < chan_state)
			chan_state = temp_chan_state;
		chan_cfreq = chan_cfreq + 20;
	}

	return chan_state;
}

/**
 * reg_get_5g_chan_state() - Get channel state for
 * 5G bonded channel using the channel frequency
 * @pdev: Pointer to pdev
 * @freq: channel center frequency.
 * @bw: channel band width
 *
 * Return: channel state
 */
static enum channel_state reg_get_5g_chan_state(struct wlan_objmgr_pdev *pdev,
						qdf_freq_t freq,
						enum phy_ch_width bw)
{
	enum channel_enum ch_indx;
	enum channel_state chan_state;
	struct regulatory_channel *reg_channels;
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	bool bw_enabled = false;
	const struct bonded_channel_freq *bonded_chan_ptr = NULL;

	if (bw > CH_WIDTH_80P80MHZ) {
		reg_err_rl("bw passed is not good");
		return CHANNEL_STATE_INVALID;
	}

	if (bw == CH_WIDTH_20MHZ)
		return reg_get_nol_channel_state(pdev, freq);

	/* Fetch the bonded_chan_ptr for width greater than 20MHZ. */
	bonded_chan_ptr = reg_get_bonded_chan_entry(freq, bw);

	if (!bonded_chan_ptr)
		return CHANNEL_STATE_INVALID;

	chan_state = reg_get_5g_bonded_chan_state(pdev, freq, bonded_chan_ptr);

	if ((chan_state == CHANNEL_STATE_INVALID) ||
	    (chan_state == CHANNEL_STATE_DISABLE))
		return chan_state;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("pdev reg obj is NULL");
		return CHANNEL_STATE_INVALID;
	}
	reg_channels = pdev_priv_obj->cur_chan_list;

	ch_indx = reg_get_chan_enum_for_freq(freq);
	if (ch_indx == INVALID_CHANNEL)
		return CHANNEL_STATE_INVALID;
	if (bw == CH_WIDTH_5MHZ)
		bw_enabled = true;
	else if (bw == CH_WIDTH_10MHZ)
		bw_enabled = (reg_channels[ch_indx].min_bw <= 10) &&
			(reg_channels[ch_indx].max_bw >= 10);
	else if (bw == CH_WIDTH_20MHZ)
		bw_enabled = (reg_channels[ch_indx].min_bw <= 20) &&
			(reg_channels[ch_indx].max_bw >= 20);
	else if (bw == CH_WIDTH_40MHZ)
		bw_enabled = (reg_channels[ch_indx].min_bw <= 40) &&
			(reg_channels[ch_indx].max_bw >= 40);
	else if (bw == CH_WIDTH_80MHZ)
		bw_enabled = (reg_channels[ch_indx].min_bw <= 80) &&
			(reg_channels[ch_indx].max_bw >= 80);
	else if (bw == CH_WIDTH_160MHZ)
		bw_enabled = (reg_channels[ch_indx].min_bw <= 160) &&
			(reg_channels[ch_indx].max_bw >= 160);
	else if (bw == CH_WIDTH_80P80MHZ)
		bw_enabled = (reg_channels[ch_indx].min_bw <= 80) &&
			(reg_channels[ch_indx].max_bw >= 80);

	if (bw_enabled)
		return chan_state;
	else
		return CHANNEL_STATE_DISABLE;
}

/**
 * reg_get_5g_channel_params ()- Set channel parameters like center
 * frequency for a bonded channel state. Also return the maximum bandwidth
 * supported by the channel.
 * @pdev: Pointer to pdev.
 * @freq: Channel center frequency.
 * ch_params: Pointer to ch_params.
 *
 * Return: void
 */
static void reg_get_5g_channel_params(struct wlan_objmgr_pdev *pdev,
				      uint16_t freq,
				      struct ch_params *ch_params)
{
	/*
	 * Set channel parameters like center frequency for a bonded channel
	 * state. Also return the maximum bandwidth supported by the channel.
	*/

	enum channel_state chan_state = CHANNEL_STATE_ENABLE;
	enum channel_state chan_state2 = CHANNEL_STATE_ENABLE;
	const struct bonded_channel_freq *bonded_chan_ptr = NULL;
	const struct bonded_channel_freq *bonded_chan_ptr2 = NULL;
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	enum channel_enum chan_enum, sec_5g_chan_enum;
	uint16_t max_bw, bw_80, sec_5g_freq_max_bw = 0;

	if (!ch_params) {
		reg_err("ch_params is NULL");
		return;
	}

	chan_enum = reg_get_chan_enum_for_freq(freq);
	if (chan_enum == INVALID_CHANNEL) {
		reg_err("chan freq is not valid");
		return;
	}

	pdev_priv_obj = reg_get_pdev_obj(pdev);
	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("reg pdev priv obj is NULL");
		return;
	}

	if (ch_params->ch_width >= CH_WIDTH_MAX) {
		if (ch_params->mhz_freq_seg1 != 0)
			ch_params->ch_width = CH_WIDTH_80P80MHZ;
		else
			ch_params->ch_width = CH_WIDTH_160MHZ;
	}

	max_bw = pdev_priv_obj->cur_chan_list[chan_enum].max_bw;
	bw_80 = reg_get_bw_value(CH_WIDTH_80MHZ);

	if (ch_params->ch_width == CH_WIDTH_80P80MHZ) {
		sec_5g_chan_enum =
			reg_get_chan_enum_for_freq(
				ch_params->mhz_freq_seg1 -
				NEAREST_20MHZ_CHAN_FREQ_OFFSET);
		if (sec_5g_chan_enum == INVALID_CHANNEL) {
			reg_err("secondary channel freq is not valid");
			return;
		}

		sec_5g_freq_max_bw =
			pdev_priv_obj->cur_chan_list[sec_5g_chan_enum].max_bw;
	}

	while (ch_params->ch_width != CH_WIDTH_INVALID) {
		if (ch_params->ch_width == CH_WIDTH_80P80MHZ) {
			if ((max_bw < bw_80) || (sec_5g_freq_max_bw < bw_80))
				goto update_bw;
		} else if (max_bw < reg_get_bw_value(ch_params->ch_width)) {
			goto update_bw;
		}

		bonded_chan_ptr = NULL;
		bonded_chan_ptr2 = NULL;
		bonded_chan_ptr =
		    reg_get_bonded_chan_entry(freq, ch_params->ch_width);

		chan_state =
		    reg_get_5g_chan_state(pdev, freq, ch_params->ch_width);

		if (ch_params->ch_width == CH_WIDTH_80P80MHZ) {
			chan_state2 = reg_get_5g_chan_state(
					pdev, ch_params->mhz_freq_seg1 -
					NEAREST_20MHZ_CHAN_FREQ_OFFSET,
					CH_WIDTH_80MHZ);

			chan_state = reg_combine_channel_states(
					chan_state, chan_state2);
		}

		if ((chan_state != CHANNEL_STATE_ENABLE) &&
		(chan_state != CHANNEL_STATE_DFS))
			goto update_bw;
		if (ch_params->ch_width <= CH_WIDTH_20MHZ) {
			ch_params->sec_ch_offset = NO_SEC_CH;
			ch_params->mhz_freq_seg0 = freq;
			ch_params->center_freq_seg0 =
				reg_freq_to_chan(pdev,
						 ch_params->mhz_freq_seg0);
			break;
		} else if (ch_params->ch_width >= CH_WIDTH_40MHZ) {
			bonded_chan_ptr2 =
				reg_get_bonded_chan_entry(freq, CH_WIDTH_40MHZ);

			if (!bonded_chan_ptr || !bonded_chan_ptr2)
				goto update_bw;
			if (freq == bonded_chan_ptr2->start_freq)
				ch_params->sec_ch_offset = LOW_PRIMARY_CH;
			else
				ch_params->sec_ch_offset = HIGH_PRIMARY_CH;

			ch_params->mhz_freq_seg0 =
				(bonded_chan_ptr->start_freq +
				 bonded_chan_ptr->end_freq) / 2;
			ch_params->center_freq_seg0 =
				reg_freq_to_chan(pdev,
						 ch_params->mhz_freq_seg0);
			break;
		}
update_bw:
		ch_params->ch_width =
			get_next_lower_bandwidth(ch_params->ch_width);
	}

	if (ch_params->ch_width == CH_WIDTH_160MHZ) {
		ch_params->mhz_freq_seg1 = ch_params->mhz_freq_seg0;
		ch_params->center_freq_seg1 =
			reg_freq_to_chan(pdev,
					   ch_params->mhz_freq_seg1);

		bonded_chan_ptr =
			reg_get_bonded_chan_entry(freq, CH_WIDTH_80MHZ);
		if (bonded_chan_ptr) {
			ch_params->mhz_freq_seg0 =
				(bonded_chan_ptr->start_freq +
		 bonded_chan_ptr->end_freq) / 2;
			ch_params->center_freq_seg0 =
				reg_freq_to_chan(pdev,
						 ch_params->mhz_freq_seg0);
		}
	}

	/* Overwrite mhz_freq_seg1 to 0 for non 160 and 80+80 width */
	if (!(ch_params->ch_width == CH_WIDTH_160MHZ ||
		ch_params->ch_width == CH_WIDTH_80P80MHZ)) {
		ch_params->mhz_freq_seg1 = 0;
		ch_params->center_freq_seg1 = 0;
	}
}

void reg_get_channel_params(struct wlan_objmgr_pdev *pdev,
			    qdf_freq_t freq,
			    qdf_freq_t sec_ch_2g_freq,
			    struct ch_params *ch_params)
{
    if (reg_is_5ghz_ch_freq(freq) || reg_is_6ghz_chan_freq(freq))
	reg_get_5g_channel_params(pdev, freq, ch_params);
    else if  (reg_is_24ghz_ch_freq(freq))
	reg_set_2g_channel_params_for_freq(pdev, freq, ch_params,
					   sec_ch_2g_freq);
}

#ifdef WLAN_FEATURE_11BE
static enum phy_ch_width reg_find_chwidth_from_bw(uint16_t bw)
{
	switch (bw) {
	case BW_5_MHZ:
		return CH_WIDTH_5MHZ;
	case BW_10_MHZ:
		return CH_WIDTH_10MHZ;
	case BW_20_MHZ:
		return CH_WIDTH_20MHZ;
	case BW_40_MHZ:
		return CH_WIDTH_40MHZ;
	case BW_80_MHZ:
		return CH_WIDTH_80MHZ;
	case BW_160_MHZ:
		return CH_WIDTH_160MHZ;
	case BW_320_MHZ:
		return CH_WIDTH_320MHZ;
	default:
		return CH_WIDTH_INVALID;
	}
}
#else
static enum phy_ch_width reg_find_chwidth_from_bw(uint16_t bw)
{
	switch (bw) {
	case BW_5_MHZ:
		return CH_WIDTH_5MHZ;
	case BW_10_MHZ:
		return CH_WIDTH_10MHZ;
	case BW_20_MHZ:
		return CH_WIDTH_20MHZ;
	case BW_40_MHZ:
		return CH_WIDTH_40MHZ;
	case BW_80_MHZ:
		return CH_WIDTH_80MHZ;
	case BW_160_MHZ:
		return CH_WIDTH_160MHZ;
	default:
		return CH_WIDTH_INVALID;
	}
}
#endif

#ifdef WLAN_FEATURE_11BE
static void
fill_320mhz_modes(int max_bw, uint64_t *wireless_modes)
{
	if (max_bw < BW_320_MHZ)
		*wireless_modes &= (~WIRELESS_320_MODES);
}
#else
static inline void
fill_320mhz_modes(int max_bw, uint64_t *wireless_modes)
{
}
#endif

void reg_filter_wireless_modes(struct wlan_objmgr_pdev *pdev,
			       uint64_t *mode_select,
			       bool include_nol_chan)
{
	struct wlan_regulatory_pdev_priv_obj *pdev_priv_obj;
	uint64_t in_wireless_modes = *mode_select;
	struct regulatory_channel *cur_chan_list;
	int i, max_bw = 20;
	uint64_t band_modes = 0;

	pdev_priv_obj = reg_get_pdev_obj(pdev);

	if (!IS_VALID_PDEV_REG_OBJ(pdev_priv_obj)) {
		reg_err("pdev reg obj is NULL");
		return;
	}

	cur_chan_list = pdev_priv_obj->cur_chan_list;

	for (i = 0; i < NUM_CHANNELS; i++) {
		qdf_freq_t freq = cur_chan_list[i].center_freq;
		uint16_t cur_bw = cur_chan_list[i].max_bw;
		struct ch_params ch_param = {0};

		if (reg_is_chan_disabled(&cur_chan_list[i]))
			continue;

		if (WLAN_REG_IS_24GHZ_CH_FREQ(freq))
			band_modes |= WIRELESS_2G_MODES;

		if (WLAN_REG_IS_49GHZ_FREQ(freq))
			band_modes |= WIRELESS_49G_MODES;

		if (WLAN_REG_IS_5GHZ_CH_FREQ(freq))
			band_modes |= WIRELESS_5G_MODES;

		if (WLAN_REG_IS_6GHZ_CHAN_FREQ(freq))
			band_modes |= WIRELESS_6G_MODES;

		if (!include_nol_chan) {
			ch_param.ch_width = reg_find_chwidth_from_bw(cur_bw);
			wlan_reg_set_channel_params_for_freq(pdev, freq, 0,
							     &ch_param);
			cur_bw = chwd_2_contbw_lst[ch_param.ch_width];
		}

		if (max_bw < cur_bw)
			max_bw = cur_bw;
	}

	in_wireless_modes &= band_modes;

	if (max_bw < BW_40_MHZ)
		in_wireless_modes &= (~WIRELESS_40_MODES);

	if (max_bw < BW_80_MHZ)
		in_wireless_modes &= (~WIRELESS_80_MODES);

	if (max_bw < BW_160_MHZ) {
		in_wireless_modes &= (~WIRELESS_160_MODES);
		if (include_nol_chan)
			in_wireless_modes &= (~WIRELESS_80P80_MODES);
	}

	fill_320mhz_modes(max_bw, &in_wireless_modes);
	*mode_select = in_wireless_modes;
}
