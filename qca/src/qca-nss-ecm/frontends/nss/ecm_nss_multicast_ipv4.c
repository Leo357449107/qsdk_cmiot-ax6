/*
 **************************************************************************
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
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
#include <linux/sysctl.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/pkt_sched.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/addrconf.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>	/* for put_user */
#include <net/ipv6.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/mroute.h>
#include <linux/vmalloc.h>

#include <linux/inetdevice.h>
#include <linux/if_arp.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_bridge.h>
#include <linux/if_bridge.h>
#include <net/arp.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_acct.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_zones.h>
#include <net/netfilter/nf_conntrack_core.h>
#include <net/netfilter/ipv4/nf_conntrack_ipv4.h>
#include <net/netfilter/ipv4/nf_defrag_ipv4.h>
#include <net/vxlan.h>
#ifdef ECM_INTERFACE_VLAN_ENABLE
#include <linux/../../net/8021q/vlan.h>
#include <linux/if_vlan.h>
#endif

/*
 * General operational control
 */
int ecm_front_end_ipv4_mc_stopped = 0;	/* When non-zero further traffic will not be processed */

/*
 * Debug output levels
 * 0 = OFF
 * 1 = ASSERTS / ERRORS
 * 2 = 1 + WARN
 * 3 = 2 + INFO
 * 4 = 3 + TRACE
 */
#define DEBUG_LEVEL ECM_NSS_MULTICAST_IPV4_DEBUG_LEVEL

#include <nss_api_if.h>
#include <mc_ecm.h>

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
#include "ecm_nss_ipv4.h"
#include "ecm_nss_multicast_ipv4.h"
#include "ecm_nss_common.h"
#include "ecm_front_end_common.h"

/*
 * Magic numbers
 */
#define ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC 0xED12

/*
 * struct ecm_nss_ipv4_multicast_connection_instance
 *	A connection specific front end instance for MULTICAST connections
 */
struct ecm_nss_multicast_ipv4_connection_instance {
	struct ecm_front_end_connection_instance base;		/* Base class */
#if (DEBUG_LEVEL > 0)
	uint16_t magic;
#endif
};

static int ecm_nss_multicast_ipv4_accelerated_count = 0;
						/* Array of Number of TCP and UDP connections currently offloaded */

/*
 * ecm_nss_multicast_ipv4_connection_update_callback()
 * 	Callback for handling Ack/Nack for update accelerate rules
 */
