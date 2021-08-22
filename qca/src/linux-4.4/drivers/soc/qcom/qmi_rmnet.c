// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
 */

#include <soc/qcom/qmi_rmnet.h>
#include <soc/qcom/rmnet_qmi.h>
#include <linux/soc/qcom/qmi.h>
#include <linux/rtnetlink.h>
#include <uapi/linux/rtnetlink.h>
#include <net/pkt_sched.h>
#include <net/tcp.h>
#include "qmi_rmnet_i.h"
#include <trace/events/dfc.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <linux/alarmtimer.h>

#define NLMSG_FLOW_ACTIVATE 1
#define NLMSG_FLOW_DEACTIVATE 2
#define NLMSG_CLIENT_SETUP 4
#define NLMSG_CLIENT_DELETE 5
#define NLMSG_SCALE_FACTOR 6
#define NLMSG_WQ_FREQUENCY 7
#define NLMSG_FILTER_ADD 10
#define NLMSG_FILTER_REMOVE 11

#define FLAG_DFC_MASK 0x000F
#define FLAG_POWERSAVE_MASK 0x0010
#define FLAG_QMAP_MASK 0x0020

#define FLAG_TO_MODE(f) ((f) & FLAG_DFC_MASK)

#define DFC_SUPPORTED_MODE(m) \
	((m) == DFC_MODE_FLOW_ID || (m) == DFC_MODE_MQ_NUM || \
	 (m) == DFC_MODE_SA)

#define FLAG_TO_QMAP(f) ((f) & FLAG_QMAP_MASK)

int dfc_mode;
int dfc_qmap;
#define IS_ANCILLARY(type) ((type) != AF_INET && (type) != AF_INET6)

unsigned int rmnet_wq_frequency __read_mostly = 1000;

#define PS_WORK_ACTIVE_BIT 0
#define PS_INTERVAL (((!rmnet_wq_frequency) ?                             \
					1 : rmnet_wq_frequency/10) * (HZ/100))
#define NO_DELAY (0x0000 * HZ)
#define PS_INTERVAL_KT (ms_to_ktime(1000))
#define WATCHDOG_EXPIRE_JF (msecs_to_jiffies(50))

#ifdef CONFIG_QCOM_QMI_DFC
static unsigned int qmi_rmnet_scale_factor = 5;
static LIST_HEAD(qos_cleanup_list);
#endif

static int
qmi_rmnet_del_flow(struct net_device *dev, struct tcmsg *tcm,
		   struct qmi_info *qmi);

struct qmi_elem_info data_ep_id_type_v01_ei[] = {
	{
		.data_type	= QMI_SIGNED_4_BYTE_ENUM,
		.elem_len	= 1,
		.elem_size	= sizeof(enum data_ep_type_enum_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
		.offset		= offsetof(struct data_ep_id_type_v01,
					   ep_type),
		.ei_array	= NULL,
	},
	{
		.data_type	= QMI_UNSIGNED_4_BYTE,
		.elem_len	= 1,
		.elem_size	= sizeof(u32),
		.array_type	= NO_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
		.offset		= offsetof(struct data_ep_id_type_v01,
					   iface_id),
		.ei_array	= NULL,
	},
	{
		.data_type	= QMI_EOTI,
		.elem_len	= 0,
		.elem_size	= 0,
		.array_type	= NO_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
		.offset		= 0,
		.ei_array	= NULL,
	},
};
EXPORT_SYMBOL(data_ep_id_type_v01_ei);

void *qmi_rmnet_has_dfc_client(struct qmi_info *qmi)
{
	int i;

	if (!qmi)
		return NULL;

	for (i = 0; i < MAX_CLIENT_NUM; i++) {
		if (qmi->dfc_clients[i])
			return qmi->dfc_clients[i];
	}

	return NULL;
}

static inline int
qmi_rmnet_has_client(struct qmi_info *qmi)
{
	if (qmi->wda_client)
		return 1;

	return qmi_rmnet_has_dfc_client(qmi) ? 1 : 0;
}

static int
qmi_rmnet_has_pending(struct qmi_info *qmi)
{
	int i;

	if (qmi->wda_pending)
		return 1;

	for (i = 0; i < MAX_CLIENT_NUM; i++) {
		if (qmi->dfc_pending[i])
			return 1;
	}

	return 0;
}

#ifdef CONFIG_QCOM_QMI_DFC
static void
qmi_rmnet_clean_flow_list(struct qos_info *qos)
{
	struct rmnet_bearer_map *bearer, *br_tmp;
	struct rmnet_flow_map *itm, *fl_tmp;
	struct dfc_filter *dfilter, *f_tmp;

	ASSERT_RTNL();

	list_for_each_entry_safe(itm, fl_tmp, &qos->flow_head, list) {
		list_del(&itm->list);
		kfree(itm);
	}

	list_for_each_entry_safe(bearer, br_tmp, &qos->bearer_head, list) {
		list_del(&bearer->list);
		kfree(bearer);
	}

	list_for_each_entry_safe(dfilter, f_tmp, &qos->filter_head,
				 sorted_list) {
		list_del(&dfilter->sorted_list);
		kfree(dfilter);
	}

	qos->num_filters = 0;
	memset(qos->mq, 0, sizeof(qos->mq));
}

struct rmnet_flow_map *
qmi_rmnet_get_flow_map(struct qos_info *qos, u32 flow_id, int ip_type)
{
	struct rmnet_flow_map *itm;

	if (!qos)
		return NULL;

	list_for_each_entry(itm, &qos->flow_head, list) {
		if ((itm->flow_id == flow_id) && (itm->ip_type == ip_type))
			return itm;
	}
	return NULL;
}

struct rmnet_bearer_map *
qmi_rmnet_get_bearer_map(struct qos_info *qos, uint8_t bearer_id)
{
	struct rmnet_bearer_map *itm;

	if (!qos)
		return NULL;

	list_for_each_entry(itm, &qos->bearer_head, list) {
		if (itm->bearer_id == bearer_id)
			return itm;
	}
	return NULL;
}

static void qmi_rmnet_update_flow_map(struct rmnet_flow_map *itm,
				      struct rmnet_flow_map *new_map)
{
	itm->bearer_id = new_map->bearer_id;
	itm->flow_id = new_map->flow_id;
	itm->ip_type = new_map->ip_type;
	itm->mq_idx = new_map->mq_idx;
}

int qmi_rmnet_flow_control(struct net_device *dev, u32 mq_idx, int enable)
{
	struct netdev_queue *q;

	if (unlikely(mq_idx >= dev->num_tx_queues))
		return 0;

	q = netdev_get_tx_queue(dev, mq_idx);
	if (unlikely(!q))
		return 0;

	if (enable)
		netif_tx_wake_queue(q);
	else
		netif_tx_stop_queue(q);

	trace_dfc_qmi_tc(dev->name, mq_idx, enable);

	return 0;
}

static void qmi_rmnet_reset_txq(struct net_device *dev, unsigned int txq)
{
	struct Qdisc *qdisc;

	if (unlikely(txq >= dev->num_tx_queues))
		return;

	qdisc = rtnl_dereference(netdev_get_tx_queue(dev, txq)->qdisc);
	if (qdisc) {
		spin_lock_bh(qdisc_lock(qdisc));
		qdisc_reset(qdisc);
		spin_unlock_bh(qdisc_lock(qdisc));
	}
}

/**
 * qmi_rmnet_watchdog_fn - watchdog timer func
 */
static void qmi_rmnet_watchdog_fn(unsigned long p)
{
	struct rmnet_bearer_map *bearer = (struct rmnet_bearer_map *)p;

	trace_dfc_watchdog(bearer->qos->mux_id, bearer->bearer_id, 2);

	spin_lock_bh(&bearer->qos->qos_lock);

	if (bearer->watchdog_quit)
		goto done;

	/*
	 * Possible stall, try to recover. Enable 80% query and jumpstart
	 * the bearer if disabled.
	 */
	bearer->watchdog_expire_cnt++;
	bearer->bytes_in_flight = 0;
	if (!bearer->grant_size) {
		bearer->grant_size = DEFAULT_CALL_GRANT;
		bearer->grant_thresh = qmi_rmnet_grant_per(bearer->grant_size);
		dfc_bearer_flow_ctl(bearer->qos->vnd_dev, bearer, bearer->qos);
	} else {
		bearer->grant_thresh = qmi_rmnet_grant_per(bearer->grant_size);
	}

done:
	bearer->watchdog_started = false;
	spin_unlock_bh(&bearer->qos->qos_lock);
}

/**
 * qmi_rmnet_watchdog_add - add the bearer to watch
 * Needs to be called with qos_lock
 */
void qmi_rmnet_watchdog_add(struct rmnet_bearer_map *bearer)
{
	bearer->watchdog_quit = false;

	if (bearer->watchdog_started)
		return;

	bearer->watchdog_started = true;
	mod_timer(&bearer->watchdog, jiffies + WATCHDOG_EXPIRE_JF);

	trace_dfc_watchdog(bearer->qos->mux_id, bearer->bearer_id, 1);
}

/**
 * qmi_rmnet_watchdog_remove - remove the bearer from watch
 * Needs to be called with qos_lock
 */
void qmi_rmnet_watchdog_remove(struct rmnet_bearer_map *bearer)
{
	bearer->watchdog_quit = true;

	if (!bearer->watchdog_started)
		return;

	if (try_to_del_timer_sync(&bearer->watchdog) >= 0)
		bearer->watchdog_started = false;

	trace_dfc_watchdog(bearer->qos->mux_id, bearer->bearer_id, 0);
}

/**
 * qmi_rmnet_bearer_clean - clean the removed bearer
 * Needs to be called with rtn_lock but not qos_lock
 */
static void qmi_rmnet_bearer_clean(struct qos_info *qos)
{
	if (qos->removed_bearer) {
		qos->removed_bearer->watchdog_quit = true;
		del_timer_sync(&qos->removed_bearer->watchdog);
		kfree(qos->removed_bearer);
		qos->removed_bearer = NULL;
	}
}

static struct rmnet_bearer_map *__qmi_rmnet_bearer_get(
				struct qos_info *qos_info, u8 bearer_id)
{
	struct rmnet_bearer_map *bearer;

