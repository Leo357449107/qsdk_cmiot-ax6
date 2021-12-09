/*
 * Copyright (c) 2014, 2016-2018, 2021, The Linux Foundation. All rights reserved.
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
 * @defgroup fal_acl FAL_ACL
 * @{
 */
#ifndef _FAL_ACL_H_
#define _FAL_ACL_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "sw.h"
#include "fal/fal_type.h"
#include "fal_tunnel.h"
#include "fal_vport.h"

    /**
    @brief This enum defines the ACL rule type.
    */
    typedef enum {
        FAL_ACL_RULE_MAC = 0,   /**< include MAC, udf fields*/
        FAL_ACL_RULE_IP4,       /**< include MAC, IP4 and Tcp/Udp udf fields*/
        FAL_ACL_RULE_IP6,       /**< include MAC, IP6 and Tcp/Udp udf fields*/
        FAL_ACL_RULE_UDF,       /**< only include user defined fields*/
        FAL_ACL_RULE_TUNNEL_MAC,  /**<include tunnel packet outer MAC, udf fields*/
        FAL_ACL_RULE_TUNNEL_IP4,  /**<include tunnel packet outer MAC, IP4 and Tcp/Udp udf fields*/
        FAL_ACL_RULE_TUNNEL_IP6,  /**<include tunnel packet outer MAC, IP6 and Tcp/Udp udf fields*/
        FAL_ACL_RULE_TUNNEL_UDF,  /**<only include tunnel outer udf fields*/
        FAL_ACL_RULE_BUTT,
    }
    fal_acl_rule_type_t;


    /**
    @brief This enum defines the ACL field operation type.
    */
    typedef enum
    {
        FAL_ACL_FIELD_MASK = 0, /**< match operation is mask*/
        FAL_ACL_FIELD_RANGE,    /**< match operation is range*/
        FAL_ACL_FIELD_LE,       /**< match operation is less than equal*/
        FAL_ACL_FIELD_GE,       /**< match operation is greater than equal*/
        FAL_ACL_FIELD_NE,       /**<- match operation is not equal*/
        FAL_ACL_FIELD_OP_BUTT,
    } fal_acl_field_op_t;


    typedef enum
    {
        FAL_ACL_POLICY_ROUTE = 0,
        FAL_ACL_POLICY_SNAT,
        FAL_ACL_POLICY_DNAT,
        FAL_ACL_POLICY_SNAPT,
        FAL_ACL_POLICY_DNAPT,
        FAL_ACL_POLICY_RESERVE,
    } fal_policy_forward_t;

    typedef enum
    {
        FAL_ACL_COMBINED_NONE = 0,
        FAL_ACL_COMBINED_START,
        FAL_ACL_COMBINED_CONTINUE,
        FAL_ACL_COMBINED_END,
    } fal_combined_t;

    /**
    @brief This enum defines the ACL field operation type.
    */
    typedef enum
    {
        FAL_ACL_UDF_TYPE_L2 = 0, /*start from L2 */
        FAL_ACL_UDF_TYPE_L3,     /*start from L3 */
        FAL_ACL_UDF_TYPE_L4,    /*start from L4 */
        FAL_ACL_UDF_TYPE_L2_SNAP, /*start from SNAP L2 */
        FAL_ACL_UDF_TYPE_L3_PLUS, /*start from SNAP L3 */
        FAL_ACL_UDF_TYPE_BUTT,
    } fal_acl_udf_type_t;

    /**
    @brief This enum defines the ACL rule type.
    */
    typedef enum {
        FAL_ACL_UDF_NON_IP = 0, /*UDF for non-ip packets*/
        FAL_ACL_UDF_IP4, /*UDF for IPv4 packets*/
        FAL_ACL_UDF_IP6, /*UDF for IPv6 packets*/
        FAL_ACL_UDF_BUTT,
    }fal_acl_udf_pkt_type_t;

    typedef a_uint32_t fal_acl_udf_profile_entry_field_map_t[1];
    typedef struct {
	fal_acl_udf_profile_entry_field_map_t field_flag; /*Indicate which fields are included*/
	fal_l3_type_t l3_type;
	fal_l4_type_t l4_type;
    } fal_acl_udf_profile_entry_t;

typedef enum {
	FAL_ACL_DEST_PORT_BMP = 0, /*dest info is bitmap*/
	FAL_ACL_DEST_NEXTHOP, /*dest info is nexthop*/
	FAL_ACL_DEST_PORT_ID, /*dest info is port id*/
}fal_acl_dest_type_t;

#define FAL_ACL_DEST_OFFSET(type,value) (((type)<<24)|(value))
#define FAL_ACL_DEST_TYPE(dest) (((dest)>>24)&0xff)
#define FAL_ACL_DEST_VALUE(dest) ((dest)&0xffffff)

#define FAL_ACL_UDF_PROFILE_ENTRY_FIELD_L3_TYPE 0
#define FAL_ACL_UDF_PROFILE_ENTRY_FIELD_L4_TYPE 1

