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

/**
 * @defgroup
 * @{
 */
#ifndef _NSS_MACSEC_OPS_H_
#define _NSS_MACSEC_OPS_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "qca808x_macsec.h"

#define PHY_NAPA 0x004DD100
#define PHY_NAPA_1_1 0x004DD101
#define MAX_SECY_ID      0x3

enum secy_cipher_suite_t {
	SECY_CIPHER_SUITE_128 = 0,
	SECY_CIPHER_SUITE_256 = 1,
	SECY_CIPHER_SUITE_XPN_128,
	SECY_CIPHER_SUITE_XPN_256,
	SECY_CIPHER_SUITE_MAX
};
enum secy_sc_sa_mapping_mode_t {
	SECY_SC_SA_MAP_1_1 = 0,
	SECY_SC_SA_MAP_1_2 = 1,
	SECY_SC_SA_MAP_1_4 = 2,
	SECY_SC_SA_MAP_MAX = 3
};
/** SecY special packet type */
enum secy_packet_type_t {
	PACKET_STP   = 0,   /*< Spanning Tree Protocol Packet */
	PACKET_CDP   = 1,   /*< Cisco Discovery Protocol Packet */
	PACKET_LLDP  = 2   /*< Link Layer Discovery Protocol Packet */
};
enum secy_packet_action_t {
	PACKET_CRYPTO_OR_DISCARD = 0X0,
	PACKET_CRYPTO_OR_UPLOAD  = 0X1,
	PACKET_PLAIN_OR_DISCARD  = 0X2,
	PACKET_PLAIN_OR_UPLOAD   = 0X3
};
/** SecY loopback type */
enum secy_loopback_type_t {
	MACSEC_NORMAL     = 0x0,
	MACSEC_PHY_LB     = 0x1,
	MACSEC_SWITCH_LB  = 0x2,
	MACSEC_BYPASS     = 0x3
};
enum secy_rx_prc_lut_action_t {
	SECY_RX_PRC_ACTION_PROCESS,
	SECY_RX_PRC_ACTION_PROCESS_WITH_SECTAG,
	SECY_RX_PRC_ACTION_BYPASS,
	SECY_RX_PRC_ACTION_DROP,
	SECY_RX_PRC_ACTION_MAX
};
struct secy_rx_prc_lut_t {
	bool valid;
	bool uncontrolled_port;
	enum secy_rx_prc_lut_action_t action;
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
};
enum secy_vf_t {
	STRICT    = 0,
	CHECKED   = 1,
	DISABLED  = 2
};
struct secy_sak_t {
	u8 sak[16];
	u8 sak1[16];
	u32 len;
};
struct secy_sa_ki_t {
	u8 ki[16];
};
enum secy_cofidentiality_offset_t {
	SECY_CONFIDENTIALITY_OFFSET_00 = 0, /**< 0  byte */
	SECY_CONFIDENTIALITY_OFFSET_30 = 1, /**< 30 bytes */
	SECY_CONFIDENTIALITY_OFFSET_50 = 2 /**< 50 bytes */
};
enum secy_tx_class_lut_action_t {
	SECY_TX_CLASS_ACTION_FORWARD,
	SECY_TX_CLASS_ACTION_BYPASS,
	SECY_TX_CLASS_ACTION_DROP,
	SECY_TX_CLASS_ACTION_MAX
};
struct secy_tx_class_lut_t {
	bool valid;
	enum secy_tx_class_lut_action_t action;
	u32 channel;
	u8 da[6];
	u8 da_mask;		/* first 6 bits are useful */
	u8 sa[6];
	u8 sa_mask;		/* first 6 bits are useful */
	u16 ether_type;
	u8 ether_type_mask;	/* first 2 bits are useful */
	bool udf0_valid;
	u8 udf0_byte;
	u8 udf0_location;	/* first 6 bits are useful */
	bool udf1_valid;
	u8 udf1_byte;
	u8 udf1_location;	/* first 6 bits are useful */
	bool udf2_valid;
	u8 udf2_byte;
	u8 udf2_location;	/* first 6 bits are useful */
	bool udf3_valid;
	u8 udf3_byte;
	u8 udf3_location;	/* first 6 bits are useful */
	bool vlan_valid;
	u8 vlan_valid_mask;	/* first bit is useful */
	u8 vlan_up;
	u8 vlan_up_mask;	/* first bit is useful */
	u16 vlan_id;
	u8 vlan_id_mask;	/* first 2 bits are useful */
	/*add for NAPA PHY*/
	u16 outer_vlanid;
	u8 sci[8];
	u8 tci;
	u8 offset;
	bool bc_flag;
	u32 rule_mask;
};
struct secy_tx_sa_mib_t {
	u64 hit_drop_redirect;
	u64 protected2_pkts;
	u64 protected_pkts;
	u64 encrypted_pkts;
};
struct secy_tx_sc_mib_t {
	u64 protected_pkts;
	u64 encrypted_pkts;
	u64 protected_octets;
	u64 encrypted_octets;
};
struct secy_tx_mib_t {
	u64 ctl_pkts;
	u64 unknown_sa_pkts;
	u64 untagged_pkts;
	u64 too_long;
	u64 ecc_error_pkts;
	u64 unctrl_hit_drop_redir_pkts;
};
struct secy_rx_sa_mib_t {
	u64 untagged_hit_pkts;
	u64 hit_drop_redir_pkts;
	u64 not_using_sa;
	u64 unused_sa;
	u64 not_valid_pkts;
	u64 invalid_pkts;
	u64 ok_pkts;
	u64 late_pkts;
	u64 delayed_pkts;
	u64 unchecked_pkts;
	u64 validated_octets;
	u64 decrypted_octets;
};
struct secy_rx_mib_t {
	u64 unctrl_prt_tx_octets;
	u64 ctl_pkts;
	u64 tagged_miss_pkts;
	u64 untagged_hit_pkts;
	u64 notag_pkts;
	u64 untagged_pkts;
	u64 bad_tag_pkts;
	u64 no_sci_pkts;
	u64 unknown_sci_pkts;
	u64 ctrl_prt_pass_pkts;
	u64 unctrl_prt_pass_pkts;
	u64 ctrl_prt_fail_pkts;
	u64 unctrl_prt_fail_pkts;
	u64 too_long_packets;
	u64 igpoc_ctl_pkts;
	u64 ecc_error_pkts;
};
enum secy_udf_filt_type_t {
	SECY_FILTER_ANY_PACKET = 0,
	SECY_FILTER_IP_PACKET = 1,
	SECY_FILTER_TCP_PACKET = 2
};
enum secy_udf_filt_op_t {
	SECY_FILTER_OPERATOR_EQUAL = 0,
	SECY_FILTER_OPERATOR_LESS = 1
};
struct secy_udf_filt_t {
	u16 udf_field0;
	u16 udf_field1;
	u16 udf_field2;
	u16 mask;
	enum secy_udf_filt_type_t type;
	enum secy_udf_filt_op_t operator;
	u16 offset;
};
enum secy_udf_filt_cfg_pattern_t {
	SECY_FILTER_PATTERN_AND = 0,
	SECY_FILTER_PATTERN_OR = 1,
	SECY_FILTER_PATTERN_XOR = 2
};
struct secy_udf_filt_cfg_t {
	bool enable;
	u16 priority;
	u16 inverse;
	enum secy_udf_filt_cfg_pattern_t pattern0;
	enum secy_udf_filt_cfg_pattern_t pattern1;
	enum secy_udf_filt_cfg_pattern_t pattern2;
};
enum secy_ctl_match_type_t {
	SECY_CTL_COMPARE_NO,
	SECY_CTL_COMPARE_DA,
	SECY_CTL_COMPARE_SA,
	SECY_CTL_COMPARE_HALF_DA_SA,
	SECY_CTL_COMPARE_ETHER_TYPE,
	SECY_CTL_COMPARE_DA_ETHER_TYPE,
	SECY_CTL_COMPARE_SA_ETHER_TYPE,
	SECY_CTL_COMPARE_DA_RANGE,
	SECY_CTL_COMPARE_MAX
};
struct secy_ctl_filt_t {
	bool bypass;
	enum secy_ctl_match_type_t match_type;
	u16 match_mask;
	u16 ether_type_da_range;
	u8 sa_da_addr[6];
};

