/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
#include <linux/if.h>
#include <linux/platform_device.h>
#include "wil6210.h"
#include "txrx.h"
#include "wmi.h"
#include "slave.h"
#include "slave_i.h"

struct wil_slave_entry {
	struct wil6210_priv *wil;
	void *ctx; /* master driver context */
	char *board_file; /* board file override */
	struct wil_slave_rops rops;
	struct wil_slave_platdata pdata;
	struct platform_device *pdev;
};

static DEFINE_MUTEX(slave_lock);

static atomic_t slave_counter;

static void wil_slave_clear_master_ctx(struct wil_slave_entry *slave)
{
	struct wil6210_priv *wil = slave->wil;

	lockdep_assert_held(&slave_lock);

	slave->ctx = NULL;
	/* make sure all rops will see the cleared context */
	wmb();
	/* make sure master context is not used */
	if (test_bit(wil_status_napi_en, wil->status)) {
		napi_synchronize(&wil->napi_rx);
		napi_synchronize(&wil->napi_tx);
	}
	flush_work(&wil->wmi_event_worker);
}

static int wil_slave_ioctl(void *dev, u16 code, const u8 *req_buf, u16 req_len,
			   u8 *resp_buf, u16 *resp_len)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;
	struct wil6210_vif *vif = ndev_to_vif(wil->main_ndev);
	struct wmi_internal_fw_ioctl_cmd *cmd;
	struct {
		struct wmi_cmd_hdr wmi;
		struct wmi_internal_fw_ioctl_event evt;
	} __packed * reply;
	u16 cmd_len, reply_len, evt_len;
	int rc;

	wil_dbg_misc(wil, "slave_ioctl, code %d\n", code);

	if (!resp_len)
		return -EINVAL;

	if (req_len > WMI_MAX_IOCTL_PAYLOAD_SIZE) {
		wil_err(wil, "request too large (%d, max %d)\n",
			req_len, WMI_MAX_IOCTL_PAYLOAD_SIZE);
		return -EINVAL;
	}

	cmd_len = sizeof(*cmd) + req_len;
	cmd = kmalloc(cmd_len, GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;
	reply_len = sizeof(*reply) + WMI_MAX_IOCTL_REPLY_PAYLOAD_SIZE;
	reply = kmalloc(reply_len, GFP_KERNEL);
	if (!reply) {
		rc = -ENOMEM;
		goto out_cmd;
	}
	cmd->code = cpu_to_le16(code);
	cmd->length = cpu_to_le16(req_len);
	memcpy(cmd->payload, req_buf, req_len);
	memset(reply, 0, sizeof(*reply));
	reply->evt.status = WMI_FW_STATUS_FAILURE;
	rc = wmi_call(wil, WMI_INTERNAL_FW_IOCTL_CMDID, vif->mid,
		      cmd, cmd_len,
		      WMI_INTERNAL_FW_IOCTL_EVENTID, reply, reply_len,
		      WIL6210_FW_RECOVERY_TO);
	if (rc)
		goto out_reply;
	if (reply->evt.status) {
		wil_err(wil, "ioctl failed with status %d\n",
			reply->evt.status);
		rc = -EINVAL;
		goto out_reply;
	}
	evt_len = le16_to_cpu(reply->evt.length);
	if (evt_len > *resp_len) {
		wil_err(wil, "response buffer too short (have %d need %d)\n",
			*resp_len, evt_len);
		rc = -EINVAL;
		goto out_reply;
	}
	memcpy(resp_buf, &reply->evt.payload, evt_len);
	*resp_len = evt_len;
out_reply:
	kfree(reply);
out_cmd:
	kfree(cmd);
	return rc;
}

static int wil_slave_set_key(void *dev, const u8 *mac, const u8 *key, u16 len)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;
	struct wiphy *wiphy = wil_to_wiphy(wil);
	struct key_params params = {NULL};

	if (len > 0) {
		params.key = key;
		params.key_len = len;
		return wil_cfg80211_add_key(wiphy, wil->main_ndev, 0, true,
					    mac, &params);
	} else {
		return wil_cfg80211_del_key(wiphy, wil->main_ndev, 0, true,
					    mac);
	}
}