	bearer = qmi_rmnet_get_bearer_map(qos_info, bearer_id);
	if (bearer) {
		bearer->flow_ref++;
	} else {
		bearer = kzalloc(sizeof(*bearer), GFP_ATOMIC);
		if (!bearer)
			return NULL;

		bearer->bearer_id = bearer_id;
		bearer->flow_ref = 1;
		bearer->grant_size = DEFAULT_CALL_GRANT;
		bearer->grant_thresh = qmi_rmnet_grant_per(bearer->grant_size);
		bearer->mq_idx = INVALID_MQ;
		bearer->ack_mq_idx = INVALID_MQ;
		bearer->qos = qos_info;
		setup_timer(&bearer->watchdog,
			    qmi_rmnet_watchdog_fn,
			    (unsigned long)bearer);
		list_add(&bearer->list, &qos_info->bearer_head);
	}

	return bearer;
}

static void __qmi_rmnet_bearer_put(struct net_device *dev,
				   struct qos_info *qos_info,
				   struct rmnet_bearer_map *bearer,
				   bool reset)
{
	struct mq_map *mq;
	int i, j;

	if (bearer && --bearer->flow_ref == 0) {
		for (i = 0; i < MAX_MQ_NUM; i++) {
			mq = &qos_info->mq[i];
			if (mq->bearer != bearer)
				continue;

			mq->bearer = NULL;
			if (reset) {
				qmi_rmnet_reset_txq(dev, i);
				qmi_rmnet_flow_control(dev, i, 1);

				if (dfc_mode == DFC_MODE_SA) {
					j = i + ACK_MQ_OFFSET;
					qmi_rmnet_reset_txq(dev, j);
					qmi_rmnet_flow_control(dev, j, 1);
				}
			}
		}

		/* Remove from bearer map */
		list_del(&bearer->list);
		qos_info->removed_bearer = bearer;
	}
}

static void __qmi_rmnet_update_mq(struct net_device *dev,
				  struct qos_info *qos_info,
				  struct rmnet_bearer_map *bearer,
				  struct rmnet_flow_map *itm)
{
	struct mq_map *mq;

	/* In SA mode default mq is not associated with any bearer */
	if (dfc_mode == DFC_MODE_SA && itm->mq_idx == DEFAULT_MQ_NUM)
		return;

	mq = &qos_info->mq[itm->mq_idx];
	if (!mq->bearer) {
		mq->bearer = bearer;

		if (dfc_mode == DFC_MODE_SA) {
			bearer->mq_idx = itm->mq_idx;
			bearer->ack_mq_idx = itm->mq_idx + ACK_MQ_OFFSET;
		} else {
			if (IS_ANCILLARY(itm->ip_type))
				bearer->ack_mq_idx = itm->mq_idx;
			else
				bearer->mq_idx = itm->mq_idx;
		}

		qmi_rmnet_flow_control(dev, itm->mq_idx,
				       bearer->grant_size > 0 ? 1 : 0);

		if (dfc_mode == DFC_MODE_SA)
			qmi_rmnet_flow_control(dev, bearer->ack_mq_idx,
					bearer->grant_size > 0 ? 1 : 0);
	}
}

static int __qmi_rmnet_rebind_flow(struct net_device *dev,
				   struct qos_info *qos_info,
				   struct rmnet_flow_map *itm,
				   struct rmnet_flow_map *new_map)
{
	struct rmnet_bearer_map *bearer;