#define    FAL_ACL_FIELD_MAC_DA         0
#define    FAL_ACL_FIELD_MAC_SA         1
#define    FAL_ACL_FIELD_MAC_ETHTYPE    2
#define    FAL_ACL_FIELD_MAC_TAGGED     3
#define    FAL_ACL_FIELD_MAC_UP         4
#define    FAL_ACL_FIELD_MAC_VID        5
#define    FAL_ACL_FIELD_IP4_SIP        6
#define    FAL_ACL_FIELD_IP4_DIP        7
#define    FAL_ACL_FIELD_IP6_LABEL      8
#define    FAL_ACL_FIELD_IP6_SIP        9
#define    FAL_ACL_FIELD_IP6_DIP        10
#define    FAL_ACL_FIELD_IP_PROTO       11
#define    FAL_ACL_FIELD_IP_DSCP        12
#define    FAL_ACL_FIELD_L4_SPORT       13
#define    FAL_ACL_FIELD_L4_DPORT       14
#define    FAL_ACL_FIELD_UDF            15
#define    FAL_ACL_FIELD_MAC_CFI        16
#define    FAL_ACL_FIELD_ICMP_TYPE      17
#define    FAL_ACL_FIELD_ICMP_CODE      18
#define    FAL_ACL_FIELD_TCP_FLAG       19
#define    FAL_ACL_FIELD_RIPV1          20
#define    FAL_ACL_FIELD_DHCPV4         21
#define    FAL_ACL_FIELD_DHCPV6         22
#define    FAL_ACL_FIELD_MAC_STAG_VID   23
#define    FAL_ACL_FIELD_MAC_STAG_PRI   24
#define    FAL_ACL_FIELD_MAC_STAG_DEI   25
#define    FAL_ACL_FIELD_MAC_STAGGED    26
#define    FAL_ACL_FIELD_MAC_CTAG_VID   27
#define    FAL_ACL_FIELD_MAC_CTAG_PRI   28
#define    FAL_ACL_FIELD_MAC_CTAG_CFI   29
#define    FAL_ACL_FIELD_MAC_CTAGGED    30
#define    FAL_ACL_FIELD_INVERSE_ALL    31
/*new add for hawkeye*/
#define    FAL_ACL_FIELD_POST_ROURING_EN    32
#define    FAL_ACL_FIELD_RES_CHAIN    	    33
#define    FAL_ACL_FIELD_FAKE_MAC_HEADER    34
#define    FAL_ACL_FIELD_SNAP    35
#define    FAL_ACL_FIELD_ETHERNET    36
#define    FAL_ACL_FIELD_IPV6    37
#define    FAL_ACL_FIELD_IP    38
#define    FAL_ACL_FIELD_VSI    39
#define    FAL_ACL_FIELD_PPPOE_SESSIONID    40
#define    FAL_ACL_FIELD_L3_FRAGMENT    41
#define    FAL_ACL_FIELD_AH_HEADER    42
#define    FAL_ACL_FIELD_ESP_HEADER    43
#define    FAL_ACL_FIELD_MOBILITY_HEADER    44
#define    FAL_ACL_FIELD_FRAGMENT_HEADER    45
#define    FAL_ACL_FIELD_OTHER_EXT_HEADER    46
#define    FAL_ACL_FIELD_L3_TTL    47
#define    FAL_ACL_FIELD_IPV4_OPTION    48
#define    FAL_ACL_FIELD_FIRST_FRAGMENT    49
#define    FAL_ACL_FIELD_L3_LENGTH    50
#define    FAL_ACL_FIELD_VSI_VALID    51
#define    FAL_ACL_FIELD_IP_PKT_TYPE    52

#define    FAL_ACL_FIELD_UDF0            53
#define    FAL_ACL_FIELD_UDF1            54
#define    FAL_ACL_FIELD_UDF2            55
#define    FAL_ACL_FIELD_UDF3            56
/*new add for IPQ95xx*/
#define    FAL_ACL_FIELD_UDFPROFILE      57

#define    FAL_ACL_FIELD_NUM             58


#define    FAL_ACL_ACTION_PERMIT        0
#define    FAL_ACL_ACTION_DENY          1
#define    FAL_ACL_ACTION_REDPT         2
#define    FAL_ACL_ACTION_RDTCPU        3
#define    FAL_ACL_ACTION_CPYCPU        4
#define    FAL_ACL_ACTION_MIRROR        5
#define    FAL_ACL_ACTION_MODIFY_VLAN   6
#define    FAL_ACL_ACTION_NEST_VLAN     7
#define    FAL_ACL_ACTION_REMARK_UP     8
#define    FAL_ACL_ACTION_REMARK_QUEUE  9
#define    FAL_ACL_ACTION_REMARK_STAG_VID     10
#define    FAL_ACL_ACTION_REMARK_STAG_PRI     11
#define    FAL_ACL_ACTION_REMARK_STAG_DEI     12
#define    FAL_ACL_ACTION_REMARK_CTAG_VID     13
#define    FAL_ACL_ACTION_REMARK_CTAG_PRI     14
#define    FAL_ACL_ACTION_REMARK_CTAG_CFI     15
#define    FAL_ACL_ACTION_REMARK_LOOKUP_VID   16
#define    FAL_ACL_ACTION_REMARK_DSCP         17
#define    FAL_ACL_ACTION_POLICER_EN          18
#define    FAL_ACL_ACTION_WCMP_EN             19
#define    FAL_ACL_ACTION_ARP_EN              20
#define    FAL_ACL_ACTION_POLICY_FORWARD_EN   21
#define    FAL_ACL_ACTION_BYPASS_EGRESS_TRANS 22
#define    FAL_ACL_ACTION_MATCH_TRIGGER_INTR  23
/*new add for hawkeye*/
#define    FAL_ACL_ACTION_ENQUEUE_PRI 25
#define    FAL_ACL_ACTION_INT_DP 26
#define    FAL_ACL_ACTION_SERVICE_CODE 27
#define    FAL_ACL_ACTION_CPU_CODE 28
#define    FAL_ACL_ACTION_SYN_TOGGLE 29
#define    FAL_ACL_ACTION_METADATA_EN 30
/*extend action flg*/
/*new add for IPQ95xx*/
#define    FAL_ACL_ACTION_CASCADE   0
#define    FAL_ACL_ACTION_VPN       1
#define    FAL_ACL_ACTION_LEARN_DIS 2

