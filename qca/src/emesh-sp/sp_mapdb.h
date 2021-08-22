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

#ifndef SP_MAPDB_H_
#define SP_MAPDB_H_

#include <linux/netdevice.h>
#include <linux/proc_fs.h>
#include <net/dsfield.h>
#include <linux/hashtable.h>

#include "sp_api.h"

#define SP_MAPDB_RULE_MAX			0xFF
#define SP_MAPDB_RULE_MAX_PRECEDENCENUM		0xFF

#define SP_MAPDB_RULE_DEFAULT_PCP 		0

#define SP_MAPDB_ENABLE_PRE_ROUTING_HOOK		0x1
#define SP_MAPDB_ENABLE_POST_ROUTING_HOOK		0x2

/*
 * sp_mapdb_rule_output_types
 *	Types of rule_output value
 */
enum sp_mapdb_rule_output_types {
	SP_MAPDB_USE_DSCP = 0x08,	/* Apply DSCP value */
	SP_MAPDB_USE_UP,		/* Apply UP value */
	SP_MAPDB_NO_MATCH,		/* No rule match (Apply default PCP) */
};
typedef enum sp_mapdb_rule_output_types sp_mapdb_rule_output_type_t;

/*
 * sp_mapdb_rule_node
 * 	This is an element in the double linked list pointed by prec_map[i].
 */
struct sp_mapdb_rule_node {
	struct sp_rule rule;		/* The actual rule content. */
	struct list_head rule_list;	/* Stores the list of sp_mapdb_rule_node with the same precedence */
	struct rcu_head rcu;		/* rcu head of the rule node. */
};

typedef void (*sp_mapdb_rule_update_callback_t)(uint8_t add_remove_modify,
						uint32_t valid_flag,
						struct sp_rule *r);

/*
 * sp_mapdb_rule_id_hashentry
 * 	This struct stores hashentry map whose entries will form a double linkedlist.
 */
struct sp_mapdb_rule_id_hashentry {

	/*
	 * pointing to the rule_node
	 * in the prec_map storing the actual rule info.
	 */
	struct sp_mapdb_rule_node *rule_node;

	struct hlist_node hlist;
};

/*
 * sp_mapdb_rule_manager
 * 	This is a static struct stored rule database information.

 *	It is refered globally containing information about rule prec_map and hash_map
 *	that index rules by their precedence and ruleid respectively.
 */
struct sp_mapdb_rule_manager {
	struct sp_mapdb_rule_node prec_map[SP_MAPDB_RULE_MAX_PRECEDENCENUM];	/* Stores all the rules with precedence. */

	/*
	 * A Linux hlist structure storing SP_MAXPDB_RULE_MAX hash buckets.
	 */
	struct hlist_head rule_id_hashmap[SP_MAPDB_RULE_MAX];			/* Rule id hash map. */
	int rule_count;								/* The number of rules. */
};

void sp_mapdb_ruletable_print(void);
void sp_mapdb_ruletable_flush(void);

void sp_mapdb_init(void);
void sp_mapdb_apply(struct sk_buff *skb, uint8_t *smac, uint8_t *dmac);
void sp_mapdb_fini(void);

int sp_hook_init(void);
void sp_hook_fini(void);

#endif /* SP_MAPDB_H_ */