static int wil_slave_fw_reload(void *dev, const char *board_file)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;
	int rc, len;

	wil_dbg_misc(wil, "slave_fw_reload, board_file %s\n", board_file);

	kfree(slave->board_file);
	slave->board_file = NULL;
	if (board_file) {
		len = strlen(board_file) + 1;
		slave->board_file = kmemdup(board_file, len, GFP_KERNEL);
		if (!slave->board_file)
			return -ENOMEM;
	}

	wil_dbg_misc(wil, "resetting interface...\n");
	mutex_lock(&wil->mutex);
	__wil_down(wil);
	rc = __wil_up(wil);
	mutex_unlock(&wil->mutex);

	if (rc)
		wil_err(wil, "failed to reset interface, error %d\n", rc);

	return rc;
}

static void wil_slave_get_mac(void *dev, u8 *mac)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;

	ether_addr_copy(mac, wil->main_ndev->perm_addr);
}

static netdev_tx_t wil_slave_tx_data(void *dev, u8 cid, struct sk_buff *skb)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;
	struct net_device *ndev = wil->main_ndev;

	return _wil_start_xmit(skb, ndev);
}

static int wil_slave_link_stats(void *dev, u8 cid,
				struct wil_slave_link_stats *stats)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;
	struct wil_sta_info *sta;

	wil_dbg_misc(wil, "slave_link_stats, cid %d\n", cid);

	if (cid >= WIL6210_MAX_CID || !stats)
		return -EINVAL;

	sta = &wil->sta[cid];
	if (sta->status != wil_sta_connected || sta->mid != 0)
		return -EINVAL;

	stats->rx_packets = sta->stats.rx_packets;
	stats->tx_packets = sta->stats.tx_packets;
	stats->rx_bytes = sta->stats.rx_bytes;
	stats->tx_bytes = sta->stats.tx_bytes;
	stats->rx_errors = sta->stats.rx_dropped +
		sta->stats.rx_non_data_frame +
		sta->stats.rx_short_frame +
		sta->stats.rx_large_frame +
		sta->stats.rx_mic_error +
		sta->stats.rx_key_error +
		sta->stats.rx_amsdu_error;
	stats->tx_errors = sta->stats.tx_errors;
	stats->tx_pend_packets = atomic_read(&sta->stats.tx_pend_packets);
	stats->tx_pend_bytes = atomic_read(&sta->stats.tx_pend_bytes);

	return 0;
}

static struct napi_struct *wil_slave_get_napi_rx(void *dev)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;

	return &wil->napi_rx;
}

static void wil_slave_sync_rx(void *dev)
{
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;

	wil_dbg_misc(wil, "slave_sync_rx");

	mutex_lock(&wil->mutex);
	if (test_bit(wil_status_napi_en, wil->status))
		napi_synchronize(&wil->napi_rx);
	mutex_unlock(&wil->mutex);
}

static int wil_register_master(void *dev, void *ctx,
			       const struct wil_slave_rops *rops)
{
	int rc = 0;
	struct wil_slave_entry *slave = dev;
	struct wil6210_priv *wil = slave->wil;

	if (!rops)
		return -EINVAL;

	mutex_lock(&slave_lock);
	if (rops->api_version != WIL_SLAVE_API_VERSION) {
		wil_err(wil, "mismatched master API (expected %d have %d)\n",
			WIL_SLAVE_API_VERSION, rops->api_version);
		rc = -EINVAL;
		goto out;
	}

	if (slave_mode == 2) {
		rc = wil_up(wil);
		if (rc)
			goto out;
	}

	slave->rops = *rops;
	slave->ctx = ctx;
	wil_info(wil, "registered master for interface %s\n",
		 wil->main_ndev->name);
out:
	mutex_unlock(&slave_lock);
	return rc;
}