typedef g_error_t (*secy_init_func)(u32 secy_id);
typedef g_error_t (*secy_en_set_func)(u32 secy_id, bool enable);
typedef g_error_t (*secy_en_get_func)(u32 secy_id, bool *enable);
typedef g_error_t (*secy_mtu_get_func)(u32 secy_id, u32 *pmtu);
typedef g_error_t (*secy_mtu_set_func)(u32 secy_id, u32 mtu);
typedef g_error_t (*secy_cipher_suite_get_func)(u32 secy_id,
	enum secy_cipher_suite_t *p_cipher_suite);
typedef g_error_t (*secy_cipher_suite_set_func)(u32 secy_id,
	enum secy_cipher_suite_t cipher_suite);
typedef g_error_t (*secy_controlled_port_en_get_func)(u32 secy_id,
	bool *penable);
typedef g_error_t (*secy_controlled_port_en_set_func)(u32 secy_id,
	bool enable);
typedef g_error_t (*secy_sc_sa_mapping_mode_get_func)(u32 secy_id,
	enum secy_sc_sa_mapping_mode_t *pmode);
typedef g_error_t (*secy_sc_sa_mapping_mode_set_func)(u32 secy_id,
	enum secy_sc_sa_mapping_mode_t mode);
typedef g_error_t (*secy_xpn_en_set_func)(u32 secy_id, bool enable);
typedef g_error_t (*secy_xpn_en_get_func)(u32 secy_id, bool *enable);
typedef g_error_t (*secy_flow_control_en_set_func)(u32 secy_id, bool enable);
typedef g_error_t (*secy_flow_control_en_get_func)(u32 secy_id, bool *enable);
typedef g_error_t (*secy_special_pkt_ctrl_set_func)(u32 secy_id,
	enum secy_packet_type_t packet_type,
	enum secy_packet_action_t action);
