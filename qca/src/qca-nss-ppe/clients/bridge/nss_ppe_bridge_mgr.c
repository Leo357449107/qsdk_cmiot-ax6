/*
 **************************************************************************
 * Copyright (c) 2016-2021, The Linux Foundation. All rights reserved.
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

/*
 * nss_ppe_bridge_mgr.c
 *	NSS PPE Bridge Interface manager
 */
#include <linux/sysctl.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/if_bridge.h>
#include <linux/netdevice.h>
#include <net/bonding.h>
#include <ref/ref_vsi.h>
#include <nss_ppe_vlan_mgr.h>
#include <fal/fal_fdb.h>
#include <fal/fal_stp.h>
#include <fal/fal_acl.h>
#include <fal/fal_api.h>
#include <fal/fal_port_ctrl.h>
#include <nss_dp_api_if.h>
#include "nss_ppe_bridge_mgr.h"

#if defined(NSS_BRIDGE_MGR_OVS_ENABLE)
#include <ovsmgr.h>
#endif

/*
 * Module parameter to enable/disable OVS bridge.
 */
static bool ovs_enabled = false;

static struct nss_ppe_bridge_mgr_context br_mgr_ctx;

/*
 * nss_ppe_bridge_mgr_delete_instance()
 *	Delete a bridge instance from bridge list and free the bridge instance.
 */
static void nss_ppe_bridge_mgr_delete_instance(struct nss_ppe_bridge_mgr_pvt *br)
{
	spin_lock(&br_mgr_ctx.lock);
	if (!list_empty(&br->list)) {
		list_del(&br->list);
	}

	spin_unlock(&br_mgr_ctx.lock);

	kfree(br);
}

/*
 * nss_ppe_bridge_mgr_create_instance()
 *	Create a private bridge instance.
 */
static struct nss_ppe_bridge_mgr_pvt *nss_ppe_bridge_mgr_create_instance(struct net_device *dev)
{
	struct nss_ppe_bridge_mgr_pvt *br;

	/*
	 * When OVS is enabled, we have to check for both bridge master
	 * and OVS master.
	 */
	if (!netif_is_bridge_master(dev)) {
#if !defined(NSS_BRIDGE_MGR_OVS_ENABLE)
		return NULL;
#else
		if (!ovsmgr_is_ovs_master(dev)) {
			return NULL;
		}
#endif
	}

	br = kzalloc(sizeof(*br), GFP_KERNEL);
	if (!br) {
		nss_ppe_bridge_mgr_warn("%px: failed to allocate nss_ppe_bridge_mgr_pvt instance\n", dev);
		return NULL;
	}

	INIT_LIST_HEAD(&br->list);
	return br;
}

/*
 * nss_ppe_bridge_mgr_ppe_update_port_vsi()
 *	Update VSI information for the given port number.
 *
 * Note: update fal API with a PPE API when generated.
 */
static sw_error_t nss_ppe_bridge_mgr_ppe_update_port_vsi(struct net_device *dev, fal_port_t port_num, uint32_t vsi)
{
	return fal_port_vsi_set(0, port_num, vsi);
}

/*
 * nss_ppe_bridge_mgr_ppe_update_mac_addr()
 *	Update the mac address of the bridge.
 *
 * Note: use PPE API when generated.
 */
static bool nss_ppe_bridge_mgr_ppe_update_mac_addr(struct net_device *dev, uint8_t *addr)
{
	return true;
}

/*
 * nss_ppe_bridge_mgr_ppe_update_mtu()
 *	Update the MTU of the bridge.
 *
 * Note: use PPE API when generated.
 */
static bool nss_ppe_bridge_mgr_ppe_update_mtu(struct net_device *dev, uint16_t mtu)
{
	return true;
}

/*
 * nss_ppe_bridge_mgr_ppe_unregister_br()
 *	Unregisters the bridge from PPE.
 *
 * TODO: Replace these with PPE APIs when they are generated.
 */
static void nss_ppe_bridge_mgr_ppe_unregister_br(struct net_device *dev, uint32_t vsi_id)
{
	ppe_vsi_free(NSS_PPE_BRIDGE_MGR_SWITCH_ID, vsi_id);

	/*
	 * It may happen that the same VSI is allocated again,
	 * so there is a need to flush bridge FDB table.
	 */
	if (fal_fdb_entry_del_byfid(NSS_PPE_BRIDGE_MGR_SWITCH_ID, vsi_id, FAL_FDB_DEL_STATIC)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to flush FDB table for vsi:%d in PPE\n", dev, vsi_id);
	}
}


/*
 * nss_ppe_bridge_mgr_disable_fdb_learning()
 *	Disable fdb learning in PPE
 *
 * For the first time a bond interface join bridge, we need to use flow based rule.
 * FDB learing/station move need to be disabled.
 */
static int nss_ppe_bridge_mgr_disable_fdb_learning(struct nss_ppe_bridge_mgr_pvt *br)
{
	fal_vsi_newaddr_lrn_t newaddr_lrn;
	fal_vsi_stamove_t sta_move;

	/*
	 * Disable station move
	 */
	sta_move.stamove_en = 0;
	sta_move.action = FAL_MAC_FRWRD;
	if (fal_vsi_stamove_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, &sta_move)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to disable station move for Bridge vsi\n", br);
		return -1;
	}

	/*
	 * Disable FDB learning in PPE
	 */
	newaddr_lrn.lrn_en = 0;
	newaddr_lrn.action = FAL_MAC_FRWRD;
	if (fal_vsi_newaddr_lrn_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, &newaddr_lrn)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to disable FDB learning for Bridge vsi\n", br);
		goto enable_sta_move;
	}

	/*
	 * Flush FDB table for the bridge vsi
	 */
	if (fal_fdb_entry_del_byfid(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, FAL_FDB_DEL_STATIC)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to flush FDB table for vsi:%d in PPE\n", br, br->vsi);
		goto enable_fdb_learning;
	}

	return 0;

enable_fdb_learning:
	newaddr_lrn.lrn_en = 1;
	newaddr_lrn.action = FAL_MAC_FRWRD;
	if (fal_vsi_newaddr_lrn_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, &newaddr_lrn)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to enable FDB learning for Bridge vsi\n", br);
	}