static void ecm_nss_multicast_ipv4_connection_update_callback(void *app_data, struct nss_ipv4_msg *nim)
{
	struct nss_ipv4_mc_rule_create_msg *nircm = &nim->msg.mc_rule_create;
	uint32_t serial = (uint32_t)(ecm_ptr_t)app_data;
	struct ecm_db_connection_instance *ci;
	struct ecm_front_end_connection_instance *feci;
	struct ecm_nss_multicast_ipv4_connection_instance *nmci;

	/*TODO: If the response is NACK then decelerate the flow and flushes all rules */
	DEBUG_TRACE("%px: update callback, response received from FW : %u\n", nim, nim->cm.response);

	/*
	 * Is this a response to a create message?
	 */
	if (nim->cm.type != NSS_IPV4_TX_CREATE_MC_RULE_MSG) {
		DEBUG_ERROR("%px: multicast update callback with improper type: %d, serial: %u\n", nim, nim->cm.type, serial);
		return;
	}

	/*
	 * Is this a response to a update rule message?
	 */
	if ( !(nircm->rule_flags & NSS_IPV4_MC_RULE_CREATE_FLAG_MC_UPDATE)) {
		DEBUG_ERROR("%px: multicast update callback with improper type: %d, serial: %u\n", nim, nim->cm.type, serial);
		return;
	}

	/*
	 * Look up ecm connection so that we can update the status.
	 */
	ci = ecm_db_connection_serial_find_and_ref(serial);
	if (!ci) {
		DEBUG_TRACE("%px: multicast update callback, connection not found, serial: %u\n", nim, serial);
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
	nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;
	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

	/*
	 * Dump some useful trace information.
	 */
	DEBUG_TRACE("%px: Update accelerate response for connection: %px, serial: %u\n", nmci, feci->ci, serial);
	DEBUG_TRACE("%px: valid_flags: %x\n", nmci, nircm->valid_flags);
	DEBUG_TRACE("%px: flow_ip: %pI4h:%d\n", nmci, &nircm->tuple.flow_ip, nircm->tuple.flow_ident);
	DEBUG_TRACE("%px: return_ip: %pI4h:%d\n", nmci, &nircm->tuple.return_ip, nircm->tuple.return_ident);
	DEBUG_TRACE("%px: protocol: %d\n", nmci, nircm->tuple.protocol);

	/*
	 * Release the connection.
	 */
	feci->deref(feci);
	ecm_db_connection_deref(ci);
	return;
}

/*
 * ecm_nss_multicast_ipv4_connection_create_callback()
 * 	callback for handling create ack/nack calls for multicast create commands.
 */
static void ecm_nss_multicast_ipv4_connection_create_callback(void *app_data, struct nss_ipv4_msg *nim)
{
	struct nss_ipv4_mc_rule_create_msg *__attribute__((unused))nircm = &nim->msg.mc_rule_create;
	uint32_t serial = (uint32_t)(ecm_ptr_t)app_data;
	struct ecm_db_connection_instance *ci;
	struct ecm_front_end_connection_instance *feci;
	struct ecm_nss_multicast_ipv4_connection_instance *nmci;
	ecm_front_end_acceleration_mode_t result_mode;
	bool is_defunct = false;

	/*
	 * Is this a response to a create message?
	 */
	if (nim->cm.type != NSS_IPV4_TX_CREATE_MC_RULE_MSG) {
		DEBUG_ERROR("%px: udp create callback with improper type: %d, serial: %u\n", nim, nim->cm.type, serial);
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
	nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;
	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

	/*
	 * Dump some useful trace information.
	 */
	DEBUG_TRACE("%px: accelerate response for connection: %px, serial: %u\n", nmci, feci->ci, serial);
	DEBUG_TRACE("%px: valid_flags: %x\n", nmci, nircm->valid_flags);
	DEBUG_TRACE("%px: flow_ip: %pI4h:%d\n", nmci, &nircm->tuple.flow_ip, nircm->tuple.flow_ident);
	DEBUG_TRACE("%px: return_ip: %pI4h:%d\n", nmci, &nircm->tuple.return_ip, nircm->tuple.return_ident);
	DEBUG_TRACE("%px: protocol: %d\n", nmci, nircm->tuple.protocol);

	/*
	 * Handle the creation result code.
	 */
	DEBUG_TRACE("%px: response: %d\n", nmci, nim->cm.response);
	if (nim->cm.response != NSS_CMN_RESPONSE_ACK) {
		/*
		 * Creation command failed (specific reason ignored).
		 */
		DEBUG_TRACE("%px: accel nack: %d\n", nmci, nim->cm.error);
		spin_lock_bh(&feci->lock);
		DEBUG_ASSERT(feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL_PENDING, "%px: Unexpected mode: %d\n", ci, feci->accel_mode);
		nmci->base.stats.ae_nack++;
		nmci->base.stats.ae_nack_total++;
		if (nmci->base.stats.ae_nack >= nmci->base.stats.ae_nack_limit) {
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

		spin_lock_bh(&ecm_nss_ipv4_lock);
		_ecm_nss_ipv4_accel_pending_clear(feci, result_mode);
		spin_unlock_bh(&ecm_nss_ipv4_lock);

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
	if (nmci->base.stats.flush_happened) {
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL;
		nmci->base.stats.flush_happened = false;

		/*
		 * We are decelerated, clear any pending flag as that is meaningless now.
		 */
		nmci->base.stats.decelerate_pending = false;

		/*
		 * Increement the no-action counter.  Our connectin was decelerated on us with no action occurring.
		 */
		nmci->base.stats.no_action_seen++;
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
	nmci->base.stats.ae_nack = 0;

	/*
	 * Clear the "accelerate pending" state and move to "accelerated" state bumping
	 * the accelerated counters to match our new state.
	 *
	 * Decelerate may have been attempted while we were accel pending.
	 * If decelerate is pending then we need to begin deceleration :-(
	 */
	spin_lock_bh(&ecm_nss_ipv4_lock);

	ecm_nss_multicast_ipv4_accelerated_count++;	/* Protocol specific counter */
	ecm_nss_ipv4_accelerated_count++;		/* General running counter */

	if (!_ecm_nss_ipv4_accel_pending_clear(feci, ECM_FRONT_END_ACCELERATION_MODE_ACCEL)) {
		/*
		 * Increement the no-action counter, this is reset if offload action is seen
		 */
		nmci->base.stats.no_action_seen++;

		spin_unlock_bh(&ecm_nss_ipv4_lock);
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

	spin_unlock_bh(&ecm_nss_ipv4_lock);
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
 * ecm_nss_multicast_ipv4_connection_update_accelerate()
 * 	Push destination interface list updates for a multicast connection
 * 	to NSS
 *
 * 	This function receives a list of interfaces that have either left or joined the connection,
 * 	and sends a 'multicast update' command to NSS to inform about these interface state changes.
 */
static int ecm_nss_multicast_ipv4_connection_update_accelerate(struct ecm_front_end_connection_instance *feci,
							       struct ecm_multicast_if_update *rp,
							       struct ecm_classifier_process_response *pr)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;
	uint16_t regen_occurrances;
	struct ecm_db_iface_instance *to_ifaces;
	struct ecm_db_iface_instance *ii_temp;
	struct ecm_db_iface_instance *ii_single;
	struct ecm_db_iface_instance **ifaces;
	struct ecm_db_iface_instance *from_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX];
	struct ecm_db_iface_instance *from_nss_iface;
	struct nss_ipv4_mc_rule_create_msg *create;
	struct nss_ipv4_msg *nim;
	ip_addr_t addr;
	int32_t *to_ifaces_first;
	int32_t *to_ii_first;
	int32_t vif;
	int32_t ret;
	int32_t valid_vif_idx = 0;
	int32_t from_ifaces_first;
	int32_t to_nss_iface_id = 0;
	int32_t from_nss_iface_id = 0;
	uint8_t to_nss_iface_address[ETH_ALEN];
	nss_tx_status_t nss_tx_status;
	int32_t list_index;
	int32_t to_mtu = 0;
	int from_iface_bridge_identifier = 0;
	int to_iface_bridge_identifier = 0;
	int32_t interface_type_counts[ECM_DB_IFACE_TYPE_COUNT];
	bool rule_invalid;
	uint8_t dest_mac[ETH_ALEN];

	DEBUG_INFO("%px: Accel conn: %px\n", nmci, feci->ci);

	/*
	 * Get the re-generation occurrance counter of the connection.
	 * We compare it again at the end - to ensure that the rule construction has seen no generation
	 * changes during rule creation.
	 */
	regen_occurrances = ecm_db_connection_regeneration_occurrances_get(feci->ci);

	nim = (struct nss_ipv4_msg *)kzalloc(sizeof(struct nss_ipv4_msg), GFP_ATOMIC | __GFP_NOWARN);
	if (!nim) {
		return -1;
	}

	nss_ipv4_msg_init(nim, NSS_IPV4_RX_INTERFACE, NSS_IPV4_TX_CREATE_MC_RULE_MSG,
			sizeof(struct nss_ipv4_mc_rule_create_msg),
			ecm_nss_multicast_ipv4_connection_update_callback,
			(void *)(ecm_ptr_t)ecm_db_connection_serial_get(feci->ci));

	create = &nim->msg.mc_rule_create;

	/*
	 * Construct an accel command.
	 */
	ret = ecm_db_multicast_connection_to_interfaces_get_and_ref_all(feci->ci, &to_ifaces, &to_ifaces_first);
	if (ret == 0) {
		DEBUG_WARN("%px: Accel attempt failed - no interfaces in to_interfaces list!\n", nmci);
		kfree(nim);
		return -1;
	}

	from_ifaces_first = ecm_db_connection_interfaces_get_and_ref(feci->ci, from_ifaces, ECM_DB_OBJ_DIR_FROM);
	if (from_ifaces_first == ECM_DB_IFACE_HEIRARCHY_MAX) {
		DEBUG_WARN("%px: Accel attempt failed - no interfaces in from_interfaces list!\n", nmci);
		ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);
		kfree(nim);
		return -1;
	}

	create->ingress_vlan_tag[0] =   ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
	create->ingress_vlan_tag[1] =   ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;

	/*
	 * Set the source NSS interface identifier
	 */
	from_nss_iface = from_ifaces[from_ifaces_first];
	from_nss_iface_id = ecm_db_iface_ae_interface_identifier_get(from_nss_iface);
	if (from_nss_iface_id < 0) {
                DEBUG_TRACE("%px: from_nss_iface_id: %d\n", nmci, from_nss_iface_id);
		spin_lock_bh(&feci->lock);
		if (feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL) {
			feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_NO_ACTION;
		}
		spin_unlock_bh(&feci->lock);
		ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
		ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);
		kfree(nim);
		return -1;
        }

	create->src_interface_num = from_nss_iface_id;
	from_nss_iface = from_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX - 1];
	from_iface_bridge_identifier = ecm_db_iface_interface_identifier_get(from_nss_iface);
	ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);

	/*
	 * Now examine the TO / DEST heirarchy list to construct the destination interface
	 * information
	 */
	DEBUG_TRACE("%px: Examine to/dest heirarchy list\n", nmci);
	rule_invalid = false;

	/*
	 * Loop through the list of interface updates
	 */
	for (vif = 0; vif < ECM_DB_MULTICAST_IF_MAX; vif++) {
#ifdef ECM_INTERFACE_VLAN_ENABLE
		create->if_rule[vif].egress_vlan_tag[0] = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
		create->if_rule[vif].egress_vlan_tag[1] = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
#endif

		/*
		 * If there is no state change for an interface at this index,
		 * then ignore
		 */
		if (!(rp->if_join_idx[vif] || rp->if_leave_idx[vif])) {
			continue;
		}

		ii_temp = ecm_db_multicast_if_heirarchy_get(to_ifaces, vif);

		/*
		 * We have an update for this interface. Construct the interface information
		 */
		to_nss_iface_id = -1;
		memset(interface_type_counts, 0, sizeof(interface_type_counts));
		to_ii_first = ecm_db_multicast_if_first_get_at_index(to_ifaces_first, vif);

		for (list_index = *to_ii_first; !rule_invalid && (list_index < ECM_DB_IFACE_HEIRARCHY_MAX); list_index++) {
			struct ecm_db_iface_instance *ii;
			ecm_db_iface_type_t ii_type;
			char *ii_name;

			ii_single = ecm_db_multicast_if_instance_get_at_index(ii_temp, list_index);
			ifaces = (struct ecm_db_iface_instance **)ii_single;
			ii = *ifaces;
			ii_type = ecm_db_iface_type_get(ii);
			ii_name = ecm_db_interface_type_to_string(ii_type);
			DEBUG_TRACE("%px: list_index: %d, ii: %px, type: %d (%s)\n", nmci, list_index, ii, ii_type, ii_name);

			/*
			 * Extract information from this interface type if it is applicable to the rule.
			 * Conflicting information may cause accel to be unsupported.
			 */
			switch (ii_type) {
#ifdef ECM_INTERFACE_VLAN_ENABLE
				struct ecm_db_interface_info_vlan vlan_info;
#endif

			case ECM_DB_IFACE_TYPE_BRIDGE:
				DEBUG_TRACE("%px: Bridge\n", nmci);
				if (interface_type_counts[ii_type] != 0) {

					/*
					 * Cannot cascade bridges
					 */
					rule_invalid = true;
					DEBUG_TRACE("%px: Bridge - ignore additional\n", nmci);
					break;
				}
				ecm_db_iface_bridge_address_get(ii, to_nss_iface_address);
				to_iface_bridge_identifier = ecm_db_iface_interface_identifier_get(ii);
				DEBUG_TRACE("%px: Bridge - mac: %pM\n", nmci, to_nss_iface_address);
				break;
			case ECM_DB_IFACE_TYPE_ETHERNET:
				 DEBUG_TRACE("%px: Ethernet\n", nmci);
				if (interface_type_counts[ii_type] != 0) {
					/*
					 * Ignore additional mac addresses, these are usually as a result of address propagation
					 * from bridges down to ports etc.
					 */
					DEBUG_TRACE("%px: Ethernet - ignore additional\n", nmci);
					break;
				}

				/*
				 * Can only handle one MAC, the first outermost mac.
				 */
				ecm_db_iface_ethernet_address_get(ii, to_nss_iface_address);
				to_mtu = (uint32_t)ecm_db_connection_iface_mtu_get(feci->ci, ECM_DB_OBJ_DIR_TO);
				to_nss_iface_id = ecm_db_iface_ae_interface_identifier_get(ii);
				if (to_nss_iface_id < 0) {
					DEBUG_TRACE("%px: to_nss_iface_id: %d\n", nmci, to_nss_iface_id);
					ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);
					kfree(nim);
					return -1;
			        }

				DEBUG_TRACE("%px: Ethernet - mac: %pM\n", nmci, to_nss_iface_address);
				break;
			case ECM_DB_IFACE_TYPE_PPPOE:
#ifdef ECM_INTERFACE_PPPOE_ENABLE
				/*
				 * More than one PPPoE in the list is not valid!
				 */
				if (interface_type_counts[ii_type] != 0) {
					DEBUG_TRACE("%px: PPPoE - additional unsupported\n", nmci);
					rule_invalid = true;
					break;
				}

				/*
				 * Set the PPPoE rule creation structure.
				 */
				create->if_rule[valid_vif_idx].pppoe_if_num = ecm_db_iface_ae_interface_identifier_get(ii);
				if (create->if_rule[valid_vif_idx].pppoe_if_num < 0) {
					DEBUG_TRACE("%px: PPPoE - acceleration engine interface (%d) is not valid\n",
							nmci, create->if_rule[valid_vif_idx].pppoe_if_num);
					rule_invalid = true;
					break;
				}
				create->if_rule[valid_vif_idx].valid_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_PPPOE_VALID;
				DEBUG_TRACE("%px: PPPoE - exist pppoe_if_num: %d\n", nmci,
							create->if_rule[valid_vif_idx].pppoe_if_num);
#else
				DEBUG_TRACE("%px: PPPoE - unsupported\n", nmci);
				rule_invalid = true;
#endif
				break;
			case ECM_DB_IFACE_TYPE_VLAN:
#ifdef ECM_INTERFACE_VLAN_ENABLE
				DEBUG_TRACE("%px: VLAN\n", nmci);
				if (interface_type_counts[ii_type] > 1) {
					/*
					 * Can only support two vlans
					 */
					rule_invalid = true;
					DEBUG_TRACE("%px: VLAN - additional unsupported\n", nmci);
					break;
				}
				ecm_db_iface_vlan_info_get(ii, &vlan_info);
				create->if_rule[valid_vif_idx].egress_vlan_tag[interface_type_counts[ii_type]] = ((vlan_info.vlan_tpid << 16) | vlan_info.vlan_tag);

				/*
				 * If we have not yet got an ethernet mac then take this one (very unlikely as mac should have been propagated to the slave (outer) device
				 */
				if (interface_type_counts[ECM_DB_IFACE_TYPE_ETHERNET] == 0) {
					memcpy(to_nss_iface_address, vlan_info.address, ETH_ALEN);
					interface_type_counts[ECM_DB_IFACE_TYPE_ETHERNET]++;
					DEBUG_TRACE("%px: VLAN use mac: %pM\n", nmci, to_nss_iface_address);
				}
				create->if_rule[valid_vif_idx].valid_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_VLAN_VALID;
				DEBUG_TRACE("%px: vlan tag: %x\n", nmci, create->if_rule[valid_vif_idx].egress_vlan_tag[interface_type_counts[ii_type]]);
#else
				rule_invalid = true;
				DEBUG_TRACE("%px: VLAN - unsupported\n", nmci);
#endif
				break;
			default:
				DEBUG_TRACE("%px: Ignoring: %d (%s)\n", nmci, ii_type, ii_name);
			}

			/*
			 * Seen an interface of this type
			 */
			interface_type_counts[ii_type]++;
		}

		if (rule_invalid) {
			DEBUG_WARN("%px: to/dest Rule invalid\n", nmci);
			ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);
			kfree(nim);
			return -1;
		}

		/*
		 * Is this a valid interface?
		 */
		if (to_nss_iface_id != -1) {
			bool is_bridge;
			create->if_rule[valid_vif_idx].if_num = to_nss_iface_id;
			create->if_rule[valid_vif_idx].if_mtu = to_mtu;
			if (rp->if_join_idx[vif]) {
				/*
				 * The interface has joined the group
				 */
				create->if_rule[valid_vif_idx].rule_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_JOIN;
#ifdef ECM_CLASSIFIER_OVS_ENABLE
				if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_OVS_VLAN_TAG) {
					/*
					 * Set primary egress VLAN tag
					 */
					if (pr->egress_mc_vlan_tag[vif][0] != ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED) {
						create->if_rule[valid_vif_idx].egress_vlan_tag[0] = pr->egress_mc_vlan_tag[vif][0];
						create->if_rule[valid_vif_idx].valid_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_VLAN_VALID;

						/*
						 * Set secondary egress VLAN tag
						 */
						if (pr->egress_mc_vlan_tag[vif][1] != ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED) {
							create->if_rule[valid_vif_idx].egress_vlan_tag[1] = pr->egress_mc_vlan_tag[vif][1];
						}
					}

				}
#endif
			} else if (rp->if_leave_idx[vif]) {
				/*
				 * The interface has left the group
				 */
				create->if_rule[valid_vif_idx].rule_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_LEAVE;
			}

			is_bridge = !ecm_db_connection_is_routed_get(feci->ci);

			/*
			 * Do not set the ROUTED flag for pure bridged interfaces
			 */
			if (is_bridge) {
				uint8_t from_nss_iface_address[ETH_ALEN];
				ecm_db_connection_node_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, (uint8_t *)from_nss_iface_address);
				memcpy(create->if_rule[valid_vif_idx].if_mac, from_nss_iface_address, ETH_ALEN);
				create->if_rule[valid_vif_idx].rule_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_BRIDGE_FLOW;
			} else {
				create->if_rule[valid_vif_idx].rule_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_ROUTED_FLOW;
				memcpy(create->if_rule[valid_vif_idx].if_mac, to_nss_iface_address, ETH_ALEN);
			}

			valid_vif_idx++;
		}
	}

	/*
	 * Set number of interface updates in the update rule
	 */
	create->if_count = valid_vif_idx;

	/*
	 * Set the UPDATE flag
	 */
	create->rule_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_MC_UPDATE;

	/*
	 * Set protocol
	 */
	create->tuple.protocol = IPPROTO_UDP;

	/*
	 * The src_ip is where the connection established from
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, addr);
	ECM_IP_ADDR_TO_HIN4_ADDR(create->tuple.flow_ip, addr);

	/*
	 * The destination address is what the destination IP is translated to as it is forwarded to the next interface.
	 * For egress this would yield the normal wan host and for ingress this would correctly NAT back to the LAN host
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, addr);
	ECM_IP_ADDR_TO_HIN4_ADDR(create->tuple.return_ip, addr);

	/*
	 * Same approach as above for port information
	 */
	create->tuple.flow_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_FROM);
	create->tuple.return_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_TO);

	/*
	 * Destination Node(MAC) address. This address will be same for all to side intefaces
	 */
	ecm_db_connection_node_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, dest_mac);
	memcpy(create->dest_mac, dest_mac, ETH_ALEN);

	ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);

	for (vif = 0; vif < valid_vif_idx ; vif++) {
		DEBUG_TRACE("ACCEL UPDATE %px: UDP Accelerate connection %px\n"
				"Rule flag: %x\n"
				"Vif: %d\n"
				"Protocol: %d\n"
				"to_mtu: %u\n"
				"from_ip: %pI4h:%d\n"
				"to_ip: %pI4h:%d\n"
				"to_mac: %pM\n"
				"dest_iface_num: %u\n"
				"out_vlan[0] %x\n"
				"out_vlan[1] %x\n",
				nmci,
				feci->ci,
				create->if_rule[vif].rule_flags,
				vif,
				create->tuple.protocol,
				create->if_rule[vif].if_mtu,
				&create->tuple.flow_ip, create->tuple.flow_ident,
				&create->tuple.return_ip, create->tuple.return_ident,
				create->if_rule[vif].if_mac,
				create->if_rule[vif].if_num,
				create->if_rule[vif].egress_vlan_tag[0],
				create->if_rule[vif].egress_vlan_tag[1]);

	}

	/*
	 * Now that the rule has been constructed we re-compare the generation occurrance counter.
	 * If there has been a change then we abort because the rule may have been created using
	 * unstable data - especially if another thread has begun regeneration of the connection state.
	 * NOTE: This does not prevent a regen from being flagged immediately after this line of code either,
	 * or while the acceleration rule is in flight to the nss.
	 * This is only to check for consistency of rule state - not that the state is stale.
	 */
	if (regen_occurrances != ecm_db_connection_regeneration_occurrances_get(feci->ci)) {
		DEBUG_INFO("%px: connection:%px regen occurred - aborting accel rule.\n", feci, feci->ci);
		kfree(nim);
		return -1;
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
	nss_tx_status = nss_ipv4_tx(ecm_nss_ipv4_nss_ipv4_mgr, nim);
	if (nss_tx_status == NSS_TX_SUCCESS) {
		spin_lock_bh(&feci->lock);
		nmci->base.stats.driver_fail = 0;		/* Reset */
		spin_unlock_bh(&feci->lock);
		kfree(nim);
		return 0;
	}

	/*
	 * Revert accel mode if necessary
	 */
	DEBUG_WARN("%px: ACCEL UPDATE attempt failed\n", nmci);

	/*
	 * Release that ref!
	 */
	ecm_db_connection_deref(feci->ci);

	kfree(nim);

	/*
	 * TX failed
	 */
	spin_lock_bh(&feci->lock);
	nmci->base.stats.driver_fail_total++;
	nmci->base.stats.driver_fail++;
	if (nmci->base.stats.driver_fail >= nmci->base.stats.driver_fail_limit) {
		DEBUG_WARN("%px: Accel failed - driver fail limit\n", nmci);
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DRIVER;
	}
	spin_unlock_bh(&feci->lock);
	return -1;
}