typedef g_error_t (*secy_special_pkt_ctrl_get_func)(u32 secy_id,
	enum secy_packet_type_t packet_type,
	enum secy_packet_action_t *action);
typedef g_error_t (*secy_udf_ethtype_set_func)(u32 secy_id, bool enable,
	u16 type);
typedef g_error_t (*secy_udf_ethtype_get_func)(u32 secy_id, bool *enable,
	u16 *type);
typedef g_error_t (*secy_loopback_set_func)(u32 secy_id,
	enum secy_loopback_type_t type);
typedef g_error_t (*secy_loopback_get_func)(u32 secy_id,
	enum secy_loopback_type_t *type);
typedef g_error_t (*secy_channel_number_get_func)(u32 secy_id, u32 *number);


/* rx */
typedef g_error_t (*secy_rx_prc_lut_get_func)(u32 secy_id, u32 index,
	struct secy_rx_prc_lut_t *pentry);
typedef g_error_t (*secy_rx_prc_lut_set_func)(u32 secy_id, u32 index,
	struct secy_rx_prc_lut_t *pentry);
typedef g_error_t (*secy_rx_sc_create_func)(u32 secy_id, u32 channel);
typedef g_error_t (*secy_rx_sc_del_func)(u32 secy_id, u32 channel);
typedef g_error_t (*secy_rx_sc_del_all_func)(u32 secy_id);
typedef g_error_t (*secy_rx_sc_validate_frame_get_func)(u32 secy_id,
	u32 channel, enum secy_vf_t *pmode);
typedef g_error_t (*secy_rx_sc_validate_frame_set_func)(u32 secy_id,
	u32 channel, enum secy_vf_t mode);
typedef g_error_t (*secy_rx_sc_replay_protect_get_func)(u32 secy_id,
	u32 channel, bool *penable);
typedef g_error_t (*secy_rx_sc_replay_protect_set_func)(u32 secy_id,
	u32 channel, bool enable);
typedef g_error_t (*secy_rx_sc_replay_window_get_func)(u32 secy_id,
	u32 channel, u32 *pwindow);
typedef g_error_t (*secy_rx_sc_replay_window_set_func)(u32 secy_id,
	u32 channel, u32 window);
typedef g_error_t (*secy_rx_sa_create_func)(u32 secy_id, u32 channel, u32 an);
typedef g_error_t (*secy_rx_sak_get_func)(u32 secy_id, u32 channel,
	u32 an, struct secy_sak_t *pkey);
typedef g_error_t (*secy_rx_sak_set_func)(u32 secy_id, u32 channel,
	u32 an, struct secy_sak_t *pkey);
typedef g_error_t (*secy_rx_sa_en_get_func)(u32 secy_id, u32 channel,
	u32 an, bool *penable);
typedef g_error_t (*secy_rx_sa_en_set_func)(u32 secy_id, u32 channel,
	u32 an, bool enable);
typedef g_error_t (*secy_rx_sc_in_used_get_func)(u32 secy_id,
	u32 channel, bool *p_in_used);
