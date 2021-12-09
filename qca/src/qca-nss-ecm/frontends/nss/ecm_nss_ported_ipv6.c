/*
 **************************************************************************
 * Copyright (c) 2014-2021 The Linux Foundation. All rights reserved.
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
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/icmp.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/pkt_sched.h>
#include <linux/string.h>
#include <net/ip6_route.h>
#include <net/ip6_fib.h>
#include <net/addrconf.h>
#include <net/ipv6.h>
#include <net/tcp.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>	/* for put_user */
#include <net/ipv6.h>
#include <linux/inet.h>
#include <linux/in6.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/inetdevice.h>
#include <linux/if_arp.h>
#include <linux/netfilter_ipv6.h>
#include <linux/netfilter_bridge.h>
#include <linux/if_bridge.h>
#include <net/arp.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_zones.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/ipv6/nf_conntrack_ipv6.h>
#include <net/netfilter/ipv6/nf_defrag_ipv6.h>
#ifdef ECM_INTERFACE_VXLAN_ENABLE
#include <net/vxlan.h>
#endif
#include <linux/netfilter/nf_conntrack_tftp.h>
#ifdef ECM_INTERFACE_VLAN_ENABLE
#include <linux/../../net/8021q/vlan.h>
#include <linux/if_vlan.h>
#endif

/*
 * Debug output levels
 * 0 = OFF
 * 1 = ASSERTS / ERRORS
 * 2 = 1 + WARN
 * 3 = 2 + INFO
 * 4 = 3 + TRACE
 */
#define DEBUG_LEVEL ECM_NSS_PORTED_IPV6_DEBUG_LEVEL

#include <nss_api_if.h>

#ifdef ECM_INTERFACE_IPSEC_ENABLE
#include "nss_ipsec_cmn.h"
#endif

#include "ecm_types.h"
#include "ecm_db_types.h"
#include "ecm_state.h"
#include "ecm_tracker.h"
#include "ecm_classifier.h"
#include "ecm_front_end_types.h"
#include "ecm_tracker_datagram.h"
#include "ecm_tracker_udp.h"
#include "ecm_tracker_tcp.h"
#include "ecm_db.h"
#include "ecm_classifier_default.h"
#include "ecm_interface.h"
#include "ecm_nss_ported_ipv6.h"
#include "ecm_nss_ipv6.h"
#include "ecm_nss_common.h"
#include "ecm_front_end_common.h"

/*
 * Magic numbers
 */
#define ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC 0xEB12

/*
 * Protocol type that ported file supports.
 */
enum ecm_nss_ported_ipv6_proto_types {
	ECM_NSS_PORTED_IPV6_PROTO_TCP = 0,
	ECM_NSS_PORTED_IPV6_PROTO_UDP,
	ECM_NSS_PORTED_IPV6_PROTO_MAX

};

/*
 * struct ecm_nss_ipv6_ported_connection_instance
 *	A connection specific front end instance for PORTED connections
 */
struct ecm_nss_ported_ipv6_connection_instance {
	struct ecm_front_end_connection_instance base;		/* Base class */
	uint8_t ported_accelerated_count_index;			/* Index value of accelerated count array (UDP or TCP) */
#if (DEBUG_LEVEL > 0)
	uint16_t magic;
#endif
};

static int ecm_nss_ported_ipv6_accelerated_count[ECM_NSS_PORTED_IPV6_PROTO_MAX] = {0};
						/* Array of Number of TCP and UDP connections currently offloaded */

/*
 * Expose what should be a static flag in the TCP connection tracker.
 */
#ifdef ECM_OPENWRT_SUPPORT
extern int nf_ct_tcp_no_window_check;
#endif
extern int nf_ct_tcp_be_liberal;

/*
 * ecm_nss_ported_ipv6_connection_callback()
 *	Callback for handling create ack/nack calls.
 */
static void ecm_nss_ported_ipv6_connection_callback(void *app_data, struct nss_ipv6_msg *nim)
{
	struct nss_ipv6_rule_create_msg *nircm = &nim->msg.rule_create;
	uint32_t serial = (uint32_t)(ecm_ptr_t)app_data;
	struct ecm_db_connection_instance *ci;
	struct ecm_front_end_connection_instance *feci;
	struct ecm_nss_ported_ipv6_connection_instance *npci;
	ip_addr_t flow_ip;
	ip_addr_t return_ip;
	ecm_front_end_acceleration_mode_t result_mode;
	bool is_defunct = false;

	/*
	 * Is this a response to a create message?
	 */
	if (nim->cm.type != NSS_IPV6_TX_CREATE_RULE_MSG) {
		DEBUG_ERROR("%px: ported create callback with improper type: %d, serial: %u\n", nim, nim->cm.type, serial);
		return;
	}

	/*
	 * Look up ecm connection so that we can update the status.
	 */
	ci = ecm_db_connection_serial_find_and_ref(serial);
	if (!ci) {
		DEBUG_TRACE("%px: create callback, connection not found, serial: %u\n", nim, serial);
		return;
	}

	/*
	 * Release ref held for this ack/nack response.
	 * NOTE: It's okay to do this here, ci won't go away, because the ci is held as
	 * a result of the ecm_db_connection_serial_find_and_ref()
	 */
	ecm_db_connection_deref(ci);

	/*
	 * Get the front end instance
	 */
	feci = ecm_db_connection_front_end_get_and_ref(ci);
	npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;
	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);

	ECM_NSS_IPV6_ADDR_TO_IP_ADDR(flow_ip, nircm->tuple.flow_ip);
	ECM_NSS_IPV6_ADDR_TO_IP_ADDR(return_ip, nircm->tuple.return_ip);

	/*
	 * Record command duration
	 */
	ecm_nss_ipv6_accel_done_time_update(feci);

	/*
	 * Dump some useful trace information.
	 */
	DEBUG_TRACE("%px: accelerate response for connection: %px, serial: %u\n", npci, feci->ci, serial);
	DEBUG_TRACE("%px: rule_flags: %x, valid_flags: %x\n", npci, nircm->rule_flags, nircm->valid_flags);
	DEBUG_TRACE("%px: flow_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n", npci, ECM_IP_ADDR_TO_OCTAL(flow_ip), nircm->tuple.flow_ident);
	DEBUG_TRACE("%px: return_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n", npci, ECM_IP_ADDR_TO_OCTAL(return_ip), nircm->tuple.return_ident);
	DEBUG_TRACE("%px: protocol: %d\n", npci, nircm->tuple.protocol);

	/*
	 * Handle the creation result code.
	 */
	DEBUG_TRACE("%px: response: %d\n", npci, nim->cm.response);
	if (nim->cm.response != NSS_CMN_RESPONSE_ACK) {
		/*
		 * Creation command failed (specific reason ignored).
		 */
		DEBUG_TRACE("%px: accel nack: %d\n", npci, nim->cm.error);
		spin_lock_bh(&feci->lock);
		DEBUG_ASSERT(feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL_PENDING, "%px: Unexpected mode: %d\n", ci, feci->accel_mode);
		feci->stats.ae_nack++;
		feci->stats.ae_nack_total++;
		if (feci->stats.ae_nack >= feci->stats.ae_nack_limit) {
			/*
			 * Too many NSS rejections
			 */
			result_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_ACCEL_ENGINE;
		} else {
			/*
			 * Revert to decelerated
			 */
			result_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL;
		}

		/*
		 * If connection is now defunct then set mode to ensure no further accel attempts occur
		 */
		if (feci->is_defunct) {
			result_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DEFUNCT;
		}

		spin_lock_bh(&ecm_nss_ipv6_lock);
		_ecm_nss_ipv6_accel_pending_clear(feci, result_mode);
		spin_unlock_bh(&ecm_nss_ipv6_lock);

		spin_unlock_bh(&feci->lock);

		/*
		 * Release the connection.
		 */
		feci->deref(feci);
		ecm_db_connection_deref(ci);
		return;
	}

	spin_lock_bh(&feci->lock);
	DEBUG_ASSERT(feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL_PENDING, "%px: Unexpected mode: %d\n", ci, feci->accel_mode);

	/*
	 * If a flush occured before we got the ACK then our acceleration was effectively cancelled on us
	 * GGG TODO This is a workaround for a NSS message OOO quirk, this should eventually be removed.
	 */
	if (feci->stats.flush_happened) {
		feci->stats.flush_happened = false;

		/*
		 * Increment the no-action counter.  Our connection was decelerated on us with no action occurring.
		 */
		feci->stats.no_action_seen++;

		spin_lock_bh(&ecm_nss_ipv6_lock);
		_ecm_nss_ipv6_accel_pending_clear(feci, ECM_FRONT_END_ACCELERATION_MODE_DECEL);
		spin_unlock_bh(&ecm_nss_ipv6_lock);

		spin_unlock_bh(&feci->lock);

		/*
		 * Release the connection.
		 */
		feci->deref(feci);
		ecm_db_connection_deref(ci);
		return;
	}

	/*
	 * Create succeeded
	 */

	/*
	 * Clear any nack count
	 */
	feci->stats.ae_nack = 0;

	/*
	 * Clear the "accelerate pending" state and move to "accelerated" state bumping
	 * the accelerated counters to match our new state.
	 *
	 * Decelerate may have been attempted while we were "pending accel" and
	 * this function will return true if that was the case.
	 * If decelerate was pending then we need to begin deceleration :-(
	 */
	spin_lock_bh(&ecm_nss_ipv6_lock);

	ecm_nss_ported_ipv6_accelerated_count[npci->ported_accelerated_count_index]++;	/* Protocol specific counter */
	ecm_nss_ipv6_accelerated_count++;		/* General running counter */

	if (!_ecm_nss_ipv6_accel_pending_clear(feci, ECM_FRONT_END_ACCELERATION_MODE_ACCEL)) {
		/*
		 * Increment the no-action counter, this is reset if offload action is seen
		 */
		feci->stats.no_action_seen++;

		spin_unlock_bh(&ecm_nss_ipv6_lock);
		spin_unlock_bh(&feci->lock);

		/*
		 * Release the connection.
		 */
		feci->deref(feci);
		ecm_db_connection_deref(ci);
		return;
	}

	DEBUG_INFO("%px: Decelerate was pending\n", ci);

	/*
	 * Check if the pending decelerate was done with the defunct process.
	 * If it was, set the is_defunct flag of the feci to false for re-try.
	 */
	if (feci->is_defunct) {
		is_defunct = feci->is_defunct;
		feci->is_defunct = false;
	}

	spin_unlock_bh(&ecm_nss_ipv6_lock);
	spin_unlock_bh(&feci->lock);

	/*
	 * If the pending decelerate was done through defunct process, we should
	 * re-try it here with the same defunct function, because the purpose of that
	 * process is to remove the connection from the database as well after decelerating it.
	 */
	if (is_defunct) {
		ecm_db_connection_make_defunct(ci);
	} else {
		feci->decelerate(feci);
	}

	/*
	 * Release the connection.
	 */
	feci->deref(feci);
	ecm_db_connection_deref(ci);
}