enum{
	FAL_ACL_BYPASS_IN_VLAN_MISS = 0,
	FAL_ACL_BYPASS_SOUCE_GUARD,
	FAL_ACL_BYPASS_MRU_MTU_CHECK,
	FAL_ACL_BYPASS_FLOW_QOS = 4, /*new add for IPQ60xx*/
	FAL_ACL_BYPASS_EG_VSI_MEMBER_CHECK = 8,
	FAL_ACL_BYPASS_EG_VLAN_TRANSLATION,
	FAL_ACL_BYPASS_EG_VLAN_TAG_CTRL = 10,
	FAL_ACL_BYPASS_FDB_LEARNING,
	FAL_ACL_BYPASS_FDB_REFRESH,
	FAL_ACL_BYPASS_L2_SECURITY,/*new address, station move, learn limit, hash full*/
	FAL_ACL_BYPASS_MANAGEMENT_FWD,
	FAL_ACL_BYPASS_L2_FWD = 15,
	FAL_ACL_BYPASS_IN_STP_CHECK,
	FAL_ACL_BYPASS_EG_STP_CHECK,
	FAL_ACL_BYPASS_SOURCE_FILTER,
	FAL_ACL_BYPASS_POLICYER,
	FAL_ACL_BYPASS_L2_EDIT = 20,/*VLAN tag edit*/
	FAL_ACL_BYPASS_L3_EDIT,/*Edit MAC address, PPPoE, IP address, TTL, DSCP, L4 port*/
	FAL_ACL_BYPASS_POST_ACL_CHECK_ROUTING,
	FAL_ACL_BYPASS_PORT_ISOLATION,
	/*new add for IPQ60xx*/
	FAL_ACL_BYPASS_PRE_ACL_QOS,/*ACL pre-routing qos*/
	FAL_ACL_BYPASS_POST_ACL_QOS,/*ACL post-routing qos*/
	FAL_ACL_BYPASS_DSCP_QOS,
	FAL_ACL_BYPASS_PCP_QOS,
	FAL_ACL_BYPASS_PREHEADER_QOS,
	/*new add for IPQ95xx*/
	FAL_ACL_BYPASS_FAKE_MAC_DROP,
	FAL_ACL_BYPASS_TUNL_CONTEXT,
};


    /**
      * @brief This type defines the action in Acl rule.
      *   @details  Comments:
      *  It's a bit map type, we can access it through macro FAL_ACTION_FLG_SET,
      * FAL_ACTION_FLG_CLR and FAL_ACTION_FLG_TST.
    */
    typedef a_uint32_t fal_acl_action_map_t;

#define FAL_ACTION_FLG_SET(flag, action) \
    (flag) |= (0x1UL << (action))

#define FAL_ACTION_FLG_CLR(flag, action) \
    (flag) &= (~(0x1UL << (action)))

#define FAL_ACTION_FLG_TST(flag, action) \
    ((flag) & (0x1UL << (action))) ? 1 : 0


    /**
      * @brief This type defines the field in Acl rule.
      *   @details   Comments:
      *   It's a bit map type, we can access it through macro FAL_FIELD_FLG_SET,
      *   FAL_FIELD_FLG_CLR and FAL_FIELD_FLG_TST.
    */
    typedef a_uint32_t fal_acl_field_map_t[2];

#define FAL_FIELD_FLG_SET(flag, field) \
    ((flag[(field) / 32]) |= (0x1UL << ((field) % 32)))

#define FAL_FIELD_FLG_CLR(flag, field) \
    ((flag[(field) / 32]) &= (~(0x1UL << ((field) % 32))))

#define FAL_FIELD_FLG_TST(flag, field) \
    (((flag[(field) / 32]) & (0x1UL << ((field) % 32))) ? 1 : 0)

#define FAL_ACL_UDF_MAX_LENGTH 16