typedef g_error_t (*secy_rx_sa_next_pn_get_func)(u32 secy_id, u32 channel,
	u32 an, u32 *pnpn);
typedef g_error_t (*secy_rx_reg_get_func)(u32 secy_id, u32 addr, u16 *pvalue);
typedef g_error_t (*secy_rx_reg_set_func)(u32 secy_id, u32 addr, u16 value);
typedef g_error_t (*secy_rx_sa_next_xpn_get_func)(u32 secy_id, u32 channel,
	u32 an, u32 *pnpn);
typedef g_error_t (*secy_rx_sa_next_xpn_set_func)(u32 secy_id, u32 channel,
	u32 an, u32 pnpn);
typedef g_error_t (*secy_rx_sc_ssci_get_func)(u32 secy_id, u32 channel,
	u32 *ssci);
typedef g_error_t (*secy_rx_sc_ssci_set_func)(u32 secy_id, u32 channel,
	u32 ssci);
typedef g_error_t (*secy_rx_sa_ki_get_func)(u32 secy_id, u32 channel, u32 an,
	struct secy_sa_ki_t *ki);
typedef g_error_t (*secy_rx_sa_ki_set_func)(u32 secy_id, u32 channel, u32 an,
	struct secy_sa_ki_t *ki);
typedef g_error_t (*secy_rx_prc_lut_clear_func)(u32 secy_id, u32 index);
typedef g_error_t (*secy_rx_prc_lut_clear_all_func)(u32 secy_id);
typedef g_error_t (*secy_rx_sa_del_func)(u32 secy_id, u32 channel, u32 an);
typedef g_error_t (*secy_rx_sa_del_all_func)(u32 secy_id);

typedef g_error_t (*secy_rx_sa_in_used_get_func)(u32 secy_id, u32 channel,
	u32 an, bool *p_in_used);
typedef g_error_t (*secy_rx_replay_protect_get_func)(u32 secy_id,
	bool *enable);
typedef g_error_t (*secy_rx_replay_protect_set_func)(u32 secy_id,
	bool enable);
typedef g_error_t (*secy_rx_validate_frame_get_func)(u32 secy_id,
	enum secy_vf_t *pmode);
typedef g_error_t (*secy_rx_validate_frame_set_func)(u32 secy_id,
	enum secy_vf_t mode);
typedef g_error_t (*secy_rx_sc_en_get_func)(u32 secy_id, u32 channel,
	bool *penable);
typedef g_error_t (*secy_rx_sc_en_set_func)(u32 secy_id, u32 channel,
	bool enable);
typedef g_error_t (*secy_rx_sa_next_pn_set_func)(u32 secy_id, u32 channel,
	u32 an, u32 next_pn);


/* tx */

typedef g_error_t (*secy_tx_sc_in_used_get_func)(u32 secy_id, u32 channel,
	bool *p_in_used);
typedef g_error_t (*secy_tx_sc_tci_7_2_get_func)(u32 secy_id, u32 channel,
	u8 *ptci);
typedef g_error_t (*secy_tx_sc_tci_7_2_set_func)(u32 secy_id, u32 channel,
	u8 tci);
typedef g_error_t (*secy_tx_sc_create_func)(u32 secy_id, u32 channel,
	u8 *psci, u32 sci_len);
typedef g_error_t (*secy_tx_sc_sci_get_func)(u32 secy_id, u32 channel,
	u8 *psci, u32 sci_len);
typedef g_error_t (*secy_tx_sa_en_get_func)(u32 secy_id, u32 channel, u32 an,
	bool *penable);
typedef g_error_t (*secy_tx_sa_en_set_func)(u32 secy_id, u32 channel, u32 an,
	bool enable);
typedef g_error_t (*secy_tx_sa_next_pn_get_func)(u32 secy_id, u32 channel,
	u32 an, u32 *p_next_pn);
typedef g_error_t (*secy_tx_sa_next_pn_set_func)(u32 secy_id, u32 channel,
	u32 an, u32 next_pn);
typedef g_error_t (*secy_tx_sc_an_get_func)(u32 secy_id, u32 channel,
	u32 *pan);
typedef g_error_t (*secy_tx_sc_an_set_func)(u32 secy_id, u32 channel, u32 an);
typedef g_error_t (*secy_tx_sc_del_func)(u32 secy_id, u32 channel);
typedef g_error_t (*secy_tx_sc_del_all_func)(u32 secy_id);
typedef g_error_t (*secy_tx_sc_offset_get_func)(u32 secy_id,
	u32 channel, enum secy_cofidentiality_offset_t *offset);