/*
 * ecm_nss_multicast_ipv4_connection_accelerate()
 * 	Accelerate a multicast UDP connection
 */
static void ecm_nss_multicast_ipv4_connection_accelerate(struct ecm_front_end_connection_instance *feci,
                                                                        struct ecm_classifier_process_response *pr, bool is_l2_encap,
                                                                        struct nf_conn *ct, struct sk_buff *skb)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;
	uint16_t regen_occurrances;
	struct ecm_db_iface_instance *to_ifaces;
	struct ecm_db_iface_instance *ii_temp;
	struct ecm_db_iface_instance *ii_single;
	struct ecm_db_iface_instance **ifaces;
	struct ecm_db_iface_instance *from_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX];
	struct ecm_db_iface_instance *from_nss_iface;
	struct ecm_db_iface_instance *from_nat_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX];
	struct nss_ipv4_mc_rule_create_msg *create;
	struct nss_ipv4_msg *nim;
	struct ecm_classifier_instance *assignments[ECM_CLASSIFIER_TYPES];
	int32_t *to_ifaces_first;
	int32_t *to_ii_first;
	int32_t from_ifaces_first;
	int32_t from_nss_iface_id;
	uint32_t from_nat_ifaces_first;
	int32_t to_nss_iface_id;
	int32_t from_nat_ifaces_identifier = 0;
	uint8_t to_nss_iface_address[ETH_ALEN];
	ip_addr_t addr;
	int from_iface_bridge_identifier = 0;
	int to_iface_bridge_identifier = 0;
	int aci_index;
	int vif;
	int ret;
	int assignment_count;
	nss_tx_status_t nss_tx_status;
	int32_t list_index;
	int32_t valid_vif_idx = 0;
	int32_t interface_type_counts[ECM_DB_IFACE_TYPE_COUNT];
	uint8_t dest_mac[ETH_ALEN];
	bool rule_invalid;
	ecm_front_end_acceleration_mode_t result_mode;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

	/*
	 * Get the re-generation occurrance counter of the connection.
	 * We compare it again at the end - to ensure that the rule construction has seen no generation
	 * changes during rule creation.
	 */
	regen_occurrances = ecm_db_connection_regeneration_occurrances_get(feci->ci);

	/*
	 * Can this connection be accelerated at all?
	 */
	if (!ecm_nss_ipv4_accel_pending_set(feci)) {
		DEBUG_TRACE("%px: Acceleration denied: %px\n", feci, feci->ci);
		return;
	}

	/*
	 * Construct an accel command.
	 * Initialise Multicast create structure.
	 * NOTE: We leverage the app_data void pointer to be our 32 bit connection serial number.
	 * When we get it back we re-cast it to a uint32 and do a faster connection lookup.
	 */
	nim = (struct nss_ipv4_msg *)kzalloc(sizeof(struct nss_ipv4_msg), GFP_ATOMIC | __GFP_NOWARN);
	if (!nim) {
		return;
	}

	nss_ipv4_msg_init(nim, NSS_IPV4_RX_INTERFACE, NSS_IPV4_TX_CREATE_MC_RULE_MSG,
			sizeof(struct nss_ipv4_mc_rule_create_msg),
			 ecm_nss_multicast_ipv4_connection_create_callback,
			(void *)(ecm_ptr_t)ecm_db_connection_serial_get(feci->ci));

	create = &nim->msg.mc_rule_create;

	/*
	 * Populate the multicast creation structure
	 */
	from_ifaces_first = ecm_db_connection_interfaces_get_and_ref(feci->ci, from_ifaces, ECM_DB_OBJ_DIR_FROM);
	if (from_ifaces_first == ECM_DB_IFACE_HEIRARCHY_MAX) {
		DEBUG_WARN("%px: Accel attempt failed - no interfaces in from_interfaces list!\n", nmci);
		kfree(nim);
		return;
	}

	create->ingress_vlan_tag[0] = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
	create->ingress_vlan_tag[1] = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
	from_nss_iface = from_ifaces[from_ifaces_first];
	from_nss_iface_id = ecm_db_iface_ae_interface_identifier_get(from_nss_iface);
	if (from_nss_iface_id < 0) {
                DEBUG_TRACE("%px: from_nss_iface_id: %d\n", nmci, from_nss_iface_id);
		ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);
		kfree(nim);
		return;
        }

	memset(interface_type_counts, 0, sizeof(interface_type_counts));
	rule_invalid = false;
	for (list_index = from_ifaces_first; list_index < ECM_DB_IFACE_HEIRARCHY_MAX; list_index++) {
		struct ecm_db_iface_instance *ii;
		ecm_db_iface_type_t ii_type;
		char *ii_name;

		ii = from_ifaces[list_index];
		ii_type = ecm_db_iface_type_get(ii);
		ii_name = ecm_db_interface_type_to_string(ii_type);
		DEBUG_TRACE("%px: list_index: %d, ii: %px, type: %d (%s)\n", nmci, list_index, ii, ii_type, ii_name);

		/*
		 * Extract information from this interface type if it is applicable to the rule.
		 * Conflicting information may cause accel to be unsupported.
		 */
		switch (ii_type) {
#ifdef ECM_INTERFACE_VLAN_ENABLE
			struct ecm_db_interface_info_vlan vlan_info;
#endif
		case ECM_DB_IFACE_TYPE_BRIDGE:
			DEBUG_TRACE("%px: Bridge\n", nmci);
			from_iface_bridge_identifier = ecm_db_iface_interface_identifier_get(ii);
			break;
		case ECM_DB_IFACE_TYPE_VLAN:
#ifdef ECM_INTERFACE_VLAN_ENABLE
			DEBUG_TRACE("%px: VLAN\n", nmci);
			if (interface_type_counts[ii_type] > 1) {
				/*
				 * Can only support two vlans
				 */
				rule_invalid = true;
				DEBUG_TRACE("%px: VLAN - additional unsupported\n", nmci);
				break;
			}
			ecm_db_iface_vlan_info_get(ii, &vlan_info);
			create->ingress_vlan_tag[interface_type_counts[ii_type]] = ((vlan_info.vlan_tpid << 16) | vlan_info.vlan_tag);
			create->valid_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_INGRESS_VLAN_VALID;
			DEBUG_TRACE("%px: vlan tag: %x\n", nmci, create->ingress_vlan_tag[interface_type_counts[ii_type]]);
#else
			rule_invalid = true;
			DEBUG_TRACE("%px: VLAN - unsupported\n", nmci);
#endif
			break;
		case ECM_DB_IFACE_TYPE_PPPOE:
#ifdef ECM_INTERFACE_PPPOE_ENABLE
			/*
			 * More than one PPPoE in the list is not valid!
			 */
			if (interface_type_counts[ii_type] != 0) {
				DEBUG_TRACE("%px: PPPoE - additional unsupported\n", nmci);
				rule_invalid = true;
				break;
			}

			/*
			 * Set the PPPoE rule creation structure.
			 */
			create->valid_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_INGRESS_PPPOE;
			DEBUG_TRACE("%px: PPPoE - ingress interface is valid\n", nmci);
#else
			rule_invalid = true;
#endif
			break;
		default:
			DEBUG_TRACE("%px: Ignoring: %d (%s)\n", nmci, ii_type, ii_name);
		}
		interface_type_counts[ii_type]++;
	}

	ecm_db_connection_interfaces_deref(from_ifaces, from_ifaces_first);

	ret = ecm_db_multicast_connection_to_interfaces_get_and_ref_all(feci->ci, &to_ifaces, &to_ifaces_first);
	if (!ret) {
		DEBUG_WARN("%px: Accel attempt failed - no multicast interfaces in to_interfaces list!\n", nmci);
		kfree(nim);
		return;
	}

	from_nat_ifaces_first = ecm_db_connection_interfaces_get_and_ref(feci->ci, from_nat_ifaces, ECM_DB_OBJ_DIR_FROM_NAT);
	from_nat_ifaces_identifier = ecm_db_iface_interface_identifier_get(from_nat_ifaces[ECM_DB_IFACE_HEIRARCHY_MAX - 1]);
	ecm_db_connection_interfaces_deref(from_nat_ifaces, from_nat_ifaces_first);
	rule_invalid = false;

	/*
	 * Now examine the TO / DEST heirarchy list to construct the destination part of the rule
	 */
	DEBUG_TRACE("%px: Examine to/dest heirarchy list\n", nmci);
	for (vif = 0; vif < ECM_DB_MULTICAST_IF_MAX; vif++) {
		int32_t found_nat_ii_match = 0;
		int32_t to_mtu = 0;

		to_nss_iface_id = -1;

		create->if_rule[vif].egress_vlan_tag[0] = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;
		create->if_rule[vif].egress_vlan_tag[1] = ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED;

		ii_temp = ecm_db_multicast_if_heirarchy_get(to_ifaces, vif);
		to_ii_first = ecm_db_multicast_if_first_get_at_index(to_ifaces_first, vif);
		memset(interface_type_counts, 0, sizeof(interface_type_counts));

		for (list_index = *to_ii_first; !rule_invalid && (list_index < ECM_DB_IFACE_HEIRARCHY_MAX); list_index++) {
			struct ecm_db_iface_instance *ii;
			int32_t ii_identifier;
			ecm_db_iface_type_t ii_type;
			char *ii_name;
#ifdef ECM_INTERFACE_VLAN_ENABLE
			struct ecm_db_interface_info_vlan vlan_info;
			struct net_device *vlan_out_dev = NULL;
			uint32_t vlan_prio = 0;
#endif

			ii_single = ecm_db_multicast_if_instance_get_at_index(ii_temp, list_index);
			ifaces = (struct ecm_db_iface_instance **)ii_single;
			ii = *ifaces;
			ii_type = ecm_db_iface_type_get(ii);
			ii_name = ecm_db_interface_type_to_string(ii_type);
			ii_identifier = ecm_db_iface_interface_identifier_get(ii);

			/*
			 * Find match for NAT interface in Multicast destination interface list.
			 * If found match, set the found_nat_ii_match flag here.
			 */
			if (ii_identifier == from_nat_ifaces_identifier) {
				found_nat_ii_match = 1;
			}

			DEBUG_TRACE("%px: list_index: %d, ii: %px, type: %d (%s)\n", nmci, list_index, ii, ii_type, ii_name);

			/*
			 * Extract information from this interface type if it is applicable to the rule.
			 * Conflicting information may cause accel to be unsupported.
			 */
			switch (ii_type) {
			case ECM_DB_IFACE_TYPE_BRIDGE:

				/*
				 * TODO: Find and set the bridge/route flag for this interface
				 */
				DEBUG_TRACE("%px: Bridge\n", nmci);
				if (interface_type_counts[ii_type] != 0) {
					/*
					 * Cannot cascade bridges
					 */
					rule_invalid = true;
					DEBUG_TRACE("%px: Bridge - ignore additional\n", nmci);
					break;
				}
				ecm_db_iface_bridge_address_get(ii, to_nss_iface_address);
				to_iface_bridge_identifier = ecm_db_iface_interface_identifier_get(ii);
				DEBUG_TRACE("%px: Bridge - mac: %pM\n", nmci, to_nss_iface_address);
				break;
			case ECM_DB_IFACE_TYPE_ETHERNET:
				DEBUG_TRACE("%px: Ethernet\n", nmci);
				if (interface_type_counts[ii_type] != 0) {
					/*
					 * Ignore additional mac addresses, these are usually as a result of address propagation
					 * from bridges down to ports etc.
					 */
					DEBUG_TRACE("%px: Ethernet - ignore additional\n", nmci);
					break;
				}

				/*
				 * Can only handle one MAC, the first outermost mac.
				 */
				ecm_db_iface_ethernet_address_get(ii, to_nss_iface_address);
				to_mtu = (uint32_t)ecm_db_connection_iface_mtu_get(feci->ci, ECM_DB_OBJ_DIR_TO);
				to_nss_iface_id = ecm_db_iface_ae_interface_identifier_get(ii);
				if (to_nss_iface_id < 0) {
					DEBUG_TRACE("%px: to_nss_iface_id: %d\n", nmci, to_nss_iface_id);
					ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);
					kfree(nim);
					return;
			        }
				DEBUG_TRACE("%px: Ethernet - mac: %pM, mtu %d\n", nmci, to_nss_iface_address, to_mtu);
				break;
			case ECM_DB_IFACE_TYPE_PPPOE:
#ifdef ECM_INTERFACE_PPPOE_ENABLE
				/*
				 * More than one PPPoE in the list is not valid!
				 */
				if (interface_type_counts[ii_type] != 0) {
					DEBUG_TRACE("%px: PPPoE - additional unsupported\n", nmci);
					rule_invalid = true;
					break;
				}

				/*
				 * Set the PPPoE rule creation structure.
				 */
				create->if_rule[valid_vif_idx].pppoe_if_num = ecm_db_iface_ae_interface_identifier_get(ii);
				if (create->if_rule[valid_vif_idx].pppoe_if_num < 0) {
					DEBUG_TRACE("%px: PPPoE - acceleration engine interface (%d) is not valid\n",
							nmci, create->if_rule[valid_vif_idx].pppoe_if_num);
					rule_invalid = true;
					break;
				}
				create->if_rule[valid_vif_idx].valid_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_PPPOE_VALID;
				DEBUG_TRACE("%px: PPPoE - exist if_num: %d\n", nmci,
							create->if_rule[valid_vif_idx].pppoe_if_num);
#else
				DEBUG_TRACE("%px: PPPoE - unsupported\n", nmci);
				rule_invalid = true;
#endif
				break;
			case ECM_DB_IFACE_TYPE_VLAN:
#ifdef ECM_INTERFACE_VLAN_ENABLE
				DEBUG_TRACE("%px: VLAN\n", nmci);
				if (interface_type_counts[ii_type] > 1) {
					/*
					 * Can only support two vlans
					 */
					rule_invalid = true;
					DEBUG_TRACE("%px: VLAN - additional unsupported\n", nmci);
					break;
				}

				ecm_db_iface_vlan_info_get(ii, &vlan_info);
				create->if_rule[valid_vif_idx].egress_vlan_tag[interface_type_counts[ii_type]] = ((vlan_info.vlan_tpid << 16) | vlan_info.vlan_tag);

				vlan_out_dev = dev_get_by_index(&init_net, ecm_db_iface_interface_identifier_get(ii));
				if (vlan_out_dev) {
					vlan_prio = vlan_dev_get_egress_qos_mask(vlan_out_dev, pr->flow_qos_tag);
					create->if_rule[valid_vif_idx].egress_vlan_tag[interface_type_counts[ii_type]] |= vlan_prio;
					dev_put(vlan_out_dev);
					vlan_out_dev = NULL;
				}

				/*
				 * If we have not yet got an ethernet mac then take this one (very unlikely as mac should have been propagated to the slave (outer) device
				 */
				if (interface_type_counts[ECM_DB_IFACE_TYPE_ETHERNET] == 0) {
					memcpy(to_nss_iface_address, vlan_info.address, ETH_ALEN);
					interface_type_counts[ECM_DB_IFACE_TYPE_ETHERNET]++;
					DEBUG_TRACE("%px: VLAN use mac: %pM\n", nmci, to_nss_iface_address);
				}
				create->if_rule[valid_vif_idx].valid_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_VLAN_VALID;
				DEBUG_TRACE("%px: vlan tag: %x\n", nmci, create->if_rule[vif].egress_vlan_tag[interface_type_counts[ii_type]]);
#else
				rule_invalid = true;
				DEBUG_TRACE("%px: VLAN - unsupported\n", nmci);
#endif
				break;
			default:
				DEBUG_TRACE("%px: Ignoring: %d (%s)\n", nmci, ii_type, ii_name);
			}

			/*
			 * Seen an interface of this type
			 */
			interface_type_counts[ii_type]++;
		}

		if (rule_invalid) {
			DEBUG_WARN("%px: to/dest Rule invalid\n", nmci);
			ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);
			kfree(nim);
			return;
		}

		/*
		 * Populate the interface details for a valid interface in the multicast destination
		 * interface list.
		 */
		if (to_nss_iface_id != -1) {
			uint32_t xlate_src_ip, src_ip;
			bool is_bridge;
			ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM_NAT, addr);
			ECM_IP_ADDR_TO_HIN4_ADDR(xlate_src_ip, addr);
			ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, addr);
			ECM_IP_ADDR_TO_HIN4_ADDR(src_ip, addr);

			/*
			 * Set a rule for NAT if found_nat_ii_match flag is set
			 * and source IP of packet is not matching with host IP address.
			 */
			if (found_nat_ii_match && (xlate_src_ip != src_ip)) {
				create->if_rule[valid_vif_idx].xlate_src_ip = xlate_src_ip;
				create->if_rule[valid_vif_idx].xlate_src_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_FROM_NAT);
				create->if_rule[valid_vif_idx].valid_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_NAT_VALID;
			} else {
				create->if_rule[valid_vif_idx].xlate_src_ip = src_ip;
				create->if_rule[valid_vif_idx].xlate_src_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_FROM);
			}
			create->if_rule[valid_vif_idx].rule_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_JOIN;
			create->if_rule[valid_vif_idx].if_num = to_nss_iface_id;
			create->if_rule[valid_vif_idx].if_mtu = to_mtu;
			is_bridge = !ecm_db_connection_is_routed_get(feci->ci);

			/*
			 * Identify if the destination interface blongs to pure bridge or routed flow.
			 */
			if (is_bridge) {
				uint8_t from_nss_iface_address[ETH_ALEN];
				ecm_db_connection_node_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, (uint8_t *)from_nss_iface_address);
				memcpy(create->if_rule[valid_vif_idx].if_mac, from_nss_iface_address, ETH_ALEN);
				create->if_rule[valid_vif_idx].rule_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_BRIDGE_FLOW;
			} else {
				memcpy(create->if_rule[valid_vif_idx].if_mac, to_nss_iface_address, ETH_ALEN);
				create->if_rule[valid_vif_idx].rule_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_ROUTED_FLOW;
			}

