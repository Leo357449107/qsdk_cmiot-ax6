/*
 * Copyright (c) 2016-2018, 2021, The Linux Foundation. All rights reserved.
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
 * @defgroup fal_gen FAL_CTRLPKT
 * @{
 */
#ifndef _FAL_CTRLPKT_H_
#define _FAL_CTRLPKT_H_

#ifdef __cplusplus
extern "C" {
#endif
/* __cplusplus */

#include "sw.h"
#include "fal/fal_type.h"


#if defined(SW_API_LOCK) && (!defined(HSL_STANDALONG))
#define FAL_CTRLPKT_API_LOCK
#define FAL_CTRLPKT_API_UNLOCK
#else
#define FAL_CTRLPKT_API_LOCK
#define FAL_CTRLPKT_API_UNLOCK
#endif


typedef struct {
	fal_fwd_cmd_t action; /* the action when condition matched */
	a_bool_t sg_bypass; /* check if sg_bypass when condition matched */
	a_bool_t l2_filter_bypass; /* check if l2_filter_bypass when condition matched */
	a_bool_t in_stp_bypass; /* check if in_stp_bypass when condition matched */
	a_bool_t in_vlan_fltr_bypass; /* check if in_vlan_fltr_bypass when condition matched */
} fal_ctrlpkt_action_t;

typedef struct
{
	a_bool_t mgt_eapol; /* eapol protocol management type */
	a_bool_t mgt_pppoe; /* pppoe protocol management type */
	a_bool_t mgt_igmp; /* igmp protocol management type */
	a_bool_t mgt_arp_req; /* arp request protocol management type */
	a_bool_t mgt_arp_rep; /* arp response protocol management type */
	a_bool_t mgt_dhcp4; /* dhcp4 protocol management type */
	a_bool_t mgt_mld; /* mld protocol management type */
	a_bool_t mgt_ns; /* ns protocol management type */
	a_bool_t mgt_na; /* na protocol management type */
	a_bool_t mgt_dhcp6; /* dhcp6 protocol management type */
	a_bool_t mgt_8023ah_oam; /* 8023ah oam protocol management type add it for ipq95xx*/
} fal_ctrlpkt_protocol_type_t;

typedef struct {
	fal_ctrlpkt_action_t action; /* the all action when condition matched */
	fal_pbmp_t port_map; /* the condition port bitmap */
	a_uint32_t ethtype_profile_bitmap; /* the condition ethtype_profile bitmap */
	a_uint32_t rfdb_profile_bitmap; /* the condition rfdb_profile bitmap */
	fal_ctrlpkt_protocol_type_t protocol_types; /* the condition protocol types */
} fal_ctrlpkt_profile_t;

enum {
	FUNC_MGMTCTRL_ETHTYPE_PROFILE_SET = 0,
	FUNC_MGMTCTRL_ETHTYPE_PROFILE_GET,
	FUNC_MGMTCTRL_RFDB_PROFILE_SET,
	FUNC_MGMTCTRL_RFDB_PROFILE_GET,
	FUNC_MGMTCTRL_CTRLPKT_PROFILE_ADD,
	FUNC_MGMTCTRL_CTRLPKT_PROFILE_DEL,
	FUNC_MGMTCTRL_CTRLPKT_PROFILE_GETFIRST,
	FUNC_MGMTCTRL_CTRLPKT_PROFILE_GETNEXT,
	FUNC_MGMTCTRL_VPGROUP_SET,
	FUNC_MGMTCTRL_VPGROUP_GET,
	FUNC_MGMTCTRL_TUNNEL_DECAP_SET,
	FUNC_MGMTCTRL_TUNNEL_DECAP_GET,
};

sw_error_t fal_mgmtctrl_ethtype_profile_set(a_uint32_t dev_id, a_uint32_t profile_id, a_uint32_t ethtype);
sw_error_t fal_mgmtctrl_ethtype_profile_get(a_uint32_t dev_id, a_uint32_t profile_id, a_uint32_t * ethtype);

sw_error_t fal_mgmtctrl_rfdb_profile_set(a_uint32_t dev_id, a_uint32_t profile_id, fal_mac_addr_t *addr);
sw_error_t fal_mgmtctrl_rfdb_profile_get(a_uint32_t dev_id, a_uint32_t profile_id, fal_mac_addr_t *addr);

sw_error_t fal_mgmtctrl_ctrlpkt_profile_add(a_uint32_t dev_id, fal_ctrlpkt_profile_t *ctrlpkt);
sw_error_t fal_mgmtctrl_ctrlpkt_profile_del(a_uint32_t dev_id, fal_ctrlpkt_profile_t *ctrlpkt);
sw_error_t fal_mgmtctrl_ctrlpkt_profile_getfirst(a_uint32_t dev_id, fal_ctrlpkt_profile_t *ctrlpkt);
sw_error_t fal_mgmtctrl_ctrlpkt_profile_getnext(a_uint32_t dev_id, fal_ctrlpkt_profile_t *ctrlpkt);
sw_error_t fal_mgmtctrl_vpgroup_set(a_uint32_t dev_id, fal_port_t port_id, a_uint32_t vpgroup_id);
sw_error_t fal_mgmtctrl_vpgroup_get(a_uint32_t dev_id, fal_port_t port_id, a_uint32_t *vpgroup_id);
sw_error_t fal_mgmtctrl_tunnel_decap_set(a_uint32_t dev_id, a_uint32_t cpu_code_id,
	a_bool_t enable);
sw_error_t fal_mgmtctrl_tunnel_decap_get(a_uint32_t dev_id, a_uint32_t cpu_code_id,
	a_bool_t *enable);
#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _FAL_CTRLPKT_H_ */
/**
 * @}
 */

