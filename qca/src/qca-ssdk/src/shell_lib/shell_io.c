/*
 * Copyright (c) 2013, 2015-2017, 2019, 2021, The Linux Foundation. All rights reserved.
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

//#include <stdio.h>
#include "shell_io.h"
#include "shell.h"
#include "shell_sw.h"

#define SW_RTN_ON_NULL_PARAM(rtn) \
    do { if ((rtn) == NULL) return SW_BAD_PARAM; } while(0);

#define DEFAULT_FLAG "default"
#define MAX_ARRT_NUM 16
#define INVALID_ARRT_VALUE 0xFFFFFFFF
static char **full_cmdstrp;
static int talk_mode = 1;

#define cmd_data_check_element(info, defval, usage, chk_func, param) \
{\
    sw_error_t ret;\
    do {\
        cmd = get_sub_cmd(info, defval);\
        SW_RTN_ON_NULL_PARAM(cmd);\
        \
        if (!strncasecmp(cmd, "quit", 4)) {\
            return SW_BAD_VALUE;\
        } else if (!strncasecmp(cmd, "help", 4)) {\
            ret = SW_BAD_VALUE;\
        } else {\
            ret = chk_func param; \
        }\
    } while (talk_mode && (SW_OK != ret));\
}

struct sub_attr_des_t
{
	char *sub_attr_name;
	a_uint32_t value;
};

struct attr_des_t
{
	char *attr_name;
	struct sub_attr_des_t sub_attr_des[MAX_ARRT_NUM];
};

struct attr_des_t g_attr_des[] =
{
	{
		"dest_info_type",
		{
			{"port_bmp", FAL_DEST_INFO_PORT_BMP},
			{"port_id", FAL_DEST_INFO_PORT_ID},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"mtu_type",
		{
			{"ethernet", FAL_MTU_ETHERNET},
			{"ip", FAL_MTU_IP},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"ip_ver",
		{
			{"ipv4", 1},
			{"ipv6", 2},
			{"ipv4_and_ipv6", 3},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"udp_type",
		{
			{"udp", 1},
			{"udp-lite", 2},
			{"udp_and_udp-lite", 3},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"l4_port_type",
		{
			{"dst", 1},
			{"src", 2},
			{"dst_and_src", 3},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"hdr_type",
		{
			{"ethernet", FAL_ETHERNET_HDR},
			{"ethernet-tag", FAL_ETHERNET_TAG_HDR},
			{"ipv4", FAL_IPV4_HDR},
			{"ipv6", FAL_IPV6_HDR},
			{"udp", FAL_UDP_HDR},
			{"udp-lite", FAL_UDP_LITE_HDR},
			{"tcp", FAL_TCP_HDR},
			{"gre", FAL_GRE_HDR},
			{"vxlan", FAL_VXLAN_HDR},
			{"vxlan-gpe", FAL_VXLAN_GPE_HDR},
			{"geneve", FAL_GENEVE_HDR},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"l3_type",
		{
			{"others", FAL_L3_TYPE_OTHERS},
			{"ipv4", FAL_L3_TYPE_IPV4},
			{"arp", FAL_L3_TYPE_ARP},
			{"ipv6", FAL_L3_TYPE_IPV6},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"l4_type",
		{
			{"others", FAL_L4_TYPE_OTHERS},
			{"tcp", FAL_L4_TYPE_TCP},
			{"udp", FAL_L4_TYPE_UDP},
			{"udp-lite", FAL_L4_TYPE_UDP_LITE},
			{"icmp", FAL_L4_TYPE_ICMP},
			{"gre", FAL_L4_TYPE_GRE},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"vpn_type",
		{
			{"vsi", 0},
			{"vrf", 1},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#if defined(IN_ACL) || defined(IN_TUNNEL)
	{
		"tunnel_type",
		{
			{"gre_tap_ipv4", FAL_TUNNEL_TYPE_GRE_TAP_OVER_IPV4},
			{"gre_tap_ipv6", FAL_TUNNEL_TYPE_GRE_TAP_OVER_IPV6},
			{"vxlan_ipv4", FAL_TUNNEL_TYPE_VXLAN_OVER_IPV4},
			{"vxlan_ipv6", FAL_TUNNEL_TYPE_VXLAN_OVER_IPV6},
			{"vxlan_gpe_ipv4", FAL_TUNNEL_TYPE_VXLAN_GPE_OVER_IPV4},
			{"vxlan_gpe_ipv6", FAL_TUNNEL_TYPE_VXLAN_GPE_OVER_IPV6},
			{"ipv4_ipv6", FAL_TUNNEL_TYPE_IPV4_OVER_IPV6},
			{"program0", FAL_TUNNEL_TYPE_PROGRAM0},
			{"program1", FAL_TUNNEL_TYPE_PROGRAM1},
			{"program2", FAL_TUNNEL_TYPE_PROGRAM2},
			{"program3", FAL_TUNNEL_TYPE_PROGRAM3},
			{"program4", FAL_TUNNEL_TYPE_PROGRAM4},
			{"program5", FAL_TUNNEL_TYPE_PROGRAM5},
			{"geneve_ipv4", FAL_TUNNEL_TYPE_GENEVE_OVER_IPV4},
			{"geneve_ipv6", FAL_TUNNEL_TYPE_GENEVE_OVER_IPV6},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#endif
	{
		"vport_type",
		{
			{"tunnel", FAL_VPORT_TYPE_TUNNEL},
			{"0", FAL_VPORT_TYPE_TUNNEL},
			{"normal", FAL_VPORT_TYPE_NORMAL},
			{"1", FAL_VPORT_TYPE_NORMAL},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#ifdef IN_POLICER
	{
		"policer_meter_type",
		{
			{"rfc", FAL_POLICER_METER_RFC},
			{"mef10_3", FAL_POLICER_METER_MEF10_3},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"policer_meter_color",
		{
			{"meter_yellow", FAL_POLICER_METER_YELLOW},
			{"meter_red", FAL_POLICER_METER_RED},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#endif
#ifdef IN_SHAPER
	{
		"shaper_meter_type",
		{
			{"rfc", FAL_SHAPER_METER_RFC},
			{"mef10_3", FAL_SHAPER_METER_MEF10_3},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#endif
#ifdef IN_VXLAN
	{
		"vxlan_type",
		{
			{"vxlan", FAL_VXLAN},
			{"0", FAL_VXLAN},
			{"vxlan-gpe", FAL_VXLAN_GPE},
			{"1", FAL_VXLAN_GPE},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#endif
#if defined(IN_TUNNEL_PROGRAM) || defined(IN_TUNNEL)
	{
		"tunnel_program_type",
		{
			{"program0", FAL_TUNNEL_PROGRAM_TYPE_0},
			{"program1", FAL_TUNNEL_PROGRAM_TYPE_1},
			{"program2", FAL_TUNNEL_PROGRAM_TYPE_2},
			{"program3", FAL_TUNNEL_PROGRAM_TYPE_3},
			{"program4", FAL_TUNNEL_PROGRAM_TYPE_4},
			{"program5", FAL_TUNNEL_PROGRAM_TYPE_5},
			{"0", FAL_TUNNEL_PROGRAM_TYPE_0},
			{"1", FAL_TUNNEL_PROGRAM_TYPE_1},
			{"2", FAL_TUNNEL_PROGRAM_TYPE_2},
			{"3", FAL_TUNNEL_PROGRAM_TYPE_3},
			{"4", FAL_TUNNEL_PROGRAM_TYPE_4},
			{"5", FAL_TUNNEL_PROGRAM_TYPE_5},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#endif
#ifdef IN_TUNNEL_PROGRAM
	{
		"tunnel_program_pos_mode",
		{
			{"end", 0},
			{"start", 1},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"tunnel_program_inner_type_mode",
		{
			{"fix", 0},
			{"udf", 1},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"tunnel_program_opt_len_unit",
		{
			{"1byte", 0},
			{"2bytes", 1},
			{"4bytes", 2},
			{"8bytes", 3},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#endif
#ifdef IN_IP
	{
		"udp_zero_csum_action",
		{
			{"forward", FAL_UDP_ZERO_CSUM_FRWRD},
			{"drop", FAL_UDP_ZERO_CSUM_DROP},
			{"recalc_mapt", FAL_UDP_ZERO_CSUM_RECALC_MAPT},
			{"rdtcpu", FAL_UDP_ZERO_CSUM_RDT_TO_CPU},
			{NULL, FAL_UDP_ZERO_CSUM_BUTT}
		}
	},
#endif
#ifdef IN_TUNNEL
	{
		"tunnel_overlay_type",
		{
			{"gre-tap", FAL_TUNNEL_OVERLAY_TYPE_GRE_TAP},
			{"vxlan", FAL_TUNNEL_OVERLAY_TYPE_VXLAN},
			{"vxlan-gpe", FAL_TUNNEL_OVERLAY_TYPE_VXLAN_GPE},
			{"geneve", FAL_TUNNEL_OVERLAY_TYPE_GENEVE},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"tunnel_udf_type",
		{
			{"l2", FAL_TUNNEL_UDF_TYPE_L2},
			{"l3", FAL_TUNNEL_UDF_TYPE_L3},
			{"l4", FAL_TUNNEL_UDF_TYPE_L4},
			{"overlay", FAL_TUNNEL_UDF_TYPE_OVERLAY},
			{"program", FAL_TUNNEL_UDF_TYPE_PROGRAM},
			{"payload", FAL_TUNNEL_UDF_TYPE_PAYLOAD},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
	{
		"src1_sel",
		{
			{"zero_data", FAL_TUNNEL_RULE_SRC1_ZERO_DATA},
			{"header_data", FAL_TUNNEL_RULE_SRC1_FROM_HEADER_DATA},
			{NULL, FAL_TUNNEL_RULE_SRC1_DATA_INVALID}
		}
	},
	{
		"src2_sel",
		{
			{"zero_data", FAL_TUNNEL_RULE_SRC2_ZERO_DATA},
			{"pkt_data", FAL_TUNNEL_RULE_SRC2_PKT_DATA0},
			{"napt_addr", FAL_TUNNEL_RULE_SRC2_NAPT_ADDR},
			{"tunnel_vni", FAL_TUNNEL_RULE_SRC2_TUNNEL_VNI},
			{"proto_map0", FAL_TUNNEL_RULE_SRC2_PROTO_MAP0},
			{"proto_map1", FAL_TUNNEL_RULE_SRC2_PROTO_MAP1},
			{NULL, FAL_TUNNEL_RULE_SRC2_DATA_INVALID}
		}
	},
	{
		"src3_sel",
		{
			{"zero_data", FAL_TUNNEL_RULE_SRC3_ZERO_DATA},
			{"pkt_data", FAL_TUNNEL_RULE_SRC3_PKT_DATA1},
			{"napt_port", FAL_TUNNEL_RULE_SRC3_NAPT_PORT},
			{"policy_id", FAL_TUNNEL_RULE_SRC3_POLICY_ID},
			{"hash_value", FAL_TUNNEL_RULE_SRC3_HASH_VALUE},
			{"proto_map0", FAL_TUNNEL_RULE_SRC3_PROTO_MAP0},
			{"proto_map1", FAL_TUNNEL_RULE_SRC3_PROTO_MAP1},
			{NULL, FAL_TUNNEL_RULE_SRC3_DATA_INVALID}
		}
	},
	{
		"encap_target",
		{
			{"sip", FAL_TUNNEL_ENCAP_TARGET_SIP},
			{"dip", FAL_TUNNEL_ENCAP_TARGET_DIP},
			{"tunnel", FAL_TUNNEL_ENCAP_TARGET_TUNNEL_INFO},
			{"none", FAL_TUNNEL_ENCAP_TARGET_NO_UPDATE},
			{NULL, FAL_TUNNEL_ENCAP_TARGET_MAX}
		}
	},
	{
		"payload_inner_type",
		{
			{"ethernet", FAL_TUNNEL_INNER_ETHERNET},
			{"ip", FAL_TUNNEL_INNER_IP},
			{"transport", FAL_TUNNEL_INNER_TRANSPORT},
			{NULL, FAL_TUNNEL_INNER_RESERVED}
		}
	},
	{
		"ecn_val",
		{
			{"no_ect", FAL_TUNNEL_ECN_NOT_ECT},
			{"ect_0", FAL_TUNNEL_ECN_ECT_0},
			{"ect_1", FAL_TUNNEL_ECN_ECT_1},
			{"ce", FAL_TUNNEL_ECN_CE},
			{NULL, FAL_TUNNEL_ECN_INVALID}
		}
	},
#endif
	{
		"flow_excep_type",
		{
			{"flow_aware", FAL_FLOW_AWARE},
			{"flow_hit", FAL_FLOW_HIT},
			{"flow_miss", FAL_FLOW_MISS},
			{NULL, INVALID_ARRT_VALUE}
		}
	},
#ifdef IN_MAPT
	{
		"addr_type",
		{
			{"zero_data", FAL_TUNNEL_MAPT_ZERO_DATA},
			{"src_ip", FAL_TUNNEL_MAPT_FROM_SRC},
			{"dst_ip", FAL_TUNNEL_MAPT_FROM_DST},
			{NULL, FAL_TUNNEL_MAPT_INVALID}
		}
	},
	{
		"proto_type",
		{
			{"zero_data", FAL_TUNNEL_MAPT_ZERO_DATA},
			{"src_port", FAL_TUNNEL_MAPT_FROM_SRC},
			{"dst_port", FAL_TUNNEL_MAPT_FROM_DST},
			{NULL, FAL_TUNNEL_MAPT_INVALID}
		}
	},
#endif
#ifdef IN_VPORT
	{
		"vport_type",
		{
			{"tunnel", FAL_VPORT_TYPE_TUNNEL},
			{"normal", FAL_VPORT_TYPE_NORMAL},
			{NULL, FAL_VPORT_TYPE_BUTT}
		}
	},
#endif
	{
		"cnt_mode",
		{
			{"ip_pkt", FAL_PORT_CNT_MODE_IP_PKT},
			{"full_pkt", FAL_PORT_CNT_MODE_FULL_PKT},
			{NULL, FAL_PORT_CNT_MODE_BUTT}
		}
	},

	{NULL, {{NULL, INVALID_ARRT_VALUE}}}
};

sw_error_t
cmd_data_check_attr(char * attr_name, char *cmd_str, a_uint32_t *arg_val, a_uint32_t size)
{
	a_uint32_t i, j;

	if (NULL == cmd_str)
	{
		return SW_BAD_VALUE;
	}

	for (i = 0; g_attr_des[i].attr_name != NULL; i++)
	{
		if (!strcasecmp(attr_name, g_attr_des[i].attr_name))
		{ /* find attr */
			break;
		}
	}
	if (g_attr_des[i].attr_name == NULL)
	{
		return SW_BAD_VALUE;
	}
	for (j = 0; g_attr_des[i].sub_attr_des[j].sub_attr_name != NULL; j++)
	{
		if (!strcasecmp(cmd_str, g_attr_des[i].sub_attr_des[j].sub_attr_name))
		{ /* find sub attr */
			*arg_val = g_attr_des[i].sub_attr_des[j].value;
			return SW_OK;
		}
	}
	return SW_BAD_VALUE;
}

int
get_talk_mode(void)
{
    return talk_mode ;
}

void
set_talk_mode(int mode)
{
    talk_mode = mode;
}

char ** full_cmdstrp_bak;

void
set_full_cmdstrp(char **cmdstrp)
{
    full_cmdstrp = cmdstrp;
    full_cmdstrp_bak = cmdstrp;
}

int
get_jump(void)
{
    return (full_cmdstrp-full_cmdstrp_bak);
}

static char *
get_cmd_buf(char *tag, char *defval)
{
    if(!full_cmdstrp || !(*full_cmdstrp))
    {
        return NULL;
    }

    if (!strcasecmp(*(full_cmdstrp), DEFAULT_FLAG))
    {
        full_cmdstrp++;
        return defval;
    }
    else
    {
        return *(full_cmdstrp++);
    }
}

static char *
get_sub_cmd(char *tag, char *defval)
{
    return get_cmd_buf(tag, defval);
}


static inline  a_bool_t
is_hex(char c)
{
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
            || (c >= 'A' && c <= 'F'))
        return A_TRUE;

    return A_FALSE;
}

static inline a_bool_t
is_dec(char c)
{
    if ((c >= '0') && (c <= '9'))
        return A_TRUE;

    return A_FALSE;
}

static sw_data_type_t sw_data_type[] =
{
    SW_TYPE_DEF(SW_UINT8, (param_check_t)cmd_data_check_uint8, NULL),
    SW_TYPE_DEF(SW_INT8, NULL, NULL),
    SW_TYPE_DEF(SW_UINT16, (param_check_t)cmd_data_check_uint16, NULL),
    SW_TYPE_DEF(SW_INT16, NULL, NULL),
    SW_TYPE_DEF(SW_UINT32, cmd_data_check_uint32, NULL),
    SW_TYPE_DEF(SW_INT32, NULL, NULL),
    SW_TYPE_DEF(SW_UINT64, cmd_data_check_uint64, NULL),
    SW_TYPE_DEF(SW_INT64, NULL, NULL),
#ifdef IN_PORTCONTROL
    SW_TYPE_DEF(SW_DUPLEX, cmd_data_check_duplex, NULL),
    SW_TYPE_DEF(SW_SPEED, cmd_data_check_speed, NULL),
    SW_TYPE_DEF(SW_CAP, cmd_data_check_capable, NULL),
#ifndef IN_PORTCONTROL_MINI
#if defined (APPE)
    SW_TYPE_DEF(SW_PORT_8023AH_CTRL, (param_check_t)cmd_data_check_port_8023ah_ctrl, NULL),
#endif
    SW_TYPE_DEF(SW_PORT_EEE_CONFIG, (param_check_t)cmd_data_check_port_eee_config, NULL),
    SW_TYPE_DEF(SW_PORT_LOOPBACK_CONFIG, (param_check_t)cmd_data_check_switch_port_loopback_config, NULL),
#endif
	SW_TYPE_DEF(SW_PORT_CNT_CFG, (param_check_t)cmd_data_check_port_cnt_cfg, NULL),
#endif
#ifdef IN_PORTVLAN
    SW_TYPE_DEF(SW_1QMODE, cmd_data_check_1qmode, NULL),
    SW_TYPE_DEF(SW_EGMODE, cmd_data_check_egmode, NULL),
#endif
#ifdef IN_MIB
    SW_TYPE_DEF(SW_MIB, NULL, NULL),
    SW_TYPE_DEF(SW_MIB_CNTR, NULL, NULL),
    SW_TYPE_DEF(SW_XGMIB, NULL, NULL),
#endif
#ifdef IN_VLAN
    SW_TYPE_DEF(SW_VLAN, (param_check_t)cmd_data_check_vlan, NULL),
#endif
    SW_TYPE_DEF(SW_PBMP, cmd_data_check_pbmp, NULL),
    SW_TYPE_DEF(SW_ENABLE, cmd_data_check_enable, NULL),
    SW_TYPE_DEF(SW_MACADDR, (param_check_t)cmd_data_check_macaddr, NULL),
#ifdef IN_FDB
    SW_TYPE_DEF(SW_FDBENTRY, (param_check_t)cmd_data_check_fdbentry, NULL),
#ifndef IN_FDB_MINI
    SW_TYPE_DEF(SW_MACLIMIT_CTRL, (param_check_t)cmd_data_check_maclimit_ctrl, NULL),
#endif
#endif
#ifdef IN_QOS
#ifndef IN_QOS_MINI
    SW_TYPE_DEF(SW_SCH, cmd_data_check_qos_sch, NULL),
    SW_TYPE_DEF(SW_QOS, cmd_data_check_qos_pt, NULL),
#endif
#ifndef IN_QOS_MINI
    SW_TYPE_DEF(SW_PORTGROUP, (param_check_t)cmd_data_check_port_group, NULL),
    SW_TYPE_DEF(SW_PORTPRI, (param_check_t)cmd_data_check_port_pri, NULL),
    SW_TYPE_DEF(SW_PORTREMARK, (param_check_t)cmd_data_check_port_remark, NULL),
    SW_TYPE_DEF(SW_COSMAP, (param_check_t)cmd_data_check_cosmap, NULL),
    SW_TYPE_DEF(SW_SCHEDULER, (param_check_t)cmd_data_check_queue_scheduler, NULL),
    SW_TYPE_DEF(SW_QUEUEBMP, (param_check_t)cmd_data_check_ring_queue, NULL),
#endif
#endif
#ifdef IN_RATE
	SW_TYPE_DEF(SW_STORM, cmd_data_check_storm, NULL),
#endif
#ifdef IN_STP
    SW_TYPE_DEF(SW_STP, cmd_data_check_stp_state, NULL),
#endif
#ifdef IN_LEAKY
    SW_TYPE_DEF(SW_LEAKY, cmd_data_check_leaky, NULL),
#endif
    SW_TYPE_DEF(SW_MACCMD, cmd_data_check_maccmd, NULL),
#ifdef IN_IP
#if !defined(IN_IP_MINI)
    SW_TYPE_DEF(SW_FLOWCMD, cmd_data_check_flowcmd, NULL),
    SW_TYPE_DEF(SW_FLOWTYPE, cmd_data_check_flowtype, NULL),
#endif
#endif
    SW_TYPE_DEF(SW_UINT_A, cmd_data_check_uinta, NULL),
#ifdef IN_ACL
    SW_TYPE_DEF(SW_ACLRULE, NULL, NULL),
    SW_TYPE_DEF(SW_ACL_UDF_PKT_TYPE, cmd_data_check_udf_pkt_type, NULL),
    SW_TYPE_DEF(SW_ACL_UDF_TYPE, cmd_data_check_udf_type, NULL),
#ifdef APPE
    SW_TYPE_DEF(SW_VPORT_TYPE, cmd_data_check_vport_type, NULL),
#endif
#endif
#ifdef IN_LED
    SW_TYPE_DEF(SW_LEDPATTERN, (param_check_t)cmd_data_check_ledpattern, NULL),
#endif
#ifdef IN_PORTVLAN
    SW_TYPE_DEF(SW_INVLAN, cmd_data_check_invlan_mode, NULL),
#ifdef HPPE
    SW_TYPE_DEF(SW_GLOBAL_QINQMODE, (param_check_t)cmd_data_check_global_qinqmode, NULL),
    SW_TYPE_DEF(SW_PT_QINQMODE, (param_check_t)cmd_data_check_port_qinqmode, NULL),
    SW_TYPE_DEF(SW_TPID, (param_check_t)cmd_data_check_tpid, NULL),
    SW_TYPE_DEF(SW_INGRESS_FILTER, (param_check_t)cmd_data_check_ingress_filter, NULL),
    SW_TYPE_DEF(SW_PT_VLAN_DIRECTION, cmd_data_check_port_vlan_direction, NULL),
    SW_TYPE_DEF(SW_PT_DEF_VID_EN, (param_check_t)cmd_data_check_port_default_vid_en, NULL),
    SW_TYPE_DEF(SW_PT_VLAN_TAG, (param_check_t)cmd_data_check_port_vlan_tag, NULL),
    SW_TYPE_DEF(SW_TAG_PROPAGATION, (param_check_t)cmd_data_check_tag_propagation, NULL),
    SW_TYPE_DEF(SW_EGRESS_MODE, (param_check_t)cmd_data_check_egress_mode, NULL),
    SW_TYPE_DEF(SW_PT_VLAN_TRANS_ADV_RULE,
		    (param_check_t)cmd_data_check_port_vlan_translation_adv_rule, NULL),
    SW_TYPE_DEF(SW_PT_VLAN_TRANS_ADV_ACTION,
		    (param_check_t)cmd_data_check_port_vlan_translation_adv_action, NULL),
#endif
#ifdef APPE
	SW_TYPE_DEF(SW_ISOL_CTRL, cmd_data_check_isol_ctrl, NULL),
#endif
#ifndef IN_PORTVLAN_MINI
    SW_TYPE_DEF(SW_VLANPROPAGATION, cmd_data_check_vlan_propagation, NULL),
    SW_TYPE_DEF(SW_VLANTRANSLATION, (param_check_t)cmd_data_check_vlan_translation, NULL),
    SW_TYPE_DEF(SW_QINQMODE, cmd_data_check_qinq_mode, NULL),
    SW_TYPE_DEF(SW_QINQROLE, cmd_data_check_qinq_role, NULL),
#endif
#endif
    SW_TYPE_DEF(SW_CABLESTATUS, NULL, NULL),
    SW_TYPE_DEF(SW_CABLELEN, NULL, NULL),
    SW_TYPE_DEF(SW_SSDK_CFG, NULL, NULL),
    SW_TYPE_DEF(SW_MODULE, (param_check_t)cmd_data_check_module, NULL),
    SW_TYPE_DEF(SW_FUNC_CTRL, (param_check_t)cmd_data_check_func_ctrl, NULL),
#ifdef IN_PORTCONTROL
    SW_TYPE_DEF(SW_HDRMODE, cmd_data_check_hdrmode, NULL),
#endif
#ifdef IN_FDB
    SW_TYPE_DEF(SW_FDBOPRATION, (param_check_t)cmd_data_check_fdboperation, NULL),
#endif
#ifdef IN_PPPOE
    SW_TYPE_DEF(SW_PPPOE, (param_check_t)cmd_data_check_pppoe, NULL),
    SW_TYPE_DEF(SW_PPPOE_LESS, (param_check_t)cmd_data_check_pppoe_less, NULL),
#endif
#if defined(IN_IP) || defined(IN_NAT)
    SW_TYPE_DEF(SW_IP_HOSTENTRY, (param_check_t)cmd_data_check_host_entry, NULL),
#if !defined(IN_IP_MINI)
    SW_TYPE_DEF(SW_ARP_LEARNMODE, cmd_data_check_arp_learn_mode, NULL),
    SW_TYPE_DEF(SW_IP_GUARDMODE, cmd_data_check_ip_guard_mode, NULL),
    SW_TYPE_DEF(SW_NATENTRY, (param_check_t)cmd_data_check_nat_entry, NULL),
    SW_TYPE_DEF(SW_NAPTENTRY, (param_check_t)cmd_data_check_napt_entry, NULL),
    SW_TYPE_DEF(SW_FLOWENTRY, (param_check_t)cmd_data_check_flow_entry, NULL),
    SW_TYPE_DEF(SW_FLOWCOOKIE, (param_check_t)cmd_data_check_flow_cookie, NULL),
    SW_TYPE_DEF(SW_FLOWRFS, (param_check_t)cmd_data_check_flow_rfs, NULL),
    SW_TYPE_DEF(SW_NAPTMODE, cmd_data_check_napt_mode, NULL),
    SW_TYPE_DEF(SW_INTFMACENTRY, (param_check_t)cmd_data_check_intf_mac_entry, NULL),
    SW_TYPE_DEF(SW_PUBADDRENTRY, (param_check_t)cmd_data_check_pub_addr_entry, NULL),
#endif
    SW_TYPE_DEF(SW_IP4ADDR, (param_check_t)cmd_data_check_ip4addr, NULL),
    SW_TYPE_DEF(SW_IP6ADDR, (param_check_t)cmd_data_check_ip6addr, NULL),
#endif
#ifdef IN_RATE
    SW_TYPE_DEF(SW_INGPOLICER, (param_check_t)cmd_data_check_port_policer, NULL),
    SW_TYPE_DEF(SW_EGSHAPER, (param_check_t)cmd_data_check_egress_shaper, NULL),
    SW_TYPE_DEF(SW_ACLPOLICER, (param_check_t)cmd_data_check_acl_policer, NULL),
#endif
    SW_TYPE_DEF(SW_MACCONFIG, NULL, NULL),
    SW_TYPE_DEF(SW_PHYCONFIG, NULL, NULL),
#ifdef IN_FDB
#ifndef IN_FDB_MINI
    SW_TYPE_DEF(SW_FDBSMODE, cmd_data_check_fdb_smode, NULL),
#endif
#endif
    SW_TYPE_DEF(SW_FX100CONFIG, NULL, NULL),
#ifdef IN_IGMP
    SW_TYPE_DEF(SW_SGENTRY, (param_check_t)cmd_data_check_multi, NULL),
#endif
#ifdef IN_SEC
    SW_TYPE_DEF(SW_SEC_MAC, cmd_data_check_sec_mac, NULL),
    SW_TYPE_DEF(SW_SEC_IP, cmd_data_check_sec_ip, NULL),
    SW_TYPE_DEF(SW_SEC_IP4, cmd_data_check_sec_ip4, NULL),
    SW_TYPE_DEF(SW_SEC_IP6, cmd_data_check_sec_ip6, NULL),
    SW_TYPE_DEF(SW_SEC_TCP, cmd_data_check_sec_tcp, NULL),
    SW_TYPE_DEF(SW_SEC_UDP, cmd_data_check_sec_udp, NULL),
    SW_TYPE_DEF(SW_SEC_ICMP4, cmd_data_check_sec_icmp4, NULL),
    SW_TYPE_DEF(SW_SEC_ICMP6, cmd_data_check_sec_icmp6, NULL),
#ifdef HPPE
    SW_TYPE_DEF(SW_L3_PARSER, (param_check_t)cmd_data_check_l3_parser, NULL),
    SW_TYPE_DEF(SW_L4_PARSER, (param_check_t)cmd_data_check_l4_parser, NULL),
    SW_TYPE_DEF(SW_EXP_CTRL, (param_check_t)cmd_data_check_exp_ctrl, NULL),
#endif
    SW_TYPE_DEF(SW_L2_EXP_CTRL, (param_check_t)cmd_data_check_l2_exp_ctrl, NULL),
    SW_TYPE_DEF(SW_TUNNEL_EXP_CTRL, (param_check_t)cmd_data_check_tunnel_exp_ctrl, NULL),
    SW_TYPE_DEF(SW_TUNNEL_FLAGS_PARSER, (param_check_t)cmd_data_check_tunnel_flags_parser, NULL),
#endif
#ifdef IN_COSMAP
#ifndef IN_COSMAP_MINI
    SW_TYPE_DEF(SW_REMARKENTRY, (param_check_t)cmd_data_check_remark_entry, NULL),
#endif
#endif
#ifdef IN_IP
#if !defined(IN_IP_MINI)
    SW_TYPE_DEF(SW_DEFAULT_ROUTE_ENTRY, (param_check_t)cmd_data_check_default_route_entry, NULL),
    SW_TYPE_DEF(SW_IP_RFS_IP4, (param_check_t)cmd_data_check_ip4_rfs_entry, NULL),
    SW_TYPE_DEF(SW_IP_RFS_IP6, (param_check_t)cmd_data_check_ip6_rfs_entry, NULL),
    SW_TYPE_DEF(SW_HOST_ROUTE_ENTRY, (param_check_t)cmd_data_check_host_route_entry, NULL),
    SW_TYPE_DEF(SW_IP_NETWORK_ROUTE, (param_check_t)cmd_data_check_network_route, NULL),
#endif
    SW_TYPE_DEF(SW_ARP_SG_CFG, (param_check_t)cmd_data_check_arp_sg, NULL),
    SW_TYPE_DEF(SW_IP_INTF, (param_check_t)cmd_data_check_intf, NULL),
    SW_TYPE_DEF(SW_IP_VSI_INTF, (param_check_t)cmd_data_check_vsi_intf, NULL),
    SW_TYPE_DEF(SW_IP_NEXTHOP, (param_check_t)cmd_data_check_nexthop, NULL),
    SW_TYPE_DEF(SW_IP_SG, (param_check_t)cmd_data_check_ip_sg, NULL),
    SW_TYPE_DEF(SW_IP_PUB, (param_check_t)cmd_data_check_ip_pub, NULL),
    SW_TYPE_DEF(SW_IP_PORTMAC, (param_check_t)cmd_data_check_ip_portmac, NULL),
    SW_TYPE_DEF(SW_IP_MCMODE, (param_check_t)cmd_data_check_ip_mcmode, NULL),
    SW_TYPE_DEF(SW_IP_GLOBAL, (param_check_t)cmd_data_check_ip_global, NULL),
#endif
#ifdef IN_PORTCONTROL
#ifndef IN_PORTCONTROL_MINI
    SW_TYPE_DEF(SW_CROSSOVER_MODE, cmd_data_check_crossover_mode, NULL),
    SW_TYPE_DEF(SW_CROSSOVER_STATUS, cmd_data_check_crossover_status, NULL),
    SW_TYPE_DEF(SW_PREFER_MEDIUM, cmd_data_check_prefer_medium, NULL),
    SW_TYPE_DEF(SW_FIBER_MODE, cmd_data_check_fiber_mode, NULL),
    SW_TYPE_DEF(SW_SRC_FILTER_CONFIG, cmd_data_check_src_filter_config, NULL),
    SW_TYPE_DEF(SW_MTU_ENTRY, (param_check_t)cmd_data_check_mtu_entry, NULL),
    SW_TYPE_DEF(SW_MRU_ENTRY, (param_check_t)cmd_data_check_mru_entry, NULL),
#ifdef APPE
    SW_TYPE_DEF(SW_MTU_CFG, (param_check_t)cmd_data_check_mtu_cfg, NULL),
#endif
#endif
#endif
#ifdef IN_INTERFACECONTROL
    SW_TYPE_DEF(SW_INTERFACE_MODE, cmd_data_check_interface_mode, NULL),
#endif
    SW_TYPE_DEF(SW_COUNTER_INFO, NULL, NULL),
#ifdef IN_VSI
    SW_TYPE_DEF(SW_VSI_NEWADDR_LRN, (param_check_t)cmd_data_check_newadr_lrn, NULL),
    SW_TYPE_DEF(SW_VSI_STAMOVE, (param_check_t)cmd_data_check_stamove, NULL),
    SW_TYPE_DEF(SW_VSI_MEMBER, (param_check_t)cmd_data_check_vsi_member, NULL),
#if defined (APPE)
    SW_TYPE_DEF(SW_VSI_BRIDGE_VSI,(param_check_t)cmd_data_check_vsi_bridge_vsi, NULL),
    SW_TYPE_DEF(SW_VSI_INVALIDVSI_CTRL,(param_check_t)cmd_data_check_vsi_invalidvsi_ctrl, NULL),
#endif
    SW_TYPE_DEF(SW_VSI_COUNTER, NULL, NULL),
#endif
#ifdef IN_QM
    SW_TYPE_DEF(SW_STATIC_THRESH, (param_check_t)cmd_data_check_ac_static_thresh, NULL),
    SW_TYPE_DEF(SW_DYNAMIC_THRESH, (param_check_t)cmd_data_check_ac_dynamic_thresh, NULL),
    SW_TYPE_DEF(SW_GROUP_BUFFER, (param_check_t)cmd_data_check_ac_group_buff, NULL),
    SW_TYPE_DEF(SW_AC_CTRL, (param_check_t)cmd_data_check_ac_ctrl, NULL),
    SW_TYPE_DEF(SW_AC_OBJ, (param_check_t)cmd_data_check_ac_obj, NULL),
    SW_TYPE_DEF(SW_UCAST_QUEUE_MAP, (param_check_t)cmd_data_check_u_qmap, NULL),
#endif
#ifdef IN_BM
    SW_TYPE_DEF(SW_BMSTHRESH, (param_check_t)cmd_data_check_bm_static_thresh, NULL),
    SW_TYPE_DEF(SW_BMDTHRESH, (param_check_t)cmd_data_check_bm_dynamic_thresh, NULL),
#endif
#ifdef IN_FLOW
    SW_TYPE_DEF(SW_FLOW_AGE, (param_check_t)cmd_data_check_flow_age, NULL),
    SW_TYPE_DEF(SW_FLOW_CTRL, (param_check_t)cmd_data_check_flow_ctrl, NULL),
    SW_TYPE_DEF(SW_FLOW_ENTRY, (param_check_t)cmd_data_check_flow, NULL),
    SW_TYPE_DEF(SW_FLOW_GLOBAL, (param_check_t)cmd_data_check_flow_global, NULL),
    SW_TYPE_DEF(SW_FLOW_HOST, (param_check_t)cmd_data_check_flow_host, NULL),
#endif
#ifdef IN_PORTCONTROL
#ifndef IN_PORTCONTROL_MINI
    SW_TYPE_DEF(SW_MTU_INFO, NULL, NULL),
    SW_TYPE_DEF(SW_MRU_INFO, NULL, NULL),
    SW_TYPE_DEF(SW_MTU_ENTRY, (param_check_t)cmd_data_check_mtu_entry, NULL),
    SW_TYPE_DEF(SW_MRU_ENTRY, (param_check_t)cmd_data_check_mru_entry, NULL),
#endif
#endif
#ifdef IN_SHAPER
    SW_TYPE_DEF(SW_PORT_SHAPER_TOKEN_CONFIG,
		    (param_check_t)cmd_data_check_port_shaper_token_config, NULL),
    SW_TYPE_DEF(SW_SHAPER_TOKEN_CONFIG, (param_check_t)cmd_data_check_shaper_token_config, NULL),
    SW_TYPE_DEF(SW_PORT_SHAPER_CONFIG, (param_check_t)cmd_data_check_port_shaper_config, NULL),
    SW_TYPE_DEF(SW_SHAPER_CONFIG, (param_check_t)cmd_data_check_shaper_config, NULL),
#if defined(APPE)
    SW_TYPE_DEF(SW_QUEUE_SHAPER_CTRL, (param_check_t)cmd_data_check_queue_shaper_ctrl, NULL),
    SW_TYPE_DEF(SW_FLOW_SHAPER_CTRL, (param_check_t)cmd_data_check_flow_shaper_ctrl, NULL),
#endif
#endif

#ifdef IN_POLICER
    SW_TYPE_DEF(SW_POLICER_ACL_CONFIG, (param_check_t)cmd_data_check_acl_policer_config, NULL),
    SW_TYPE_DEF(SW_POLICER_PORT_CONFIG, (param_check_t)cmd_data_check_port_policer_config, NULL),
    SW_TYPE_DEF(SW_POLICER_CMD_CONFIG, (param_check_t)cmd_data_check_policer_cmd_config, NULL),
#if defined(APPE)
    SW_TYPE_DEF(SW_POLICER_REMAP, (param_check_t)cmd_data_check_policer_remap, NULL),
    SW_TYPE_DEF(SW_POLICER_PRIORITY, (param_check_t)cmd_data_check_policer_priority, NULL),
    SW_TYPE_DEF(SW_POLICER_CTRL, (param_check_t)cmd_data_check_policer_ctrl, NULL),
#endif
#endif
#ifdef IN_SERVCODE
    SW_TYPE_DEF(SW_SERVCODE_CONFIG, (param_check_t)cmd_data_check_servcode_config, NULL),
#endif
#ifdef IN_RSS_HASH
    SW_TYPE_DEF(SW_RSS_HASH_MODE, (param_check_t)cmd_data_check_rss_hash_mode, NULL),
    SW_TYPE_DEF(SW_RSS_HASH_CONFIG, (param_check_t)cmd_data_check_rss_hash_config, NULL),
#endif
#ifdef IN_MIRROR
    SW_TYPE_DEF(SW_MIRR_DIRECTION, cmd_data_check_mirr_direction, NULL),
    SW_TYPE_DEF(SW_MIRR_ANALYSIS_CONFIG, (param_check_t)cmd_data_check_mirr_analy_cfg, NULL),
#endif
#ifdef IN_CTRLPKT
    SW_TYPE_DEF(SW_CTRLPKT_PROFILE, (param_check_t)cmd_data_check_ctrlpkt_appprofile, NULL),
#endif
#ifdef IN_TUNNEL
    SW_TYPE_DEF(SW_TUNNEL_UDP_ENTRY, (param_check_t)cmd_data_check_tunnel_udp_entry, NULL),
    SW_TYPE_DEF(SW_TUNNEL_UDF_TYPE, (param_check_t)cmd_data_check_tunnel_udf_type, NULL),
#endif
#ifdef IN_VXLAN
    SW_TYPE_DEF(SW_VXLAN_TYPE, (param_check_t)cmd_data_check_vxlan_type, NULL),
    SW_TYPE_DEF(SW_VXLAN_GPE_PROTO, (param_check_t)cmd_data_check_vxlan_gpe_proto, NULL),
#endif
#ifdef IN_TUNNEL_PROGRAM
    SW_TYPE_DEF(SW_TUNNEL_PROGRAM_TYPE, (param_check_t)cmd_data_check_tunnel_program_type, NULL),
    SW_TYPE_DEF(SW_TUNNEL_PROGRAM_ENTRY, (param_check_t)cmd_data_check_tunnel_program_entry, NULL),
    SW_TYPE_DEF(SW_TUNNEL_PROGRAM_CFG, (param_check_t)cmd_data_check_tunnel_program_cfg, NULL),
#endif
#ifdef IN_TUNNEL
    SW_TYPE_DEF(SW_TUNNEL_UDF_TYPE,
		    (param_check_t)cmd_data_check_tunnel_udf_type, NULL),
    SW_TYPE_DEF(SW_TUNNEL_INTF,
		    (param_check_t)cmd_data_check_tunnel_intf, NULL),
    SW_TYPE_DEF(SW_TUNNEL_PORT_INTF,
		    (param_check_t)cmd_data_check_tunnel_port_intf, NULL),
    SW_TYPE_DEF(SW_TUNNEL_ENCAP_RULE_ENTRY,
		    (param_check_t)cmd_data_check_tunnel_encap_rule_entry, NULL),
    SW_TYPE_DEF(SW_TUNNEL_TUNNEL_ID,
		    (param_check_t)cmd_data_check_tunnel_encap_tunnelid, NULL),
    SW_TYPE_DEF(SW_TUNNEL_VLAN_INTF,
		    (param_check_t)cmd_data_check_tunnel_vlan_intf, NULL),
    SW_TYPE_DEF(SW_TUNNEL_DECAP_ENTRY,
		    (param_check_t)cmd_data_check_tunnel_decap_entry, NULL),
    SW_TYPE_DEF(SW_TUNNEL_ENCAP_ENTRY,
		    (param_check_t)cmd_data_check_tunnel_encap_entry, NULL),
    SW_TYPE_DEF(SW_TUNNEL_GLOBAL_CFG,
		    (param_check_t)cmd_data_check_tunnel_global_cfg, NULL),
    SW_TYPE_DEF(SW_TUNNEL_ENCAP_HEADER_CTRL,
		    (param_check_t)cmd_data_check_tunnel_encap_header_ctrl, NULL),
    SW_TYPE_DEF(SW_TUNNEL_DECAP_ECN_RULE,
		    (param_check_t)cmd_data_check_decap_ecn_rule, NULL),
    SW_TYPE_DEF(SW_TUNNEL_DECAP_ECN_ACTION,
		    (param_check_t)cmd_data_check_decap_ecn_action, NULL),
    SW_TYPE_DEF(SW_TUNNEL_ENCAP_ECN_RULE,
		    (param_check_t)cmd_data_check_encap_ecn_rule, NULL),
    SW_TYPE_DEF(SW_TUNNEL_ECN_VAL,
		    (param_check_t)cmd_data_check_ecn_val, NULL),
    SW_TYPE_DEF(SW_TUNNEL_TYPE,
		    cmd_data_check_tunnel_type, NULL),
    SW_TYPE_DEF(SW_TUNNEL_KEY,
		    (param_check_t)cmd_data_check_tunnel_key, NULL),
#endif
#ifdef IN_MAPT
    SW_TYPE_DEF(SW_MAPT_DECAP_CTRL,
		    (param_check_t)cmd_data_check_mapt_decap_ctrl, NULL),
    SW_TYPE_DEF(SW_MAPT_DECAP_RULE_ENTRY,
		    (param_check_t)cmd_data_check_mapt_decap_rule_entry, NULL),
    SW_TYPE_DEF(SW_MAPT_DECAP_ENTRY,
		    (param_check_t)cmd_data_check_mapt_decap_entry, NULL),
#endif
#ifdef IN_VPORT
    SW_TYPE_DEF(SW_VPORT_STATE, (param_check_t)cmd_data_check_vport_state, NULL),
#endif
};

