/*
 **************************************************************************
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <net/route.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>	/* for put_user */
#include <net/ipv6.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_bridge.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_core.h>

/*
 * Debug output levels
 * 0 = OFF
 * 1 = ASSERTS / ERRORS
 * 2 = 1 + WARN
 * 3 = 2 + INFO
 * 4 = 3 + TRACE
 */
#define DEBUG_LEVEL ECM_CLASSIFIER_MSCS_DEBUG_LEVEL

#include "ecm_types.h"
#include "ecm_db_types.h"
#include "ecm_state.h"
#include "ecm_tracker.h"
#include "ecm_classifier.h"
#include "ecm_front_end_types.h"
#include "ecm_tracker_udp.h"
#include "ecm_tracker_tcp.h"
#include "ecm_db.h"
#include "ecm_classifier_mscs.h"
#include "exports/ecm_classifier_mscs_public.h"

/*
 * Magic numbers
 */
#define ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC 0x1234

/*
 * struct ecm_classifier_mscs_instance
 * 	State to allow tracking of MSCS QoS tag for a connection
 */
struct ecm_classifier_mscs_instance {
	struct ecm_classifier_instance base;			/* Base type */

	struct ecm_classifier_mscs_instance *next;		/* Next classifier state instance (for accouting and reporting purposes) */
	struct ecm_classifier_mscs_instance *prev;		/* Next classifier state instance (for accouting and reporting purposes) */

	uint32_t ci_serial;					/* RO: Serial of the connection */
	struct ecm_classifier_process_response process_response;/* Last process response computed */

	int refs;						/* Integer to trap we never go negative */
#if (DEBUG_LEVEL > 0)
	uint16_t magic;
#endif
};

/*
 * Operational control
 */
static int ecm_classifier_mscs_enabled = 0;			/* Operational behaviour */

/*
 * Management thread control
 */
static bool ecm_classifier_mscs_terminate_pending;	/* True when the user wants us to terminate */

/*
 * Debugfs dentry object.
 */
static struct dentry *ecm_classifier_mscs_dentry;

/*
 * Locking of the classifier structures
 */
static DEFINE_SPINLOCK(ecm_classifier_mscs_lock);			/* Protect SMP access. */

/*
 * List of our classifier instances
 */
static struct ecm_classifier_mscs_instance *ecm_classifier_mscs_instances;
								/* list of all active instances */
static int ecm_classifier_mscs_count;			/* Tracks number of instances allocated */

/*
 * Callback structure to support MSCS peer lookup in external module
 */
static struct ecm_classifier_mscs_callbacks ecm_mscs;

/*
 * ecm_classifier_mscs_ref()
 *	Ref
 */
static void ecm_classifier_mscs_ref(struct ecm_classifier_instance *ci)
{
	struct ecm_classifier_mscs_instance *cmscsi;
	cmscsi = (struct ecm_classifier_mscs_instance *)ci;

	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed\n", cmscsi);
	spin_lock_bh(&ecm_classifier_mscs_lock);
	cmscsi->refs++;
	DEBUG_TRACE("%px: cmscsi ref %d\n", cmscsi, cmscsi->refs);
	DEBUG_ASSERT(cmscsi->refs > 0, "%px: ref wrap\n", cmscsi);
	spin_unlock_bh(&ecm_classifier_mscs_lock);
}

/*
 * ecm_classifier_mscs_deref()
 *	Deref
 */