/**
 *@brief This is tunnel inner packet rule field
 *
*/
typedef struct
{
	fal_acl_rule_type_t rule_type;/*mac, IP4, IP6 ,UDF*/
	fal_acl_field_map_t field_flg;/*Indicate which fields are selected*/

	/* fields of mac rule */
	fal_mac_addr_t src_mac_val;/*source mac*/
	fal_mac_addr_t src_mac_mask;/*source mac mask*/
	fal_mac_addr_t dest_mac_val;/*destination mac*/
	fal_mac_addr_t dest_mac_mask;/*destionation mac mask*/
	a_uint16_t ethtype_val;/*ethernet type*/
	a_uint16_t ethtype_mask;/*ethernet type mask*/

	/* fields of vlan rule*/
	a_uint8_t stagged_val; /*1-untag, 2-pritag, 4-tagged*/
	a_uint8_t stagged_mask;
	a_uint8_t ctagged_val;/*same as stagged define*/
	a_uint8_t ctagged_mask;
	a_uint16_t stag_vid_val;/*stag vlan id*/
	a_uint16_t stag_vid_mask;/*stag vlan id mask*/
	fal_acl_field_op_t stag_vid_op;/*vlan id operation,
					*larger than or smaller than or range or mask*/
	a_uint16_t ctag_vid_val;
	a_uint16_t ctag_vid_mask;
	fal_acl_field_op_t ctag_vid_op;
	a_uint8_t stag_pri_val;/*stag priority*/
	a_uint8_t stag_pri_mask;/*stag priority mask*/
	a_uint8_t ctag_pri_val;
	a_uint8_t ctag_pri_mask;
	a_uint8_t stag_dei_val;/*stag dei*/
	a_uint8_t stag_dei_mask;/*stag dei mask*/
	a_uint8_t ctag_cfi_val;/*ctag cfi*/
	a_uint8_t ctag_cfi_mask;/*ctag cfi mask*/

	/* fields of ip4 rule */
	fal_ip4_addr_t src_ip4_val;/*source ipv4 address*/
	fal_ip4_addr_t src_ip4_mask;/*source ipv4 address mask*/
	fal_ip4_addr_t dest_ip4_val;/*destination ipv4 address*/
	fal_ip4_addr_t dest_ip4_mask;/*destination ipv4 address mask*/

	/* fields of ip6 rule */
	fal_ip6_addr_t src_ip6_val;/*source ipv6 address*/
	fal_ip6_addr_t src_ip6_mask;/*source ipv6 address mask*/
	fal_ip6_addr_t dest_ip6_val;/*destination ipv6 address*/
	fal_ip6_addr_t dest_ip6_mask;/*destination ipv6 address mask*/

	/* fields of ip rule */
	a_uint8_t ip_proto_val;/*IP protocol*/
	a_uint8_t ip_proto_mask;/*IP protocol mask*/
	a_uint8_t ip_dscp_val;/*IP DSCP*/
	a_uint8_t ip_dscp_mask;/*IP DSCP mask*/

	/* fields of layer four */
	a_uint16_t src_l4port_val;/*source L4 port*/
	a_uint16_t src_l4port_mask;/*source L4 port mask*/
	fal_acl_field_op_t src_l4port_op;/*source L4 port operation*/
	a_uint16_t dest_l4port_val;/*destination L4 port*/
	a_uint16_t dest_l4port_mask;/*destination L4 mask*/
	fal_acl_field_op_t dest_l4port_op;/*destination L4 operation*/
	a_uint8_t icmp_type_val;/*ICMP type*/
	a_uint8_t icmp_type_mask;/*ICMP type mask*/
	a_uint8_t icmp_code_val;/*ICMP code*/
	a_uint8_t icmp_code_mask;/*ICMP code mask*/
	a_uint8_t tcp_flag_val;/*tcp flags value*/
	a_uint8_t tcp_flag_mask;/*tcp flags mask*/

	/* fileds of is or not */
	a_bool_t is_ip_val;/*is ip or not*/
	a_uint8_t is_ip_mask;
	a_bool_t is_ipv6_val;/*is ipv6 or ipv4*/
	a_uint8_t is_ipv6_mask;
	a_bool_t is_fake_mac_header_val;/*is fake mac header or not*/
	a_uint8_t is_fake_mac_header_mask;
	a_bool_t is_snap_val;/*is snap or not*/
	a_uint8_t is_snap_mask;
	a_bool_t is_ethernet_val;/*is ethernet or not*/
	a_uint8_t is_ethernet_mask;
	a_bool_t is_fragement_val;/*is fragment or not*/
	a_uint8_t is_fragement_mask;
	a_bool_t is_ah_header_val;/*is ah header or not*/
	a_uint8_t is_ah_header_mask;
	a_bool_t is_esp_header_val;/*is esp header or not*/
	a_uint8_t is_esp_header_mask;
	a_bool_t is_mobility_header_val;/*is mobility header or not*/
	a_uint8_t is_mobility_header_mask;
	a_bool_t is_fragment_header_val;/*is fragment header or not*/
	a_uint8_t is_fragment_header_mask;
	a_bool_t is_other_header_val;/*is other header or not*/
	a_uint8_t is_other_header_mask;
	a_bool_t is_ipv4_option_val;/*is ipv4 option or not*/
	a_uint8_t is_ipv4_option_mask;
	a_bool_t is_first_frag_val;/*is first fragment or not*/
	a_uint8_t is_first_frag_mask;

	/*fields of L2 MISC rule*/
	a_uint16_t pppoe_sessionid;/*pppoe session id*/
	a_uint16_t pppoe_sessionid_mask;/*pppoe session mask*/
	fal_acl_field_op_t icmp_type_code_op;/*icmp type operation*/

	/*fields of IP MISC rule*/
	a_uint8_t l3_ttl;/*L3 TTL,0-ttl 0, 1-ttl 1, 2-ttl 255, 3- ttl other*/
	a_uint8_t l3_ttl_mask;/*L3 TTL mask*/
	fal_acl_field_op_t l3_length_op;/*L3 TTL operation*/
	a_uint16_t l3_length;/*L3 length*/
	a_uint16_t l3_length_mask;/*L3 length mask*/
	a_uint16_t l3_pkt_type;/*l3 packet type, 0-tcp, 1-udp, 3-udp_lite, 5-arp, 7-icmp*/
	a_uint16_t l3_pkt_type_mask;

	/*field of udf*/
	fal_acl_field_op_t udf0_op;/*udf operation*/
	a_uint16_t udf0_val;/*udf value, 2bytes*/
	a_uint16_t udf0_mask;/*udf mask, 2bytes*/
	fal_acl_field_op_t udf1_op;
	a_uint16_t udf1_val;
	a_uint16_t udf1_mask;
	fal_acl_field_op_t udf2_op;
	a_uint16_t udf2_val;
	a_uint16_t udf2_mask;
	a_uint16_t udf3_val;
	a_uint16_t udf3_mask;
	a_uint8_t udfprofile_val;
	a_uint8_t udfprofile_mask;
} fal_acl_rule_field_t;


/**
 *@brief This is tunnel info field
 *
*/
#define    FAL_ACL_FIELD_TUNNEL_TYPE        0
#define    FAL_ACL_FIELD_TUNNEL_INNER_TYPE  1
#define    FAL_ACL_FIELD_TUNNEL_KEY_VALID   2
#define    FAL_ACL_FIELD_TUNNEL_KEY         3
#define    FAL_ACL_FIELD_TUNNEL_DECAP_EN    4
#define    FAL_ACL_FIELD_TUNNEL_INVERSE_ALL 5

