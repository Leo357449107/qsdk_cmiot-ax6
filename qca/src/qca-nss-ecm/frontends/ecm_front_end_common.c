/*
 **************************************************************************
 * Copyright (c) 2015, 2016, 2020-2021, The Linux Foundation.  All rights reserved.
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
 **************************************************************************
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/inet.h>
#include <linux/etherdevice.h>
#include <linux/inetdevice.h>
#include <linux/sysctl.h>
#include <net/netfilter/nf_conntrack.h>
#ifdef ECM_CLASSIFIER_DSCP_ENABLE
#include <linux/netfilter/xt_dscp.h>
#include <net/netfilter/nf_conntrack_dscpremark_ext.h>
#endif
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/addrconf.h>
#include <net/gre.h>

/*
 * Debug output levels
 * 0 = OFF
 * 1 = ASSERTS / ERRORS
 * 2 = 1 + WARN
 * 3 = 2 + INFO
 * 4 = 3 + TRACE
 */
#define DEBUG_LEVEL ECM_FRONT_END_COMMON_DEBUG_LEVEL

#include "ecm_types.h"
#include "ecm_db_types.h"
#include "ecm_state.h"
#include "ecm_tracker.h"
#include "ecm_front_end_types.h"
#include "ecm_classifier.h"
#include "ecm_tracker_datagram.h"
#include "ecm_tracker_udp.h"
#include "ecm_tracker_tcp.h"
#include "ecm_db.h"
#include "ecm_front_end_common.h"
#include "ecm_interface.h"

/*
 * Sysctl table header
 */
static struct ctl_table_header *ecm_front_end_ctl_tbl_hdr;

/*
 * Flag to limit the number of DB connections at any point to the maximum number
 * that can be accelerated by NSS. This may need to be enabled for low memory
 * platforms to control memory allocated by ECM databases.
 */
unsigned int ecm_front_end_conn_limit = 0;

#ifdef ECM_INTERFACE_BOND_ENABLE
/*
 * ecm_front_end_bond_notifier_stop()
 */
void ecm_front_end_bond_notifier_stop(int num)
{
	if (ECM_FRONT_END_TYPE_NSS == ecm_front_end_type_get()) {
		ecm_nss_bond_notifier_stop(num);
	}
}

/*
 * ecm_front_end_bond_notifier_init()
 */
int ecm_front_end_bond_notifier_init(struct dentry *dentry)
{
	if (ECM_FRONT_END_TYPE_NSS == ecm_front_end_type_get()) {
		return ecm_nss_bond_notifier_init(dentry);
	}

	return 0;
}

/*
 * ecm_front_end_bond_notifier_exit()
 */
void ecm_front_end_bond_notifier_exit(void)
{
	if (ECM_FRONT_END_TYPE_NSS == ecm_front_end_type_get()) {
		ecm_nss_bond_notifier_exit();
	}
}
#endif

/*
 * ecm_front_end_gre_proto_is_accel_allowed()
 * 	Handle the following GRE cases:
 *
 * 1. PPTP locally terminated - allow acceleration
 * 2. PPTP pass through - do not allow acceleration
 * 3. GRE V4 or V6 TAP - allow acceleration
 * 4. GRE V4 or V6 TUN - allow acceleration
 * 5. NVGRE locally terminated - do not allow acceleration
 * 6. NVGRE pass through - do not allow acceleration
 * 7. GRE pass through - allow acceleration
 */