static void wil_unregister_master(void *dev)
{
	struct wil_slave_entry *slave;
	struct wil6210_priv *wil;

	if (!dev)
		return;

	mutex_lock(&slave_lock);
	slave = dev;

	wil = slave->wil;
	if (!wil)
		goto out;

	if (slave_mode == 2)
		wil_down(wil);

	wil_slave_clear_master_ctx(slave);

	wil_info(wil, "unregistered master for interface %s\n",
		 wil->main_ndev->name);
out:
	mutex_unlock(&slave_lock);
}

static struct wil_slave_ops slave_ops = {
	.api_version = WIL_SLAVE_API_VERSION,
	.register_master = wil_register_master,
	.unregister_master = wil_unregister_master,
	.ioctl = wil_slave_ioctl,
	.set_key = wil_slave_set_key,
	.tx_data = wil_slave_tx_data,
	.link_stats = wil_slave_link_stats,
	.fw_reload = wil_slave_fw_reload,
	.get_mac = wil_slave_get_mac,
	.get_napi_rx = wil_slave_get_napi_rx,
	.sync_rx = wil_slave_sync_rx,
};

int wil_register_slave(struct wil6210_priv *wil)
{
	int rc = 0;
	struct wil_slave_entry *slave;

	slave = kzalloc(sizeof(*slave), GFP_KERNEL);
	if (!slave)
		return -ENOMEM;

	slave->pdev = platform_device_alloc(
		"qwilvendor", atomic_inc_return(&slave_counter) - 1);
	if (!slave->pdev) {
		rc = -ENOMEM;
		goto out_slave;
	}

	slave->wil = wil;
	wil->slave_ctx = slave;
	slave->pdata.ops = &slave_ops;
	slave->pdata.dev_ctx = slave;
	rc = platform_device_add_data(slave->pdev, &slave->pdata,
				      sizeof(slave->pdata));
	if (rc)
		goto out;
	rc = platform_device_add(slave->pdev);
	if (rc) {
		wil_err(wil, "failed to add platform device, err %d\n", rc);
		goto out;
	}

	wil_info(wil, "added slave entry, interface %s\n",
		 wil->main_ndev->name);
	return 0;

out:
	platform_device_put(slave->pdev);
out_slave:
	kfree(slave);
	return rc;
}

void wil_unregister_slave(struct wil6210_priv *wil)
{
	struct wil_slave_entry *slave;

	mutex_lock(&slave_lock);

	slave = wil->slave_ctx;

	if (!slave) {
		mutex_unlock(&slave_lock);
		return;
	}

	if (slave->ctx) {
		mutex_unlock(&slave_lock);
		slave->rops.slave_going_down(slave->ctx);
		mutex_lock(&slave_lock);
		wil_slave_clear_master_ctx(slave);
	}

	mutex_unlock(&slave_lock);

	platform_device_unregister(slave->pdev);
	kfree(slave->board_file);
	kfree(slave);
}

static inline struct wil_slave_entry *
wil_get_slave_ctx(struct wil6210_vif *vif, void **master_ctx_out)
{
	struct wil6210_priv *wil = vif_to_wil(vif);
	struct wil_slave_entry *slave = wil->slave_ctx;
	void *master_ctx;

	if (!slave)
		return NULL;
	if (vif->mid != 0)
		return NULL;
	master_ctx = slave->ctx;
	if (!master_ctx) {
		wil_err(wil, "master not registered for interface %s\n",
			wil->main_ndev->name);
		return NULL;
	}
	*master_ctx_out = master_ctx;
	return slave;
}

void wil_slave_evt_internal_fw_event(struct wil6210_vif *vif,
				     struct wmi_internal_fw_event_event *evt,
				     int len)
{
	struct wil_slave_entry *slave;
	void *master_ctx;

	slave = wil_get_slave_ctx(vif, &master_ctx);
	if (!slave || len < sizeof(struct wmi_internal_fw_event_event))
		return;
	slave->rops.rx_event(master_ctx, le16_to_cpu(evt->id),
			     (u8 *)evt->payload, le16_to_cpu(evt->length));
}