/*
 * ecm_nss_ported_ipv6_connection_accelerate()
 *	Accelerate a connection
 */
static void ecm_nss_ported_ipv6_connection_accelerate(struct ecm_front_end_connection_instance *feci,
                                                                        struct ecm_classifier_process_response *pr, bool is_l2_encap,
                                                                        struct nf_conn *ct, struct sk_buff *skb)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;
	uint16_t regen_occurrances;
	int protocol;
	int32_t from_ifaces_first;
	int32_t to_ifaces_first;
	struct ecm_db_iface_instance *from_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX];
	struct ecm_db_iface_instance *to_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX];
	struct ecm_db_iface_instance *from_nss_iface;
	struct ecm_db_iface_instance *to_nss_iface;
	int32_t from_nss_iface_id;
	int32_t to_nss_iface_id;
	uint8_t from_nss_iface_address[ETH_ALEN];
	uint8_t to_nss_iface_address[ETH_ALEN];
	struct nss_ipv6_msg *nim;
	struct nss_ipv6_rule_create_msg *nircm;
	struct ecm_classifier_instance *assignments[ECM_CLASSIFIER_TYPES];
	int aci_index;
	int assignment_count;
	nss_tx_status_t nss_tx_status;
	int32_t list_index;
	int32_t interface_type_counts[ECM_DB_IFACE_TYPE_COUNT];
	bool rule_invalid;
	ip_addr_t src_ip;
	ip_addr_t dest_ip;
	ecm_front_end_acceleration_mode_t result_mode;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);

	/*
	 * Get the re-generation occurrance counter of the connection.
	 * We compare it again at the end - to ensure that the rule construction has seen no generation
	 * changes during rule creation.
	 */
	regen_occurrances = ecm_db_connection_regeneration_occurrances_get(feci->ci);

	/*
	 * Test if acceleration is permitted
	 */
	if (!ecm_nss_ipv6_accel_pending_set(feci)) {
		DEBUG_TRACE("%px: Acceleration not permitted: %px\n", feci, feci->ci);
		return;
	}

	nim = (struct nss_ipv6_msg *)kzalloc(sizeof(struct nss_ipv6_msg), GFP_ATOMIC | __GFP_NOWARN);
	if (!nim) {
		DEBUG_WARN("%px: no memory for nss ipv6 message structure instance: %px\n", feci, feci->ci);
		ecm_nss_ipv6_accel_pending_clear(feci, ECM_FRONT_END_ACCELERATION_MODE_DECEL);
		return;
	}

	/*
	 * Okay construct an accel command.
	 * Initialise creation structure.
	 * NOTE: We leverage the app_data void pointer to be our 32 bit connection serial number.
	 * When we get it back we re-cast it to a uint32 and do a faster connection lookup.
	 */
	nss_ipv6_msg_init(nim, NSS_IPV6_RX_INTERFACE, NSS_IPV6_TX_CREATE_RULE_MSG,
			sizeof(struct nss_ipv6_rule_create_msg),
			ecm_nss_ported_ipv6_connection_callback,
			(void *)(ecm_ptr_t)ecm_db_connection_serial_get(feci->ci));

	nircm = &nim->msg.rule_create;
	nircm->valid_flags = 0;
	nircm->rule_flags = 0;

	/*
	 * Initialize VLAN tag information
	 */
	nircm->vlan_primary_rule.ingress_vlan_tag = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
	nircm->vlan_primary_rule.egress_vlan_tag = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
	nircm->vlan_secondary_rule.ingress_vlan_tag = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
	nircm->vlan_secondary_rule.egress_vlan_tag = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;

	/*
	 * Get the interface lists of the connection, we must have at least one interface in the list to continue
	 */
	from_ifaces_first = ecm_db_connection_interfaces_get_and_ref(feci->ci, from_ifaces, ECM_DB_OBJ_DIR_FROM);
	if (from_ifaces_first == ECM_DB_IFACE_HEIRARCHY_MAX) {
		DEBUG_WARN("%px: Accel attempt failed - no interfaces in from_interfaces list!\n", npci);
		goto ported_accel_bad_rule;
	}

	to_ifaces_first = ecm_db_connection_interfaces_get_and_ref(feci->ci, to_ifaces, ECM_DB_OBJ_DIR_TO);
	if (to_ifaces_first == ECM_DB_IFACE_HEIRARCHY_MAX) {
		DEBUG_WARN("%px: Accel attempt failed - no interfaces in to_interfaces list!\n", npci);
		ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
		goto ported_accel_bad_rule;
	}

	/*
	 * First interface in each must be a known nss interface
	 */
	from_nss_iface = from_ifaces[from_ifaces_first];
	to_nss_iface = to_ifaces[to_ifaces_first];
	from_nss_iface_id = ecm_db_iface_ae_interface_identifier_get(from_nss_iface);
	to_nss_iface_id = ecm_db_iface_ae_interface_identifier_get(to_nss_iface);
	if ((from_nss_iface_id < 0) || (to_nss_iface_id < 0)) {
		DEBUG_TRACE("%px: from_nss_iface_id: %d, to_nss_iface_id: %d\n", npci, from_nss_iface_id, to_nss_iface_id);
		ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
		ecm_db_connection_interfaces_deref(to_ifaces, to_ifaces_first);
		goto ported_accel_bad_rule;
	}

	/*
	 * Get NSS interface ID of the top interface in heirarchy
	 */
	from_nss_iface = from_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX - 1];
	to_nss_iface = to_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX - 1];
	nircm->nexthop_rule.flow_nexthop = ecm_db_iface_ae_interface_identifier_get(from_nss_iface);
	nircm->nexthop_rule.return_nexthop = ecm_db_iface_ae_interface_identifier_get(to_nss_iface);

	/*
	 * New rule being created
	 */
	nircm->valid_flags |= NSS_IPV6_RULE_CREATE_CONN_VALID;

	/*
	 * Set Nexthop interface number valid flag
	 */
	nircm->valid_flags |= NSS_IPV6_RULE_CREATE_NEXTHOP_VALID;

	/*
	 * Set interface numbers involved in accelerating this connection.
	 * These are the outer facing addresses from the heirarchy interface lists we got above.
	 * These may be overridden later if we detect special interface types e.g. ipsec.
	 */
	nircm->conn_rule.flow_interface_num = from_nss_iface_id;
	nircm->conn_rule.return_interface_num = to_nss_iface_id;

	/*
	 * We know that each outward facing interface is known to the NSS and so this connection could be accelerated.
	 * However the lists may also specify other interesting details that must be included in the creation command,
	 * for example, ethernet MAC, VLAN tagging or PPPoE session information.
	 * We get this information by walking from the outer to the innermost interface for each list and examine the interface types.
	 *
	 * Start with the 'from' (src) side.
	 * NOTE: The lists may contain a complex heirarchy of similar type of interface e.g. multiple vlans or tunnels within tunnels.
	 * This NSS cannot handle that - there is no way to describe this in the rule - if we see multiple types that would conflict we have to abort.
	 */
	DEBUG_TRACE("%px: Examine from/src heirarchy list\n", npci);
	memset(interface_type_counts, 0, sizeof(interface_type_counts));
	rule_invalid = false;
	for (list_index = from_ifaces_first; !rule_invalid && (list_index < ECM_DB_IFACE_HEIRARCHY_MAX); list_index++) {
		struct ecm_db_iface_instance *ii;
		ecm_db_iface_type_t ii_type;
		char *ii_name;
#ifdef ECM_INTERFACE_VLAN_ENABLE
		struct ecm_db_interface_info_vlan vlan_info;
		uint32_t vlan_value = 0;
		struct net_device *vlan_in_dev = NULL;
#endif

		ii = from_ifaces[list_index];
		ii_type = ecm_db_iface_type_get(ii);
		ii_name = ecm_db_interface_type_to_string(ii_type);
		DEBUG_TRACE("%px: list_index: %d, ii: %px, type: %d (%s)\n", npci, list_index, ii, ii_type, ii_name);

		/*
		 * Extract information from this interface type if it is applicable to the rule.
		 * Conflicting information may cause accel to be unsupported.
		 */
		switch (ii_type) {
		case ECM_DB_IFACE_TYPE_BRIDGE:
			DEBUG_TRACE("%px: Bridge\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Cannot cascade bridges
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: Bridge - ignore additional\n", npci);
				break;
			}

			ecm_db_iface_bridge_address_get(ii, from_nss_iface_address);
			if (is_valid_ether_addr(from_nss_iface_address)) {
				ether_addr_copy((uint8_t *)nircm->src_mac_rule.flow_src_mac, from_nss_iface_address);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_FLOW_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: Bridge - mac: %pM\n", npci, from_nss_iface_address);
			break;

		case ECM_DB_IFACE_TYPE_OVS_BRIDGE:
#ifdef ECM_INTERFACE_OVS_BRIDGE_ENABLE
			DEBUG_TRACE("%px: OVS Bridge\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Cannot cascade bridges
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: OVS Bridge - ignore additional\n", npci);
				break;
			}

			ecm_db_iface_ovs_bridge_address_get(ii, from_nss_iface_address);
			if (is_valid_ether_addr(from_nss_iface_address)) {
				ether_addr_copy((uint8_t *)nircm->src_mac_rule.flow_src_mac, from_nss_iface_address);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_FLOW_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: OVS Bridge - mac: %pM\n", npci, from_nss_iface_address);
#else
			rule_invalid = true;
#endif
			break;

		case ECM_DB_IFACE_TYPE_ETHERNET:
			DEBUG_TRACE("%px: Ethernet\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Ignore additional mac addresses, these are usually as a result of address propagation
				 * from bridges down to ports etc.
				 */
				DEBUG_TRACE("%px: Ethernet - ignore additional\n", npci);
				break;
			}

			/*
			 * Can only handle one MAC, the first outermost mac.
			 */
			ecm_db_iface_ethernet_address_get(ii, from_nss_iface_address);
			DEBUG_TRACE("%px: Ethernet - mac: %pM\n", npci, from_nss_iface_address);
			break;
		case ECM_DB_IFACE_TYPE_PPPOE:
#ifdef ECM_INTERFACE_PPPOE_ENABLE
			/*
			 * More than one PPPoE in the list is not valid!
			 */
			if (interface_type_counts[ii_type] != 0) {
				DEBUG_TRACE("%px: PPPoE - additional unsupported\n", npci);
				rule_invalid = true;
				break;
			}

			/*
			 * Set the PPPoE rule creation structure.
			 */
			nircm->pppoe_rule.flow_if_num = ecm_db_iface_ae_interface_identifier_get(ii);
			if (nircm->pppoe_rule.flow_if_num < 0) {
				DEBUG_TRACE("%px: PPPoE - acceleration engine flow interface (%d) is not valid\n",
						npci, nircm->pppoe_rule.flow_if_num);
				rule_invalid = true;
				break;
			}
			nircm->pppoe_rule.flow_if_exist = 1;
			nircm->valid_flags |= NSS_IPV6_RULE_CREATE_PPPOE_VALID;

			DEBUG_TRACE("%px: PPPoE - exist: %d flow_if_num: %d\n", npci,
					nircm->pppoe_rule.flow_if_exist,
					nircm->pppoe_rule.flow_if_num);
#else
			rule_invalid = true;
#endif
			break;
		case ECM_DB_IFACE_TYPE_VLAN:
#ifdef ECM_INTERFACE_VLAN_ENABLE
			DEBUG_TRACE("%px: VLAN\n", npci);
			if (interface_type_counts[ii_type] > 1) {
				/*
				 * Can only support two vlans
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: VLAN - additional unsupported\n", npci);
				break;
			}
			ecm_db_iface_vlan_info_get(ii, &vlan_info);
			vlan_value = ((vlan_info.vlan_tpid << 16) | vlan_info.vlan_tag);

			/*
			 * Look up the vlan device and incorporate the vlan priority into the vlan_value
			 */
			vlan_in_dev = dev_get_by_index(&init_net, ecm_db_iface_interface_identifier_get(ii));
			if (vlan_in_dev) {
				vlan_value |= vlan_dev_get_egress_qos_mask(vlan_in_dev, pr->return_qos_tag);
				dev_put(vlan_in_dev);
				vlan_in_dev = NULL;
			}

			/*
			 * Primary or secondary (QinQ) VLAN?
			 */
			if (interface_type_counts[ii_type] == 0) {
				nircm->vlan_primary_rule.ingress_vlan_tag = vlan_value;
			} else {
				nircm->vlan_secondary_rule.ingress_vlan_tag = vlan_value;
			}
			nircm->valid_flags |= NSS_IPV6_RULE_CREATE_VLAN_VALID;

			/*
			 * If we have not yet got an ethernet mac then take this one (very unlikely as mac should have been propagated to the slave (outer) device
			 */
			memcpy(from_nss_iface_address, vlan_info.address, ETH_ALEN);
			if (is_valid_ether_addr(from_nss_iface_address)) {
				DEBUG_TRACE("%px: VLAN use mac: %pM\n", npci, from_nss_iface_address);
				interface_type_counts[ECM_DB_IFACE_TYPE_ETHERNET]++;
				memcpy(nircm->src_mac_rule.flow_src_mac, from_nss_iface_address, ETH_ALEN);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_FLOW_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: vlan tag: %x\n", npci, vlan_value);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: VLAN - unsupported\n", npci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_MACVLAN:
#ifdef ECM_INTERFACE_MACVLAN_ENABLE
			ecm_db_iface_macvlan_address_get(ii, from_nss_iface_address);
			if (is_valid_ether_addr(from_nss_iface_address)) {
				ether_addr_copy((uint8_t *)nircm->src_mac_rule.flow_src_mac, from_nss_iface_address);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_FLOW_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: Macvlan - mac: %pM\n", npci, from_nss_iface_address);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: MACVLAN - unsupported\n", npci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_IPSEC_TUNNEL:
#ifdef ECM_INTERFACE_IPSEC_ENABLE
			DEBUG_TRACE("%px: IPSEC\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Can only support one ipsec
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: IPSEC - additional unsupported\n", npci);
				break;
			}

			nircm->conn_rule.flow_interface_num = ecm_nss_common_ipsec_get_ifnum(from_nss_iface_id);
			nircm->nexthop_rule.flow_nexthop = ecm_nss_common_ipsec_get_ifnum(nircm->nexthop_rule.flow_nexthop);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: IPSEC - unsupported\n", npci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_OVPN:
#ifdef ECM_INTERFACE_OVPN_ENABLE
			DEBUG_TRACE("%px: OVPN interface\n", npci);
			nircm->conn_rule.flow_interface_num = nss_qvpn_ifnum_with_core_id(from_nss_iface_id);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: OVPN - unsupported\n", npci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_VXLAN:
#ifdef ECM_INTERFACE_VXLAN_ENABLE
			DEBUG_TRACE("%px: From VXLAN interface\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Can support only one VxLAN interface.
				 */
				DEBUG_WARN("%px: VxLAN - ignore additional\n", npci);
				rule_invalid = true;
				break;
			}

			/*
			 * For VxLAN device, a 5-tuple connection rule is added with the same src and dest ports in both the directions.
			 * Source interface is a VxLAN interface for the routed flow which is the case for VxLAN->IPsec or VxLAN->WAN rule.
			 * Override the flow MTU to MAX, to avoid fragmentation for flows coming in from WAN.
			 * Note: These rules are always expected to be pushed only in tunnel to WAN direction.
			 */
			if (ecm_db_connection_is_routed_get(feci->ci)) {
				nircm->conn_rule.flow_mtu = ECM_DB_IFACE_MTU_MAX;
				nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_NO_SRC_IDENT;
			}
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: VXLAN - unsupported\n", npci);
#endif
			break;
		default:
			DEBUG_TRACE("%px: Ignoring: %d (%s)\n", npci, ii_type, ii_name);
		}

		/*
		 * Seen an interface of this type
		 */
		interface_type_counts[ii_type]++;
	}

	if (rule_invalid) {
		DEBUG_WARN("%px: from/src Rule invalid\n", npci);
		ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
		ecm_db_connection_interfaces_deref(to_ifaces, to_ifaces_first);
		goto ported_accel_bad_rule;
	}

	/*
	 * Now examine the TO / DEST heirarchy list to construct the destination part of the rule
	 */
	DEBUG_TRACE("%px: Examine to/dest heirarchy list\n", npci);
	memset(interface_type_counts, 0, sizeof(interface_type_counts));
	rule_invalid = false;
	for (list_index = to_ifaces_first; !rule_invalid && (list_index < ECM_DB_IFACE_HEIRARCHY_MAX); list_index++) {
		struct ecm_db_iface_instance *ii;
		ecm_db_iface_type_t ii_type;
		char *ii_name;
#ifdef ECM_INTERFACE_VLAN_ENABLE
		struct ecm_db_interface_info_vlan vlan_info;
		uint32_t vlan_value = 0;
		struct net_device *vlan_out_dev = NULL;
#endif

		ii = to_ifaces[list_index];
		ii_type = ecm_db_iface_type_get(ii);
		ii_name = ecm_db_interface_type_to_string(ii_type);
		DEBUG_TRACE("%px: list_index: %d, ii: %px, type: %d (%s)\n", npci, list_index, ii, ii_type, ii_name);

		/*
		 * Extract information from this interface type if it is applicable to the rule.
		 * Conflicting information may cause accel to be unsupported.
		 */
		switch (ii_type) {
		case ECM_DB_IFACE_TYPE_BRIDGE:
			DEBUG_TRACE("%px: Bridge\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Cannot cascade bridges
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: Bridge - ignore additional\n", npci);
				break;
			}

			ecm_db_iface_bridge_address_get(ii, to_nss_iface_address);
			if (is_valid_ether_addr(to_nss_iface_address)) {
				ether_addr_copy((uint8_t *)nircm->src_mac_rule.return_src_mac, to_nss_iface_address);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_RETURN_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: Bridge - mac: %pM\n", npci, to_nss_iface_address);
			break;

		case ECM_DB_IFACE_TYPE_OVS_BRIDGE:
#ifdef ECM_INTERFACE_OVS_BRIDGE_ENABLE
			DEBUG_TRACE("%px: OVS Bridge\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Cannot cascade bridges
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: OVS Bridge - ignore additional\n", npci);
				break;
			}

			ecm_db_iface_ovs_bridge_address_get(ii, to_nss_iface_address);
			if (is_valid_ether_addr(to_nss_iface_address)) {
				ether_addr_copy((uint8_t *)nircm->src_mac_rule.return_src_mac, to_nss_iface_address);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_RETURN_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: OVS Bridge - mac: %pM\n", npci, to_nss_iface_address);
#else
			rule_invalid = true;
#endif
			break;

		case ECM_DB_IFACE_TYPE_ETHERNET:
			DEBUG_TRACE("%px: Ethernet\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Ignore additional mac addresses, these are usually as a result of address propagation
				 * from bridges down to ports etc.
				 */
				DEBUG_TRACE("%px: Ethernet - ignore additional\n", npci);
				break;
			}

			/*
			 * Can only handle one MAC, the first outermost mac.
			 */
			ecm_db_iface_ethernet_address_get(ii, to_nss_iface_address);
			DEBUG_TRACE("%px: Ethernet - mac: %pM\n", npci, to_nss_iface_address);
			break;
		case ECM_DB_IFACE_TYPE_PPPOE:
#ifdef ECM_INTERFACE_PPPOE_ENABLE
			/*
			 * More than one PPPoE in the list is not valid!
			 */
			if (interface_type_counts[ii_type] != 0) {
				DEBUG_TRACE("%px: PPPoE - additional unsupported\n", npci);
				rule_invalid = true;
				break;
			}

			/*
			 * Set the PPPoE rule creation structure.
			 */
			nircm->pppoe_rule.return_if_num = ecm_db_iface_ae_interface_identifier_get(ii);
			if (nircm->pppoe_rule.return_if_num < 0) {
				DEBUG_TRACE("%px: PPPoE - acceleration engine return interface (%d) is not valid\n",
						npci, nircm->pppoe_rule.return_if_num);
				rule_invalid = true;
				break;
			}
			nircm->pppoe_rule.return_if_exist = 1;
			nircm->valid_flags |= NSS_IPV6_RULE_CREATE_PPPOE_VALID;

			DEBUG_TRACE("%px: PPPoE - exist: %d return_if_num: %d\n", npci,
					nircm->pppoe_rule.return_if_exist,
					nircm->pppoe_rule.return_if_num);
#else
			rule_invalid = true;
#endif
			break;
		case ECM_DB_IFACE_TYPE_VLAN:
#ifdef ECM_INTERFACE_VLAN_ENABLE
			DEBUG_TRACE("%px: VLAN\n", npci);
			if (interface_type_counts[ii_type] > 1) {
				/*
				 * Can only support two vlans
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: VLAN - additional unsupported\n", npci);
				break;
			}
			ecm_db_iface_vlan_info_get(ii, &vlan_info);
			vlan_value = ((vlan_info.vlan_tpid << 16) | vlan_info.vlan_tag);

			/*
			 * Look up the vlan device and incorporate the vlan priority into the vlan_value
			 */
			vlan_out_dev = dev_get_by_index(&init_net, ecm_db_iface_interface_identifier_get(ii));
			if (vlan_out_dev) {
				vlan_value |= vlan_dev_get_egress_qos_mask(vlan_out_dev, pr->flow_qos_tag);
				dev_put(vlan_out_dev);
				vlan_out_dev = NULL;
			}

			/*
			 * Primary or secondary (QinQ) VLAN?
			 */
			if (interface_type_counts[ii_type] == 0) {
				nircm->vlan_primary_rule.egress_vlan_tag = vlan_value;
			} else {
				nircm->vlan_secondary_rule.egress_vlan_tag = vlan_value;
			}
			nircm->valid_flags |= NSS_IPV6_RULE_CREATE_VLAN_VALID;

			/*
			 * If we have not yet got an ethernet mac then take this one (very unlikely as mac should have been propagated to the slave (outer) device
			 */
			memcpy(to_nss_iface_address, vlan_info.address, ETH_ALEN);
			if (is_valid_ether_addr(to_nss_iface_address)) {
				DEBUG_TRACE("%px: VLAN use mac: %pM\n", npci, to_nss_iface_address);
				interface_type_counts[ECM_DB_IFACE_TYPE_ETHERNET]++;
				memcpy(nircm->src_mac_rule.return_src_mac, to_nss_iface_address, ETH_ALEN);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_RETURN_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: vlan tag: %x\n", npci, vlan_value);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: VLAN - unsupported\n", npci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_MACVLAN:
#ifdef ECM_INTERFACE_MACVLAN_ENABLE
			ecm_db_iface_macvlan_address_get(ii, to_nss_iface_address);
			if (is_valid_ether_addr(to_nss_iface_address)) {
				ether_addr_copy((uint8_t *)nircm->src_mac_rule.return_src_mac, to_nss_iface_address);
				nircm->src_mac_rule.mac_valid_flags |= NSS_IPV6_SRC_MAC_RETURN_VALID;
				nircm->valid_flags |= NSS_IPV6_RULE_CREATE_SRC_MAC_VALID;
			}

			DEBUG_TRACE("%px: Macvlan - mac: %pM\n", npci, to_nss_iface_address);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: MACVLAN - unsupported\n", npci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_IPSEC_TUNNEL:
#ifdef ECM_INTERFACE_IPSEC_ENABLE
			DEBUG_TRACE("%px: IPSEC\n", npci);
			if (interface_type_counts[ii_type] != 0) {
				/*
				 * Can only support one ipsec
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: IPSEC - additional unsupported\n", npci);
				break;
			}

			nircm->conn_rule.return_interface_num = ecm_nss_common_ipsec_get_ifnum(to_nss_iface_id);
			nircm->nexthop_rule.return_nexthop = ecm_nss_common_ipsec_get_ifnum(nircm->nexthop_rule.return_nexthop);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: IPSEC - unsupported\n", npci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_OVPN:
#ifdef ECM_INTERFACE_OVPN_ENABLE
			DEBUG_TRACE("%px: OVPN interface\n", npci);
			nircm->conn_rule.return_interface_num = nss_qvpn_ifnum_with_core_id(to_nss_iface_id);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: OVPN - unsupported\n", npci);
#endif
			break;
		default:
			DEBUG_TRACE("%px: Ignoring: %d (%s)\n", npci, ii_type, ii_name);
		}

		/*
		 * Seen an interface of this type
		 */
		interface_type_counts[ii_type]++;
	}

	if (rule_invalid) {
		DEBUG_WARN("%px: to/dest Rule invalid\n", npci);
		ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
		ecm_db_connection_interfaces_deref(to_ifaces, to_ifaces_first);
		goto ported_accel_bad_rule;
	}

	/*
	 * Routed or bridged?
	 */
	if (ecm_db_connection_is_routed_get(feci->ci)) {
		nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_ROUTED;
	} else {
		nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_BRIDGE_FLOW;
		if (is_l2_encap) {
			nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_L2_ENCAP;
		}
	}

	if (ecm_interface_src_check) {
		DEBUG_INFO("%px: Source interface check flag is enabled\n", npci);
		nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_SRC_INTERFACE_CHECK;
	}

	/*
	 * Set up the flow and return qos tags
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_QOS_TAG) {
		nircm->qos_rule.flow_qos_tag = (uint32_t)pr->flow_qos_tag;
		nircm->qos_rule.return_qos_tag = (uint32_t)pr->return_qos_tag;
		nircm->valid_flags |= NSS_IPV6_RULE_CREATE_QOS_VALID;
	}

#ifdef ECM_CLASSIFIER_DSCP_ENABLE
#ifdef ECM_CLASSIFIER_DSCP_IGS
	/*
	 * Set up ingress shaper qostag values.
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_IGS_QOS_TAG) {
		nircm->igs_rule.igs_flow_qos_tag = (uint16_t)pr->igs_flow_qos_tag;
		nircm->igs_rule.igs_return_qos_tag = (uint16_t)pr->igs_return_qos_tag;
		nircm->valid_flags |= NSS_IPV6_RULE_CREATE_IGS_VALID;
	}
#endif
	/*
	 * DSCP information?
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_DSCP) {
		nircm->dscp_rule.flow_dscp = pr->flow_dscp;
		nircm->dscp_rule.return_dscp = pr->return_dscp;
		nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_DSCP_MARKING;
		nircm->valid_flags |= NSS_IPV6_RULE_CREATE_DSCP_MARKING_VALID;
	}
#endif

#ifdef ECM_CLASSIFIER_EMESH_ENABLE
	/*
	 * Mark the rule as E-MESH Service Prioritization valid.
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_EMESH_SP_FLOW) {
		nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_EMESH_SP;
	}
#endif

#ifdef ECM_CLASSIFIER_PCC_ENABLE
	/*
	 * Set up the interfaces for mirroring.
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_MIRROR_ENABLED) {
		if (!ecm_nss_common_fill_mirror_info(pr, &nircm->mirror_rule.flow_ifnum,
					 &nircm->mirror_rule.return_ifnum)) {
			DEBUG_WARN("Invalid Mirror interface information\n");
			ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
			ecm_db_connection_interfaces_deref(to_ifaces, to_ifaces_first);
			goto ported_accel_bad_rule;
		}

		if (nircm->mirror_rule.flow_ifnum != -1) {
			nircm->mirror_rule.valid |= NSS_IPV6_MIRROR_FLOW_VALID;
		}

		if (nircm->mirror_rule.return_ifnum != -1) {
			nircm->mirror_rule.valid |= NSS_IPV6_MIRROR_RETURN_VALID;
		}

		nircm->valid_flags |= NSS_IPV6_RULE_CREATE_MIRROR_VALID;
	}
#endif

	if (ecm_nss_ipv6_vlan_passthrough_enable && !ecm_db_connection_is_routed_get(feci->ci) &&
	   (nircm->vlan_primary_rule.ingress_vlan_tag == ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED) &&
	   (nircm->vlan_primary_rule.egress_vlan_tag == ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED)) {
		int vlan_present = 0;
		vlan_present = skb_vlan_tag_present(skb);
		if (vlan_present) {
			uint32_t vlan_value;
			vlan_value = (ETH_P_8021Q << 16) | skb_vlan_tag_get(skb);
			nircm->vlan_primary_rule.ingress_vlan_tag = vlan_value;
			nircm->vlan_primary_rule.egress_vlan_tag = vlan_value;
			nircm->valid_flags |= NSS_IPV6_RULE_CREATE_VLAN_VALID;
		}
	}

#ifdef ECM_CLASSIFIER_OVS_ENABLE
		/*
		 * Copy the both primary and secondary (if exist) VLAN tags.
		 */
		if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_OVS_VLAN_TAG) {
			nircm->vlan_primary_rule.ingress_vlan_tag = pr->ingress_vlan_tag[0];
			nircm->vlan_primary_rule.egress_vlan_tag = pr->egress_vlan_tag[0];
			nircm->valid_flags |= NSS_IPV6_RULE_CREATE_VLAN_VALID;
		}

		if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_OVS_VLAN_QINQ_TAG) {
			nircm->vlan_secondary_rule.ingress_vlan_tag = pr->ingress_vlan_tag[1];
			nircm->vlan_secondary_rule.egress_vlan_tag = pr->egress_vlan_tag[1];
		}
#endif
	protocol = ecm_db_connection_protocol_get(feci->ci);

	/*
	 * Set protocol
	 */
	nircm->tuple.protocol = (int32_t)protocol;

	/*
	 * The flow_ip is where the connection established from
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, src_ip);
	ECM_IP_ADDR_TO_NSS_IPV6_ADDR(nircm->tuple.flow_ip, src_ip);

	/*
	 * The dest_ip is where the connection is established to
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, dest_ip);
	ECM_IP_ADDR_TO_NSS_IPV6_ADDR(nircm->tuple.return_ip, dest_ip);

	/*
	 * Same approach as above for port information
	 */
	nircm->tuple.flow_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_FROM);
	nircm->tuple.return_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_TO);

	/*
	 * Get mac addresses.
	 * The src_mac is the mac address of the node that established the connection.
	 * This will work whether the from_node is LAN (egress) or WAN (ingress).
	 */
	ecm_db_connection_node_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, (uint8_t *)nircm->conn_rule.flow_mac);

	/*
	 * The dest_mac is more complex.  For egress it is the node address of the 'to' side of the connection.
	 * For ingress it is the node adress of the NAT'ed 'to' IP.
	 * Essentially it is the MAC of node associated with create.dest_ip and this is "to nat" side.
	 */
	ecm_db_connection_node_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, (uint8_t *)nircm->conn_rule.return_mac);

	/*
	 * Get MTU information
	 */
	nircm->conn_rule.flow_mtu = (uint32_t)ecm_db_connection_iface_mtu_get(feci->ci, ECM_DB_OBJ_DIR_FROM);
	nircm->conn_rule.return_mtu = (uint32_t)ecm_db_connection_iface_mtu_get(feci->ci, ECM_DB_OBJ_DIR_TO);

	if (protocol == IPPROTO_TCP) {
		/*
		 * Need window scaling information from conntrack if available
		 * Start by looking up the conntrack connection
		 */
		if (!ct) {
			/*
			 * No conntrack so no need to check window sequence space
			 */
			DEBUG_TRACE("%px: TCP Accel no ct from conn %px to get window data\n", npci, feci->ci);
			nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_NO_SEQ_CHECK;
		} else {
			int flow_dir;
			int return_dir;

			ecm_front_end_flow_and_return_directions_get(ct, src_ip, 6, &flow_dir, &return_dir);

			DEBUG_TRACE("%px: TCP Accel Get window data from ct %px for conn %px\n", npci, ct, feci->ci);
			spin_lock_bh(&ct->lock);
			nircm->tcp_rule.flow_window_scale = ct->proto.tcp.seen[flow_dir].td_scale;
			nircm->tcp_rule.flow_max_window = ct->proto.tcp.seen[flow_dir].td_maxwin;
			nircm->tcp_rule.flow_end = ct->proto.tcp.seen[flow_dir].td_end;
			nircm->tcp_rule.flow_max_end = ct->proto.tcp.seen[flow_dir].td_maxend;
			nircm->tcp_rule.return_window_scale = ct->proto.tcp.seen[return_dir].td_scale;
			nircm->tcp_rule.return_max_window = ct->proto.tcp.seen[return_dir].td_maxwin;
			nircm->tcp_rule.return_end = ct->proto.tcp.seen[return_dir].td_end;
			nircm->tcp_rule.return_max_end = ct->proto.tcp.seen[return_dir].td_maxend;
#ifdef ECM_OPENWRT_SUPPORT
			if (nf_ct_tcp_be_liberal || nf_ct_tcp_no_window_check
#else
			if (nf_ct_tcp_be_liberal
#endif
					|| (ct->proto.tcp.seen[flow_dir].flags & IP_CT_TCP_FLAG_BE_LIBERAL)
					|| (ct->proto.tcp.seen[return_dir].flags & IP_CT_TCP_FLAG_BE_LIBERAL)) {
				nircm->rule_flags |= NSS_IPV6_RULE_CREATE_FLAG_NO_SEQ_CHECK;
			}
			spin_unlock_bh(&ct->lock);
		}

		nircm->valid_flags |= NSS_IPV6_RULE_CREATE_TCP_VALID;
	}

	/*
	 * Sync our creation command from the assigned classifiers to get specific additional creation rules.
	 * NOTE: These are called in ascending order of priority and so the last classifier (highest) shall
	 * override any preceding classifiers.
	 * This also gives the classifiers a chance to see that acceleration is being attempted.
	 */
	assignment_count = ecm_db_connection_classifier_assignments_get_and_ref(feci->ci, assignments);
	for (aci_index = 0; aci_index < assignment_count; ++aci_index) {
		struct ecm_classifier_instance *aci;
		struct ecm_classifier_rule_create ecrc;
		/*
		 * NOTE: The current classifiers do not sync anything to the underlying accel engines.
		 * In the future, if any of the classifiers wants to pass any parameter, these parameters
		 * should be received via this object and copied to the accel engine's create object (nircm).
		*/
#ifdef ECM_CLASSIFIER_EMESH_ENABLE
		ecrc.skb = skb;
#endif
		aci = assignments[aci_index];
		DEBUG_TRACE("%px: sync from: %px, type: %d\n", npci, aci, aci->type_get(aci));
		aci->sync_from_v6(aci, &ecrc);
	}
	ecm_db_connection_assignments_release(assignment_count, assignments);

	/*
	 * Release the interface lists
	 */
	ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
	ecm_db_connection_interfaces_deref(to_ifaces, to_ifaces_first);

	DEBUG_INFO("%px: Ported Accelerate connection %px\n"
			"Protocol: %d\n"
			"from_mtu: %u\n"
			"to_mtu: %u\n"
			"from_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n"
			"to_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n"
			"from_mac: %pM\n"
			"to_mac: %pM\n"
			"src_iface_num: %u\n"
			"dest_iface_num: %u\n"
			"src_nexthop_num: %u\n"
			"dest_nexthop_num: %u\n"
			"ingress_inner_vlan_tag: %x\n"
			"egress_inner_vlan_tag: %x\n"
			"ingress_outer_vlan_tag: %x\n"
			"egress_outer_vlan_tag: %x\n"
			"rule_flags: %x\n"
			"valid_flags: %x\n"
			"pppoe_return_if_exist: %u\n"
			"pppoe_return_if_num: %u\n"
			"pppoe_flow_if_exist: %u\n"
			"pppoe_flow_if_num: %u\n"
			"flow_qos_tag: %x (%u)\n"
			"return_qos_tag: %x (%u)\n"
			"igs_flow_qos_tag: %x (%u)\n"
			"igs_return_qos_tag: %x (%u)\n"
			"flow_dscp: %x\n"
			"return_dscp: %x\n",
			npci,
			feci->ci,
			nircm->tuple.protocol,
			nircm->conn_rule.flow_mtu,
			nircm->conn_rule.return_mtu,
			ECM_IP_ADDR_TO_OCTAL(src_ip), nircm->tuple.flow_ident,
			ECM_IP_ADDR_TO_OCTAL(dest_ip), nircm->tuple.return_ident,
			nircm->conn_rule.flow_mac,
			nircm->conn_rule.return_mac,
			nircm->conn_rule.flow_interface_num,
			nircm->conn_rule.return_interface_num,
			nircm->nexthop_rule.flow_nexthop,
			nircm->nexthop_rule.return_nexthop,
			nircm->vlan_primary_rule.ingress_vlan_tag,
			nircm->vlan_primary_rule.egress_vlan_tag,
			nircm->vlan_secondary_rule.ingress_vlan_tag,
			nircm->vlan_secondary_rule.egress_vlan_tag,
			nircm->rule_flags,
			nircm->valid_flags,
			nircm->pppoe_rule.return_if_exist,
			nircm->pppoe_rule.return_if_num,
			nircm->pppoe_rule.flow_if_exist,
			nircm->pppoe_rule.flow_if_num,
			nircm->qos_rule.flow_qos_tag, nircm->qos_rule.flow_qos_tag,
			nircm->qos_rule.return_qos_tag, nircm->qos_rule.return_qos_tag,
			nircm->igs_rule.igs_flow_qos_tag, nircm->igs_rule.igs_flow_qos_tag,
			nircm->igs_rule.igs_return_qos_tag, nircm->igs_rule.igs_return_qos_tag,
			nircm->dscp_rule.flow_dscp,
			nircm->dscp_rule.return_dscp);

	if (protocol == IPPROTO_TCP) {
		DEBUG_INFO("flow_window_scale: %u\n"
			"flow_max_window: %u\n"
			"flow_end: %u\n"
			"flow_max_end: %u\n"
			"return_window_scale: %u\n"
			"return_max_window: %u\n"
			"return_end: %u\n"
			"return_max_end: %u\n",
			nircm->tcp_rule.flow_window_scale,
			nircm->tcp_rule.flow_max_window,
			nircm->tcp_rule.flow_end,
			nircm->tcp_rule.flow_max_end,
			nircm->tcp_rule.return_window_scale,
			nircm->tcp_rule.return_max_window,
			nircm->tcp_rule.return_end,
			nircm->tcp_rule.return_max_end);
	}

	/*
	 * Now that the rule has been constructed we re-compare the generation occurrance counter.
	 * If there has been a change then we abort because the rule may have been created using
	 * unstable data - especially if another thread has begun regeneration of the connection state.
	 * NOTE: This does not prevent a regen from being flagged immediately after this line of code either,
	 * or while the acceleration rule is in flight to the nss.
	 * This is only to check for consistency of rule state - not that the state is stale.
	 * Remember that the connection is marked as "accel pending state" so if a regen is flagged immediately
	 * after this check passes, the connection will be decelerated and refreshed very quickly.
	 */
	if (regen_occurrances != ecm_db_connection_regeneration_occurrances_get(feci->ci)) {
		DEBUG_INFO("%px: connection:%px regen occurred - aborting accel rule.\n", feci, feci->ci);
		ecm_nss_ipv6_accel_pending_clear(feci, ECM_FRONT_END_ACCELERATION_MODE_DECEL);
		kfree(nim);
		return;
	}

	/*
	 * Ref the connection before issuing an NSS rule
	 * This ensures that when the NSS responds to the command - which may even be immediately -
	 * the callback function can trust the correct ref was taken for its purpose.
	 * NOTE: remember that this will also implicitly hold the feci.
	 */
	ecm_db_connection_ref(feci->ci);

	/*
	 * We are about to issue the command, record the time of transmission
	 */
	spin_lock_bh(&feci->lock);
	feci->stats.cmd_time_begun = jiffies;
	spin_unlock_bh(&feci->lock);

	/*
	 * Call the rule create function
	 */
	nss_tx_status = nss_ipv6_tx(ecm_nss_ipv6_nss_ipv6_mgr, nim);
	if (nss_tx_status == NSS_TX_SUCCESS) {
		/*
		 * Reset the driver_fail count - transmission was okay here.
		 */
		spin_lock_bh(&feci->lock);
		feci->stats.driver_fail = 0;
		spin_unlock_bh(&feci->lock);
		kfree(nim);
		return;
	}

	kfree(nim);

	/*
	 * Release that ref!
	 */
	ecm_db_connection_deref(feci->ci);

	/*
	 * TX failed
	 */
	spin_lock_bh(&feci->lock);
	DEBUG_ASSERT(feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL_PENDING, "%px: Accel mode unexpected: %d\n", npci, feci->accel_mode);
	feci->stats.driver_fail_total++;
	feci->stats.driver_fail++;
	if (feci->stats.driver_fail >= feci->stats.driver_fail_limit) {
		DEBUG_WARN("%px: Accel failed - driver fail limit\n", npci);
		result_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DRIVER;
	} else {
		result_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL;
	}

	spin_lock_bh(&ecm_nss_ipv6_lock);
	_ecm_nss_ipv6_accel_pending_clear(feci, result_mode);
	spin_unlock_bh(&ecm_nss_ipv6_lock);

	spin_unlock_bh(&feci->lock);
	return;

ported_accel_bad_rule:
	;

	kfree(nim);

	/*
	 * Jump to here when rule data is bad and an offload command cannot be constructed
	 */
	DEBUG_WARN("%px: Accel failed - bad rule\n", npci);
	ecm_nss_ipv6_accel_pending_clear(feci, ECM_FRONT_END_ACCELERATION_MODE_FAIL_RULE);
}

/*
 * ecm_nss_ported_ipv6_connection_destroy_callback()
 *	Callback for handling destroy ack/nack calls.
 */
static void ecm_nss_ported_ipv6_connection_destroy_callback(void *app_data, struct nss_ipv6_msg *nim)
{
	struct nss_ipv6_rule_destroy_msg *nirdm = &nim->msg.rule_destroy;
	uint32_t serial = (uint32_t)(ecm_ptr_t)app_data;
	struct ecm_db_connection_instance *ci;
	struct ecm_front_end_connection_instance *feci;
	struct ecm_nss_ported_ipv6_connection_instance *npci;
	ip_addr_t flow_ip;
	ip_addr_t return_ip;

	/*
	 * Is this a response to a destroy message?
	 */
	if (nim->cm.type != NSS_IPV6_TX_DESTROY_RULE_MSG) {
		DEBUG_ERROR("%px: ported destroy callback with improper type: %d\n", nim, nim->cm.type);
		return;
	}

	/*
	 * Look up ecm connection so that we can update the status.
	 */
	ci = ecm_db_connection_serial_find_and_ref(serial);
	if (!ci) {
		DEBUG_TRACE("%px: destroy callback, connection not found, serial: %u\n", nim, serial);
		return;
	}

	/*
	 * Release ref held for this ack/nack response.
	 * NOTE: It's okay to do this here, ci won't go away, because the ci is held as
	 * a result of the ecm_db_connection_serial_find_and_ref()
	 */
	ecm_db_connection_deref(ci);

	/*
	 * Get the front end instance
	 */
	feci = ecm_db_connection_front_end_get_and_ref(ci);
	npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;
	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);

	ECM_NSS_IPV6_ADDR_TO_IP_ADDR(flow_ip, nirdm->tuple.flow_ip);
	ECM_NSS_IPV6_ADDR_TO_IP_ADDR(return_ip, nirdm->tuple.return_ip);

	/*
	 * Record command duration
	 */
	ecm_nss_ipv6_decel_done_time_update(feci);

	/*
	 * Dump some useful trace information.
	 */
	DEBUG_TRACE("%px: decelerate response for connection: %px\n", npci, feci->ci);
	DEBUG_TRACE("%px: flow_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n", npci, ECM_IP_ADDR_TO_OCTAL(flow_ip), nirdm->tuple.flow_ident);
	DEBUG_TRACE("%px: return_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n", npci, ECM_IP_ADDR_TO_OCTAL(return_ip), nirdm->tuple.return_ident);
	DEBUG_TRACE("%px: protocol: %d\n", npci, nirdm->tuple.protocol);

	/*
	 * Drop decel pending counter
	 */
	spin_lock_bh(&ecm_nss_ipv6_lock);
	ecm_nss_ipv6_pending_decel_count--;
	DEBUG_ASSERT(ecm_nss_ipv6_pending_decel_count >= 0, "Bad decel pending counter\n");
	spin_unlock_bh(&ecm_nss_ipv6_lock);

	spin_lock_bh(&feci->lock);

	/*
	 * If decel is not still pending then it's possible that the NSS ended acceleration by some other reason e.g. flush
	 * In which case we cannot rely on the response we get here.
	 */
	if (feci->accel_mode != ECM_FRONT_END_ACCELERATION_MODE_DECEL_PENDING) {
		spin_unlock_bh(&feci->lock);

		/*
		 * Release the connections.
		 */
		feci->deref(feci);
		ecm_db_connection_deref(ci);
		return;
	}

	DEBUG_TRACE("%px: response: %d\n", npci, nim->cm.response);
	if (nim->cm.response != NSS_CMN_RESPONSE_ACK) {
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DECEL;
	} else {
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL;
	}

	/*
	 * If connection became defunct then set mode so that no further accel/decel attempts occur.
	 */
	if (feci->is_defunct) {
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DEFUNCT;
	}
	spin_unlock_bh(&feci->lock);

	/*
	 * Ported acceleration ends
	 */
	spin_lock_bh(&ecm_nss_ipv6_lock);
	ecm_nss_ported_ipv6_accelerated_count[npci->ported_accelerated_count_index]--;	/* Protocol specific counter */
	DEBUG_ASSERT(ecm_nss_ported_ipv6_accelerated_count[npci->ported_accelerated_count_index] >= 0, "Bad udp accel counter\n");
	ecm_nss_ipv6_accelerated_count--;		/* General running counter */
	DEBUG_ASSERT(ecm_nss_ipv6_accelerated_count >= 0, "Bad accel counter\n");
	spin_unlock_bh(&ecm_nss_ipv6_lock);

	/*
	 * Release the connections.
	 */
	feci->deref(feci);
	ecm_db_connection_deref(ci);
}

/*
 * ecm_nss_ported_ipv6_connection_decelerate_msg_send()
 *	Prepares and sends a decelerate message to acceleration engine.
 */
static bool ecm_nss_ported_ipv6_connection_decelerate_msg_send(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;
	struct nss_ipv6_msg nim;
	struct nss_ipv6_rule_destroy_msg *nirdm;
	ip_addr_t src_ip;
	ip_addr_t dest_ip;
	nss_tx_status_t nss_tx_status;
	bool ret;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);

	/*
	 * Increment the decel pending counter
	 */
	spin_lock_bh(&ecm_nss_ipv6_lock);
	ecm_nss_ipv6_pending_decel_count++;
	spin_unlock_bh(&ecm_nss_ipv6_lock);

	/*
	 * Prepare deceleration message
	 */
	nss_ipv6_msg_init(&nim, NSS_IPV6_RX_INTERFACE, NSS_IPV6_TX_DESTROY_RULE_MSG,
			sizeof(struct nss_ipv6_rule_destroy_msg),
			ecm_nss_ported_ipv6_connection_destroy_callback,
			(void *)(ecm_ptr_t)ecm_db_connection_serial_get(feci->ci));

	nirdm = &nim.msg.rule_destroy;
	nirdm->tuple.protocol = (int32_t)ecm_db_connection_protocol_get(feci->ci);

	/*
	 * Get addressing information
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, src_ip);
	ECM_IP_ADDR_TO_NSS_IPV6_ADDR(nirdm->tuple.flow_ip, src_ip);
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, dest_ip);
	ECM_IP_ADDR_TO_NSS_IPV6_ADDR(nirdm->tuple.return_ip, dest_ip);
	nirdm->tuple.flow_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_FROM);
	nirdm->tuple.return_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_TO);

	DEBUG_INFO("%px: Ported Connection %px decelerate\n"
			"protocol: %d\n"
			"src_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n"
			"dest_ip: " ECM_IP_ADDR_OCTAL_FMT ":%d\n",
			npci, feci->ci, nirdm->tuple.protocol,
			ECM_IP_ADDR_TO_OCTAL(src_ip), nirdm->tuple.flow_ident,
			ECM_IP_ADDR_TO_OCTAL(dest_ip), nirdm->tuple.return_ident);

	/*
	 * Take a ref to the feci->ci so that it will persist until we get a response from the NSS.
	 * NOTE: This will implicitly hold the feci too.
	 */
	ecm_db_connection_ref(feci->ci);

	/*
	 * We are about to issue the command, record the time of transmission
	 */
	spin_lock_bh(&feci->lock);
	feci->stats.cmd_time_begun = jiffies;
	spin_unlock_bh(&feci->lock);

	/*
	 * Destroy the NSS connection cache entry.
	 */
	nss_tx_status = nss_ipv6_tx(ecm_nss_ipv6_nss_ipv6_mgr, &nim);
	if (nss_tx_status == NSS_TX_SUCCESS) {
		/*
		 * Reset the driver_fail count - transmission was okay here.
		 */
		spin_lock_bh(&feci->lock);
		feci->stats.driver_fail = 0;
		spin_unlock_bh(&feci->lock);
		return true;
	}

	/*
	 * TX failed
	 */
	ret = ecm_front_end_destroy_failure_handle(feci);

	/*
	 * Release the ref take, NSS driver did not accept our command.
	 */
	ecm_db_connection_deref(feci->ci);

	/*
	 * Could not send the request, decrement the decel pending counter
	 */
	spin_lock_bh(&ecm_nss_ipv6_lock);
	ecm_nss_ipv6_pending_decel_count--;
	DEBUG_ASSERT(ecm_nss_ipv6_pending_decel_count >= 0, "Bad decel pending counter\n");
	spin_unlock_bh(&ecm_nss_ipv6_lock);

	return ret;
}

/*
 * ecm_nss_ported_ipv6_connection_decelerate()
 *     Decelerate a connection
 */
static bool ecm_nss_ported_ipv6_connection_decelerate(struct ecm_front_end_connection_instance *feci)
{
	/*
	 * Check if accel mode is OK for the deceleration.
	 */
	spin_lock_bh(&feci->lock);
	if (!ecm_front_end_common_connection_decelerate_accel_mode_check(feci)) {
		spin_unlock_bh(&feci->lock);
		return false;
	}
	feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL_PENDING;
	spin_unlock_bh(&feci->lock);

	return ecm_nss_ported_ipv6_connection_decelerate_msg_send(feci);
}

/*
 * ecm_nss_ported_ipv6_connection_defunct_callback()
 *	Callback to be called when a ported connection has become defunct.
 */
bool ecm_nss_ported_ipv6_connection_defunct_callback(void *arg, int *accel_mode)
{
	bool ret;
	struct ecm_front_end_connection_instance *feci = (struct ecm_front_end_connection_instance *)arg;
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);

	spin_lock_bh(&feci->lock);
	/*
	 * Check if the connection can be defuncted.
	 */
	if (!ecm_front_end_common_connection_defunct_check(feci)) {
		*accel_mode = feci->accel_mode;
		spin_unlock_bh(&feci->lock);
		return false;
	}

	/*
	 * If none of the cases matched above, this means the connection is in the
	 * accel mode, so we force a deceleration.
	 * NOTE: If the mode is accel pending then the decel will be actioned when that is completed.
	 */
	if (!ecm_front_end_common_connection_decelerate_accel_mode_check(feci)) {
		*accel_mode = feci->accel_mode;
		spin_unlock_bh(&feci->lock);
		return false;
	}
	feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL_PENDING;
	spin_unlock_bh(&feci->lock);

	ret = ecm_nss_ported_ipv6_connection_decelerate_msg_send(feci);

	/*
	 * Copy the accel_mode which is returned from the decelerate message function. This value
	 * will be used in the caller to decide releasing the final reference of the connection.
	 * But if this function reaches to here, the caller care about the ret. If ret is true,
	 * the reference will be released regardless of the accel_mode. If ret is false, accel_mode
	 * will be in the ACCEL state (for destroy re-try) and this state will not be used in the
	 * caller's decision. It looks for ACCEL_FAIL states.
	 */
	spin_lock_bh(&feci->lock);
	*accel_mode = feci->accel_mode;
	spin_unlock_bh(&feci->lock);

	return ret;
}

/*
 * ecm_nss_ported_ipv6_connection_accel_state_get()
 *	Get acceleration state
 */
static ecm_front_end_acceleration_mode_t ecm_nss_ported_ipv6_connection_accel_state_get(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;
	ecm_front_end_acceleration_mode_t state;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);
	spin_lock_bh(&feci->lock);
	state = feci->accel_mode;
	spin_unlock_bh(&feci->lock);
	return state;
}

/*
 * ecm_nss_ported_ipv6_connection_action_seen()
 *	Acceleration action / activity has been seen for this connection.
 *
 * NOTE: Call the action_seen() method when the NSS has demonstrated that it has offloaded some data for a connection.
 */
static void ecm_nss_ported_ipv6_connection_action_seen(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);
	DEBUG_INFO("%px: Action seen\n", npci);
	spin_lock_bh(&feci->lock);
	feci->stats.no_action_seen = 0;
	spin_unlock_bh(&feci->lock);
}