bool ecm_front_end_gre_proto_is_accel_allowed(struct net_device *indev,
							     struct net_device *outdev,
							     struct sk_buff *skb,
							     struct nf_conntrack_tuple *tuple,
							     int ip_version)
{
	struct net_device *dev;
	struct gre_base_hdr *greh;

	skb_pull(skb, sizeof(struct iphdr));
	greh = (struct gre_base_hdr *)(skb->data);
	skb_push(skb, sizeof(struct iphdr));

	if ((greh->flags & GRE_VERSION) == ECM_GRE_VERSION_1) {
		/*
		 * Case 1: PPTP locally terminated
		 */
		if (ecm_interface_is_pptp(skb, outdev)) {
			DEBUG_TRACE("%px: PPTP GRE locally terminated - allow acceleration\n", skb);
			return true;
		}

		/*
		 * Case 2: PPTP pass through
		 */
		DEBUG_TRACE("%px: PPTP GRE pass through - do not allow acceleration\n", skb);
		return false;
	}

	if ((greh->flags & GRE_VERSION) != ECM_GRE_VERSION_0) {
		DEBUG_WARN("%px: Unknown GRE version - do not allow acceleration\n", skb);
		return false;
	}

	/*
	 * Case 3: GRE V4 or V6 TAP
	 */
	if ((indev->priv_flags_ext & (IFF_EXT_GRE_V4_TAP | IFF_EXT_GRE_V6_TAP))
		|| (outdev->priv_flags_ext & (IFF_EXT_GRE_V4_TAP | IFF_EXT_GRE_V6_TAP))) {
#ifdef ECM_INTERFACE_GRE_TAP_ENABLE
		DEBUG_TRACE("%px: GRE IPv%d TAP flow - allow acceleration\n", skb, ip_version);
		return true;
#else
		DEBUG_TRACE("%px: GRE IPv%d TAP feature is disabled - do not allow acceleration\n", skb, ip_version);
		return false;
#endif
	}

	/*
	 * Case 4: GRE V4 or V6 TUN
	 */
	if ((indev->type == ARPHRD_IPGRE) || (outdev->type == ARPHRD_IPGRE)
		|| (indev->type == ARPHRD_IP6GRE) || (outdev->type == ARPHRD_IP6GRE)) {
#ifdef ECM_INTERFACE_GRE_TUN_ENABLE
		DEBUG_TRACE("%px: GRE IPv%d TUN flow - allow acceleration\n", skb, ip_version);
		return true;
#else
		DEBUG_TRACE("%px: GRE IPv%d TUN feature is disabled - do not allow acceleration\n", skb, ip_version);
		return false;
#endif
	}

	/*
	 * Case 5: NVGRE locally terminated
	 *
	 * Check both source and dest interface.
	 * If either is locally terminated, we cannot accelerate.
	 */
	if (ip_version == 4) {
		dev = ip_dev_find(&init_net, tuple->src.u3.ip);
		if (dev) {
			/*
			 * Source IP address is local
			 */
			dev_put(dev);
			DEBUG_TRACE("%px: NVGRE locally terminated (src) - do not allow acceleration\n", skb);
			return false;
		}

		dev = ip_dev_find(&init_net, tuple->dst.u3.ip);
		if (dev) {
			/*
			 * Destination IP address is local
			 */
			dev_put(dev);
			DEBUG_TRACE("%px: NVGRE locally terminated (dest) - do not allow acceleration\n", skb);
			return false;
		}
	} else {
#ifdef ECM_IPV6_ENABLE
		dev = ipv6_dev_find(&init_net, &(tuple->src.u3.in6), 1);
		if (dev) {
			/*
			 * Source IP address is local
			 */
			dev_put(dev);
			DEBUG_TRACE("%px: NVGRE locally terminated (src) - do not allow acceleration\n", skb);
			return false;
		}

		dev = ipv6_dev_find(&init_net, &(tuple->dst.u3.in6), 1);
		if (dev) {
			/*
			 * Destination IP address is local
			 */
			dev_put(dev);
			DEBUG_TRACE("%px: NVGRE locally terminated (dest) - do not allow acceleration\n", skb);
			return false;
		}
#else
			DEBUG_TRACE("%px: IPv6 support not enabled\n", skb);
			return false;
#endif
	}

	/*
	 * Case 6: NVGRE pass through
	 */
	if (greh->flags & GRE_KEY) {
		DEBUG_TRACE("%px: NVGRE pass through - do not allow acceleration\n", skb);
		return false;
	}

	/*
	 * Case 7: GRE pass through
	 */
	DEBUG_TRACE("%px: GRE IPv%d pass through - allow acceleration\n", skb, ip_version);
	return true;
}

#ifdef ECM_CLASSIFIER_DSCP_ENABLE
/*
 * ecm_front_end_tcp_set_dscp_ext()
 *	Sets the DSCP remark extension.
 */
void ecm_front_end_tcp_set_dscp_ext(struct nf_conn *ct,
					      struct ecm_tracker_ip_header *iph,
					      struct sk_buff *skb,
					      ecm_tracker_sender_type_t sender)
{
	struct nf_ct_dscpremark_ext *dscpcte;

	/*
	 * Extract the priority and DSCP from skb during the TCP handshake
	 * and store into ct extension for each direction.
	 */
	spin_lock_bh(&ct->lock);
	dscpcte = nf_ct_dscpremark_ext_find(ct);
	if (dscpcte && ct->proto.tcp.state != TCP_CONNTRACK_ESTABLISHED) {
		if (sender == ECM_TRACKER_SENDER_TYPE_SRC) {
			dscpcte->flow_priority = skb->priority;
			dscpcte->flow_dscp = iph->ds >> XT_DSCP_SHIFT;
			dscpcte->flow_set_flags = NF_CT_DSCPREMARK_EXT_PRIO | NF_CT_DSCPREMARK_EXT_DSCP;
			DEBUG_TRACE("%px: sender: %d flow priority: %d flow dscp: %d flow_set_flags: 0x%x\n",
				    ct, sender, dscpcte->flow_priority, dscpcte->flow_dscp, dscpcte->flow_set_flags);
		} else {
			dscpcte->reply_priority =  skb->priority;
			dscpcte->reply_dscp = iph->ds >> XT_DSCP_SHIFT;
			dscpcte->return_set_flags = NF_CT_DSCPREMARK_EXT_PRIO | NF_CT_DSCPREMARK_EXT_DSCP;
			DEBUG_TRACE("%px: sender: %d reply priority: %d reply dscp: %d return_set_flags: 0x%x\n",
				    ct, sender, dscpcte->reply_priority, dscpcte->reply_dscp, dscpcte->return_set_flags);
		}
	}
	spin_unlock_bh(&ct->lock);
}
#endif