	__qmi_rmnet_bearer_put(dev, qos_info, itm->bearer, false);

	bearer = __qmi_rmnet_bearer_get(qos_info, new_map->bearer_id);
	if (!bearer)
		return -ENOMEM;

	qmi_rmnet_update_flow_map(itm, new_map);
	itm->bearer = bearer;

	__qmi_rmnet_update_mq(dev, qos_info, bearer, itm);

	return 0;
}

static void __qmi_rmnet_remove_filters(struct qos_info *qos,
				       struct rmnet_flow_map *flow)
{
	struct dfc_filter *dfilter, *tmp;

	list_for_each_entry_safe(dfilter, tmp, &flow->filter_head, flow_list) {
		list_del(&dfilter->flow_list);
		list_del(&dfilter->sorted_list);
		kfree(dfilter);
		if (likely(qos->num_filters))
			qos->num_filters--;
	}
}

static int qmi_rmnet_add_flow(struct net_device *dev, struct tcmsg *tcm,
			      struct qmi_info *qmi)
{
	struct qos_info *qos_info = (struct qos_info *)rmnet_get_qos_pt(dev);
	struct rmnet_flow_map new_map, *itm;
	struct rmnet_bearer_map *bearer;
	struct tcmsg tmp_tcm;
	int rc = 0;

	if (!qos_info || !tcm || tcm->tcm_handle >= MAX_MQ_NUM)
		return -EINVAL;

	ASSERT_RTNL();

	/* flow activate
	 * tcm->tcm__pad1 - bearer_id, tcm->tcm_parent - flow_id,
	 * tcm->tcm_ifindex - ip_type, tcm->tcm_handle - mq_idx
	 */

	new_map.bearer_id = tcm->tcm__pad1;
	new_map.flow_id = tcm->tcm_parent;
	new_map.ip_type = tcm->tcm_ifindex;
	new_map.mq_idx = tcm->tcm_handle;
	trace_dfc_flow_info(dev->name, new_map.bearer_id, new_map.flow_id,
			    new_map.ip_type, new_map.mq_idx, 1);

again:
	spin_lock_bh(&qos_info->qos_lock);

	itm = qmi_rmnet_get_flow_map(qos_info, new_map.flow_id,
				     new_map.ip_type);
	if (itm) {
		if (itm->bearer_id != new_map.bearer_id) {
			rc = __qmi_rmnet_rebind_flow(
				dev, qos_info, itm, &new_map);
			goto done;
		} else if (itm->mq_idx != new_map.mq_idx) {
			tmp_tcm.tcm__pad1 = itm->bearer_id;
			tmp_tcm.tcm_parent = itm->flow_id;
			tmp_tcm.tcm_ifindex = itm->ip_type;
			tmp_tcm.tcm_handle = itm->mq_idx;
			spin_unlock_bh(&qos_info->qos_lock);
			qmi_rmnet_del_flow(dev, &tmp_tcm, qmi);
			goto again;
		} else {
			goto done;
		}
	}

	/* Create flow map */
	itm = kzalloc(sizeof(*itm), GFP_ATOMIC);
	if (!itm) {
		spin_unlock_bh(&qos_info->qos_lock);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&itm->filter_head);

	qmi_rmnet_update_flow_map(itm, &new_map);
	list_add(&itm->list, &qos_info->flow_head);

	/* Create or update bearer map */
	bearer = __qmi_rmnet_bearer_get(qos_info, new_map.bearer_id);
	if (!bearer) {
		rc = -ENOMEM;
		goto done;
	}

	itm->bearer = bearer;

	__qmi_rmnet_update_mq(dev, qos_info, bearer, itm);

done:
	spin_unlock_bh(&qos_info->qos_lock);

	qmi_rmnet_bearer_clean(qos_info);

	return rc;
}

static int
qmi_rmnet_del_flow(struct net_device *dev, struct tcmsg *tcm,
		   struct qmi_info *qmi)
{
	struct qos_info *qos_info = (struct qos_info *)rmnet_get_qos_pt(dev);
	struct rmnet_flow_map new_map, *itm;

	if (!qos_info)
		return -EINVAL;

	ASSERT_RTNL();

	/* flow deactivate
	 * tcm->tcm__pad1 - bearer_id, tcm->tcm_parent - flow_id,
	 * tcm->tcm_ifindex - ip_type
	 */

	spin_lock_bh(&qos_info->qos_lock);

	new_map.bearer_id = tcm->tcm__pad1;
	new_map.flow_id = tcm->tcm_parent;
	new_map.ip_type = tcm->tcm_ifindex;
	itm = qmi_rmnet_get_flow_map(qos_info, new_map.flow_id,
				     new_map.ip_type);
	if (itm) {
		trace_dfc_flow_info(dev->name, new_map.bearer_id,
				    new_map.flow_id, new_map.ip_type,
				    itm->mq_idx, 0);

		__qmi_rmnet_bearer_put(dev, qos_info, itm->bearer, true);

		/* Remove filters associated with the flow */
		__qmi_rmnet_remove_filters(qos_info, itm);

		/* Remove from flow map */
		list_del(&itm->list);
		kfree(itm);
	}

	if (list_empty(&qos_info->flow_head))
		netif_tx_wake_all_queues(dev);

	spin_unlock_bh(&qos_info->qos_lock);

	qmi_rmnet_bearer_clean(qos_info);

	return 0;
}

static void qmi_rmnet_query_flows(struct qmi_info *qmi)
{
	int i;

	for (i = 0; i < MAX_CLIENT_NUM; i++) {
		if (qmi->dfc_clients[i] && !dfc_qmap &&
		    !qmi->dfc_client_exiting[i])
			dfc_qmi_query_flow(qmi->dfc_clients[i]);
	}
}

struct rmnet_bearer_map *qmi_rmnet_get_bearer_noref(struct qos_info *qos_info,
						    u8 bearer_id)
{
	struct rmnet_bearer_map *bearer;

	bearer = __qmi_rmnet_bearer_get(qos_info, bearer_id);
	if (bearer)
		bearer->flow_ref--;

	return bearer;
}

static void qmi_rmnet_add_filter(struct net_device *dev,
				 struct tcmsg *tcm, int attrlen)
{
	struct qos_info *qos = (struct qos_info *)rmnet_get_qos_pt(dev);
	struct rmnet_flow_map *flow;
	struct qos_filter *filter;
	struct dfc_filter *dfilter, *entry;
	bool added = false;
	int i;