static int ecm_classifier_mscs_deref(struct ecm_classifier_instance *ci)
{
	struct ecm_classifier_mscs_instance *cmscsi;
	cmscsi = (struct ecm_classifier_mscs_instance *)ci;

	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed\n", cmscsi);
	spin_lock_bh(&ecm_classifier_mscs_lock);
	cmscsi->refs--;
	DEBUG_ASSERT(cmscsi->refs >= 0, "%px: refs wrapped\n", cmscsi);
	DEBUG_TRACE("%px: MSCS classifier deref %d\n", cmscsi, cmscsi->refs);
	if (cmscsi->refs) {
		int refs = cmscsi->refs;
		spin_unlock_bh(&ecm_classifier_mscs_lock);
		return refs;
	}

	/*
	 * Object to be destroyed
	 */
	ecm_classifier_mscs_count--;
	DEBUG_ASSERT(ecm_classifier_mscs_count >= 0, "%px: ecm_classifier_mscs_count wrap\n", cmscsi);

	/*
	 * UnLink the instance from our list
	 */
	if (cmscsi->next) {
		cmscsi->next->prev = cmscsi->prev;
	}
	if (cmscsi->prev) {
		cmscsi->prev->next = cmscsi->next;
	} else {
		DEBUG_ASSERT(ecm_classifier_mscs_instances == cmscsi, "%px: list bad %px\n", cmscsi, ecm_classifier_mscs_instances);
		ecm_classifier_mscs_instances = cmscsi->next;
	}
	cmscsi->next = NULL;
	cmscsi->prev = NULL;
	spin_unlock_bh(&ecm_classifier_mscs_lock);

	/*
	 * Final
	 */
	DEBUG_INFO("%px: Final MSCS classifier instance\n", cmscsi);
	kfree(cmscsi);

	return 0;
}

/*
 * ecm_classifier_mscs_process()
 *	Process new data for connection
 */
static void ecm_classifier_mscs_process(struct ecm_classifier_instance *aci, ecm_tracker_sender_type_t sender,
						struct ecm_tracker_ip_header *ip_hdr, struct sk_buff *skb,
						struct ecm_classifier_process_response *process_response)
{
	struct ecm_classifier_mscs_instance *cmscsi;
	ecm_classifier_relevence_t relevance;
	struct ecm_db_connection_instance *ci = NULL;
	struct ecm_front_end_connection_instance *feci;
	ecm_front_end_acceleration_mode_t accel_mode;
	int protocol;
	uint32_t became_relevant = 0;
	ecm_classifier_mscs_process_callback_t cb = NULL;
	ecm_classifier_mscs_result_t result = 0;
	uint8_t smac[ETH_ALEN];
	uint8_t dmac[ETH_ALEN];
#ifdef ECM_MULTICAST_ENABLE
	ip_addr_t dst_ip;
#endif

	cmscsi = (struct ecm_classifier_mscs_instance *)aci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed\n", cmscsi);

	/*
	 * Are we yet to decide if this instance is relevant to the connection?
	 */
	spin_lock_bh(&ecm_classifier_mscs_lock);
	relevance = cmscsi->process_response.relevance;

	/*
	 * Are we relevant?
	 */
	if (relevance == ECM_CLASSIFIER_RELEVANCE_NO) {
		/*
		 * Lock still held
		 */
		goto mscs_classifier_out;
	}

	if (!ecm_classifier_mscs_enabled) {
		/*
		 * Lock still held
		 */
		cmscsi->process_response.relevance = ECM_CLASSIFIER_RELEVANCE_NO;
		goto mscs_classifier_out;
	}
	spin_unlock_bh(&ecm_classifier_mscs_lock);

	/*
	 * Can we accelerate?
	 */
	ci = ecm_db_connection_serial_find_and_ref(cmscsi->ci_serial);
	if (!ci) {
		DEBUG_TRACE("%px: No ci found for %u\n", cmscsi, cmscsi->ci_serial);
		spin_lock_bh(&ecm_classifier_mscs_lock);
		cmscsi->process_response.relevance = ECM_CLASSIFIER_RELEVANCE_NO;
		goto mscs_classifier_out;
	}

	feci = ecm_db_connection_front_end_get_and_ref(ci);
	accel_mode = feci->accel_state_get(feci);
	feci->deref(feci);
	protocol = ecm_db_connection_protocol_get(ci);