/*
 * ecm_front_end_fill_ovs_params()
 *	Set the OVS flow lookup parameters.
 *
 */
void ecm_front_end_fill_ovs_params(struct ecm_front_end_ovs_params ovs_params[],
					ip_addr_t ip_src_addr, ip_addr_t ip_src_addr_nat,
					ip_addr_t ip_dest_addr, ip_addr_t ip_dest_addr_nat,
					int src_port, int src_port_nat,
					int dest_port, int dest_port_nat, ecm_db_direction_t ecm_dir)
{
	DEBUG_INFO("ip_src_addr " ECM_IP_ADDR_DOT_FMT "\n", ECM_IP_ADDR_TO_DOT(ip_src_addr));
	DEBUG_INFO("ip_src_addr_nat " ECM_IP_ADDR_DOT_FMT "\n", ECM_IP_ADDR_TO_DOT(ip_src_addr_nat));
	DEBUG_INFO("ip_dest_addr " ECM_IP_ADDR_DOT_FMT "\n", ECM_IP_ADDR_TO_DOT(ip_dest_addr));
	DEBUG_INFO("ip_dest_addr_nat " ECM_IP_ADDR_DOT_FMT "\n", ECM_IP_ADDR_TO_DOT(ip_dest_addr_nat));
	DEBUG_INFO("src_port: %d, src_port_nat: %d, dest_port:%d, dest_port_nat:%d\n", src_port, src_port_nat, dest_port, dest_port_nat);

	/*
	 * A routed flow can go through ECM in 4 different NAT cases.
	 *
	 * 1. SNAT:
	 * PC1 -------------> eth1-ovs-br1--->ovs-br2-eth2 -------------> PC2
	 *			FROM		TO
	 *					TO_NAT
	 *					FROM_NAT
	 *
	 * ip_src_addr				ip_src_addr_nat		ip_dest_addr/ip_dest_addr_nat
	 *
	 *
	 * 2. DNAT:
	 * PC1 <------------- eth1-ovs-br1<---ovs-br2-eth2 <------------- PC2
	 *			TO		FROM
	 *					FROM_NAT
	 *					TO_NAT
	 *
	 * ip_dest_addr				ip_dest_addr_nat	ip_src_addr/ip_src_addr_nat
	 *
	 *
	 * 3. Non-NAT - Egress:
	 * PC1 -------------> eth1-ovs-br1--->ovs-br2-eth2 -------------> PC2
	 *			FROM		TO
	 *			FROM_NAT	TO_NAT
	 *
	 * ip_src_addr/ip_src_addr_nat					ip_dest_addr/ip_dest_addr_nat
	 *
	 *
	 * 4. Non-NAT - Ingress:
	 * PC1 <------------- eth1-ovs-br1<---ovs-br2-eth2 <------------- PC2
	 *			TO		FROM
	 *			TO_NAT		FROM_NAT
	 *
	 * ip_dest_addr/ip_dest_addr_nat				ip_src_addr/ip_src_addr_nat
	 */

	/*
	 * To look-up the FROM and TO ports, in all NAT cases we use the same IP/port combinations.
	 */
	ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM].src_ip, ip_dest_addr_nat);
	ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM].dest_ip, ip_src_addr);
	ovs_params[ECM_DB_OBJ_DIR_FROM].src_port = dest_port_nat;
	ovs_params[ECM_DB_OBJ_DIR_FROM].dest_port = src_port;

	ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO].src_ip, ip_src_addr_nat);
	ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO].dest_ip, ip_dest_addr);
	ovs_params[ECM_DB_OBJ_DIR_TO].src_port = src_port_nat;
	ovs_params[ECM_DB_OBJ_DIR_TO].dest_port = dest_port;

	if (ecm_dir == ECM_DB_DIRECTION_EGRESS_NAT) {
		/*
		 * For SNAT case (EGRESS_NAT), we use specific IP/port combinations to look-up
		 * the FROM_NAT and TO_NAT ports.
		 */
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].src_ip, ip_src_addr_nat);
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].dest_ip, ip_dest_addr_nat);
		ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].src_port = src_port_nat;
		ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].dest_port = dest_port_nat;

		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO_NAT].src_ip, ip_src_addr_nat);
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO_NAT].dest_ip, ip_dest_addr_nat);
		ovs_params[ECM_DB_OBJ_DIR_TO_NAT].src_port = src_port_nat;
		ovs_params[ECM_DB_OBJ_DIR_TO_NAT].dest_port = dest_port_nat;
	} else if (ecm_dir == ECM_DB_DIRECTION_INGRESS_NAT) {
		/*
		 * For DNAT case (INGRESS_NAT), we use specific IP/port combinations to look-up
		 * the FROM_NAT and TO_NAT ports.
		 */
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].src_ip, ip_dest_addr_nat);
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].dest_ip, ip_src_addr_nat);
		ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].src_port = dest_port_nat;
		ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].dest_port = src_port_nat;

		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO_NAT].src_ip, ip_dest_addr_nat);
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO_NAT].dest_ip, ip_src_addr_nat);
		ovs_params[ECM_DB_OBJ_DIR_TO_NAT].src_port = dest_port_nat;
		ovs_params[ECM_DB_OBJ_DIR_TO_NAT].dest_port = src_port_nat;
	} else {
		/*
		 * For Non-NAT case, we use the same IP/port combinations of FROM and TO to look-up
		 * the FROM_NAT and TO_NAT ports.
		 */
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].src_ip, ip_dest_addr_nat);
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].dest_ip, ip_src_addr);
		ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].src_port = dest_port_nat;
		ovs_params[ECM_DB_OBJ_DIR_FROM_NAT].dest_port = src_port;

		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO_NAT].src_ip, ip_src_addr_nat);
		ECM_IP_ADDR_COPY(ovs_params[ECM_DB_OBJ_DIR_TO_NAT].dest_ip, ip_dest_addr);
		ovs_params[ECM_DB_OBJ_DIR_TO_NAT].src_port = src_port_nat;
		ovs_params[ECM_DB_OBJ_DIR_TO_NAT].dest_port = dest_port;
	}
}

