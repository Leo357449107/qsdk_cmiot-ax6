/*
 **************************************************************************
 * Copyright (c) 2015-2016, 2019-2021, The Linux Foundation.  All rights reserved.
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

#include <linux/if_pppox.h>
#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_acct.h>

/*
 * Flag to limit the number of DB connections at any point to the maximum number
 * that can be accelerated by NSS. This may need to be enabled for low memory
 * platforms to control memory allocated by ECM databases.
 */
extern unsigned int ecm_front_end_conn_limit;

#ifdef ECM_FRONT_END_NSS_ENABLE
#include "ecm_nss_bond_notifier.h"
#else
static inline void ecm_nss_bond_notifier_stop(int num)
{
	/*
	 * Just return if nss front end is not enabled
	 */
	return;
}

static inline int ecm_nss_bond_notifier_init(struct dentry *dentry)
{
	/*
	 * Just return if nss front end is not enabled
	 */
	return 0;
}

static inline void ecm_nss_bond_notifier_exit(void)
{
	/*
	 * Just return if nss front end is not enabled
	 */
	return;
}
#endif

/*
 * ecm_front_end_l2_encap_header_len()
 *      Return length of encapsulating L2 header
 */
static inline uint32_t ecm_front_end_l2_encap_header_len(uint16_t protocol)
{
	switch (protocol) {
	case ETH_P_PPP_SES:
		return PPPOE_SES_HLEN;
	default:
		return 0;
	}
}

/*
 * ecm_front_end_pull_l2_encap_header()
 *      Pull encapsulating L2 header
 */
static inline void ecm_front_end_pull_l2_encap_header(struct sk_buff *skb, uint32_t len)
{
	skb->data += len;
	skb->network_header += len;
}

/*
 * ecm_front_end_push_l2_encap_header()
 *      Push encapsulating L2 header
 */
static inline void ecm_front_end_push_l2_encap_header(struct sk_buff *skb, uint32_t len)
{
	skb->data -= len;
	skb->network_header -= len;
}

/*
 * ecm_front_end_acceleration_rejected()
 *      Check if the acceleration of a flow could be rejected quickly.
 */
static inline bool ecm_front_end_acceleration_rejected(struct sk_buff *skb)
{
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	struct nf_conn_counter *acct;

	ct = nf_ct_get(skb, &ctinfo);
	if (unlikely(!ct)) {
		/*
		 * Maybe bridged traffic, no decision here.
		 */
		return false;
	}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 12, 0))
	if (unlikely(nf_ct_is_untracked(ct))) {
#else
	if (unlikely(ctinfo == IP_CT_UNTRACKED)) {
#endif
		/*
		 * Untracked traffic certainly can't be accelerated.
		 */
		return true;
	}

	acct = nf_conn_acct_find(ct)->counter;
	if (acct) {
		long long packets = atomic64_read(&acct[CTINFO2DIR(ctinfo)].packets);
		if ((packets > 0xff) && (packets & 0xff)) {
			/*
			 * Connection hits slow path at least 256 times, so it must be not able to accelerate.
			 * But we also give it a chance to walk through ECM every 256 packets
			 */
			return true;
		}
	}

	return false;
}

/*
 * ecm_front_end_flow_and_return_directions_get()
 *	Gets the flow and return flows directions respect to conntrack entry.
 */
static inline void ecm_front_end_flow_and_return_directions_get(struct nf_conn *ct, ip_addr_t flow_ip, int ip_version, int *flow_dir, int *return_dir)
{
	ip_addr_t ct_src_ip;

	if (ip_version == 4) {
		uint32_t flow_ip_32;
		uint32_t ct_src_ip_32;
		ECM_NIN4_ADDR_TO_IP_ADDR(ct_src_ip, ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.ip);

		/*
		 * Print the IP addresses for debug purpose.
		 */
		ECM_IP_ADDR_TO_HIN4_ADDR(flow_ip_32, flow_ip);
		ECM_IP_ADDR_TO_HIN4_ADDR(ct_src_ip_32, ct_src_ip);
		DEBUG_TRACE("flow_ip: %pI4h ct_src_ip: %pI4h\n", &flow_ip_32, &ct_src_ip_32);
	} else if (ip_version == 6) {
		ECM_NIN6_ADDR_TO_IP_ADDR(ct_src_ip, ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u3.in6);

		/*
		 * Print the IP addresses for debug purpose.
		 */
		DEBUG_TRACE("flow_ip: " ECM_IP_ADDR_OCTAL_FMT " ct_src_ip: " ECM_IP_ADDR_OCTAL_FMT "\n",
				ECM_IP_ADDR_TO_OCTAL(flow_ip), ECM_IP_ADDR_TO_OCTAL(ct_src_ip));
	} else {
		DEBUG_ASSERT(NULL, "Invalid ip version");
	}

	if (ECM_IP_ADDR_MATCH(ct_src_ip, flow_ip)) {
		*flow_dir = IP_CT_DIR_ORIGINAL;
		*return_dir = IP_CT_DIR_REPLY;
		DEBUG_TRACE("flow_ip and ct_src_ip match\n");
	} else {
		*flow_dir = IP_CT_DIR_REPLY;
		*return_dir = IP_CT_DIR_ORIGINAL;
		DEBUG_TRACE("flow_ip and ct_src_ip do not match\n");
	}

	DEBUG_TRACE("flow_dir: %d return_dir: %d\n", *flow_dir, *return_dir);
}

/*
 * ecm_front_end_common_connection_defunct_check()
 *	Checks if the connection can be defuncted.
 * The return value indicates that the caller is allowed to send a defunct message.
 */
static inline bool ecm_front_end_common_connection_defunct_check(struct ecm_front_end_connection_instance *feci)
{
	DEBUG_ASSERT(spin_is_locked(&feci->lock), "%px: feci lock is not held\n", feci);

	/*
	 * If we have not completed the destroy failure handling, do nothing.
	 */
	if (feci->destroy_fail_handle_pending) {
		return false;
	}

	/*
	 * If connection has already become defunct, do nothing.
	 */
	if (feci->is_defunct) {
		return false;
	}
	feci->is_defunct = true;

	/*
	 * If the connection is already in one of the fail modes, do nothing, keep the current accel_mode.
	 */
	if (ECM_FRONT_END_ACCELERATION_FAILED(feci->accel_mode)) {
		return false;
	}

	/*
	 * If the connection is decel then ensure it will not attempt accel while defunct.
	 */
	if (feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_DECEL) {
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DEFUNCT;
		return false;
	 }

	/*
	 * If the connection is decel pending then decel operation is in progress anyway.
	 */
	if (feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_DECEL_PENDING) {
		return false;
	}

	return true;
}

/*
 * ecm_front_end_common_connection_decelerate_accel_mode_check()
 *	Checks the accel mode of the connection to see it is ok for deceleration.
 * The return value indicates that the caller is allowed to send a decelerate message.
 */
static inline bool ecm_front_end_common_connection_decelerate_accel_mode_check(struct ecm_front_end_connection_instance *feci)
{
	DEBUG_ASSERT(spin_is_locked(&feci->lock), "%px: feci lock is not held\n", feci);

	/*
	 * If decelerate is in error or already pending then ignore
	 */
	if (feci->stats.decelerate_pending) {
		return false;
	}

	/*
	 * If acceleration is pending then we cannot decelerate right now or we will race with it
	 * Set a decelerate pending flag that will be actioned when the acceleration command is complete.
	 */
	if (feci->accel_mode == ECM_FRONT_END_ACCELERATION_MODE_ACCEL_PENDING) {
		feci->stats.decelerate_pending = true;
		return false;
	}

	/*
	 * Can only decelerate if accelerated
	 * NOTE: This will also deny accel when the connection is in fail condition too.
	 */
	if (feci->accel_mode != ECM_FRONT_END_ACCELERATION_MODE_ACCEL) {
		return false;
	}

	return true;
}

/*
 * ecm_front_end_destroy_failure_handle()
 *	Destroy request failure handler.
 */
static inline bool ecm_front_end_destroy_failure_handle(struct ecm_front_end_connection_instance *feci)
{

	spin_lock_bh(&feci->lock);
	feci->stats.driver_fail_total++;
	feci->stats.driver_fail++;
	if (feci->stats.driver_fail >= feci->stats.driver_fail_limit) {
		/*
		 * Reached to the driver failure limit. ECM no longer allows
		 * re-trying deceleration.
		 */
		feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_FAIL_DRIVER;
		spin_unlock_bh(&feci->lock);
		DEBUG_WARN("%px: Decel failed - driver fail limit\n", feci);
		return true;
	}

	/*
	 * Destroy request failed. The accelerated connection couldn't be destroyed
	 * in the acceleration engine. Revert back the accel_mode, unset the is_defunct
	 * flag just in case this request has come through the defunct process.
	 */
	feci->accel_mode = ECM_FRONT_END_ACCELERATION_MODE_ACCEL;
	feci->is_defunct = false;
	feci->destroy_fail_handle_pending = true;
	spin_unlock_bh(&feci->lock);

	/*
	 * Set the defunct timer to a smaller timeout value so that the connection will be
	 * tried to be defuncted again, when the timeout expires (its value is 5 seconds).
	 */
	ecm_db_connection_defunct_timer_remove_and_set(feci->ci, ECM_DB_TIMER_GROUPS_CONNECTION_DEFUNCT_RETRY_TIMEOUT);

	spin_lock_bh(&feci->lock);
	feci->destroy_fail_handle_pending = false;
	spin_unlock_bh(&feci->lock);

	return false;
}

extern void ecm_front_end_bond_notifier_stop(int num);
extern int ecm_front_end_bond_notifier_init(struct dentry *dentry);
extern void ecm_front_end_bond_notifier_exit(void);

#ifdef ECM_STATE_OUTPUT_ENABLE
extern int ecm_front_end_common_connection_state_get(struct ecm_front_end_connection_instance *feci,
						    struct ecm_state_file_instance *sfi,
						    char *conn_type);
#endif
extern bool ecm_front_end_gre_proto_is_accel_allowed(struct net_device *indev,
						      struct net_device *outdev,
						      struct sk_buff *skb,
						      struct nf_conntrack_tuple *tuple,
						      int ip_version);
extern uint64_t ecm_front_end_get_slow_packet_count(struct ecm_front_end_connection_instance *feci);
#ifdef ECM_CLASSIFIER_DSCP_ENABLE
void ecm_front_end_tcp_set_dscp_ext(struct nf_conn *ct,
					      struct ecm_tracker_ip_header *iph,
					      struct sk_buff *skb,
					      ecm_tracker_sender_type_t sender);
#endif
void ecm_front_end_fill_ovs_params(struct ecm_front_end_ovs_params ovs_params[],
					ip_addr_t ip_src_addr, ip_addr_t ip_src_addr_nat,
					ip_addr_t ip_dest_addr, ip_addr_t ip_dest_addr_nat,
					int src_port, int src_port_nat,
					int dest_port, int dest_port_nat, ecm_db_direction_t ecm_dir);
void ecm_front_end_common_sysctl_register(void);
void ecm_front_end_common_sysctl_unregister(void);
