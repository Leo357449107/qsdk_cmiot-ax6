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

#ifndef _NSS_MACSEC_SECY_RX_H_
#define _NSS_MACSEC_SECY_RX_H_

#include "nss_macsec_types.h"

typedef enum {
	IG_CTL_COMPARE_NO,
	IG_CTL_COMPARE_DA,
	IG_CTL_COMPARE_SA,
	IG_CTL_COMPARE_HALF_DA_SA,
	IG_CTL_COMPARE_ETHER_TYPE,
	IG_CTL_COMPARE_DA_ETHER_TYPE,
	IG_CTL_COMPARE_SA_EHTER_TYPE,
	IG_CTL_COMPARE_DA_RANGE,
	IG_CTL_COMPARE_MAX
} fal_ig_ctl_match_type_e;

typedef struct {
	bool bypass;
	fal_ig_ctl_match_type_e match_type;
	u16 match_mask;
	u16 ether_type_da_range;
	u8 sa_da_addr[6];
} fal_rx_ctl_filt_t;

#define FAL_RX_CTL_FILT_NUM  24

typedef enum {
	FAL_RX_PRC_ACTION_PROCESS,
	FAL_RX_PRC_ACTION_PROCESS_WITH_SECTAG,
	FAL_RX_PRC_ACTION_BYPASS,
	FAL_RX_PRC_ACTION_DROP,
	FAL_RX_PRC_ACTION_MAX
} fal_rx_prc_lut_action_e;

typedef struct {
	bool valid;
	bool uncontrolled_port;
	fal_rx_prc_lut_action_e action;
	u32 channel;
	u8 sci[8];
	u8 sci_mask;		/* first 8 bits are useful */
	u8 tci;
	u8 tci_mask;		/* first 6 bits are useful */
	u8 da[6];
	u8 da_mask;		/* first 6 bits are useful */
	u8 sa[6];
	u8 sa_mask;		/* first 6 bits are useful */
	u16 ether_type;
	u8 ether_type_mask;	/* first 2 bits are useful */
	u8 offset;
	/*add for NAPA PHY*/
	u16 outer_vlanid;
	u16 inner_vlanid;
	bool bc_flag;
	u32 rule_mask;
} fal_rx_prc_lut_t;

#define FAL_RX_CLASS_LUT_NUM  48

typedef enum {
	FAL_RX_SC_VALIDATE_FRAME_STRICT,
	FAL_RX_SC_VALIDATE_FRAME_CHECK,
	FAL_RX_SC_VALIDATE_FRAME_DISABLED,
	FAL_RX_SC_VALIDATE_FRAME_MAX
} fal_rx_sc_validate_frame_e;

typedef struct {
	u8 sak[16];
	u8 sak1[16];
	u32 sak_len;
} fal_rx_sak_t;

struct fal_rx_sa_ki_t {
	u8 ki[16];
};

#define FAL_RX_UDF_FILT_MAX_ID     0x4

enum fal_rx_udf_filt_cfg_pattern_e {
	FAL_RX_FILTER_PATTERN_AND = 0,
	FAL_RX_FILTER_PATTERN_OR = 1,
	FAL_RX_FILTER_PATTERN_XOR = 2
};
struct fal_rx_udf_filt_cfg_t {
	bool enable;
	u16 priority;
	u16 inverse;
	enum fal_rx_udf_filt_cfg_pattern_e pattern0;
	enum fal_rx_udf_filt_cfg_pattern_e pattern1;
	enum fal_rx_udf_filt_cfg_pattern_e pattern2;
};
enum fal_rx_udf_filt_type_e {
	FAL_RX_FILTER_ANY_PACKET = 0,
	FAL_RX_FILTER_IP_PACKET = 1,
	FAL_RX_FILTER_TCP_PACKET = 2
};
enum fal_rx_udf_filt_op_e {
	FAL_RX_FILTER_OPERATOR_EQUAL = 0,
	FAL_RX_FILTER_OPERATOR_LESS = 1
};
struct fal_rx_udf_filt_t {
	u16 udf_field0;
	u16 udf_field1;
	u16 udf_field2;
	u16 mask;
	enum fal_rx_udf_filt_type_e type;
	enum fal_rx_udf_filt_op_e operator;
	u16 offset;
};

/**
* @param[in] secy_id
* @param[in] addr
* @param[out] pvalue
**/
u32 nss_macsec_secy_rx_reg_get(u32 secy_id, u32 addr, u32 *pvalue);

/**
* @param[in] secy_id
* @param[in] addr
* @param[in] value
**/
u32 nss_macsec_secy_rx_reg_set(u32 secy_id, u32 addr, u32 value);