/*
 * ecm_front_end_get_slow_packet_count()
 *	Gets the slow path packet count for the given connection.
 */
uint64_t ecm_front_end_get_slow_packet_count(struct ecm_front_end_connection_instance *feci)
{
	uint64_t slow_pkts;

	spin_lock_bh(&feci->lock);
	slow_pkts = feci->stats.slow_path_packets;
	spin_unlock_bh(&feci->lock);
	return slow_pkts;
}

/*
 * ecm_front_end_db_conn_limit_handler()
 *	Database connection limit sysctl node handler.
 */
int ecm_front_end_db_conn_limit_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;
	int current_value;

	/*
	 * Take the current value
	 */
	current_value = ecm_front_end_conn_limit;

	/*
	 * Write the variable with user input
	 */
	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (ret || (!write)) {
		/*
		 * Return failure.
		 */
		return ret;
	}

	if ((ecm_front_end_conn_limit != 0) &&
			(ecm_front_end_conn_limit != 1)) {
		DEBUG_WARN("Invalid input. Valid values 0/1\n");
		ecm_front_end_conn_limit = current_value;
		return -EINVAL;
	}

	return ret;
}

static struct ctl_table ecm_front_end_conn_limit_tbl[] = {
	{
		.procname	= "front_end_conn_limit",
		.data		= &ecm_front_end_conn_limit,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= &ecm_front_end_db_conn_limit_handler,
	},
	{}
};

static struct ctl_table ecm_front_end_common_root[] = {
	{
		.procname	= "ecm",
		.mode		= 0555,
		.child		= ecm_front_end_conn_limit_tbl,
	},
	{ }
};

static struct ctl_table ecm_front_end_common_root_dir[] = {
	{
		.procname		= "net",
		.mode			= 0555,
		.child			= ecm_front_end_common_root,
	},
	{ }
};

/*
 * ecm_front_end_common_sysctl_register()
 *	Function to register sysctl node during front end init
 */
void ecm_front_end_common_sysctl_register()
{
	/*
	 * Register sysctl table.
	 */
	ecm_front_end_ctl_tbl_hdr = register_sysctl_table(ecm_front_end_common_root_dir);
}

/*
 * ecm_front_end_common_sysctl_unregister()
 *	Function to unregister sysctl node during front end exit
 */
void ecm_front_end_common_sysctl_unregister()
{
	/*
	 * Unregister sysctl table.
	 */
	if (ecm_front_end_ctl_tbl_hdr) {
		unregister_sysctl_table(ecm_front_end_ctl_tbl_hdr);
	}
}