sw_data_type_t *
cmd_data_type_find(sw_data_type_e type)
{
    a_uint16_t i = 0;

    do
    {
        if (type == sw_data_type[i].data_type)
            return &sw_data_type[i];
    }
    while ( ++i < sizeof(sw_data_type)/sizeof(sw_data_type[0]));

    return NULL;
}

sw_error_t __cmd_data_check_quit_help(char *cmd, char *usage)
{
    sw_error_t ret = SW_OK;

    if (!strncasecmp(cmd, "quit", 4)) {
        return SW_ABORTED;
    } else if (!strncasecmp(cmd, "help", 4)) {
        ret = SW_BAD_VALUE;
    }

    return ret;
}

sw_error_t __cmd_data_check_complex(char *info, char *defval, char *usage,
				param_check_t chk_func, void *arg_val,
				a_uint32_t size)
{
    sw_error_t ret;
    char *cmd;

    do {
        cmd = get_sub_cmd(info, defval);
        SW_RTN_ON_NULL_PARAM(cmd);

        ret = __cmd_data_check_quit_help(cmd, usage);
        if (ret == SW_ABORTED)
            return ret;
        else if (ret == SW_OK) {
            ret = chk_func(cmd, arg_val, size);
        }
    } while (talk_mode && (SW_OK != ret));

    return SW_OK;
}

sw_error_t __cmd_data_check_range(char *info, char *defval, char *usage,
				param_check_range_t chk_func, void *arg_val,
				a_uint32_t max_val, a_uint32_t min_val)
{
    sw_error_t ret;
    char *cmd;

    do {
        cmd = get_sub_cmd(info, defval);
        SW_RTN_ON_NULL_PARAM(cmd);

        ret = __cmd_data_check_quit_help(cmd, usage);
        if (ret == SW_ABORTED)
            return ret;
        else if (ret == SW_OK) {
            ret = chk_func(cmd, arg_val, max_val, min_val);
        }
    } while (talk_mode && (SW_OK != ret));

    return SW_OK;
}

sw_error_t __cmd_data_check_boolean(char *info, char *defval, char *usage,
				param_check_boolean_t chk_func, a_bool_t def, a_bool_t *val,
				a_uint32_t size)
{
    sw_error_t ret;
    char *cmd;

    do {
        cmd = get_sub_cmd(info, defval);
        SW_RTN_ON_NULL_PARAM(cmd);

        ret = __cmd_data_check_quit_help(cmd, usage);
        if (ret == SW_ABORTED)
            return ret;
        else if (ret == SW_OK) {
            ret = chk_func(cmd, def, val, size);
        }
    } while (talk_mode && (SW_OK != ret));

    return SW_OK;
}

sw_error_t
cmd_data_check_uint8(char *cmd_str, a_uint32_t *arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (0 == cmd_str[0])
    {
        return SW_BAD_VALUE;
    }

    if (cmd_str[0] == '0' && (cmd_str[1] == 'x' || cmd_str[1] == 'X'))
        sscanf(cmd_str, "%x", arg_val);
    else
        sscanf(cmd_str, "%d", arg_val);

    if (255 < *arg_val)
    {
        return SW_BAD_PARAM;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_uint64(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (0 == cmd_str[0])
    {
        return SW_BAD_VALUE;
    }

    if (strspn(cmd_str, "1234567890abcdefABCDEFXx") != strlen(cmd_str)){
        return SW_BAD_VALUE;
    }

    if (cmd_str[0] == '0' && (cmd_str[1] == 'x' || cmd_str[1] == 'X'))
        sscanf(cmd_str, "%llx", (a_uint64_t *)arg_val);
    else
        sscanf(cmd_str, "%lld", (a_uint64_t *)arg_val);

    return SW_OK;
}

sw_error_t
cmd_data_check_uint32(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (0 == cmd_str[0])
    {
        return SW_BAD_VALUE;
    }

    if (strspn(cmd_str, "1234567890abcdefABCDEFXx") != strlen(cmd_str)){
        return SW_BAD_VALUE;
    }

    if (cmd_str[0] == '0' && (cmd_str[1] == 'x' || cmd_str[1] == 'X'))
        sscanf(cmd_str, "%x", arg_val);
    else
        sscanf(cmd_str, "%d", arg_val);

    return SW_OK;
}

sw_error_t
cmd_data_check_uint16(char *cmd_str, a_uint32_t *arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (0 == cmd_str[0])
    {
        return SW_BAD_VALUE;
    }

    if (cmd_str[0] == '0' && (cmd_str[1] == 'x' || cmd_str[1] == 'X'))
        sscanf(cmd_str, "%x", arg_val);
    else
        sscanf(cmd_str, "%d", arg_val);

    if (65535 < *arg_val)
    {
        return SW_BAD_PARAM;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_pbmp(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (cmd_str[0] == '0' && (cmd_str[1] == 'x' || cmd_str[1] == 'X'))
        sscanf(cmd_str, "%x", arg_val);
    else
        sscanf(cmd_str, "%d", arg_val);

    return SW_OK;

}

sw_error_t
cmd_data_check_enable(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "disable"))
        *arg_val = FAL_DISABLE;
    else if (!strcasecmp(cmd_str, "enable"))
        *arg_val = FAL_ENABLE;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#ifdef IN_PORTCONTROL
/*port ctrl*/
sw_error_t
cmd_data_check_duplex(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "half"))
        *arg_val = FAL_HALF_DUPLEX;
    else if (!strcasecmp(cmd_str, "full"))
        *arg_val = FAL_FULL_DUPLEX;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_speed(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strncasecmp(cmd_str, "10", 3))
        *arg_val = FAL_SPEED_10;
    else if (!strncasecmp(cmd_str, "100", 4))
        *arg_val = FAL_SPEED_100;
    else if (!strncasecmp(cmd_str, "1000", 5))
        *arg_val = FAL_SPEED_1000;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_capable(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    cmd_strtol(cmd_str, arg_val);
    if (*arg_val & (~FAL_PHY_COMBO_ADV_ALL))
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#ifndef IN_PORTCONTROL_MINI
sw_error_t
cmd_data_check_port_eee_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_port_eee_cfg_t cfg;

    aos_mem_zero(&cfg, sizeof (fal_port_eee_cfg_t));

    do
    {
        cmd = get_sub_cmd("eee_enable", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(cfg.enable),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
    do
    {
        cmd = get_sub_cmd("eee_capability", "0xffff");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.capability), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("lpi_sleep_timer", "256");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.lpi_sleep_timer), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("advertisement", "0xffff");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.advertisement), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("lpi_tx_enable", "1");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.lpi_tx_enable), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("eee_status", "0xffff");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.eee_status), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("lpi_wakeup_timer", "32");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.lpi_wakeup_timer), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("link_partner_advertisement", "0xffff");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.link_partner_advertisement), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_port_eee_cfg_t *)val = cfg;
    return SW_OK;
}

sw_error_t
cmd_data_check_switch_port_loopback_config(char *cmd_str, void * val,
	a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_loopback_config_t cfg;

    aos_mem_zero(&cfg, sizeof (fal_loopback_config_t));

    do
    {
        cmd = get_sub_cmd("loopback_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(cfg.enable),
                                        sizeof (a_bool_t));
            SW_RTN_ON_ERROR(rv);
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("crc_stripped_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(cfg.crc_stripped),
                                        sizeof (a_bool_t));
            SW_RTN_ON_ERROR(rv);
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("loopback_rate", "1-0x12c");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(cfg.loopback_rate), sizeof (a_uint32_t));
            SW_RTN_ON_ERROR(rv);
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_loopback_config_t *)val = cfg;
    return SW_OK;
}

#if defined (APPE)
sw_error_t
cmd_data_check_port_8023ah_ctrl(char *cmd_str, void * val,
	a_uint32_t size)
{
    char *cmd;
    fal_port_8023ah_ctrl_t port_8023ah_ctrl;

    aos_mem_zero(&port_8023ah_ctrl, sizeof (fal_port_8023ah_ctrl_t));

    cmd_data_check_element("loopback_en", "disable",
                        "usage:loopback_en,enable/disable\n",
                        cmd_data_check_enable, (cmd, &(port_8023ah_ctrl.loopback_enable),
                        sizeof (port_8023ah_ctrl.loopback_enable)));

    *(fal_port_8023ah_ctrl_t *)val = port_8023ah_ctrl;
    return SW_OK;
}
#endif

sw_error_t
cmd_data_check_crossover_mode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strncasecmp(cmd_str, "auto", 5))
        *arg_val = PHY_MDIX_AUTO;
    else if (!strncasecmp(cmd_str, "mdi", 4))
        *arg_val = PHY_MDIX_MDI;
    else if (!strncasecmp(cmd_str, "mdix", 5))
        *arg_val = PHY_MDIX_MDIX;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_crossover_status(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;
    if (!strncasecmp(cmd_str, "mdi", 4))
        *arg_val = PHY_MDIX_STATUS_MDI;
    else if (!strncasecmp(cmd_str, "mdix", 5))
        *arg_val = PHY_MDIX_STATUS_MDIX;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_prefer_medium(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;
    if (!strncasecmp(cmd_str, "copper", 7))
        *arg_val = PHY_MEDIUM_COPPER;
    else if (!strncasecmp(cmd_str, "fiber", 6))
        *arg_val = PHY_MEDIUM_FIBER;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_fiber_mode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;
    if (!strncasecmp(cmd_str, "100fx", 6))
    {
	*arg_val = PHY_FIBER_100FX;
    }
    else if (!strncasecmp(cmd_str, "1000bx", 7))
    {
	*arg_val = PHY_FIBER_1000BX;
    }
    else if (!strncasecmp(cmd_str, "10g_r", 7))
    {
	*arg_val = PHY_FIBER_10G_R;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_source_filter_mode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
	if (cmd_str == NULL)
	{
		return SW_BAD_PARAM;
	}
	if (!strncasecmp(cmd_str, "virtual_port", 15))
	{
		*arg_val = FAL_SRC_FILTER_MODE_VP;
	}
	else if (!strncasecmp(cmd_str, "physical_port", 15))
	{
		*arg_val = FAL_SRC_FILTER_MODE_PHYSICAL;
	}
	else
	{
		return SW_BAD_VALUE;
	}

	return SW_OK;
}

sw_error_t
cmd_data_check_src_filter_config(char *cmd_str, a_uint32_t *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_src_filter_config_t src_filter_config;

	aos_mem_zero(&src_filter_config, sizeof (fal_src_filter_config_t));

	do
	{
		cmd = get_sub_cmd("src_filter_enable", "enable");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_enable(cmd, &(src_filter_config.src_filter_enable),
					sizeof (a_bool_t));
			SW_RTN_ON_ERROR(rv);
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("srcfilter_mode", "virtual_port");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			cmd_data_check_source_filter_mode(cmd, &(src_filter_config.src_filter_mode),
				sizeof(a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	*(fal_src_filter_config_t *)arg_val = src_filter_config;

	return SW_OK;
}

sw_error_t
cmd_data_check_mtu_entry(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_mtu_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_mtu_ctrl_t));

    do
    {
        cmd = get_sub_cmd("mtu_size", "1514");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.mtu_size), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mtu_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.action), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
    *(fal_mtu_ctrl_t *)val = entry;
    return SW_OK;
}
#ifdef APPE
sw_error_t
cmd_data_check_mtu_cfg(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    fal_mtu_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_mtu_cfg_t));
    cmd_data_check_element("mtu_enable", "enable",
                        "usage: usage: enable/disable\n",
                        cmd_data_check_enable, (cmd,
                        &(entry.mtu_enable), sizeof(entry.mtu_enable)));

    cmd_data_check_element("mtu_type", "ethernet",
                        "usage:mtu_type:ethernet/ip, etc\n",
                        cmd_data_check_attr, ("mtu_type", cmd,
                        &(entry.mtu_type), sizeof(entry.mtu_type)));

    cmd_data_check_element("extra_header_len", "0x80",
                        "usage: extra_header_len\n",
                        cmd_data_check_uint32, (cmd,
                        &(entry.extra_header_len),
                        sizeof(entry.extra_header_len)));

    cmd_data_check_element("eg_vlan_tag_flag", "0",
                        "usage: eg_vlan_tag_flag,bit 0 ctag, bit 1 stag\n",
                        cmd_data_check_uint32, (cmd,
                        &(entry.eg_vlan_tag_flag),
                        sizeof(entry.eg_vlan_tag_flag)));

    *(fal_mtu_cfg_t *)val = entry;

    return SW_OK;
}
#endif
sw_error_t
cmd_data_check_mru_entry(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_mru_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_mru_ctrl_t));

    do
    {
        cmd = get_sub_cmd("mru_size", "1514");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.mru_size), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mru_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.action), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_mru_ctrl_t *)val = entry;
    return SW_OK;
}

#endif

sw_error_t
cmd_data_check_port_cnt_cfg(char *cmd_str, fal_port_cnt_cfg_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_port_cnt_cfg_t entry;

	aos_mem_zero(&entry, sizeof(fal_port_cnt_cfg_t));

	cmd_data_check_element("rx_cnt_enable", "disable",
						"usage: usage: enable/disable\n",
						cmd_data_check_enable, (cmd,
						&(entry.rx_cnt_en), sizeof(entry.rx_cnt_en)));
	cmd_data_check_element("uc_tx_cnt_enable", "disable",
						"usage: usage: enable/disable\n",
						cmd_data_check_enable, (cmd,
						&(entry.uc_tx_cnt_en), sizeof(entry.uc_tx_cnt_en)));
	cmd_data_check_element("mc_tx_cnt_enable", "disable",
						"usage: usage: enable/disable\n",
						cmd_data_check_enable, (cmd,
						&(entry.mc_tx_cnt_en), sizeof(entry.mc_tx_cnt_en)));

#if defined(APPE)
	cmd_data_check_element("tl_rx_cnt_enable", "disable",
						"usage: usage: enable/disable\n",
						cmd_data_check_enable, (cmd,
						&(entry.tl_rx_cnt_en), sizeof(entry.tl_rx_cnt_en)));

	cmd_data_check_element("rx_cnt_mode", "full_pkt",
						"usage: full_pkt or ip_pkt\n",
						cmd_data_check_attr, ("cnt_mode", cmd,
						&entry.rx_cnt_mode, sizeof(entry.rx_cnt_mode)));

	cmd_data_check_element("tx_cnt_mode", "full_pkt",
						"usage: full_pkt or ip_pkt\n",
						cmd_data_check_attr, ("cnt_mode", cmd,
						&entry.tx_cnt_mode, sizeof(entry.tx_cnt_mode)));
#endif

	*(fal_port_cnt_cfg_t *)arg_val = entry;

	return SW_OK;
}

#endif

#ifdef IN_INTERFACECONTROL
sw_error_t
cmd_data_check_interface_mode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strncasecmp(cmd_str, "psgmii_baset", 13))
     {
	*arg_val = PHY_PSGMII_BASET;
     }
    else if (!strncasecmp(cmd_str, "psgmii_bx1000", 14))
    {
	*arg_val = PHY_PSGMII_BX1000;
    }
    else if (!strncasecmp(cmd_str, "psgmii_fx100", 13))
    {
	*arg_val = PHY_PSGMII_FX100;
    }
    else if (!strncasecmp(cmd_str, "psgmii_amdet", 13))
    {
	*arg_val = PHY_PSGMII_AMDET;
    }
    else if (!strncasecmp(cmd_str, "rgmii_amdet", 13))
    {
	*arg_val = PORT_RGMII_AMDET;
    }
    else if (!strncasecmp(cmd_str, "rgmii_baset", 13))
    {
	*arg_val = PORT_RGMII_BASET;
    }
    else if (!strncasecmp(cmd_str, "rgmii_bx1000", 13))
    {
	*arg_val = PORT_RGMII_BX1000;
    }
    else if (!strncasecmp(cmd_str, "rgmii_fx100", 13))
    {
	*arg_val = PORT_RGMII_FX100;
    }
    else if (!strncasecmp(cmd_str, "sgmii_baset", 13))
    {
	*arg_val = PHY_SGMII_BASET;
    }
    else if (!strncasecmp(cmd_str, "qsgmii", 13))
    {
	*arg_val = PORT_QSGMII;
    }
    else if (!strncasecmp(cmd_str, "sgmii_plus", 13))
    {
	*arg_val = PORT_SGMII_PLUS;
    }
    else if (!strncasecmp(cmd_str, "usxgmii", 13))
    {
	*arg_val = PORT_USXGMII;
    }
    else if (!strncasecmp(cmd_str, "10gbase_r", 13))
    {
	*arg_val = PORT_10GBASE_R;
    }
    else if (!strncasecmp(cmd_str, "interfacemode_max", 20))
    {
	*arg_val = PORT_INTERFACE_MODE_MAX;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif
/*portvlan*/
#ifdef IN_PORTVLAN
sw_error_t
cmd_data_check_1qmode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "disable"))
    {
        *arg_val = FAL_1Q_DISABLE;
    }
    else if (!strcasecmp(cmd_str, "secure"))
    {
        *arg_val = FAL_1Q_SECURE;
    }
    else if (!strcasecmp(cmd_str, "check"))
    {
        *arg_val = FAL_1Q_CHECK;
    }
    else if (!strcasecmp(cmd_str, "fallback"))
    {
        *arg_val = FAL_1Q_FALLBACK;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}