/*
 * ecm_nss_ported_ipv6_connection_accel_ceased()
 *	NSS has indicated that acceleration has stopped.
 *
 * NOTE: This is called in response to an NSS self-initiated termination of acceleration.
 * This must NOT be called because the ECM terminated the acceleration.
 */
static void ecm_nss_ported_ipv6_connection_accel_ceased(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);
	DEBUG_INFO("%px: accel ceased\n", npci);

	spin_lock_bh(&feci->lock);

	/*
	 * If we are in accel-pending state then the NSS has issued a flush out-of-order
	 * with the ACK/NACK we are actually waiting for.
	 * To work around this we record a "flush has already happened" and will action it when we finally get that ACK/NACK.
	 * GGG TODO This should eventually be removed when the NSS honours messaging sequence.
	 */
	if (feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL_PENDING) {
		feci->stats.flush_happened = true;
		feci->stats.flush_happened_total++;
		spin_unlock_bh(&feci->lock);
		return;
	}

	/*
	 * If connection is no longer accelerated by the time we get here just ignore the command
	 */
	if (feci->accel_mode != ECM_FRONT_END_ACCELERATION_MODE_ACCEL) {
		spin_unlock_bh(&feci->lock);
		return;
	}

	/*
	 * If the no_action_seen counter was not reset then acceleration ended without any offload action
	 */
	if (feci->stats.no_action_seen) {
		feci->stats.no_action_seen_total++;
	}

	/*
	 * If the no_action_seen indicates successive cessations of acceleration without any offload action occuring
	 * then we fail out this connection
	 */
	if (feci->stats.no_action_seen >= feci->stats.no_action_seen_limit) {
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_NO_ACTION;
	} else {
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL;
	}
	spin_unlock_bh(&feci->lock);

	/*
	 * Ported acceleration ends
	 */
	spin_lock_bh(&ecm_nss_ipv6_lock);
	ecm_nss_ported_ipv6_accelerated_count[npci->ported_accelerated_count_index]--;	/* Protocol specific counter */
	DEBUG_ASSERT(ecm_nss_ported_ipv6_accelerated_count[npci->ported_accelerated_count_index] >= 0, "Bad ported accel counter\n");
	ecm_nss_ipv6_accelerated_count--;		/* General running counter */
	DEBUG_ASSERT(ecm_nss_ipv6_accelerated_count >= 0, "Bad accel counter\n");
	spin_unlock_bh(&ecm_nss_ipv6_lock);
}