enable_sta_move:
	sta_move.stamove_en = 1;
	sta_move.action = FAL_MAC_FRWRD;
	if (fal_vsi_stamove_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, &sta_move)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to enable station move for Bridge vsi\n", br);
	}

	return -1;
}

/*
 * nss_ppe_bridge_mgr_enable_fdb_learning()
 *	Enable fdb learning in PPE.
 */
static int nss_ppe_bridge_mgr_enable_fdb_learning(struct nss_ppe_bridge_mgr_pvt *br)
{
	fal_vsi_newaddr_lrn_t newaddr_lrn;
	fal_vsi_stamove_t sta_move;

	/*
	 * Enable station move
	 */
	sta_move.stamove_en = 1;
	sta_move.action = FAL_MAC_FRWRD;
	if (fal_vsi_stamove_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, &sta_move)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to enable station move for Bridge vsi\n", br);
		return -1;
	}

	/*
	 * Enable FDB learning in PPE
	 */
	newaddr_lrn.lrn_en = 1;
	newaddr_lrn.action = FAL_MAC_FRWRD;
	if (fal_vsi_newaddr_lrn_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, &newaddr_lrn)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to enable FDB learning for Bridge vsi\n", br);
		goto disable_sta_move;
	}

	return 0;

disable_sta_move:
	sta_move.stamove_en = 0;
	sta_move.action = FAL_MAC_FRWRD;
	if (fal_vsi_stamove_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, br->vsi, &sta_move))
		nss_ppe_bridge_mgr_warn("%px: Failed to disable station move for Bridge vsi\n", br);

	return -1;
}

/*
 * nss_ppe_bridge_mgr_ppe_register_br()
 *	Registers the bridge to PPE.
 *
 * TODO: Replace these with PPE APIs when they are generated.
 */
static bool nss_ppe_bridge_mgr_ppe_register_br(struct nss_ppe_bridge_mgr_pvt *b_pvt, struct net_device *dev,
							uint8_t *dev_addr, uint16_t mtu, uint32_t *vsi_id_ptr)
{
	int err;
	err = ppe_vsi_alloc(NSS_PPE_BRIDGE_MGR_SWITCH_ID, vsi_id_ptr);
	if (err) {
		nss_ppe_bridge_mgr_warn("%px: failed to alloc bridge vsi, error = %d\n", dev, err);
		goto fail;
	}

	if (!nss_ppe_bridge_mgr_ppe_update_mac_addr(dev, dev_addr)) {
		nss_ppe_bridge_mgr_warn("%px: failed to set mac_addr msg\n", dev);
		goto fail2;
	}

	if (!nss_ppe_bridge_mgr_ppe_update_mtu(dev, mtu)) {
		nss_ppe_bridge_mgr_warn("%px: failed to set mtu msg\n", dev);
		goto fail2;
	}

	/*
	 * Disable FDB learning if OVS is enabled for
	 * all bridges (including Linux bridge).
	 */
	if (ovs_enabled) {
		nss_ppe_bridge_mgr_disable_fdb_learning(b_pvt);
	}

	return true;

fail2:
	ppe_vsi_free(NSS_PPE_BRIDGE_MGR_SWITCH_ID, *vsi_id_ptr);
fail:
	return false;
}

/*
 * nss_ppe_bridge_mgr_ppe_leave_br()
 *	Leave net_device from the bridge.
 *
 * Note: use PPE API when generated.
 */
static int nss_ppe_bridge_mgr_ppe_leave_br(struct net_device *dev, uint32_t port, bool is_wan, uint32_t port_vsi, uint32_t br_vsi)
{
	fal_port_t port_num = (fal_port_t)port;

	if (fal_stp_port_state_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, NSS_PPE_BRIDGE_MGR_SPANNING_TREE_ID, port_num, FAL_STP_FORWARDING)) {
		nss_ppe_bridge_mgr_warn("%px: failed to set the STP state to forwarding\n", dev);
		return -EPERM;
	}

	/*
	 * If the configuration WAN port is added to bridge,
	 * The configuration will be done separely. No need to do any change here.
	 */
	if (is_wan) {
		return 1;
	}

	if (ppe_port_vsi_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, port_num, port_vsi)) {
		nss_ppe_bridge_mgr_warn("%px: failed to restore port VSI of physical interface\n", dev);
		fal_stp_port_state_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, NSS_PPE_BRIDGE_MGR_SPANNING_TREE_ID, port_num, FAL_STP_DISABLED);
		return -EPERM;
	}

	if (nss_ppe_bridge_mgr_ppe_update_port_vsi(dev, port_num, port_vsi)) {
		nss_ppe_bridge_mgr_warn("%px: failed to update port VSI of physical interface\n", dev);
		fal_stp_port_state_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, NSS_PPE_BRIDGE_MGR_SPANNING_TREE_ID, port_num, FAL_STP_DISABLED);
		ppe_port_vsi_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, port_num, br_vsi);
		return -EIO;
	}

	return 0;
}

/*
 * nss_ppe_bridge_mgr_ppe_join_br()
 *	Joins net_device from the bridge.
 *
 * Note: use PPE API when generated.
 */
static int nss_ppe_bridge_mgr_ppe_join_br(struct net_device *dev, uint32_t port, uint32_t br_vsi, uint32_t *port_vsi_ptr)
{
	fal_port_t port_num = (fal_port_t)port;

	if (ppe_port_vsi_get(NSS_PPE_BRIDGE_MGR_SWITCH_ID, port_num, port_vsi_ptr)) {
		nss_ppe_bridge_mgr_warn("%px: failed to save port VSI of physical interface\n", dev);
		return -EIO;
	}

	if (ppe_port_vsi_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, port_num, br_vsi)) {
		nss_ppe_bridge_mgr_warn("%px: failed to set bridge VSI for physical interface\n", dev);
		return -EIO;
	}

	if (nss_ppe_bridge_mgr_ppe_update_port_vsi(dev, port_num, br_vsi)) {
		nss_ppe_bridge_mgr_warn("%px: failed to update port VSI of physical interface\n", dev);
		ppe_port_vsi_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, port_num, *port_vsi_ptr);
		return -EIO;
	}

	return 0;
}