	/*
	 * MSCS classifier is not applicable for Multicast Traffic
	 */
#ifdef ECM_MULTICAST_ENABLE
	ecm_db_connection_address_get(ci, ECM_DB_OBJ_DIR_TO, dst_ip);
	if (ecm_ip_addr_is_multicast(dst_ip)) {
		DEBUG_TRACE("%px: Multicast Traffic, skip MSCS classification\n", ci);
		ecm_db_connection_deref(ci);
		spin_lock_bh(&ecm_classifier_mscs_lock);
		cmscsi->process_response.relevance = ECM_CLASSIFIER_RELEVANCE_NO;
		goto mscs_classifier_out;
	}
#endif

	/*
	 * Get the source mac address for this connection
	 */
	if (sender == ECM_TRACKER_SENDER_TYPE_SRC) {
		DEBUG_TRACE("%px: sender is SRC\n", aci);
		ecm_db_connection_node_address_get(ci, ECM_DB_OBJ_DIR_FROM, smac);
		ecm_db_connection_node_address_get(ci, ECM_DB_OBJ_DIR_TO, dmac);
	} else {
		DEBUG_TRACE("%px: sender is DEST\n", aci);
		ecm_db_connection_node_address_get(ci, ECM_DB_OBJ_DIR_TO, smac);
		ecm_db_connection_node_address_get(ci, ECM_DB_OBJ_DIR_FROM, dmac);
	}
	ecm_db_connection_deref(ci);

	spin_lock_bh(&ecm_classifier_mscs_lock);

	/*
	 * Get the WiFi datapath callback registered with MSCS client to check
	 * if MSCS QoS tag is valid for WiFi peer corresponding to
	 * skb->src_mac_addr
	 */
	cb = ecm_mscs.get_peer_priority;
	if (!cb) {
		cmscsi->process_response.relevance = ECM_CLASSIFIER_RELEVANCE_NO;
		goto mscs_classifier_out;
	}

	spin_unlock_bh(&ecm_classifier_mscs_lock);

	/*
	 * Invoke callback registered to classifier for peer look up
	 */
	result = cb(smac, dmac, skb);

	/*
	 * check the result of callback
	 */
	if (result == ECM_CLASSIFIER_MSCS_RESULT_DENY_PRIORITY) {
		spin_lock_bh(&ecm_classifier_mscs_lock);
		cmscsi->process_response.accel_mode = ECM_CLASSIFIER_ACCELERATION_MODE_NO;
		goto mscs_classifier_out;
	}

	/*
	 * We are relevant to the connection
	 */
	became_relevant = ecm_db_time_get();
	DEBUG_TRACE("MSCS Flow Priority: Flow priority: %d, Return priority: %d sender: %d\n",
			skb->priority, skb->priority, sender);

	spin_lock_bh(&ecm_classifier_mscs_lock);
	cmscsi->process_response.relevance = ECM_CLASSIFIER_RELEVANCE_YES;
	cmscsi->process_response.became_relevant = became_relevant;

	cmscsi->process_response.process_actions = ECM_CLASSIFIER_PROCESS_ACTION_QOS_TAG;

	if (result == ECM_CLASSIFIER_MSCS_RESULT_UPDATE_PRIORITY) {
		cmscsi->process_response.flow_qos_tag = skb->priority;
		cmscsi->process_response.return_qos_tag = skb->priority;
	} else if (result == ECM_CLASSIFIER_MSCS_RESULT_UPDATE_INVALID_TAG) {
		cmscsi->process_response.flow_qos_tag = ECM_CLASSIFIER_MSCS_INVALID_QOS_TAG;
		cmscsi->process_response.return_qos_tag = ECM_CLASSIFIER_MSCS_INVALID_QOS_TAG;
	}

mscs_classifier_out:

	/*
	 * Return our process response
	 */
	*process_response = cmscsi->process_response;
	spin_unlock_bh(&ecm_classifier_mscs_lock);
}

/*
 * ecm_classifier_mscs_sync_to_v4()
 *	Front end is pushing accel engine state to us
 */