	if (!qos || !tcm ||
	    attrlen - sizeof(*tcm) != sizeof(struct qos_filter))
		return;

	dfilter = kzalloc(sizeof(struct dfc_filter), GFP_KERNEL);
	if (!dfilter)
		return;

	filter = (struct qos_filter *)((u8 *)tcm + sizeof(*tcm));
	memcpy(&dfilter->filter, filter, sizeof(struct qos_filter));

	for (i = 0; i < 4; i++)
		dfilter->filter.saddr[i] &= dfilter->filter.smask[i];
	for (i = 0; i < 4; i++)
		dfilter->filter.daddr[i] &= dfilter->filter.dmask[i];

	dfilter->filter.tos &= dfilter->filter.tos_mask;

	dfilter->filter.sport_range =
		dfilter->filter.sport + dfilter->filter.sport_range;
	dfilter->filter.dport_range =
		dfilter->filter.dport + dfilter->filter.dport_range;

	spin_lock_bh(&qos->qos_lock);

	/* tcm->tcm_parent - flow_id,
	 * tcm->tcm_ifindex - ip_type
	 */

	flow = qmi_rmnet_get_flow_map(qos, tcm->tcm_parent, tcm->tcm_ifindex);
	if (!flow) {
		kfree(dfilter);
		goto out;
	}

	/* Add to flow's filter list */
	dfilter->flow = flow;
	list_add(&dfilter->flow_list, &flow->filter_head);

