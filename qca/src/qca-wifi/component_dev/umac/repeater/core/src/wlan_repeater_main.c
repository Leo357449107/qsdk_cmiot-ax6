/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * DOC: This file contains main repeater function definitions
 */

#include <wlan_objmgr_peer_obj.h>
#include <wlan_objmgr_global_obj.h>
#include <wlan_mlme_dispatcher.h>
#include <wlan_repeater_internal.h>
#include <wlan_repeater_api.h>
#if ATH_SUPPORT_WRAP
#if !WLAN_QWRAP_LEGACY
#include <dp_wrap.h>
#endif
#endif

#define SKIP_SCAN_ENTRIES 10000

static struct wlan_rptr_global_priv g_rptr_ctx;
static struct wlan_rptr_global_priv *gp_rptr_ctx;

bool
wlan_rptr_is_psta_vdev(struct wlan_objmgr_vdev *vdev)
{
#if ATH_SUPPORT_WRAP
#if WLAN_QWRAP_LEGACY
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;
#else
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
#endif

#if WLAN_QWRAP_LEGACY
	if (ext_cb->get_mpsta_vdev(vdev)) {
		if (!ext_cb->vdev_is_mpsta(vdev) &&
		    ext_cb->vdev_is_psta(vdev)) {
#else
	if (wlan_rptr_pdev_is_qwrap(pdev)) {
		if (!wlan_rptr_vdev_is_mpsta(vdev) &&
		    wlan_rptr_vdev_is_psta(vdev)) {
#endif
			return 1;
		}
	}
#endif
	return 0;
}

#if ATH_SUPPORT_WRAP
/**
 * wlan_rptr_is_psta_bssid_valid– validate PSTA bssid with MPSTA bssid
 * param: vdev- vdev object manager, bssid- bssid mac address
 * Return: return true if valid; otherwise false
 */
QDF_STATUS
wlan_rptr_get_mpsta_bssid(struct wlan_objmgr_vdev *vdev,
			  struct qdf_mac_addr *mpsta_bssid)
{
	struct wlan_objmgr_vdev *mpsta_vdev = NULL;
#if WLAN_QWRAP_LEGACY
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;

	mpsta_vdev = ext_cb->get_mpsta_vdev(vdev);
#else
	mpsta_vdev = dp_wrap_get_mpsta_vdev(wlan_vdev_get_pdev(vdev));
#endif
	if (mpsta_vdev) {
		wlan_vdev_get_bss_peer_mac(mpsta_vdev, mpsta_bssid);
		return QDF_STATUS_SUCCESS;
	}
	return QDF_STATUS_E_INVAL;
}
#endif

struct wlan_rptr_pdev_priv *wlan_rptr_get_pdev_priv(
		struct wlan_objmgr_pdev *pdev)
{
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = (struct wlan_rptr_pdev_priv *)
		wlan_objmgr_pdev_get_comp_private_obj(pdev,
						      WLAN_UMAC_COMP_REPEATER);

	return pdev_priv;
}

struct wlan_rptr_psoc_priv *wlan_rptr_get_psoc_priv(
		struct wlan_objmgr_psoc *psoc)
{
	struct wlan_rptr_psoc_priv *psoc_priv = NULL;

	psoc_priv = (struct wlan_rptr_psoc_priv *)
		wlan_objmgr_psoc_get_comp_private_obj(psoc,
						      WLAN_UMAC_COMP_REPEATER);
	return psoc_priv;
}

struct wlan_rptr_vdev_priv *wlan_rptr_get_vdev_priv(
		struct wlan_objmgr_vdev *vdev)
{
	struct wlan_rptr_vdev_priv *vdev_priv = NULL;

	vdev_priv = (struct wlan_rptr_vdev_priv *)
		wlan_objmgr_vdev_get_comp_private_obj(vdev,
						      WLAN_UMAC_COMP_REPEATER);
	return vdev_priv;
}

struct wlan_rptr_peer_priv *wlan_rptr_get_peer_priv(
		struct wlan_objmgr_peer *peer)
{
	struct wlan_rptr_peer_priv *peer_priv = NULL;

	peer_priv = (struct wlan_rptr_peer_priv *)
		wlan_objmgr_peer_get_comp_private_obj(peer,
						      WLAN_UMAC_COMP_REPEATER);
	return peer_priv;
}

struct wlan_rptr_global_priv *wlan_rptr_get_global_ctx(void)
{
	QDF_BUG(gp_rptr_ctx);

	return gp_rptr_ctx;
}

static
QDF_STATUS wlan_rptr_create_global_ctx(void)
{
	gp_rptr_ctx = &g_rptr_ctx;

	return QDF_STATUS_SUCCESS;
}

static
QDF_STATUS wlan_rptr_destroy_global_ctx(void)
{
	gp_rptr_ctx = NULL;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_rptr_core_register_ext_cb(struct rptr_ext_cbacks *ext_cbacks)
{
	struct wlan_rptr_global_priv *g_priv = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		g_priv->ext_cbacks.vdev_set_powersave =
				ext_cbacks->vdev_set_powersave;
		g_priv->ext_cbacks.vdev_set_param = ext_cbacks->vdev_set_param;
		g_priv->ext_cbacks.vdev_pwrsave_force_sleep =
				ext_cbacks->vdev_pwrsave_force_sleep;
		g_priv->ext_cbacks.send_wds_cmd = ext_cbacks->send_wds_cmd;
		g_priv->ext_cbacks.get_stavap = ext_cbacks->get_stavap;
		g_priv->ext_cbacks.peer_disassoc = ext_cbacks->peer_disassoc;
		g_priv->ext_cbacks.pdev_update_beacon =
				ext_cbacks->pdev_update_beacon;
		g_priv->ext_cbacks.target_lithium = ext_cbacks->target_lithium;
		g_priv->ext_cbacks.dessired_ssid_found =
				ext_cbacks->dessired_ssid_found;
#if DBDC_REPEATER_SUPPORT
		g_priv->ext_cbacks.legacy_dbdc_flags_get =
				ext_cbacks->legacy_dbdc_flags_get;
		g_priv->ext_cbacks.max_pri_stavap_process_up =
				ext_cbacks->max_pri_stavap_process_up;
		g_priv->ext_cbacks.delay_stavap_conn_process_up =
				ext_cbacks->delay_stavap_conn_process_up;
		g_priv->ext_cbacks.dbdc_process_mac_db_up =
				ext_cbacks->dbdc_process_mac_db_up;
		g_priv->ext_cbacks.act_update_force_cli_mcast_process_up =
				ext_cbacks->act_update_force_cli_mcast_process_up;
		g_priv->ext_cbacks.legacy_dbdc_rootap_set =
				ext_cbacks->legacy_dbdc_rootap_set;
		g_priv->ext_cbacks.max_pri_stavap_process_down =
				ext_cbacks->max_pri_stavap_process_down;
		g_priv->ext_cbacks.delay_stavap_conn_process_down =
				ext_cbacks->delay_stavap_conn_process_down;
		g_priv->ext_cbacks.force_cli_mcast_process_down =
				ext_cbacks->force_cli_mcast_process_down;
#endif
		g_priv->ext_cbacks.rptr_send_event = ext_cbacks->rptr_send_event;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
		g_priv->ext_cbacks.vdev_nss_ol_psta_add =
				ext_cbacks->vdev_nss_ol_psta_add;
		g_priv->ext_cbacks.vdev_nss_ol_psta_delete =
				ext_cbacks->vdev_nss_ol_psta_delete;
		g_priv->ext_cbacks.vdev_nss_ol_set_cfg =
				ext_cbacks->vdev_nss_ol_set_cfg;
		g_priv->ext_cbacks.nss_dbdc_process_mac_db_down =
				ext_cbacks->nss_dbdc_process_mac_db_down;
		g_priv->ext_cbacks.nss_prep_mac_db_store_stavap =
				ext_cbacks->nss_prep_mac_db_store_stavap;
#endif
#if ATH_SUPPORT_WRAP
#if WLAN_QWRAP_LEGACY
		g_priv->ext_cbacks.vdev_is_psta = ext_cbacks->vdev_is_psta;
		g_priv->ext_cbacks.vdev_is_mpsta = ext_cbacks->vdev_is_mpsta;
		g_priv->ext_cbacks.get_mpsta_vdev = ext_cbacks->get_mpsta_vdev;
#endif
#endif
		return QDF_STATUS_SUCCESS;
	}

	return QDF_STATUS_E_INVAL;
}

void wlan_rptr_core_vdev_set_feat_cap(struct wlan_objmgr_vdev *vdev,
				      uint32_t cap)
{
	struct wlan_rptr_vdev_priv *vdev_priv = NULL;

	vdev_priv = wlan_rptr_get_vdev_priv(vdev);
	if (vdev_priv)
		vdev_priv->vdev_feature_caps |= cap;
}

void wlan_rptr_core_vdev_clear_feat_cap(struct wlan_objmgr_vdev *vdev,
					uint32_t cap)
{
	struct wlan_rptr_vdev_priv *vdev_priv = NULL;

	vdev_priv = wlan_rptr_get_vdev_priv(vdev);
	if (vdev_priv)
		vdev_priv->vdev_feature_caps &= ~cap;
}

uint8_t wlan_rptr_core_vdev_is_feat_cap_set(struct wlan_objmgr_vdev *vdev,
					 uint32_t cap)
{
	struct wlan_rptr_vdev_priv *vdev_priv = NULL;

	vdev_priv = wlan_rptr_get_vdev_priv(vdev);
	if (vdev_priv)
		return (vdev_priv->vdev_feature_caps & cap) ? 1 : 0;
	return 0;
}

void wlan_rptr_core_pdev_set_feat_cap(struct wlan_objmgr_pdev *pdev,
				      uint32_t cap)
{
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (pdev_priv)
		pdev_priv->pdev_feature_caps |= cap;
}

void wlan_rptr_core_pdev_clear_feat_cap(struct wlan_objmgr_pdev *pdev,
					uint32_t cap)
{
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (pdev_priv)
		pdev_priv->pdev_feature_caps &= ~cap;
}

uint8_t wlan_rptr_core_pdev_is_feat_cap_set(struct wlan_objmgr_pdev *pdev,
					 uint32_t cap)
{
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (pdev_priv)
		return (pdev_priv->pdev_feature_caps & cap) ? 1 : 0;
	return 0;
}

void wlan_rptr_core_psoc_set_feat_cap(struct wlan_objmgr_psoc *psoc,
				      uint32_t cap)
{
	struct wlan_rptr_psoc_priv *psoc_priv = NULL;

	psoc_priv = wlan_rptr_get_psoc_priv(psoc);
	if (psoc_priv)
		psoc_priv->psoc_feature_caps |= cap;
}

void wlan_rptr_core_psoc_clear_feat_cap(struct wlan_objmgr_psoc *psoc,
					uint32_t cap)
{
	struct wlan_rptr_psoc_priv *psoc_priv = NULL;

	psoc_priv = wlan_rptr_get_psoc_priv(psoc);
	if (psoc_priv)
		psoc_priv->psoc_feature_caps &= ~cap;
}

uint8_t wlan_rptr_core_psoc_is_feat_cap_set(struct wlan_objmgr_psoc *psoc,
					 uint32_t cap)
{
	struct wlan_rptr_psoc_priv *psoc_priv = NULL;

	psoc_priv = wlan_rptr_get_psoc_priv(psoc);
	if (psoc_priv)
		return (psoc_priv->psoc_feature_caps & cap) ? 1 : 0;
	return 0;
}

void wlan_rptr_core_global_set_feat_cap(uint32_t cap)
{
	struct wlan_rptr_global_priv *g_priv = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		g_priv->global_feature_caps |= cap;
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

void wlan_rptr_core_global_clear_feat_cap(uint32_t cap)
{
	struct wlan_rptr_global_priv *g_priv = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		g_priv->global_feature_caps &= ~cap;
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

uint8_t wlan_rptr_core_global_is_feat_cap_set(uint32_t cap)
{
	struct wlan_rptr_global_priv *g_priv = NULL;
	u8 retv = 0;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		retv = (g_priv->global_feature_caps & cap) ? 1 : 0;
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
		return retv;
	}
	return 0;
}

void wlan_rptr_core_reset_pdev_flags(struct wlan_objmgr_pdev *pdev)
{
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (pdev_priv)
		pdev_priv->pdev_feature_caps = 0;
}

void wlan_rptr_core_reset_global_flags(void)
{
	struct wlan_rptr_global_priv *g_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	int i;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		g_priv->global_feature_caps = 0;
		g_priv->disconnect_timeout = 10;
		g_priv->reconfiguration_timeout = 60;
		ss_info = &g_priv->ss_info;
		ss_info->same_ssid_disable = 0;
		ss_info->num_rptr_clients = 0;
		ss_info->ap_preference = ap_preference_type_init;
		ss_info->extender_info = 0;
		ss_info->rootap_access_downtime = 0;
		for (i = 0; i < RPTR_MAX_RADIO_CNT; i++) {
			OS_MEMZERO(&ss_info->preferred_bssid_list[i][0],
				   QDF_MAC_ADDR_SIZE);
			OS_MEMZERO(&ss_info->denied_client_list[i][0],
				   QDF_MAC_ADDR_SIZE);
		}
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

static bool
wlan_rptr_dessired_ssid_found(struct wlan_objmgr_vdev *vdev,
			      wlan_scan_entry_t se)
{
	struct wlan_rptr_global_priv *g_priv = NULL;
	struct rptr_ext_cbacks *ext_cb = NULL;
	u8 *se_ssid;
	u8 se_ssid_len;

	g_priv = wlan_rptr_get_global_ctx();
	ext_cb = &g_priv->ext_cbacks;
	se_ssid_len = util_scan_entry_ssid(se)->length;
	se_ssid = util_scan_entry_ssid(se)->ssid;

	if (ext_cb->dessired_ssid_found(vdev, se_ssid, se_ssid_len))
		return 1;

	return 0;
}

QDF_STATUS
wlan_rptr_get_rootap_bssid(void *arg, wlan_scan_entry_t se)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)arg;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	u8 *extender_ie;
	u8 *bssid;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!pdev_priv)
		return QDF_STATUS_SUCCESS;

	if (!IS_NULL_ADDR(pdev_priv->preferred_bssid))
		return QDF_STATUS_SUCCESS;

	bssid = util_scan_entry_bssid(se);

	if (wlan_rptr_dessired_ssid_found(vdev, se)) {
		extender_ie = util_scan_entry_extenderie(se);
		if (!extender_ie) {
			/*When RootAP is present,give priority to RootAP bssid*/
			bssid = util_scan_entry_bssid(se);
			qdf_mem_copy(pdev_priv->preferred_bssid, bssid,
				  QDF_MAC_ADDR_SIZE);
		}
	}
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS
wlan_rptr_process_scan_entries(void *arg, wlan_scan_entry_t se)
{
	struct wlan_objmgr_vdev *vdev = (struct wlan_objmgr_vdev *)arg;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_rptr_global_priv *g_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	u8 *bssid;

	struct ieee80211_ie_extender *extender_ie;
	bool rptr_bssid_found = 0;
	int i;

	g_priv = wlan_rptr_get_global_ctx();
	if (!g_priv)
		return QDF_STATUS_SUCCESS;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);

	if (!IS_NULL_ADDR(pdev_priv->preferred_bssid))
		return QDF_STATUS_SUCCESS;

	bssid = util_scan_entry_bssid(se);

	if (!wlan_rptr_dessired_ssid_found(vdev, se))
		return QDF_STATUS_SUCCESS;

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	extender_ie = (struct ieee80211_ie_extender *)
			util_scan_entry_extenderie(se);
	ss_info = &g_priv->ss_info;
	if ((ss_info->extender_info & ROOTAP_ACCESS_MASK) !=
						ROOTAP_ACCESS_MASK) {
		/* When this RE has no RootAP access*/
		if (extender_ie) {
			/* When 1 STAVAP is connected,
			 * don't allow further connection
			*/
			if (g_priv->num_stavaps_up == 1) {
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
				return QDF_STATUS_SUCCESS;
			}
			if (ss_info->num_rptr_clients &&
			    ((extender_ie->extender_info &
			    STAVAP_CONNECTION_MASK) ==
			    STAVAP_CONNECTION_MASK) &&
			    ((extender_ie->extender_info &
			    ROOTAP_ACCESS_MASK)
			    != ROOTAP_ACCESS_MASK)) {
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
				return QDF_STATUS_SUCCESS;
			}
		}
	} else {
		/* When this RE has RootAP access*/
		if (ss_info->ap_preference == ap_preference_type_root) {
			/*Connect only to RootAP*/
			if (extender_ie) {
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
				return QDF_STATUS_SUCCESS;
			}
		} else if (ss_info->ap_preference == ap_preference_type_rptr) {
			/*Connect to RE whose bssid matches with preferred mac*/
			for (i = 0; i < RPTR_MAX_RADIO_CNT; i++) {
				if (OS_MEMCMP(bssid,
					      &ss_info->preferred_bssid_list[i][0],
				   QDF_MAC_ADDR_SIZE) == 0) {
					rptr_bssid_found = 1;
					break;
				}
			}
			if (!rptr_bssid_found) {
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
				return QDF_STATUS_SUCCESS;
			}
		}
	}
	OS_MEMCPY(pdev_priv->preferred_bssid, bssid, QDF_MAC_ADDR_SIZE);
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	return QDF_STATUS_SUCCESS;
}

void
wlan_rptr_core_ss_parse_scan_entries(struct wlan_objmgr_vdev *vdev,
				     struct scan_event *event)
{
	static u32 last_scanid;
	enum QDF_OPMODE opmode;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_rptr_global_priv *g_priv = NULL;
	struct rptr_ext_cbacks *ext_cb = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;

	g_priv = wlan_rptr_get_global_ctx();
	if (!g_priv)
		return;

	ext_cb = &g_priv->ext_cbacks;
	if ((g_priv->global_feature_caps & wlan_rptr_global_f_s_ssid) &&
	    (event->type == SCAN_EVENT_TYPE_COMPLETED)) {
		opmode = wlan_vdev_mlme_get_opmode(vdev);
		if (opmode == QDF_STA_MODE) {
			if (wlan_rptr_is_psta_vdev(vdev))
				return;

			if (last_scanid == event->scan_id)
				return;

			last_scanid = event->scan_id;
			pdev_priv = wlan_rptr_get_pdev_priv(pdev);
			OS_MEMSET(pdev_priv->preferred_bssid, 0,
				  QDF_MAC_ADDR_SIZE);
			ucfg_scan_db_iterate(pdev, wlan_rptr_get_rootap_bssid,
					     (void *)vdev);
			if (!IS_NULL_ADDR(pdev_priv->preferred_bssid)) {
				RPTR_LOGI("RPTR sending event with preferred RootAP bssid:%s vdev_id:%d\n",
					  ether_sprintf(pdev_priv->preferred_bssid),
					  wlan_vdev_get_id(vdev));
				ext_cb->rptr_send_event(vdev, pdev_priv->preferred_bssid,
							QDF_MAC_ADDR_SIZE, IEEE80211_EV_PREFERRED_BSSID);
			} else {
				RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
				ss_info = &g_priv->ss_info;
				if ((ss_info->extender_info & ROOTAP_ACCESS_MASK) != ROOTAP_ACCESS_MASK) {
					/* When this RE has no RootAP access
					 * Skip scan entries for 10secs to propagate RootAP Access info
					 * across all REs, because FW sending stopped event after 8 secs,
					 * in case of beacon miss.
					 * Ideally, it should be 5 * beacon interval, but beacon miss detection
					 * is taking time, need to debug
					*/
					systime_t current_time = OS_GET_TIMESTAMP();

					if ((CONVERT_SYSTEM_TIME_TO_MS(current_time - ss_info->rootap_access_downtime)) < SKIP_SCAN_ENTRIES) {
						RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
						return;
					}
				}
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
				ucfg_scan_db_iterate(pdev, wlan_rptr_process_scan_entries, (void *)vdev);
				if (!IS_NULL_ADDR(pdev_priv->preferred_bssid)) {
					RPTR_LOGI("RPTR sending event with preferred Repeater bssid:%s vdev_id:%d\n",
						  ether_sprintf(pdev_priv->preferred_bssid),
						  wlan_vdev_get_id(vdev));
					ext_cb->rptr_send_event(vdev, pdev_priv->preferred_bssid,
								QDF_MAC_ADDR_SIZE, IEEE80211_EV_PREFERRED_BSSID);
				}
			}
		}
	}
}

QDF_STATUS
wlan_rptr_core_validate_stavap_bssid(struct wlan_objmgr_vdev *vdev,
				     u8 *bssid)
{
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	enum QDF_OPMODE opmode;
#if ATH_SUPPORT_WRAP
	struct qdf_mac_addr mpsta_bssid;
	u8 retv = QDF_STATUS_SUCCESS;
#endif

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (IS_NULL_ADDR(pdev_priv->preferred_bssid))
		return QDF_STATUS_E_BUSY;

	opmode = wlan_vdev_mlme_get_opmode(vdev);
	if (opmode == QDF_STA_MODE) {
		if (!wlan_rptr_is_psta_vdev(vdev)) {
			if (WLAN_ADDR_EQ(pdev_priv->preferred_bssid,
					 bssid) != QDF_STATUS_SUCCESS) {
				return QDF_STATUS_E_BUSY;
			}
		} else {
#if ATH_SUPPORT_WRAP
			retv = wlan_rptr_get_mpsta_bssid(vdev, &mpsta_bssid);
			if (retv == QDF_STATUS_SUCCESS) {
				if (WLAN_ADDR_EQ(&mpsta_bssid, bssid)
						!= QDF_STATUS_SUCCESS) {
					WLAN_ADDR_COPY(bssid, &mpsta_bssid);
				}
			}
#endif
		}
	}
	return QDF_STATUS_SUCCESS;
}

void
wlan_rptr_core_pdev_pref_uplink_set(struct wlan_objmgr_pdev *pdev,
				    u32 value)
{
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	u8 pdev_id;

	pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (pdev_priv) {
		RPTR_PDEV_LOCK(&pdev_priv->rptr_pdev_lock);
		pdev_priv->preferredUplink = value;
		RPTR_PDEV_UNLOCK(&pdev_priv->rptr_pdev_lock);
	}
	RPTR_LOGI("Preferred uplink set as %d for pdev_id: %d\n",
		  value, pdev_id);
}

void
wlan_rptr_core_pdev_pref_uplink_get(struct wlan_objmgr_pdev *pdev,
				    u32 *val)
{
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (pdev_priv) {
		RPTR_PDEV_LOCK(&pdev_priv->rptr_pdev_lock);
		*val = pdev_priv->preferredUplink;
		RPTR_PDEV_UNLOCK(&pdev_priv->rptr_pdev_lock);
	}
}

void wlan_rptr_core_global_disconnect_timeout_set(u32 value)
{
	struct wlan_rptr_global_priv *g_priv = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		g_priv->disconnect_timeout = (u_int16_t)value;
		RPTR_LOGI("RPTR disconnect_timeout:%d\n",
			  g_priv->disconnect_timeout);
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

void wlan_rptr_core_global_disconnect_timeout_get(u32 *value)
{
	struct wlan_rptr_global_priv *g_priv = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		*value = g_priv->disconnect_timeout;
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

void wlan_rptr_core_global_reconfig_timeout_set(u32 value)
{
	struct wlan_rptr_global_priv *g_priv = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		g_priv->reconfiguration_timeout = (u_int16_t)value;
		RPTR_LOGI("RPTR reconfiguration_timeout:%d\n",
			  g_priv->reconfiguration_timeout);
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

void wlan_rptr_core_global_reconfig_timeout_get(u32 *value)
{
	struct wlan_rptr_global_priv *g_priv = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		*value = g_priv->reconfiguration_timeout;
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

void wlan_rptr_core_global_same_ssid_disable(u32 value)
{
	struct wlan_rptr_global_priv *g_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info = NULL;

	g_priv = wlan_rptr_get_global_ctx();
	if (g_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		ss_info = &g_priv->ss_info;
		ss_info->same_ssid_disable = value;
		if (value) {
			g_priv->global_feature_caps &=
				~wlan_rptr_global_f_s_ssid;
		}
		RPTR_LOGI("RPTR Same ssid disable:%d\n",
			  ss_info->same_ssid_disable);
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}
}

#if ATH_SUPPORT_WRAP
#define ATH_NSCAN_PSTA_VAPS 0
static void
wlan_rptr_vdev_attach(
		struct wlan_objmgr_vdev *vdev)
{
	u32 flags = vdev->vdev_objmgr.c_flags;
	enum QDF_OPMODE opmode;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_rptr_vdev_priv *vdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	vdev_priv = wlan_rptr_get_vdev_priv(vdev);

	if (!pdev_priv || !vdev_priv)
		return;

	opmode = wlan_vdev_mlme_get_opmode(vdev);

	if ((opmode == QDF_SAP_MODE) && (flags & IEEE80211_WRAP_VAP))
		wlan_rptr_vdev_set_wrap(vdev);

	if ((opmode == QDF_STA_MODE) && (flags & IEEE80211_CLONE_MACADDR)) {
		if (!(flags & IEEE80211_WRAP_NON_MAIN_STA)) {
			/*
			 * Main ProxySTA VAP for uplink WPS PBC and
			 * downlink multicast receive.
			 */
			wlan_rptr_vdev_set_mpsta(vdev);
			wlan_rptr_pdev_set_qwrap(pdev);
		} else {
			/*
			 * Generally, non-Main ProxySTA VAP's don't need to
			 * register umac event handlers. We can save some memory
			 * space by doing so. This is required to be done before
			 * ieee80211_vap_setup. However we still give the scan
			 * capability to the first ATH_NSCAN_PSTA_VAPS non-Main
			 * PSTA VAP's. This optimizes the association speed for
			 * the first several PSTA VAP's (common case).
			 */
			if (pdev_priv) {
				if (pdev_priv->nscanpsta >= ATH_NSCAN_PSTA_VAPS)
					wlan_rptr_vdev_set_no_event(vdev);
				else
					pdev_priv->nscanpsta++;
			}
		}
		wlan_rptr_vdev_set_psta(vdev);
	}
	if (flags & IEEE80211_CLONE_MATADDR)
		wlan_rptr_vdev_set_mat(vdev);

	if (flags & IEEE80211_WRAP_WIRED_STA)
		wlan_rptr_vdev_set_wired_psta(vdev);

#if !WLAN_QWRAP_LEGACY
	dp_wrap_vdev_attach(vdev);

	if ((wlan_rptr_vdev_is_wrap(vdev)) ||
	    (wlan_rptr_vdev_is_psta(vdev))) {
		if (dp_wrap_vdev_get_nwrapvaps(pdev))
			wlan_rptr_pdev_set_eir(pdev);
	}
#endif
}

static
void wlan_rptr_vdev_detach(
		struct wlan_objmgr_vdev *vdev)
{
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (pdev_priv) {
		if (wlan_rptr_vdev_is_psta(vdev)) {
			if (!wlan_rptr_vdev_is_mpsta(vdev)) {
				if (wlan_rptr_vdev_is_no_event(vdev) == 0)
					pdev_priv->nscanpsta--;
			}
		}
	}
}
#endif

/**
 * @brief Initialization for Psoc create handler for REPEATER
 *
 * @param [in] psoc , to handle psoc initialization.
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static
QDF_STATUS wlan_repeater_psoc_create_handler(struct wlan_objmgr_psoc *psoc,
					     void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_psoc_priv *psoc_priv;

	if (!psoc) {
		RPTR_LOGF("RPTR Psoc is null Investigate  %s %d ",
			  __func__, __LINE__);
		status = QDF_STATUS_E_INVAL;
		goto failure;
	}

	psoc_priv = qdf_mem_malloc(sizeof(struct wlan_rptr_psoc_priv));

	if (!psoc_priv) {
		RPTR_LOGF("RPTR Insufficient memory can't allocate Repeater psoc");
		status = QDF_STATUS_E_NOMEM;
		goto failure;
	}

	psoc_priv->psoc = psoc;
	if (wlan_objmgr_psoc_component_obj_attach(psoc,
						  WLAN_UMAC_COMP_REPEATER,
						  psoc_priv,
						  QDF_STATUS_SUCCESS)
						  != QDF_STATUS_SUCCESS) {
		RPTR_LOGF("RPTR Failed to attach REPEATER psoc ");
		status = QDF_STATUS_E_FAILURE;
		goto attach_failure;
	}
	return status;

attach_failure:
	qdf_mem_free(psoc_priv);
failure:
	return status;
}

/**
 * @brief Initialization for Pdev create handler for REPEATER
 *
 * @param [in] pdev , to handle pdev initialization.
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static
QDF_STATUS wlan_repeater_pdev_create_handler(struct wlan_objmgr_pdev *pdev,
					     void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_pdev_priv *pdev_priv;

	if (!pdev) {
		RPTR_LOGF("RPTR Pdev is null Investigate  %s %d ",
			  __func__, __LINE__);
		status = QDF_STATUS_E_INVAL;
		goto failure;
	}

	pdev_priv = qdf_mem_malloc(sizeof(struct wlan_rptr_pdev_priv));

	if (!pdev_priv) {
		RPTR_LOGF("RPTR Insufficient memory can not allocate Repeater pdev");
		status = QDF_STATUS_E_NOMEM;
		goto failure;
	}

	pdev_priv->pdev = pdev;
	if (wlan_objmgr_pdev_component_obj_attach(pdev,
						  WLAN_UMAC_COMP_REPEATER,
						  pdev_priv,
						  QDF_STATUS_SUCCESS)
						  != QDF_STATUS_SUCCESS) {
		RPTR_LOGF("RPTR Failed to attach REPEATER pdev ");
		status = QDF_STATUS_E_FAILURE;
		goto attach_failure;
	}
	qdf_spinlock_create(&pdev_priv->rptr_pdev_lock);

	return status;

attach_failure:
	qdf_mem_free(pdev_priv);
failure:
	return status;
}

/**
 * @brief Initialization for Vdev create handler for REPEATER
 *
 * @param [in] vdev , to handle vdev initialization.
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static
QDF_STATUS wlan_repeater_vdev_create_handler(struct wlan_objmgr_vdev *vdev,
					     void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_vdev_priv *vdev_priv;

	if (!vdev) {
		RPTR_LOGF("RPTR Vdev is null Investigate  %s %d ",
			  __func__, __LINE__);
		status = QDF_STATUS_E_INVAL;
		goto failure;
	}

	vdev_priv = qdf_mem_malloc(sizeof(struct wlan_rptr_vdev_priv));

	if (!vdev_priv) {
		RPTR_LOGF("RPTR Insufficient memory can not allocate Repeater vdev");
		status = QDF_STATUS_E_NOMEM;
		goto failure;
	}

	vdev_priv->vdev = vdev;
	if (wlan_objmgr_vdev_component_obj_attach(vdev,
						  WLAN_UMAC_COMP_REPEATER,
						  vdev_priv,
						  QDF_STATUS_SUCCESS)
						  != QDF_STATUS_SUCCESS) {
		RPTR_LOGF("RPTR Failed to attach REPEATER vdev ");
		status = QDF_STATUS_E_FAILURE;
		goto attach_failure;
	}

#if ATH_SUPPORT_WRAP
	wlan_rptr_vdev_attach(vdev);
#endif
	return status;

attach_failure:
	qdf_mem_free(vdev_priv);
failure:
	return status;
}

/**
 * @brief Initialization for Peer create handler for REPEATER
 *
 * @param [in] peer , to handle peer initialization.
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static
QDF_STATUS wlan_repeater_peer_create_handler(struct wlan_objmgr_peer *peer,
					     void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_peer_priv *peer_priv;

	if (!peer) {
		RPTR_LOGF("RPTR Peer is null Investigate  %s %d ",
			  __func__, __LINE__);
		status = QDF_STATUS_E_INVAL;
		goto failure;
	}

	peer_priv = qdf_mem_malloc(sizeof(struct wlan_rptr_peer_priv));

	if (!peer_priv) {
		RPTR_LOGF("RPTR Insufficient memory can not allocate Repeater peer");
		status = QDF_STATUS_E_NOMEM;
		goto failure;
	}

	peer_priv->peer = peer;
	if (wlan_objmgr_peer_component_obj_attach(peer,
						  WLAN_UMAC_COMP_REPEATER,
						  peer_priv, QDF_STATUS_SUCCESS)
						  != QDF_STATUS_SUCCESS) {
		RPTR_LOGF("RPTR Failed to attach REPEATER peer ");
		status = QDF_STATUS_E_FAILURE;
		goto attach_failure;
	}
	return status;

attach_failure:
	qdf_mem_free(peer_priv);
failure:
	return status;
}

/**
 * @brief Initialization for Psoc delete handler for REPEATER
 *
 * @param [in] psoc
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static QDF_STATUS
wlan_repeater_psoc_delete_handler(struct wlan_objmgr_psoc *psoc,
				  void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_psoc_priv *psoc_priv = NULL;

	psoc_priv = wlan_rptr_get_psoc_priv(psoc);

	if (psoc_priv) {
		if (wlan_objmgr_psoc_component_obj_detach(psoc,
							  WLAN_UMAC_COMP_REPEATER,
							  psoc_priv)) {
			status = QDF_STATUS_E_FAILURE;
		}
		qdf_mem_free(psoc_priv);
	}
	return status;
}

/**
 * @brief Initialization for Pdev delete handler for REPEATER
 *
 * @param [in] pdev
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static
QDF_STATUS wlan_repeater_pdev_delete_handler(struct wlan_objmgr_pdev *pdev,
					     void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);

	if (pdev_priv) {
		qdf_spinlock_destroy(&pdev_priv->rptr_pdev_lock);

		if (wlan_objmgr_pdev_component_obj_detach(pdev,
							  WLAN_UMAC_COMP_REPEATER,
							  pdev_priv)) {
			status = QDF_STATUS_E_FAILURE;
		}
		qdf_mem_free(pdev_priv);
	}
	return status;
}

/**
 * @brief Initialization for Vdev delete handler for REPEATER
 *
 * @param [in] vdev
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static
QDF_STATUS wlan_repeater_vdev_delete_handler(struct wlan_objmgr_vdev *vdev,
					     void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_vdev_priv *vdev_priv = NULL;

#if ATH_SUPPORT_WRAP
	wlan_rptr_vdev_detach(vdev);
#endif
	vdev_priv = wlan_rptr_get_vdev_priv(vdev);
	if (vdev_priv) {
		if (wlan_objmgr_vdev_component_obj_detach(vdev,
							  WLAN_UMAC_COMP_REPEATER,
							  vdev_priv)) {
			status = QDF_STATUS_E_FAILURE;
		}
		qdf_mem_free(vdev_priv);
	}
	return status;
}

/**
 * @brief Initialization for peer delete handler for REPEATER
 *
 * @param [in] peer
 *
 * @param [in] arg
 *
 * @return QDF_STATUS_SUCCESS if successfully initialized
 *         else QDF_STATUS_FAIL.
 */

static QDF_STATUS
wlan_repeater_peer_delete_handler(struct wlan_objmgr_peer *peer,
				  void *arg)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_rptr_peer_priv *peer_priv = NULL;
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	wlan_rptr_same_ssid_feature_t   *ss_info;

	if (!g_priv)
		return QDF_STATUS_E_FAILURE;

	peer_priv = wlan_rptr_get_peer_priv(peer);

	if (peer_priv) {
		RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
		ss_info = &g_priv->ss_info;
		if (peer_priv->is_extender_client)
			ss_info->num_rptr_clients--;

		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
		if (wlan_objmgr_peer_component_obj_detach(peer,
							  WLAN_UMAC_COMP_REPEATER,
							  peer_priv)) {
			status = QDF_STATUS_E_FAILURE;
		}
		qdf_mem_free(peer_priv);
	}
	return status;
}

/**
 * @brief
 *
 * @param [in]
 *
 * @return non-zero if it is enabled; otherwise 0
 */

QDF_STATUS wlan_repeater_init(void)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct rptr_ext_cbacks rptr_ext_cbacks;

	status = wlan_rptr_create_global_ctx();
	if (QDF_IS_STATUS_ERROR(status)) {
		RPTR_LOGF("RPTR Failed to create context; status:%d", status);
		return status;
	}
	rptr_ext_cbacks.vdev_set_powersave = wlan_vdev_set_powersave;
	rptr_ext_cbacks.vdev_set_param = wlan_vdev_set_param;
	rptr_ext_cbacks.vdev_pwrsave_force_sleep =
					wlan_vdev_pwrsave_force_sleep;
	rptr_ext_cbacks.send_wds_cmd = wlan_send_wds_cmd;
	rptr_ext_cbacks.get_stavap = wlan_get_stavap;
	rptr_ext_cbacks.target_lithium = wlan_target_lithium;
	rptr_ext_cbacks.peer_disassoc = wlan_peer_disassoc;
	rptr_ext_cbacks.pdev_update_beacon = wlan_pdev_update_beacon;
	rptr_ext_cbacks.dessired_ssid_found = wlan_dessired_ssid_found;
#if DBDC_REPEATER_SUPPORT
	rptr_ext_cbacks.legacy_dbdc_flags_get = wlan_legacy_dbdc_flags_get;
	rptr_ext_cbacks.max_pri_stavap_process_up = wlan_max_pri_stavap_process_up;
	rptr_ext_cbacks.delay_stavap_conn_process_up =
		wlan_delay_stavap_conn_process_up;
	rptr_ext_cbacks.dbdc_process_mac_db_up = wlan_dbdc_process_mac_db_up;
	rptr_ext_cbacks.act_update_force_cli_mcast_process_up =
		wlan_act_update_force_cli_mcast_process_up;
	rptr_ext_cbacks.legacy_dbdc_rootap_set = wlan_legacy_dbdc_rootap_set;
	rptr_ext_cbacks.max_pri_stavap_process_down =
		wlan_max_pri_stavap_process_down;
	rptr_ext_cbacks.delay_stavap_conn_process_down =
		wlan_delay_stavap_conn_process_down;
	rptr_ext_cbacks.force_cli_mcast_process_down =
		wlan_force_cli_mcast_process_down;
#endif
	rptr_ext_cbacks.rptr_send_event = wlan_rptr_send_event;
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
	rptr_ext_cbacks.vdev_nss_ol_psta_add = wlan_vdev_nss_ol_psta_add;
	rptr_ext_cbacks.vdev_nss_ol_psta_delete = wlan_vdev_nss_ol_psta_delete;
	rptr_ext_cbacks.vdev_nss_ol_set_cfg = wlan_vdev_nss_ol_set_cfg;
	rptr_ext_cbacks.nss_dbdc_process_mac_db_down =
		wlan_nss_dbdc_process_mac_db_down;
	rptr_ext_cbacks.nss_prep_mac_db_store_stavap =
		wlan_nss_prep_mac_db_store_stavap;
#endif
#if ATH_SUPPORT_WRAP
#if WLAN_QWRAP_LEGACY
	rptr_ext_cbacks.vdev_is_psta = wlan_vdev_is_psta;
	rptr_ext_cbacks.vdev_is_mpsta = wlan_vdev_is_mpsta;
	rptr_ext_cbacks.get_mpsta_vdev = wlan_get_mpsta_vdev;
#endif
#endif
	wlan_rptr_core_register_ext_cb(&rptr_ext_cbacks);
	qdf_spinlock_create(&gp_rptr_ctx->rptr_global_lock);

	if (wlan_objmgr_register_psoc_create_handler(WLAN_UMAC_COMP_REPEATER,
						     wlan_repeater_psoc_create_handler,
						     NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto psoc_create_failed;
	}

	if (wlan_objmgr_register_pdev_create_handler(WLAN_UMAC_COMP_REPEATER,
						     wlan_repeater_pdev_create_handler,
						     NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto pdev_create_failed;
	}

	if (wlan_objmgr_register_vdev_create_handler(WLAN_UMAC_COMP_REPEATER,
						     wlan_repeater_vdev_create_handler,
						     NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto vdev_create_failed;
	}

	if (wlan_objmgr_register_peer_create_handler(WLAN_UMAC_COMP_REPEATER,
						     wlan_repeater_peer_create_handler,
						     NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto peer_create_failed;
	}

	if (wlan_objmgr_register_psoc_destroy_handler(WLAN_UMAC_COMP_REPEATER,
						      wlan_repeater_psoc_delete_handler,
						      NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto psoc_destroy_failed;
	}

	if (wlan_objmgr_register_pdev_destroy_handler(WLAN_UMAC_COMP_REPEATER,
						      wlan_repeater_pdev_delete_handler,
						      NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto pdev_destroy_failed;
	}

	if (wlan_objmgr_register_vdev_destroy_handler(WLAN_UMAC_COMP_REPEATER,
						      wlan_repeater_vdev_delete_handler,
						      NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto vdev_destroy_failed;
	}

	if (wlan_objmgr_register_peer_destroy_handler(WLAN_UMAC_COMP_REPEATER,
						      wlan_repeater_peer_delete_handler,
						      NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto peer_destroy_failed;
	}

	return status;

peer_destroy_failed:
	if (wlan_objmgr_unregister_vdev_destroy_handler(WLAN_UMAC_COMP_REPEATER,
							wlan_repeater_vdev_delete_handler,
							NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}
vdev_destroy_failed:
	if (wlan_objmgr_unregister_pdev_destroy_handler(WLAN_UMAC_COMP_REPEATER,
							wlan_repeater_pdev_delete_handler,
							NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

pdev_destroy_failed:
	if (wlan_objmgr_unregister_psoc_destroy_handler(WLAN_UMAC_COMP_REPEATER,
							wlan_repeater_psoc_delete_handler,
							NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}
psoc_destroy_failed:
	if (wlan_objmgr_unregister_peer_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_peer_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}
peer_create_failed:
	if (wlan_objmgr_unregister_vdev_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_vdev_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}
vdev_create_failed:
	if (wlan_objmgr_unregister_pdev_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_pdev_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}
pdev_create_failed:
	if (wlan_objmgr_unregister_psoc_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_psoc_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
	}

psoc_create_failed:
	return status;
}

/**
 * @brief
 *
 * @param [in] void.
 *
 * @return Return QDF_STATUS_SUCCESS or QDF_STATUS_FAIL based
 * on condition.
 */

QDF_STATUS wlan_repeater_deinit(void)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	qdf_spinlock_destroy(&gp_rptr_ctx->rptr_global_lock);
	status = wlan_rptr_destroy_global_ctx();
	if (QDF_IS_STATUS_ERROR(status)) {
		RPTR_LOGE("RPTR Failed to destroy context; status:%d", status);
		return status;
	}

	if (wlan_objmgr_unregister_psoc_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_psoc_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}

	if (wlan_objmgr_unregister_pdev_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_pdev_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}

	if (wlan_objmgr_unregister_vdev_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_vdev_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}

	if (wlan_objmgr_unregister_peer_create_handler(WLAN_UMAC_COMP_REPEATER,
						       wlan_repeater_peer_create_handler,
						       NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}

	if (wlan_objmgr_unregister_psoc_destroy_handler(WLAN_UMAC_COMP_REPEATER,
							wlan_repeater_psoc_delete_handler,
							NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}

	if (wlan_objmgr_unregister_pdev_destroy_handler(WLAN_UMAC_COMP_REPEATER,
							wlan_repeater_pdev_delete_handler,
							NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}

	if (wlan_objmgr_unregister_vdev_destroy_handler(WLAN_UMAC_COMP_REPEATER,
							wlan_repeater_vdev_delete_handler,
							NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}

	if (wlan_objmgr_unregister_peer_destroy_handler(WLAN_UMAC_COMP_REPEATER,
							wlan_repeater_peer_delete_handler,
							NULL) != QDF_STATUS_SUCCESS) {
		status = QDF_STATUS_E_FAILURE;
		goto failure;
	}
failure:
	return status;
}