sw_error_t
cmd_data_check_egmode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "unmodified"))
    {
        *arg_val = FAL_EG_UNMODIFIED;
    }
    else if (!strcasecmp(cmd_str, "untagged"))
    {
        *arg_val = FAL_EG_UNTAGGED;
    }
    else if (!strcasecmp(cmd_str, "tagged"))
    {
        *arg_val = FAL_EG_TAGGED;
    }
    else if (!strcasecmp(cmd_str, "hybrid"))
    {
        *arg_val = FAL_EG_HYBRID;
    }
    else if (!strcasecmp(cmd_str, "untouched"))
    {
        *arg_val = FAL_EG_UNTOUCHED;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif
/*vlan*/
#ifdef IN_VLAN
sw_error_t
cmd_data_check_vlan(char *cmdstr, fal_vlan_t * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_vlan_t entry;
    a_uint32_t tmp = 0;

    memset(&entry, 0, sizeof (fal_vlan_t));

    rv = __cmd_data_check_complex("vlanid", NULL,
                        "usage: the range is 0 -- 4095\n",
                        (param_check_t)cmd_data_check_uint16, &tmp, sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.vid = tmp & 0xffff;

    rv = __cmd_data_check_complex("fid", NULL,
                        "usage: the range is 0 -- 4095 or 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp, sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.fid = tmp & 0xffff;

    rv = __cmd_data_check_complex("port member", "null",
                        "usage: input port number such as 1,3\n",
                        (param_check_t)cmd_data_check_portmap, &entry.mem_ports,
                        sizeof (fal_pbmp_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("tagged member", "null",
                        "usage: input port number such as 1,3\n",
                        (param_check_t)cmd_data_check_portmap, &entry.tagged_ports,
                        sizeof (fal_pbmp_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("untagged member", "null",
                        "usage: input port number such as 1,3\n",
                        (param_check_t)cmd_data_check_portmap, &entry.untagged_ports,
                        sizeof (fal_pbmp_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("unmodify member", "null",
                        "usage: input port number such as 1,3\n",
                        (param_check_t)cmd_data_check_portmap, &entry.unmodify_ports,
                        sizeof (fal_pbmp_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("unmodify member", "null",
                        "usage: input port number such as 1,3\n",
                        (param_check_t)cmd_data_check_portmap, &entry.unmodify_ports,
                        sizeof (fal_pbmp_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("learn disable", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.learn_dis,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("queue override", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.vid_pri_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == entry.vid_pri_en)
    {
        rv = __cmd_data_check_complex("queue", NULL,
                        "usage: input number such as <0/1/2/3>\n",
                        (param_check_t)cmd_data_check_uint32, &tmp, sizeof (a_uint32_t));
        if (rv)
            return rv;

        entry.vid_pri = tmp;
    }

    *val = entry;
    return SW_OK;
}
#endif
/*qos*/
#ifdef IN_QOS
#ifndef IN_QOS_MINI
sw_error_t
cmd_data_check_port_group(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_qos_group_t entry;

    aos_mem_zero(&entry, sizeof (fal_qos_group_t));

    do
    {
        cmd = get_sub_cmd("pcp_group", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.pcp_group),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dscp_group", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.dscp_group),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("flow_group", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.flow_group),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_qos_group_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_port_pri(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_qos_pri_precedence_t entry;
#if defined(APPE)
    a_uint32_t tmp = 0;
#endif

    aos_mem_zero(&entry, sizeof (fal_qos_pri_precedence_t));

    do
    {
        cmd = get_sub_cmd("pcp_pri_prece", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.pcp_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dscp_pri_prece", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.dscp_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("preheader_pri_prece", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.preheader_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("flow_pri_prece", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.flow_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("acl_pri_prece", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.acl_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("post_acl_pri_prece", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.post_acl_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("pcp_pri_force", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.pcp_pri_force),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dscp_pri_force", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.dscp_pri_force),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do {
	    cmd = get_sub_cmd("pre_acl_outer_pri_prece", "0");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_uint8(cmd, &tmp, sizeof(a_uint32_t));
		    if (SW_OK == rv)
			    entry.pre_acl_outer_pri = tmp;
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("pre_acl_inner_pri_prece", "0");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_uint8(cmd, &tmp, sizeof(a_uint32_t));
		    if (SW_OK == rv)
			    entry.pre_acl_inner_pri = tmp;
	    }
    } while (talk_mode && (SW_OK != rv));
#endif

    *(fal_qos_pri_precedence_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_port_remark(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_qos_remark_enable_t entry;

    aos_mem_zero(&entry, sizeof (fal_qos_remark_enable_t));

    do
    {
        cmd = get_sub_cmd("pcp_change_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.pcp_change_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dei_change_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.dei_chage_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dscp_change_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.dscp_change_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_qos_remark_enable_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_cosmap(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_qos_cosmap_t entry;

    aos_mem_zero(&entry, sizeof (fal_qos_cosmap_t));

    do
    {
        cmd = get_sub_cmd("internal_pcp", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.internal_pcp),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("internal_dei", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.internal_dei),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("internal_pri", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.internal_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("internal_dscp", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.internal_dscp),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("internal_dp", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.internal_dp),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dscp_mask", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.dscp_mask),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dscp_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.dscp_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("pcp_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.pcp_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dei_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.dei_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("pri_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.pri_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dp_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.dp_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("qos_prec", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.qos_prec),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_qos_cosmap_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_queue_scheduler(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_qos_scheduler_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_qos_scheduler_cfg_t));

    do
    {
        cmd = get_sub_cmd("sp_id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.sp_id),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("e_pri", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.e_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("c_pri", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.c_pri),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("e_drr_id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.e_drr_id),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("c_drr_id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.c_drr_id),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("e_drr_wt", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.e_drr_wt),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("c_drr_wt", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.c_drr_wt),
                                       sizeof (a_uint16_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("c_drr_ut", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.c_drr_unit),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("e_drr_ut", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.e_drr_unit),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("drr_frame_mode", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.drr_frame_mode),
                                       sizeof (a_uint32_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_qos_scheduler_cfg_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ring_queue(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_uint32_t i = 0;
    sw_error_t rv;
    fal_queue_bmp_t entry;

    aos_mem_zero(&entry, sizeof (fal_queue_bmp_t));

    do
    {
        cmd = get_sub_cmd("bmp", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.bmp[i]),
                                       sizeof (a_uint32_t));
        }

    }
    while ((talk_mode && (SW_OK != rv)) || (++i < 10));

    *(fal_queue_bmp_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_qos_sch(char *cmdstr, fal_sch_mode_t * val, a_uint32_t size)
{
    if (cmdstr == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmdstr, "sp"))
    {
        *val = FAL_SCH_SP_MODE;
    }
    else if (!strcasecmp(cmdstr, "wrr"))
    {
        *val = FAL_SCH_WRR_MODE;
    }
    else if (!strcasecmp(cmdstr, "mixplus"))
    {
        *val = FAL_SCH_MIX_PLUS_MODE;
    }
    else if (!strcasecmp(cmdstr, "mix"))
    {
        *val = FAL_SCH_MIX_MODE;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_qos_pt(char *cmdstr, fal_qos_mode_t * val, a_uint32_t size)
{
    if (cmdstr == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmdstr, "da"))
    {
        *val = FAL_QOS_DA_MODE;
    }
    else if (!strcasecmp(cmdstr, "up"))
    {
        *val = FAL_QOS_UP_MODE;
    }
    else if (!strcasecmp(cmdstr, "dscp"))
    {
        *val = FAL_QOS_DSCP_MODE;
    }
    else if (!strcasecmp(cmdstr, "port"))
    {
        *val = FAL_QOS_PORT_MODE;
    }
    else if (!strcasecmp(cmdstr, "flow"))
    {
        *val = FAL_QOS_FLOW_MODE;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif
#endif

/*rate*/
#ifdef IN_RATE
sw_error_t
cmd_data_check_storm(char *cmdstr, fal_storm_type_t * val, a_uint32_t size)
{
    if (cmdstr == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmdstr, "unicast"))
    {
        *val = FAL_UNICAST_STORM;
    }
    else if (!strcasecmp(cmdstr, "multicast"))
    {
        *val = FAL_MULTICAST_STORM;
    }
    else if (!strcasecmp(cmdstr, "broadcast"))
    {
        *val = FAL_BROADCAST_STORM;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif

/*stp*/
#ifdef IN_STP
sw_error_t
cmd_data_check_stp_state(char *cmdstr, fal_stp_state_t * val, a_uint32_t size)
{
    if (cmdstr == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmdstr, "disable"))
    {
        *val = FAL_STP_DISABLED;
    }
    else if (!strcasecmp(cmdstr, "block"))
    {
        *val = FAL_STP_BLOKING;
    }
    else if (!strcasecmp(cmdstr, "listen"))
    {
        *val = FAL_STP_LISTENING;
    }
    else if (!strcasecmp(cmdstr, "learn"))
    {
        *val = FAL_STP_LEARNING;
    }
    else if (!strcasecmp(cmdstr, "forward"))
    {
        *val = FAL_STP_FARWARDING;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif

#ifdef IN_LEAKY
/*general*/
sw_error_t
cmd_data_check_leaky(char *cmdstr, fal_leaky_ctrl_mode_t * val, a_uint32_t size)
{
    if (cmdstr == NULL)
        return SW_BAD_VALUE;

    if (!strcasecmp(cmdstr, "port"))
    {
        *val = FAL_LEAKY_PORT_CTRL;
    }
    else if (!strcasecmp(cmdstr, "fdb"))
    {
        *val = FAL_LEAKY_FDB_CTRL;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif

sw_error_t
cmd_data_check_uinta(char *cmdstr, a_uint32_t * val, a_uint32_t size)
{
    char *tmp_str = NULL;
    a_uint32_t *tmp_ptr = val;
    a_uint32_t i = 0;

    tmp_str = (void *) strsep(&cmdstr, ",");

    while (tmp_str)
    {
        if (i >= (size / 4))
        {
            return SW_BAD_VALUE;
        }

        sscanf(tmp_str, "%d", tmp_ptr);
        tmp_ptr++;

        i++;
        tmp_str = (void *) strsep(&cmdstr, ",");
    }

    if (i != (size / 4))
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


/*fdb*/
sw_error_t
cmd_data_check_maccmd(char *cmdstr, fal_fwd_cmd_t * val, a_uint32_t size)
{
    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmdstr[0])
    {
        *val = FAL_MAC_FRWRD;   //defualt
    }
    else if (!strcasecmp(cmdstr, "forward"))
    {
        *val = FAL_MAC_FRWRD;
    }
    else if (!strcasecmp(cmdstr, "drop"))
    {
        *val = FAL_MAC_DROP;
    }
    else if (!strcasecmp(cmdstr, "cpycpu"))
    {
        *val = FAL_MAC_CPY_TO_CPU;
    }
    else if (!strcasecmp(cmdstr, "rdtcpu"))
    {
        *val = FAL_MAC_RDT_TO_CPU;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

/*flow*/
#ifdef IN_IP
#if !defined(IN_IP_MINI)
sw_error_t
cmd_data_check_flowcmd(char *cmdstr, fal_default_flow_cmd_t * val, a_uint32_t size)
{
    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmdstr[0])
    {
        *val = FAL_DEFAULT_FLOW_FORWARD;   //default
    }
    else if (!strcasecmp(cmdstr, "forward"))
    {
        *val = FAL_DEFAULT_FLOW_FORWARD;
    }
    else if (!strcasecmp(cmdstr, "drop"))
    {
        *val = FAL_DEFAULT_FLOW_DROP;
    }
    else if (!strcasecmp(cmdstr, "rdtcpu"))
    {
        *val = FAL_DEFAULT_FLOW_RDT_TO_CPU;
    }
    else if (!strcasecmp(cmdstr, "admit_all"))
    {
        *val = FAL_DEFAULT_FLOW_ADMIT_ALL;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_flowtype(char *cmd_str, fal_flow_type_t * arg_val,
                        a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmd_str, "lan2lan"))
    {
        *arg_val = FAL_FLOW_LAN_TO_LAN;
    }
    else if (!strcasecmp(cmd_str, "wan2lan"))
    {
        *arg_val = FAL_FLOW_WAN_TO_LAN;
    }
    else if (!strcasecmp(cmd_str, "lan2wan"))
    {
        *arg_val = FAL_FLOW_LAN_TO_WAN;
    }
    else if (!strcasecmp(cmd_str, "wan2wan"))
    {
        *arg_val = FAL_FLOW_WAN_TO_WAN;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif
#endif
sw_error_t
cmd_data_check_confirm(char *cmdstr, a_bool_t def, a_bool_t * val,
                       a_uint32_t size)
{
    if (0 == cmdstr[0])
    {
        *val = def;
    }
    else if ((!strcasecmp(cmdstr, "yes")) || (!strcasecmp(cmdstr, "y")))
    {
        *val = A_TRUE;
    }
    else if ((!strcasecmp(cmdstr, "no")) || (!strcasecmp(cmdstr, "n")))
    {
        *val = A_FALSE;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_portmap(char *cmdstr, fal_pbmp_t * val, a_uint32_t size)
{
    char *tmp = NULL;
    char tmp_str[256] = {0};
    a_uint32_t i = 0;
    a_uint32_t port;

    *val = 0;
    //default input null
    if(!strcasecmp(cmdstr, "null"))
    {
        return SW_OK;
    }

    strlcpy(tmp_str, cmdstr, sizeof(tmp_str));
    tmp = (void *) strsep(&cmdstr, ",");
    while (tmp)
    {
        if (SW_MAX_NR_PORT <= i)
        {
            return SW_BAD_VALUE;
        }

        sscanf(tmp, "%d", &port);
        if (SW_MAX_NR_PORT <= port)
        {
            return cmd_data_check_uint32(tmp_str, val, sizeof(a_uint32_t));
        }

        *val |= (0x1 << port);
        tmp = (void *) strsep(&cmdstr, ",");
	i++;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_macaddr(char *cmdstr, void *val, a_uint32_t size)
{
    char *tmp = NULL;
    a_uint32_t i = 0, j;
    a_uint32_t addr;
    fal_mac_addr_t mac;

    memset(&mac, 0, sizeof (fal_mac_addr_t));
    if (NULL == cmdstr)
    {
        *(fal_mac_addr_t *) val = mac;
        return SW_BAD_VALUE; /*was: SW_OK;*/
    }

    if (0 == cmdstr[0])
    {
        *(fal_mac_addr_t *) val = mac;
        return SW_OK;
    }

    tmp = (void *) strsep(&cmdstr, "-");
    while (tmp)
    {
        if (6 <= i)
        {
            return SW_BAD_VALUE;
        }

        if ((2 < strlen(tmp)) || (0 == strlen(tmp)))
        {
            return SW_BAD_VALUE;
        }

        for (j = 0; j < strlen(tmp); j++)
        {
            if (A_FALSE == is_hex(tmp[j]))
                return SW_BAD_VALUE;
        }

        sscanf(tmp, "%x", &addr);
        if (0xff < addr)
        {
            return SW_BAD_VALUE;
        }

        mac.uc[i++] = addr;
        tmp = (void *) strsep(&cmdstr, "-");
    }

    if (6 != i)
    {
        return SW_BAD_VALUE;
    }

    *(fal_mac_addr_t *) val = mac;
    return SW_OK;
}
#ifdef IN_FDB
sw_error_t
cmd_data_check_fdbentry(char *info, void *val, a_uint32_t size)
{
    sw_error_t rv;
    fal_fdb_entry_t entry;
    a_uint32_t tmp = 0;

    memset(&entry, 0, sizeof (fal_fdb_entry_t));

    rv = __cmd_data_check_complex("addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_macaddr, &entry.addr,
                        sizeof (fal_mac_addr_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("fid", "65535",
                        "usage: the range is 1 -- 4095 or 65535\n",
                        (param_check_t)cmd_data_check_uint32, &tmp, sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.fid = tmp & 0xffff;

    rv = __cmd_data_check_complex("dacmd", "forward",
                        "usage: <forward/drop/cpycpu/rdtcpu>\n",
                        (param_check_t)cmd_data_check_maccmd, &entry.dacmd,
                        sizeof (fal_fwd_cmd_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("sacmd", "forward",
                        "usage: <forward/drop/cpycpu/rdtcpu>\n",
                        (param_check_t)cmd_data_check_maccmd, &entry.sacmd,
                        sizeof (fal_fwd_cmd_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("dest port", "null",
                        "usage: input port number such as 1,3\n",
                        (param_check_t)cmd_data_check_portmap, &entry.port.map,
                        sizeof (fal_pbmp_t));
    if (rv)
        return rv;

    entry.portmap_en = A_TRUE;

    rv = __cmd_data_check_boolean("static", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.static_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("leaky", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.leaky_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("mirror", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.mirror_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("clone", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.clone_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("queue override", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE,
			&entry.da_pri_en, sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == entry.da_pri_en)
    {
        rv = __cmd_data_check_complex("queue", NULL,
                            "usage: input number such as <0/1/2/3>\n",
                            (param_check_t)cmd_data_check_uint32, &tmp,
			    sizeof (a_uint32_t));
        if (rv)
            return rv;
        entry.da_queue = tmp;
    }

    rv = __cmd_data_check_boolean("cross_pt_state", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, 
			&entry.cross_pt_state, sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("white_list_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE,
			&entry.white_list_en, sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("load_balance_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.load_balance_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == entry.load_balance_en)
    {
        rv = __cmd_data_check_complex("load_balance", NULL,
                            "usage: input number such as <0/1/2/3>\n",
                            (param_check_t)cmd_data_check_uint32, &tmp, sizeof (a_uint32_t));
        if (rv)
            return rv;
        entry.load_balance = tmp;
    }

    *(fal_fdb_entry_t *) val = entry;

    return SW_OK;
}
#ifndef IN_FDB_MINI
sw_error_t
cmd_data_check_maclimit_ctrl(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_maclimit_ctrl_t maclimit_ctrl;

    memset(&maclimit_ctrl, 0, sizeof (fal_maclimit_ctrl_t));

    do
    {
        cmd = get_sub_cmd("enable", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &maclimit_ctrl.enable,
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("limit num", "2048");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &maclimit_ctrl.limit_num,
                                        sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &maclimit_ctrl.action,
                                        sizeof (fal_fwd_cmd_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_maclimit_ctrl_t *) val = maclimit_ctrl;

    return SW_OK;
}
#endif
#endif
sw_error_t
cmd_data_check_integer(char *cmd_str, a_uint32_t * arg_val, a_uint32_t max_val,
                       a_uint32_t min_val)
{
    a_uint32_t tmp;
    a_uint32_t i;

    if (NULL == cmd_str)
    {
        return SW_BAD_PARAM;
    }

    if (0 == cmd_str[0])
    {
        return SW_BAD_PARAM;
    }

    if ((cmd_str[0] == '0') && ((cmd_str[1] == 'x') || (cmd_str[1] == 'X')))
    {
        for (i = 2; i < strlen(cmd_str); i++)
        {
            if (A_FALSE == is_hex(cmd_str[i]))
            {
                return SW_BAD_VALUE;
            }
        }
        sscanf(cmd_str, "%x", &tmp);
    }
    else
    {
        for (i = 0; i < strlen(cmd_str); i++)
        {
            if (A_FALSE == is_dec(cmd_str[i]))
            {
                return SW_BAD_VALUE;
            }
        }
        sscanf(cmd_str, "%d", &tmp);
    }

    if ((tmp > max_val) || (tmp < min_val))
        return SW_BAD_PARAM;

    *arg_val = tmp;
    return SW_OK;
}
#ifdef IN_ACL
sw_error_t
cmd_data_check_ruletype(char *cmd_str, fal_acl_rule_type_t * arg_val,
                        a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmd_str, "mac"))
    {
        *arg_val = FAL_ACL_RULE_MAC;
    }
    else if (!strcasecmp(cmd_str, "ip4"))
    {
        *arg_val = FAL_ACL_RULE_IP4;
    }
    else if (!strcasecmp(cmd_str, "ip6"))
    {
        *arg_val = FAL_ACL_RULE_IP6;
    }
    else if (!strcasecmp(cmd_str, "udf"))
    {
        *arg_val = FAL_ACL_RULE_UDF;
    }
    else
    {
#ifdef APPE
        if (!strcasecmp(cmd_str, "tunnel_mac"))
        {
            *arg_val = FAL_ACL_RULE_TUNNEL_MAC;
        }
        else if (!strcasecmp(cmd_str, "tunnel_ip4"))
        {
            *arg_val = FAL_ACL_RULE_TUNNEL_IP4;
        }
        else if (!strcasecmp(cmd_str, "tunnel_ip6"))
        {
            *arg_val = FAL_ACL_RULE_TUNNEL_IP6;
        }
        else if (!strcasecmp(cmd_str, "tunnel_udf"))
        {
            *arg_val = FAL_ACL_RULE_TUNNEL_UDF;
        }
        else
        {
            return SW_BAD_VALUE;
        }
        return SW_OK;
#endif
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_udf_pkt_type(char *cmdstr, fal_acl_udf_pkt_type_t * arg_val, a_uint32_t size)
{
    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmdstr, "non-ip"))
    {
        *arg_val = FAL_ACL_UDF_NON_IP;
    }
    else if (!strcasecmp(cmdstr, "ipv4"))
    {
        *arg_val = FAL_ACL_UDF_IP4;
    }
    else if (!strcasecmp(cmdstr, "ipv6"))
    {
        *arg_val = FAL_ACL_UDF_IP6;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_udf_type(char *cmdstr, fal_acl_udf_type_t * arg_val, a_uint32_t size)
{
    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmdstr, "l2"))
    {
        *arg_val = FAL_ACL_UDF_TYPE_L2;
    }
    else if (!strcasecmp(cmdstr, "l2snap"))
    {
        *arg_val = FAL_ACL_UDF_TYPE_L2_SNAP;
    }
    else if (!strcasecmp(cmdstr, "l3"))
    {
        *arg_val = FAL_ACL_UDF_TYPE_L3;
    }
    else if (!strcasecmp(cmdstr, "l3plus"))
    {
        *arg_val = FAL_ACL_UDF_TYPE_L3_PLUS;
    }
    else if (!strcasecmp(cmdstr, "l4"))
    {
        *arg_val = FAL_ACL_UDF_TYPE_L4;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_udf_element(char *cmdstr, a_uint8_t * val, a_uint32_t * len)
{
    char *tmp = NULL;
    a_uint32_t i = 0, j;
    a_uint32_t data;

    memset(val, 0, 16);
    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmdstr[0])
    {
        return SW_BAD_VALUE;
    }

    tmp = (void *) strsep(&cmdstr, "-");
    while (tmp)
    {
        if (16 <= i)
        {
            return SW_BAD_VALUE;
        }

        if ((2 < strlen(tmp)) || (0 == strlen(tmp)))
        {
            return SW_BAD_VALUE;
        }

        for (j = 0; j < strlen(tmp); j++)
        {
            if (A_FALSE == is_hex(tmp[j]))
            {
                return SW_BAD_VALUE;
            }
        }

        sscanf(tmp, "%x", &data);

        val[i++] = data & 0xff;
        tmp = (void *) strsep(&cmdstr, "-");
    }

    if (0 == i)
    {
        return SW_BAD_VALUE;
    }

    *len = i;
    return SW_OK;
}

sw_error_t
cmd_data_check_fieldop(char *cmdstr, fal_acl_field_op_t def,
                       fal_acl_field_op_t * val)
{
    if ('\0' == cmdstr[0])
    {
        *val = def;
    }
    else if ((!strcasecmp(cmdstr, "mask")) || (!strcasecmp(cmdstr, "m")))
    {
        *val = FAL_ACL_FIELD_MASK;
    }
    else if ((!strcasecmp(cmdstr, "range")) || (!strcasecmp(cmdstr, "r")))
    {
        *val = FAL_ACL_FIELD_RANGE;
    }
    else if ((!strcasecmp(cmdstr, "le")) || (!strcasecmp(cmdstr, "l")))
    {
        *val = FAL_ACL_FIELD_LE;
    }
    else if ((!strcasecmp(cmdstr, "ge")) || (!strcasecmp(cmdstr, "g")))
    {
        *val = FAL_ACL_FIELD_GE;
    }
    else if ((!strcasecmp(cmdstr, "ne")) || (!strcasecmp(cmdstr, "n")))
    {
        *val = FAL_ACL_FIELD_NE;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

#ifdef APPE
sw_error_t
cmd_data_check_vport_type(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    return cmd_data_check_attr("vport_type", cmd_str,
                       arg_val, sizeof(*arg_val));
}
#endif
#endif
sw_error_t
cmd_data_check_ip4addr(char *cmdstr, void * val, a_uint32_t size)
{
    char *tmp = NULL;
    a_uint32_t i = 0, j;
    a_uint32_t addr;
    fal_ip4_addr_t ip4;
    char cmd[128] = { 0 };
	char *str;

    memset(&ip4, 0, sizeof (fal_ip4_addr_t));
    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmdstr[0])
    {
        return SW_BAD_VALUE;
    }

    for (i = 0; i < 128; i++)
    {
        if (0 == cmdstr[i])
        {
            break;
        }
    }

    i++;
    if (128 < i)
    {
        i = 128;
    }

    memcpy(cmd, cmdstr, i);
	str = cmd;
    tmp = (void *) strsep(&str, ".");
    i = 0;
    while (tmp)
    {
        if (4 <= i)
        {
            return SW_BAD_VALUE;
        }

        if ((3 < strlen(tmp)) || (0 == strlen(tmp)))
        {
            return SW_BAD_VALUE;
        }

        for (j = 0; j < strlen(tmp); j++)
        {
            if (A_FALSE == is_dec(tmp[j]))
            {
                return SW_BAD_VALUE;
            }
        }

        sscanf(tmp, "%d", &addr);
        if (255 < addr)
        {
            return SW_BAD_VALUE;
        }

        ip4 |= ((addr & 0xff) << (24 - i * 8));
        i++;
        tmp = (void *) strsep(&str, ".");
    }

    if (4 != i)
    {
        return SW_BAD_VALUE;
    }

    *(fal_ip4_addr_t*)val = ip4;
    return SW_OK;
}

#ifdef IN_IGMP
sw_error_t
cmd_data_check_multi(char *info, void *val, a_uint32_t size)
{
    sw_error_t rv;
    fal_igmp_sg_entry_t entry;

    memset(&entry, 0, sizeof (fal_igmp_sg_entry_t));

    rv = __cmd_data_check_complex("group type", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.group.type),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    if(entry.group.type == 0)
    {
        rv = __cmd_data_check_complex("group ip4 addr", "0.0.0.0",
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.group.u.ip4_addr),
                            4);
        if (rv)
            return rv;
    }
    else
    {
        rv = __cmd_data_check_complex("group ip6 addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip6addr, &(entry.group.u.ip6_addr),
                            16);
        if (rv)
            return rv;

    }

    rv = __cmd_data_check_complex("source type", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.source.type),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    if(entry.source.type == 0)
    {
        rv = __cmd_data_check_complex("source ip4 addr", "0.0.0.0",
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.source.u.ip4_addr),
                            4);
        if (rv)
            return rv;
    }
    else
    {
        rv = __cmd_data_check_complex("source ip6 addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip6addr, &(entry.source.u.ip6_addr),
                            16);
        if (rv)
            return rv;
    }

    rv = __cmd_data_check_complex("portmap", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.port_map),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vlanid", "0xffff",
                        "usage: the range is 0 -- 4095 or 65535\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.vlan_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    *(fal_igmp_sg_entry_t *)val = entry;

    return SW_OK;
}
#endif

sw_error_t
cmd_data_check_ip6addr(char *cmdstr, void * val, a_uint32_t size)
{
    char *tmp = NULL;
    a_uint32_t j;
    a_uint32_t i = 0, cnt = 0, rep = 0, loc = 0;
    a_uint32_t data;
    a_uint32_t addr[8];
    fal_ip6_addr_t ip6;

    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmdstr[0])
    {
        return SW_BAD_VALUE;
    }

    for (i = 0; i < 8; i++)
    {
        addr[i] = 0;
    }

    for (i = 0; i < strlen(cmdstr); i++)
    {
        if (':' == cmdstr[i])
        {
            if ((i == (strlen(cmdstr) - 1))
                    || (0 == i))
            {
                return SW_BAD_VALUE;
            }
            cnt++;

            if (':' == cmdstr[i - 1])
            {
                rep++;
                loc = cnt - 1;
            }
        }
    }

    if (1 < rep)
    {
        return SW_BAD_VALUE;
    }

    tmp = (void *) strsep(&cmdstr, ":");
    i = 0;
    while (tmp)
    {
        if (8 <= i)
        {
            return SW_BAD_VALUE;
        }

        if (!strcmp(tmp, "")) {
                tmp = (void *)strsep(&cmdstr, ":");
                continue;
        }

        if ((4 < strlen(tmp)) || (0 == strlen(tmp)))
        {
            return SW_BAD_VALUE;
        }

        for (j = 0; j < strlen(tmp); j++)
        {
            if (A_FALSE == is_hex(tmp[j]))
            {
                return SW_BAD_VALUE;
            }
        }

        sscanf(tmp, "%x", &data);
        if (65535 < data)
        {
            return SW_BAD_VALUE;
        }

        addr[i++] = data;
        tmp = (void *) strsep(&cmdstr, ":");
    }

    if (0 == rep)
    {
        if (8 != i)
        {
            return SW_BAD_VALUE;
        }
    }
    else
    {
        if (8 <= i)
        {
            return SW_BAD_VALUE;
        }

        for (j = i - 1; j >= loc; j--)
        {
            addr[8 - i + j] = addr[j];
            addr[j] = 0;
        }
    }

    for (i = 0; i < 4; i++)
    {
        ip6.ul[i] = (addr[i * 2] << 16) | addr[i * 2 + 1];
    }

    *(fal_ip6_addr_t*)val = ip6;
    return SW_OK;
}

sw_error_t
cmd_data_check_patternmode(char *cmd_str, led_pattern_mode_t * arg_val,
                           a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmd_str, "always_off"))
    {
        *arg_val = LED_ALWAYS_OFF;
    }
    else if (!strcasecmp(cmd_str, "always_blink"))
    {
        *arg_val = LED_ALWAYS_BLINK;
    }
    else if (!strcasecmp(cmd_str, "always_on"))
    {
        *arg_val = LED_ALWAYS_ON;
    }
    else  if (!strcasecmp(cmd_str, "map"))
    {
        *arg_val = LED_PATTERN_MAP_EN;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#ifdef IN_LED
sw_error_t
cmd_data_check_blinkfreq(char *cmd_str, led_blink_freq_t * arg_val,
                         a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmd_str, "2HZ"))
    {
        *arg_val = LED_BLINK_2HZ;
    }
    else if (!strcasecmp(cmd_str, "4HZ"))
    {
        *arg_val = LED_BLINK_4HZ;
    }
    else if (!strcasecmp(cmd_str, "8HZ"))
    {
        *arg_val = LED_BLINK_8HZ;
    }
    else if (!strcasecmp(cmd_str, "TXRX"))
    {
        *arg_val = LED_BLINK_TXRX;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_ledpattern(char *info, void * val, a_uint32_t size)
{
    led_ctrl_pattern_t pattern;
    a_uint32_t tmpdata;
    sw_error_t rv;

    memset(&pattern, 0, sizeof (led_ctrl_pattern_t));

    /* get pattern mode configuration */
    rv = __cmd_data_check_complex("pattern_mode", NULL,
                        "usage: <always_off/always_blink/always_on/map>\n",
                        (param_check_t)cmd_data_check_patternmode, &pattern.mode,
                        sizeof (led_pattern_mode_t));
    if (rv)
        return rv;

    if (LED_PATTERN_MAP_EN == pattern.mode)
    {
        rv = __cmd_data_check_boolean("full_duplex_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << FULL_DUPLEX_LIGHT_EN);
        }

        rv = __cmd_data_check_boolean("half_duplex_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << HALF_DUPLEX_LIGHT_EN);
        }

        rv = __cmd_data_check_boolean("power_on_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << POWER_ON_LIGHT_EN);
        }

        rv = __cmd_data_check_boolean("link_1000m_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << LINK_1000M_LIGHT_EN);
        }

        rv = __cmd_data_check_boolean("link_100m_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << LINK_100M_LIGHT_EN);
        }

        rv = __cmd_data_check_boolean("link_10m_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << LINK_10M_LIGHT_EN);
        }

        rv = __cmd_data_check_boolean("conllision_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << COLLISION_BLINK_EN);
        }

        rv = __cmd_data_check_boolean("rx_traffic_blink", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << RX_TRAFFIC_BLINK_EN);
        }

        rv = __cmd_data_check_boolean("tx_traffic_blink", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << TX_TRAFFIC_BLINK_EN);
        }

        rv = __cmd_data_check_boolean("linkup_override_light", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &tmpdata,
                        sizeof (a_bool_t));
        if (rv)
            return rv;

        if (1 == tmpdata)
        {
            pattern.map |= (1 << LINKUP_OVERRIDE_EN);
        }

        rv = __cmd_data_check_complex("blink freq", NULL,
                        "usage: <2HZ/4HZ/8HZ/TXRX> \n",
                        (param_check_t)cmd_data_check_blinkfreq, &pattern.freq,
                        sizeof (led_blink_freq_t));
        if (rv)
            return rv;
    }

    *(led_ctrl_pattern_t *)val = pattern;

    return SW_OK;
}
#endif

/*Shiva*/
#ifdef IN_PORTVLAN
sw_error_t
cmd_data_check_invlan_mode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "admit_all"))
    {
        *arg_val = FAL_INVLAN_ADMIT_ALL;
    }
    else if (!strcasecmp(cmd_str, "admit_tagged"))
    {
        *arg_val = FAL_INVLAN_ADMIT_TAGGED;
    }
    else if (!strcasecmp(cmd_str, "admit_untagged"))
    {
        *arg_val = FAL_INVLAN_ADMIT_UNTAGGED;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#ifdef HPPE
sw_error_t
cmd_data_check_global_qinqmode(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_global_qinq_mode_t *pEntry = (fal_global_qinq_mode_t *)val;

    memset(pEntry, 0, sizeof(fal_global_qinq_mode_t));

    /* get mask */
    do
    {
        cmd = get_sub_cmd("mask", "0x0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->mask), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get ingress mode */
    do
    {
        cmd = get_sub_cmd("ingress_qinq_mode", "ctag");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_qinq_mode(cmd, &(pEntry->ingress_mode), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get egress mode */
    do
    {
        cmd = get_sub_cmd("egress_qinq_mode", "ctag");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_qinq_mode(cmd, &(pEntry->egress_mode), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("untouched_for_cpucode", "enable");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    } else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    } else {
		    rv = cmd_data_check_enable(cmd, &(pEntry->untouched_for_cpucode),
				    sizeof(a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    return SW_OK;
}

sw_error_t
cmd_data_check_port_qinqmode(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_port_qinq_role_t *pEntry = (fal_port_qinq_role_t *)val;

    memset(pEntry, 0, sizeof(fal_port_qinq_role_t));

    /* get mask */
    do
    {
        cmd = get_sub_cmd("mask", "0x0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->mask), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get ingress mode */
    do
    {
        cmd = get_sub_cmd("ingress_qinq_role", "edge");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_qinq_role(cmd, &(pEntry->ingress_port_role), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get egress mode */
    do
    {
        cmd = get_sub_cmd("egress_qinq_role", "edge");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_qinq_role(cmd, &(pEntry->egress_port_role), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
	    cmd = get_sub_cmd("tunnel_qinq_role", "edge");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_qinq_role(cmd,
				    &(pEntry->tunnel_port_role), sizeof(a_uint32_t));
	    }
    } while (talk_mode && (SW_OK != rv));
#endif

    return SW_OK;
}

sw_error_t
cmd_data_check_tpid(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_tpid_t *pEntry = (fal_tpid_t *)val;
    a_uint32_t tmp = 0;

    memset(pEntry, 0, sizeof(fal_tpid_t));

    /* get mask */
    do
    {
        cmd = get_sub_cmd("mask", "0x0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->mask), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get ctpid */
    do
    {
        cmd = get_sub_cmd("ctagtpid", "0x8100");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->ctpid = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get stpid */
    do
    {
        cmd = get_sub_cmd("stagtpid", "0x88a8");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->stpid = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do {
	    cmd = get_sub_cmd("tunnel_ctagtpid", "0x8100");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));
		    if (rv == SW_OK)
			    pEntry->tunnel_ctpid = (a_uint16_t)tmp;
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("tunnel_stagtpid", "0x88a8");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));
		    if (rv == SW_OK)
			    pEntry->tunnel_stpid = (a_uint16_t)tmp;
	    }
    } while (talk_mode && (SW_OK != rv));
#endif

    return SW_OK;
}

sw_error_t
cmd_data_check_ingress_filter(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ingress_vlan_filter_t *pEntry = (fal_ingress_vlan_filter_t *)val;

    memset(pEntry, 0, sizeof(fal_ingress_vlan_filter_t));

    /* get in vlan filter */
    do
    {
        cmd = get_sub_cmd("membership_filter_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_enable(cmd, &(pEntry->membership_filter), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get tag filter */
    do
    {
        cmd = get_sub_cmd("tagged_filter_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_enable(cmd, &(pEntry->tagged_filter), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get untag filter */
    do
    {
        cmd = get_sub_cmd("untagged_filter_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_enable(cmd, &(pEntry->untagged_filter), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get priority tag filter */
    do
    {
        cmd = get_sub_cmd("priority_tagged_filter_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_enable(cmd, &(pEntry->priority_filter), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do {
	    cmd = get_sub_cmd("ctag_tagged_filter_en", "disable");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_enable(cmd, &(pEntry->ctag_tagged_filter),
				    sizeof(a_uint32_t));
	    }
    } while(talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("ctag_untagged_filter_en", "disable");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_enable(cmd, &(pEntry->ctag_untagged_filter),
				    sizeof(a_uint32_t));
	    }
    } while(talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("ctag_priority_tagged_filter_en", "disable");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_enable(cmd, &(pEntry->ctag_priority_filter),
				    sizeof(a_uint32_t));
	    }
    } while(talk_mode && (SW_OK != rv));
#endif

    return SW_OK;
}

sw_error_t
cmd_data_check_port_vlan_direction(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "all"))
    {
        *arg_val = FAL_PORT_VLAN_ALL;
    }
    else if (!strcasecmp(cmd_str, "ingress"))
    {
        *arg_val = FAL_PORT_VLAN_INGRESS;
    }
    else if (!strcasecmp(cmd_str, "egress"))
    {
        *arg_val = FAL_PORT_VLAN_EGRESS;
    }
    else
    {
        //dprintf("input error \n");
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_port_default_vid_en(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_port_default_vid_enable_t *pEntry = (fal_port_default_vid_enable_t *)val;

    memset(pEntry, 0, sizeof(fal_port_default_vid_enable_t));

    do
    {
        cmd = get_sub_cmd("default_ctag_vid_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_enable(cmd, &(pEntry->default_cvid_en), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("default_stag_vid_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_enable(cmd, &(pEntry->default_svid_en), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    return SW_OK;
}

sw_error_t
cmd_data_check_port_vlan_tag(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_port_vlan_tag_t *pEntry = (fal_port_vlan_tag_t *)val;
    a_uint32_t tmp = 0;

    memset(pEntry, 0, sizeof(fal_port_vlan_tag_t));

    /* get mask */
    do
    {
        cmd = get_sub_cmd("mask", "0x0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->mask), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get cvid */
    do
    {
        cmd = get_sub_cmd("default_ctag_vid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->cvid = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get svid */
    do
    {
        cmd = get_sub_cmd("default_stag_vid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->svid = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get cpri */
    do
    {
        cmd = get_sub_cmd("default_ctag_pri", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->cpri = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get spri */
    do
    {
        cmd = get_sub_cmd("default_stag_pri", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->spri = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get cdei */
    do
    {
        cmd = get_sub_cmd("default_ctag_dei", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->cdei = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get sdei */
    do
    {
        cmd = get_sub_cmd("default_stag_dei", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->sdei = (a_uint16_t)tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    return SW_OK;
}

sw_error_t
cmd_data_check_tag_propagation(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_vlantag_propagation_t *pEntry = (fal_vlantag_propagation_t *)val;

    memset(pEntry, 0, sizeof(fal_vlantag_propagation_t));

    /* get mask */
    do
    {
        cmd = get_sub_cmd("mask", "0x0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->mask), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get vid propagation */
    do
    {
        cmd = get_sub_cmd("vid_propagation_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_vlan_propagation(cmd, (a_uint32_t *) & (pEntry->vid_propagation), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get pri propagation */
    do
    {
        cmd = get_sub_cmd("pri_propagation_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_vlan_propagation(cmd, (a_uint32_t *) & (pEntry->pri_propagation), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get dei propagation */
    do
    {
        cmd = get_sub_cmd("dei_propagation_en", "disable");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_vlan_propagation(cmd, (a_uint32_t *) & (pEntry->dei_propagation), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    return SW_OK;
}

sw_error_t
cmd_data_check_egress_mode(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_vlantag_egress_mode_t *pEntry = (fal_vlantag_egress_mode_t *)val;

    memset(pEntry, 0, sizeof(fal_vlantag_egress_mode_t));

    /* get mask */
    do
    {
        cmd = get_sub_cmd("mask", "0x0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->mask), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get ctag mode */
    do
    {
        cmd = get_sub_cmd("ctag_egress_vlan_mode", "unmodified");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_egmode(cmd, (a_uint32_t *) & (pEntry->ctag_mode), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get stag mode */
    do
    {
        cmd = get_sub_cmd("stag_egress_vlan_mode", "unmodified");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_egmode(cmd, (a_uint32_t *) & (pEntry->stag_mode), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    return SW_OK;
}

sw_error_t
cmd_data_check_port_vlan_translation_adv_rule(char *info, void *val, a_uint32_t size)
{
	char *cmd = NULL;
	sw_error_t rv;
#if defined(APPE)
	a_uint32_t tmp;
#endif
	fal_vlan_trans_adv_rule_t *pEntry = (fal_vlan_trans_adv_rule_t *)val;

	memset(pEntry, 0, sizeof(fal_vlan_trans_adv_rule_t));

	do
	{
		cmd = get_sub_cmd("stagformat", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->s_tagged), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("svid_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;

		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->s_vid_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("svid", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;

		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(pEntry->s_vid), sizeof (a_uint32_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("spcp_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->s_pcp_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("spcp", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->s_pcp), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("sdei_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->s_dei_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("sdei", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->s_dei), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("ctagformat", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->c_tagged), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cvid_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;

		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->c_vid_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cvid", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;

		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(pEntry->c_vid), sizeof (a_uint32_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cpcp_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->c_pcp_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cpcp", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->c_pcp), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cdei_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->c_dei_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cdei", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->c_dei), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("frame_type_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->frmtype_enable),
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("frametype", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(pEntry->frmtype), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("protocol_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_enable),
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("protocol", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->protocol), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vsivalid", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->vsi_valid),
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vsi_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->vsi_enable),
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vsi", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(pEntry->vsi), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

#if defined(APPE)
	do
	{
		cmd = get_sub_cmd("vni_resv_enable", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &pEntry->vni_resv_enable,
					sizeof (a_bool_t));
		}
	} while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vni_resv_type", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_integer(cmd, &tmp, 0x1, 0x0);
			if (SW_OK == rv)
				pEntry->vni_resv_type = tmp;
		}
	} while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vni_resv", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;

		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &pEntry->vni_resv, sizeof (a_uint32_t));
		}

	} while (talk_mode && (SW_OK != rv));
#endif

	return SW_OK;
}

#if defined(APPE)
sw_error_t
cmd_data_check_srctype(char *cmdstr, a_uint8_t def, a_uint8_t *val, a_uint32_t size) {
	if (0 == cmdstr[0]) {
		*val = def;
	} else if (!strcasecmp(cmdstr, "vp")) {
		*val = 0;
	} else if (!strcasecmp(cmdstr, "l3_if")) {
		*val = 1;
	} else {
		return SW_BAD_VALUE;
	}
	return SW_OK;
}
#endif

sw_error_t
cmd_data_check_port_vlan_translation_adv_action(char *info, void *val, a_uint32_t size)
{
	char *cmd = NULL;
	sw_error_t rv;
	fal_vlan_trans_adv_action_t *pEntry = (fal_vlan_trans_adv_action_t *)val;

	memset(pEntry, 0, sizeof(fal_vlan_trans_adv_action_t));

	do
	{
		cmd = get_sub_cmd("swap_svid_cvid", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->swap_svid_cvid),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("svid_translation_cmd", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(pEntry->svid_xlt_cmd), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("svidtranslation", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->svid_xlt), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cvid_translation_cmd", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(pEntry->cvid_xlt_cmd), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cvidtranslation", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->cvid_xlt), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("swap_spcp_cpcp", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->swap_spcp_cpcp),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("spcp_translation_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->spcp_xlt_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("spcptranslation", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->spcp_xlt), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cpcp_translation_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->cpcp_xlt_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cpcptranslation", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->cpcp_xlt), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("swap_sdei_cdei", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->swap_sdei_cdei),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("sdei_translation_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->sdei_xlt_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("sdeitranslation", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->sdei_xlt), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cdei_translation_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->cdei_xlt_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("cdeitranslation", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->cdei_xlt), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("counter_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->counter_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("counter_id", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, (a_uint32_t *)&(pEntry->counter_id), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vsi_translation_en", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &(pEntry->vsi_xlt_enable),
					sizeof (a_bool_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vsitranslation", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd,
					(a_uint32_t *)&(pEntry->vsi_xlt), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

#if defined(APPE)
	do
	{
		cmd = get_sub_cmd("src_info_enable", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &pEntry->src_info_enable,
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("src_info_type", "vp");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_srctype(cmd, 0, &pEntry->src_info_type,
					sizeof(a_uint8_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("src_info", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;

		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &pEntry->src_info,
					sizeof(a_uint32_t));
		}

	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vni_resv_enable", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &pEntry->vni_resv_enable,
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("vni_resv", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;

		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;

		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &pEntry->vni_resv,
					sizeof(a_uint32_t));
		}

	}
	while (talk_mode && (SW_OK != rv));
#endif

	return SW_OK;
}
#endif
#ifdef APPE
sw_error_t
cmd_data_check_isol_ctrl(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_portvlan_isol_ctrl_t entry;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_portvlan_isol_ctrl_t));

	do {
		cmd = get_sub_cmd("isol_en", "n");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4)) {
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4)) {
			rv = SW_BAD_VALUE;
		}
		else {
			rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.enable),
					sizeof(a_bool_t));
			if (SW_OK != rv)
				rv = SW_BAD_VALUE;
		}
	} while(talk_mode && (SW_OK != rv));

	do {
		cmd = get_sub_cmd("group_id", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4)) {
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4)) {
			rv = SW_BAD_VALUE;
		} else {
			rv = cmd_data_check_uint8(cmd, &tmp, sizeof(tmp));
			if (SW_OK != rv)
				rv = SW_BAD_VALUE;
			else
				entry.group_id = tmp;
		}
	} while(talk_mode && (SW_OK != rv));

	*(fal_portvlan_isol_ctrl_t *)arg_val = entry;

	return SW_OK;
}
#endif
sw_error_t
cmd_data_check_vlan_propagation(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "disable"))
    {
        *arg_val = FAL_VLAN_PROPAGATION_DISABLE;
    }
    else if (!strcasecmp(cmd_str, "clone"))
    {
        *arg_val = FAL_VLAN_PROPAGATION_CLONE;
    }
    else if (!strcasecmp(cmd_str, "replace"))
    {
        *arg_val = FAL_VLAN_PROPAGATION_REPLACE;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

#ifndef IN_PORTVLAN_MINI
sw_error_t
cmd_data_check_vlan_translation(char *info, fal_vlan_trans_entry_t *val, a_uint32_t size)
{
    sw_error_t rv;
    fal_vlan_trans_entry_t entry;

    memset(&entry, 0, sizeof (fal_vlan_trans_entry_t));

    rv = __cmd_data_check_complex("ovid", "1",
                        "usage: the range is 0 -- 4095\n",
                        (param_check_t)cmd_data_check_uint32, &entry.o_vid,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("bi direction", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.bi_dir,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("forward direction", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.forward_dir,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("reverse direction", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.reverse_dir,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("svid ", "1",
                        "usage: the range is 0 -- 4095\n",
                        (param_check_t)cmd_data_check_uint32, &entry.s_vid,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("cvid ", "1",
                        "usage: the range is 0 -- 4095\n",
                        (param_check_t)cmd_data_check_uint32, &entry.c_vid,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("ovid_is_cvid", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.o_vid_is_cvid,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("svid_enable", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.s_vid_enable,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("cvid_enable", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.c_vid_enable,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("one_2_one_vlan", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.one_2_one_vlan,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    *val = entry;
    return SW_OK;
}
#endif


sw_error_t
cmd_data_check_qinq_mode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "ctag"))
    {
        *arg_val = FAL_QINQ_CTAG_MODE;
    }
    else if (!strcasecmp(cmd_str, "stag"))
    {
        *arg_val = FAL_QINQ_STAG_MODE;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_qinq_role(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "edge"))
    {
        *arg_val = FAL_QINQ_EDGE_PORT;
    }
    else if (!strcasecmp(cmd_str, "core"))
    {
        *arg_val = FAL_QINQ_CORE_PORT;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif

#ifdef IN_PORTCONTROL
sw_error_t
cmd_data_check_hdrmode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "noheader"))
    {
        *arg_val = FAL_NO_HEADER_EN;
    }
    else if (!strcasecmp(cmd_str, "onlymanagement"))
    {
        *arg_val = FAL_ONLY_MANAGE_FRAME_EN;
    }
    else if (!strcasecmp(cmd_str, "allframe"))
    {
        *arg_val = FAL_ALL_TYPE_FRAME_EN;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif
#ifdef IN_FDB
sw_error_t
cmd_data_check_fdboperation(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_fdb_op_t entry;

    memset(&entry, 0, sizeof (fal_fdb_op_t));

    rv = __cmd_data_check_boolean("port_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.port_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("fid_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.fid_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("multi_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.multicast_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    *(fal_fdb_op_t *) val = entry;
    return SW_OK;
}
#endif
#ifdef IN_PPPOE
sw_error_t
cmd_data_check_pppoe(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_pppoe_session_t entry;

    aos_mem_zero(&entry, sizeof (fal_pppoe_session_t));

    rv = __cmd_data_check_complex("entryid", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.entry_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("sessionid", "0",
                        "usage: the range is 0 -- 65535\n",
                        (param_check_t)cmd_data_check_uint32, &entry.session_id,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("multi_session", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.multi_session,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("uni_session", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.uni_session,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vrf_id", "0",
                        "usage: the range is 0 -- 7\n",
                        (param_check_t)cmd_data_check_uint32, &entry.vrf_id,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("port", "null",
                        "usage: input port number such as 1,3\n",
                        cmd_data_check_portmap, &entry.port_bitmap,
                        sizeof (fal_pbmp_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("l3_interface_index", "0",
                        "usage: the range is 0 -- 255\n",
                        cmd_data_check_uint32, &entry.l3_if_index,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("l3_interface_index_valid", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.l3_if_valid,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("smac_addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_macaddr, &(entry.smac_addr),
                        sizeof (fal_mac_addr_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("smac_valid", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.smac_valid,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

#if defined(APPE)
    rv = __cmd_data_check_complex("tl_l3_interface_index", "0",
		    "usage: the range is 0 -- 255\n",
		    cmd_data_check_uint32, &entry.tl_l3_if_index,
		    sizeof (a_uint32_t));
    if (rv)
	    return rv;

    rv = __cmd_data_check_boolean("tl_l3_interface_index_valid", "no",
		    "usage: <yes/no/y/n>\n",
		    cmd_data_check_confirm, A_FALSE, &entry.tl_l3_if_valid,
		    sizeof (a_bool_t));
    if (rv)
	    return rv;
#endif

    *(fal_pppoe_session_t*)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_pppoe_less(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_pppoe_session_t entry;

    aos_mem_zero(&entry, sizeof (fal_pppoe_session_t));

    rv = __cmd_data_check_complex("sessionid", "0",
                        "usage: the range is 0 -- 65535\n",
                        cmd_data_check_uint32, &entry.session_id,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    *(fal_pppoe_session_t*)val = entry;
    return SW_OK;
}
#endif

#if defined(IN_IP) || defined(IN_NAT)
sw_error_t
cmd_data_check_host_entry(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_host_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_host_entry_t));

    rv = __cmd_data_check_complex("entryid", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.entry_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entryflags", "0x1",
                        "usage: bitmap for host entry\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.flags),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entrystatus", "0",
                        "usage: bitmap for host entry\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.status),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    if ((FAL_IP_IP4_ADDR & (entry.flags)) ==  FAL_IP_IP4_ADDR|| 
		(FAL_IP_IP4_ADDR_MCAST & (entry.flags)) ==  FAL_IP_IP4_ADDR_MCAST)
    {
        rv = __cmd_data_check_complex("ip4 addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.ip4_addr),
                            4);
        if (rv)
            return rv;
    }
    else if ((FAL_IP_IP6_ADDR & (entry.flags)) == FAL_IP_IP6_ADDR || 
		(FAL_IP_IP6_ADDR_MCAST& (entry.flags)) == FAL_IP_IP6_ADDR_MCAST)
    {
        rv = __cmd_data_check_complex("ip6 addr", NULL,
                            "usage: the format is xxxx::xxxx \n",
                            (param_check_t)cmd_data_check_ip6addr, &(entry.ip6_addr),
                            16);
        if (rv)
            return rv;
    }

    rv = __cmd_data_check_complex("mac addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_macaddr, &(entry.mac_addr),
                        sizeof (fal_mac_addr_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("interface id", "0",
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_uint32, &(entry.intf_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("load_balance num", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.lb_num),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vrf id", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.vrf_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("port id", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.port_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("action", "forward",
                        "usage: <forward/drop/cpycpu/rdtcpu>\n",
                        (param_check_t)cmd_data_check_maccmd, &(entry.action),
                        sizeof (fal_fwd_cmd_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("mirror", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.mirror_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("counter", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.counter_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == entry.counter_en)
    {
        rv = __cmd_data_check_complex("counter id", "0",
                            "usage: integer\n",
                            (param_check_t)cmd_data_check_uint32, &(entry.counter_id),
                            sizeof (a_uint32_t));
        if (rv)
            return rv;
    }

    rv = __cmd_data_check_complex("dst info", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.dst_info),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("sync_toggle", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint8, &(entry.syn_toggle),
                        sizeof (a_uint8_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("lan_wan", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint8, &(entry.lan_wan),
                        sizeof (a_uint8_t));
    if (rv)
        return rv;

    if ((FAL_IP_IP4_ADDR_MCAST & (entry.flags)) == FAL_IP_IP4_ADDR_MCAST || 
		(FAL_IP_IP6_ADDR_MCAST& (entry.flags)) == FAL_IP_IP6_ADDR_MCAST) {
         rv = __cmd_data_check_complex("vsi", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint8, &(entry.mcast_info.vsi),
                        sizeof (a_uint8_t));
        if (rv)
            return rv;
        if ((FAL_IP_IP4_ADDR_MCAST & (entry.flags)) == FAL_IP_IP4_ADDR_MCAST) {
		rv = __cmd_data_check_complex("ip4 addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.mcast_info.sip4_addr),
                            4);
            if (rv)
                return rv;
        } else {
            rv = __cmd_data_check_complex("ip6 addr", NULL,
                            "usage: the format is xxxx::xxxx \n",
                            (param_check_t)cmd_data_check_ip6addr, &(entry.mcast_info.sip6_addr),
                            16);
            if (rv)
                return rv;
        }
    }

    

    *(fal_host_entry_t *)val = entry;
    return SW_OK;
}

#if !defined(IN_IP_MINI)
sw_error_t
cmd_data_check_arp_learn_mode(char *cmd_str, fal_arp_learn_mode_t * arg_val,
                              a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmd_str, "learnlocal"))
    {
        *arg_val = FAL_ARP_LEARN_LOCAL;
    }
    else if (!strcasecmp(cmd_str, "learnall"))
    {
        *arg_val = FAL_ARP_LEARN_ALL;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_ip_guard_mode(char *cmd_str, fal_source_guard_mode_t * arg_val, a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmd_str, "mac_ip"))
    {
        *arg_val = FAL_MAC_IP_GUARD;
    }
    else if (!strcasecmp(cmd_str, "mac_ip_port"))
    {
        *arg_val = FAL_MAC_IP_PORT_GUARD;
    }
    else if (!strcasecmp(cmd_str, "mac_ip_vlan"))
    {
        *arg_val = FAL_MAC_IP_VLAN_GUARD;
    }
    else if (!strcasecmp(cmd_str, "mac_ip_port_vlan"))
    {
        *arg_val = FAL_MAC_IP_PORT_VLAN_GUARD;
    }
    else if (!strcasecmp(cmd_str, "no_guard"))
    {
        *arg_val = FAL_NO_SOURCE_GUARD;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}



sw_error_t
cmd_data_check_nat_entry(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    a_uint32_t tmp = 0;
    fal_nat_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_nat_entry_t));

    rv = __cmd_data_check_complex("entryid", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.entry_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entryflags", "0",
                        "usage: bitmap for host entry\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.flags),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entrystatus", "0xf",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.status),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("select_idx", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.slct_idx),
                        sizeof (a_uint32_t));

	if (rv)
        return rv;

	rv = __cmd_data_check_complex("vrf_id", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.vrf_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("src addr", "0.0.0.0",
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.src_addr),
                        4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("trans addr", "0.0.0.0",
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.trans_addr),
                        4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("port num", "0",
                        "usage: 0- 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp, sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.port_num = tmp & 0xffff;


    rv = __cmd_data_check_complex("port range", "0",
                        "usage: 0- 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp, sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.port_range = tmp & 0xffff;

    rv = __cmd_data_check_complex("action", "forward",
                        "usage: <forward/drop/cpycpu/rdtcpu>\n",
                        (param_check_t)cmd_data_check_maccmd, &entry.action,
                        sizeof (fal_fwd_cmd_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("mirror", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.mirror_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("counter", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.counter_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == entry.counter_en)
    {
        rv = __cmd_data_check_complex("counter id", "0",
                            "usage: integer\n",
                            (param_check_t)cmd_data_check_uint32, &(entry.counter_id),
                            sizeof (a_uint32_t));
        if (rv)
            return rv;
    }

    *(fal_nat_entry_t *)val = entry;
    return SW_OK;
}



sw_error_t
cmd_data_check_napt_entry(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    a_uint32_t tmp = 0;
    fal_napt_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_napt_entry_t));

    rv = __cmd_data_check_complex("entryid", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &entry.entry_id,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entryflags", "0",
                        "usage: bitmap for host entry\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.flags),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entrystatus", "0xf",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.status),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vrf_id", "0x0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.vrf_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("flow_cookie", "0x0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.flow_cookie),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("load_balance", "0x0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.load_balance),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("src addr", "0.0.0.0",
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.src_addr),
                        4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("dst addr", "0.0.0.0",
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.dst_addr),
                        4);
    if (rv)
        return rv;

    if (FAL_NAT_ENTRY_TRANS_IPADDR_INDEX & (entry.flags))
    {
        rv = __cmd_data_check_complex("trans addr index", "0",
                            "usage: integer\n",
                            (param_check_t)cmd_data_check_uint32, &(entry.trans_addr),
                            sizeof (a_uint32_t));
        if (rv)
            return rv;
    }
    else
    {
        rv = __cmd_data_check_complex("trans addr", "0.0.0.0",
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.trans_addr),
                            4);
        if (rv)
            return rv;
    }

    rv = __cmd_data_check_complex("src port", "0",
                        "usage: 0- 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.src_port = tmp & 0xffff;

    rv = __cmd_data_check_complex("dst port", "0",
                        "usage: 0- 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.dst_port = tmp & 0xffff;

    rv = __cmd_data_check_complex("trans port", "0",
                        "usage: 0- 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.trans_port = tmp & 0xffff;

    rv = __cmd_data_check_complex("action", "forward",
                        "usage: <forward/drop/cpycpu/rdtcpu>\n",
                        (param_check_t)cmd_data_check_maccmd, &(entry.action),
                        sizeof (fal_fwd_cmd_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("mirror", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.mirror_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;


    rv = __cmd_data_check_boolean("counter", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.counter_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;


    if (A_TRUE == entry.counter_en)
    {
        rv = __cmd_data_check_complex("counter id", "0",
                            "usage: integer\n",
                            (param_check_t)cmd_data_check_uint32, &(entry.counter_id),
                            sizeof (a_uint32_t));
        if (rv)
            return rv;
    }

	rv = __cmd_data_check_boolean("priority", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.priority_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == entry.priority_en)
    {
        rv = __cmd_data_check_complex("priority value", "0",
                            "usage: integer\n",
                            (param_check_t)cmd_data_check_uint32, &(entry.priority_val),
                            sizeof (a_uint32_t));
        if (rv)
            return rv;
    }

    *(fal_napt_entry_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_flow_entry(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    a_uint32_t tmp = 0;
    fal_napt_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_napt_entry_t));

    rv = __cmd_data_check_complex("entryid", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &entry.entry_id,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entryflags", "0",
                        "usage: bitmap for host entry\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.flags),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("entrystatus", "0xf",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.status),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vrf_id", "0x0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.vrf_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("flow_cookie", "0x0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.flow_cookie),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("load_balance", "0x0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.load_balance),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("src addr", "0.0.0.0",
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.src_addr),
                        4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("dst addr", "0.0.0.0",
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.dst_addr),
                        4);
    if (rv)
        return rv;


    rv = __cmd_data_check_complex("src port", "0",
                        "usage: 0- 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.src_port = tmp & 0xffff;

    rv = __cmd_data_check_complex("dst port", "0",
                        "usage: 0- 65535\n",
                        (param_check_t)cmd_data_check_uint16, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.dst_port = tmp & 0xffff;


    rv = __cmd_data_check_complex("action", "forward",
                        "usage: <forward/drop/cpycpu/rdtcpu>\n",
                        (param_check_t)cmd_data_check_maccmd, &(entry.action),
                        sizeof (fal_fwd_cmd_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("mirror", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.mirror_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;


    rv = __cmd_data_check_boolean("counter", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.counter_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;


    if (A_TRUE == entry.counter_en)
    {
        rv = __cmd_data_check_complex("counter id", "0",
                            "usage: integer\n",
                            (param_check_t)cmd_data_check_uint32, &(entry.counter_id),
                            sizeof (a_uint32_t));
        if (rv)
            return rv;
    }

	rv = __cmd_data_check_boolean("priority", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.priority_en,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == entry.priority_en)
    {
        rv = __cmd_data_check_complex("priority value", "0",
                            "usage: integer\n",
                            (param_check_t)cmd_data_check_uint32, &(entry.priority_val),
                            sizeof (a_uint32_t));
        if (rv)
            return rv;
    }

    *(fal_napt_entry_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_napt_mode(char *cmd_str, fal_napt_mode_t * arg_val,
                         a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (!strcasecmp(cmd_str, "fullcone"))
    {
        *arg_val = FAL_NAPT_FULL_CONE;
    }
    else if (!strcasecmp(cmd_str, "strictcone"))
    {
        *arg_val = FAL_NAPT_STRICT_CONE;
    }
    else if (!strcasecmp(cmd_str, "portstrict"))
    {
        *arg_val = FAL_NAPT_PORT_STRICT;
    }
    else if (!strcasecmp(cmd_str, "synmatric"))
    {
        *arg_val = FAL_NAPT_SYNMETRIC;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_intf_mac_entry(char *cmd_str, void * val, a_uint32_t size)
{
    a_uint32_t tmp = 0;
    sw_error_t rv;
    fal_intf_mac_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_intf_mac_entry_t));

    rv = __cmd_data_check_complex("entryid", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.entry_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vrf id", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.vrf_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vid low", NULL,
                        "usage: low vlan id\n",
                        (param_check_t)cmd_data_check_uint16, &tmp, sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.vid_low = tmp & 0xffff;

    rv = __cmd_data_check_complex("vid high", NULL,
                        "usage: high vlan id\n",
                        (param_check_t)cmd_data_check_uint16, &tmp, sizeof (a_uint32_t));
    if (rv)
        return rv;
    entry.vid_high = tmp & 0xffff;

    rv = __cmd_data_check_complex("mac addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_macaddr, &(entry.mac_addr),
                        sizeof (fal_mac_addr_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("ip4_route", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.ip4_route,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("ip6_route", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &entry.ip6_route,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    *(fal_intf_mac_entry_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_pub_addr_entry(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_nat_pub_addr_t entry;

    aos_mem_zero(&entry, sizeof (fal_nat_pub_addr_t));

    rv = __cmd_data_check_complex("entryid", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.entry_id),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("pub addr", NULL,
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.pub_addr),
                        4);
    if (rv)
        return rv;

    *(fal_nat_pub_addr_t *)val = entry;
    return SW_OK;
}
#endif
#endif

#ifdef IN_RATE
sw_error_t
cmd_data_check_egress_shaper(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    a_bool_t bool = 0;
    fal_egress_shaper_t entry;

    aos_mem_zero(&entry, sizeof (fal_egress_shaper_t));

    rv = __cmd_data_check_boolean("bytebased", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &bool,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == bool)
    {
        entry.meter_unit = FAL_BYTE_BASED;
    }
    else
    {
        entry.meter_unit = FAL_FRAME_BASED;
    }

    rv = __cmd_data_check_complex("cir", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.cir),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("cbs", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.cbs),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("eir", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.eir),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("ebs", "0",
                        "usage: integer\n",
                        (param_check_t)cmd_data_check_uint32, &(entry.ebs),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    *(fal_egress_shaper_t *)val = entry;
    return SW_OK;
}


sw_error_t
cmd_data_check_acl_policer(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    a_bool_t bool = A_FALSE;
    fal_acl_policer_t entry;

    aos_mem_zero(&entry, sizeof (fal_acl_policer_t));

    rv = __cmd_data_check_boolean("counter_mode", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.counter_mode),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("bytebased", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &bool,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == bool)
    {
        entry.meter_unit = FAL_BYTE_BASED;
    }
    else
    {
        entry.meter_unit = FAL_FRAME_BASED;
    }

    if (A_TRUE == entry.counter_mode)
    {
        *(fal_acl_policer_t *)val = entry;
        return SW_OK;
    }

    rv = __cmd_data_check_boolean("couple_flag", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.couple_flag),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("color_aware", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.color_mode),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("deficit_flag", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.deficit_en),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("cir", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.cir),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("cbs", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.cbs),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("eir", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.eir),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("ebs", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.ebs),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("meter_interval", "1ms",
                        "usage: the format <100us/1ms/10ms/100ms>\n",
                        (param_check_t)cmd_data_check_policer_timesslot, &(entry.meter_interval),
                        sizeof (fal_rate_mt_t));
    if (rv)
        return rv;

    *(fal_acl_policer_t *)val = entry;
    return SW_OK;
}


sw_error_t
cmd_data_check_policer_timesslot(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strncasecmp(cmd_str, "100us", 5))
        *arg_val = FAL_RATE_MI_100US;
    else if (!strncasecmp(cmd_str, "1ms", 3))
        *arg_val = FAL_RATE_MI_1MS;
    else if (!strncasecmp(cmd_str, "10ms", 4))
        *arg_val = FAL_RATE_MI_10MS;
    else if (!strncasecmp(cmd_str, "100ms", 5))
        *arg_val = FAL_RATE_MI_100MS;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_port_policer(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    a_bool_t bool = 0;
    fal_port_policer_t entry;

    aos_mem_zero(&entry, sizeof (fal_port_policer_t));

    rv = __cmd_data_check_boolean("combine_enable", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.combine_mode),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("bytebased", "yes",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_TRUE, &bool,
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    if (A_TRUE == bool)
    {
        entry.meter_unit = FAL_BYTE_BASED;
    }
    else
    {
        entry.meter_unit = FAL_FRAME_BASED;
    }

    rv = __cmd_data_check_boolean("couple_flag", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.couple_flag),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("color_aware", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.color_mode),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("deficit_flag", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.deficit_en),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("c_enable", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.c_enable),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("cir", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.cir),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("cbs", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.cbs),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("c_rate_flag", "0xfe",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.c_rate_flag),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("c_meter_interval", "1ms",
                        "usage: the format <100us/1ms/10ms/100ms>\n",
                        (param_check_t)cmd_data_check_policer_timesslot, &(entry.c_meter_interval),
                        sizeof (fal_rate_mt_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_boolean("e_enable", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.e_enable),
                        sizeof (a_bool_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("eir", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.eir),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("ebs", "0",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.ebs),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("e_rate_flag", "0xfe",
                        "usage: integer\n",
                        cmd_data_check_uint32, &(entry.e_rate_flag),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("e_meter_interval", "1ms",
                        "usage: the format <100us/1ms/10ms/100ms>\n",
                        (param_check_t)cmd_data_check_policer_timesslot, &(entry.e_meter_interval),
                        sizeof (fal_rate_mt_t));
    if (rv)
        return rv;

    *(fal_port_policer_t *)val = entry;
    return SW_OK;
}
#endif
#if 0
sw_error_t
cmd_data_check_mac_mode(char *cmd_str, fal_interface_mac_mode_t * arg_val,
                        a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmd_str[0])
    {
        *arg_val = FAL_MAC_MODE_RGMII;
    }
    else if (!strcasecmp(cmd_str, "rgmii"))
    {
        *arg_val = FAL_MAC_MODE_RGMII;
    }
    else if (!strcasecmp(cmd_str, "rmii"))
    {
        *arg_val = FAL_MAC_MODE_RMII;
    }
    else if (!strcasecmp(cmd_str, "gmii"))
    {
        *arg_val = FAL_MAC_MODE_GMII;
    }
    else if (!strcasecmp(cmd_str, "mii"))
    {
        *arg_val = FAL_MAC_MODE_MII;
    }
    else if (!strcasecmp(cmd_str, "sgmii"))
    {
        *arg_val = FAL_MAC_MODE_SGMII;
    }
    else if (!strcasecmp(cmd_str, "fiber"))
    {
        *arg_val = FAL_MAC_MODE_FIBER;
    }
    else if (!strcasecmp(cmd_str, "default"))
    {
        *arg_val = FAL_MAC_MODE_DEFAULT;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_clock_mode(char *cmd_str, fal_interface_clock_mode_t * arg_val,
                          a_uint32_t size)
{
    if (NULL == cmd_str)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmd_str[0])
    {
        *arg_val = FAL_INTERFACE_CLOCK_MAC_MODE;
    }
    if (!strcasecmp(cmd_str, "mac"))
    {
        *arg_val = FAL_INTERFACE_CLOCK_MAC_MODE;
    }
    else if (!strcasecmp(cmd_str, "phy"))
    {
        *arg_val = FAL_INTERFACE_CLOCK_PHY_MODE;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif
#ifdef IN_FDB
#ifndef IN_FDB_MINI
sw_error_t
cmd_data_check_fdb_smode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "ivl"))
        *arg_val = INVALID_VLAN_IVL;
    else if (!strcasecmp(cmd_str, "svl"))
        *arg_val = INVALID_VLAN_SVL;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif
#endif
#ifdef IN_SEC
sw_error_t
cmd_data_check_sec_mac(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "resv_vid"))
        *arg_val = FAL_NORM_MAC_RESV_VID_CMD;
    else if (!strcasecmp(cmd_str, "invalid_src_addr"))
        *arg_val = FAL_NORM_MAC_INVALID_SRC_ADDR_CMD;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_sec_ip(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "invalid_ver"))
        *arg_val = FAL_NORM_IP_INVALID_VER_CMD;
    else if (!strcasecmp(cmd_str, "same_addr"))
        *arg_val = FAL_NROM_IP_SAME_ADDR_CMD;
    else if (!strcasecmp(cmd_str, "ttl_change_status"))
        *arg_val = FAL_NROM_IP_TTL_CHANGE_STATUS;
    else if (!strcasecmp(cmd_str, "ttl_val"))
        *arg_val = FAL_NROM_IP_TTL_VALUE;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_sec_ip4(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "invalid_hl"))
        *arg_val = FAL_NROM_IP4_INVALID_HL_CMD;
    else if (!strcasecmp(cmd_str, "hdr_opts"))
        *arg_val = FAL_NROM_IP4_HDR_OPTIONS_CMD;
    else if (!strcasecmp(cmd_str, "invalid_df"))
        *arg_val = FAL_NROM_IP4_INVALID_DF_CMD;
    else if (!strcasecmp(cmd_str, "frag_offset_min_len"))
        *arg_val = FAL_NROM_IP4_FRAG_OFFSET_MIN_LEN_CMD;
    else if (!strcasecmp(cmd_str, "frag_offset_min_size"))
        *arg_val = FAL_NROM_IP4_FRAG_OFFSET_MIN_SIZE;
    else if (!strcasecmp(cmd_str, "frag_offset_max_len"))
        *arg_val = FAL_NROM_IP4_FRAG_OFFSET_MAX_LEN_CMD;
    else if (!strcasecmp(cmd_str, "invalid_frag_offset"))
        *arg_val = FAL_NROM_IP4_INVALID_FRAG_OFFSET_CMD;
    else if (!strcasecmp(cmd_str, "invalid_sip"))
        *arg_val = FAL_NROM_IP4_INVALID_SIP_CMD;
    else if (!strcasecmp(cmd_str, "invalid_dip"))
        *arg_val = FAL_NROM_IP4_INVALID_DIP_CMD;
    else if (!strcasecmp(cmd_str, "invalid_chksum"))
        *arg_val = FAL_NROM_IP4_INVALID_CHKSUM_CMD;
    else if (!strcasecmp(cmd_str, "invalid_pl"))
        *arg_val = FAL_NROM_IP4_INVALID_PL_CMD;
    else if (!strcasecmp(cmd_str, "df_clear_status"))
        *arg_val = FAL_NROM_IP4_DF_CLEAR_STATUS;
    else if (!strcasecmp(cmd_str, "ipid_random_status"))
        *arg_val = FAL_NROM_IP4_IPID_RANDOM_STATUS;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_sec_ip6(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "invalid_dip"))
        *arg_val = FAL_NROM_IP6_INVALID_DIP_CMD;
    else if (!strcasecmp(cmd_str, "invalid_sip"))
        *arg_val = FAL_NROM_IP6_INVALID_SIP_CMD;
    else if (!strcasecmp(cmd_str, "invalid_pl"))
        *arg_val = FAL_NROM_IP6_INVALID_PL_CMD;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_sec_tcp(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "blat"))
        *arg_val = FAL_NROM_TCP_BLAT_CMD;
    else if (!strcasecmp(cmd_str, "invalid_hl"))
        *arg_val = FAL_NROM_TCP_INVALID_HL_CMD;
    else if (!strcasecmp(cmd_str, "min_hdr_size"))
        *arg_val = FAL_NROM_TCP_MIN_HDR_SIZE;
    else if (!strcasecmp(cmd_str, "invalid_syn"))
        *arg_val = FAL_NROM_TCP_INVALID_SYN_CMD;
    else if (!strcasecmp(cmd_str, "su_block"))
        *arg_val = FAL_NROM_TCP_SU_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "sp_block"))
        *arg_val = FAL_NROM_TCP_SP_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "sap_block"))
        *arg_val = FAL_NROM_TCP_SAP_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "xmas_scan"))
        *arg_val = FAL_NROM_TCP_XMAS_SCAN_CMD;
    else if (!strcasecmp(cmd_str, "null_scan"))
        *arg_val = FAL_NROM_TCP_NULL_SCAN_CMD;
    else if (!strcasecmp(cmd_str, "sr_block"))
        *arg_val = FAL_NROM_TCP_SR_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "sf_block"))
        *arg_val = FAL_NROM_TCP_SF_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "sar_block"))
        *arg_val = FAL_NROM_TCP_SAR_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "rst_scan"))
        *arg_val = FAL_NROM_TCP_RST_SCAN_CMD;
    else if (!strcasecmp(cmd_str, "rst_with_data"))
        *arg_val = FAL_NROM_TCP_RST_WITH_DATA_CMD;
    else if (!strcasecmp(cmd_str, "fa_block"))
        *arg_val = FAL_NROM_TCP_FA_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "pa_block"))
        *arg_val = FAL_NROM_TCP_PA_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "ua_block"))
        *arg_val = FAL_NROM_TCP_UA_BLOCK_CMD;
    else if (!strcasecmp(cmd_str, "invalid_chksum"))
        *arg_val = FAL_NROM_TCP_INVALID_CHKSUM_CMD;
    else if (!strcasecmp(cmd_str, "invalid_urgptr"))
        *arg_val = FAL_NROM_TCP_INVALID_URGPTR_CMD;
    else if (!strcasecmp(cmd_str, "invalid_opts"))
        *arg_val = FAL_NROM_TCP_INVALID_OPTIONS_CMD;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}


sw_error_t
cmd_data_check_sec_udp(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "blat"))
        *arg_val = FAL_NROM_UDP_BLAT_CMD;
    else if (!strcasecmp(cmd_str, "invalid_len"))
        *arg_val = FAL_NROM_UDP_INVALID_LEN_CMD;
    else if (!strcasecmp(cmd_str, "invalid_chksum"))
        *arg_val = FAL_NROM_UDP_INVALID_CHKSUM_CMD;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_sec_icmp4(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "ping_pl_exceed"))
        *arg_val = FAL_NROM_ICMP4_PING_PL_EXCEED_CMD;
    else if (!strcasecmp(cmd_str, "ping_frag"))
        *arg_val = FAL_NROM_ICMP4_PING_FRAG_CMD;
    else if (!strcasecmp(cmd_str, "ping_max_pl"))
        *arg_val = FAL_NROM_ICMP4_PING_MAX_PL_VALUE;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_sec_icmp6(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "ping_pl_exceed"))
        *arg_val = FAL_NROM_ICMP6_PING_PL_EXCEED_CMD;
    else if (!strcasecmp(cmd_str, "ping_frag"))
        *arg_val = FAL_NROM_ICMP6_PING_FRAG_CMD;
    else if (!strcasecmp(cmd_str, "ping_max_pl"))
        *arg_val = FAL_NROM_ICMP6_PING_MAX_PL_VALUE;
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#ifdef HPPE
sw_error_t
cmd_data_check_l3_parser(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_uint32_t tmp;
    sw_error_t rv;
    fal_l3_excep_parser_ctrl entry;

    aos_mem_zero(&entry, sizeof (fal_l3_excep_parser_ctrl));

    do
    {
        cmd = get_sub_cmd("small_ip4ttl", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (a_uint32_t));
            if (!rv)
                entry.small_ip4ttl = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("small_ip6hoplimit", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (a_uint32_t));
            if (!rv)
                entry.small_ip6hoplimit = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_l3_excep_parser_ctrl *)val = entry;
    return SW_OK;

}

sw_error_t
cmd_data_check_l4_parser(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_uint32_t tmp;
    sw_error_t rv;
    fal_l4_excep_parser_ctrl entry;

    aos_mem_zero(&entry, sizeof (fal_l4_excep_parser_ctrl));

    do
    {
        cmd = get_sub_cmd("tcp_flags0", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[0] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask0", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[0] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags1", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[1] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask1", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[1] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags2", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[2] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask2", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[2] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags3", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[3] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask3", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[3] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags4", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[4] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask4", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[4] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags5", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[5] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask5", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[5]= tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags6", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[6] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask6", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[6] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags7", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags[7] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcp_flags_mask7", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (tmp));
	entry.tcp_flags_mask[7] = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));


    *(fal_l4_excep_parser_ctrl *)val = entry;
    return SW_OK;

}

sw_error_t
cmd_data_check_exp_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_l3_excep_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_l3_excep_ctrl_t));

    do
    {
        cmd = get_sub_cmd("excep_cmd", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.cmd),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("deacclr_en", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.deacclr_en),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("l3route_only_en", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.l3route_only_en),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("l2fwd_only_en", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.l2fwd_only_en),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("l2flow_en", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.l2flow_en),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("l3flow_en", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.l3flow_en),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("multicast_en", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.multicast_en),
                                       sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#ifdef APPE
    do
    {
        cmd = get_sub_cmd("l2flow_type", "flow_aware");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
           return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
           rv = SW_BAD_VALUE;
        }
        else
        {
           rv = cmd_data_check_attr("flow_excep_type", cmd,
                                     &entry.l2flow_type, sizeof(entry.l2flow_type));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("l3flow_type", "flow_aware");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
           return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
           rv = SW_BAD_VALUE;
        }
        else
        {
           rv = cmd_data_check_attr("flow_excep_type", cmd,
                                     &entry.l3flow_type, sizeof(entry.l3flow_type));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif
    *(fal_l3_excep_ctrl_t *)val = entry;
    return SW_OK;
}
#endif
sw_error_t
cmd_data_check_l2_exp_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_l2_excep_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_l2_excep_ctrl_t));

    do
    {
        cmd = get_sub_cmd("excep_cmd", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.cmd),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_l2_excep_ctrl_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_exp_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    int i = 0;
    char Idstr[20];
    fal_tunnel_excep_ctrl_t entry;


    aos_mem_zero(&entry, sizeof (fal_tunnel_excep_ctrl_t));

    do
    {
        cmd = get_sub_cmd("excep_cmd", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.cmd),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("deacclr_en", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.deacclr_en),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    for(i = 0; i < EXP_PROFILE_ID_MAX; i++) {
        memset(Idstr, 0, sizeof(Idstr));
        snprintf(Idstr, sizeof(Idstr), "profile%d_en", i);

        do
        {
            cmd = get_sub_cmd(Idstr, "0");
            SW_RTN_ON_NULL_PARAM(cmd);

            if (!strncasecmp(cmd, "quit", 4))
            {
                return SW_BAD_VALUE;
            }
            else if (!strncasecmp(cmd, "help", 4))
            {
                rv = SW_BAD_VALUE;
            }
            else
            {
                rv = cmd_data_check_uint8(cmd, &(entry.profile_exp_en[i]),
                                           sizeof (a_uint8_t));
            }
        }
        while (talk_mode && (SW_OK != rv));
    }

    *(fal_tunnel_excep_ctrl_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_flags_parser(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_bool_t tmpbool;
    a_uint32_t tmpdata = 0;
    sw_error_t rv;
    fal_tunnel_flags_excep_parser_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_tunnel_flags_excep_parser_ctrl_t));

    do
    {
        cmd = get_sub_cmd("entry_valid", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &entry.entry_valid,
                                           sizeof (a_bool_t));
       }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("equal", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &tmpbool, sizeof(tmpbool));

            entry.comp_mode = !tmpbool;
       }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tunnel_header_type", "vxlan");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_attr("tunnel_overlay_type", cmd,
                                                   &tmpdata, sizeof(tmpdata));
            entry.hdr_type = tmpdata & 0x3;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("flags", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmpdata,
                                       sizeof (tmpdata));
            entry.flags = tmpdata&0xffff;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mask", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmpdata,
                                       sizeof (tmpdata));
            entry.mask = tmpdata&0xffff;
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_tunnel_flags_excep_parser_ctrl_t *)val = entry;
    return SW_OK;
}
#endif
#ifdef IN_COSMAP
#ifndef IN_COSMAP_MINI
sw_error_t
cmd_data_check_remark_entry(char *info, void *val, a_uint32_t size)
{
    sw_error_t rv;
    fal_egress_remark_table_t *pEntry = (fal_egress_remark_table_t *)val;
    a_uint32_t tmp = 0;

    memset(pEntry, 0, sizeof(fal_egress_remark_table_t));

    /* get remark_dscp */
    rv = __cmd_data_check_complex("remark dscp", "enable",
                        "usage: <enable/disable>\n",
                        (param_check_t)cmd_data_check_enable, &(pEntry->remark_dscp),
                        sizeof(a_bool_t));
    if (rv)
        return rv;

    /* get remark_up */
    rv = __cmd_data_check_complex("remark up", "enable",
                        "usage: <enable/disable>\n",
                        (param_check_t)cmd_data_check_enable, &(pEntry->remark_up),
                        sizeof(a_bool_t));
    if (rv)
        return rv;

    /* get remark_dei */
    rv = __cmd_data_check_complex("remark dei", "enable",
                        "usage: <enable/disable>\n",
                        (param_check_t)cmd_data_check_enable, &(pEntry->remark_dei),
                        sizeof(a_bool_t));
    if (rv)
        return rv;

    /* get g_dscp */
    rv = __cmd_data_check_range("green dscp", NULL,
                        "usage: the range is 0 -- 63\n",
                        cmd_data_check_integer, &tmp, 63, 0);
    if (rv)
        return rv;
    pEntry->g_dscp = tmp;

    /* get y_dscp */
    rv = __cmd_data_check_range("yellow dscp", NULL,
                        "usage: the range is 0 -- 63\n",
                        cmd_data_check_integer, &tmp, 63, 0);
    if (rv)
        return rv;
    pEntry->y_dscp = tmp;

    /* get g_up */
    rv = __cmd_data_check_range("green up", NULL,
                        "usage: the range is 0 -- 63\n",
                        cmd_data_check_integer, &tmp, 7, 0);
    if (rv)
        return rv;
    pEntry->g_up = tmp;

    /* get y_up */
    rv = __cmd_data_check_range("yellow up", NULL,
                        "usage: the range is 0 -- 63\n",
                        cmd_data_check_integer, &tmp, 7, 0);
    if (rv)
        return rv;
    pEntry->y_up = tmp;

    /* get g_dei */
    rv = __cmd_data_check_range("green dei", NULL,
                        "usage: the range is 0 -- 1\n",
                        cmd_data_check_integer, &tmp, 1, 0);
    if (rv)
        return rv;
    pEntry->g_dei = tmp;

    /* get y_dei */
    rv = __cmd_data_check_range("yellow dei", NULL,
                        "usage: the range is 0 -- 1\n",
                        cmd_data_check_integer, &tmp, 1, 0);
    if (rv)
        return rv;
    pEntry->y_dei = tmp;

/*
    dprintf("remark_dscp=%d, remark_up=%d, g_dscp=%d, y_dscp=%d\n",
            pEntry->remark_dscp,
            pEntry->remark_up,
            pEntry->g_dscp,
            pEntry->y_dscp);

    *(fal_egress_remark_table_t *) val = entry;
*/
    return SW_OK;
}
#endif
#endif
#ifdef IN_IP
#if !defined(IN_IP_MINI)
sw_error_t
cmd_data_check_default_route_entry(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_default_route_t entry;

    aos_mem_zero(&entry, sizeof(entry));

    do
    {
        cmd = get_sub_cmd("entry valid", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.valid), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("vrf id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &(entry.vrf_id), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ip version", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &(entry.ip_version), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("route type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &(entry.droute_type), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("index", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &(entry.index), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_default_route_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_host_route_entry(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_host_route_t entry;

    aos_mem_zero(&entry, sizeof (fal_intf_mac_entry_t));

    do
    {
        cmd = get_sub_cmd("entry valid", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.valid), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("vrf id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &(entry.vrf_id), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ip version", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &(entry.ip_version), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    if (entry.ip_version == 0) /*IPv4*/
    {
        rv = __cmd_data_check_complex("ip4 addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.route_addr.ip4_addr),
                            4);
        if (rv)
            return rv;
    }
    else if (entry.ip_version == 1) /*IPv6*/
    {
        rv = __cmd_data_check_complex("ip6 addr", NULL,
                            "usage: the format is xxxx::xxxx \n",
                            (param_check_t)cmd_data_check_ip6addr, &(entry.route_addr.ip6_addr),
                            16);
        if (rv)
            return rv;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    do
    {
        cmd = get_sub_cmd("prefix_length", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &(entry.prefix_length), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_host_route_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ip4_rfs_entry(char *cmd_str, void * val, a_uint32_t size)
{
	a_uint32_t tmp = 0;
	sw_error_t rv;
	fal_ip4_rfs_t entry;

	aos_mem_zero(&entry, sizeof (fal_ip4_rfs_t));

	rv = __cmd_data_check_complex("mac addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_macaddr, &(entry.mac_addr),
                        sizeof (fal_mac_addr_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("ip4 addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.ip4_addr),
                            4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vid", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.vid),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("loadbalance", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;
    
    entry.load_balance = tmp;
    *(fal_ip4_rfs_t *)val = entry;
    
    return SW_OK;
}
#endif
#endif
#if 0
sw_error_t
cmd_data_check_fdb_rfs(char *cmd_str, void * val, a_uint32_t size)
{
	char *cmd;
	a_uint32_t tmp;
	sw_error_t rv;
	fal_fdb_rfs_t entry;

	aos_mem_zero(&entry, sizeof (fal_fdb_rfs_t));

	rv = __cmd_data_check_complex("mac addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        cmd_data_check_macaddr, &(entry.addr),
                        sizeof (fal_mac_addr_t));
    if (rv)
        return rv;

	rv = __cmd_data_check_complex("fid", NULL,
                            "usage: the format is xx\n",
                            cmd_data_check_uint32, &tmp,
                            sizeof (a_uint32_t));
    if (rv)
        return rv;
	entry.fid = tmp;

	rv = __cmd_data_check_complex("loadbalance", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

	entry.load_balance = tmp;

	*(fal_fdb_rfs_t *)val = entry;
	return SW_OK;
}
#endif
#if defined(IN_IP) || defined(IN_NAT)
#if !defined(IN_IP_MINI)
sw_error_t
cmd_data_check_flow_cookie(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_flow_cookie_t entry;

    aos_mem_zero(&entry, sizeof (fal_flow_cookie_t));

    rv = __cmd_data_check_complex("proto", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.proto),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("src addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.src_addr),
                            4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("dst addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.dst_addr),
                            4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("src port", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.src_port),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("dst port", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.dst_port),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("flow cookie", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.flow_cookie),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;


    *(fal_flow_cookie_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_flow_rfs(char *cmd_str, void * val, a_uint32_t size)
{
    a_uint32_t tmp = 0;
    sw_error_t rv;
    fal_flow_rfs_t entry;

    aos_mem_zero(&entry, sizeof (fal_flow_cookie_t));

    rv = __cmd_data_check_complex("proto", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.proto),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("src addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.src_addr),
                            4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("dst addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.dst_addr),
                            4);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("src port", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.src_port),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("dst port", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.dst_port),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("flow rfs", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    entry.load_balance = tmp;


    *(fal_flow_rfs_t *)val = entry;
    return SW_OK;
}
#endif
#endif
#ifdef IN_IP
sw_error_t
cmd_data_check_ip_global(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_uint32_t tmp;
    sw_error_t rv;
    fal_ip_global_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_ip_global_cfg_t));

    do
    {
        cmd = get_sub_cmd("mru_fail_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.mru_fail_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mru_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.mru_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mtu_fail_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.mtu_fail_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mtu_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.mtu_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mtu_nonfrag_fail_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.mtu_nonfrag_fail_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mtu_nonfrag_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.mtu_df_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("prefix_bc_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.prefix_bc_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("prefix_bc_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.prefix_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("icmp_rdt_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.icmp_rdt_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("icmp_rdt_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.icmp_rdt_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("hash_mode_0", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (a_uint32_t));
            if (!rv)
                entry.hash_mode_0 = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("hash_mode_1", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (a_uint32_t));
            if (!rv)
                entry.hash_mode_1 = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do {
	    cmd = get_sub_cmd("route_fail_no_eth_action", "forward");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_maccmd(cmd, &(entry.rt_fail_no_eth_action),
				    sizeof (fal_fwd_cmd_t));
	    }
    } while (talk_mode && (SW_OK != rv));
#endif

    *(fal_ip_global_cfg_t *)val = entry;
    return SW_OK;

}

sw_error_t
cmd_data_check_ip_mcmode(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_mc_mode_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_mc_mode_cfg_t));

    do
    {
        cmd = get_sub_cmd("ipv4_mc_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.l2_ipv4_mc_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv4_igmpv3_mode", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.l2_ipv4_mc_mode), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_mc_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.l2_ipv6_mc_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_mldv2_mode", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.l2_ipv6_mc_mode), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_mc_mode_cfg_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ip_portmac(char *cmd_str, void * val, a_uint32_t size)
{
	char *cmd;
    sw_error_t rv;
    fal_macaddr_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_macaddr_entry_t));

    do
    {
        cmd = get_sub_cmd("entry_valid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.valid), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    rv = __cmd_data_check_complex("mac_addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_macaddr, &(entry.mac_addr),
                        sizeof (fal_mac_addr_t));
    if (rv)
		return rv;

    *(fal_macaddr_entry_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ip_pub(char *cmd_str, void * val, a_uint32_t size)
{
    sw_error_t rv;
    fal_ip_pub_addr_t entry;

    aos_mem_zero(&entry, sizeof (fal_ip_pub_addr_t));

    rv = __cmd_data_check_complex(" pub_ip_addr", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.pub_ip_addr),
                            4);
    if (rv)
        return rv; 

    *(fal_ip_pub_addr_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ip_sg(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_sg_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_sg_cfg_t));

    do
    {
        cmd = get_sub_cmd("ipv4_sg_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_sg_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv4_sg_violation_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ipv4_sg_vio_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv4_sg_port_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_sg_port_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv4_sg_svlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_sg_svlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv4_sg_cvlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_sg_cvlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv4_src_unk_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ipv4_src_unk_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_sg_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv6_sg_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_sg_violation_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ipv6_sg_vio_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_sg_port_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv6_sg_port_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_sg_svlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv6_sg_svlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_sg_cvlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv6_sg_cvlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_src_unk_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ipv6_src_unk_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_sg_cfg_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_nexthop(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ip_nexthop_t entry;

    aos_mem_zero(&entry, sizeof (fal_ip_nexthop_t));

    do
    {
        cmd = get_sub_cmd("type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.type), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    if (entry.type == 0) {
		do
    		{
		        cmd = get_sub_cmd("vsi", "0");
		        SW_RTN_ON_NULL_PARAM(cmd);

		        if (!strncasecmp(cmd, "quit", 4))
		        {
		            return SW_BAD_VALUE;
		        }
		        else if (!strncasecmp(cmd, "help", 4))
		        {
		            rv = SW_BAD_VALUE;
		        }
		        else
		        {
		            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.vsi), sizeof (a_uint8_t));
		        }
    		}
    		while (talk_mode && (SW_OK != rv));
    } else {
    		do
    		{
		        cmd = get_sub_cmd("port", "0");
		        SW_RTN_ON_NULL_PARAM(cmd);

		        if (!strncasecmp(cmd, "quit", 4))
		        {
		            return SW_BAD_VALUE;
		        }
		        else if (!strncasecmp(cmd, "help", 4))
		        {
		            rv = SW_BAD_VALUE;
		        }
		        else
		        {
		            rv = cmd_data_check_uint8(cmd, &(entry.port), sizeof (a_uint8_t));
		        }
    		}
    		while (talk_mode && (SW_OK != rv));
    }

    do
    {
        cmd = get_sub_cmd("if_index", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.if_index), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ip_to_me_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.ip_to_me_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("pub_ip_index", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.pub_ip_index), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("stag_fmt", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.stag_fmt), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("svid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.svid), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ctag_fmt", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.ctag_fmt), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cvid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.cvid), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    rv = __cmd_data_check_complex("mac_addr", NULL,
    		"usage: the format is xx-xx-xx-xx-xx-xx \n",
    		(param_check_t)cmd_data_check_macaddr, &(entry.mac_addr),
    		sizeof (fal_mac_addr_t));
    if (rv)
    	return rv;

    rv = __cmd_data_check_complex(" dnat_ip", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.dnat_ip),
                            4);
    if (rv)
        return rv;   
	

    *(fal_ip_nexthop_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_vsi_intf(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_intf_id_t entry;

    aos_mem_zero(&entry, sizeof (fal_intf_id_t));

    do
    {
        cmd = get_sub_cmd("l3_if_valid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.l3_if_valid), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("l3_if_index", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.l3_if_index), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_intf_id_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_intf(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_intf_entry_t entry;
#if defined(APPE)
    a_uint32_t tmp = 0;
#endif

    aos_mem_zero(&entry, sizeof (fal_intf_entry_t));

    do
    {
        cmd = get_sub_cmd("mru", "0x5dc");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.mru), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mtu", "0x5dc");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.mtu), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ttl_dec_bypass_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.ttl_dec_bypass_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv4_uc_route_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_uc_route_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ipv6_uc_route_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv6_uc_route_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("icmp_trigger_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.icmp_trigger_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ttl_exceed_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ttl_exceed_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ttl_exceed_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.ttl_exceed_deacclr_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("mac_addr_bitmap", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.mac_addr_bitmap), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    rv = __cmd_data_check_complex("mac_addr", NULL,
    		"usage: the format is xx-xx-xx-xx-xx-xx \n",
    		(param_check_t)cmd_data_check_macaddr, &(entry.mac_addr),
    		sizeof (fal_mac_addr_t));
    if (rv)
    	return rv;

#if defined(APPE)
    do {
	    cmd = get_sub_cmd("dmac_check_en", "no");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_confirm(cmd, A_FALSE,
				    &(entry.dmac_check_en), sizeof(a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("ipv6_mru", "0x5dc");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));
		    if (SW_OK == rv)
			    entry.ip6_mru = tmp;
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("ipv6_mtu", "0x5dc");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));
		    if (SW_OK == rv)
			    entry.ip6_mtu = tmp;
	    }
    } while (talk_mode && (SW_OK != rv));

    cmd_data_check_element("udp_zero_csum_action", "forward",
		    "usage: action: forward, drop, recalc_mapt, rdtcpu\n",
		    cmd_data_check_attr, ("udp_zero_csum_action", cmd,
			    &tmp, sizeof(tmp)));
    entry.udp_zero_csum_action = tmp;

    do {
	    cmd = get_sub_cmd("vpn_id", "0");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_uint16(cmd, &tmp, sizeof(a_uint32_t));
		    if (SW_OK == rv)
			    entry.vpn_id = tmp;
	    }
    } while (talk_mode && (SW_OK != rv));
#endif

    *(fal_intf_entry_t *)val = entry;
    return SW_OK;
}

#if !defined(IN_IP_MINI)
sw_error_t
cmd_data_check_network_route(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_network_route_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_network_route_entry_t));

    do
    {
        cmd = get_sub_cmd("type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.type), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    if (entry.type == 0) /*IPv4*/
    {
        rv = __cmd_data_check_complex("ip4_addr", NULL,
                        "usage: the format is xx.xx.xx.xx \n",
                        (param_check_t)cmd_data_check_ip4addr, &(entry.route_addr.ip4_addr),
                        4);
	 if (rv)
	 	return rv;
    }
    else if (entry.type == 1) /*IPv6*/
    {
	  rv = __cmd_data_check_complex("ip6_addr", NULL,
                        "usage: the format is xxxx::xxxx \n",
                        (param_check_t)cmd_data_check_ip6addr, &(entry.route_addr.ip6_addr),
                        16);
	 if (rv)
	 	return rv;

    }
    else
    {
        return SW_BAD_VALUE;
    }

     if (entry.type == 0) /*IPv4*/
    {
        rv = __cmd_data_check_complex("ip4_addr_mask", NULL,
                            "usage: the format is xx.xx.xx.xx \n",
                            (param_check_t)cmd_data_check_ip4addr, &(entry.route_addr_mask.ip4_addr_mask),
                            4);
        if (rv)
            return rv;
    }
    else if (entry.type == 1) /*IPv6*/
    {
        rv = __cmd_data_check_complex("ip6_addr_mask", NULL,
                            "usage: the format is xxxx::xxxx \n",
                            (param_check_t)cmd_data_check_ip6addr, &(entry.route_addr_mask.ip6_addr_mask),
                            16);
        if (rv)
            return rv;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    do
    {
        cmd = get_sub_cmd("action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dst_info", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.dst_info), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("lan_wan", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.lan_wan), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_network_route_entry_t *)val = entry;
    return SW_OK;
}
#endif

sw_error_t
cmd_data_check_arp_sg(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_arp_sg_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_arp_sg_cfg_t));

    do
    {
        cmd = get_sub_cmd("arp_sg_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_arp_sg_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("arp_sg_violation_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ipv4_arp_sg_vio_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("arp_sg_port_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_arp_sg_port_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("arp_sg_svlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_arp_sg_svlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("arp_sg_cvlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ipv4_arp_sg_cvlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("arp_sg_unk_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ipv4_arp_src_unk_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("nd_sg_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ip_nd_sg_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("nd_sg_violation_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ip_nd_sg_vio_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("nd_sg_port_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ip_nd_sg_port_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("nd_sg_svlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ip_nd_sg_svlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("nd_sg_cvlan_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ip_nd_sg_cvlan_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("nd_sg_unk_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.ip_nd_src_unk_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_arp_sg_cfg_t *)val = entry;
    return SW_OK;
}

#if !defined(IN_IP_MINI)
sw_error_t
cmd_data_check_ip6_rfs_entry(char *cmd_str, void * val, a_uint32_t size)
{
    a_uint32_t tmp = 0;
    sw_error_t rv;
    fal_ip6_rfs_t entry;

    aos_mem_zero(&entry, sizeof (fal_ip4_rfs_t));

    rv = __cmd_data_check_complex("mac addr", NULL,
                        "usage: the format is xx-xx-xx-xx-xx-xx \n",
                        (param_check_t)cmd_data_check_macaddr, &(entry.mac_addr),
                        sizeof (fal_mac_addr_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("ip6 addr", NULL,
                            "usage: the format is xxxx::xxxx \n",
                            (param_check_t)cmd_data_check_ip6addr, &(entry.ip6_addr),
                            16);
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("vid", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &(entry.vid),
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    rv = __cmd_data_check_complex("loadbalance", "0",
                        "usage: the format is xx \n",
                        cmd_data_check_uint32, &tmp,
                        sizeof (a_uint32_t));
    if (rv)
        return rv;

    entry.load_balance = tmp;
    *(fal_ip6_rfs_t *)val = entry;

    return SW_OK;
}
#endif
#endif
#ifdef IN_VSI
sw_error_t
cmd_data_check_newadr_lrn(char *cmd_str, void * val, a_uint32_t size)
{
	char *cmd;
	fal_vsi_newaddr_lrn_t entry;

	aos_mem_zero(&entry, sizeof (fal_vsi_newaddr_lrn_t));

	cmd_data_check_element("learnstatus_en", "enable", "usage: enable/disable\n",
			cmd_data_check_enable, (cmd, &(entry.lrn_en), sizeof(entry.lrn_en)));

	cmd_data_check_element("learnaction", "forward", "usage: forward/drop/cpycpu/rdtcpu\n",
			cmd_data_check_maccmd, (cmd, &(entry.action), sizeof(entry.action)));

	*(fal_vsi_newaddr_lrn_t *)val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_stamove(char *cmd_str, void * val, a_uint32_t size)
{
	char *cmd;
	fal_vsi_stamove_t entry;

	aos_mem_zero(&entry, sizeof (fal_vsi_stamove_t));

	cmd_data_check_element("stationmove_en", "enable", "usage: enable/disable\n",
			cmd_data_check_enable, (cmd, &(entry.stamove_en), sizeof(entry.stamove_en)));

	cmd_data_check_element("stationmove_action", "forward", "usage: forward/drop/cpycpu/rdtcpu\n",
			cmd_data_check_maccmd, (cmd, &(entry.action), sizeof(entry.action)));

	*(fal_vsi_stamove_t *)val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_vsi_member(char *cmd_str, void * val, a_uint32_t size)
{
	sw_error_t rv;
	fal_vsi_member_t entry;

	aos_mem_zero(&entry, sizeof (fal_vsi_member_t));

	rv = __cmd_data_check_complex("membership", 0,
                        "usage: Bit0-port0 Bit1-port1 ....\n",
                        cmd_data_check_pbmp, &(entry.member_ports),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("unknown_unicast_membership", 0,
                        "usage: Bit0-port0 Bit1-port1 ....\n",
                        cmd_data_check_pbmp, &(entry.uuc_ports),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("unknown_multicast_membership", 0,
                        "usage: Bit0-port0 Bit1-port1 ....\n",
                        cmd_data_check_pbmp, &(entry.umc_ports),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("broadcast_membership", 0,
                        "usage: Bit0-port0 Bit1-port1 ....\n",
                        cmd_data_check_pbmp, &(entry.bc_ports),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;
#if defined (APPE)
	rv = __cmd_data_check_complex("vports_bitmap(port64-port95)", 0,
                        "usage: Bit0-port64 Bit1-port65 ....\n",
                        cmd_data_check_pbmp, &(entry.member_vports[0]),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("vports_bitmap(port96-port127)", 0,
                        "usage: Bit0-port96 Bit1-port97 ....\n",
                        cmd_data_check_pbmp, &(entry.member_vports[1]),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("vports_bitmap(por128-port159)", 0,
                        "usage: Bit0-port128 Bit1-port129 ....\n",
                        cmd_data_check_pbmp, &(entry.member_vports[2]),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("vports_bitmap(port160-port191)", 0,
                        "usage: Bit0-port160 Bit1-port161 ....\n",
                        cmd_data_check_pbmp, &(entry.member_vports[3]),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("vports_bitmap(port192-port223)", 0,
                        "usage: Bit0-port192 Bit1-port193 ...\n",
                        cmd_data_check_pbmp, &(entry.member_vports[4]),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("vports_bitmap(port224-port255)", 0,
                        "usage: Bit0-port224 Bit1-port225 ....\n",
                        cmd_data_check_pbmp, &(entry.member_vports[5]),
                        sizeof (a_uint32_t));
	if (rv)
		return rv;
#endif
	*(fal_vsi_member_t *)val = entry;
	return SW_OK;
}

#if defined (APPE)
sw_error_t
cmd_data_check_vsi_bridge_vsi(char *cmd_str, void *arg_val, a_uint32_t size)
{
    sw_error_t rv;
    fal_vsi_bridge_vsi_t bridge_vsi;

    aos_mem_zero(&bridge_vsi, sizeof (fal_vsi_bridge_vsi_t));

    rv = __cmd_data_check_complex("bridge_vsi_en", "disable",
                        "usage: bridge vsi enable or disable\n",
                        cmd_data_check_enable, &(bridge_vsi.bridge_vsi_enable),
                        sizeof (a_bool_t));
    SW_RTN_ON_ERROR(rv);

    rv = __cmd_data_check_complex("bridge_vsi_id", "0",
                        "usage: bridge vsi id is 0~63\n",
                        cmd_data_check_uint32, &(bridge_vsi.bridge_vsi_id),
                        sizeof (a_uint32_t));

    *(fal_vsi_bridge_vsi_t *)arg_val = bridge_vsi;

    return rv;

}

sw_error_t
cmd_data_check_vsi_invalidvsi_ctrl(char *cmd_str, void *arg_val, a_uint32_t size)
{
    char *cmd;

    fal_vsi_invalidvsi_ctrl_t invalidvsi_ctrl;

    aos_mem_zero(&invalidvsi_ctrl, sizeof (fal_vsi_invalidvsi_ctrl_t));

    cmd_data_check_element("dest_en", "disable",
                        "usage:dest_en,enable/disable\n",
                        cmd_data_check_enable, (cmd, &(invalidvsi_ctrl.dest_en),
                        sizeof (invalidvsi_ctrl.dest_en)));

    cmd_data_check_element("dest_info_type", "port_id",
                        "usage:dest_info_type:port_id\n",
                        cmd_data_check_attr, ("dest_info_type", cmd,
                        &(invalidvsi_ctrl.dest_info.dest_info_type),
                        sizeof(invalidvsi_ctrl.dest_info.dest_info_type)));

    cmd_data_check_element("dest_info_value", "0",
                        "usage:dest_info_value:port_id\n",
                        cmd_data_check_uint32, (cmd, &(invalidvsi_ctrl.dest_info.dest_info_value),
                        sizeof (invalidvsi_ctrl.dest_info.dest_info_value)));

    *(fal_vsi_invalidvsi_ctrl_t *)arg_val = invalidvsi_ctrl;

    return SW_OK;
}
#endif
#endif

#ifdef IN_FLOW
sw_error_t
cmd_data_check_flow_global(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_uint32_t tmp;
    sw_error_t rv;
    fal_flow_global_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_flow_global_cfg_t));

    do
    {
        cmd = get_sub_cmd("src_intf_check_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.src_if_check_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("src_intf_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.src_if_check_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("service_loop_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.service_loop_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("service_loop_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.service_loop_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("service_loop_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.service_loop_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("flow_deacclr_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.flow_deacclr_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("sync_mismatch_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.sync_mismatch_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("sync_mismatch_deacclr_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.sync_mismatch_deacclr_en),
                                       sizeof (a_bool_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("hash_mode_0", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (a_uint32_t));
            if (!rv)
                 entry.hash_mode_0 = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("hash_mode_1", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp,
                                       sizeof (a_uint32_t));
            if (!rv)
                entry.hash_mode_1 = tmp;
        }

    }
    while (talk_mode && (SW_OK != rv));

#if defined(CPPE) || defined(APPE)
    do
    {
	    cmd = get_sub_cmd("flow_mismatch_copy_escape_en", "no");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.flow_mismatch_copy_escape_en),
				    sizeof (a_bool_t));
	    }

    }
    while (talk_mode && (SW_OK != rv));
#endif

#if defined(APPE)
    do {
	    cmd = get_sub_cmd("ptmu_fail_action", "forward");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_maccmd(cmd, &(entry.ptmu_fail_action),
				    sizeof (fal_fwd_cmd_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("ptmu_fail_deacclr_en", "no");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_confirm(cmd, A_FALSE,
				    &(entry.ptmu_fail_deacclr_en),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("ptmu_fail_df_action", "forward");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_maccmd(cmd, &(entry.ptmu_fail_df_action),
				    sizeof (fal_fwd_cmd_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("ptmu_fail_df_deacclr_en", "no");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_confirm(cmd, A_FALSE,
				    &(entry.ptmu_fail_df_deacclr_en),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("l2_vpn_en", "no");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_confirm(cmd, A_FALSE,
				    &(entry.l2_vpn_en),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("l3_vpn_en", "no");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_confirm(cmd, A_FALSE,
				    &(entry.l3_vpn_en),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));
#endif

    *(fal_flow_global_cfg_t *)val = entry;
    return SW_OK;

}

sw_error_t
cmd_data_check_flow(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_flow_entry_t entry;

    aos_mem_zero(&entry, sizeof (fal_flow_entry_t));

    do
    {
        cmd = get_sub_cmd("entry_id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.entry_id), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("entry_type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.entry_type), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("host_addr_type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.host_addr_type), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("host_addr_index", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.host_addr_index), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("protocol", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.protocol), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("age", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.age), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("src_intf_valid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.src_intf_valid), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("src_intf_index", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.src_intf_index), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("fwd_type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.fwd_type), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    
    do
    {
        cmd = get_sub_cmd("snat_nexthop", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.snat_nexthop), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("snat_srcport", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.snat_srcport), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dnat_nexthop", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.dnat_nexthop), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dnat_dstport", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.dnat_dstport), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("route_nexthop", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.route_nexthop), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("port_valid", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &(entry.port_valid), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("route_port", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.route_port), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("bridge_port", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.bridge_port), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("deacclr", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.deacclr_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("copy tocpu", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.copy_tocpu_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("syn toggle", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.syn_toggle), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("pri profile", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.pri_profile), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("sevice code", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.sevice_code), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ip type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.ip_type), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("src port", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.src_port), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dst port", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.dst_port), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    if (entry.entry_type & FAL_FLOW_IP4_5TUPLE_ADDR || entry.entry_type & FAL_FLOW_IP4_3TUPLE_ADDR) {
        rv = __cmd_data_check_complex("ip addr", NULL,
		"usage: the format is xx.xx.xx.xx \n",
		(param_check_t)cmd_data_check_ip4addr, &(entry.flow_ip.ipv4),
		4);
        if (rv)
		return rv;
		
    } else if (entry.entry_type & FAL_FLOW_IP6_5TUPLE_ADDR || entry.entry_type & FAL_FLOW_IP6_3TUPLE_ADDR) {
        rv = __cmd_data_check_complex("ip addr", NULL,
		"usage: the format is xxxx::xxxx \n",
		(param_check_t)cmd_data_check_ip6addr, &(entry.flow_ip.ipv6),
		16);
        if (rv)
		return rv;
    }

    do
    {
        cmd = get_sub_cmd("tree id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.tree_id), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do {
	    cmd = get_sub_cmd("pmtu_check_l3", "yes");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.pmtu_check_l3), sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("pmtu", "0x5dc");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_uint32(cmd, &(entry.pmtu), sizeof (a_uint32_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("vpn_id", "0");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_uint32(cmd, &(entry.vpn_id), sizeof (a_uint32_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("bridge_vlan_format_valid", "yes");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.vlan_fmt_valid),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("bridge_svlan_format", "yes");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.svlan_fmt),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("bridge_cvlan_format", "yes");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4))
	    {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4))
	    {
		    rv = SW_BAD_VALUE;
	    }
	    else
	    {
		    rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.cvlan_fmt),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("wifi_qos_en", "no");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.wifi_qos_en),
				    sizeof (a_bool_t));
	    }
    } while (talk_mode && (SW_OK != rv));

    do {
	    cmd = get_sub_cmd("wifi_qos", "0");
	    SW_RTN_ON_NULL_PARAM(cmd);

	    if (!strncasecmp(cmd, "quit", 4)) {
		    return SW_BAD_VALUE;
	    }
	    else if (!strncasecmp(cmd, "help", 4)) {
		    rv = SW_BAD_VALUE;
	    }
	    else {
		    rv = cmd_data_check_uint32(cmd, &(entry.wifi_qos), sizeof(a_uint32_t));
	    }
    } while (talk_mode && (SW_OK != rv));
#endif

    *(fal_flow_entry_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_flow_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_uint32_t tmp;
    sw_error_t rv;
    fal_flow_mgmt_t entry;

    aos_mem_zero(&entry, sizeof (fal_flow_mgmt_t));

    do
    {
        cmd = get_sub_cmd("miss_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, (fal_fwd_cmd_t *)&(entry.miss_action),
                                       sizeof (fal_fwd_cmd_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("frag_bypass_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.frag_bypass_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tcpspec_bypass_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.tcp_spec_bypass_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("all_bypass_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.all_bypass_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("key_sel", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, &tmp, sizeof (a_uint32_t));
            if (!rv)
                entry.key_sel = tmp;
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_flow_mgmt_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_flow_age(char *cmd_str, void * val, a_uint32_t size)
{
	char *cmd;
    a_uint32_t tmp;
    sw_error_t rv;
    fal_flow_age_timer_t entry;

    aos_mem_zero(&entry, sizeof (fal_flow_age_timer_t));

    do
    {
        cmd = get_sub_cmd("age_time", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof (a_uint32_t));
            if (!rv)
                entry.age_time = tmp;
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("age_unit", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof (a_uint32_t));
            if (!rv)
                entry.unit = tmp;
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_flow_age_timer_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_flow_host(char *cmd_str, void * val, a_uint32_t size)
{
        fal_flow_host_entry_t *flow_host = (fal_flow_host_entry_t *)val;
        fal_flow_entry_t *flow_entry = &(flow_host->flow_entry);
#ifdef IN_IP
        fal_host_entry_t *host_entry = &(flow_host->host_entry);
#endif

        cmd_data_check_flow(cmd_str, flow_entry, size);
#ifdef IN_IP
        cmd_data_check_host_entry(cmd_str, host_entry, size);
#endif
	return SW_OK;
}
#endif

#ifdef IN_BM
sw_error_t
cmd_data_check_bm_static_thresh(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_bm_static_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_bm_static_cfg_t));

    do
    {
        cmd = get_sub_cmd("max_thresh", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.max_thresh),
                                       sizeof (a_uint16_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.resume_off),
                                       sizeof (a_uint16_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_bm_static_cfg_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_bm_dynamic_thresh(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_bm_dynamic_cfg_t entry;

    aos_mem_zero(&entry, sizeof (fal_bm_dynamic_cfg_t));

    do
    {
        cmd = get_sub_cmd("weight", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.weight),
                                       sizeof (a_uint8_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("shared_ceiling", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.shared_ceiling),
                                       sizeof (a_uint16_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.resume_off),
                                       sizeof (a_uint16_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("resume_min_thresh", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.resume_min_thresh),
                                       sizeof (a_uint16_t));
        }

    }
    while (talk_mode && (SW_OK != rv));

    *(fal_bm_dynamic_cfg_t *)val = entry;
    return SW_OK;
}
#endif

#ifdef IN_QM
sw_error_t
cmd_data_check_u_qmap(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ucast_queue_dest_t entry;

    aos_mem_zero(&entry, sizeof (fal_ucast_queue_dest_t));

    do
    {
        cmd = get_sub_cmd("src_profile", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.src_profile), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("service_code_en", "no");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.service_code_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("service_code", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.service_code), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cpu_code_en", "no");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.cpu_code_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cpu_code", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.cpu_code), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("dst_port", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.dst_port), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_ucast_queue_dest_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ac_static_thresh(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ac_static_threshold_t entry;

    aos_mem_zero(&entry, sizeof (fal_ac_static_threshold_t));

    do
    {
        cmd = get_sub_cmd("color_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.color_enable), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("wred_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.wred_enable), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("green_max", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.green_max), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("green_min_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.green_min_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yel_max_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.yel_max_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yel_min_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.yel_min_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_max_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.red_max_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_min_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.red_min_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("green_resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.green_resume_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yel_resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.yel_resume_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.red_resume_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
	
    *(fal_ac_static_threshold_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ac_dynamic_thresh(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    a_uint32_t tmp;
    sw_error_t rv;
    fal_ac_dynamic_threshold_t entry;

    aos_mem_zero(&entry, sizeof (fal_ac_dynamic_threshold_t));

    do
    {
        cmd = get_sub_cmd("color_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.color_enable), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("wred_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.wred_enable), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("shared_weight", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint8(cmd, (a_uint32_t *)&(entry.shared_weight), sizeof (a_uint8_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("green_min_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.green_min_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yel_max_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.yel_max_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yel_min_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.yel_min_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_max_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.red_max_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_min_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.red_min_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("green_resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.green_resume_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yel_resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.yel_resume_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_resume_off", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.red_resume_off), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ceiling", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, &tmp, sizeof (a_uint32_t));
            if (!rv)
                entry.ceiling = tmp;
        }
    }
    while (talk_mode && (SW_OK != rv));
	
    *(fal_ac_dynamic_threshold_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ac_group_buff(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ac_group_buffer_t entry;

    do
    {
        cmd = get_sub_cmd("prealloc_buffer", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.prealloc_buffer), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("total_buffer", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint16(cmd, (a_uint32_t *)&(entry.total_buffer), sizeof (a_uint16_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_ac_group_buffer_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ac_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ac_ctrl_t entry;

    do
    {
        cmd = get_sub_cmd("ac_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ac_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ac_fc_en", "yes");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_TRUE, &(entry.ac_fc_en), sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_ac_ctrl_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_ac_obj(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ac_obj_t entry;

    do
    {
        cmd = get_sub_cmd("obj_type", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.type), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("obj_id", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.obj_id), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_ac_obj_t *)val = entry;
    return SW_OK;
}

#endif

#ifdef IN_SHAPER

sw_error_t
cmd_data_check_port_shaper_token_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_shaper_token_number_t entry;

    aos_mem_zero(&entry, sizeof (fal_shaper_token_number_t));

    do
    {
        cmd = get_sub_cmd("ctoken_negative_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.c_token_number_negative_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ctoken_number", "0-0x3FFFFFFF");
	SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.c_token_number), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_shaper_token_number_t *)val = entry;
    return SW_OK;
}


sw_error_t
cmd_data_check_shaper_token_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_shaper_token_number_t entry;

    aos_mem_zero(&entry, sizeof (fal_shaper_token_number_t));

    do
    {
        cmd = get_sub_cmd("ctoken_negative_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.c_token_number_negative_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("ctoken_number", "0-0x3FFFFFFF");
	SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.c_token_number), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("etoken_negative_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.e_token_number_negative_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("etoken_number", "0-0x3FFFFFFF");
	SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.e_token_number), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_shaper_token_number_t *)val = entry;
    return SW_OK;
}


sw_error_t
cmd_data_check_port_shaper_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_shaper_config_t entry;

    aos_mem_zero(&entry, sizeof (fal_shaper_config_t));

    do
    {
        cmd = get_sub_cmd("meter_unit", "0-1");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.meter_unit), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cshaper_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.c_shaper_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cir", "0");
	SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cir), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cbs", "0");
	SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cbs), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

     do
    {
        cmd = get_sub_cmd("shaper_frame_mode", "0-2");
	SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.shaper_frame_mode), sizeof (fal_shaper_frame_mode_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_shaper_config_t *)val = entry;
    return SW_OK;
}


sw_error_t
cmd_data_check_shaper_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_shaper_config_t entry;

    aos_mem_zero(&entry, sizeof (fal_shaper_config_t));
#if defined(APPE)
    cmd_data_check_element("meter_type", "rfc",
                        "usage:meter_type:rfc/mef10_3, etc\n",
                        cmd_data_check_attr, ("shaper_meter_type", cmd,
                        &(entry.meter_type), sizeof(entry.meter_type)));
#endif

    do
    {
        cmd = get_sub_cmd("couple_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.couple_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("meter_unit", "0-1");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.meter_unit), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cshaper_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.c_shaper_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cir", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cir), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("cir_max", "0");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cir_max), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("cbs", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cbs), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("eshaper_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.e_shaper_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("eir", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.eir), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("eir_max", "0");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.eir_max), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("ebs", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.ebs), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("next_ptr", "0");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.next_ptr), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("grp_end", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.grp_end),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("grp_couple_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.grp_couple_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("shaper_frame_mode", "0-2");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.shaper_frame_mode), sizeof (fal_shaper_frame_mode_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_shaper_config_t *)val = entry;
    return SW_OK;
}

#if defined(APPE)
sw_error_t
cmd_data_check_queue_shaper_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_shaper_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_shaper_ctrl_t));

    do
    {
        cmd = get_sub_cmd("head", "0-299");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.head), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tail", "0-299");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.tail), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_shaper_ctrl_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_flow_shaper_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_shaper_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_shaper_ctrl_t));

    do
    {
        cmd = get_sub_cmd("head", "0-63");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.head), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tail", "0-63");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.tail), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_shaper_ctrl_t *)val = entry;
    return SW_OK;
}
#endif
#endif

#ifdef IN_POLICER
sw_error_t
cmd_data_check_port_policer_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_policer_config_t entry;

    aos_mem_zero(&entry, sizeof (fal_policer_config_t));

#if defined(APPE)
    cmd_data_check_element("meter_type", "rfc",
                        "usage:meter_type:rfc/mef10_3, etc\n",
                        cmd_data_check_attr, ("policer_meter_type", cmd,
                        &(entry.meter_type), sizeof(entry.meter_type)));
#endif

    do
    {
        cmd = get_sub_cmd("meter_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.meter_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("vp_policer_index", "0-511");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.vp_meter_index), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("couple_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.couple_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("color_mode", "0-1");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.color_mode), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("frame_type", "0-0x1f");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.frame_type), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("meter_mode", "0-1");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.meter_mode), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("meter_unit", "0-1");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.meter_unit), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cir", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cir), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("cir_max", "0");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cir_max), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("cbs", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cbs), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("eir", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.eir), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("eir_max", "0");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.eir_max), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("ebs", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.ebs), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("next_ptr", "0");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.next_ptr), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("grp_end", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.grp_end),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("grp_couple_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.grp_couple_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    *(fal_policer_config_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_acl_policer_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_policer_config_t entry;

    aos_mem_zero(&entry, sizeof (fal_policer_config_t));

#if defined(APPE)
    cmd_data_check_element("meter_type", "rfc",
                        "usage:meter_type:rfc/mef10_3, etc\n",
                        cmd_data_check_attr, ("policer_meter_type", cmd,
                        &(entry.meter_type), sizeof(entry.meter_type)));
#endif

    do
    {
        cmd = get_sub_cmd("meter_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.meter_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("couple_enable", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.couple_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("color_mode", "0-1");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.color_mode), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("meter_mode", "0-1");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.meter_mode), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("meter_unit", "0-1");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.meter_unit), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("cir", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cir), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
	do
	{
		cmd = get_sub_cmd("cir_max", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(entry.cir_max), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("cbs", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.cbs), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("eir", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.eir), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
	do
	{
		cmd = get_sub_cmd("eir_max", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(entry.eir_max), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("ebs", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.ebs), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
	do
	{
		cmd = get_sub_cmd("next_ptr", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &(entry.next_ptr), sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("grp_end", "no");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.grp_end),
				sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("grp_couple_enable", "no");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
	    }
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.grp_couple_en),
				sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));
#endif

    *(fal_policer_config_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_policer_cmd_config(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_policer_action_t entry;

    aos_mem_zero(&entry, sizeof (fal_policer_action_t));

    do
    {
        cmd = get_sub_cmd("yellow_priority_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.yellow_priority_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yellow_drop_priority_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.yellow_drop_priority_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("yellow_pcp_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.yellow_pcp_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yellow_dei_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.yellow_dei_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("yellow_dscp_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.yellow_dscp_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yellow_remap", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.yellow_remap_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("yellow_priority", "0-15");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.yellow_priority), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yellow_drop_priority", "0-3");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.yellow_drop_priority), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yellow_pcp", "0-7");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.yellow_pcp), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("yellow_dei", "0-1");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.yellow_dei), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("yellow_dscp", "0-63");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.yellow_dscp), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif
       do
    {
        cmd = get_sub_cmd("red_action", "drop");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(entry.red_action), sizeof(a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_priority_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.red_priority_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_drop_priority_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {

            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.red_drop_priority_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_pcp_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.red_pcp_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_dei_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.red_dei_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("red_dscp_remark", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.red_dscp_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
    do
    {
        cmd = get_sub_cmd("red_remap", "no");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(entry.red_remap_en),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    do
    {
        cmd = get_sub_cmd("red_priority", "0-15");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.red_priority), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_drop_priority", "0-3");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.red_drop_priority), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_pcp", "0-7");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.red_pcp), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("red_dei", "0-1");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.red_dei), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

#if defined(APPE)
    do
    {
        cmd = get_sub_cmd("red_dscp", "0-63");
        SW_RTN_ON_NULL_PARAM(cmd);
        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.red_dscp), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif

    *(fal_policer_action_t *)val = entry;
    return SW_OK;
}

#if defined(APPE)
sw_error_t
cmd_data_check_policer_remap(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_policer_remap_t entry;

    aos_mem_zero(&entry, sizeof (fal_policer_remap_t));

    do
    {
        cmd = get_sub_cmd("remap_dscp", "0-63");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.dscp), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("remap_pcp", "0-7");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.pcp), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    do
    {
        cmd = get_sub_cmd("remap_dei", "0-1");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.dei), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_policer_remap_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_policer_priority(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_policer_priority_t entry;

    aos_mem_zero(&entry, sizeof (fal_policer_priority_t));

    do
    {
        cmd = get_sub_cmd("internal_pri", "0-15");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.internal_pri), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("internal_dp", "0-3");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.internal_dp), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    cmd_data_check_element("meter_color", "meter_yellow",
                        "usage:meter_colore:meter_yellow/meter_red, etc\n",
                        cmd_data_check_attr, ("policer_meter_color", cmd,
                        &(entry.meter_color), sizeof(entry.meter_color)));

    *(fal_policer_priority_t *)val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_policer_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_policer_ctrl_t entry;

    aos_mem_zero(&entry, sizeof (fal_policer_ctrl_t));

    do
    {
        cmd = get_sub_cmd("head", "0-511");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.head), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("tail", "0-511");
		SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(entry.tail), sizeof (a_uint32_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    *(fal_policer_ctrl_t *)val = entry;
    return SW_OK;
}
#endif
#endif

#ifdef IN_SERVCODE
sw_error_t
cmd_data_check_servcode_config(char *info, fal_servcode_config_t *val, a_uint32_t size)
{
	char *cmd = NULL;
	sw_error_t rv;
	fal_servcode_config_t entry;

	memset(&entry, 0, sizeof (fal_servcode_config_t));

	do
	{
		cmd = get_sub_cmd("dest_port_valid", "yes");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_TRUE, &entry.dest_port_valid,
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("dest_port_id", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.dest_port_id, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("bypass_bitmap[0]", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd,
				&entry.bypass_bitmap[0], sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("bypass_bitmap[1]", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd,
				&entry.bypass_bitmap[1], sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("bypass_bitmap[2]", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd,
				&entry.bypass_bitmap[2], sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));
#ifdef APPE
	do
	{
		cmd = get_sub_cmd("bypass_bitmap[3]", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd,
				&entry.bypass_bitmap[3], sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));
#endif
	do
	{
		cmd = get_sub_cmd("direction", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.direction, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("field_update_bitmap", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.field_update_bitmap, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("next_service_code", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.next_service_code, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hw_services", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.hw_services, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("offset_sel", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.offset_sel, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	*val = entry;
	return SW_OK;
}
#endif

sw_error_t
cmd_data_check_uint8_array(char *cmdstr, void *val, a_uint32_t size)
{
    char *tmp = NULL;
    a_uint32_t i = 0, j;
    a_uint32_t addr;
    a_uint8_t *dst = (a_uint8_t*)val;

    if (NULL == cmdstr)
    {
        return SW_BAD_VALUE;
    }

    if (0 == cmdstr[0])
    {
        return SW_OK;
    }

    tmp = (void *) strsep(&cmdstr, "-");
    while (tmp)
    {
        if (size <= i)
        {
            return SW_BAD_VALUE;
        }

        if ((2 < strlen(tmp)) || (0 == strlen(tmp)))
        {
            return SW_BAD_VALUE;
        }

        for (j = 0; j < strlen(tmp); j++)
        {
            if (A_FALSE == is_hex(tmp[j]))
                return SW_BAD_VALUE;
        }

        sscanf(tmp, "%x", &addr);
        if (0xff < addr)
        {
            return SW_BAD_VALUE;
        }

        dst[i++] = addr;
        tmp = (void *) strsep(&cmdstr, "-");
    }

    if (size != i)
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

#ifdef IN_RSS_HASH
sw_error_t
cmd_data_check_rss_hash_mode(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "ipv4v6"))
    {
        *arg_val = FAL_RSS_HASH_IPV4V6;
    }
    else if (!strcasecmp(cmd_str, "ipv4"))
    {
        *arg_val = FAL_RSS_HASH_IPV4ONLY;
    }
    else if (!strcasecmp(cmd_str, "ipv6"))
    {
        *arg_val = FAL_RSS_HASH_IPV6ONLY;
    }
    else
    {
        return SW_BAD_VALUE;
    }

    return SW_OK;
}

sw_error_t
cmd_data_check_rss_hash_config(char *info, fal_rss_hash_config_t *val, a_uint32_t size)
{
	char *cmd = NULL;
	a_uint32_t tmp;
	sw_error_t rv;
	fal_rss_hash_config_t entry;

	memset(&entry, 0, sizeof (fal_rss_hash_config_t));

	do
	{
		cmd = get_sub_cmd("hash_mask", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.hash_mask, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_fragment_mode", "no");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_confirm(cmd, A_FALSE, &entry.hash_fragment_mode,
					sizeof (a_bool_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_seed", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint32(cmd, &entry.hash_seed, sizeof (a_uint32_t));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_sip_mix", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint8_array(cmd, &entry.hash_sip_mix[0],
						sizeof (entry.hash_sip_mix));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_dip_mix", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint8_array(cmd, &entry.hash_dip_mix[0],
						sizeof (entry.hash_dip_mix));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_protocol_mix", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint8(cmd, &tmp, sizeof (a_uint32_t));
			if (SW_OK == rv)
			{
				entry.hash_protocol_mix = tmp;
			}
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_sport_mix", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint8(cmd, &tmp, sizeof (a_uint32_t));
			if (SW_OK == rv)
			{
				entry.hash_sport_mix = tmp;
			}
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_dport_mix", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint8(cmd, &tmp, sizeof (a_uint32_t));
			if (SW_OK == rv)
			{
				entry.hash_dport_mix = tmp;
			}
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_fin_inner", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint8_array(cmd, &entry.hash_fin_inner[0],
						sizeof (entry.hash_fin_inner));
		}
	}
	while (talk_mode && (SW_OK != rv));

	do
	{
		cmd = get_sub_cmd("hash_fin_outer", "0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4))
		{
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4))
		{
			rv = SW_BAD_VALUE;
		}
		else
		{
			rv = cmd_data_check_uint8_array(cmd, &entry.hash_fin_outer[0],
						sizeof (entry.hash_fin_outer));
		}
	}
	while (talk_mode && (SW_OK != rv));

	*val = entry;
	return SW_OK;
}
#endif

#ifdef IN_MIRROR
sw_error_t
cmd_data_check_mirr_analy_cfg(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_mirr_analysis_config_t *pEntry = (fal_mirr_analysis_config_t *)val;

    memset(pEntry, 0, sizeof(fal_mirr_analysis_config_t));

    do
    {
        cmd = get_sub_cmd("analysis_port", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->port_id), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    do
    {
        cmd = get_sub_cmd("analysis_priority", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &(pEntry->priority), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    return SW_OK;
}

sw_error_t
cmd_data_check_mirr_direction(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
    if (cmd_str == NULL)
        return SW_BAD_PARAM;

    if (!strcasecmp(cmd_str, "both"))
    {
        *arg_val = FAL_MIRR_BOTH;
    }
    else if (!strcasecmp(cmd_str, "ingress"))
    {
        *arg_val = FAL_MIRR_INGRESS;
    }
    else if (!strcasecmp(cmd_str, "egress"))
    {
        *arg_val = FAL_MIRR_EGRESS;
    }
    else
    {
        //dprintf("input error \n");
        return SW_BAD_VALUE;
    }

    return SW_OK;
}
#endif

#ifdef IN_CTRLPKT
sw_error_t
cmd_data_check_ctrlpkt_appprofile(char *info, void *val, a_uint32_t size)
{
    char *cmd;
    sw_error_t rv;
    fal_ctrlpkt_profile_t *pEntry = (fal_ctrlpkt_profile_t *)val;
    a_uint32_t tmp = 0;

    memset(pEntry, 0, sizeof(fal_ctrlpkt_profile_t));

    /* get port bitmap */
    do
    {
        cmd = get_sub_cmd("port_bitmap", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->port_map = tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get ethernet type profile */
    do
    {
        cmd = get_sub_cmd("ethtype_profile_bitmap", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->ethtype_profile_bitmap = tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get rfdb profile */
    do
    {
        cmd = get_sub_cmd("rfdb_profile_bitmap", "0");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_uint32(cmd, &tmp, sizeof(a_uint32_t));

	     pEntry->rfdb_profile_bitmap= tmp;
        }
    }while (talk_mode && (SW_OK != rv));

    /* get mgt_eapol */
    do
    {
        cmd = get_sub_cmd("eapol_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_eapol),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get mgt_eapol */
    do
    {
        cmd = get_sub_cmd("pppoe_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_pppoe),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get mgt_igmp */
    do
    {
        cmd = get_sub_cmd("igmp_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_igmp),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get mgt_arp_req */
    do
    {
        cmd = get_sub_cmd("arp_request_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_arp_req),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get mgt_arp_rep */
    do
    {
        cmd = get_sub_cmd("arp_response_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_arp_rep),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get mgt_dhcp4 */
    do
    {
        cmd = get_sub_cmd("dhcp4_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_dhcp4),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get mgt_dhcp6 */
    do
    {
        cmd = get_sub_cmd("dhcp6_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_dhcp6),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#if defined (APPE)
    /* get mgt_8023ah_oam */
    do
    {
        cmd = get_sub_cmd("8023ah_oam_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_8023ah_oam),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));
#endif
    /* get mgt_mld */
    do
    {
        cmd = get_sub_cmd("mld_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_mld),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));


    /* get mgt_ns */
    do
    {
        cmd = get_sub_cmd("ip6ns_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_ns),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get mgt_na */
    do
    {
        cmd = get_sub_cmd("ip6na_en", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->protocol_types.mgt_na),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get forward command */
    do
    {
        cmd = get_sub_cmd("ctrlpkt_profile_action", "forward");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_maccmd(cmd, &(pEntry->action.action), sizeof(a_uint32_t));
        }
    }while (talk_mode && (SW_OK != rv));

    /* get sg_byp */
    do
    {
        cmd = get_sub_cmd("sourceguard_bypass", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->action.sg_bypass),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get l2_filter_byp */
    do
    {
        cmd = get_sub_cmd("l2filter_bypass", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->action.l2_filter_bypass),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get in_stp_byp */
    do
    {
        cmd = get_sub_cmd("ingress_stp_bypass", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->action.in_stp_bypass),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    /* get in_vlan_fltr_byp */
    do
    {
        cmd = get_sub_cmd("ingress_vlan_filter_bypass", "no");
        SW_RTN_ON_NULL_PARAM(cmd);

        if (!strncasecmp(cmd, "quit", 4))
        {
            return SW_BAD_VALUE;
        }
        else if (!strncasecmp(cmd, "help", 4))
        {
            rv = SW_BAD_VALUE;
        }
        else
        {
            rv = cmd_data_check_confirm(cmd, A_FALSE, &(pEntry->action.in_vlan_fltr_bypass),
                                        sizeof (a_bool_t));
        }
    }
    while (talk_mode && (SW_OK != rv));

    return SW_OK;
}
#endif

sw_error_t
cmd_data_check_module(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{
	if (cmd_str == NULL)
		return SW_BAD_PARAM;

	if (!strcasecmp(cmd_str, "acl")) {
		*arg_val = FAL_MODULE_ACL;
	} else if (!strcasecmp(cmd_str, "vsi")) {
		*arg_val = FAL_MODULE_VSI;
	} else if (!strcasecmp(cmd_str, "ip")) {
		*arg_val = FAL_MODULE_IP;
	} else if (!strcasecmp(cmd_str, "flow")) {
		*arg_val = FAL_MODULE_FLOW;
	} else if (!strcasecmp(cmd_str, "qm")) {
		*arg_val = FAL_MODULE_QM;
	} else if (!strcasecmp(cmd_str, "qos")) {
		*arg_val = FAL_MODULE_QOS;
	} else if (!strcasecmp(cmd_str, "bm")) {
		*arg_val = FAL_MODULE_BM;
	} else if (!strcasecmp(cmd_str, "servcode")) {
		*arg_val = FAL_MODULE_SERVCODE;
	} else if (!strcasecmp(cmd_str, "rsshash")) {
		*arg_val = FAL_MODULE_RSS_HASH;
	} else if (!strcasecmp(cmd_str, "pppoe")) {
		*arg_val = FAL_MODULE_PPPOE;
	} else if (!strcasecmp(cmd_str, "portctrl")) {
		*arg_val = FAL_MODULE_PORTCTRL;
	} else if (!strcasecmp(cmd_str, "shaper")) {
		*arg_val = FAL_MODULE_SHAPER;
	} else if (!strcasecmp(cmd_str, "mib")) {
		*arg_val = FAL_MODULE_MIB;
	} else if (!strcasecmp(cmd_str, "mirror")) {
		*arg_val = FAL_MODULE_MIRROR;
	} else if (!strcasecmp(cmd_str, "fdb")) {
		*arg_val = FAL_MODULE_FDB;
	} else if (!strcasecmp(cmd_str, "stp")) {
		*arg_val = FAL_MODULE_STP;
	} else if (!strcasecmp(cmd_str, "sec")) {
		*arg_val = FAL_MODULE_SEC;
	} else if (!strcasecmp(cmd_str, "trunk")) {
		*arg_val = FAL_MODULE_TRUNK;
	} else if (!strcasecmp(cmd_str, "portvlan")) {
		*arg_val = FAL_MODULE_PORTVLAN;
	} else if (!strcasecmp(cmd_str, "ctrlpkt")) {
		*arg_val = FAL_MODULE_CTRLPKT;
	} else if (!strcasecmp(cmd_str, "policer")) {
		*arg_val = FAL_MODULE_POLICER;
#ifdef APPE
	} else if (!strcasecmp(cmd_str, "vport")) {
		*arg_val = FAL_MODULE_VPORT;
	} else if (!strcasecmp(cmd_str, "tunnel")) {
		*arg_val = FAL_MODULE_TUNNEL;
	} else if (!strcasecmp(cmd_str, "vxlan")){
		*arg_val = FAL_MODULE_VXLAN;
	} else if (!strcasecmp(cmd_str, "geneve")){
		*arg_val = FAL_MODULE_GENEVE;
	} else if (!strcasecmp(cmd_str, "mapt")) {
		*arg_val = FAL_MODULE_MAPT;
	} else if (!strcasecmp(cmd_str, "tunnelprogram")){
		*arg_val = FAL_MODULE_TUNNEL_PROGRAM;
#endif
	}
	else
	{
		return SW_BAD_VALUE;
	}

	return SW_OK;
}

sw_error_t
cmd_data_check_func_ctrl(char *cmd_str, void * val, a_uint32_t size)
{
	sw_error_t rv;
	fal_func_ctrl_t entry;

	aos_mem_zero(&entry, sizeof (fal_func_ctrl_t));

	rv = __cmd_data_check_complex("bitmap0", "0",
			"usage: the format is HEX \n", cmd_data_check_uint32,
			&(entry.bitmap[0]), sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("bitmap1", "0",
			"usage: the format is HEX \n", cmd_data_check_uint32,
			&(entry.bitmap[1]), sizeof (a_uint32_t));
	if (rv)
		return rv;

	rv = __cmd_data_check_complex("bitmap2", "0",
			"usage: the format is HEX \n", cmd_data_check_uint32,
			&(entry.bitmap[2]), sizeof (a_uint32_t));
	if (rv)
		return rv;

	*(fal_func_ctrl_t *)val = entry;

	return SW_OK;
}
#ifdef IN_TUNNEL
sw_error_t
cmd_data_check_tunnel_udp_entry(char * cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    fal_tunnel_udp_entry_t entry;
    a_uint32_t tmpdata = 0;

    memset(&entry, 0, sizeof (fal_tunnel_udp_entry_t));

    cmd_data_check_element("ip ver", "ipv4",
                       "usage: ipv4, ipv6, ipv4_and_ipv6\n",
                       cmd_data_check_attr, ("ip_ver", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.ip_ver= tmpdata & 0x3;

    cmd_data_check_element("udp type", "udp",
                       "usage: udp, udp-lite, udp_and_udp-lite\n",
                       cmd_data_check_attr, ("udp_type", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.udp_type= tmpdata & 0x3;

    cmd_data_check_element("l4 port type", "dst",
                       "usage: dst, src, dst_and_src\n",
                       cmd_data_check_attr, ("l4_port_type", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.l4_port_type= tmpdata & 0x3;

    cmd_data_check_element("l4 port", "0",
                       "usage: the format is 0x0-0xffff or 0-65535\n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0xffff, 0x0));
    entry.l4_port = tmpdata & 0xffff;

    *(fal_tunnel_udp_entry_t *) val = entry;
    return SW_OK;
}
#endif

#ifdef IN_VXLAN
sw_error_t
cmd_data_check_vxlan_type(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{

    return cmd_data_check_attr("vxlan_type", cmd_str,
                    arg_val, sizeof(*arg_val));
}

sw_error_t
cmd_data_check_vxlan_gpe_proto(char * cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    fal_vxlan_gpe_proto_cfg_t proto;
    a_uint32_t tmpdata = 0;

    memset(&proto, 0, sizeof (fal_vxlan_gpe_proto_cfg_t));

    cmd_data_check_element("ipv4", "1",
                       "usage: the format is 0x0-0xff or 0-255 \n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0xff, 0x0));
    proto.ipv4 = tmpdata & 0xff;

    cmd_data_check_element("ipv6", "2",
                       "usage: the format is 0x0-0xff or 0-255 \n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0xff, 0x0));
    proto.ipv6 = tmpdata & 0xff;

    cmd_data_check_element("ethernet", "3",
                       "usage: the format is 0x0-0xff or 0-255 \n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0xff, 0x0));
    proto.ethernet = tmpdata & 0xff;

    *(fal_vxlan_gpe_proto_cfg_t *) val = proto;
    return SW_OK;
}
#endif

#ifdef IN_TUNNEL_PROGRAM
sw_error_t
cmd_data_check_tunnel_program_type(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{

    return cmd_data_check_attr("tunnel_program_type", cmd_str,
                    arg_val, sizeof(*arg_val));
}

sw_error_t
cmd_data_check_tunnel_program_entry(char * cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    fal_tunnel_program_entry_t entry;
    a_uint32_t tmpdata = 0;

    memset(&entry, 0, sizeof (fal_tunnel_program_entry_t));

    cmd_data_check_element("ip ver", "ipv4",
                       "usage: ipv4, ipv6, ipv4_and_ipv6\n",
                       cmd_data_check_attr, ("ip_ver", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.ip_ver= tmpdata & 0x3;

    cmd_data_check_element("outer hdr type", "ethernet",
                       "usage: ethernet, ipv4, ipv6, udp, udp-lite, tcp, "
                       "gre, vxlan, vxlan-gpe, geneve\n",
                       cmd_data_check_attr, ("hdr_type", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.outer_hdr_type = tmpdata & 0xf;

    if(entry.outer_hdr_type == FAL_ETHERNET_HDR)
    {
        cmd_data_check_element("ethernet type", NULL,
                           "usage: the format is 0x0-0xffff or 0-65535\n",
                           cmd_data_check_integer, (cmd, &tmpdata,
                           0xffff, 0x0));
        entry.protocol = tmpdata & 0xffff;

        cmd_data_check_element("ethernet type mask", NULL,
                           "usage: the format is 0x0-0xffff or 0-65535\n",
                           cmd_data_check_integer, (cmd, &tmpdata, 0xffff,
                                   0x0));
        entry.protocol_mask = tmpdata & 0xffff;
    }
    else if(entry.outer_hdr_type == FAL_IPV4_HDR || entry.outer_hdr_type == FAL_IPV6_HDR)
    {
        cmd_data_check_element("ip protocol", NULL,
                           "usage: the format is 0x0-0xff or 0-255 \n",
                           cmd_data_check_integer, (cmd, &tmpdata, 0xff,
                                   0x0));
        entry.protocol = tmpdata & 0xff;

        cmd_data_check_element("ip protocol mask", NULL,
                           "usage: the format is 0x0-0xff or 0-255 \n",
                           cmd_data_check_integer, (cmd, &tmpdata, 0xff,
                                   0x0));
        entry.protocol_mask = tmpdata & 0xff;
    }
    else if(entry.outer_hdr_type >= FAL_UDP_HDR && entry.outer_hdr_type <= FAL_TCP_HDR)
    {
        cmd_data_check_element("l4 dst port", NULL,
                           "usage: the format is 0x0-0xffff or 0-65535 \n",
                           cmd_data_check_integer, (cmd, &tmpdata,
                                   0xffff, 0x0));
        entry.protocol = tmpdata & 0xffff;

        cmd_data_check_element("l4 dst port mask", NULL,
                           "usage: the format is 0x0-0xffff or 0-65535 \n",
                           cmd_data_check_integer, (cmd, &tmpdata,
                                   0xffff, 0x0));
        entry.protocol_mask = tmpdata & 0xffff;

        cmd_data_check_element("l4 src port", NULL,
                           "usage: the format is 0x0-0xffff or 0-65535 \n",
                           cmd_data_check_integer, (cmd, &tmpdata,
                                   0xffff, 0x0));
        entry.protocol |= (tmpdata & 0xffff)<<16;

        cmd_data_check_element("l4 src port mask", NULL,
                           "usage: the format is 0x0-0xffff or 0-65535 \n",
                           cmd_data_check_integer, (cmd, &tmpdata,
                                   0xffff, 0x0));
        entry.protocol_mask |= (tmpdata & 0xffff)<<16;
    }
    else if(entry.outer_hdr_type >= FAL_GRE_HDR && entry.outer_hdr_type <= FAL_GENEVE_HDR)
    {
        cmd_data_check_element("first 32bit filed of tunnel header", NULL,
                           "usage: the format is 0x0-0xffffffff or 0-4294967295\n",
                           cmd_data_check_integer, (cmd, &tmpdata,
                                   0xffffffff, 0x0));
        entry.protocol = tmpdata & 0xffffffff;

        cmd_data_check_element("first 32bit filed mask of tunnel header", NULL,
                           "usage: the format is 0x0-0xffffffff or 0-4294967295 \n",
                           cmd_data_check_integer, (cmd, &tmpdata,
                                   0xffffffff, 0x0));
        entry.protocol_mask = tmpdata & 0xffffffff;
    }
    else
    {
        return SW_BAD_PARAM;
    }

    *(fal_tunnel_program_entry_t *) val = entry;
    return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_program_cfg(char * cmd_str, void * val, a_uint32_t size)
{
    char *cmd;
    fal_tunnel_program_cfg_t entry;
    a_uint32_t tmpdata = 0;

    memset(&entry, 0, sizeof (fal_tunnel_program_cfg_t));

    cmd_data_check_element("program pos mode", "end",
                       "usage: end for end of outer hdr, start for start of outer hdr\n",
                       cmd_data_check_attr, ("tunnel_program_pos_mode", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.program_pos_mode = tmpdata & 0x1;

    cmd_data_check_element("inner type mode", "fix",
                       "usage: fix, udf\n",
                       cmd_data_check_attr, ("tunnel_program_inner_type_mode", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.inner_type_mode = tmpdata & 0x1;

    cmd_data_check_element("inner hdr type", "ethernet",
                       "usage: ethernet, ethernet-tag, ipv4, ipv6\n",
                       cmd_data_check_attr, ("hdr_type", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.inner_hdr_type = tmpdata & 0x3;

    cmd_data_check_element("basic hdr length", "0",
                       "usage: the format is 0x0-0x7e or 0-126, must be even\n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0x7e, 0x0));
    entry.basic_hdr_len = tmpdata;

    cmd_data_check_element("option length unit", "1byte",
                       "usage: 1byte, 2bytes, 4bytes, 8bytes\n",
                       cmd_data_check_attr, ("tunnel_program_opt_len_unit", cmd,
                               &tmpdata, sizeof(tmpdata)));
    entry.opt_len_unit = tmpdata & 0x3;

    cmd_data_check_element("option length mask", "0",
                       "usage: the format is 0x0-0xffff or 0-65535\n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0xffff, 0x0));
    entry.opt_len_mask = tmpdata & 0xffff;

    cmd_data_check_element("udf0 offset", "0",
                       "usage: the format is 0x0-0x7e or 0-126, must be even\n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0x7e, 0x0));
    entry.udf_offset[0] = tmpdata;

    cmd_data_check_element("udf1 offset", "0",
                       "usage: the format is 0x0-0x7e or 0-126, must be even\n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0x7e, 0x0));
    entry.udf_offset[1] = tmpdata;
    cmd_data_check_element("udf2 offset", "0",
                       "usage: the format is 0x0-0x7e or 0-126, must be even\n",
                       cmd_data_check_integer, (cmd, &tmpdata,
                               0x7e, 0x0));
    entry.udf_offset[2] = tmpdata;

    *(fal_tunnel_program_cfg_t *) val = entry;
    return SW_OK;
}
#endif

#ifdef IN_TUNNEL
sw_error_t
cmd_data_check_tunnel_udf_type(char *cmd_str, a_uint32_t * arg_val, a_uint32_t size)
{

    return cmd_data_check_attr("tunnel_udf_type", cmd_str,
                    arg_val, sizeof(*arg_val));
}

sw_error_t
cmd_data_check_tunnel_intf(char *cmd_str, fal_tunnel_intf_t *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_tunnel_intf_t entry;

	aos_mem_zero(&entry, sizeof(fal_tunnel_intf_t));

        rv = __cmd_data_check_boolean("ipv4_decap_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.ipv4_decap_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("ipv6_decap_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.ipv6_decap_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("dmac_check_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.dmac_check_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("ttl_exceed_deacce_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.ttl_exceed_deacce_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("ttl_exceed_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.ttl_exceed_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("lpm_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.lpm_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("mini_ipv6_mtu", "0x500",
			"usage: ipv6 mtu\n",
			cmd_data_check_uint32, (cmd, &(entry.mini_ipv6_mtu),
				sizeof(entry.mini_ipv6_mtu)));

	*(fal_tunnel_intf_t* )arg_val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_port_intf(char *cmd_str, fal_tunnel_port_intf_t *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_tunnel_port_intf_t entry;

	aos_mem_zero(&entry, sizeof(fal_tunnel_port_intf_t));

        rv = __cmd_data_check_boolean("tl_l3if_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.l3_if.l3_if_valid),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("tl_l3if", "0",
			"usage: tunnel l3 interface\n",
			cmd_data_check_uint32, (cmd, &(entry.l3_if.l3_if_index),
				sizeof(entry.l3_if.l3_if_index)));

	rv = __cmd_data_check_complex("dmac_addr", "0-0-0-0-0-0",
			"usage: the format is xx-xx-xx-xx-xx-xx \n",
			(param_check_t)cmd_data_check_macaddr, &entry.mac_addr,
			sizeof (fal_mac_addr_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("pppoe_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.pppoe_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("pppoe_group_id", "0",
			"usage: pppoe group id\n",
			cmd_data_check_uint32, (cmd, &(entry.pppoe_group_id),
				sizeof(a_uint32_t)));

	cmd_data_check_element("vlan_group_id", "0",
			"usage: vlan group id\n",
			cmd_data_check_uint32, (cmd, &(entry.vlan_group_id),
				sizeof(a_uint32_t)));

	*(fal_tunnel_port_intf_t *)arg_val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_encap_rule_entry(char *cmd_str,
		fal_tunnel_encap_rule_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_encap_rule_t entry;
	sw_error_t rv = SW_OK;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_tunnel_encap_rule_t));

	cmd_data_check_element("src1_sel", "header_data",
			"usage: src1 data: zero_data or header_data\n",
			cmd_data_check_attr, ("src1_sel", cmd, &(entry.src1_sel),
				sizeof(entry.src1_sel)));

	cmd_data_check_element("src1_start(byte)", "0",
			"usage: select 16Bytes from position of header_data\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src1_start = tmp;

	cmd_data_check_element("src2_sel", "napt_addr",
			"usage: src2 data: zero_data, pkt_data, napt_addr, "
			"tunnel_vni, proto_map0, proto_map1\n",
			cmd_data_check_attr, ("src2_sel", cmd, &(entry.src2_sel),
				sizeof(entry.src2_sel)));

        rv = __cmd_data_check_boolean("src2_en0", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.src2_entry[0].enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("src2_start0(bit)", "0",
			"usage: the start positon to be selected\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src2_entry[0].src_start = tmp;


	cmd_data_check_element("src2_width0(bit)", "0",
			"usage: the selected width\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src2_entry[0].src_width = tmp;

	cmd_data_check_element("dest2_pos0(bit)", "0",
			"usage: the position for the selected data to src1\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src2_entry[0].dest_pos = tmp;

        rv = __cmd_data_check_boolean("src2_en1", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.src2_entry[1].enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("src2_start1(bit)", "0",
			"usage: the start positon to be selected\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src2_entry[1].src_start = tmp;

	cmd_data_check_element("src2_width1(bit)", "0",
			"usage: the selected width\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src2_entry[1].src_width = tmp;

	cmd_data_check_element("dest2_pos1(bit)", "0",
			"usage: the position for the selected data to src2\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src2_entry[1].dest_pos = tmp;

	cmd_data_check_element("src3_sel", "napt_port",
			"usage: src3 data: zero_data, pkt_data, napt_port, policy_id, "
			"hash_value, proto_map0, proto_map1\n",
			cmd_data_check_attr, ("src3_sel", cmd, &(entry.src3_sel),
				sizeof(entry.src3_sel)));


        rv = __cmd_data_check_boolean("src3_en0", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.src3_entry[0].enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("src3_start0(bit)", "0",
			"usage: the start positon to be selected\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src3_entry[0].src_start = tmp;

	cmd_data_check_element("src3_width0(bit)", "0",
			"usage: the selected width\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src3_entry[0].src_width = tmp;

	cmd_data_check_element("dest3_pos0(bit)", "0",
			"usage: the position for the selected data to src3\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src3_entry[0].dest_pos = tmp;

        rv = __cmd_data_check_boolean("src3_en1", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.src3_entry[1].enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("src3_start1(bit)", "0",
			"usage: the start positon to be selected\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src3_entry[1].src_start = tmp;

	cmd_data_check_element("src3_width1(bit)", "0",
			"usage: the selected width\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src3_entry[1].src_width = tmp;

	cmd_data_check_element("dest3_pos1(bit)", "0",
			"usage: the position for the selected data to src3\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.src3_entry[1].dest_pos = tmp;

	*(fal_tunnel_encap_rule_t *)arg_val = entry;
	return rv;
}

sw_error_t
cmd_data_check_tunnel_encap_tunnelid(char *cmd_str, fal_tunnel_id_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_id_t entry;
	sw_error_t rv = SW_OK;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof (fal_tunnel_id_t));

        rv = __cmd_data_check_boolean("tunnel_id_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.tunnel_id_valid),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("tunnel_id", "0",
			"usage: tunnel id value\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.tunnel_id = tmp;

	*(fal_tunnel_id_t *)arg_val = entry;

	return rv;
}

sw_error_t
cmd_data_check_vlantag(char *cmd_str, a_uint8_t *arg_val, a_uint32_t size)
{
	if (cmd_str == NULL)
		return SW_BAD_PARAM;

	if (!strcasecmp(cmd_str, "tag")) {
		*arg_val = BIT(2);
	}
	else if (!strcasecmp(cmd_str, "pritag")) {
		*arg_val = BIT(1);
	}
	else if (!strcasecmp(cmd_str, "untag")) {
		*arg_val = BIT(0);
	}
	else {
		return SW_BAD_VALUE;
	}

	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_vlan_intf(char *cmd_str, fal_tunnel_vlan_intf_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_vlan_intf_t entry;
	a_bool_t enable;
	sw_error_t rv = SW_OK;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof (fal_tunnel_vlan_intf_t));

	rv = __cmd_data_check_complex("port", "0",
			"usage: port id\n",
			(param_check_t)cmd_data_check_portmap, &entry.port_id,
			sizeof (fal_pbmp_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("svlan_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= FAL_TUNNEL_SVLAN_CHECK_EN;
	else
		entry.key_bmp &= ~FAL_TUNNEL_SVLAN_CHECK_EN;

	cmd_data_check_element("svlan_fmt", "untag",
			"usage: tag or pritag or untag\n",
			cmd_data_check_vlantag, (cmd, &(entry.svlan_fmt),
				sizeof(entry.svlan_fmt)));

	cmd_data_check_element("svlan_id", "0",
			"usage: svlan id\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.svlan_id = tmp;

        rv = __cmd_data_check_boolean("cvlan_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= FAL_TUNNEL_CVLAN_CHECK_EN;
	else
		entry.key_bmp &= ~FAL_TUNNEL_CVLAN_CHECK_EN;

	cmd_data_check_element("cvlan_fmt", "untag",
			"usage: tag or pritag or untag\n",
			cmd_data_check_vlantag, (cmd, &(entry.cvlan_fmt),
				sizeof(entry.cvlan_fmt)));

	cmd_data_check_element("cvlan_id", "0",
			"usage: cvlan id\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.cvlan_id = tmp;

        rv = __cmd_data_check_boolean("pppoe_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.pppoe_en,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("tl_l3if_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &entry.l3_if.l3_if_valid,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("tl_l3if", "0",
			"usage: tunnel l3 interface\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry.l3_if.l3_if_index = tmp;

	*(fal_tunnel_vlan_intf_t *)arg_val = entry;
	return rv;
}

sw_error_t
cmd_data_check_tag_format(char *cmd_str, a_uint32_t *arg_val, a_uint32_t size)
{
	if (cmd_str == NULL)
		return SW_BAD_PARAM;

	if (!strcasecmp(cmd_str, "tag")) {
		*arg_val = 1;
	}
	else if (!strcasecmp(cmd_str, "untag")) {
		*arg_val = 0;
	}
	else {
		return SW_BAD_VALUE;
	}

	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_mode(char *cmd_str, fal_tunnel_mode_t *arg_val, a_uint32_t size)
{
	if (cmd_str == NULL)
		return SW_BAD_PARAM;

	if (!strcasecmp(cmd_str, "uniform")) {
		*arg_val = FAL_TUNNEL_UNIFORM_MODE;
	}
	else if (!strcasecmp(cmd_str, "pipe")) {
		*arg_val = FAL_TUNNEL_PIPE_MODE;
	}
	else {
		return SW_BAD_VALUE;
	}

	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_decap_rule_entry(char *cmd_str,
		fal_tunnel_rule_t *arg_val, a_uint32_t size)
{
	fal_tunnel_rule_t entry_rule;
	char *cmd;
	a_bool_t enable = A_FALSE;
	sw_error_t rv = SW_OK;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry_rule, sizeof(fal_tunnel_rule_t));

	cmd_data_check_element("tunnel_type", "gre_tap_ipv4",
			"usage: tunnel_type: gre_tap_ipv4, gre_tap_ipv6, vxlan_ipv4, "
			"vxlan_ipv6, vxlan_gpe_ipv4, vxlan_gpe_ipv6, ipv4_ipv6, "
			"program0, program1, program2, program3, program4, program5, "
			"geneve_ipv4, geneve_ipv6",
			cmd_data_check_attr, ("tunnel_type", cmd, &(entry_rule.tunnel_type),
				sizeof(entry_rule.tunnel_type)));

	cmd_data_check_element("entry_id", "0",
			"usage: entry index\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.entry_id = tmp;

	cmd_data_check_element("ip_ver", "0",
			"usage: 0 for ipv4, 1 for ipv6\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.ip_ver = tmp;

	/*
        rv = __cmd_data_check_boolean("key_sip_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_SIP_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_SIP_EN);
	*/

	if(entry_rule.ip_ver == 0) {
		cmd_data_check_element("sip4_addr", "0.0.0.0",
				"usage: the format is xx.xx.xx.xx \n",
				cmd_data_check_ip4addr, (cmd, &(entry_rule.sip.ip4_addr), 4));
	} else {
		cmd_data_check_element("sip6_addr", "0::0",
				"usage: the format is xxxx::xxxx \n",
				cmd_data_check_ip6addr, (cmd, &(entry_rule.sip.ip6_addr), 16));
	}

	/*
        rv = __cmd_data_check_boolean("key_dip_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_DIP_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_DIP_EN);
	*/

	if(entry_rule.ip_ver == 0) {
		cmd_data_check_element("dip4_addr", "0.0.0.0",
				"usage: the format is xx.xx.xx.xx \n",
				cmd_data_check_ip4addr, (cmd, &(entry_rule.dip.ip4_addr), 4));
	} else {
		cmd_data_check_element("dip6_addr", "0::0",
				"usage: the format is xxxx::xxxx \n",
				cmd_data_check_ip6addr, (cmd, &(entry_rule.dip.ip6_addr), 16));
	}

	/*
        rv = __cmd_data_check_boolean("key_l4proto_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_L4PROTO_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_L4PROTO_EN);
	*/

	cmd_data_check_element("l4_proto", "0",
			"usage: layer4 proto id\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.l4_proto = tmp;

	/*
        rv = __cmd_data_check_boolean("key_sport_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_SPORT_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_SPORT_EN);
	*/

	cmd_data_check_element("sport", "0",
			"usage: src port\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.sport = tmp;

	/*
        rv = __cmd_data_check_boolean("key_dport_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_DPORT_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_DPORT_EN);
	*/

	cmd_data_check_element("dport", "0",
			"usage: dst port\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.dport = tmp;

        rv = __cmd_data_check_boolean("key_tlinfo_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_TLINFO_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_TLINFO_EN);

	/*
	cmd_data_check_element("tunnel_info_mask", "0",
			"usage: tunnel info\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.tunnel_info_mask = tmp;
	*/

	cmd_data_check_element("tunnel_info", "0",
			"usage: tunnel info\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.tunnel_info = tmp;

        rv = __cmd_data_check_boolean("key_udf0_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_UDF0_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_UDF0_EN);

	/*
	cmd_data_check_element("udf0_idx", "0",
			"usage: udf0 id to select\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.udf0_idx = tmp;
	*/

	cmd_data_check_element("udf0", "0",
			"usage: udf0 value\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.udf0 = tmp;

	/*
	cmd_data_check_element("udf0_mask", "0",
			"usage: udf0 mask value\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.udf0_mask = tmp;
	*/

        rv = __cmd_data_check_boolean("key_udf1_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_rule.key_bmp |= BIT(FAL_TUNNEL_KEY_UDF1_EN);
	else
		entry_rule.key_bmp &= ~BIT(FAL_TUNNEL_KEY_UDF1_EN);

	/*
	cmd_data_check_element("udf1_idx", "0",
			"usage: udf1 id to select\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.udf1_idx = tmp;
	*/

	cmd_data_check_element("udf1", "0",
			"usage: udf1 value\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.udf1 = tmp;

	/*
	cmd_data_check_element("udf1_mask", "0",
			"usage: udf1 mask value\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_rule.udf1_mask = tmp;
	*/

	*arg_val = entry_rule;
	return rv;
}

sw_error_t
cmd_data_check_tunnel_decap_action_entry(char *cmd_str,
		fal_tunnel_action_t *arg_val, a_uint32_t size)
{
	fal_tunnel_action_t entry_action;
	char *cmd;
	a_bool_t enable = A_FALSE;
	sw_error_t rv = SW_OK;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry_action, sizeof(fal_tunnel_action_t));

        rv = __cmd_data_check_boolean("decap_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry_action.decap_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("fwd_cmd", "forward",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry_action.fwd_cmd,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("svlan_check", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry_action.verify_entry.verify_bmp |= FAL_TUNNEL_SVLAN_CHECK_EN;
	else
		entry_action.verify_entry.verify_bmp &= ~FAL_TUNNEL_SVLAN_CHECK_EN;

	cmd_data_check_element("svlan_fmt", "untag",
			"usage: tag or untag\n",
			cmd_data_check_tag_format,
			(cmd, (a_uint32_t *)&(entry_action.verify_entry.svlan_fmt),
			 sizeof(a_uint32_t)));

	cmd_data_check_element("svlan_id", "0",
			"usage: svlan id\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_action.verify_entry.svlan_id = tmp;

        rv = __cmd_data_check_boolean("cvlan_check", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);
	if (enable == A_TRUE)
		entry_action.verify_entry.verify_bmp |=
			FAL_TUNNEL_CVLAN_CHECK_EN;
	else
		entry_action.verify_entry.verify_bmp &=
			~FAL_TUNNEL_CVLAN_CHECK_EN;

	cmd_data_check_element("cvlan_fmt", "untag",
			"usage: tag or untag\n",
			cmd_data_check_tag_format,
			(cmd, (a_uint32_t *)&(entry_action.verify_entry.cvlan_fmt),
			 sizeof(a_uint32_t)));

	cmd_data_check_element("cvlan_id", "0",
			"usage: cvlan id\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_action.verify_entry.cvlan_id = tmp;

        rv = __cmd_data_check_boolean("tl_l3if_check", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);
	if (enable == A_TRUE)
		entry_action.verify_entry.verify_bmp |=
			FAL_TUNNEL_L3IF_CHECK_EN;
	else
		entry_action.verify_entry.verify_bmp &=
			~FAL_TUNNEL_L3IF_CHECK_EN;

	cmd_data_check_element("tl_l3if", "0",
			"usage: tunnel l3 interface\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_action.verify_entry.tl_l3_if = tmp;

        rv = __cmd_data_check_boolean("deacce_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry_action.deacce_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("udp_csum_zero", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry_action.udp_csum_zero),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("exp_profile_id", "0",
			"usage: Exception profile ID\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_action.exp_profile = tmp;

        rv = __cmd_data_check_boolean("service_code_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry_action.service_code_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("service_code", "0",
			"usage: updated service code\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry_action.service_code = tmp;

        rv = __cmd_data_check_boolean("src_info_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry_action.src_info_enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("src_info_type", "vp",
			"usage: vp: virtual port, l3_if: layer 3 interface\n",
			cmd_data_check_srctype,
			(cmd, 0, &entry_action.src_info_type,
			 sizeof(a_uint8_t)));

	cmd_data_check_element("src_info", "0",
			"usage: src info value\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry_action.src_info = tmp;

	cmd_data_check_element("spcp_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry_action.spcp_mode),
			  strlen(cmd)));

	cmd_data_check_element("sdei_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry_action.sdei_mode),
			  strlen(cmd)));

	cmd_data_check_element("cpcp_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry_action.cpcp_mode),
			  strlen(cmd)));

	cmd_data_check_element("cdei_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry_action.cdei_mode),
			  strlen(cmd)));

	cmd_data_check_element("ttl_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry_action.ttl_mode),
			  strlen(cmd)));

	cmd_data_check_element("dscp_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry_action.dscp_mode),
			  strlen(cmd)));

	cmd_data_check_element("ecn_mode", "0",
			"usage: ecn mode\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry_action.ecn_mode = tmp;

	*arg_val = entry_action;
	return rv;
}

sw_error_t
cmd_data_check_tunnel_decap_entry(char *cmd_str,
		fal_tunnel_decap_entry_t *arg_val, a_uint32_t size)
{
	fal_tunnel_decap_entry_t entry;
	fal_tunnel_rule_t *entry_rule;
	fal_tunnel_action_t *entry_action;
	sw_error_t rv = SW_OK;

	aos_mem_zero(&entry, sizeof (fal_tunnel_decap_entry_t));
	entry_rule = &entry.decap_rule;
	entry_action = &entry.decap_action;

	rv = cmd_data_check_tunnel_decap_rule_entry(cmd_str,
			entry_rule, sizeof(fal_tunnel_rule_t));
	SW_RTN_ON_ERROR(rv);

	rv = cmd_data_check_tunnel_decap_action_entry(cmd_str,
			entry_action, sizeof(fal_tunnel_action_t));
	SW_RTN_ON_ERROR(rv);

	*arg_val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_encap_entry(char *cmd_str, fal_tunnel_encap_cfg_t *arg_val, a_uint32_t size)
{
	char *cmd, cmd_byte[3];
	sw_error_t rv;
	fal_tunnel_encap_cfg_t entry;
	a_uint32_t bytes = 0, tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_tunnel_encap_cfg_t));

	cmd_data_check_element("encap_type", "0",
			"usage: 0 tunnel\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.encap_type = tmp;

	cmd_data_check_element("ip_ver", "0",
			"usage: 0 ipv4, 1 ipv6\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.ip_ver = tmp;

	cmd_data_check_element("encap_target", "none",
			"usage: none, sip, dip, tunnel\n",
			cmd_data_check_attr, ("encap_target", cmd,
				&(entry.encap_target), sizeof(entry.encap_target)));

	cmd_data_check_element("payload_inner_type", "ethernet",
			"usage: ethernet, ip, transport\n",
			cmd_data_check_attr, ("payload_inner_type", cmd,
				&(entry.payload_inner_type),
				sizeof(entry.payload_inner_type)));

	cmd_data_check_element("edit_rule_id", "0",
			"usage: edit rule entry id\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.edit_rule_id = tmp;

	cmd_data_check_element("tunnel_len", "0",
			"usage: tunnel header length\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.tunnel_len = tmp;

	cmd_data_check_element("tunnel_offset", "0",
			"usage: tunnel header offset\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.tunnel_offset = tmp;

	cmd_data_check_element("vlan_offset", "0",
			"usage: vlan  offset\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.vlan_offset = tmp;

	cmd_data_check_element("l3_offset", "0",
			"usage: layer3 offset\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.l3_offset = tmp;

	cmd_data_check_element("l4_offset", "0",
			"usage: layer4 offset\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.l4_offset = tmp;

	cmd_data_check_element("svlan_fmt", "untag",
			"usage: tag or untag\n",
			cmd_data_check_tag_format,
			(cmd, (a_uint32_t *)&(entry.svlan_fmt),
			 sizeof(a_uint32_t)));

	cmd_data_check_element("spcp_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry.spcp_mode),
			  strlen(cmd)));

	cmd_data_check_element("sdei_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry.sdei_mode),
			  strlen(cmd)));

	cmd_data_check_element("cvlan_fmt", "untag",
			"usage: tag or untag\n",
			cmd_data_check_tag_format,
			(cmd, (a_uint32_t *)&(entry.cvlan_fmt),
			 sizeof(a_uint32_t)));

	cmd_data_check_element("cpcp_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry.cpcp_mode),
			  strlen(cmd)));

	cmd_data_check_element("cdei_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry.cdei_mode),
			  strlen(cmd)));

	cmd_data_check_element("dscp_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry.dscp_mode),
			  strlen(cmd)));

	cmd_data_check_element("ttl_mode", "pipe",
			"usage: <piep/uniform>\n",
			cmd_data_check_tunnel_mode,
			(cmd, &(entry.ttl_mode),
			  strlen(cmd)));

	cmd_data_check_element("ecn_mode", "0",
			"usage: ecn mode\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.ecn_mode = tmp;

        rv = __cmd_data_check_boolean("ip_proto_update", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.ip_proto_update),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("ipv4_df_mode", "0",
			"usage: ipv4 df mode\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.ipv4_df_mode = tmp;

	cmd_data_check_element("ipv4_id_mode", "0",
			"usage: ipv4 id mode\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.ipv4_id_mode = tmp;

	cmd_data_check_element("ipv6_flowlable_mode", "0",
			"usage: ipv6 id mode\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.ipv6_flowlable_mode = tmp;

        rv = __cmd_data_check_boolean("sport_entry_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.sport_entry_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("l4_proto", "0",
			"usage: ipv4 proto value\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.l4_proto = tmp;

        rv = __cmd_data_check_boolean("l4_checksum_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.l4_checksum_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("vni_mode", "0",
			"usage:  0 from header data, 1 from vlan xlat\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.vni_mode = tmp;

        rv = __cmd_data_check_boolean("pppoe_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.pppoe_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("vport_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.vport_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("cpu_vport", "0",
			"usage:  virtual port carried to cpu\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));
	entry.vport = tmp;

	do {
		cmd = get_sub_cmd("eg_header_data", "0x0");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4)) {
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4)) {
			rv = SW_BAD_VALUE;
		}
		else if (strspn(cmd, "1234567890abcdefABCDEFXx") != strlen(cmd) ||
				(strlen(cmd) -2) > 2 * FAL_TUNNEL_ENCAP_HEADER_MAX_LEN) {
			rv = SW_BAD_VALUE;
		}
		else {
			if (cmd[0] == '0' && (cmd[1] == 'x' || cmd[1] == 'X')) {
				cmd += 2;
				for (bytes = 0; bytes < FAL_TUNNEL_ENCAP_HEADER_MAX_LEN; bytes++) {
					if (strlen(cmd) == 0) {
						break;
					}
					/* copy 2 chars from cmd */
					strlcpy(cmd_byte, cmd, sizeof(cmd_byte));
					sscanf(cmd_byte, "%hhx",
						&(entry.pkt_header.pkt_header_data[bytes]));
					cmd += 2;
				}
			} else {
				rv = SW_BAD_VALUE;
			}
		}
	} while (talk_mode && (SW_OK != rv));

	*(fal_tunnel_encap_cfg_t* )arg_val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_encap_header_ctrl(char *cmd_str,
		fal_tunnel_encap_header_ctrl_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_encap_header_ctrl_t entry;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_tunnel_encap_header_ctrl_t));

	cmd_data_check_element("ipv4_id_seed", "0",
			"usage:  ipv4 id seed\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ipv4_id_seed = tmp;

	cmd_data_check_element("ipv4_df_set", "0",
			"usage:  ipv4 df\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ipv4_df_set = tmp;

	cmd_data_check_element("udp_sport_base", "0",
			"usage:  udp sport base\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.udp_sport_base = tmp;

	cmd_data_check_element("udp_sport_mask", "0",
			"usage:  udp sport mask\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.udp_sport_mask = tmp;

	cmd_data_check_element("ipv4_addr_map_data", "0",
			"usage:  address map data for ipv4\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.proto_map_data[0] = tmp;

	cmd_data_check_element("ipv4_proto_map_data", "0",
			"usage:  proto map data for ipv4\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.proto_map_data[1] = tmp;

	cmd_data_check_element("ipv6_addr_map_data", "0",
			"usage:  address map data for ipv6\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.proto_map_data[2] = tmp;

	cmd_data_check_element("ipv6_proto_map_data", "0",
			"usage:  proto map data for ipv6\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.proto_map_data[3] = tmp;

	*(fal_tunnel_encap_header_ctrl_t *)arg_val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_global_cfg(char *cmd_str, fal_tunnel_global_cfg_t *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_tunnel_global_cfg_t entry;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_tunnel_global_cfg_t));

	rv = __cmd_data_check_complex("deacce_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.deacce_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("src_if_check_deacce_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.src_if_check_deacce_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("src_if_check_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.src_if_check_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("vlan_check_deacce_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.vlan_check_deacce_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("vlan_check_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.vlan_check_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("udp_csum_zero_deacce_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.udp_csum_zero_deacce_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("udp_csum_zero_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.udp_csum_zero_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("pppoe_multicast_deacce_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.pppoe_multicast_deacce_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("pppoe_multicast_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.pppoe_multicast_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("hash_mode0", "0",
			"usage:  0 crc10, 1 xor, 2 crc16\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.hash_mode[0] = tmp;

	cmd_data_check_element("hash_mode1", "0",
			"usage:  0 crc10, 1 xor, 2 crc16\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.hash_mode[1] = tmp;

	*(fal_tunnel_global_cfg_t *)arg_val = entry;
	return SW_OK;
}

sw_error_t
cmd_data_check_encap_ecn_rule(char *cmd_str, fal_tunnel_encap_ecn_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_encap_ecn_t entry;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_tunnel_encap_ecn_t));

	cmd_data_check_element("ecn_mode", "1",
			"usage:  1:rfc3168_limit_rfc6040_cmpat_mode, "
			"2:rfc3168_full_mode, 3:rfc4301_rfc6040_normal_mode\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ecn_mode = tmp;

	cmd_data_check_element("inner_ecn", "no_ect",
			"usage: no_ect, ect_0, ect_1, ce\n",
			cmd_data_check_attr, ("ecn_val", cmd,
				&(entry.inner_ecn), sizeof(entry.inner_ecn)));

	*(fal_tunnel_encap_ecn_t *)arg_val = entry;

	return SW_OK;
}

sw_error_t
cmd_data_check_ecn_val(char *cmd_str, fal_tunnel_ecn_val_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_ecn_val_t ecn_val;

	cmd_data_check_element("ecn_val", "no_ect",
			"usage: no_ect, ect_0, ect_1, ce\n",
			cmd_data_check_attr, ("ecn_val", cmd,
				&ecn_val, sizeof(fal_tunnel_ecn_val_t)));
	*arg_val = ecn_val;

	return SW_OK;
}

sw_error_t
cmd_data_check_decap_ecn_rule(char *cmd_str, fal_tunnel_decap_ecn_rule_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_decap_ecn_rule_t entry;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_tunnel_decap_ecn_rule_t));

	cmd_data_check_element("ecn_mode", "0",
			"usage:  0:rfc3168_mode, 1:rfc4301_mode, "
			"2:rfc6040_mode\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ecn_mode = tmp;

	cmd_data_check_element("outer_ecn", "no_ect",
			"usage: no_ect, ect_0, ect_1, ce\n",
			cmd_data_check_attr, ("ecn_val", cmd,
				&entry.outer_ecn, sizeof(fal_tunnel_ecn_val_t)));

	cmd_data_check_element("inner_ecn", "no_ect",
			"usage: no_ect, ect_0, ect_1, ce\n",
			cmd_data_check_attr, ("ecn_val", cmd,
				&entry.inner_ecn, sizeof(fal_tunnel_ecn_val_t)));

	*(fal_tunnel_decap_ecn_rule_t *)arg_val = entry;

	return SW_OK;
}

sw_error_t
cmd_data_check_decap_ecn_action(char *cmd_str,
		fal_tunnel_decap_ecn_action_t *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_tunnel_decap_ecn_action_t entry;

	aos_mem_zero(&entry, sizeof(fal_tunnel_decap_ecn_action_t));

	cmd_data_check_element("ecn_value", "no_ect",
			"usage: no_ect, ect_0, ect_1, ce\n",
			cmd_data_check_attr, ("ecn_val", cmd,
				&entry.ecn_value, sizeof(fal_tunnel_ecn_val_t)));

        rv = __cmd_data_check_boolean("ecn_exp_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.ecn_exp),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	*(fal_tunnel_decap_ecn_action_t *)arg_val = entry;

	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_type(char *cmd_str, fal_tunnel_type_t *arg_val, a_uint32_t size)
{

	return cmd_data_check_attr("tunnel_type", cmd_str,
			arg_val, sizeof(*arg_val));
}

sw_error_t
cmd_data_check_tunnel_key(char *cmd_str, fal_tunnel_decap_key_t *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_tunnel_decap_key_t entry;
	a_uint32_t tmp = 0;
	a_bool_t enable = A_FALSE;
	sw_error_t rv = SW_OK;

	aos_mem_zero(&entry, sizeof(fal_tunnel_decap_key_t));

        rv = __cmd_data_check_boolean("key_sip_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_SIP_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_SIP_EN);

        rv = __cmd_data_check_boolean("key_dip_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_DIP_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_DIP_EN);

        rv = __cmd_data_check_boolean("key_l4proto_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_L4PROTO_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_L4PROTO_EN);

        rv = __cmd_data_check_boolean("key_sport_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_SPORT_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_SPORT_EN);

        rv = __cmd_data_check_boolean("key_dport_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_DPORT_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_DPORT_EN);

        rv = __cmd_data_check_boolean("key_tlinfo_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_TLINFO_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_TLINFO_EN);

	cmd_data_check_element("tunnel_info_mask", "0",
			"usage: tunnel info\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	entry.tunnel_info_mask = tmp;

        rv = __cmd_data_check_boolean("key_udf0_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_UDF0_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_UDF0_EN);

	cmd_data_check_element("udf0_idx", "0",
			"usage: udf0 id to select\n",
			cmd_data_check_uint8, (cmd, &tmp, sizeof(a_uint8_t)));

	entry.udf0_idx = tmp;

	cmd_data_check_element("udf0_mask", "0",
			"usage: udf0 mask value\n",
			cmd_data_check_uint16, (cmd, &tmp, sizeof(a_uint16_t)));

	entry.udf0_mask = tmp;

        rv = __cmd_data_check_boolean("key_udf1_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof(a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.key_bmp |= BIT(FAL_TUNNEL_KEY_UDF1_EN);
	else
		entry.key_bmp &= ~BIT(FAL_TUNNEL_KEY_UDF1_EN);

	cmd_data_check_element("udf1_idx", "0",
			"usage: udf1 id to select\n",
			cmd_data_check_uint8, (cmd, &tmp, sizeof(a_uint8_t)));
	entry.udf1_idx = tmp;

	cmd_data_check_element("udf1_mask", "0",
			"usage: udf1 mask value\n",
			cmd_data_check_uint16, (cmd, &tmp, sizeof(a_uint16_t)));
	entry.udf1_mask = tmp;

	*arg_val = entry;

	return SW_OK;
}
#endif

#ifdef IN_MAPT
sw_error_t
cmd_data_check_mapt_decap_ctrl(char *cmd_str, void *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_mapt_decap_ctrl_t entry;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_mapt_decap_ctrl_t));

	rv = __cmd_data_check_complex("src_check_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.src_check_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("dst_check_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.dst_check_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("no_tcp_udp_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.no_tcp_udp_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

	rv = __cmd_data_check_complex("udp_csum_zero_action", "drop",
			"usage: <forward/drop/cpycpu/rdtcpu>\n",
			(param_check_t)cmd_data_check_maccmd, &entry.udp_csum_zero_action,
			sizeof (fal_fwd_cmd_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("ipv4_df_set", "0",
			"usage: ipv df set\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ipv4_df_set = tmp;

	*(fal_mapt_decap_ctrl_t *)arg_val = entry;

	return SW_OK;
}

sw_error_t
cmd_data_check_mapt_decap_rule_entry(char *cmd_str, void *arg_val, a_uint32_t size)
{
	char *cmd;
	fal_mapt_decap_edit_rule_entry_t entry;
	sw_error_t rv = SW_OK;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_mapt_decap_edit_rule_entry_t));

	cmd_data_check_element("ipv4_prefix", "0.0.0.0",
			"usage: the format is xx.xx.xx.xx \n",
			cmd_data_check_ip4addr, (cmd, &(entry.ip4_addr), 4));

	cmd_data_check_element("ipv6_addr_type", "zero_data",
			"usage: ipv6 addr type:zero_data, src_ip, dst_ip\n",
			cmd_data_check_attr, ("addr_type", cmd,
				&entry.ip6_addr_src, sizeof(entry.ip6_addr_src)));

	cmd_data_check_element("suffix_start", "0",
			"usage: the start positon to be selected\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ip6_suffix_sel.src_start = tmp;

	cmd_data_check_element("suffix_width", "0",
			"usage: the selected width\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ip6_suffix_sel.src_width = tmp;

	cmd_data_check_element("suffix_pos", "0",
			"usage: the position for the selected data to ipv4\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ip6_suffix_sel.dest_pos = tmp;

        rv = __cmd_data_check_boolean("psid1_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.ip6_proto_sel.enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("psid1_start", "0",
			"usage: the start positon to be selected\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ip6_proto_sel.src_start = tmp;

	cmd_data_check_element("psid1_width", "0",
			"usage: the selected width\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ip6_proto_sel.src_width = tmp;

	cmd_data_check_element("proto_type", "zero_data",
			"usage: ipv6 addr type:zero_data, src_port, dst_port\n",
			cmd_data_check_attr, ("proto_type", cmd,
				&entry.proto_src, sizeof(entry.proto_src)));

        rv = __cmd_data_check_boolean("psid2_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.proto_sel.enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("psid2_start", "0",
			"usage: the start positon to be selected\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.proto_sel.src_start = tmp;

	cmd_data_check_element("psid2_width", "0",
			"usage: the selected width\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.proto_sel.src_width = tmp;

        rv = __cmd_data_check_boolean("psid_check_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.check_proto_enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	*(fal_mapt_decap_edit_rule_entry_t *)arg_val = entry;

	return SW_OK;
}

sw_error_t
cmd_data_check_mapt_decap_entry(char *cmd_str, void *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	a_bool_t enable;
	fal_mapt_decap_entry_t entry;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry, sizeof(fal_mapt_decap_entry_t));

        rv = __cmd_data_check_boolean("dst_is_local", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.dst_is_local),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("ipv6_addr", "0::0",
			"usage: the format is xxxx::xxxx \n",
			cmd_data_check_ip6addr, (cmd, &(entry.ip6_addr), 16));

	cmd_data_check_element("prefix_len", "0",
			"usage: ipv6 prefix length\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.ip6_prefix_len = tmp;

        rv = __cmd_data_check_boolean("svlan_check", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.verify_entry.verify_bmp |= FAL_TUNNEL_SVLAN_CHECK_EN;
	else
		entry.verify_entry.verify_bmp &= ~FAL_TUNNEL_SVLAN_CHECK_EN;

	cmd_data_check_element("svlan_fmt", "untag",
			"usage: tag or untag\n",
			cmd_data_check_tag_format,
			(cmd, (a_uint32_t *)&(entry.verify_entry.svlan_fmt),
			 sizeof(a_uint32_t)));

	cmd_data_check_element("svlan_id", "0",
			"usage: svlan id\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.verify_entry.svlan_id = tmp;

        rv = __cmd_data_check_boolean("cvlan_check", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.verify_entry.verify_bmp |= FAL_TUNNEL_CVLAN_CHECK_EN;
	else
		entry.verify_entry.verify_bmp &= ~FAL_TUNNEL_CVLAN_CHECK_EN;

	cmd_data_check_element("cvlan_fmt", "untag",
			"usage: tag or untag\n",
			cmd_data_check_tag_format,
			(cmd, (a_uint32_t *)&(entry.verify_entry.cvlan_fmt),
			 sizeof(a_uint32_t)));

	cmd_data_check_element("cvlan_id", "0",
			"usage: cvlan id\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.verify_entry.cvlan_id = tmp;

        rv = __cmd_data_check_boolean("tl_l3if_check", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &enable,
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	if (enable == A_TRUE)
		entry.verify_entry.verify_bmp |= FAL_TUNNEL_L3IF_CHECK_EN;
	else
		entry.verify_entry.verify_bmp &= ~FAL_TUNNEL_L3IF_CHECK_EN;

	cmd_data_check_element("tl_l3if", "0",
			"usage: tunnel l3 interface\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.verify_entry.tl_l3_if = tmp;

        rv = __cmd_data_check_boolean("src_info_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.src_info_enable),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	do {
		cmd = get_sub_cmd("src_info_type", "vp");
		SW_RTN_ON_NULL_PARAM(cmd);

		if (!strncasecmp(cmd, "quit", 4)) {
			return SW_BAD_VALUE;
		}
		else if (!strncasecmp(cmd, "help", 4)) {
			rv = SW_BAD_VALUE;
		}
		else {
			rv = cmd_data_check_srctype(cmd, 0, &entry.src_info_type,
					sizeof(a_uint8_t));
		}
	} while (talk_mode && (SW_OK != rv));

	cmd_data_check_element("src_info", "0",
			"usage: src info value\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.src_info = tmp;

	cmd_data_check_element("edit_rule_id", "0",
			"usage: mapt rule id\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.edit_rule_id = tmp;

	cmd_data_check_element("exp_profile", "0",
			"usage: exception profile id\n",
			cmd_data_check_uint32, (cmd, &tmp,
				sizeof(a_uint32_t)));
	entry.exp_profile = tmp;

	*(fal_mapt_decap_entry_t *)arg_val = entry;

	return SW_OK;
}
#endif
#ifdef IN_VPORT
sw_error_t
cmd_data_check_vport_state(char *cmd_str, fal_vport_state_t *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv;
	fal_vport_state_t entry;

	aos_mem_zero(&entry, sizeof(fal_vport_state_t));

        rv = __cmd_data_check_boolean("check_en", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.check_en),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	cmd_data_check_element("vport_type", "normal",
			"usage: normal or tunnel\n",
			cmd_data_check_attr, ("vport_type", cmd,
				&entry.vp_type, sizeof(entry.vp_type)));

        rv = __cmd_data_check_boolean("vport_active", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.vp_active),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

        rv = __cmd_data_check_boolean("tunnel_active", "no",
                        "usage: <yes/no/y/n>\n",
                        cmd_data_check_confirm, A_FALSE, &(entry.eg_data_valid),
			sizeof (a_bool_t));
	SW_RTN_ON_ERROR(rv);

	*(fal_vport_state_t *)arg_val = entry;

	return SW_OK;
}

sw_error_t
cmd_data_check_tunnel_action(char *cmd_str, fal_tunnel_action_t *arg_val, a_uint32_t size)
{
	char *cmd;
	sw_error_t rv = SW_OK;
	fal_tunnel_action_t entry_action;
	a_uint32_t tmp = 0;

	aos_mem_zero(&entry_action, sizeof(fal_tunnel_action_t));

	cmd_data_check_element("update_fields", "0",
			"usage: such as updating svlan & cvlan, input 3 "
			"bit0: SVLAN_UPDATE "
			"bit1: CVLAN_UPDATE "
			"bit2: L3IF_UPDATE "
			"bit3: DECAP_UPDATE "
			"bit4: DEACCE_UPDATE "
			"bit5: SRCINFO_UPDATE "
			"bit6: PKT_MODE_UPDATE"
			"bit7: SERVICE_CODE_UPDATE"
			"bit8: UDP_CSUM_ZERO_UPDATE"
			"bit9: EXP_PROFILE_UPDATE"
			"bit10: FWD_CMD_UPDATE\n",
			cmd_data_check_uint32, (cmd, &tmp, sizeof(a_uint32_t)));

	rv = cmd_data_check_tunnel_decap_action_entry(cmd_str,
			&entry_action, sizeof(fal_tunnel_action_t));
	SW_RTN_ON_ERROR(rv);

	/* update the update_bmp field */
	entry_action.update_bmp = tmp;

	*arg_val = entry_action;
	return rv;
}

#endif