typedef a_uint32_t fal_acl_tunnel_field_map_t[1];
typedef struct
{
	fal_acl_tunnel_field_map_t field_flg;/*Indicate which fields are selected*/
	fal_tunnel_type_t tunnel_type;
	a_uint8_t tunnel_type_mask;
	fal_hdr_type_t inner_type;
	a_uint8_t inner_type_mask;
	a_bool_t tunnel_key_valid;
	a_uint8_t tunnel_key_valid_mask;
	a_uint32_t tunnel_key;
	a_uint32_t tunnel_key_mask;
	a_bool_t tunnel_decap_en;
	a_uint8_t tunnel_decap_en_mask;
} fal_acl_tunnel_info_t;

    /**
     * @brief This structure defines the Acl rule.
     *   @details  Fields description:
     *
     *
     *    vid_val - If vid_op equals FAL_ACL_FIELD_MASK it's vlan id field value.
     *     If vid_op equals FAL_ACL_FIELD_RANGE it's vlan id field low value. If
     *     vid_op equals other value it's the compared value.
     *
     *     vid_mask - If vid_op equals FAL_ACL_FIELD_MASK it's vlan id field mask.
     *     If vid_op equals FAL_ACL_FIELD_RANGE it's vlan id field high value. If vid_op
     *     equals other value it's meaningless.
     *
     *
     *     ip_dscp_val - It's eight bits field we can set any value between 0 - 255.
     *     ip_dscp_mask - It's eight bits field we can set any value between 0 - 255.
     *
     *
     *     src_l4port_val - If src_l4port_op equals FAL_ACL_FIELD_MASK it's layer four
     *     source port field value. If src_l4port_op equals FAL_ACL_FIELD_RANGE it's
     *     layer four source port field low value. If src_l4port_op equals other value
     *     it's the compared value.
     *
     *
     *     src_l4port_mask - If src_l4port_op equals FAL_ACL_FIELD_MASK it's layer four
     *     source port field mask. If src_l4port_op equals FAL_ACL_FIELD_RANGE it's
     *     layer four source port field high value. If src_l4port_op equals other value
     *     it's meaningless.
     *
     *
     *     dest_l4port_val - If dest_l4port_op equals FAL_ACL_FIELD_MASK it's layer four
     *     destination port field value. If dest_l4port_op equals FAL_ACL_FIELD_RANGE it's
     *     layer four source port field low value. If dest_l4port_op equals other value
     *     it's the compared value.
     *
     *
     *     dest_l4port_mask - If dest_l4port_op equals FAL_ACL_FIELD_MASK it's layer four
     *     source port field mask. If dest_l4port_op equals FAL_ACL_FIELD_RANGE it's
     *     layer four source port field high value. If dest_l4port_op equals other value
     *     it's meaningless.
     *
     *
     *     ports - If FAL_ACL_ACTION_REDPT bit is setted in action_flg it's redirect
     *     destination ports.
     *
     *
     *     dot1p - If FAL_ACL_ACTION_REMARK_DOT1P bit is setted in action_flg it's
     *     the expected dot1p value.
     *
     *
     *     queue - If FAL_ACL_ACTION_REMARK_QUEUE bit is setted in action_flg it's
     *     the expected queue value.
     *
     *
     *     vid - If FAL_ACL_ACTION_MODIFY_VLAN or FAL_ACL_ACTION_NEST_VLAN bit is
     *     setted in action_flg it's the expected vlan id value.
     */