/*
 * ecm_nss_ported_ipv6_connection_ref()
 *	Ref a connection front end instance
 */
static void ecm_nss_ported_ipv6_connection_ref(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);
	spin_lock_bh(&feci->lock);
	feci->refs++;
	DEBUG_TRACE("%px: npci ref %d\n", npci, feci->refs);
	DEBUG_ASSERT(feci->refs > 0, "%px: ref wrap\n", npci);
	spin_unlock_bh(&feci->lock);
}

/*
 * ecm_nss_ported_ipv6_connection_deref()
 *	Deref a connection front end instance
 */
static int ecm_nss_ported_ipv6_connection_deref(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);

	spin_lock_bh(&feci->lock);
	feci->refs--;
	DEBUG_ASSERT(feci->refs >= 0, "%px: ref wrap\n", npci);

	if (feci->refs > 0) {
		int refs = feci->refs;
		spin_unlock_bh(&feci->lock);
		DEBUG_TRACE("%px: npci deref %d\n", npci, refs);
		return refs;
	}
	spin_unlock_bh(&feci->lock);

	/*
	 * We can now destroy the instance
	 */
	DEBUG_TRACE("%px: npci final\n", npci);
	DEBUG_CLEAR_MAGIC(npci);
	kfree(npci);

	return 0;
}