typedef g_error_t (*secy_tx_sc_offset_set_func)(u32 secy_id,
	u32 channel, enum secy_cofidentiality_offset_t offset);
typedef g_error_t (*secy_tx_class_lut_get_func)(u32 secy_id, u32 index,
	struct secy_tx_class_lut_t *pentry);
typedef g_error_t (*secy_tx_class_lut_set_func)(u32 secy_id, u32 index,
	struct secy_tx_class_lut_t *pentry);

typedef g_error_t (*secy_tx_sak_get_func)(u32 secy_id, u32 channel,
	u32 an, struct secy_sak_t *pentry);
typedef g_error_t (*secy_tx_sak_set_func)(u32 secy_id, u32 channel,
	u32 an, struct secy_sak_t *pentry);
typedef g_error_t (*secy_tx_sc_protect_get_func)(u32 secy_id, u32 channel,
	bool *penable);
typedef g_error_t (*secy_tx_sc_protect_set_func)(u32 secy_id, u32 channel,
	bool enable);
typedef g_error_t (*secy_tx_reg_get_func)(u32 secy_id, u32 addr, u16 *pvalue);
typedef g_error_t (*secy_tx_reg_set_func)(u32 secy_id, u32 addr, u16 value);
typedef g_error_t (*secy_tx_sa_next_xpn_get_func)(u32 secy_id, u32 channel,
	u32 an, u32 *pnpn);
typedef g_error_t (*secy_tx_sa_next_xpn_set_func)(u32 secy_id, u32 channel,
	u32 an, u32 pnpn);
typedef g_error_t (*secy_tx_sc_ssci_get_func)(u32 secy_id, u32 channel,
	u32 *ssci);
typedef g_error_t (*secy_tx_sc_ssci_set_func)(u32 secy_id, u32 channel,
	u32 ssci);
typedef g_error_t (*secy_tx_sa_ki_get_func)(u32 secy_id, u32 channel, u32 an,
	struct secy_sa_ki_t *ki);
typedef g_error_t (*secy_tx_sa_ki_set_func)(u32 secy_id, u32 channel, u32 an,
	struct secy_sa_ki_t *ki);
typedef g_error_t (*secy_tx_class_lut_clear_func)(u32 secy_id, u32 index);
typedef g_error_t (*secy_tx_class_lut_clear_all_func)(u32 secy_id);
typedef g_error_t (*secy_tx_sa_create_func)(u32 secy_id, u32 channel, u32 an);
typedef g_error_t (*secy_tx_sa_del_func)(u32 secy_id, u32 channel, u32 an);
typedef g_error_t (*secy_tx_sa_del_all_func)(u32 secy_id);
typedef g_error_t (*secy_tx_sa_in_used_get_func)(u32 secy_id, u32 channel,
	u32 an, bool *p_in_used);
typedef g_error_t (*secy_tx_sc_en_get_func)(u32 secy_id, u32 channel,
	bool *penable);
typedef g_error_t (*secy_tx_sc_en_set_func)(u32 secy_id, u32 channel,
	bool enable);
typedef g_error_t (*secy_tx_class_lut_sci_update_func)(u32 secy_id,
	u32 channel, u8 *sci);


/*mib */

typedef g_error_t (*secy_tx_sa_mib_get_func)(u32 secy_id, u32 channel, u32 an,
	struct secy_tx_sa_mib_t *pmib);
typedef g_error_t (*secy_tx_sc_mib_get_func)(u32 secy_id, u32 channel,
	struct secy_tx_sc_mib_t *pmib);
typedef g_error_t (*secy_tx_mib_get_func)(u32 secy_id,
	struct secy_tx_mib_t *pmib);
typedef g_error_t (*secy_rx_sa_mib_get_func)(u32 secy_id, u32 channel, u32 an,
	struct secy_rx_sa_mib_t *pmib);
typedef g_error_t (*secy_rx_mib_get_func)(u32 secy_id,
	struct secy_rx_mib_t *pmib);
typedef g_error_t (*secy_tx_sa_mib_clear_func)(u32 secy_id, u32 channel,
	u32 an);
typedef g_error_t (*secy_tx_sc_mib_clear_func)(u32 secy_id, u32 channel);
typedef g_error_t (*secy_tx_mib_clear_func)(u32 secy_id);
typedef g_error_t (*secy_rx_sa_mib_clear_func)(u32 secy_id, u32 channel,
	u32 an);