#ifdef ECM_CLASSIFIER_OVS_ENABLE
			if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_OVS_VLAN_TAG) {
				/*
				 * Set primary egress VLAN tag
				 */
				if (pr->egress_mc_vlan_tag[vif][0] != ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED) {
					create->if_rule[valid_vif_idx].egress_vlan_tag[0] = pr->egress_mc_vlan_tag[vif][0];
					create->if_rule[valid_vif_idx].valid_flags |= NSS_IPV4_MC_RULE_CREATE_IF_FLAG_VLAN_VALID;

					/*
					 * Set secondary egress VLAN tag
					 */
					if (pr->egress_mc_vlan_tag[vif][1] != ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED) {
						create->if_rule[valid_vif_idx].egress_vlan_tag[1] = pr->egress_mc_vlan_tag[vif][1];
					}
				}
			}
#endif
			valid_vif_idx++;
		}
	}

	create->if_count = valid_vif_idx;
	create->src_interface_num = from_nss_iface_id;

	/*
	 * Set up the flow qos tags
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_QOS_TAG) {
		create->qos_tag = (uint32_t)pr->flow_qos_tag;
		create->valid_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_QOS_VALID;
	}

#ifdef ECM_CLASSIFIER_DSCP_ENABLE
#ifdef ECM_CLASSIFIER_DSCP_IGS
	/*
	 * Set up ingress shaper flow qos tags.
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_IGS_QOS_TAG) {
		create->igs_qos_tag = (uint16_t)pr->igs_flow_qos_tag;
		create->valid_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_IGS_VALID;
	}
#endif
	/*
	 * DSCP information?
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_DSCP) {
			create->egress_dscp = pr->flow_dscp;
			create->valid_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_DSCP_MARKING_VALID;
	}
#endif

#ifdef ECM_CLASSIFIER_EMESH_ENABLE
	/*
	 * Mark the rule as E-MESH Service Prioritization valid.
	 */
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_EMESH_SP_FLOW) {
		create->rule_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_MC_EMESH_SP;
	}
#endif

#ifdef ECM_CLASSIFIER_OVS_ENABLE
	if (pr->process_actions & ECM_CLASSIFIER_PROCESS_ACTION_OVS_VLAN_TAG) {
		/*
		 * Set ingress VLAN tag
		 */
		if (pr->ingress_vlan_tag[0] != ECM_FRONT_END_VLAN_ID_NOT_CONFIGURED) {
			create->ingress_vlan_tag[0] = pr->ingress_vlan_tag[0];
			create->valid_flags |= NSS_IPV4_MC_RULE_CREATE_FLAG_INGRESS_VLAN_VALID;
			ecm_db_multicast_tuple_set_ovs_ingress_vlan(feci->ci->ti, pr->ingress_vlan_tag);
		}
	}