static void ecm_classifier_mscs_sync_to_v4(struct ecm_classifier_instance *aci, struct ecm_classifier_rule_sync *sync)
{
	struct ecm_classifier_mscs_instance *cmscsi __attribute__((unused));

	cmscsi = (struct ecm_classifier_mscs_instance *)aci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed", cmscsi);
}

/*
 * ecm_classifier_mscs_sync_from_v4()
 *	Front end is retrieving accel engine state from us
 */
static void ecm_classifier_mscs_sync_from_v4(struct ecm_classifier_instance *aci, struct ecm_classifier_rule_create *ecrc)
{
	struct ecm_classifier_mscs_instance *cmscsi __attribute__((unused));

	cmscsi = (struct ecm_classifier_mscs_instance *)aci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed", cmscsi);
}

/*
 * ecm_classifier_mscs_sync_to_v6()
 *	Front end is pushing accel engine state to us
 */
static void ecm_classifier_mscs_sync_to_v6(struct ecm_classifier_instance *aci, struct ecm_classifier_rule_sync *sync)
{
	struct ecm_classifier_mscs_instance *cmscsi __attribute__((unused));

	cmscsi = (struct ecm_classifier_mscs_instance *)aci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed", cmscsi);
}

/*
 * ecm_classifier_mscs_sync_from_v6()
 *	Front end is retrieving accel engine state from us
 */
static void ecm_classifier_mscs_sync_from_v6(struct ecm_classifier_instance *aci, struct ecm_classifier_rule_create *ecrc)
{
	struct ecm_classifier_mscs_instance *cmscsi __attribute__((unused));

	cmscsi = (struct ecm_classifier_mscs_instance *)aci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed", cmscsi);
}

/*
 * ecm_classifier_mscs_type_get()
 *	Get type of classifier this is
 */
static ecm_classifier_type_t ecm_classifier_mscs_type_get(struct ecm_classifier_instance *ci)
{
	struct ecm_classifier_mscs_instance *cmscsi;
	cmscsi = (struct ecm_classifier_mscs_instance *)ci;

	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed\n", cmscsi);
	return ECM_CLASSIFIER_TYPE_MSCS;
}

/*
 * ecm_classifier_mscs_last_process_response_get()
 *	Get result code returned by the last process call
 */
static void ecm_classifier_mscs_last_process_response_get(struct ecm_classifier_instance *ci,
							struct ecm_classifier_process_response *process_response)
{
	struct ecm_classifier_mscs_instance *cmscsi;

	cmscsi = (struct ecm_classifier_mscs_instance *)ci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed\n", cmscsi);

	spin_lock_bh(&ecm_classifier_mscs_lock);
	*process_response = cmscsi->process_response;
	spin_unlock_bh(&ecm_classifier_mscs_lock);
}

/*
 * ecm_classifier_mscs_reclassify_allowed()
 *	Indicate if reclassify is allowed
 */
static bool ecm_classifier_mscs_reclassify_allowed(struct ecm_classifier_instance *ci)
{
	struct ecm_classifier_mscs_instance *cmscsi;
	cmscsi = (struct ecm_classifier_mscs_instance *)ci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed\n", cmscsi);

	return false;
}

/*
 * ecm_classifier_mscs_reclassify()
 *	Reclassify
 */
static void ecm_classifier_mscs_reclassify(struct ecm_classifier_instance *ci)
{
	struct ecm_classifier_mscs_instance *cmscsi;
	cmscsi = (struct ecm_classifier_mscs_instance *)ci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed\n", cmscsi);

}

#ifdef ECM_STATE_OUTPUT_ENABLE
/*
 * ecm_classifier_mscs_state_get()
 *	Return state
 */