void wil_slave_evt_internal_set_channel(
	struct wil6210_vif *vif,
	struct wmi_internal_fw_set_channel_event *evt,
	int len)
{
	struct wil_slave_entry *slave;
	void *master_ctx;

	slave = wil_get_slave_ctx(vif, &master_ctx);
	if (!slave || len < sizeof(struct wmi_internal_fw_set_channel_event))
		return;
	slave->rops.set_channel(master_ctx, evt->channel_num);
}

void wil_slave_tdm_connect(struct wil6210_vif *vif,
			   struct wmi_tdm_connect_event *evt,
			   int len)
{
	struct wil6210_priv *wil = vif_to_wil(vif);
	int rc;

	if (slave_mode != 2) {
		wil_err(wil, "TDM connection ignored, not full slave\n");
		return;
	}
	if (len < sizeof(*evt)) {
		wil_err(wil, "TDM Connect event too short : %d bytes\n", len);
		return;
	}
	if (evt->cid >= WIL6210_MAX_CID) {
		wil_err(wil, "TDM Connect CID invalid : %d\n", evt->cid);
		return;
	}

	wil_dbg_misc(wil, "TDM connect, CID %d MAC %pM link rx %d tx %d\n",
		     evt->cid, evt->mac_addr,
		     evt->link_id_rx, evt->link_id_tx);

	if (test_bit(wil_status_resetting, wil->status) ||
	    !test_bit(wil_status_fwready, wil->status)) {
		wil_err(wil, "status_resetting, cancel connect event, CID %d\n",
			evt->cid);
		/* no need for cleanup, wil_reset will do that */
		return;
	}

	mutex_lock(&wil->mutex);

	if (wil->sta[evt->cid].status != wil_sta_unused) {
		wil_err(wil, "Invalid status %d for CID %d\n",
			wil->sta[evt->cid].status, evt->cid);
		mutex_unlock(&wil->mutex);
		return;
	}

	ether_addr_copy(wil->sta[evt->cid].addr, evt->mac_addr);
	wil->sta[evt->cid].mid = vif->mid;
	wil->sta[evt->cid].status = wil_sta_conn_pending;

	rc = wil_ring_init_tx(vif, evt->cid);
	if (rc) {
		wil_err(wil, "config tx vring failed for CID %d, rc (%d)\n",
			evt->cid, rc);
		/* TODO we should notify FW about failure,
		 * cannot call wmi_sta_disconnect in full slave mode.
		 */
		goto out;
	}

	wil->sta[evt->cid].status = wil_sta_connected;
	wil->sta[evt->cid].aid = 0;
	if (!test_and_set_bit(wil_vif_fwconnected, vif->status))
		atomic_inc(&wil->connected_vifs);
	wil_update_cid_net_queues_bh(wil, vif, evt->cid, false);

	/* enable PBSS for data path support (no broadcast VRING) */
	vif->pbss = true;

	wil_slave_evt_connect(vif, evt->link_id_tx, evt->link_id_rx,
			      evt->mac_addr, evt->cid);
out:
	if (rc) {
		wil->sta[evt->cid].status = wil_sta_unused;
		wil->sta[evt->cid].mid = U8_MAX;
	}
	clear_bit(wil_vif_fwconnecting, vif->status);
	mutex_unlock(&wil->mutex);
}

void wil_slave_tdm_disconnect(struct wil6210_vif *vif,
			      struct wmi_tdm_disconnect_event *evt,
			      int len)