	/* Add to device's filter list sorted by precedence */
	i = 0;
	list_for_each_entry(entry, &qos->filter_head, sorted_list) {
		if (entry->filter.precedence >= dfilter->filter.precedence) {
			__list_add(&dfilter->sorted_list,
				   (&entry->sorted_list)->prev,
				   &entry->sorted_list);
			added = true;
			break;
		}
		i++;
	}
	if (!added)
		list_add_tail(&dfilter->sorted_list, &qos->filter_head);
	qos->num_filters++;

out:
	spin_unlock_bh(&qos->qos_lock);
}

static void qmi_rmnet_remove_filters(struct net_device *dev, struct tcmsg *tcm)
{
	struct qos_info *qos = (struct qos_info *)rmnet_get_qos_pt(dev);
	struct rmnet_flow_map *flow;

	if (!qos || !tcm)
		return;

	spin_lock_bh(&qos->qos_lock);

	/* tcm->tcm_parent - flow_id,
	 * tcm->tcm_ifindex - ip_type
	 */

	flow = qmi_rmnet_get_flow_map(qos, tcm->tcm_parent, tcm->tcm_ifindex);
	if (flow)
		__qmi_rmnet_remove_filters(qos, flow);

	spin_unlock_bh(&qos->qos_lock);
}

#else
static inline void
qmi_rmnet_update_flow_map(struct rmnet_flow_map *itm,
			  struct rmnet_flow_map *new_map)
{
}

static inline int
qmi_rmnet_add_flow(struct net_device *dev, struct tcmsg *tcm,
		   struct qmi_info *qmi)
{
	return -EINVAL;
}

static inline int
qmi_rmnet_del_flow(struct net_device *dev, struct tcmsg *tcm,
		   struct qmi_info *qmi)
{
	return -EINVAL;
}

static inline void qmi_rmnet_query_flows(struct qmi_info *qmi)
{
}

static inline void qmi_rmnet_add_filter(struct net_device *dev,
					struct tcmsg *tcm, int attrlen)
{
}

static inline void qmi_rmnet_remove_filters(struct net_device *dev,
					    struct tcmsg *tcm)
{
}
#endif

static int
qmi_rmnet_setup_client(void *port, struct qmi_info *qmi, struct tcmsg *tcm)
{
	int idx, err = 0;
	struct svc_info svc;

	ASSERT_RTNL();

	/* client setup
	 * tcm->tcm_handle - instance, tcm->tcm_info - ep_type,
	 * tcm->tcm_parent - iface_id, tcm->tcm_ifindex - flags
	 */
	idx = (tcm->tcm_handle == 0) ? 0 : 1;

	if (!qmi) {
		qmi = kzalloc(sizeof(struct qmi_info), GFP_ATOMIC);
		if (!qmi)
			return -ENOMEM;

		rmnet_init_qmi_pt(port, qmi);
	}

	qmi->flag = tcm->tcm_ifindex;
	svc.instance = tcm->tcm_handle;
	svc.ep_type = tcm->tcm_info;
	svc.iface_id = tcm->tcm_parent;

	if (DFC_SUPPORTED_MODE(dfc_mode) &&
	    !qmi->dfc_clients[idx] && !qmi->dfc_pending[idx]) {
		if (dfc_qmap)
			err = dfc_qmap_client_init(port, idx, &svc, qmi);
		else
			err = dfc_qmi_client_init(port, idx, &svc, qmi);
		qmi->dfc_client_exiting[idx] = false;
	}

	if ((tcm->tcm_ifindex & FLAG_POWERSAVE_MASK) &&
	    (idx == 0) && !qmi->wda_client && !qmi->wda_pending) {
		err = wda_qmi_client_init(port, &svc, qmi);
	}

	return err;
}

static int
__qmi_rmnet_delete_client(void *port, struct qmi_info *qmi, int idx)
{
	void *data = NULL;

	ASSERT_RTNL();

	if (qmi->dfc_clients[idx])
		data = qmi->dfc_clients[idx];
	else if (qmi->dfc_pending[idx])
		data = qmi->dfc_pending[idx];

	if (data) {
		if (dfc_qmap)
			dfc_qmap_client_exit(data);
		else
			dfc_qmi_client_exit(data);
		qmi->dfc_clients[idx] = NULL;
		qmi->dfc_pending[idx] = NULL;
	}

	if (!qmi_rmnet_has_client(qmi) && !qmi_rmnet_has_pending(qmi)) {
		rmnet_reset_qmi_pt(port);
		kfree(qmi);
		return 0;
	}

	return 1;
}

static void
qmi_rmnet_delete_client(void *port, struct qmi_info *qmi, struct tcmsg *tcm)
{
	int idx;
	void *data = NULL;

	/* client delete: tcm->tcm_handle - instance*/
	idx = (tcm->tcm_handle == 0) ? 0 : 1;

	ASSERT_RTNL();
	if (qmi->wda_client)
		data = qmi->wda_client;
	else if (qmi->wda_pending)
		data = qmi->wda_pending;

	if ((idx == 0) && data) {
		wda_qmi_client_exit(data);
		qmi->wda_client = NULL;
		qmi->wda_pending = NULL;
	} else {
		qmi->dfc_client_exiting[idx] = true;
		qmi_rmnet_flush_ps_wq();
	}

	__qmi_rmnet_delete_client(port, qmi, idx);
}

void qmi_rmnet_change_link(struct net_device *dev, void *port, void *tcm_pt,
			   int attr_len)
{
	struct qmi_info *qmi = (struct qmi_info *)rmnet_get_qmi_pt(port);
	struct tcmsg *tcm = (struct tcmsg *)tcm_pt;

	switch (tcm->tcm_family) {
	case NLMSG_FLOW_ACTIVATE:
		if (!qmi || !DFC_SUPPORTED_MODE(dfc_mode) ||
		    !qmi_rmnet_has_dfc_client(qmi))
			return;

		qmi_rmnet_add_flow(dev, tcm, qmi);
		break;
	case NLMSG_FLOW_DEACTIVATE:
		if (!qmi || !DFC_SUPPORTED_MODE(dfc_mode))
			return;

		qmi_rmnet_del_flow(dev, tcm, qmi);
		break;
	case NLMSG_CLIENT_SETUP:
		dfc_mode = FLAG_TO_MODE(tcm->tcm_ifindex);
		dfc_qmap = FLAG_TO_QMAP(tcm->tcm_ifindex);

		if (!DFC_SUPPORTED_MODE(dfc_mode) &&
		    !(tcm->tcm_ifindex & FLAG_POWERSAVE_MASK))
			return;

		if (qmi_rmnet_setup_client(port, qmi, tcm) < 0) {
			/* retrieve qmi again as it could have been changed */
			qmi = (struct qmi_info *)rmnet_get_qmi_pt(port);
			if (qmi &&
			    !qmi_rmnet_has_client(qmi) &&
			    !qmi_rmnet_has_pending(qmi)) {
				rmnet_reset_qmi_pt(port);
				kfree(qmi);
			}
		} else if (tcm->tcm_ifindex & FLAG_POWERSAVE_MASK) {
			qmi_rmnet_work_init(port);
			rmnet_set_powersave_format(port);
		}
		break;
	case NLMSG_CLIENT_DELETE:
		if (!qmi)
			return;
		if (tcm->tcm_handle == 0) { /* instance 0 */
			rmnet_clear_powersave_format(port);
			qmi_rmnet_work_exit(port);
		}
		qmi_rmnet_delete_client(port, qmi, tcm);
		break;
	case NLMSG_SCALE_FACTOR:
		if (!tcm->tcm_ifindex)
			return;
		qmi_rmnet_scale_factor = tcm->tcm_ifindex;
		break;
	case NLMSG_WQ_FREQUENCY:
		rmnet_wq_frequency = tcm->tcm_ifindex;
		break;
	case NLMSG_FILTER_ADD:
		qmi_rmnet_add_filter(dev, tcm, attr_len);
		break;
	case NLMSG_FILTER_REMOVE:
		qmi_rmnet_remove_filters(dev, tcm);
		break;
	default:
		pr_debug("%s(): No handler\n", __func__);
		break;
	}
}
EXPORT_SYMBOL(qmi_rmnet_change_link);

void qmi_rmnet_qmi_exit(void *qmi_pt, void *port)
{
	struct qmi_info *qmi = (struct qmi_info *)qmi_pt;
	int i;
	void *data = NULL;

	if (!qmi)
		return;

	ASSERT_RTNL();

	qmi_rmnet_work_exit(port);

	if (qmi->wda_client)
		data = qmi->wda_client;
	else if (qmi->wda_pending)
		data = qmi->wda_pending;

	if (data) {
		wda_qmi_client_exit(data);
		qmi->wda_client = NULL;
		qmi->wda_pending = NULL;
	}

	for (i = 0; i < MAX_CLIENT_NUM; i++) {
		if (!__qmi_rmnet_delete_client(port, qmi, i))
			return;
	}
}
EXPORT_SYMBOL(qmi_rmnet_qmi_exit);

void qmi_rmnet_enable_all_flows(struct net_device *dev)
{
	struct qos_info *qos;
	struct rmnet_bearer_map *bearer;
	bool do_wake;

	qos = (struct qos_info *)rmnet_get_qos_pt(dev);
	if (!qos)
		return;

	spin_lock_bh(&qos->qos_lock);

	list_for_each_entry(bearer, &qos->bearer_head, list) {
		bearer->seq = 0;
		bearer->ack_req = 0;
		bearer->bytes_in_flight = 0;
		bearer->tcp_bidir = false;
		bearer->rat_switch = false;

		qmi_rmnet_watchdog_remove(bearer);

		if (bearer->tx_off)
			continue;

		do_wake = !bearer->grant_size;
		bearer->grant_size = DEFAULT_GRANT;
		bearer->grant_thresh = qmi_rmnet_grant_per(DEFAULT_GRANT);

		if (do_wake)
			dfc_bearer_flow_ctl(dev, bearer, qos);
	}

	spin_unlock_bh(&qos->qos_lock);
}
EXPORT_SYMBOL(qmi_rmnet_enable_all_flows);

bool qmi_rmnet_all_flows_enabled(struct net_device *dev)
{
	struct qos_info *qos;
	struct rmnet_bearer_map *bearer;
	bool ret = true;

	qos = (struct qos_info *)rmnet_get_qos_pt(dev);
	if (!qos)
		return true;

	spin_lock_bh(&qos->qos_lock);

	list_for_each_entry(bearer, &qos->bearer_head, list) {
		if (!bearer->grant_size) {
			ret = false;
			break;
		}
	}

	spin_unlock_bh(&qos->qos_lock);

	return ret;
}
EXPORT_SYMBOL(qmi_rmnet_all_flows_enabled);

#ifdef CONFIG_QCOM_QMI_DFC
void qmi_rmnet_burst_fc_check(struct net_device *dev,
			      int ip_type, u32 mark, unsigned int len)
{
	struct qos_info *qos = rmnet_get_qos_pt(dev);

	if (!qos)
		return;

	dfc_qmi_burst_check(dev, qos, ip_type, mark, len);
}
EXPORT_SYMBOL(qmi_rmnet_burst_fc_check);

static bool _qmi_rmnet_is_tcp_ack(struct sk_buff *skb)
{
	struct tcphdr *th;
	int ip_hdr_len;
	int ip_payload_len;

	if (skb->protocol == htons(ETH_P_IP) &&
	    ip_hdr(skb)->protocol == IPPROTO_TCP) {
		ip_hdr_len = ip_hdr(skb)->ihl << 2;
		ip_payload_len = ntohs(ip_hdr(skb)->tot_len) - ip_hdr_len;
	} else if (skb->protocol == htons(ETH_P_IPV6) &&
		   ipv6_hdr(skb)->nexthdr == IPPROTO_TCP) {
		ip_hdr_len = sizeof(struct ipv6hdr);
		ip_payload_len = ntohs(ipv6_hdr(skb)->payload_len);
	} else {
		return false;
	}

	th = (struct tcphdr *)(skb->data + ip_hdr_len);
	if ((ip_payload_len == th->doff << 2) &&
	    ((tcp_flag_word(th) & cpu_to_be32(0x00FF0000)) == TCP_FLAG_ACK))
		return true;

	return false;
}

static inline bool qmi_rmnet_is_tcp_ack(struct sk_buff *skb)
{
	/* Locally generated TCP acks */
	if (skb_is_tcp_pure_ack(skb))
		return true;

	/* Forwarded */
	if (_qmi_rmnet_is_tcp_ack(skb))
		return true;

	return false;
}

static int qmi_rmnet_get_queue_sa(struct qos_info *qos, struct sk_buff *skb)
{
	struct rmnet_flow_map *itm;
	int ip_type;
	int txq = DEFAULT_MQ_NUM;

	/* Put NDP in default mq */
	if (skb->protocol == htons(ETH_P_IPV6) &&
	    ipv6_hdr(skb)->nexthdr == IPPROTO_ICMPV6) {
		struct icmp6hdr *h = (struct icmp6hdr *)
				(skb->data + sizeof(struct ipv6hdr));
		if (h->icmp6_type >= 133 && h->icmp6_type <= 137)
			return DEFAULT_MQ_NUM;
	}

	ip_type = (skb->protocol == htons(ETH_P_IPV6)) ? AF_INET6 : AF_INET;

	spin_lock_bh(&qos->qos_lock);

	itm = qmi_rmnet_get_flow_map(qos, skb->mark, ip_type);
	if (unlikely(!itm))
		goto done;

	/* Put the packet in the assigned mq except TCP ack */
	if (likely(itm->bearer) && qmi_rmnet_is_tcp_ack(skb))
		txq = itm->bearer->ack_mq_idx;
	else
		txq = itm->mq_idx;

done:
	spin_unlock_bh(&qos->qos_lock);
	return txq;
}

int qmi_rmnet_get_queue(struct net_device *dev, struct sk_buff *skb)
{
	struct qos_info *qos = rmnet_get_qos_pt(dev);
	int txq = 0, ip_type = AF_INET;
	struct rmnet_flow_map *itm;
	u32 mark = skb->mark;

	if (!qos)
		return 0;

	/* If mark is mq num return it */
	if (dfc_mode == DFC_MODE_MQ_NUM)
		return mark;

	if (dfc_mode == DFC_MODE_SA)
		return qmi_rmnet_get_queue_sa(qos, skb);

	/* Default flows */
	if (!mark) {
		if (qmi_rmnet_is_tcp_ack(skb))
			return 1;
		else
			return 0;
	}

	ip_type = (skb->protocol == htons(ETH_P_IPV6)) ? AF_INET6 : AF_INET;

	/* Dedicated flows */
	spin_lock_bh(&qos->qos_lock);

	itm = qmi_rmnet_get_flow_map(qos, mark, ip_type);
	if (unlikely(!itm))
		goto done;

	txq = itm->mq_idx;

done:
	spin_unlock_bh(&qos->qos_lock);
	return txq;
}
EXPORT_SYMBOL(qmi_rmnet_get_queue);

inline unsigned int qmi_rmnet_grant_per(unsigned int grant)
{
	return grant / qmi_rmnet_scale_factor;
}
EXPORT_SYMBOL(qmi_rmnet_grant_per);

void *qmi_rmnet_qos_init(struct net_device *real_dev,
			 struct net_device *vnd_dev, u8 mux_id)
{
	struct qos_info *qos;