#ifdef ECM_STATE_OUTPUT_ENABLE
/*
 * ecm_nss_ported_ipv6_connection_state_get()
 *	Return the state of this ported front end instance
 */
static int ecm_nss_ported_ipv6_connection_state_get(struct ecm_front_end_connection_instance *feci, struct ecm_state_file_instance *sfi)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci = (struct ecm_nss_ported_ipv6_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", npci);

	return ecm_front_end_common_connection_state_get(feci, sfi, "nss_v6.ported");
}
#endif

/*
 * ecm_nss_ported_ipv6_connection_instance_alloc()
 *	Create a front end instance specific for ported connection
 */
struct ecm_nss_ported_ipv6_connection_instance *ecm_nss_ported_ipv6_connection_instance_alloc(
								bool can_accel,
								int protocol,
								struct ecm_db_connection_instance **nci)
{
	struct ecm_nss_ported_ipv6_connection_instance *npci;
	struct ecm_front_end_connection_instance *feci;
	struct ecm_db_connection_instance *ci;

	if (ecm_nss_ipv6_is_conn_limit_reached()) {
		DEBUG_TRACE("Reached connection limit\n");
		return NULL;
	}

	/*
	 * Now allocate the new connection
	 */
	*nci = ecm_db_connection_alloc();
	if (!*nci) {
		DEBUG_WARN("Failed to allocate connection\n");
		return NULL;
	}