#endif

	ecm_db_connection_node_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, dest_mac);
	memcpy(create->dest_mac, dest_mac, ETH_ALEN);

	/*
	 * Set protocol
	 */
	create->tuple.protocol = IPPROTO_UDP;

	/*
	 * The src_ip is where the connection established from
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, addr);
	ECM_IP_ADDR_TO_HIN4_ADDR(create->tuple.flow_ip, addr);

	/*
	 * The destination address is what the destination IP is translated to as it is forwarded to the next interface.
	 * For egress this would yield the normal wan host and for ingress this would correctly NAT back to the LAN host
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, addr);
	ECM_IP_ADDR_TO_HIN4_ADDR(create->tuple.return_ip, addr);

	/*
	 * Same approach as above for port information
	 */
	create->tuple.flow_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_FROM);
	create->tuple.return_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_TO);

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
		aci = assignments[aci_index];
		DEBUG_TRACE("%px: sync from: %px, type: %d\n", nmci, aci, aci->type_get(aci));
		aci->sync_from_v4(aci, &ecrc);
	}
	ecm_db_connection_assignments_release(assignment_count, assignments);

	/*
	 * Release the interface lists
	 */
	ecm_db_multicast_connection_to_interfaces_deref_all(to_ifaces, to_ifaces_first);

	for (vif = 0; vif < valid_vif_idx ; vif++) {
		DEBUG_TRACE("%px: UDP Accelerate connection %px\n"
			"Vif: %d\n"
			"Protocol: %d\n"
			"to_mtu: %u\n"
			"from_ip: %pI4h:%d\n"
			"to_ip: %pI4h:%d\n"
			"xlate_ip: %pI4h:%d\n"
			"to_mac: %pM\n"
			"dest_iface_num: %u\n"
			"in_vlan[0] %x\n"
			"in_vlan[1] %x\n"
			"out_vlan[0] %x\n"
			"out_vlan[1] %x\n",
			nmci,
			feci->ci,
			vif,
			create->tuple.protocol,
			create->if_rule[vif].if_mtu,
			&create->tuple.flow_ip, create->tuple.flow_ident,
			&create->tuple.return_ip, create->tuple.return_ident,
			&create->if_rule[vif].xlate_src_ip, create->if_rule[vif].xlate_src_ident,
			create->if_rule[vif].if_mac,
			create->if_rule[vif].if_num,
			create->ingress_vlan_tag[0],
			create->ingress_vlan_tag[1],
			create->if_rule[vif].egress_vlan_tag[0],
			create->if_rule[vif].egress_vlan_tag[1]);
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
		ecm_nss_ipv4_accel_pending_clear(feci, ECM_FRONT_END_ACCELERATION_MODE_DECEL);
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
	nss_tx_status = nss_ipv4_tx(ecm_nss_ipv4_nss_ipv4_mgr, nim);
	if (nss_tx_status == NSS_TX_SUCCESS) {
		/*
		 * Reset the driver_fail count - transmission was okay here.
		 */
		spin_lock_bh(&feci->lock);
		nmci->base.stats.driver_fail = 0; /* Reset */
		spin_unlock_bh(&feci->lock);
		kfree(nim);
		return;
	}

	/*
	 * Release that ref!
	 */
	ecm_db_connection_deref(feci->ci);

	/*
	 * TX failed
	 */
	spin_lock_bh(&feci->lock);
	DEBUG_ASSERT(feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL_PENDING, "%px: accel mode unexpected: %d\n", nmci, feci->accel_mode);
	nmci->base.stats.driver_fail_total++;
	nmci->base.stats.driver_fail++;
	if (feci->stats.driver_fail >= feci->stats.driver_fail_limit) {
		DEBUG_WARN("%px: Accel failed - driver fail limit\n", nmci);
		result_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DRIVER;
	} else {
		result_mode = ECM_FRONT_END_ACCELERATION_MODE_DECEL;
	}

	spin_lock_bh(&ecm_nss_ipv4_lock);
	_ecm_nss_ipv4_accel_pending_clear(feci, result_mode);
	spin_unlock_bh(&ecm_nss_ipv4_lock);

	spin_unlock_bh(&feci->lock);

	kfree(nim);

	return;
}

/*
 * ecm_nss_multicast_ipv4_connection_destroy_callback()
 *	Callback for handling destroy ack/nack calls.
 */
static void ecm_nss_multicast_ipv4_connection_destroy_callback(void *app_data, struct nss_ipv4_msg *nim)
{
	struct nss_ipv4_rule_destroy_msg * __attribute__((unused))nirdm = &nim->msg.rule_destroy;
	uint32_t serial = (uint32_t)(ecm_ptr_t)app_data;
	struct ecm_db_connection_instance *ci;
	struct ecm_front_end_connection_instance *feci;
	struct ecm_nss_multicast_ipv4_connection_instance *nmci;

	/*
	 * Is this a response to a destroy message?
	 */
	if (nim->cm.type != NSS_IPV4_TX_DESTROY_RULE_MSG) {
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
	nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;
	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

	/*
	 * Record command duration
	 */
	ecm_nss_ipv4_decel_done_time_update(feci);

	/*
	 * Dump some useful trace information.
	 */
	DEBUG_TRACE("%px: decelerate response for connection: %px\n", nmci, feci->ci);
	DEBUG_TRACE("%px: flow_ip: %pI4h:%d\n", nmci, &nirdm->tuple.flow_ip, nirdm->tuple.flow_ident);
	DEBUG_TRACE("%px: return_ip: %pI4h:%d\n", nmci, &nirdm->tuple.return_ip, nirdm->tuple.return_ident);
	DEBUG_TRACE("%px: protocol: %d\n", nmci, nirdm->tuple.protocol);

	/*
	 * Drop decel pending counter
	 */
	spin_lock_bh(&ecm_nss_ipv4_lock);
	ecm_nss_ipv4_pending_decel_count--;
	DEBUG_ASSERT(ecm_nss_ipv4_pending_decel_count >= 0, "Bad decel pending counter\n");
	spin_unlock_bh(&ecm_nss_ipv4_lock);

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

	DEBUG_TRACE("%px: response: %d\n", nmci, nim->cm.response);
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
	 * Mcast acceleration ends
	 */
	spin_lock_bh(&ecm_nss_ipv4_lock);
	ecm_nss_multicast_ipv4_accelerated_count--;	/* Protocol specific counter */
	DEBUG_ASSERT(ecm_nss_multicast_ipv4_accelerated_count >= 0, "Bad udp accel counter\n");
	ecm_nss_ipv4_accelerated_count--;		/* General running counter */
	DEBUG_ASSERT(ecm_nss_ipv4_accelerated_count >= 0, "Bad accel counter\n");
	spin_unlock_bh(&ecm_nss_ipv4_lock);

	/*
	 * Release the connections.
	 */
	feci->deref(feci);
	ecm_db_connection_deref(ci);
}

/*
 * ecm_nss_multicast_ipv4_connection_decelerate_msg_send()
 *	Prepares and sends a decelerate message to acceleration engine.
 */
static bool ecm_nss_multicast_ipv4_connection_decelerate_msg_send(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;
	struct nss_ipv4_msg nim;
	struct nss_ipv4_rule_destroy_msg *nirdm;
	ip_addr_t src_addr;
	ip_addr_t group_addr;
	nss_tx_status_t nss_tx_status;
	bool ret;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

	/*
	 * Increment the decel pending counter
	 */
	spin_lock_bh(&ecm_nss_ipv4_lock);
	ecm_nss_ipv4_pending_decel_count++;
	spin_unlock_bh(&ecm_nss_ipv4_lock);

	/*
	 * Prepare deceleration message
	 */
	nss_ipv4_msg_init(&nim, NSS_IPV4_RX_INTERFACE, NSS_IPV4_TX_DESTROY_RULE_MSG,
			sizeof(struct nss_ipv4_rule_destroy_msg),
			ecm_nss_multicast_ipv4_connection_destroy_callback,
			(void *)(ecm_ptr_t)ecm_db_connection_serial_get(feci->ci));

	nirdm = &nim.msg.rule_destroy;
	nirdm->tuple.protocol = (int32_t)ecm_db_connection_protocol_get(feci->ci);

	/*
	 * Get addressing information
	 */
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_FROM, src_addr);
	ECM_IP_ADDR_TO_HIN4_ADDR(nirdm->tuple.flow_ip, src_addr);
	ecm_db_connection_address_get(feci->ci, ECM_DB_OBJ_DIR_TO, group_addr);
	ECM_IP_ADDR_TO_HIN4_ADDR(nirdm->tuple.return_ip, group_addr);
	nirdm->tuple.flow_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_FROM);
	nirdm->tuple.return_ident = ecm_db_connection_port_get(feci->ci, ECM_DB_OBJ_DIR_TO);

	DEBUG_INFO("%px: Mcast Connection %px decelerate\n"
			"protocol: %d\n"
			"src_ip: %pI4:%d\n"
			"dest_ip: %pI4:%d\n",
			nmci, feci->ci, nirdm->tuple.protocol,
			&nirdm->tuple.flow_ip, nirdm->tuple.flow_ident,
			&nirdm->tuple.return_ip, nirdm->tuple.return_ident);

	/*
	 * Right place to free multicast destination interfaces list.
	 */
	ecm_db_multicast_connection_to_interfaces_clear(feci->ci);

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
	nss_tx_status = nss_ipv4_tx(ecm_nss_ipv4_nss_ipv4_mgr, &nim);
	if (nss_tx_status == NSS_TX_SUCCESS) {
		spin_lock_bh(&feci->lock);
		feci->stats.driver_fail = 0;			/* Reset */
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
	spin_lock_bh(&ecm_nss_ipv4_lock);
	ecm_nss_ipv4_pending_decel_count--;
	DEBUG_ASSERT(ecm_nss_ipv4_pending_decel_count >= 0, "Bad decel pending counter\n");
	spin_unlock_bh(&ecm_nss_ipv4_lock);

	return ret;
}

/*
 * ecm_nss_multicast_ipv4_connection_decelerate()
 *     Decelerate a connection
 */
static bool ecm_nss_multicast_ipv4_connection_decelerate(struct ecm_front_end_connection_instance *feci)
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

	return ecm_nss_multicast_ipv4_connection_decelerate_msg_send(feci);
}

/*
 * ecm_nss_multicast_ipv4_connection_defunct_callback()
 *	Callback to be called when a Mcast connection has become defunct.
 */
bool ecm_nss_multicast_ipv4_connection_defunct_callback(void *arg, int *accel_mode)
{
	bool ret;
	struct ecm_front_end_connection_instance *feci = (struct ecm_front_end_connection_instance *)arg;
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

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

	/*
	 * If none of the cases matched above, this means the connection is in the
	 * accel mode, so we force a deceleration.
	 * NOTE: If the mode is accel pending then the decel will be actioned when that is completed.
	 */
	ret = ecm_nss_multicast_ipv4_connection_decelerate_msg_send(feci);

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
 * ecm_nss_multicast_ipv4_connection_accel_state_get()
 *	Get acceleration state
 */
static ecm_front_end_acceleration_mode_t ecm_nss_multicast_ipv4_connection_accel_state_get(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;
	ecm_front_end_acceleration_mode_t state;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);
	spin_lock_bh(&feci->lock);
	state = feci->accel_mode;
	spin_unlock_bh(&feci->lock);
	return state;
}

/*
 * ecm_nss_multicast_ipv4_connection_action_seen()
 *	Acceleration action / activity has been seen for this connection.
 *
 * NOTE: Call the action_seen() method when the NSS has demonstrated that it has offloaded some data for a connection.
 */
static void ecm_nss_multicast_ipv4_connection_action_seen(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);
	DEBUG_INFO("%px: Action seen\n", nmci);
	spin_lock_bh(&feci->lock);
	feci->stats.no_action_seen = 0;
	spin_unlock_bh(&feci->lock);
}

/*
 * ecm_nss_multicast_ipv4_connection_accel_ceased()
 *	NSS has indicated that acceleration has stopped.
 *
 * NOTE: This is called in response to an NSS self-initiated termination of acceleration.
 * This must NOT be called because the ECM terminated the acceleration.
 */
static void ecm_nss_multicast_ipv4_connection_accel_ceased(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);
	DEBUG_INFO("%px: accel ceased\n", nmci);

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
	 * Mcast acceleration ends
	 */
	spin_lock_bh(&ecm_nss_ipv4_lock);
	ecm_nss_multicast_ipv4_accelerated_count--;	/* Protocol specific counter */
	DEBUG_ASSERT(ecm_nss_multicast_ipv4_accelerated_count >= 0, "Bad Mcast accel counter\n");
	ecm_nss_ipv4_accelerated_count--;		/* General running counter */
	DEBUG_ASSERT(ecm_nss_ipv4_accelerated_count >= 0, "Bad accel counter\n");
	spin_unlock_bh(&ecm_nss_ipv4_lock);
}

/*
 * ecm_nss_multicast_ipv4_connection_ref()
 *	Ref a connection front end instance
 */
static void ecm_nss_multicast_ipv4_connection_ref(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);
	spin_lock_bh(&feci->lock);
	feci->refs++;
	DEBUG_TRACE("%px: nmci ref %d\n", feci, feci->refs);
	DEBUG_ASSERT(feci->refs > 0, "%px: ref wrap\n", feci);
	spin_unlock_bh(&feci->lock);
}