/**
* @param[in] secy_id
* @param[in] filt_id
* @param[out] pfilt
**/
u32 nss_macsec_secy_rx_ctl_filt_get(u32 secy_id, u32 filt_id,
                                   fal_rx_ctl_filt_t *pfilt);

/**
* @param[in] secy_id
* @param[in] filt_id
* @param[in] pfilt
**/
u32 nss_macsec_secy_rx_ctl_filt_set(u32 secy_id, u32 filt_id,
                                   fal_rx_ctl_filt_t *pfilt);

/**
* @param[in] secy_id
* @param[in] filt_id
**/
u32 nss_macsec_secy_rx_ctl_filt_clear(u32 secy_id, u32 filt_id);

/**
* @param[in] secy_id
**/
u32 nss_macsec_secy_rx_ctl_filt_clear_all(u32 secy_id);

/**
* @param[in] secy_id
* @param[in] index
* @param[out] pentry
**/
u32 nss_macsec_secy_rx_prc_lut_get(u32 secy_id, u32 index,
                                  fal_rx_prc_lut_t *pentry);

/**
* @param[in] secy_id
* @param[in] index
* @param[in] pentry
**/
u32 nss_macsec_secy_rx_prc_lut_set(u32 secy_id, u32 index,
                                  fal_rx_prc_lut_t *pentry);

/**
* @param[in] secy_id
* @param[in] index
**/
u32 nss_macsec_secy_rx_prc_lut_clear(u32 secy_id, u32 index);

/**
* @param[in] secy_id
**/
u32 nss_macsec_secy_rx_prc_lut_clear_all(u32 secy_id);

/**
* @param[in] secy_id
* @param[in] channel
**/
u32 nss_macsec_secy_rx_sc_create(u32 secy_id, u32 channel);

/**
* @param[in] secy_id
* @param[in] channel
* @param[out] penable
**/
u32 nss_macsec_secy_rx_sc_en_get(u32 secy_id, u32 channel, bool *penable);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] enable
**/
u32 nss_macsec_secy_rx_sc_en_set(u32 secy_id, u32 channel, bool enable);

/**
* @param[in] secy_id
* @param[in] channel
**/
u32 nss_macsec_secy_rx_sc_del(u32 secy_id, u32 channel);

/**
* @param[in] secy_id
**/
u32 nss_macsec_secy_rx_sc_del_all(u32 secy_id);

/**
* @param[in] secy_id
* @param[in] channel
* @param[out] pmode
**/
u32 nss_macsec_secy_rx_sc_validate_frame_get(u32 secy_id, u32 channel,
                                            fal_rx_sc_validate_frame_e *pmode);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] mode
**/
u32 nss_macsec_secy_rx_sc_validate_frame_set(u32 secy_id, u32 channel,
                                            fal_rx_sc_validate_frame_e mode);

/**
* @param[in] secy_id
* @param[in] channel
* @param[out] penable
**/
u32 nss_macsec_secy_rx_sc_replay_protect_get(u32 secy_id, u32 channel,
                                           bool *penable);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] enable
**/
u32 nss_macsec_secy_rx_sc_replay_protect_set(u32 secy_id, u32 channel,
                                           bool enable);

/**
* @param[in] secy_id
* @param[in] channel
* @param[out] pwindow
**/
u32 nss_macsec_secy_rx_sc_anti_replay_window_get(u32 secy_id, u32 channel,
                                                u32 *pwindow);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] window
**/
u32 nss_macsec_secy_rx_sc_anti_replay_window_set(u32 secy_id, u32 channel,
                                                u32 window);

/**
* @param[in] secy_id
* @param[in] channel
* @param[out] p_in_used
**/
u32 nss_macsec_secy_rx_sc_in_used_get(u32 secy_id, u32 channel, bool *p_in_used);

/**
* @param[in] secy_id
* @param[in] channel
* @param[out] penable
**/
u32 nss_macsec_secy_rx_sc_an_roll_over_get(u32 secy_id, u32 channel,
                                          bool *penable);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] enable
**/
u32 nss_macsec_secy_rx_sc_an_roll_over_set(u32 secy_id, u32 channel, bool enable);

/**
* @param[in] secy_id
* @param[in] channel
* @param[out] p_start_time
* @param[out] p_stop_time
**/
u32 nss_macsec_secy_rx_sc_start_stop_time_get(u32 secy_id, u32 channel,
                                         u32 *p_start_time, u32 *p_stop_time);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] start_time
* @param[in] stop_time
**/
u32 nss_macsec_secy_rx_sc_start_stop_time_set(u32 secy_id, u32 channel,
                                             u32 start_time, u32 stop_time);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