__acquires(&sta->tid_rx_lock) __releases(&sta->tid_rx_lock)
{
	uint i;
	struct wil6210_priv *wil = vif_to_wil(vif);
	struct wil_sta_info *sta;

	if (slave_mode != 2) {
		wil_err(wil, "TDM disconnect ignored, not full slave\n");
		return;
	}
	if (len < sizeof(*evt)) {
		wil_err(wil, "TDM disconnect event too short : %d bytes\n",
			len);
		return;
	}
	if (evt->cid >= WIL6210_MAX_CID) {
		wil_err(wil, "TDM disconnect CID invalid : %d\n", evt->cid);
		return;
	}

	sta =  &wil->sta[evt->cid];
	wil_dbg_misc(wil, "TDM disconnect_cid: CID %d, MID %d, status %d\n",
		     evt->cid, sta->mid, sta->status);

	if (test_bit(wil_status_resetting, wil->status) ||
	    !test_bit(wil_status_fwready, wil->status)) {
		wil_err(wil, "status_resetting, cancel disconnect event\n");
		/* no need for cleanup, wil_reset will do that */
		return;
	}

	mutex_lock(&wil->mutex);

	might_sleep();

	/* inform upper/lower layers */
	if (sta->status != wil_sta_unused) {
		wil_slave_evt_disconnect(vif, evt->cid);

		if (WIL_Q_PER_STA_USED(vif))
			wil_update_cid_net_queues_bh(wil, vif, evt->cid, true);
		sta->status = wil_sta_unused;
		sta->mid = U8_MAX;
		sta->fst_link_loss = false;
	}

	/* reorder buffers */
	for (i = 0; i < WIL_STA_TID_NUM; i++) {
		struct wil_tid_ampdu_rx *r;

		spin_lock_bh(&sta->tid_rx_lock);

		r = sta->tid_rx[i];
		sta->tid_rx[i] = NULL;
		wil_tid_ampdu_rx_free(wil, r);

		spin_unlock_bh(&sta->tid_rx_lock);
	}

	/* crypto context */
	memset(sta->tid_crypto_rx, 0, sizeof(sta->tid_crypto_rx));
	memset(&sta->group_crypto_rx, 0, sizeof(sta->group_crypto_rx));

	/* release vrings */
	for (i = 0; i < ARRAY_SIZE(wil->ring_tx); i++) {
		if (wil->ring2cid_tid[i][0] == evt->cid)
			wil_ring_fini_tx(wil, i);
	}

	/* statistics */
	memset(&sta->stats, 0, sizeof(sta->stats));

	mutex_unlock(&wil->mutex);
}

void wil_slave_evt_connect(struct wil6210_vif *vif,
			   int tx_link_id, int rx_link_id,
			   const u8 *mac, u8 cid)
{
	struct wil_slave_entry *slave;
	void *master_ctx;

	slave = wil_get_slave_ctx(vif, &master_ctx);
	if (!slave)
		return;
	slave->rops.connected(master_ctx, tx_link_id, rx_link_id, mac, cid);
}

void wil_slave_evt_disconnect(struct wil6210_vif *vif, u8 cid)
{
	struct wil_slave_entry *slave;
	void *master_ctx;

	slave = wil_get_slave_ctx(vif, &master_ctx);
	if (!slave)
		return;
	slave->rops.disconnected(master_ctx, cid);
}

int wil_slave_rx_data(struct wil6210_vif *vif, u8 cid, struct sk_buff *skb)
{
	struct wil6210_priv *wil = vif_to_wil(vif);
	struct wil_slave_entry *slave;
	void *master_ctx;

	slave = wil_get_slave_ctx(vif, &master_ctx);
	if (unlikely(!slave)) {
		dev_kfree_skb(skb);
		return GRO_DROP;
	}

	/* pass security packets to wireless interface (partial slave mode) */
	if (slave_mode == 2 ||
	    skb->protocol != cpu_to_be16(ETH_P_PAE))
		return slave->rops.rx_data(master_ctx, cid, skb);
	else
		return napi_gro_receive(&wil->napi_rx, skb);
}

const char *wil_slave_get_board_file(struct wil6210_priv *wil)
{
	struct wil6210_vif *vif = ndev_to_vif(wil->main_ndev);
	struct wil_slave_entry *slave;
	void *master_ctx;

	slave = wil_get_slave_ctx(vif, &master_ctx);
	if (!slave)
		return NULL;
	return slave->board_file;
}
