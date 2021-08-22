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

#include <linux/kernel.h>
#include <linux/netfilter.h>
#include <linux/if_vlan.h>
#include <linux/netdevice.h>
#include <linux/version.h>
#include <linux/netfilter_bridge.h>

#include "sp_mapdb.h"
#include "sp_types.h"

#define SP_HOOK_IEEE19051_CMDU 0x893A

extern int sp_sysctl_data;

/*
 * sp_hook_pre_routing()
 * 	Hook function on the PREROUTING hook point.
 *
 * It applies Service Prioritization Rules on every packet
 * it receives except for vlan tagged packet
 * to avoid double applying SP rules on backhaul traffic.
 */
unsigned int sp_hook_pre_routing(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct ethhdr *precheck;

	if (sp_sysctl_data & SP_MAPDB_ENABLE_PRE_ROUTING_HOOK) {

		precheck = eth_hdr(skb);
		if ((precheck->h_proto == SP_HOOK_IEEE19051_CMDU) || skb->vlan_proto) {
			return NF_ACCEPT;
		}

		DEBUG_TRACE("%px: Prerouting bridge\n%pM\nto: %pM\n", skb, precheck->h_source, precheck->h_dest);

		sp_mapdb_apply(skb, precheck->h_source, precheck->h_dest);
	}

	return NF_ACCEPT;
}

/*
 * sp_hook_post_routing()
 * 	Hook function on the POSTROUTING hook point.
 *
 * It will be compiled and registered only when we want downlink traffic
 * to be applied to SP rules too.
 */
#if defined(SP_DOWNLINK_APPLICABLE)
unsigned int sp_hook_post_routing(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct ethhdr *precheck;

	if (sp_sysctl_data & SP_MAPDB_ENABLE_POST_ROUTING_HOOK) {

		precheck = eth_hdr(skb);
		if (precheck->h_proto == SP_HOOK_IEEE19051_CMDU || skb->vlan_proto) {
			return NF_ACCEPT;
		}

		DEBUG_TRACE("%px: Postrouting bridge\n%pM\nto: %pM\n", skb, precheck->h_source, precheck->h_dest);

		/*
		 * We will apply the SP rules if the incoming interface
		 * of this packet is different and the outgoing interface.
		 */
		if (skb->skb_iif != skb->dev->ifindex) {
			sp_mapdb_apply(skb, precheck->h_source, precheck->h_dest);
		}
	}

	return NF_ACCEPT;
}
#endif

struct nf_hook_ops sp_hook_ops[] __read_mostly = {
	{
		.pf = NFPROTO_BRIDGE,
		.priority = 1,
		.hooknum = NF_BR_PRE_ROUTING,
		.hook = sp_hook_pre_routing,
	}
#if defined(SP_DOWNLINK_APPLICABLE)
	,
	{
		.pf = NFPROTO_BRIDGE,
		.priority = 1,
		.hooknum = NF_BR_POST_ROUTING,
		.hook = sp_hook_post_routing,
	}
#endif
};

/*
 * sp_hook_init()
 * 	Function registers the hook function on hook point(s).
 *
 * This function is invoked when Service Prioritization module
 * enabled through procfs. (Not invoked when sp is loaded but not
 * enabled through procfs.)
 */
int sp_hook_init(void)
{
	int ret;

	DEBUG_INFO("==Service Prioritization(SP) Hook init==\n");
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0))
	ret = nf_register_hooks(sp_hook_ops, ARRAY_SIZE(sp_hook_ops));
#else
	ret = nf_register_net_hooks(&init_net, sp_hook_ops, ARRAY_SIZE(sp_hook_ops));
#endif
	if (ret < 0) {
		DEBUG_ERROR("SP: Can't register sp_hook\n");
	}
	return ret;
}

/*
 * sp_hook_fini()
 *	 Function unregisters the hook function on hook point(s).
 *
 * This function is invoked when Service Prioritization module
 * is unloaded or disabled through procfs.
 */
void sp_hook_fini(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0))
	nf_unregister_hooks(sp_hook_ops, ARRAY_SIZE(sp_hook_ops));
#else
	nf_unregister_net_hooks(&init_net, sp_hook_ops, ARRAY_SIZE(sp_hook_ops));
#endif
}