**/
u32 nss_macsec_secy_rx_sa_create(u32 secy_id, u32 channel, u32 an);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[out] penable
**/
u32 nss_macsec_secy_rx_sa_en_get(u32 secy_id, u32 channel, u32 an, bool *penable);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[in] enable
**/
u32 nss_macsec_secy_rx_sa_en_set(u32 secy_id, u32 channel, u32 an, bool enable);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[out] pnpn
**/
u32 nss_macsec_secy_rx_sa_next_pn_get(u32 secy_id, u32 channel,
                                      u32 an, u32 *pnpn);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
**/
u32 nss_macsec_secy_rx_sa_del(u32 secy_id, u32 channel, u32 an);

/**
* @param[in] secy_id
**/
u32 nss_macsec_secy_rx_sa_del_all(u32 secy_id);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[out] pkey
**/
u32 nss_macsec_secy_rx_sak_get(u32 secy_id, u32 channel,
                              u32 an, fal_rx_sak_t *pkey);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[in] pkey
**/
u32 nss_macsec_secy_rx_sak_set(u32 secy_id, u32 channel,
                              u32 an, fal_rx_sak_t *pkey);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[out] p_in_used
**/
u32 nss_macsec_secy_rx_sa_in_used_get(u32 secy_id, u32 channel,
                                     u32 an, bool *p_in_used);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[out] p_start_time
* @param[out] p_stop_time
**/
u32 nss_macsec_secy_rx_sa_start_stop_time_get(u32 secy_id, u32 channel, u32 an,
                                         u32 *p_start_time, u32 *p_stop_time);

/**
* @param[in] secy_id
* @param[in] channel
* @param[in] an
* @param[in] start_time
* @param[in] stop_time
**/
u32 nss_macsec_secy_rx_sa_start_stop_time_set(u32 secy_id, u32 channel, u32 an,
                                             u32 start_time, u32 stop_time);

/**
* @param[in] secy_id
* @param[out] p_pn_threshold
**/
u32 nss_macsec_secy_rx_pn_threshold_get(u32 secy_id, u32 *p_pn_threshold);

/**
* @param[in] secy_id
* @param[in] pn_threshold
**/
u32 nss_macsec_secy_rx_pn_threshold_set(u32 secy_id, u32 pn_threshold);

/**
* @param[in] secy_id
* @param[out] enable
**/
u32 nss_macsec_secy_rx_replay_protect_get(u32 secy_id, u32 *enable);

/**
* @param[in] secy_id
* @param[in] enable
**/
u32 nss_macsec_secy_rx_replay_protect_set(u32 secy_id, u32 enable);

/**
* @param[in] secy_id
* @param[out] mode
**/
u32 nss_macsec_secy_rx_validate_frame_get(u32 secy_id, u32 *mode);

/**
* @param[in] secy_id
* @param[in] mode
**/
u32 nss_macsec_secy_rx_validate_frame_set(u32 secy_id, u32 mode);

u32 nss_macsec_secy_rx_sa_next_xpn_get(u32 secy_id, u32 channel,
	u32 an, u32 *p_next_xpn);
u32 nss_macsec_secy_rx_sa_next_xpn_set(u32 secy_id, u32 channel,
	u32 an, u32 next_xpn);
u32 nss_macsec_secy_rx_sc_ssci_get(u32 secy_id, u32 channel, u32 *pssci);
u32 nss_macsec_secy_rx_sc_ssci_set(u32 secy_id, u32 channel, u32 ssci);
u32 nss_macsec_secy_rx_sa_ki_get(u32 secy_id, u32 channel, u32 an,
	struct fal_rx_sa_ki_t *ki);
u32 nss_macsec_secy_rx_sa_ki_set(u32 secy_id, u32 channel, u32 an,
	struct fal_rx_sa_ki_t *ki);

u32 nss_macsec_secy_rx_udf_filt_set(u32 secy_id, u32 filt_id,
	struct fal_rx_udf_filt_t *pfilt);
u32 nss_macsec_secy_rx_udf_filt_get(u32 secy_id, u32 filt_id,
	struct fal_rx_udf_filt_t *pfilt);
u32 nss_macsec_secy_rx_udf_ufilt_cfg_set(u32 secy_id,
	struct fal_rx_udf_filt_cfg_t *cfg);
u32 nss_macsec_secy_rx_udf_ufilt_cfg_get(u32 secy_id,
	struct fal_rx_udf_filt_cfg_t *cfg);
u32 nss_macsec_secy_rx_sa_next_pn_set(u32 secy_id, u32 channel,
	u32 an, u32 next_pn);

#endif /* _NSS_MACSEC_SECY_RX_H_ */