	ci = *nci;

	npci = (struct ecm_nss_ported_ipv6_connection_instance *)kzalloc(sizeof(struct ecm_nss_ported_ipv6_connection_instance), GFP_ATOMIC | __GFP_NOWARN);
	if (!npci) {
		DEBUG_WARN("Ported Front end alloc failed\n");
		ecm_db_connection_deref(ci);
		return NULL;
	}

	/*
	 * Refs is 1 for the creator of the connection
	 */
	feci = (struct ecm_front_end_connection_instance *)npci;
	feci->refs = 1;
	DEBUG_SET_MAGIC(npci, ECM_NSS_PORTED_IPV6_CONNECTION_INSTANCE_MAGIC);
	spin_lock_init(&feci->lock);

	feci->can_accel = can_accel;
	feci->accel_mode = (can_accel) ? ECM_FRONT_END_ACCELERATION_MODE_DECEL : ECM_FRONT_END_ACCELERATION_MODE_FAIL_DENIED;
	feci->accel_engine = ECM_FRONT_END_ENGINE_NSS;
	spin_lock_bh(&ecm_nss_ipv6_lock);
	feci->stats.no_action_seen_limit = ecm_nss_ipv6_no_action_limit_default;
	feci->stats.driver_fail_limit = ecm_nss_ipv6_driver_fail_limit_default;
	feci->stats.ae_nack_limit = ecm_nss_ipv6_nack_limit_default;
	spin_unlock_bh(&ecm_nss_ipv6_lock);