/*
 * ecm_nss_multicast_ipv4_connection_deref()
 *	Deref a connection front end instance
 */
static int ecm_nss_multicast_ipv4_connection_deref(struct ecm_front_end_connection_instance *feci)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

	spin_lock_bh(&feci->lock);
	feci->refs--;
	DEBUG_ASSERT(feci->refs >= 0, "%px: ref wrap\n", feci);

	if (feci->refs > 0) {
		int refs = feci->refs;
		spin_unlock_bh(&feci->lock);
		DEBUG_TRACE("%px: nmci deref %d\n", nmci, refs);
		return refs;
	}
	spin_unlock_bh(&feci->lock);

	/*
	 * We can now destroy the instance
	 */
	DEBUG_TRACE("%px: nmci final\n", nmci);
	DEBUG_CLEAR_MAGIC(nmci);
	kfree(nmci);
	return 0;
}

#ifdef ECM_STATE_OUTPUT_ENABLE
/*
 * ecm_nss_multicast_ipv4_connection_state_get()
 *	Return state of this multicast front end instance
 */
static int ecm_nss_multicast_ipv4_connection_state_get(struct ecm_front_end_connection_instance *feci, struct ecm_state_file_instance *sfi)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)feci;

	DEBUG_CHECK_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC, "%px: magic failed", nmci);

	return ecm_front_end_common_connection_state_get(feci, sfi, "nss_v4.multicast");
}
#endif

/*
 * ecm_nss_multicast_ipv4_bridge_update_connections()
 * 	Update NSS with new multicast egress ports.
 */
static void ecm_nss_multicast_ipv4_bridge_update_connections(ip_addr_t dest_ip, struct net_device *brdev)
{
	struct ecm_front_end_connection_instance *feci;
	struct ecm_db_multicast_tuple_instance *ti;
	struct ecm_db_multicast_tuple_instance *ti_next;
	struct ecm_multicast_if_update mc_update;
	struct ecm_db_connection_instance *ci;
	ip_addr_t grp_ip;
	ip_addr_t src_ip;
	int i, ret;
	uint32_t mc_dst_dev[ECM_DB_MULTICAST_IF_MAX];
	int32_t if_num;
	uint32_t mc_flags = 0;
	bool if_update;
	bool is_routed;
	struct net_device *l2_br_dev, *l3_br_dev;

	ti = ecm_db_multicast_connection_get_and_ref_first(dest_ip);
	if (!ti) {
		DEBUG_WARN("no multicast tuple entry found. Group IP: " ECM_IP_ADDR_OCTAL_FMT , ECM_IP_ADDR_TO_OCTAL(dest_ip));
		return;
	}

	while (ti) {
		struct ecm_classifier_process_response aci_pr;

		memset(&aci_pr, 0, sizeof(aci_pr));

		/*
		 * Get the group IP address stored in tuple_instance and match this with
		 * the group IP received from MCS update callback.
		 */
		ecm_db_multicast_tuple_instance_group_ip_get(ti, grp_ip);
		if (!ECM_IP_ADDR_MATCH(grp_ip, dest_ip)) {
			goto find_next_tuple;
		}

		/*
		 * Get the source IP address for this entry for the group
		 */
		ecm_db_multicast_tuple_instance_source_ip_get(ti, src_ip);

		/*
		 * Before querying mcs, clear the destination interface list, as we get
		 * the interface list for each unique source IP.
		 *
		 * Query bridge snooper for the destination list when given the group and source
		 * if, 	if_num < 0   mc_bridge_ipv4_get_if has encountered with some error, check for next existing tuple.
		 * 	if_num == 0   All slaves have left the group. Deacel the flow.
		 * 	if_num > 0   An interface has either left or joined the group. Process the leave/join interface request.
		 */
		memset(mc_dst_dev, 0, sizeof(mc_dst_dev));
		if_num = mc_bridge_ipv4_get_if(brdev, htonl(src_ip[0]), htonl(dest_ip[0]), ECM_DB_MULTICAST_IF_MAX, mc_dst_dev);
		if (if_num < 0) {
			/*
			 * This may a valid case when all the interfaces have left a multicast group.
			 * In this case the MCS will return if_num 0, But we may have an oudated
			 * interface in multicast interface hierarchy list. At next step we have to
			 * check whether the DB instance is present or not.
			 */
			DEBUG_TRACE("No valid bridge slaves for the group/source\n");
			goto find_next_tuple;
		}

		/*
		 * Get a DB connection instance for the 5-tuple
		 */
		ci = ecm_db_multicast_connection_get_from_tuple(ti);
		DEBUG_TRACE("MCS-cb: src_ip = 0x%x, dest_ip = 0x%x, Num if = %d\n", src_ip[0], dest_ip[0], if_num);
		is_routed = ecm_db_connection_is_routed_get(ci);

		/*
		 * The source interface could have joined the group as well.
		 * In such cases, the destination interface list returned by
		 * the snooper would include the source interface as well.
		 * We need to filter the source interface from the list in such cases.
		 */
		if (if_num > 0) {
			if_num = ecm_interface_multicast_filter_src_interface(ci, mc_dst_dev);
			if (if_num == ECM_DB_IFACE_HEIRARCHY_MAX) {
				DEBUG_WARN("%px: MCS Snooper Update: no interfaces in from_interfaces list!\n", ci);
				goto find_next_tuple;
			}
		}

		feci = ecm_db_connection_front_end_get_and_ref(ci);

		/*
		 * All bridge slaves have left the group. If flow is pure bridge, decel the connection and return.
		 * If flow is routed, let MFC callback handle this.
		 *
		 * If there are no routed interfaces, then decelerate. Else
		 * we first send an update message to the firmware for the
		 * interface that have left, before issuing a decelerate
		 * at a later point via the MFC callback. This is because
		 * there might be a few seconds delay before MFC issues
		 * the delete callback
		 *
		 */
		if (!is_routed) {
			/*
			 * L2-only multicast: Update the flow only if the flow's bridge device matches the bridge device passed by MCS.
			 */
			if (!if_num) {
				/*
				 * Decelerate the flow since there is no active ports left
				 */
				feci->decelerate(feci);
				feci->deref(feci);
				goto find_next_tuple;
			}

			/*
			 * Decelerate the flow if the bridge device from the MCS update does not match the bridge with which flow was created.
			 */
			l2_br_dev = ecm_db_multicast_tuple_instance_get_l2_br_dev(ti);
			if (!l2_br_dev) {
				DEBUG_WARN("Not found a valid l2_br_dev in ti for bridged mc flow");
				feci->deref(feci);
				goto find_next_tuple;
			}

			if (l2_br_dev != brdev) {
				DEBUG_WARN("L2 bridge device does not match the MCS update. l2_br_dev:%s brdev:%s", l2_br_dev->name, brdev->name);
				feci->deref(feci);
				goto find_next_tuple;
			}
		} else {
			/*
			 * Check whether the bridge for which we are processing an update,
			 * is part of the the routed destination interface list for the flow.
			 * The flow could be one of the below
			 *
			 * a. WAN (upstream) <-> br-lan (downstream)
			 * b. WAN (upstream) <-> br-lan1(downstream), br-lan2 (downstream)
			 */
			int i;
			__be32 ip_src;
			__be32 ip_grp;
			uint32_t dst_if_cnt;
			uint32_t dst_dev[ECM_DB_MULTICAST_IF_MAX];

			/*
			 * If l3_br_dev is NULL then the we only need to check the ipmr destination list.
			 */
			l3_br_dev = ecm_db_multicast_tuple_instance_get_l3_br_dev(ti);
			if (!l3_br_dev) {
				goto process_ipmr_entry;
			}

			/*
			 * 'brdev' is already part of the multicast interface list, no need to check ipmr entry.
			 */
			if (l3_br_dev == brdev) {
				goto process_packet;
			}

process_ipmr_entry:
			memset(dst_dev, 0, sizeof(dst_dev));
			ECM_IP_ADDR_TO_NIN4_ADDR(ip_src, src_ip);
			ECM_IP_ADDR_TO_NIN4_ADDR(ip_grp, grp_ip);
			dst_if_cnt =  ipmr_find_mfc_entry(&init_net, ip_src, ip_grp, ECM_DB_MULTICAST_IF_MAX, dst_dev);
			if (dst_if_cnt < 0) {
				/*
				 * Decelerate the flow since there is no active ports left
				 */
				DEBUG_WARN("Not found a valid vif count %d\n", dst_if_cnt);
				feci->decelerate(feci);
				feci->deref(feci);
				goto find_next_tuple;
			}

			/*
			 * Update should be allowed for the connection only if 'brdev' is part of ipmr destination interface list.
			 */
			for (i = 0; i < dst_if_cnt; i++) {
				if (dst_dev[i] == brdev->ifindex)
					goto process_packet;
			}

			DEBUG_WARN("brdev: %s is neither part of mcproxy configuration nor same as ingress bridge port device.\n", brdev->name);
			feci->deref(feci);
			goto find_next_tuple;
		}

process_packet:
		/*
		 * Find out changes to the destination interfaces hierarchy
		 * of the connection. We try to find out the interfaces that
		 * have joined new, and the existing interfaces in the list
		 * that have left seperately.
		 */
		memset(&mc_update, 0, sizeof(mc_update));
		spin_lock_bh(&ecm_nss_ipv4_lock);
		if_update = ecm_interface_multicast_find_updates_to_iface_list(ci, &mc_update, mc_flags, true, mc_dst_dev, if_num, brdev);
		spin_unlock_bh(&ecm_nss_ipv4_lock);
		if (!if_update) {
			/*
			 * No updates to this multicast flow. Move on to the next
			 * flow for the same group
			 */
			goto find_next_tuple;
		}

		DEBUG_TRACE("BRIDGE UPDATE callback ===> leave_cnt %d, join_cnt %d\n", mc_update.if_leave_cnt, mc_update.if_join_cnt);

		feci = ecm_db_connection_front_end_get_and_ref(ci);

		/*
		 * Do we have any new interfaces that have joined?
		 */
		if (mc_update.if_join_cnt) {
			struct ecm_db_iface_instance *to_list;
			int32_t to_list_first[ECM_DB_MULTICAST_IF_MAX];
			uint32_t if_cnt;
			uint8_t src_node_addr[ETH_ALEN];
			uint32_t tuple_instance_flags;
			__be16 layer4hdr[2] = {0, 0};
			__be16 port = 0;

			to_list = (struct ecm_db_iface_instance *)kzalloc(ECM_DB_TO_MCAST_INTERFACES_SIZE, GFP_ATOMIC | __GFP_NOWARN);
			if (!to_list) {
				feci->deref(feci);
				goto find_next_tuple;
			}

			/*
			 * Initialize the hierarchy's indices for the 'to_list'
			 * which will hold the interface hierarchies for the new joinees
			 */
			for (i = 0; i < ECM_DB_MULTICAST_IF_MAX; i++) {
				to_list_first[i] = ECM_DB_IFACE_HEIRARCHY_MAX;
			}

			ecm_db_connection_node_address_get(ci, ECM_DB_OBJ_DIR_FROM, src_node_addr);

			/*
			 * Create the interface hierarchy list for the new interfaces. We append this list later to
			 * the existing list of destination interfaces.
			 */
			port = (__be16)(ecm_db_connection_port_get(ci, ECM_DB_OBJ_DIR_FROM));
			layer4hdr[0] = htons(port);
			port = (__be16)(ecm_db_connection_port_get(ci, ECM_DB_OBJ_DIR_TO));
			layer4hdr[1] = htons(port);

			if_cnt = ecm_interface_multicast_heirarchy_construct_bridged(feci, to_list, brdev, src_ip, dest_ip, mc_update.if_join_cnt, mc_update.join_dev, to_list_first, src_node_addr, layer4hdr, NULL);
			if (!if_cnt) {
				DEBUG_WARN("Failed to obtain 'to_mcast_update' hierarchy list\n");
				feci->decelerate(feci);
				feci->deref(feci);
				kfree(to_list);
				goto find_next_tuple;
			}

			/*
			 * Append the interface hierarchy array of the new joinees to the existing destination list
			 */
			ecm_db_multicast_connection_to_interfaces_update(ci, to_list, to_list_first, mc_update.if_join_idx);

			/*
			 * In Routed + Bridge mode, if there is a group leave request arrives for the last
			 * slave of the bridge then MFC will clear ECM_DB_MULTICAST_CONNECTION_BRIDGE_DEV_SET_FLAG
			 * in tuple_instance. If the bridge slave joins again then we need to set the flag again
			 * in tuple_instance here.
			 */
			tuple_instance_flags = ecm_db_multicast_tuple_instance_flags_get(ti);
			if (is_routed && !(tuple_instance_flags & ECM_DB_MULTICAST_CONNECTION_BRIDGE_DEV_SET_FLAG)) {
				ecm_db_multicast_tuple_instance_flags_set(ti, ECM_DB_MULTICAST_CONNECTION_BRIDGE_DEV_SET_FLAG);
			}

			/*
			 * De-ref the updated destination interface list
			 */
			for (i = 0; i < ECM_DB_MULTICAST_IF_MAX; i++) {
				if (mc_update.if_join_idx[i]) {
					struct ecm_db_iface_instance *to_list_single;
					struct ecm_db_iface_instance *to_list_temp[ECM_DB_IFACE_HEIRARCHY_MAX];

					to_list_single = ecm_db_multicast_if_heirarchy_get(to_list, i);
					ecm_db_multicast_copy_if_heirarchy(to_list_temp, to_list_single);
					ecm_db_connection_interfaces_deref(to_list_temp, to_list_first[i]);
				}
			}

			kfree(to_list);
		} else if (mc_update.if_leave_cnt) {
			/*
			 * If these are the last interface set leaving the to interface
			 * list of the connection, then decelerate the connection
			 */
			int mc_to_interface_count = ecm_db_multicast_connection_to_interfaces_get_count(ci);

			if (mc_update.if_leave_cnt == mc_to_interface_count) {
				DEBUG_INFO("%px: Decelerating the flow as there are no to interfaces in the multicast group: 0x%x\n", feci, dest_ip[0]);
				feci->decelerate(feci);
				feci->deref(feci);
				goto find_next_tuple;
			}
		}

#ifdef ECM_CLASSIFIER_OVS_ENABLE
		/*
		 * Verify the 'to' interface list with OVS classifier.
		 */
		if (ecm_front_end_is_ovs_bridge_device(brdev) &&
			ecm_db_multicast_ovs_verify_to_list(ci, &aci_pr)) {
			/*
			 * We defunct the flow when the OVS returns "DENY_ACCEL" for the port.
			 * This can happen when the first port has joined on an OVS bridge
			 * on a flow that already exists. In this case, OVS needs to see the
			 * packet to update the flow. So we defunct the existing rule.
			 */
			DEBUG_WARN("%px: Verification of the ovs 'to_list' has failed. Hence, defunct the connection: %px\n", feci, feci->ci);
			ecm_db_connection_make_defunct(ci);
			feci->deref(feci);
			goto find_next_tuple;
		}
#endif
		/*
		 * Push the updates to NSS
		 */
		DEBUG_TRACE("%px: Update accel\n", ci);
		if ((feci->accel_mode <= ECM_FRONT_END_ACCELERATION_MODE_FAIL_DENIED) ||
				(feci->accel_mode != ECM_FRONT_END_ACCELERATION_MODE_ACCEL)) {
			DEBUG_TRACE("%px: Ignoring wrong mode accel for conn: %px\n", feci, feci->ci);
			feci->deref(feci);
			goto find_next_tuple;
		}

		/*
		 * Update the new rules in FW. If returns error decelerate the connection
		 * and flush all ECM rules.
		 */
		ret = ecm_nss_multicast_ipv4_connection_update_accelerate(feci, &mc_update, &aci_pr);
		if (ret < 0) {
			feci->decelerate(feci);
			feci->deref(feci);
			goto find_next_tuple;
		}

		feci->deref(feci);

		/*
		 * Release the interfaces that may have left the connection
		 */
		ecm_db_multicast_connection_to_interfaces_leave(ci, &mc_update);

find_next_tuple:
		ti_next = ecm_db_multicast_connection_get_and_ref_next(ti);
		ecm_db_multicast_connection_deref(ti);
		ti = ti_next;
	}
}