/*
 * nss_ppe_bridge_mgr_del_bond_slave()
 *	A slave interface being removed from a bond master that belongs to a bridge.
 */
static int nss_ppe_bridge_mgr_del_bond_slave(struct net_device *bond_master,
		struct net_device *slave, struct nss_ppe_bridge_mgr_pvt *b_pvt)
{
	int32_t port_id;
	uint32_t port_number;
	int res;

	/*
	 * TODO: Investigate how to get LAG ID from bond_id when lag support is added.
	 */
	nss_ppe_bridge_mgr_trace("%px: Bond Slave %s leaving bridge\n",
			b_pvt, slave->name);

	/*
	 * Hardware supports only PHYSICAL Ports as trunk ports
	 */
	if (!nss_dp_is_netdev_physical(slave)) {
		nss_ppe_bridge_mgr_warn("%px: Interface %s is not Physical Interface\n",
						b_pvt, slave->name);
		return -1;
	}

	port_number = nss_dp_get_port_num(slave);
	if (port_number == NSS_DP_INVALID_INTERFACE) {
		nss_ppe_bridge_mgr_warn("%px: Invalid port number\n", b_pvt);
		return -1;
	}
	port_id = (fal_port_t)port_number;

	/*
	 * Take bridge lock as we are updating vsi and port forwarding
	 * details in PPE Hardware
	 * TODO: Investihate whether this lock is needed or not.
	 */
	spin_lock(&br_mgr_ctx.lock);
	res = nss_ppe_bridge_mgr_ppe_leave_br(slave, port_id, false, b_pvt->port_vsi[port_id - 1], b_pvt->vsi);
	if (res) {
		nss_ppe_bridge_mgr_warn("%px: Unable to leave bridge %d\n", b_pvt, port_id);
		return -1;
	}
	spin_unlock(&br_mgr_ctx.lock);

	/*
	 * Set STP state to forwarding after bond physical port leaves bridge
	 */
	fal_stp_port_state_set(NSS_PPE_BRIDGE_MGR_SWITCH_ID, NSS_PPE_BRIDGE_MGR_SPANNING_TREE_ID,
					port_id, FAL_STP_FORWARDING);
	return 0;
}

/*
 * nss_ppe_bridge_mgr_add_bond_slave()
 *	A slave interface being added to a bond master that belongs to a bridge.
 */
static int nss_ppe_bridge_mgr_add_bond_slave(struct net_device *bond_master,
		struct net_device *slave, struct nss_ppe_bridge_mgr_pvt *b_pvt)
{
	uint32_t port_number;
	uint32_t port_vsi;
	int32_t port_id;
	int res;

	nss_ppe_bridge_mgr_trace("%px: Bond Slave %s is added bridge\n",
			b_pvt, slave->name);

	if (!nss_dp_is_netdev_physical(slave)) {
		nss_ppe_bridge_mgr_warn("%px: Interface %s is not Physical Interface\n",
					b_pvt, slave->name);
		return -1;
	}

	port_number = nss_dp_get_port_num(slave);
	if (port_number == NSS_DP_INVALID_INTERFACE) {
		nss_ppe_bridge_mgr_warn("%px: Invalid port number\n", b_pvt);
		return -1;
	}

	nss_ppe_bridge_mgr_trace("%px: Interface %s adding into bridge\n",
			b_pvt, slave->name);
	port_id = port_number;

	/*
	 * Take bridge lock as we are updating vsi and port forwarding
	 * details in PPE Hardware
	 * TODO: Investigate whether this lock is needed or not.
	 */
	spin_lock(&br_mgr_ctx.lock);
	res = nss_ppe_bridge_mgr_ppe_join_br(slave, port_id, b_pvt->vsi, &port_vsi);
	if (res) {
		nss_ppe_bridge_mgr_warn("%px: Unable to join bridge %d\n", b_pvt, port_id);
		return -1;
	}
	b_pvt->port_vsi[port_id - 1] = port_vsi;
	spin_unlock(&br_mgr_ctx.lock);
	return 0;
}

/*
 * nss_ppe_bridge_mgr_bond_master_join()
 *	Add a bond interface to bridge
 */
static int nss_ppe_bridge_mgr_bond_master_join(struct net_device *bond_master,
		struct nss_ppe_bridge_mgr_pvt *b_pvt)
{
	struct net_device *slave;
	struct list_head *iter;
	struct bonding *bond;

	nss_ppe_bridge_mgr_assert(netif_is_bond_master(bond_master));
	bond = netdev_priv(bond_master);

	ASSERT_RTNL();

	/*
	 * Join each of the bonded slaves to the VSI group
	 */
	bond_for_each_slave(bond, slave, iter) {
		if (nss_ppe_bridge_mgr_add_bond_slave(bond_master, slave, b_pvt)) {
			nss_ppe_bridge_mgr_warn("%px: Failed to add slave (%s) state in Bridge\n", b_pvt, slave->name);
			goto cleanup;
		}
	}

	/*
	 * If already other bond devices are attached to bridge,
	 * only increment bond_slave_num,
	 */
	spin_lock(&br_mgr_ctx.lock);
	if (b_pvt->bond_slave_num) {
		b_pvt->bond_slave_num++;
		spin_unlock(&br_mgr_ctx.lock);
		return NOTIFY_DONE;
	}
	spin_unlock(&br_mgr_ctx.lock);

	/*
	 * This is the first bond device being attached to bridge. In order to enforce Linux
	 * bond slave selection in bridge flows involving bond interfaces, we need to disable
	 * fdb learning on this bridge master to allow flow based bridging.
	 */
	if (!nss_ppe_bridge_mgr_disable_fdb_learning(b_pvt)) {
		spin_lock(&br_mgr_ctx.lock);
		b_pvt->bond_slave_num = 1;
		spin_unlock(&br_mgr_ctx.lock);

		return NOTIFY_DONE;
	}

cleanup:

	bond_for_each_slave(bond, slave, iter) {
		if (nss_ppe_bridge_mgr_del_bond_slave(bond_master, slave, b_pvt)) {
			nss_ppe_bridge_mgr_warn("%px: Failed to remove slave (%s) from Bridge\n", b_pvt, slave->name);
		}
	}