static int ecm_classifier_mscs_state_get(struct ecm_classifier_instance *ci, struct ecm_state_file_instance *sfi)
{
	int result;
	struct ecm_classifier_mscs_instance *cmscsi;
	struct ecm_classifier_process_response process_response;

	cmscsi = (struct ecm_classifier_mscs_instance *)ci;
	DEBUG_CHECK_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC, "%px: magic failed", cmscsi);

	if ((result = ecm_state_prefix_add(sfi, "mscs"))) {
		return result;
	}

	spin_lock_bh(&ecm_classifier_mscs_lock);
	process_response = cmscsi->process_response;
	spin_unlock_bh(&ecm_classifier_mscs_lock);

	/*
	 * Output our last process response
	 */
	if ((result = ecm_classifier_process_response_state_get(sfi, &process_response))) {
		return result;
	}

	return ecm_state_prefix_remove(sfi);
}
#endif

/*
 * ecm_classifier_mscs_instance_alloc()
 *	Allocate an instance of the mscs classifier
 */
struct ecm_classifier_mscs_instance *ecm_classifier_mscs_instance_alloc(struct ecm_db_connection_instance *ci)
{
	struct ecm_classifier_mscs_instance *cmscsi;

	/*
	 * Allocate the instance
	 */
	cmscsi = (struct ecm_classifier_mscs_instance *)kzalloc(sizeof(struct ecm_classifier_mscs_instance), GFP_ATOMIC | __GFP_NOWARN);
	if (!cmscsi) {
		DEBUG_WARN("Failed to allocate mscs instance\n");
		return NULL;
	}

	DEBUG_SET_MAGIC(cmscsi, ECM_CLASSIFIER_MSCS_INSTANCE_MAGIC);
	cmscsi->refs = 1;
	cmscsi->base.process = ecm_classifier_mscs_process;
	cmscsi->base.sync_from_v4 = ecm_classifier_mscs_sync_from_v4;
	cmscsi->base.sync_to_v4 = ecm_classifier_mscs_sync_to_v4;
	cmscsi->base.sync_from_v6 = ecm_classifier_mscs_sync_from_v6;
	cmscsi->base.sync_to_v6 = ecm_classifier_mscs_sync_to_v6;
	cmscsi->base.type_get = ecm_classifier_mscs_type_get;
	cmscsi->base.last_process_response_get = ecm_classifier_mscs_last_process_response_get;
	cmscsi->base.reclassify_allowed = ecm_classifier_mscs_reclassify_allowed;
	cmscsi->base.reclassify = ecm_classifier_mscs_reclassify;
#ifdef ECM_STATE_OUTPUT_ENABLE
	cmscsi->base.state_get = ecm_classifier_mscs_state_get;
#endif
	cmscsi->base.ref = ecm_classifier_mscs_ref;
	cmscsi->base.deref = ecm_classifier_mscs_deref;
	cmscsi->ci_serial = ecm_db_connection_serial_get(ci);
	cmscsi->process_response.process_actions = 0;
	cmscsi->process_response.relevance = ECM_CLASSIFIER_RELEVANCE_MAYBE;

	spin_lock_bh(&ecm_classifier_mscs_lock);

	/*
	 * Final check if we are pending termination
	 */
	if (ecm_classifier_mscs_terminate_pending) {
		spin_unlock_bh(&ecm_classifier_mscs_lock);
		DEBUG_INFO("%px: Terminating\n", ci);
		kfree(cmscsi);
		return NULL;
	}

	/*
	 * Link the new instance into our list at the head
	 */
	cmscsi->next = ecm_classifier_mscs_instances;
	if (ecm_classifier_mscs_instances) {
		ecm_classifier_mscs_instances->prev = cmscsi;
	}
	ecm_classifier_mscs_instances = cmscsi;

	/*
	 * Increment stats
	 */
	ecm_classifier_mscs_count++;
	DEBUG_ASSERT(ecm_classifier_mscs_count > 0, "%px: ecm_classifier_mscs_count wrap\n", cmscsi);
	spin_unlock_bh(&ecm_classifier_mscs_lock);