	/*
	 * Copy reference to connection - no need to ref ci as ci maintains a ref to this instance instead (this instance persists for as long as ci does)
	 */
	feci->ci = ci;

	feci->ip_version = 6;

	feci->protocol = protocol;

	/*
	 * Populate the methods and callbacks
	 */
	feci->ref = ecm_nss_ported_ipv6_connection_ref;
	feci->deref = ecm_nss_ported_ipv6_connection_deref;
	feci->accelerate = ecm_nss_ported_ipv6_connection_accelerate;
	feci->decelerate = ecm_nss_ported_ipv6_connection_decelerate;
	feci->accel_state_get = ecm_nss_ported_ipv6_connection_accel_state_get;
	feci->action_seen = ecm_nss_ported_ipv6_connection_action_seen;
	feci->accel_ceased = ecm_nss_ported_ipv6_connection_accel_ceased;
#ifdef ECM_STATE_OUTPUT_ENABLE
	feci->state_get = ecm_nss_ported_ipv6_connection_state_get;
#endif
	feci->ae_interface_number_by_dev_get = ecm_nss_common_get_interface_number_by_dev;
	feci->ae_interface_number_by_dev_type_get = ecm_nss_common_get_interface_number_by_dev_type;
	feci->ae_interface_type_get = ecm_nss_common_get_interface_type;
	feci->regenerate = ecm_nss_common_connection_regenerate;