	qos = kzalloc(sizeof(*qos), GFP_KERNEL);
	if (!qos)
		return NULL;

	qos->mux_id = mux_id;
	qos->real_dev = real_dev;
	qos->vnd_dev = vnd_dev;
	qos->tran_num = 0;
	INIT_LIST_HEAD(&qos->flow_head);
	INIT_LIST_HEAD(&qos->bearer_head);
	spin_lock_init(&qos->qos_lock);
	INIT_LIST_HEAD(&qos->filter_head);

	return qos;
}
EXPORT_SYMBOL(qmi_rmnet_qos_init);

void qmi_rmnet_qos_exit_pre(void *qos)
{
	struct qos_info *qosi = (struct qos_info *)qos;
	struct rmnet_bearer_map *bearer;

	if (!qos)
		return;

	list_for_each_entry(bearer, &qosi->bearer_head, list) {
		bearer->watchdog_quit = true;
		del_timer_sync(&bearer->watchdog);
	}

	list_add(&qosi->list, &qos_cleanup_list);
}
EXPORT_SYMBOL(qmi_rmnet_qos_exit_pre);

void qmi_rmnet_qos_exit_post(void)
{
	struct qos_info *qos, *tmp;

	synchronize_rcu();
	list_for_each_entry_safe(qos, tmp, &qos_cleanup_list, list) {
		list_del(&qos->list);
		qmi_rmnet_clean_flow_list(qos);
		kfree(qos);
	}
}
EXPORT_SYMBOL(qmi_rmnet_qos_exit_post);

static int qmi_rmnet_dissect_skb(struct sk_buff *skb, struct dissect_info *di)
{
	struct iphdr *ip;
	struct ipv6hdr *ip6;
	struct tcphdr *thdr;
	struct udphdr *uhdr;

	if (unlikely(skb_is_nonlinear(skb)))
		return -EINVAL;

	di->ver = skb->data[0] & 0xF0;
	di->is_frag = 0;
	di->sport = 0;
	di->dport = 0;

	if (di->ver == 0x40) {
		ip = (struct iphdr *)skb->data;
		di->proto = ip->protocol;
		di->saddr = &ip->saddr;
		di->daddr = &ip->daddr;
		di->tos = ip->tos >> 2;
		if (ip->frag_off & htons(0x3FF)) {
			di->is_frag = 1;
			return 0;
		}
		if (di->proto == IPPROTO_TCP) {
			thdr = (struct tcphdr *)(skb->data + (ip->ihl << 2));
			di->sport = ntohs(thdr->source);
			di->dport = ntohs(thdr->dest);
		} else if (di->proto == IPPROTO_UDP) {
			uhdr = (struct udphdr *)(skb->data + (ip->ihl << 2));
			di->sport = ntohs(uhdr->source);
			di->dport = ntohs(uhdr->dest);
		}
	} else if (di->ver == 0x60) {
		ip6 = (struct ipv6hdr *)skb->data;
		di->proto = ip6->nexthdr;
		if (di->proto == IPPROTO_FRAGMENT) {
			struct frag_hdr *fhdr = (struct frag_hdr *)
					(skb->data + sizeof(struct ipv6hdr));
			di->proto = fhdr->nexthdr;
			di->is_frag = 1;
		}
		di->saddr = ip6->saddr.in6_u.u6_addr32;
		di->daddr = ip6->daddr.in6_u.u6_addr32;
		if (di->is_frag)
			return 0;
		if (di->proto == IPPROTO_TCP) {
			thdr = (struct tcphdr *)(skb->data +
						 sizeof(struct ipv6hdr));
			di->sport = ntohs(thdr->source);
			di->dport = ntohs(thdr->dest);
		} else if (di->proto == IPPROTO_UDP) {
			uhdr = (struct udphdr *)(skb->data +
						 sizeof(struct ipv6hdr));
			di->sport = ntohs(uhdr->source);
			di->dport = ntohs(uhdr->dest);
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static bool qmi_rmnet_match_filter(struct dissect_info *di,
				   struct qos_filter *filter, int ip_type)
{
	int i;
	int addr_len;

	if (di->ver == 0x40) {
		if (ip_type != AF_INET)
			return false;
		addr_len = 1;

		if ((filter->filter_mask & QOS_FILTER_MASK_TOS) &&
		    (di->tos & filter->tos_mask) != filter->tos)
			return false;
	} else if (di->ver == 0x60) {
		if (ip_type != AF_INET6)
			return false;
		addr_len = 4;
	} else {
		return false;
	}

	if ((filter->filter_mask & QOS_FILTER_MASK_PROTO) &&
	    (di->proto != filter->proto))
		return false;

	if (filter->filter_mask & QOS_FILTER_MASK_SADDR)
		for (i = 0; i < addr_len; i++)
			if ((di->saddr[i] & filter->smask[i])
					!= filter->saddr[i])
				return false;

	if (filter->filter_mask & QOS_FILTER_MASK_DADDR)
		for (i = 0; i < addr_len; i++)
			if ((di->daddr[i] & filter->dmask[i])
					!= filter->daddr[i])
				return false;

	if (!(filter->filter_mask & QOS_FILTER_MASK_SPORT) &&
	    !(filter->filter_mask & QOS_FILTER_MASK_DPORT))
		return true;

	if (di->is_frag)
		return false;

	if (di->proto != IPPROTO_TCP && di->proto != IPPROTO_UDP)
		return false;

	if ((filter->filter_mask & QOS_FILTER_MASK_SPORT) &&
	    (di->sport < filter->sport || di->sport > filter->sport_range))
		return false;

	if ((filter->filter_mask & QOS_FILTER_MASK_DPORT) &&
	    (di->dport < filter->dport || di->dport > filter->dport_range))
		return false;

	return true;
}

/* Mark skb with matching UL filter's flow id */
void qmi_rmnet_mark_skb(struct net_device *dev, struct sk_buff *skb)
{
	struct qos_info *qos;
	struct dfc_filter *dfilter;
	struct dissect_info di;

	skb->mark = 0;

	qos = (struct qos_info *)rmnet_get_qos_pt(dev);
	if (!qos || !qos->num_filters)
		return;

	if (qmi_rmnet_dissect_skb(skb, &di) < 0)
		return;

	spin_lock_bh(&qos->qos_lock);

	list_for_each_entry(dfilter, &qos->filter_head, sorted_list) {
		if (qmi_rmnet_match_filter(&di, &dfilter->filter,
					   dfilter->flow->ip_type)) {
			skb->mark = dfilter->flow->flow_id;
			break;
		}
	}

	spin_unlock_bh(&qos->qos_lock);
}
EXPORT_SYMBOL(qmi_rmnet_mark_skb);

#endif

#ifdef CONFIG_QCOM_QMI_POWER_COLLAPSE
static struct workqueue_struct  *rmnet_ps_wq;
static struct rmnet_powersave_work *rmnet_work;
static bool rmnet_work_quit;
static LIST_HEAD(ps_list);

struct rmnet_powersave_work {
	struct delayed_work work;
	struct alarm atimer;
	void *port;
	u64 old_rx_pkts;
	u64 old_tx_pkts;
};

void qmi_rmnet_ps_on_notify(void *port)
{
	struct qmi_rmnet_ps_ind *tmp;

	list_for_each_entry_rcu(tmp, &ps_list, list)
		tmp->ps_on_handler(port);
}
EXPORT_SYMBOL(qmi_rmnet_ps_on_notify);

void qmi_rmnet_ps_off_notify(void *port)
{
	struct qmi_rmnet_ps_ind *tmp;

	list_for_each_entry_rcu(tmp, &ps_list, list)
		tmp->ps_off_handler(port);

}
EXPORT_SYMBOL(qmi_rmnet_ps_off_notify);

int qmi_rmnet_ps_ind_register(void *port,
			      struct qmi_rmnet_ps_ind *ps_ind)
{

	if (!port || !ps_ind || !ps_ind->ps_on_handler ||
	    !ps_ind->ps_off_handler)
		return -EINVAL;

	list_add_rcu(&ps_ind->list, &ps_list);

	return 0;
}
EXPORT_SYMBOL(qmi_rmnet_ps_ind_register);

int qmi_rmnet_ps_ind_deregister(void *port,
				struct qmi_rmnet_ps_ind *ps_ind)
{
	struct qmi_rmnet_ps_ind *tmp;

	if (!port || !ps_ind)
		return -EINVAL;

	list_for_each_entry_rcu(tmp, &ps_list, list) {
		if (tmp == ps_ind) {
			list_del_rcu(&ps_ind->list);
			goto done;
		}
	}
done:
	return 0;
}
EXPORT_SYMBOL(qmi_rmnet_ps_ind_deregister);

int qmi_rmnet_set_powersave_mode(void *port, uint8_t enable)
{
	int rc = -EINVAL;
	struct qmi_info *qmi = (struct qmi_info *)rmnet_get_qmi_pt(port);

	if (!qmi || !qmi->wda_client)
		return rc;

	rc = wda_set_powersave_mode(qmi->wda_client, enable);
	if (rc < 0) {
		pr_err("%s() failed set powersave mode[%u], err=%d\n",
			__func__, enable, rc);
		return rc;
	}

	return 0;
}
EXPORT_SYMBOL(qmi_rmnet_set_powersave_mode);

static void qmi_rmnet_work_restart(void *port)
{
	rcu_read_lock();
	if (!READ_ONCE(rmnet_work_quit))
		queue_delayed_work(rmnet_ps_wq, &rmnet_work->work, NO_DELAY);
	rcu_read_unlock();
}

static enum alarmtimer_restart qmi_rmnet_work_alarm(struct alarm *atimer,
						    ktime_t now)
{
	struct rmnet_powersave_work *real_work;

	real_work = container_of(atimer, struct rmnet_powersave_work, atimer);
	qmi_rmnet_work_restart(real_work->port);
	return ALARMTIMER_NORESTART;
}

static void qmi_rmnet_check_stats(struct work_struct *work)
{
	struct rmnet_powersave_work *real_work;
	struct qmi_info *qmi;
	u64 rxd, txd;
	u64 rx, tx;
	bool dl_msg_active;
	bool use_alarm_timer = true;

	real_work = container_of(to_delayed_work(work),
				 struct rmnet_powersave_work, work);

	if (unlikely(!real_work || !real_work->port))
		return;

	qmi = (struct qmi_info *)rmnet_get_qmi_pt(real_work->port);
	if (unlikely(!qmi))
		return;

	if (qmi->ps_enabled) {

		/* Ready to accept grant */
		qmi->ps_ignore_grant = false;

		/* Register to get QMI DFC and DL marker */
		if (qmi_rmnet_set_powersave_mode(real_work->port, 0) < 0)
			goto end;

		qmi->ps_enabled = false;

		/* Do a query when coming out of powersave */
		qmi_rmnet_query_flows(qmi);

		if (rmnet_get_powersave_notif(real_work->port))
			qmi_rmnet_ps_off_notify(real_work->port);

		goto end;
	}

	rmnet_get_packets(real_work->port, &rx, &tx);
	rxd = rx - real_work->old_rx_pkts;
	txd = tx - real_work->old_tx_pkts;
	real_work->old_rx_pkts = rx;
	real_work->old_tx_pkts = tx;

	dl_msg_active = qmi->dl_msg_active;
	qmi->dl_msg_active = false;

	if (!rxd && !txd) {
		/* If no DL msg received and there is a flow disabled,
		 * (likely in RLF), no need to enter powersave
		 */
		if (!dl_msg_active &&
		    !rmnet_all_flows_enabled(real_work->port)) {
			use_alarm_timer = false;
			goto end;
		}

		/* Deregister to suppress QMI DFC and DL marker */
		if (qmi_rmnet_set_powersave_mode(real_work->port, 1) < 0)
			goto end;

		qmi->ps_enabled = true;

		/* Ignore grant after going into powersave */
		qmi->ps_ignore_grant = true;

		/* Clear the bit before enabling flow so pending packets
		 * can trigger the work again
		 */
		clear_bit(PS_WORK_ACTIVE_BIT, &qmi->ps_work_active);
		rmnet_enable_all_flows(real_work->port);

		if (rmnet_get_powersave_notif(real_work->port))
			qmi_rmnet_ps_on_notify(real_work->port);

		return;
	}
end:
	rcu_read_lock();
	if (!READ_ONCE(rmnet_work_quit)) {
		if (use_alarm_timer)
			alarm_start_relative(&real_work->atimer,
					     PS_INTERVAL_KT);
		else
			queue_delayed_work(rmnet_ps_wq, &real_work->work,
					   PS_INTERVAL);
	}
	rcu_read_unlock();
}

static void qmi_rmnet_work_set_active(void *port, int status)
{
	struct qmi_info *qmi;

	qmi = (struct qmi_info *)rmnet_get_qmi_pt(port);
	if (unlikely(!qmi))
		return;

	if (status)
		set_bit(PS_WORK_ACTIVE_BIT, &qmi->ps_work_active);
	else
		clear_bit(PS_WORK_ACTIVE_BIT, &qmi->ps_work_active);
}

void qmi_rmnet_work_init(void *port)
{
	if (rmnet_ps_wq)
		return;

	rmnet_ps_wq = alloc_workqueue("rmnet_powersave_work",
					WQ_CPU_INTENSIVE, 1);

	if (!rmnet_ps_wq)
		return;

	rmnet_work = kzalloc(sizeof(*rmnet_work), GFP_ATOMIC);
	if (!rmnet_work) {
		destroy_workqueue(rmnet_ps_wq);
		rmnet_ps_wq = NULL;
		return;
	}
	INIT_DEFERRABLE_WORK(&rmnet_work->work, qmi_rmnet_check_stats);
	alarm_init(&rmnet_work->atimer, ALARM_BOOTTIME, qmi_rmnet_work_alarm);
	rmnet_work->port = port;
	rmnet_get_packets(rmnet_work->port, &rmnet_work->old_rx_pkts,
			  &rmnet_work->old_tx_pkts);

	WRITE_ONCE(rmnet_work_quit, false);
	qmi_rmnet_work_set_active(rmnet_work->port, 1);
	queue_delayed_work(rmnet_ps_wq, &rmnet_work->work, PS_INTERVAL);
}
EXPORT_SYMBOL(qmi_rmnet_work_init);

void qmi_rmnet_work_maybe_restart(void *port)
{
	struct qmi_info *qmi;

	qmi = (struct qmi_info *)rmnet_get_qmi_pt(port);
	if (unlikely(!qmi))
		return;

	if (!test_and_set_bit(PS_WORK_ACTIVE_BIT, &qmi->ps_work_active))
		qmi_rmnet_work_restart(port);
}
EXPORT_SYMBOL(qmi_rmnet_work_maybe_restart);

void qmi_rmnet_work_exit(void *port)
{
	if (!rmnet_ps_wq || !rmnet_work)
		return;

	WRITE_ONCE(rmnet_work_quit, true);
	/* Make sure others see the new value after barrier */
	smp_mb();
	synchronize_rcu();

	alarm_cancel(&rmnet_work->atimer);
	cancel_delayed_work(&rmnet_work->work);
	destroy_workqueue(rmnet_ps_wq);
	rmnet_ps_wq = NULL;
	kfree(rmnet_work);
	rmnet_work = NULL;
}
EXPORT_SYMBOL(qmi_rmnet_work_exit);

void qmi_rmnet_set_dl_msg_active(void *port)
{
	struct qmi_info *qmi;

	qmi = (struct qmi_info *)rmnet_get_qmi_pt(port);
	if (unlikely(!qmi))
		return;

	qmi->dl_msg_active = true;
}
EXPORT_SYMBOL(qmi_rmnet_set_dl_msg_active);

void qmi_rmnet_flush_ps_wq(void)
{
	if (rmnet_ps_wq)
		flush_workqueue(rmnet_ps_wq);
}

bool qmi_rmnet_ignore_grant(void *port)
{
	struct qmi_info *qmi;

	qmi = (struct qmi_info *)rmnet_get_qmi_pt(port);
	if (unlikely(!qmi))
		return false;

	return qmi->ps_ignore_grant;
}
EXPORT_SYMBOL(qmi_rmnet_ignore_grant);
#endif