	DEBUG_INFO("mscs instance alloc: %px\n", cmscsi);
	return cmscsi;
}
EXPORT_SYMBOL(ecm_classifier_mscs_instance_alloc);

/*
 * ecm_classifier_mscs_rul_get_enabled()
 */
static int ecm_classifier_mscs_rule_get_enabled(void *data, u64 *val)
{
	*val = ecm_classifier_mscs_enabled;

	return 0;
}

/*
 * ecm_classifier_mscs_rule_set_enabled()
 */
static int ecm_classifier_mscs_rule_set_enabled(void *data, u64 val)
{
	DEBUG_TRACE("ecm_classifier_mscs_enabled = %u\n", (uint32_t)val);

	if ((val != 0) && (val != 1)) {
		DEBUG_WARN("Invalid value: %u. Valid values are 0 and 1.\n", (uint32_t)val);
		return -EINVAL;
	}

	ecm_classifier_mscs_enabled = (uint32_t)val;

	return 0;
}

/*
 * Debugfs attribute for Emesh Enabled parameter.
 */
DEFINE_SIMPLE_ATTRIBUTE(ecm_classifier_mscs_enabled_fops, ecm_classifier_mscs_rule_get_enabled, ecm_classifier_mscs_rule_set_enabled, "%llu\n");

/*
 * ecm_interface_ovpn_register
 */
int ecm_classifier_mscs_callback_register(struct ecm_classifier_mscs_callbacks *mscs_cb)
{
	spin_lock_bh(&ecm_classifier_mscs_lock);
	if (ecm_mscs.get_peer_priority) {
		spin_unlock_bh(&ecm_classifier_mscs_lock);
		DEBUG_ERROR("MSCS callbacks are registered\n");
		return -1;
	}

	ecm_mscs.get_peer_priority = mscs_cb->get_peer_priority;
	spin_unlock_bh(&ecm_classifier_mscs_lock);

	return 0;
}
EXPORT_SYMBOL(ecm_classifier_mscs_callback_register);

/*
 * ecm_interface_ovpn_unregister
 */
void ecm_classifier_mscs_callback_unregister (void)
{
	spin_lock_bh(&ecm_classifier_mscs_lock);
	ecm_mscs.get_peer_priority = NULL;
	spin_unlock_bh(&ecm_classifier_mscs_lock);
}
EXPORT_SYMBOL(ecm_classifier_mscs_callback_unregister);

/*
 * ecm_classifier_mscs_init()
 */
int ecm_classifier_mscs_init(struct dentry *dentry)
{
	DEBUG_INFO("mscs classifier Module init\n");

	ecm_classifier_mscs_dentry = debugfs_create_dir("ecm_classifier_mscs", dentry);
	if (!ecm_classifier_mscs_dentry) {
		DEBUG_ERROR("Failed to create ecm mscs directory in debugfs\n");
		return -1;
	}

	if (!debugfs_create_file("enabled", S_IRUGO | S_IWUSR, ecm_classifier_mscs_dentry,
				NULL, &ecm_classifier_mscs_enabled_fops)) {
		DEBUG_ERROR("Failed to create ecm nl classifier enabled file in debugfs\n");
		debugfs_remove_recursive(ecm_classifier_mscs_dentry);
		return -1;
	}

	return 0;
}
EXPORT_SYMBOL(ecm_classifier_mscs_init);

/*
 * ecm_classifier_mscs_exit()
 */
void ecm_classifier_mscs_exit(void)
{
	DEBUG_INFO("mscs classifier Module exit\n");

	spin_lock_bh(&ecm_classifier_mscs_lock);
	ecm_classifier_mscs_terminate_pending = true;
	spin_unlock_bh(&ecm_classifier_mscs_lock);

	/*
	 * Remove the debugfs files recursively.
	 */
	if (ecm_classifier_mscs_dentry) {
		debugfs_remove_recursive(ecm_classifier_mscs_dentry);
	}
}
EXPORT_SYMBOL(ecm_classifier_mscs_exit);