typedef struct
{
	fal_acl_rule_type_t rule_type;/*mac, IP4, IP6 and UDF*/
	fal_acl_field_map_t field_flg;/*Indicate which fields are selected*/

	/* fields of mac rule */
	fal_mac_addr_t     src_mac_val;/*source mac*/
	fal_mac_addr_t     src_mac_mask;/*source mac mask*/
	fal_mac_addr_t     dest_mac_val;/*destination mac*/
	fal_mac_addr_t     dest_mac_mask;/*destionation mac mask*/
	a_uint16_t         ethtype_val;/*ethernet type*/
	a_uint16_t         ethtype_mask;/*ethernet type mask*/
	a_uint16_t         vid_val;/*vlan id, IPQ807x not support*/
	a_uint16_t         vid_mask;/*vlan id mask*/
	fal_acl_field_op_t vid_op;/*vlan id operation, larger than or smaller than or range or mask*/
	a_uint8_t          tagged_val;/*tagged or not, IPQ807x not support*/
	a_uint8_t          tagged_mask;/*tagged or not mask*/
	a_uint8_t          up_val;/*cos priority, IPQ807x not support*/
	a_uint8_t          up_mask;/*cos priority mask*/
	a_uint8_t          cfi_val;/*CFI value, IPQ807x not support*/
	a_uint8_t          cfi_mask;/*CFI value mask*/
	a_uint16_t         resv0;/*reserved*/

	/* fields of enhanced mac rule*/
	a_uint8_t          stagged_val; /*for s17c : 0-untag, 1-tag, for hawkeye: 2-pritag, 3-utag+pritag, 4- untag+tag, 5-tag+pritag, 6-all*/
	a_uint8_t          stagged_mask;
	a_uint8_t          ctagged_val;/*same as stagged define*/
	a_uint8_t          ctagged_mask;
	a_uint16_t         stag_vid_val;/*stag vlan id*/
	a_uint16_t         stag_vid_mask;/*stag vlan id mask*/
	fal_acl_field_op_t stag_vid_op;/*vlan id operation, larger than or smaller than or range or mask*/
	a_uint16_t         ctag_vid_val;
	a_uint16_t         ctag_vid_mask;
	fal_acl_field_op_t ctag_vid_op;
	a_uint8_t          stag_pri_val;/*stag priority*/
	a_uint8_t          stag_pri_mask;/*stag priority mask*/
	a_uint8_t          ctag_pri_val;
	a_uint8_t          ctag_pri_mask;
	a_uint8_t          stag_dei_val;/*stag dei*/
	a_uint8_t          stag_dei_mask;/*stag dei mask*/
	a_uint8_t          ctag_cfi_val;/*ctag cfi*/
	a_uint8_t          ctag_cfi_mask;/*ctag cfi mask*/

	/* fields of ip4 rule */
	fal_ip4_addr_t      src_ip4_val;/*source ipv4 address*/
	fal_ip4_addr_t      src_ip4_mask;/*source ipv4 address mask*/
	fal_ip4_addr_t      dest_ip4_val;/*destination ipv4 address*/
	fal_ip4_addr_t      dest_ip4_mask;/*destination ipv4 address mask*/

	/* fields of ip6 rule */
	a_uint32_t         ip6_lable_val;/*ipv6 flow lable, IPQ807x not support*/
	a_uint32_t         ip6_lable_mask;/*ipv6 flow lable mask*/
	fal_ip6_addr_t      src_ip6_val;/*source ipv6 address*/
	fal_ip6_addr_t      src_ip6_mask;/*source ipv6 address mask*/
	fal_ip6_addr_t      dest_ip6_val;/*destination ipv6 address*/
	fal_ip6_addr_t      dest_ip6_mask;/*destination ipv6 address mask*/

	/* fields of ip rule */
	a_uint8_t          ip_proto_val;/*IP protocal*/
	a_uint8_t          ip_proto_mask;/*IP protocal mask*/
	a_uint8_t          ip_dscp_val;/*IP DSCP*/
	a_uint8_t          ip_dscp_mask;/*IP DSCP mask*/

	/* fields of layer four */
	a_uint16_t         src_l4port_val;/*source L4 port*/
	a_uint16_t         src_l4port_mask;/*source L4 port mask*/
	fal_acl_field_op_t src_l4port_op;/*source L4 port operation*/
	a_uint16_t         dest_l4port_val;/*destination L4 port*/
	a_uint16_t         dest_l4port_mask;/*destination L4 mask*/
	fal_acl_field_op_t dest_l4port_op;/*destination L4 operation*/
	a_uint8_t          icmp_type_val;/*ICMP type*/
	a_uint8_t          icmp_type_mask;/*ICMP type mask*/
	a_uint8_t          icmp_code_val;/*ICMP code*/
	a_uint8_t          icmp_code_mask;/*ICMP code mask*/
	a_uint8_t          tcp_flag_val;/*tcp flags value*/
	a_uint8_t          tcp_flag_mask;/*tcp flags mask*/
	a_uint8_t          ripv1_val;/*Is RIPv1 or not, IPQ807x not support*/
	a_uint8_t          ripv1_mask;/*RIPv1 mask*/
	a_uint8_t          dhcpv4_val;/*Is DHCPv4 or not, IPQ807x not support*/
	a_uint8_t          dhcpv4_mask;
	a_uint8_t          dhcpv6_val;/*Is DHCPv6 or not, IPQ807x not support*/
	a_uint8_t          dhcpv6_mask;

	/* user defined fields */
	fal_acl_udf_type_t udf_type;/*UDF type, IPQ807x not support*/
	a_uint8_t udf_offset;/*UDF offset, IPQ807x not support*/
	a_uint8_t udf_len;/*UDF length, IPQ807x not support*/
	a_uint8_t udf_val[FAL_ACL_UDF_MAX_LENGTH];/*UDF field value*/
	a_uint8_t udf_mask[FAL_ACL_UDF_MAX_LENGTH];/*UDF field mask*/

	/* fields of action */
	fal_acl_action_map_t  action_flg;/*Indicate which action apply*/
	fal_pbmp_t            ports; /*high 8bits, 00-port bitmap, 01-nexthop, 10-vp*/
	a_uint32_t            match_cnt;/*rule match frame counter*/
	a_uint16_t            vid;/*modify vlan id, IPQ807x not support*/
	a_uint8_t             up;/*modify COS priority, IPQ807x not support*/
	a_uint8_t             queue;/*modify queue*/
	a_uint16_t            stag_vid;/*modify stag vlan id*/
	a_uint8_t             stag_pri;/*modify stag priority*/
	a_uint8_t             stag_dei;/*modify stag dei*/
	a_uint16_t            ctag_vid;/*modify ctag vlan id*/
	a_uint8_t             ctag_pri;/*modify ctag priority*/
	a_uint8_t             ctag_cfi;/*modify ctag dei*/
	a_uint16_t            policer_ptr;/*specify policer index*/
	a_uint16_t            arp_ptr;/*specify arp table index, IPQ807x not support*/
	a_uint16_t            wcmp_ptr;/*specify wcmp table index, IPQ807x not support*/
	a_uint8_t             dscp;/*modify dscp*/
	a_uint8_t             rsv;
	fal_policy_forward_t  policy_fwd;/*SNAT or DNAT or ROUTE, IPQ807x not support*/
	fal_combined_t    combined;

	/*Only IPQ807x support start*/
	a_uint8_t pri; /*rule priority 0-7*/
	a_bool_t post_routing;/*post routing or not*/
	a_uint8_t acl_pool;/*acl pool*/

	a_bool_t is_ip_val;/*is ip or not*/
	a_uint8_t is_ip_mask;
	a_bool_t is_ipv6_val;/*is ipv6 or ipv4*/
	a_uint8_t is_ipv6_mask;
	a_bool_t is_fake_mac_header_val;/*is fake mac header or not*/
	a_uint8_t is_fake_mac_header_mask;
	a_bool_t is_snap_val;/*is snap or not*/
	a_uint8_t is_snap_mask;
	a_bool_t is_ethernet_val;/*is ethernet or not*/
	a_uint8_t is_ethernet_mask;

	a_bool_t is_fragement_val;/*is fragment or not*/
	a_uint8_t is_fragement_mask;

	a_bool_t is_ah_header_val;/*is ah header or not*/
	a_uint8_t is_ah_header_mask;

	a_bool_t is_esp_header_val;/*is esp header or not*/
	a_uint8_t is_esp_header_mask;

	a_bool_t is_mobility_header_val;/*is mobility header or not*/
	a_uint8_t is_mobility_header_mask;

	a_bool_t is_fragment_header_val;/*is fragment header or not*/
	a_uint8_t is_fragment_header_mask;

	a_bool_t is_other_header_val;/*is other header or not*/
	a_uint8_t is_other_header_mask;

	a_bool_t is_ipv4_option_val;/*is ipv4 option or not*/
	a_uint8_t is_ipv4_option_mask;

	a_bool_t is_first_frag_val;/*is first fragment or not*/
	a_uint8_t is_first_frag_mask;

	/*fields of VLAN rule*/
	a_bool_t vsi_valid;/*vsi valid or not*/
	a_uint8_t vsi_valid_mask;
	a_uint8_t vsi; /*vsi value 0-31*/
	a_uint8_t vsi_mask; /*vsi mask 0-31*/
	/*fields of L2 MISC rule*/
	a_uint16_t pppoe_sessionid;/*pppoe session id*/
	a_uint16_t pppoe_sessionid_mask;/*pppoe session mask*/
	fal_acl_field_op_t icmp_type_code_op;/*icmp type operation*/
	/*fields of IP MISC rule*/
	a_uint8_t l3_ttl;/*L3 TTL,0-ttl 0, 1-ttl 1, 2-ttl 255, 3- ttl other*/
	a_uint8_t l3_ttl_mask;/*L3 TTL mask*/
	fal_acl_field_op_t l3_length_op;/*L3 TTL operation*/
	a_uint16_t l3_length;/*L3 length*/
	a_uint16_t l3_length_mask;/*L3 length mask*/
	a_uint16_t l3_pkt_type;/*l3 packet type, 0-tcp, 1-udp, 3-udp_lite, 5-arp, 7-icmp, 4-gre*/
	a_uint16_t l3_pkt_type_mask;
	/*field of udf*/
	fal_acl_field_op_t udf0_op;/*udf operation*/
	a_uint16_t udf0_val;/*udf value, 2bytes*/
	a_uint16_t udf0_mask;/*udf mask, 2bytes*/
	fal_acl_field_op_t udf1_op;
	a_uint16_t udf1_val;
	a_uint16_t udf1_mask;
	a_uint16_t udf2_val;
	a_uint16_t udf2_mask;
	a_uint16_t udf3_val;
	a_uint16_t udf3_mask;

	/*new add acl action for hawkeye*/
	a_uint32_t bypass_bitmap;/*bypass bitmap*/
	a_uint8_t enqueue_pri;/*enqueue priority*/
	a_uint8_t stag_fmt;/*stag format*/
	a_uint8_t ctag_fmt;/*ctag format*/
	a_uint8_t int_dp;/*internal dp*/
	a_uint8_t service_code;/*service code*/
	a_uint8_t cpu_code;/*cpu code*/
	a_uint64_t match_bytes;/*rule match bytes counter*/
	/*Only IPQ807x support End*/

	/*new add acl action for IPQ60xx*/
	a_uint8_t dscp_mask;/*modify dscp mask,IPQ60xx support*/
	a_uint8_t qos_res_prec;/*qos res prec,IPQ60xx support*/

	/*new add acl udf for IPQ95xx*/
	fal_acl_field_op_t udf2_op;
	a_uint8_t udfprofile_val;
	a_uint8_t udfprofile_mask;
	/*new add acl action for IPQ95xx*/
	fal_acl_action_map_t  action_flg_ext;
	a_uint8_t cascade_data;
	a_uint8_t vpn_type; /*0 vsi; 1 vrf*/
	a_uint8_t vpn_id;
	a_uint16_t napt_l4_port; /*l4 port for NAPT*/
	a_uint16_t policy_id; /*policy id used for tunnel encapsulation*/

	/*new add acl rule fields for IPQ95xx*/
	fal_acl_tunnel_info_t  tunnel_info; /*tunnel info fields*/
	fal_acl_rule_field_t   inner_rule_field; /*tunnel inner packet rule fileds*/
} fal_acl_rule_t;


    /**
    @brief This enum defines the ACL will work on which derection traffic.
    */
    typedef enum
    {
        FAL_ACL_DIREC_IN = 0,   /**<   Acl will work on ingressive traffic */
        FAL_ACL_DIREC_EG,       /**<   Acl will work on egressive traffic */
        FAL_ACL_DIREC_BOTH,     /**<    Acl will work on both ingressive and egressive traffic*/
    } fal_acl_direc_t;


    /**
    @brief This enum defines the ACL will work on which partiualr object.
    */
    typedef enum
    {
        FAL_ACL_BIND_PORT = 0,  /**<   Acl wil work on particular port and virtual port */
        FAL_ACL_BIND_PORTBITMAP = 1,  /**<   Acl wil work on port bitmap */
        FAL_ACL_BIND_SERVICE_CODE = 2,  /**<   Acl wil work on service code */
        FAL_ACL_BIND_L3_IF = 3,  /**<   Acl wil work on l3 interface */
        FAL_ACL_BIND_VP_GROUP =4, /**<  Acl will work on vp group */
        FAL_ACL_BIND_TUNNEL_L3_IF = 5, /** <  Acl will work on tunnel decap l3 interface */
        FAL_ACL_BIND_TUNNEL_PORT = 6,  /** <  Acl will work on tunnel decap port and virtual port*/
        FAL_ACL_BIND_TUNNEL_VP_GROUP = 7, /** <  Acl will work on tunnel decap vp group*/
        FAL_ACL_BIND_SERVICE_PORTBITMAP = 8, /** <  Acl will work on service port bitmap*/
    } fal_acl_bind_obj_t;

enum
{
	/*acl*/
	FUNC_ACL_LIST_CREAT = 0,
	FUNC_ACL_LIST_DESTROY,
	FUNC_ACL_RULE_ADD,
	FUNC_ACL_RULE_DELETE,
	FUNC_ACL_RULE_QUERY,
	FUNC_ACL_RULE_DUMP,
	FUNC_ACL_LIST_BIND,
	FUNC_ACL_LIST_UNBIND,
	FUNC_ACL_LIST_DUMP,
	FUNC_ACL_UDF_PROFILE_SET,
	FUNC_ACL_UDF_PROFILE_GET,
	FUNC_ACL_UDF_PROFILE_ENTRY_ADD,
	FUNC_ACL_UDF_PROFILE_ENTRY_DEL,
	FUNC_ACL_UDF_PROFILE_ENTRY_GETFIRST,
	FUNC_ACL_UDF_PROFILE_ENTRY_GETNEXT,
	FUNC_ACL_UDF_PROFILE_CFG_SET,
	FUNC_ACL_UDF_PROFILE_CFG_GET,
	FUNC_ACL_VPGROUP_SET,
	FUNC_ACL_VPGROUP_GET,
};


sw_error_t
fal_acl_list_creat(a_uint32_t dev_id, a_uint32_t list_id, a_uint32_t list_pri);

sw_error_t
fal_acl_list_destroy(a_uint32_t dev_id, a_uint32_t list_id);

sw_error_t
fal_acl_rule_add(a_uint32_t dev_id, a_uint32_t list_id, a_uint32_t rule_id, a_uint32_t rule_nr, fal_acl_rule_t * rule);

sw_error_t
fal_acl_rule_delete(a_uint32_t dev_id, a_uint32_t list_id, a_uint32_t rule_id, a_uint32_t rule_nr);

sw_error_t
fal_acl_rule_query(a_uint32_t dev_id, a_uint32_t list_id, a_uint32_t rule_id, fal_acl_rule_t * rule);

sw_error_t
fal_acl_list_bind(a_uint32_t dev_id, a_uint32_t list_id, fal_acl_direc_t direc, fal_acl_bind_obj_t obj_t, a_uint32_t obj_idx);

sw_error_t
fal_acl_list_unbind(a_uint32_t dev_id, a_uint32_t list_id, fal_acl_direc_t direc, fal_acl_bind_obj_t obj_t, a_uint32_t obj_idx);

sw_error_t
fal_acl_status_set(a_uint32_t dev_id, a_bool_t enable);

sw_error_t
fal_acl_status_get(a_uint32_t dev_id, a_bool_t * enable);

sw_error_t
fal_acl_list_dump(a_uint32_t dev_id);

sw_error_t
fal_acl_rule_dump(a_uint32_t dev_id);

sw_error_t
fal_acl_port_udf_profile_set(a_uint32_t dev_id, fal_port_t port_id, fal_acl_udf_type_t udf_type, a_uint32_t offset, a_uint32_t length);
sw_error_t
fal_acl_port_udf_profile_get(a_uint32_t dev_id, fal_port_t port_id, fal_acl_udf_type_t udf_type, a_uint32_t * offset, a_uint32_t * length);

sw_error_t
fal_acl_udf_profile_set(a_uint32_t dev_id, fal_acl_udf_pkt_type_t pkt_type,a_uint32_t udf_idx, fal_acl_udf_type_t udf_type, a_uint32_t offset);

sw_error_t
fal_acl_udf_profile_get(a_uint32_t dev_id, fal_acl_udf_pkt_type_t pkt_type,a_uint32_t udf_idx, fal_acl_udf_type_t *udf_type, a_uint32_t *offset);

sw_error_t
fal_acl_rule_active(a_uint32_t dev_id, a_uint32_t list_id, a_uint32_t rule_id, a_uint32_t rule_nr);
sw_error_t
fal_acl_rule_deactive(a_uint32_t dev_id, a_uint32_t list_id, a_uint32_t rule_id, a_uint32_t rule_nr);
sw_error_t
fal_acl_rule_src_filter_sts_set(a_uint32_t dev_id, a_uint32_t rule_id, a_bool_t enable);
sw_error_t
fal_acl_rule_src_filter_sts_get(a_uint32_t dev_id, a_uint32_t rule_id, a_bool_t* enable);

sw_error_t
fal_acl_udf_profile_entry_add(a_uint32_t dev_id, a_uint32_t profile_id,
		fal_acl_udf_profile_entry_t * entry);

sw_error_t
fal_acl_udf_profile_entry_del(a_uint32_t dev_id, a_uint32_t profile_id,
		fal_acl_udf_profile_entry_t * entry);

sw_error_t
fal_acl_udf_profile_entry_getfirst(a_uint32_t dev_id, a_uint32_t profile_id,
		fal_acl_udf_profile_entry_t * entry);

sw_error_t
fal_acl_udf_profile_entry_getnext(a_uint32_t dev_id, a_uint32_t profile_id,
		fal_acl_udf_profile_entry_t * entry);

sw_error_t
fal_acl_udf_profile_cfg_set(a_uint32_t dev_id, a_uint32_t profile_id,
		a_uint32_t udf_idx, fal_acl_udf_type_t udf_type, a_uint32_t offset);

sw_error_t
fal_acl_udf_profile_cfg_get(a_uint32_t dev_id, a_uint32_t profile_id,
		a_uint32_t udf_idx, fal_acl_udf_type_t * udf_type, a_uint32_t * offset);

sw_error_t
fal_acl_vpgroup_set(a_uint32_t dev_id, a_uint32_t vport_id,
		fal_vport_type_t vport_type, a_uint32_t vpgroup_id);

sw_error_t
fal_acl_vpgroup_get(a_uint32_t dev_id, a_uint32_t vport_id,
                fal_vport_type_t vport_type, a_uint32_t * vpgroup_id);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _FAL_ACL_H_ */
/**
 * @}
 */
