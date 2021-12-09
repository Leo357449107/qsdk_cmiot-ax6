/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#ifndef _SP_API_H
#define _SP_API_H

#include <linux/types.h>

/*
 * sp_rule_match_flag
 * 	Rule match mask
 */
#define	SP_RULE_FLAG_MATCH_ALWAYS_TRUE		0x01		/* Rule match always true. s(skip field matching) mask */
#define	SP_RULE_FLAG_MATCH_UP			0x02		/* Match up in 802.11 qos control mask */
#define	SP_RULE_FLAG_MATCH_UP_SENSE		0x04 		/* UP in 802.11 qos control match sense mask */
#define	SP_RULE_FLAG_MATCH_SOURCE_MAC		0x08		/* Match source mac address mask */
#define	SP_RULE_FLAG_MATCH_SOURCE_MAC_SENSE	0x10		/* Match source mac address mask */
#define	SP_RULE_FLAG_MATCH_DST_MAC		0x20		/* Match destination mac address mask */
#define	SP_RULE_FLAG_MATCH_DST_MAC_SENSE	0x40		/* Destination mac address match sense mask */
#define	SP_RULE_FLAG_MATCH_SRC_IPV4		0x80		/* Match source ipv4 address match mask */
#define	SP_RULE_FLAG_MATCH_SRC_IPV4_SENSE	0x100		/* Source ipv4 address sense mask */
#define	SP_RULE_FLAG_MATCH_SRC_IPV6		0x200		/* Match source ipv6 address match mask */
#define	SP_RULE_FLAG_MATCH_SRC_IPV6_SENSE	0x400		/* Source ipv6 address sense mask */
#define	SP_RULE_FLAG_MATCH_DST_IPV4		0x800		/* Match destination ipv4 address match mask */
#define	SP_RULE_FLAG_MATCH_DST_IPV4_SENSE	0x1000		/* Destination ipv4 address sense mask */
#define	SP_RULE_FLAG_MATCH_DST_IPV6		0x2000		/* Match destination ipv6 address match mask */
#define	SP_RULE_FLAG_MATCH_DST_IPV6_SENSE	0x4000		/* Destination ipv6 address sense mask */
#define	SP_RULE_FLAG_MATCH_SRC_PORT		0x8000		/* Match source port match mask */
#define	SP_RULE_FLAG_MATCH_SRC_PORT_SENSE	0x10000		/* Source port sense mask */
#define	SP_RULE_FLAG_MATCH_DST_PORT		0x20000		/* Match destination port match mask */
#define	SP_RULE_FLAG_MATCH_DST_PORT_SENSE	0x40000		/* Destination port sense mask */
#define	SP_RULE_FLAG_MATCH_PROTOCOL		0x80000		/* Match protocol match mask */
#define	SP_RULE_FLAG_MATCH_PROTOCOL_SENSE	0x100000	/* Protocol sense mask */
#define	SP_RULE_FLAG_MATCH_VLAN_ID		0x200000	/* Match VLAN id match mask */
#define	SP_RULE_FLAG_MATCH_VLAN_ID_SENSE	0x400000	/* VLAN id sense mask */
#define	SP_RULE_FLAG_MATCH_DSCP			0x800000	/* Match dscp match mask */
#define	SP_RULE_FLAG_MATCH_DSCP_SENSE		0x1000000	/* DSCP sense mask */

/*
 * sp_mapdb_update_results
 * 	Result values of rule update.
 */
enum sp_mapdb_update_results {
	SP_MAPDB_UPDATE_RESULT_ERR_TBLFULL, 		/* The rule table is full. */
	SP_MAPDB_UPDATE_RESULT_ERR_TBLEMPTY, 		/* The rule table is empty. */
	SP_MAPDB_UPDATE_RESULT_ERR_ALLOCNODE, 		/* Failed at allocating memory for a rule node. (struct sp_mapdb_rule_node) */
	SP_MAPDB_UPDATE_RESULT_ERR_ALLOCHASH, 		/* Failed at allocating memory for a hashentry. */
	SP_MAPDB_UPDATE_RESULT_ERR_RULENOEXIST, 	/* There is no such rule with the given rule id. */
	SP_MAPDB_UPDATE_RESULT_ERR_UNKNOWNBIT, 		/* Unknown add/remove filter bit. */
	SP_MAPDB_UPDATE_RESULT_ERR_SINGLE_WRITER,	/* Single writer protection violation. */
	SP_MAPDB_UPDATE_RESULT_ERR_INVALIDENTRY,	/* Invalid entry of rule field. */
	SP_MAPDB_UPDATE_RESULT_ERR_NEWRULE_NULLPTR,	/* New rule is a null pointer. */
	SP_MAPDB_UPDATE_RESULT_ERR, 			/* Delimiter */
	SP_MAPDB_UPDATE_RESULT_SUCCESS_ADD, 		/* Successful rule add */
	SP_MAPDB_UPDATE_RESULT_SUCCESS_DELETE, 		/* Successful rule deletion */
	SP_MAPDB_UPDATE_RESULT_SUCCESS_MODIFY, 		/* Successful rule modification */
	SP_MAPDB_UPDATE_RESULT_LAST,
};
typedef enum sp_mapdb_update_results sp_mapdb_update_result_t;