	if (protocol == IPPROTO_TCP) {
		npci->ported_accelerated_count_index = ECM_NSS_PORTED_IPV6_PROTO_TCP;
	} else if (protocol == IPPROTO_UDP) {
		npci->ported_accelerated_count_index = ECM_NSS_PORTED_IPV6_PROTO_UDP;
	} else {
		DEBUG_WARN("%px: Wrong protocol: %d\n", npci, protocol);
		DEBUG_CLEAR_MAGIC(npci);
		ecm_db_connection_deref(ci);
		kfree(npci);
		return NULL;
	}

	return npci;
}

/*
 * ecm_nss_ported_ipv6_debugfs_init()
 */
bool ecm_nss_ported_ipv6_debugfs_init(struct dentry *dentry)
{
	struct dentry *udp_dentry;

	udp_dentry = debugfs_create_u32("udp_accelerated_count", S_IRUGO, dentry,
						&ecm_nss_ported_ipv6_accelerated_count[ECM_NSS_PORTED_IPV6_PROTO_UDP]);
	if (!udp_dentry) {
		DEBUG_ERROR("Failed to create ecm nss ipv6 udp_accelerated_count file in debugfs\n");
		return false;
	}

	if (!debugfs_create_u32("tcp_accelerated_count", S_IRUGO, dentry,
					&ecm_nss_ported_ipv6_accelerated_count[ECM_NSS_PORTED_IPV6_PROTO_TCP])) {
		DEBUG_ERROR("Failed to create ecm nss ipv6 tcp_accelerated_count file in debugfs\n");
		debugfs_remove(udp_dentry);
		return false;
	}

	return true;
}