typedef g_error_t (*secy_rx_mib_clear_func)(u32 secy_id);

/*filter*/
typedef g_error_t (*secy_tx_udf_ufilt_cfg_set_func)(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg);
typedef g_error_t (*secy_tx_udf_ufilt_cfg_get_func)(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg);
typedef g_error_t (*secy_tx_udf_cfilt_cfg_set_func)(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg);
typedef g_error_t (*secy_tx_udf_cfilt_cfg_get_func)(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg);
typedef g_error_t (*secy_rx_udf_ufilt_cfg_set_func)(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg);
typedef g_error_t (*secy_rx_udf_ufilt_cfg_get_func)(u32 secy_id,
	struct secy_udf_filt_cfg_t *cfg);
typedef g_error_t (*secy_rx_udf_filt_set_func)(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter);
typedef g_error_t (*secy_rx_udf_filt_get_func)(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter);
typedef g_error_t (*secy_tx_udf_filt_set_func)(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter);
typedef g_error_t (*secy_tx_udf_filt_get_func)(u32 secy_id, u32 index,
	struct secy_udf_filt_t *filter);
typedef g_error_t (*secy_tx_ctl_filt_set_func)(u32 secy_id, u32 filt_id,
	struct secy_ctl_filt_t *pfilt);
typedef g_error_t (*secy_tx_ctl_filt_get_func)(u32 secy_id, u32 filt_id,
	struct secy_ctl_filt_t *pfilt);
typedef g_error_t (*secy_tx_ctl_filt_clear_func)(u32 secy_id, u32 filt_id);
typedef g_error_t (*secy_tx_ctl_filt_clear_all_func)(u32 secy_id);
typedef g_error_t (*secy_rx_ctl_filt_set_func)(u32 secy_id, u32 filt_id,
	struct secy_ctl_filt_t *pfilt);
typedef g_error_t (*secy_rx_ctl_filt_get_func)(u32 secy_id, u32 filt_id,
	struct secy_ctl_filt_t *pfilt);
typedef g_error_t (*secy_rx_ctl_filt_clear_func)(u32 secy_id, u32 filt_id);
typedef g_error_t (*secy_rx_ctl_filt_clear_all_func)(u32 secy_id);

struct secy_drv_t {
	secy_init_func secy_init;
	secy_controlled_port_en_get_func secy_controlled_port_en_get;
	secy_controlled_port_en_set_func secy_controlled_port_en_set;
	secy_cipher_suite_get_func secy_cipher_suite_get;
	secy_cipher_suite_set_func secy_cipher_suite_set;
	secy_mtu_get_func secy_mtu_get;
	secy_mtu_set_func secy_mtu_set;
	secy_en_set_func secy_en_set;
	secy_en_get_func secy_en_get;
	secy_sc_sa_mapping_mode_set_func secy_sc_sa_mapping_mode_set;
	secy_sc_sa_mapping_mode_get_func secy_sc_sa_mapping_mode_get;
	secy_xpn_en_set_func secy_xpn_en_set;
	secy_xpn_en_get_func secy_xpn_en_get;
	secy_flow_control_en_set_func secy_flow_control_en_set;
	secy_flow_control_en_get_func secy_flow_control_en_get;
	secy_special_pkt_ctrl_set_func secy_special_pkt_ctrl_set;
	secy_special_pkt_ctrl_get_func secy_special_pkt_ctrl_get;
	secy_udf_ethtype_set_func secy_udf_ethtype_set;
	secy_udf_ethtype_get_func secy_udf_ethtype_get;
	secy_loopback_set_func secy_loopback_set;
	secy_loopback_get_func secy_loopback_get;
	secy_channel_number_get_func secy_channel_number_get;