/*
 * sp_mapdb_notify_types
 * 	Types of notification.
 */
enum sp_mapdb_notify_types {
	SP_MAPDB_ADD_RULE,				/* Notify rule has been added. */
	SP_MAPDB_REMOVE_RULE,				/* Notify rule has been removed. */
	SP_MAPDB_MODIFY_RULE,				/* Notify rule has been modified. */
};
typedef enum sp_mapdb_notify_types sp_mapdb_notify_type_t;

/*
 * sp_mapdb_add_remove_filter_types
 * 	Possible value of Add-remove filter bit.
 */
enum sp_mapdb_add_remove_filter_types {
	SP_MAPDB_ADD_REMOVE_FILTER_DELETE, 		/* Delete a rule. */
	SP_MAPDB_ADD_REMOVE_FILTER_ADD, 		/* Add a rule. */
};
typedef enum sp_mapdb_add_remove_filter_types sp_mapdb_add_remove_filter_type_t;

struct sp_rule_inner {

	/*
	 * The value of rule_output determines how to set the pcp value
	 * to be marked in the matched packet.
	 */
	uint16_t rule_output;
	uint32_t flags;				/* Flag bits for rule match */
	uint8_t user_priority;			/* UP in 802.11 qos control */

	/*
	 * Source mac address
	 * If “match source mac address” flag bit is set,
	 * this field shall be included, otherwise this field shall be omitted.
	 */
	uint8_t sa[ETH_ALEN];

	/*
	 * Destination mac address
	 * If “match destination mac address” flag
	 * bit is set, this field shall be included,
	 * otherwise this field shall be omitted.
	*/
	uint8_t da[ETH_ALEN];

	/*
	 * Source ipv4 address
	 */
	uint32_t src_ipv4_addr;
	/*
	 * Source ipv6 address
	 */
	uint32_t src_ipv6_addr[4];
	/*
	 * Destination ipv4 address
	 */
	uint32_t dst_ipv4_addr;
	/*
	 * Destination ipv6 address
	 */
	uint32_t dst_ipv6_addr[4];
	/*
	 * Source port number
	 */
	uint16_t src_port;
	/*
	 * Destination port number
	 */
	uint16_t dst_port;
	/*
	 * VLAN id
	 */
	uint16_t vlan_id;
	/*
	 * Protocol number
	 */
	uint8_t protocol_number;
	/*
	 * DSCP value
	 */
	uint8_t dscp;
	/*
	 * Service interval
	 * Specified msec
	 * This is min latency expectation and is used
	 * by Wi-Fi FW for peer tid queue scheduling
	 */
	uint8_t service_interval_dl;
	uint8_t service_interval_ul;
	/*
	 * Burst size
	 * Specified in bytes
	 * This is used by Wi-Fi FW for peer tid queue
	 * scheduling
	 */
	uint32_t burst_size_dl;
	uint32_t burst_size_ul;
};

/*
 * sp_rule
 *	This struct stores Service Prioritization rule structure.
 */
struct sp_rule {
	u_int32_t id;						/* Service prioritization rule identifier */
	sp_mapdb_add_remove_filter_type_t cmd;			/* Command type. 1 means add 0 means delete. */
	struct sp_rule_inner inner;				/* Inner structure */
	uint8_t rule_precedence;				/* Rule precedence – higher number means higher priority. */
};

sp_mapdb_update_result_t sp_mapdb_rule_update(struct sp_rule*);

void sp_mapdb_get_wlan_latency_params(struct sk_buff *skb, uint8_t *service_interval_dl, uint32_t *burst_size_dl, uint8_t *service_interval_ul, uint32_t *burst_size_ul, uint8_t *smac, uint8_t *dmac);
void sp_mapdb_apply(struct sk_buff *skb, uint8_t *smac, uint8_t *dmac);
int sp_mapdb_rule_update_register_notify(void (*sp_mapdb_rule_update_callback)(uint8_t add_rm_md, uint32_t valid_flag, struct sp_rule *r));
void sp_mapdb_rule_update_unregister_notify(void);
void sp_mapdb_ruletable_flush(void);
#endif