/*
 * ecm_nss_multicast_ipv4_connection_instance_alloc()
 *	Create a front end instance specific for Mcast connection
 */
struct ecm_nss_multicast_ipv4_connection_instance *ecm_nss_multicast_ipv4_connection_instance_alloc(
								bool can_accel,
								struct ecm_db_connection_instance **nci)
{
	struct ecm_nss_multicast_ipv4_connection_instance *nmci;
	struct ecm_front_end_connection_instance *feci;
	struct ecm_db_connection_instance *ci;

	if (ecm_nss_ipv4_is_conn_limit_reached()) {
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

	nmci = (struct ecm_nss_multicast_ipv4_connection_instance *)kzalloc(sizeof(struct ecm_nss_multicast_ipv4_connection_instance), GFP_ATOMIC | __GFP_NOWARN);
	if (!nmci) {
		DEBUG_WARN("Mcast Front end alloc failed\n");
		ecm_db_connection_deref(ci);
		return NULL;
	}

	/*
	 * Refs is 1 for the creator of the connection
	 */
	feci = (struct ecm_front_end_connection_instance *)nmci;
	feci->refs = 1;
	DEBUG_SET_MAGIC(nmci, ECM_NSS_MULTICAST_IPV4_CONNECTION_INSTANCE_MAGIC);
	spin_lock_init(&feci->lock);

	feci->can_accel = can_accel;
	feci->accel_mode = (can_accel)? ECM_FRONT_END_ACCELERATION_MODE_DECEL : ECM_FRONT_END_ACCELERATION_MODE_FAIL_DENIED;
	feci->accel_engine = ECM_FRONT_END_ENGINE_NSS;
	spin_lock_bh(&ecm_nss_ipv4_lock);
	feci->stats.no_action_seen_limit = ecm_nss_ipv4_no_action_limit_default;
	feci->stats.driver_fail_limit = ecm_nss_ipv4_driver_fail_limit_default;
	feci->stats.ae_nack_limit = ecm_nss_ipv4_nack_limit_default;
	spin_unlock_bh(&ecm_nss_ipv4_lock);

	/*
	 * Copy reference to connection - no need to ref ci as ci maintains a ref to this instance instead (this instance persists for as long as ci does)
	 */
	feci->ci = ci;

	feci->ip_version = 4;

	/*
	 * Populate the methods and callbacks
	 */
	feci->ref = ecm_nss_multicast_ipv4_connection_ref;
	feci->deref = ecm_nss_multicast_ipv4_connection_deref;
	feci->accelerate = ecm_nss_multicast_ipv4_connection_accelerate;
	feci->decelerate = ecm_nss_multicast_ipv4_connection_decelerate;
	feci->accel_state_get = ecm_nss_multicast_ipv4_connection_accel_state_get;
	feci->action_seen = ecm_nss_multicast_ipv4_connection_action_seen;
	feci->accel_ceased = ecm_nss_multicast_ipv4_connection_accel_ceased;
#ifdef ECM_STATE_OUTPUT_ENABLE
	feci->state_get = ecm_nss_multicast_ipv4_connection_state_get;
#endif
	feci->ae_interface_number_by_dev_get = ecm_nss_common_get_interface_number_by_dev;
	feci->ae_interface_number_by_dev_type_get = ecm_nss_common_get_interface_number_by_dev_type;
	feci->ae_interface_type_get = ecm_nss_common_get_interface_type;
	feci->regenerate = ecm_nss_common_connection_regenerate;
	feci->multicast_update = ecm_nss_multicast_ipv4_bridge_update_connections;

	return nmci;
}

/*
 * ecm_nss_multicast_ipv4_get_accelerated_count()
 */
ssize_t ecm_nss_multicast_ipv4_get_accelerated_count(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	ssize_t count;
	int num;

	/*
	 * Operate under our locks
	 */
	spin_lock_bh(&ecm_nss_ipv4_lock);
	num = ecm_nss_multicast_ipv4_accelerated_count;
	spin_unlock_bh(&ecm_nss_ipv4_lock);

	count = snprintf(buf, (ssize_t)PAGE_SIZE, "%d\n", num);
	return count;
}

/*
 * ecm_br_multicast_update_event_callback()
 * 	Callback received from bridge multicast snooper module in the
 * 	following events:
 * 	a.) Updates to a muticast group due to IGMP JOIN/LEAVE.
 *
 * 	group: IP multicast group address
 * 	brdev: bridge netdevice
 *
 * This function does the following:
 * a.) Lookup the tuple_instance table to find any flows through the bridge for the group
 * b.) Query the MCS bridge using the source and group address, to receive a list
 *     of bridge slaves for the flow
 * c.) Process the updates by matching the received bridge slave list with existing
 *     destination interfaces heirarchy for the CI, and decide whether to delete an
 *     exiting interface heirarchy or add a new heirarchy.
 */
static void ecm_br_multicast_update_event_callback(struct net_device *brdev, uint32_t group)
{
	ip_addr_t dest_ip;

	DEBUG_TRACE("ecm_br_multicast_update_event_callback 0x%x, brdev: %s\n", group, brdev->name);

	ECM_HIN4_ADDR_TO_IP_ADDR(dest_ip, htonl(group));
	ecm_nss_multicast_ipv4_bridge_update_connections(dest_ip, brdev);
}

/*
 * ecm_mfc_update_event_callback()
 * 	Callback called by Linux kernel multicast routing module in the
 * 	following events:
 * 	a.) Updates to destination interface lists for a multicast route due to
 * 	    IGMP JOIN/LEAVE
 * 	b.) Multicast route delete event
 *
 * 	group: IP multicast group address
 *	origin: IP source for the multicast route
 *	to_dev_idx[]: Array of ifindex for destination interfaces for the route
 *	max_to_dev: size of the array
 *	op: UPDATE or DELETE event
 *
 * This function does the following:
 * a.) Lookup the tuple_instance table to find any accelerated flows for the group/source,
 *     and access their CI
 * b.) Process the updates by matching the received destination interface list with existing
 *     destination interfaces heirarchy for the CI, and decide whether to delete an
 *     exiting interface heirarchy or add a new heirarchy.
 */
static void ecm_mfc_update_event_callback(__be32 group, __be32 origin, uint32_t max_to_dev, uint32_t to_dev_idx[], uint8_t op)
{
	struct ecm_db_connection_instance *ci;
	struct ecm_db_multicast_tuple_instance *tuple_instance;
	struct ecm_db_multicast_tuple_instance *tuple_instance_next;
	struct ecm_multicast_if_update mc_update;
	int32_t vif_cnt;
	ip_addr_t src_ip;
	ip_addr_t dest_ip;
	ip_addr_t src_addr;
	ip_addr_t grp_addr;
	uint32_t tuple_instance_flags = 0;
	__be16 layer4hdr[2] = {0, 0};
	__be16 port = 0;

	ECM_HIN4_ADDR_TO_IP_ADDR(src_ip, htonl(origin));
	ECM_HIN4_ADDR_TO_IP_ADDR(dest_ip, htonl(group));

	DEBUG_TRACE("origin : group " ECM_IP_ADDR_DOT_FMT ":" ECM_IP_ADDR_DOT_FMT "\n", ECM_IP_ADDR_TO_DOT(src_ip), ECM_IP_ADDR_TO_DOT(dest_ip));

	/*
	 * Access the 5-tuple information from the tuple_instance table, using the
	 * source and group addresses
	 */
	tuple_instance = ecm_db_multicast_connection_find_and_ref(src_ip, dest_ip);
	if (!tuple_instance) {
		DEBUG_TRACE("MFC_EVENT: Tuple info is not found\n");
		return;
	}

	switch (op) {
	case IPMR_MFC_EVENT_UPDATE:
	{
		struct ecm_front_end_connection_instance *feci;
		struct ecm_db_iface_instance *to_list;
		struct ecm_db_iface_instance *to_list_single;
		struct ecm_db_iface_instance *to_list_temp[ECM_DB_IFACE_HEIRARCHY_MAX];
		uint32_t to_list_first[ECM_DB_MULTICAST_IF_MAX];
		uint32_t i, ret;
		uint32_t mc_flag = 0;
		bool if_update = false;
		bool found_br_dev = false;

		/*
		 * Find the connections belongs to the same source and group address and
		 * apply the updates received from event handler
		 */
		while (tuple_instance) {
			struct ecm_classifier_process_response aci_pr;

			memset(&aci_pr, 0, sizeof(aci_pr));

			/*
			 * Get the source/group IP address for this multicast tuple and match
			 * with the source and group IP received from the event handler. If
			 * there is not match found jumps to the next multicast tuple.
			 */
			ecm_db_multicast_tuple_instance_source_ip_get(tuple_instance, src_addr);
			ecm_db_multicast_tuple_instance_group_ip_get(tuple_instance, grp_addr);
			if (!(ECM_IP_ADDR_MATCH(src_addr, src_ip) && ECM_IP_ADDR_MATCH(grp_addr, dest_ip))) {
				DEBUG_TRACE("%px: Multicast tuple not matched, try next multicast tuple %d\n", tuple_instance, op);
				goto find_next_tuple;
			}

			/*
			 * Get the DB connection instance using the tuple_instance, ref to 'ci'
			 * has been already taken by ecm_db_multicast_connection_find_and_ref()
			 */
			ci = ecm_db_multicast_connection_get_from_tuple(tuple_instance);
			DEBUG_TRACE("%px: update the multicast flow: %px\n", ci, tuple_instance);

			/*
			 * Check if this multicast connection has a bridge
			 * device in multicast destination interface list.
			 */
			tuple_instance_flags = ecm_db_multicast_tuple_instance_flags_get(tuple_instance);
			if ((tuple_instance_flags & ECM_DB_MULTICAST_CONNECTION_BRIDGE_DEV_SET_FLAG)) {
				mc_flag |= ECM_DB_MULTICAST_CONNECTION_BRIDGE_DEV_SET_FLAG;
			}

			memset(&mc_update, 0, sizeof(mc_update));
			spin_lock_bh(&ecm_nss_ipv4_lock);

			/*
			 * Find out if any change in destination interfaces heirarchy
			 * list. We try to find out the interfaces that
			 * have joined new, and the existing interfaces in the list
			 * that have left seperately.
			 */
			if_update = ecm_interface_multicast_find_updates_to_iface_list(ci, &mc_update, mc_flag, false, to_dev_idx, max_to_dev, NULL);
			if (!if_update) {
				spin_unlock_bh(&ecm_nss_ipv4_lock);
				DEBUG_TRACE("%px: no update, check next multicast tuple: %px\n", ci, tuple_instance);
				goto find_next_tuple;
			}

			found_br_dev = ecm_interface_multicast_check_for_br_dev(to_dev_idx, max_to_dev);
			if (!found_br_dev) {
				ecm_db_multicast_tuple_instance_flags_clear(tuple_instance, ECM_DB_MULTICAST_CONNECTION_BRIDGE_DEV_SET_FLAG);
			}

			spin_unlock_bh(&ecm_nss_ipv4_lock);
			DEBUG_TRACE("%px: MFC update callback leave_cnt %d, join_cnt %d\n", ci, mc_update.if_leave_cnt, mc_update.if_join_cnt);

			feci = ecm_db_connection_front_end_get_and_ref(ci);

			/*
			 * Do we have any new interfaces that have joined?
			 */
			if (mc_update.if_join_cnt > 0) {
				to_list = (struct ecm_db_iface_instance *)kzalloc(ECM_DB_TO_MCAST_INTERFACES_SIZE, GFP_ATOMIC | __GFP_NOWARN);
				if (!to_list) {
					feci->deref(feci);
					goto find_next_tuple;
				}

				/*
				 * Initialize the heirarchy's indices for the 'to_list'
				 * which will hold the interface heirarchies for the new joinees
				 */
				for (i = 0; i < ECM_DB_MULTICAST_IF_MAX; i++) {
					to_list_first[i] = ECM_DB_IFACE_HEIRARCHY_MAX;
				}

				/*
				 * Create the interface heirarchy list for the new interfaces. We append this list later to
				 * the existing list of destination interfaces.
				 */
				port = (__be16)(ecm_db_connection_port_get(ci, ECM_DB_OBJ_DIR_FROM));
				layer4hdr[0] = htons(port);
				port = (__be16)(ecm_db_connection_port_get(ci, ECM_DB_OBJ_DIR_TO));
				layer4hdr[1] = htons(port);

				vif_cnt = ecm_interface_multicast_heirarchy_construct_routed(feci, to_list, NULL, src_ip, dest_ip, mc_update.if_join_cnt, mc_update.join_dev, to_list_first, true, layer4hdr, NULL);
				if (vif_cnt == 0) {
					DEBUG_WARN("Failed to obtain 'to_mcast_update' heirarchy list\n");
					feci->decelerate(feci);
					feci->deref(feci);
					kfree(to_list);
					goto find_next_tuple;
				}

				/*
				 * Append the interface heirarchy array of the new joinees to the existing destination list
				 */
				ecm_db_multicast_connection_to_interfaces_update(ci, to_list, to_list_first, mc_update.if_join_idx);

				/*
				 * De-ref the updated destination interface list
				 */
				for (i = 0; i < ECM_DB_MULTICAST_IF_MAX; i++) {
					if (mc_update.if_join_idx[i]) {
						to_list_single = ecm_db_multicast_if_heirarchy_get(to_list, i);
						ecm_db_multicast_copy_if_heirarchy(to_list_temp, to_list_single);
						ecm_db_connection_interfaces_deref(to_list_temp, to_list_first[i]);
					}
				}

				kfree(to_list);
			}

#ifdef ECM_CLASSIFIER_OVS_ENABLE
			/*
			 * Verify the 'to' interface list with OVS classifier.
			 */
			if (ecm_interface_multicast_check_for_ovs_br_dev(to_dev_idx, max_to_dev) &&
				ecm_db_multicast_ovs_verify_to_list(ci, &aci_pr)) {
				/*
				 * We defunct the flow when the OVS returns "DENY_ACCEL" for the port.
				 * This can happen when the first port has joined on an OVS bridge
				 * on a flow that already exists. In this case, OVS needs to see the
				 * packet to update the flow. So we defunct the existing rule.
				 */
				DEBUG_TRACE("%px: Verification of the ovs 'to_list' has failed. Hence, defunct the connection: %px\n", feci, feci->ci);
				ecm_db_connection_make_defunct(ci);
				feci->deref(feci);
				goto find_next_tuple;
			}
#endif
			/*
			 * Push the updates to NSS
			 */
			DEBUG_TRACE("%px: Update accel %px\n", ci, tuple_instance);
			if ((feci->accel_mode <= ECM_FRONT_END_ACCELERATION_MODE_FAIL_DENIED) ||
					(feci->accel_mode != ECM_FRONT_END_ACCELERATION_MODE_ACCEL)) {
				DEBUG_TRACE("%px: Ignoring wrong mode accel for conn: %px\n", feci, feci->ci);
				feci->deref(feci);
				goto find_next_tuple;
			}

			/*
			 * Update the new rules in FW. If returns error decelerate the connection
			 * and flush Multicast rule.
			 */
			ret = ecm_nss_multicast_ipv4_connection_update_accelerate(feci, &mc_update, &aci_pr);
			if (ret < 0) {
				feci->decelerate(feci);
				feci->deref(feci);
				goto find_next_tuple;
			}

			feci->deref(feci);

			/*
			 * Release the interfaces that may have left the connection
			 */
			ecm_db_multicast_connection_to_interfaces_leave(ci, &mc_update);

find_next_tuple:
			/*
			 * Move on to the next flow for the same source and group
			 */
			tuple_instance_next = ecm_db_multicast_connection_get_and_ref_next(tuple_instance);
			ecm_db_multicast_connection_deref(tuple_instance);
			tuple_instance = tuple_instance_next;
		}
		break;
	}
	case IPMR_MFC_EVENT_DELETE:
	{
		struct ecm_front_end_connection_instance *feci;

		/*
		 * MFC reports that the multicast connection has died.
		 * Find the connections belongs to the same source/group address,
		 * Decelerate connections and free the frontend instance
		 */
		while (tuple_instance) {
			/*
			 * Get the source/group IP address for this multicast tuple and match
			 * with the source and group IP received from the event handler. If
			 * there is not match found jumps to the next multicast tuple.
			 */
			ecm_db_multicast_tuple_instance_source_ip_get(tuple_instance, src_addr);
			ecm_db_multicast_tuple_instance_group_ip_get(tuple_instance, grp_addr);
			if (!(ECM_IP_ADDR_MATCH(src_addr, src_ip) && ECM_IP_ADDR_MATCH(grp_addr, dest_ip))) {
				DEBUG_TRACE("%px: Multicast tuple not matched, try next tuple %d\n", tuple_instance, op);
				tuple_instance_next = ecm_db_multicast_connection_get_and_ref_next(tuple_instance);
				ecm_db_multicast_connection_deref(tuple_instance);
				tuple_instance = tuple_instance_next;
				continue;
			}

			/*
			 * Get the DB connection instance using the tuple_instance, ref to 'ci'
			 * has been already taken by ecm_db_multicast_connection_find_and_ref()
			 */
			ci = ecm_db_multicast_connection_get_from_tuple(tuple_instance);
			DEBUG_TRACE("%px:%d delete the multicast flow: %px\n", ci, op, tuple_instance);

			/*
			 * Get the front end instance
			 */
			feci = ecm_db_connection_front_end_get_and_ref(ci);
			feci->decelerate(feci);
			feci->deref(feci);

			/*
			 * Move on to the next flow for the same source and group
			 */
			tuple_instance_next = ecm_db_multicast_connection_get_and_ref_next(tuple_instance);
			ecm_db_multicast_connection_deref(tuple_instance);
			tuple_instance = tuple_instance_next;
		}
		break;
	}
	default:
		DEBUG_TRACE("Wrong op, shouldn't be here\n");
		break;
	}

	return;
}

/*
 * ecm_nss_multicast_ipv4_debugfs_init()
 */
bool ecm_nss_multicast_ipv4_debugfs_init(struct dentry *dentry)
{
	struct dentry *multicast_dentry;

	multicast_dentry = debugfs_create_u32("multicast_accelerated_count", S_IRUGO, dentry,
						&ecm_nss_multicast_ipv4_accelerated_count);
	if (!multicast_dentry) {
		DEBUG_ERROR("Failed to create ecm nss ipv4 multicast_accelerated_count file in debugfs\n");
		return false;
	}

	return true;
}

/*
 * ecm_nss_multicast_ipv4_stop()
 */
void ecm_nss_multicast_ipv4_stop(int num)
{
	ecm_front_end_ipv4_mc_stopped = num;
}

/*
 * ecm_nss_multicast_ipv4_init()
 * 	Register the callbacks for MCS snooper and MFC update
 */
int ecm_nss_multicast_ipv4_init(struct dentry *dentry)
{
	if (!debugfs_create_u32("ecm_nss_multicast_ipv4_stop", S_IRUGO | S_IWUSR, dentry,
					(u32 *)&ecm_front_end_ipv4_mc_stopped)) {
		DEBUG_ERROR("Failed to create ecm front end ipv4 mc stop file in debugfs\n");
		return -1;
	}

	/*
	 * Register multicast update callback to MCS snooper
	 */
	mc_bridge_ipv4_update_callback_register(ecm_br_multicast_update_event_callback);

	/*
	 * Register multicast update callbacks to MFC
	 */
	ipmr_register_mfc_event_offload_callback(ecm_mfc_update_event_callback);
	return 0;
}

/*
 * ecm_nss_multicast_ipv4_exit()
 * 	De-register the callbacks for MCS snooper and MFC update
 */
void ecm_nss_multicast_ipv4_exit(void)
{
	/*
	 * De-register multicast update callbacks to
	 * MFC and MCS snooper
	 */
	ipmr_unregister_mfc_event_offload_callback();
	mc_bridge_ipv4_update_callback_deregister();
}