	/*rx */
	secy_rx_prc_lut_get_func secy_rx_prc_lut_get;
	secy_rx_prc_lut_set_func secy_rx_prc_lut_set;
	secy_rx_sc_create_func secy_rx_sc_create;
	secy_rx_sc_del_func secy_rx_sc_del;
	secy_rx_sc_del_all_func secy_rx_sc_del_all;
	secy_rx_sc_validate_frame_get_func secy_rx_sc_validate_frame_get;
	secy_rx_sc_validate_frame_set_func secy_rx_sc_validate_frame_set;
	secy_rx_sc_replay_protect_get_func secy_rx_sc_replay_protect_get;
	secy_rx_sc_replay_protect_set_func secy_rx_sc_replay_protect_set;
	secy_rx_sc_replay_window_get_func secy_rx_sc_anti_replay_window_get;
	secy_rx_sc_replay_window_set_func secy_rx_sc_anti_replay_window_set;
	secy_rx_sa_create_func secy_rx_sa_create;
	secy_rx_sak_get_func secy_rx_sak_get;
	secy_rx_sak_set_func secy_rx_sak_set;
	secy_rx_sa_en_get_func secy_rx_sa_en_get;
	secy_rx_sa_en_set_func secy_rx_sa_en_set;
	secy_rx_sa_next_pn_get_func secy_rx_sa_next_pn_get;
	secy_rx_sc_in_used_get_func secy_rx_sc_in_used_get;
	secy_rx_reg_get_func secy_rx_reg_get;
	secy_rx_reg_set_func secy_rx_reg_set;
	secy_rx_sa_next_xpn_set_func secy_rx_sa_next_xpn_set;
	secy_rx_sa_next_xpn_get_func secy_rx_sa_next_xpn_get;
	secy_rx_sc_ssci_set_func secy_rx_sc_ssci_set;
	secy_rx_sc_ssci_get_func secy_rx_sc_ssci_get;
	secy_rx_sa_ki_set_func secy_rx_sa_ki_set;
	secy_rx_sa_ki_get_func secy_rx_sa_ki_get;
	secy_rx_prc_lut_clear_func secy_rx_prc_lut_clear;
	secy_rx_prc_lut_clear_all_func secy_rx_prc_lut_clear_all;
	secy_rx_sa_in_used_get_func secy_rx_sa_in_used_get;
	secy_rx_replay_protect_get_func secy_rx_replay_protect_get;
	secy_rx_replay_protect_set_func secy_rx_replay_protect_set;
	secy_rx_validate_frame_get_func secy_rx_validate_frame_get;
	secy_rx_validate_frame_set_func secy_rx_validate_frame_set;
	secy_rx_sc_en_get_func secy_rx_sc_en_get;
	secy_rx_sc_en_set_func secy_rx_sc_en_set;
	secy_rx_sa_del_func secy_rx_sa_del;
	secy_rx_sa_del_all_func secy_rx_sa_del_all;
	secy_rx_sa_next_pn_set_func secy_rx_sa_next_pn_set;

	/*tx*/
	secy_tx_sc_in_used_get_func secy_tx_sc_in_used_get;
	secy_tx_sc_tci_7_2_get_func secy_tx_sc_tci_7_2_get;
	secy_tx_sc_tci_7_2_set_func secy_tx_sc_tci_7_2_set;
	secy_tx_sa_next_pn_get_func secy_tx_sa_next_pn_get;
	secy_tx_sa_next_pn_set_func secy_tx_sa_next_pn_set;
	secy_tx_sc_an_get_func secy_tx_sc_an_get;
	secy_tx_sc_an_set_func secy_tx_sc_an_set;
	secy_tx_sa_en_get_func secy_tx_sa_en_get;
	secy_tx_sa_en_set_func secy_tx_sa_en_set;
	secy_tx_sc_create_func secy_tx_sc_create;
	secy_tx_sc_sci_get_func secy_tx_sc_sci_get;
	secy_tx_sc_del_func secy_tx_sc_del;
	secy_tx_sc_del_all_func secy_tx_sc_del_all;
	secy_tx_sc_offset_get_func secy_tx_sc_confidentiality_offset_get;
	secy_tx_sc_offset_set_func secy_tx_sc_confidentiality_offset_set;
	secy_tx_class_lut_get_func secy_tx_class_lut_get;
	secy_tx_class_lut_set_func secy_tx_class_lut_set;
	secy_tx_sak_get_func secy_tx_sak_get;
	secy_tx_sak_set_func secy_tx_sak_set;
	secy_tx_sc_protect_get_func secy_tx_sc_protect_get;
	secy_tx_sc_protect_set_func secy_tx_sc_protect_set;
	secy_tx_reg_get_func secy_tx_reg_get;
	secy_tx_reg_set_func secy_tx_reg_set;
	secy_tx_sa_next_xpn_set_func secy_tx_sa_next_xpn_set;
	secy_tx_sa_next_xpn_get_func secy_tx_sa_next_xpn_get;
	secy_tx_sc_ssci_set_func secy_tx_sc_ssci_set;
	secy_tx_sc_ssci_get_func secy_tx_sc_ssci_get;
	secy_tx_sa_ki_set_func secy_tx_sa_ki_set;
	secy_tx_sa_ki_get_func secy_tx_sa_ki_get;
	secy_tx_class_lut_clear_func secy_tx_class_lut_clear;
	secy_tx_class_lut_clear_all_func secy_tx_class_lut_clear_all;
	secy_tx_sa_create_func secy_tx_sa_create;
	secy_tx_sa_del_func secy_tx_sa_del;
	secy_tx_sa_del_all_func secy_tx_sa_del_all;
	secy_tx_sa_in_used_get_func secy_tx_sa_in_used_get;
	secy_tx_sc_en_get_func secy_tx_sc_en_get;
	secy_tx_sc_en_set_func secy_tx_sc_en_set;
	secy_tx_class_lut_sci_update_func secy_tx_class_lut_sci_update;