	return NOTIFY_BAD;
}

/*
 * nss_ppe_bridge_mgr_bond_slave_changeupper()
 *	Add bond slave to bridge VSI
 */
static int nss_ppe_bridge_mgr_bond_slave_changeupper(struct netdev_notifier_changeupper_info *cu_info,
		struct net_device *bond_slave)
{
	struct net_device *master;
	struct nss_ppe_bridge_mgr_pvt *b_pvt;

	/*
	 * Checking if our bond master is part of a bridge
	 */
	master = netdev_master_upper_dev_get(cu_info->upper_dev);
	if (!master) {
		return NOTIFY_DONE;
	}

	b_pvt = nss_ppe_bridge_mgr_find_instance(master);
	if (!b_pvt) {
		nss_ppe_bridge_mgr_warn("The bond master is not part of Bridge dev:%s\n", master->name);
		return NOTIFY_DONE;
	}

	/*
	 * Add or remove the slave based based on linking event
	 */
	if (cu_info->linking) {
		if (nss_ppe_bridge_mgr_add_bond_slave(cu_info->upper_dev, bond_slave, b_pvt)) {
			nss_ppe_bridge_mgr_warn("%px: Failed to add slave (%s) state in Bridge %s\n", b_pvt,
					cu_info->upper_dev->name, master->name);
		}
	} else {
		if (nss_ppe_bridge_mgr_del_bond_slave(cu_info->upper_dev, bond_slave, b_pvt)) {
			nss_ppe_bridge_mgr_warn("%px: Failed to remove slave (%s) state in Bridge %s\n", b_pvt,
					cu_info->upper_dev->name, master->name);
		}
	}

	return NOTIFY_DONE;
}

/*
 * nss_ppe_bridge_mgr_bond_master_leave()
 *	Remove a bond interface from bridge
 */
static int nss_ppe_bridge_mgr_bond_master_leave(struct net_device *bond_master,
		struct nss_ppe_bridge_mgr_pvt *b_pvt)
{
	struct net_device *slave;
	struct list_head *iter;
	struct bonding *bond;

	ASSERT_RTNL();

	nss_ppe_bridge_mgr_assert(netif_is_bond_master(bond_master));
	bond = netdev_priv(bond_master);

	nss_ppe_bridge_mgr_assert(b_pvt->bond_slave_num == 0);

	/*
	 * Remove each of the bonded slaves from the VSI group
	 */
	bond_for_each_slave(bond, slave, iter) {
		if (nss_ppe_bridge_mgr_del_bond_slave(bond_master, slave, b_pvt)) {
			nss_ppe_bridge_mgr_warn("%px: Failed to remove slave (%s) from Bridge\n", b_pvt, slave->name);
			goto cleanup;
		}
	}

	/*
	 * If more than one bond devices are attached to bridge,
	 * only decrement the bond_slave_num
	 */
	spin_lock(&br_mgr_ctx.lock);
	if (b_pvt->bond_slave_num > 1) {
		b_pvt->bond_slave_num--;
		spin_unlock(&br_mgr_ctx.lock);
		return NOTIFY_DONE;
	}
	spin_unlock(&br_mgr_ctx.lock);

	/*
	 * The last bond interface is removed from bridge, we can switch back to FDB
	 * learning mode.
	 */
	if (!ovs_enabled && !nss_ppe_bridge_mgr_enable_fdb_learning(b_pvt)) {
		spin_lock(&br_mgr_ctx.lock);
		b_pvt->bond_slave_num = 0;
		spin_unlock(&br_mgr_ctx.lock);
	}

	return NOTIFY_DONE;

cleanup:
	bond_for_each_slave(bond, slave, iter) {
		if (nss_ppe_bridge_mgr_add_bond_slave(bond_master, slave, b_pvt)) {
			nss_ppe_bridge_mgr_warn("%px: Failed to add slave (%s) state in Bridge\n", b_pvt, slave->name);
		}
	}

	return NOTIFY_BAD;
}

/*
 * nss_ppe_bridge_mgr_changemtu_event()
 *     Change bridge MTU.
 */
static int nss_ppe_bridge_mgr_changemtu_event(struct netdev_notifier_info *info)
{
	struct net_device *dev = netdev_notifier_info_to_dev(info);
	struct nss_ppe_bridge_mgr_pvt *b_pvt = nss_ppe_bridge_mgr_find_instance(dev);

	if (!b_pvt) {
		return NOTIFY_DONE;
	}

	spin_lock(&br_mgr_ctx.lock);
	if (b_pvt->mtu == dev->mtu) {
		spin_unlock(&br_mgr_ctx.lock);
		return NOTIFY_DONE;
	}
	spin_unlock(&br_mgr_ctx.lock);

	nss_ppe_bridge_mgr_trace("%px: MTU changed to %d, \n", b_pvt, dev->mtu);
	if (!nss_ppe_bridge_mgr_ppe_update_mtu(dev, dev->mtu)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to update MTU\n", b_pvt);
		return NOTIFY_DONE;
	}

	spin_lock(&br_mgr_ctx.lock);
	b_pvt->mtu = dev->mtu;
	spin_unlock(&br_mgr_ctx.lock);
	return NOTIFY_DONE;
}

/*
 * nss_ppe_bridge_mgr_changeaddr_event()
 *	Change bridge MAC address.
 */
static int nss_ppe_bridge_mgr_changeaddr_event(struct netdev_notifier_info *info)
{
	struct net_device *dev = netdev_notifier_info_to_dev(info);
	struct nss_ppe_bridge_mgr_pvt *b_pvt = nss_ppe_bridge_mgr_find_instance(dev);

	if (!b_pvt) {
		return NOTIFY_DONE;
	}

	spin_lock(&br_mgr_ctx.lock);
	if (!memcmp(b_pvt->dev_addr, dev->dev_addr, ETH_ALEN)) {
		spin_unlock(&br_mgr_ctx.lock);
		nss_ppe_bridge_mgr_trace("%px: MAC are the same..skip processing it\n", b_pvt);
		return NOTIFY_DONE;
	}

	spin_unlock(&br_mgr_ctx.lock);

	nss_ppe_bridge_mgr_trace("%px: MAC changed to %pM, update PPE\n", b_pvt, dev->dev_addr);
	if (!nss_ppe_bridge_mgr_ppe_update_mac_addr(dev, dev->dev_addr)) {
		nss_ppe_bridge_mgr_warn("%px: Failed to update MAC address\n", b_pvt);
		return NOTIFY_DONE;
	}

	spin_lock(&br_mgr_ctx.lock);
	ether_addr_copy(b_pvt->dev_addr, dev->dev_addr);
	spin_unlock(&br_mgr_ctx.lock);
	return NOTIFY_DONE;
}

/*
 * nss_ppe_bridge_mgr_changeupper_event()
 *	Bridge manager handles netdevice joining or leaving bridge notification.
 */
static int nss_ppe_bridge_mgr_changeupper_event(struct netdev_notifier_info *info)
{
	struct net_device *dev = netdev_notifier_info_to_dev(info);
	struct net_device *master_dev;
	struct netdev_notifier_changeupper_info *cu_info;
	struct nss_ppe_bridge_mgr_pvt *b_pvt;

	cu_info = (struct netdev_notifier_changeupper_info *)info;

	/*
	 * Check if the master pointer is valid
	 */
	if (!cu_info->master) {
		return NOTIFY_DONE;
	}

	/*
	 * The master is a bond that we don't need to process, but the bond might be part of a bridge.
	 */
	if (netif_is_bond_slave(dev)) {
		return nss_ppe_bridge_mgr_bond_slave_changeupper(cu_info, dev);
	}

	master_dev = cu_info->upper_dev;

	/*
	 * Check if upper_dev is a known bridge.
	 */
	b_pvt = nss_ppe_bridge_mgr_find_instance(master_dev);
	if (!b_pvt)
		return NOTIFY_DONE;

	/*
	 * Slave device is bond master and it is added/removed to/from bridge
	 */
	if (netif_is_bond_master(dev)) {
		if (cu_info->linking)
			return nss_ppe_bridge_mgr_bond_master_join(dev, b_pvt);
		else
			return nss_ppe_bridge_mgr_bond_master_leave(dev, b_pvt);
	}

	if (cu_info->linking) {
		nss_ppe_bridge_mgr_trace("%px: Interface %s joining bridge %s\n", b_pvt, dev->name, master_dev->name);
		if (nss_ppe_bridge_mgr_join_bridge(dev, b_pvt)) {
			nss_ppe_bridge_mgr_warn("%px: Interface %s failed to join bridge %s\n", b_pvt, dev->name, master_dev->name);
		}

		return NOTIFY_DONE;
	}

	nss_ppe_bridge_mgr_trace("%px: Interface %s leaving bridge %s\n", b_pvt, dev->name, master_dev->name);
	if (nss_ppe_bridge_mgr_leave_bridge(dev, b_pvt)) {
		nss_ppe_bridge_mgr_warn("%px: Interface %s failed to leave bridge %s\n", b_pvt, dev->name, master_dev->name);
	}

	return NOTIFY_DONE;
}

/*
 * nss_ppe_bridge_mgr_register_event()
 *	Bridge manager handles bridge registration notification.
 */
static int nss_ppe_bridge_mgr_register_event(struct netdev_notifier_info *info)
{
	nss_ppe_bridge_mgr_register_br(netdev_notifier_info_to_dev(info));
	return NOTIFY_DONE;
}

/*
 * nss_ppe_bridge_mgr_unregister_event()
 *	Bridge manager handles bridge unregistration notification.
 */
static int nss_ppe_bridge_mgr_unregister_event(struct netdev_notifier_info *info)
{
	nss_ppe_bridge_mgr_unregister_br(netdev_notifier_info_to_dev(info));
	return NOTIFY_DONE;
}

/*
 * nss_ppe_bridge_mgr_netdevice_event()
 *	Bridge manager handles bridge operation notifications.
 */
static int nss_ppe_bridge_mgr_netdevice_event(struct notifier_block *unused,
				unsigned long event, void *ptr)
{
	struct netdev_notifier_info *info = (struct netdev_notifier_info *)ptr;

	switch (event) {
	case NETDEV_CHANGEADDR:
		return nss_ppe_bridge_mgr_changeaddr_event(info);
	case NETDEV_CHANGEMTU:
		return nss_ppe_bridge_mgr_changemtu_event(info);
	case NETDEV_CHANGEUPPER:
		return nss_ppe_bridge_mgr_changeupper_event(info);
	case NETDEV_REGISTER:
		return nss_ppe_bridge_mgr_register_event(info);
	case NETDEV_UNREGISTER:
		return nss_ppe_bridge_mgr_unregister_event(info);
	}

	/*
	 * Notify done for all the events we don't care
	 */
	return NOTIFY_DONE;
}

static struct notifier_block nss_ppe_bridge_mgr_netdevice_nb __read_mostly = {
	.notifier_call = nss_ppe_bridge_mgr_netdevice_event,
};

/*
 * nss_ppe_bridge_mgr_is_physical_dev()
 *	Check if the device is on physical device.
 */
static bool nss_ppe_bridge_mgr_is_physical_dev(struct net_device *dev)
{
	struct net_device *root_dev = dev;
	if (!dev) {
		return false;
	}

	/*
	 * Check if it is VLAN first because VLAN can be over bond interface.
	 * However, the bond over VLAN is not supported in our driver.
	 */
	if (is_vlan_dev(dev)) {
		root_dev = nss_ppe_vlan_mgr_get_real_dev(dev);
		if (!root_dev) {
			goto error;
		}

		if (is_vlan_dev(root_dev)) {
			root_dev = nss_ppe_vlan_mgr_get_real_dev(root_dev);
			if (!root_dev) {
				goto error;
			}
		}
	}

	/*
	 * Don't consider bond interface because FDB learning is disabled.
	 */
	if (netif_is_bond_master(root_dev)) {
		return false;
	}

	if (!nss_dp_is_netdev_physical(root_dev)) {
		nss_ppe_bridge_mgr_warn("%px: interface %s is not physical interface\n",
				root_dev, root_dev->name);
		return false;
	}

	return true;

error:
	nss_ppe_bridge_mgr_warn("%px: cannot find the real device for VLAN %s\n", dev, dev->name);
	return false;
}

/*
 * nss_ppe_bridge_mgr_fdb_update_callback()
 *	Get invoked when there is a FDB update.
 */
static int nss_ppe_bridge_mgr_fdb_update_callback(struct notifier_block *notifier,
					      unsigned long val, void *ctx)
{
	struct br_fdb_event *event = (struct br_fdb_event *)ctx;
	struct nss_ppe_bridge_mgr_pvt *b_pvt = NULL;
	struct net_device *br_dev = NULL;
	fal_fdb_entry_t entry;

	if (!event->br)
		return NOTIFY_DONE;

	br_dev = br_fdb_bridge_dev_get_and_hold(event->br);
	if (!br_dev) {
		nss_ppe_bridge_mgr_warn("%px: bridge device not found\n", event->br);
		return NOTIFY_DONE;
	}

	nss_ppe_bridge_mgr_trace("%px: MAC: %pM, original source: %s, new source: %s, bridge: %s\n",
			event, event->addr, event->orig_dev->name, event->dev->name, br_dev->name);

	/*
	 * When a MAC address move from a physical interface to a non-physical
	 * interface, the FDB entry in the PPE needs to be flushed.
	 */
	if (!nss_ppe_bridge_mgr_is_physical_dev(event->orig_dev)) {
		nss_ppe_bridge_mgr_trace("%px: original source is not a physical interface\n", event->orig_dev);
		dev_put(br_dev);
		return NOTIFY_DONE;
	}

	if (nss_ppe_bridge_mgr_is_physical_dev(event->dev)) {
		nss_ppe_bridge_mgr_trace("%px: new source is not a non-physical interface\n", event->dev);
		dev_put(br_dev);
		return NOTIFY_DONE;
	}

	b_pvt = nss_ppe_bridge_mgr_find_instance(br_dev);
	dev_put(br_dev);
	if (!b_pvt) {
		nss_ppe_bridge_mgr_warn("%px: bridge instance not found\n", event->br);
		return NOTIFY_DONE;
	}

	memset(&entry, 0, sizeof(entry));
	memcpy(&entry.addr, event->addr, ETH_ALEN);
	entry.fid = b_pvt->vsi;
	if (SW_OK != fal_fdb_entry_del_bymac(NSS_PPE_BRIDGE_MGR_SWITCH_ID, &entry)) {
		nss_ppe_bridge_mgr_warn("%px: FDB entry delete failed with MAC %pM and fid %d\n",
				    b_pvt, &entry.addr, entry.fid);
	}

	return NOTIFY_OK;
}

/*
 * Notifier block for FDB update
 */
static struct notifier_block nss_ppe_bridge_mgr_fdb_update_notifier = {
	.notifier_call = nss_ppe_bridge_mgr_fdb_update_callback,
};

/*
 * nss_ppe_bridge_mgr_wan_inf_add_handler
 *	Marks an interface as a WAN interface for special handling by bridge.
 */
static int nss_ppe_bridge_mgr_wan_intf_add_handler(struct ctl_table *table,
						int write, void __user *buffer,
						size_t *lenp, loff_t *ppos)
{
	struct net_device *dev;
	char *dev_name;
	char *if_name;
	int ret;

	/*
	 * Find the string, return an error if not found
	 */
	ret = proc_dostring(table, write, buffer, lenp, ppos);
	if (ret || !write) {
		return ret;
	}

	if_name = br_mgr_ctx.wan_ifname;
	dev_name = strsep(&if_name, " ");
	dev = dev_get_by_name(&init_net, dev_name);
	if (!dev) {
		nss_ppe_bridge_mgr_warn("Cannot find the net device associated with %s\n", dev_name);
		return -ENODEV;
	}

	if (!nss_dp_is_netdev_physical(dev)) {
		dev_put(dev);
		nss_ppe_bridge_mgr_warn("Only physical interfaces can be marked as WAN interface: %s\n", dev_name);
		return -ENOMSG;
	}

	if (br_mgr_ctx.wan_netdev) {
		dev_put(dev);
		nss_ppe_bridge_mgr_warn("Cannot overwrite a pre-existing wan interface\n");
		return -ENOMSG;
	}

	br_mgr_ctx.wan_netdev = dev;
	dev_put(dev);
	nss_ppe_bridge_mgr_always("For adding netdev: %s as WAN interface, do a network restart\n", dev_name);
	return ret;
}

/*
 * nss_ppe_bridge_mgr_wan_inf_del_handler
 *	Un-marks an interface as a WAN interface.
 */
static int nss_ppe_bridge_mgr_wan_intf_del_handler(struct ctl_table *table,
						int write, void __user *buffer,
						size_t *lenp, loff_t *ppos)
{
	struct net_device *dev;
	char *dev_name;
	char *if_name;
	int ret;

	ret = proc_dostring(table, write, buffer, lenp, ppos);
	if (ret)
		return ret;

	if (!write)
		return ret;

	if_name = br_mgr_ctx.wan_ifname;
	dev_name = strsep(&if_name, " ");
	dev = dev_get_by_name(&init_net, dev_name);
	if (!dev) {
		nss_ppe_bridge_mgr_warn("Cannot find the net device associated with %s\n", dev_name);
		return -ENODEV;
	}

	if (!nss_dp_is_netdev_physical(dev)) {
		dev_put(dev);
		nss_ppe_bridge_mgr_warn("Only physical interfaces can be marked/unmarked, if_num: %s\n", dev_name);
		return -ENOMSG;
	}

	if (br_mgr_ctx.wan_netdev != dev) {
		dev_put(dev);
		nss_ppe_bridge_mgr_warn("This interface is not marked as a WAN interface\n");
		return -ENOMSG;
	}

	br_mgr_ctx.wan_netdev = NULL;
	dev_put(dev);
	nss_ppe_bridge_mgr_always("For deleting netdev: %s as WAN interface, do a network restart\n", dev_name);
	return ret;
}

static struct ctl_table nss_ppe_bridge_mgr_table[] = {
	{
		.procname	= "add_wanif",
		.data           = &br_mgr_ctx.wan_ifname,
		.maxlen         = sizeof(char) * IFNAMSIZ,
		.mode           = 0644,
		.proc_handler   = &nss_ppe_bridge_mgr_wan_intf_add_handler,
	},
	{
		.procname	= "del_wanif",
		.data           = &br_mgr_ctx.wan_ifname,
		.maxlen         = sizeof(char) * IFNAMSIZ,
		.mode           = 0644,
		.proc_handler   = &nss_ppe_bridge_mgr_wan_intf_del_handler,
	},
	{ }
};

static struct ctl_table nss_ppe_bridge_mgr_dir[] = {
	{
		.procname	= "bridge_mgr",
		.mode		= 0555,
		.child		= nss_ppe_bridge_mgr_table,
	},
	{ }
};

static struct ctl_table nss_ppe_bridge_mgr_root_dir[] = {
	{
		.procname	= "nss",
		.mode		= 0555,
		.child		= nss_ppe_bridge_mgr_dir,
	},
	{ }
};

/*
 * nss_ppe_bridge_mgr_find_instance()
 *	Find a bridge instance from bridge list.
 */
struct nss_ppe_bridge_mgr_pvt *nss_ppe_bridge_mgr_find_instance(struct net_device *dev)
{
	struct nss_ppe_bridge_mgr_pvt *br;

#if !defined(NSS_BRIDGE_MGR_OVS_ENABLE)
	if (!netif_is_bridge_master(dev))
		return NULL;
#else
	/*
	 * When OVS is enabled, we have to check for both bridge master
	 * and OVS master.
	 */
	if (!netif_is_bridge_master(dev) && !ovsmgr_is_ovs_master(dev))
		return NULL;
#endif

	/*
	 * Do we have it on record?
	 */
	spin_lock(&br_mgr_ctx.lock);
	list_for_each_entry(br, &br_mgr_ctx.list, list) {
		if (br->dev == dev) {
			spin_unlock(&br_mgr_ctx.lock);
			return br;
		}
	}

	spin_unlock(&br_mgr_ctx.lock);
	return NULL;
}

/*
 * nss_ppe_bridge_mgr_leave_bridge()
 *	Netdevice leave bridge.
 */
int nss_ppe_bridge_mgr_leave_bridge(struct net_device *dev, struct nss_ppe_bridge_mgr_pvt *br)
{
	uint32_t port;
	int res;
	bool is_wan = false;

	if (nss_dp_is_netdev_physical(dev)) {
		port = nss_dp_get_port_num(dev);
		if (port == NSS_DP_INVALID_INTERFACE) {
			nss_ppe_bridge_mgr_warn("%px: Invalid port number\n", br);
			return -EPERM;
		}

		/*
		 * If there is a wan interface added in bridge, a separate
		 * VSI is created for it.
		 */
		if ((br->wan_if_enabled) && (br->wan_netdev == dev)) {
			is_wan = true;
		}


		res = nss_ppe_bridge_mgr_ppe_leave_br(dev, port, is_wan, br->port_vsi[port - 1], br->vsi);
		if (res < 0) {
			nss_ppe_bridge_mgr_warn("%px: failed to leave bridge\n", br);
			return res;
		} else if (res == 1) {
			br->wan_if_enabled = false;
			br->wan_netdev = NULL;
			nss_ppe_bridge_mgr_info("Netdev %px (%s) is added as WAN interface \n", dev, dev->name);
			return 0;
		}
	} else if (is_vlan_dev(dev)) {
		struct net_device *real_dev;

		/*
		 * Find real_dev associated with the VLAN.
		 */
		real_dev = nss_ppe_vlan_mgr_get_real_dev(dev);
		if (real_dev && is_vlan_dev(real_dev)) {
			real_dev = nss_ppe_vlan_mgr_get_real_dev(real_dev);
		}

		if (real_dev == NULL) {
			nss_ppe_bridge_mgr_warn("%px: real dev for the vlan: %s in NULL\n", br, dev->name);
			return -1;
		}

		/*
		 * This is a valid vlan dev, remove the vlan dev from bridge.
		 */
		if (nss_ppe_vlan_mgr_leave_bridge(dev, br->vsi)) {
			nss_ppe_bridge_mgr_warn("%px: vlan device failed to leave bridge\n", br);
			return -1;
		}

		/*
		 * dev is a bond with VLAN and VLAN is removed from bridge
		 */
		if (netif_is_bond_master(real_dev)) {

			/*
			 * Remove the bond_master from bridge.
			 */
			if (nss_ppe_bridge_mgr_bond_master_leave(real_dev, br) != NOTIFY_DONE) {
				nss_ppe_bridge_mgr_warn("%px: Slaves of bond interface %s leave bridge failed\n", br, real_dev->name);
				nss_ppe_vlan_mgr_join_bridge(dev, br->vsi);
				return -1;
			}

			return 0;
		}
	}

	return 0;
}

/*
 * nss_ppe_bridge_mgr_join_bridge()
 *	Netdevice join bridge.
 */
int nss_ppe_bridge_mgr_join_bridge(struct net_device *dev, struct nss_ppe_bridge_mgr_pvt *br)
{
	uint32_t port;
	uint32_t port_vsi;
	int res;

	if (nss_dp_is_netdev_physical(dev)) {
		port = nss_dp_get_port_num(dev);
		if (port == NSS_DP_INVALID_INTERFACE) {
			nss_ppe_bridge_mgr_warn("%px: Invalid port number\n", br);
			return -1;
		}

		/*
		 * If there is a wan interface added in bridge, create a
		 * separate VSI for it, hence avoiding FDB based forwarding.
		 * This is done by not setting fal_port
		 */
		if (br_mgr_ctx.wan_netdev == dev) {
			br->wan_if_enabled = true;
			br->wan_netdev = dev;
			nss_ppe_bridge_mgr_info("Netdev %px (%s) is added as WAN interface \n", dev, dev->name);
			return 0;
		}

		res = nss_ppe_bridge_mgr_ppe_join_br(dev, port, br->vsi, &port_vsi);
		if (res < 0) {
			nss_ppe_bridge_mgr_warn("%px: failed to join bridge\n", br);
			return res;
		}
		br->port_vsi[port - 1] = port_vsi;

	} else if (is_vlan_dev(dev)) {
		struct net_device *real_dev;

		/*
		 * Find real_dev associated with the VLAN
		 */
		real_dev = nss_ppe_vlan_mgr_get_real_dev(dev);
		if (real_dev && is_vlan_dev(real_dev))
			real_dev = nss_ppe_vlan_mgr_get_real_dev(real_dev);
		if (real_dev == NULL) {
			nss_ppe_bridge_mgr_warn("%px: real dev for the vlan: %s in NULL\n", br, dev->name);
			return -EINVAL;
		}

		/*
		 * This is a valid vlan dev, add the vlan dev to bridge
		 */
		if (nss_ppe_vlan_mgr_join_bridge(dev, br->vsi)) {
			nss_ppe_bridge_mgr_warn("%px: vlan device failed to join bridge\n", br);
			return -ENODEV;
		}

		/*
		 * dev is a bond with VLAN and VLAN is added to bridge
		 */
		if (netif_is_bond_master(real_dev)) {

			/*
			 * Add the bond_master to bridge.
			 */
			if (nss_ppe_bridge_mgr_bond_master_join(real_dev, br) != NOTIFY_DONE) {
				nss_ppe_bridge_mgr_warn("%px: Slaves of bond interface %s join bridge failed\n", br, real_dev->name);
				nss_ppe_vlan_mgr_leave_bridge(dev, br->vsi);
				return -EINVAL;
			}

			return 0;
		}
	}

	return 0;
}

/*
 * nss_ppe_bridge_mgr_unregister_br()
 *	Unregister bridge device, dev, from bridge manager database.
 */
int nss_ppe_bridge_mgr_unregister_br(struct net_device *dev)
{
	struct nss_ppe_bridge_mgr_pvt *b_pvt;

	/*
	 * Do we have it on record?
	 */
	b_pvt = nss_ppe_bridge_mgr_find_instance(dev);
	if (!b_pvt) {
		return -1;
	}

	nss_ppe_bridge_mgr_ppe_unregister_br(dev, b_pvt->vsi);

	nss_ppe_bridge_mgr_trace("%px: Bridge %s unregistered. Freeing bridge\n", b_pvt, dev->name);
	nss_ppe_bridge_mgr_delete_instance(b_pvt);
	return 0;
}

/*
 * nss_ppe_bridge_mgr_register_br()
 *	Register new bridge instance in bridge manager database.
 */
int nss_ppe_bridge_mgr_register_br(struct net_device *dev)
{
	struct nss_ppe_bridge_mgr_pvt *b_pvt;
	uint32_t vsi_id = 0;

	nss_ppe_bridge_mgr_info("%px: Bridge register: %s\n", dev, dev->name);

	b_pvt = nss_ppe_bridge_mgr_create_instance(dev);
	if (!b_pvt) {
		return -EINVAL;
	}

	b_pvt->dev = dev;

	if (!nss_ppe_bridge_mgr_ppe_register_br(b_pvt, dev, dev->dev_addr, dev->mtu, &vsi_id)) {
		nss_ppe_bridge_mgr_warn("%px: PPE registeration failed for net_dev %s\n", b_pvt, dev->name);
		nss_ppe_bridge_mgr_delete_instance(b_pvt);
		return -EFAULT;
	}

	/*
	 * All done, take a snapshot of the current mtu and mac addrees
	 */
	b_pvt->vsi = vsi_id;
	b_pvt->mtu = dev->mtu;
	b_pvt->wan_netdev = NULL;
	b_pvt->wan_if_enabled = false;
	ether_addr_copy(b_pvt->dev_addr, dev->dev_addr);

	spin_lock(&br_mgr_ctx.lock);
	list_add(&b_pvt->list, &br_mgr_ctx.list);
	spin_unlock(&br_mgr_ctx.lock);
	return 0;
}

/*
 * nss_ppe_bridge_mgr_exit_module()
 *	bridge_mgr module exit function
 */
static void __exit nss_ppe_bridge_mgr_exit_module(void)
{
	unregister_netdevice_notifier(&nss_ppe_bridge_mgr_netdevice_nb);
	nss_ppe_bridge_mgr_info("Module unloaded\n");
	br_fdb_update_unregister_notify(&nss_ppe_bridge_mgr_fdb_update_notifier);

	if (br_mgr_ctx.nss_ppe_bridge_mgr_header) {
		unregister_sysctl_table(br_mgr_ctx.nss_ppe_bridge_mgr_header);
	}

#if defined (NSS_BRIDGE_MGR_OVS_ENABLE)
	nss_bridge_mgr_ovs_exit();
#endif
}

/*
 * nss_ppe_bridge_mgr_init_module()
 *	bridge_mgr module init function
 */
static int __init nss_ppe_bridge_mgr_init_module(void)
{
	/*
	 * Monitor bridge activity only on supported platform
	 */
	if (!of_machine_is_compatible("qcom,ipq9574-emulation")
			&& !of_machine_is_compatible("qcom,ipq9574")) {
		return -EINVAL;
	}

	INIT_LIST_HEAD(&br_mgr_ctx.list);
	spin_lock_init(&br_mgr_ctx.lock);
	register_netdevice_notifier(&nss_ppe_bridge_mgr_netdevice_nb);
	nss_ppe_bridge_mgr_info("Module (Build %s) loaded\n", NSS_PPE_BUILD_ID);
	br_mgr_ctx.wan_netdev = NULL;
	br_fdb_update_register_notify(&nss_ppe_bridge_mgr_fdb_update_notifier);
	br_mgr_ctx.nss_ppe_bridge_mgr_header = register_sysctl_table(nss_ppe_bridge_mgr_root_dir);

#if defined (NSS_BRIDGE_MGR_OVS_ENABLE)
	nss_bridge_mgr_ovs_init();
#endif

	return 0;
}

module_init(nss_ppe_bridge_mgr_init_module);
module_exit(nss_ppe_bridge_mgr_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("NSS PPE bridge manager");