	/*mib*/
	secy_tx_sa_mib_get_func secy_tx_sa_mib_get;
	secy_tx_sc_mib_get_func secy_tx_sc_mib_get;
	secy_tx_mib_get_func secy_tx_mib_get;
	secy_rx_sa_mib_get_func secy_rx_sa_mib_get;
	secy_rx_mib_get_func secy_rx_mib_get;
	secy_tx_sa_mib_clear_func secy_tx_sa_mib_clear;
	secy_tx_sc_mib_clear_func secy_tx_sc_mib_clear;
	secy_tx_mib_clear_func secy_tx_mib_clear;
	secy_rx_sa_mib_clear_func secy_rx_sa_mib_clear;
	secy_rx_mib_clear_func secy_rx_mib_clear;

	secy_tx_udf_ufilt_cfg_set_func secy_tx_udf_ufilt_cfg_set;
	secy_tx_udf_ufilt_cfg_get_func secy_tx_udf_ufilt_cfg_get;
	secy_tx_udf_cfilt_cfg_set_func secy_tx_udf_cfilt_cfg_set;
	secy_tx_udf_cfilt_cfg_get_func secy_tx_udf_cfilt_cfg_get;
	secy_rx_udf_ufilt_cfg_set_func secy_rx_udf_ufilt_cfg_set;
	secy_rx_udf_ufilt_cfg_get_func secy_rx_udf_ufilt_cfg_get;
	secy_rx_udf_filt_set_func secy_rx_udf_filt_set;
	secy_rx_udf_filt_get_func secy_rx_udf_filt_get;
	secy_tx_udf_filt_set_func secy_tx_udf_filt_set;
	secy_tx_udf_filt_get_func secy_tx_udf_filt_get;
	secy_tx_ctl_filt_set_func secy_tx_ctl_filt_set;
	secy_tx_ctl_filt_get_func secy_tx_ctl_filt_get;
	secy_tx_ctl_filt_clear_func secy_tx_ctl_filt_clear;
	secy_tx_ctl_filt_clear_all_func secy_tx_ctl_filt_clear_all;
	secy_rx_ctl_filt_set_func secy_rx_ctl_filt_set;
	secy_rx_ctl_filt_get_func secy_rx_ctl_filt_get;
	secy_rx_ctl_filt_clear_func secy_rx_ctl_filt_clear;
	secy_rx_ctl_filt_clear_all_func secy_rx_ctl_filt_clear_all;

};
enum secy_phy_type_t {
	NAPA_PHY_CHIP = 0,
	MAX_PHY_CHIP,
};
struct secy_driver_instance_t {
	enum secy_phy_type_t secy_phy_type;
	struct secy_drv_t *secy_ops;
	int (*init)(u32 secy_id);
	int (*exit)(u32 secy_id);
};
struct secy_info_t {
	u32 phy_addr;
	enum secy_phy_type_t secy_phy_type;
};

g_error_t secy_ops_init(enum secy_phy_type_t phy_type);

g_error_t secy_ops_register(enum secy_phy_type_t phy_type,
	struct secy_drv_t *secy_ops);

struct secy_drv_t *nss_macsec_secy_ops_get(u32 secy_id);

void nss_macsec_secy_info_set(u32 secy_id, u32 phy_addr);

u32 secy_id_to_phy_addr(u32 secy_id);

int nss_macsec_secy_driver_init(u32 secy_id);

g_error_t nss_macsec_secy_driver_cleanup(u32 secy_id);

g_error_t secy_ops_unregister(enum secy_phy_type_t phy_type);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif


