/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
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

#include <wlan_repeater_api.h>
#include <wlan_repeater_internal.h>
#include <cfg_ucfg_api.h>
#include <dp_extap.h>
#include <wlan_cm_api.h>
#include <wlan_utility.h>
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
#include "osif_nss_wifiol_vdev_if.h"
#endif
#include <wlan_son_pub.h>
#if DBDC_REPEATER_SUPPORT
#include <qca_multi_link_tbl.h>
#include <qca_multi_link.h>
#endif
#if ATH_SUPPORT_WRAP
#if !WLAN_QWRAP_LEGACY
#include <dp_wrap.h>
#endif
#endif

#define IE_CONTENT_SIZE 1

extern bool
wlan_rptr_is_psta_vdev(struct wlan_objmgr_vdev *vdev);

extern QDF_STATUS
wlan_rptr_get_mpsta_bssid(struct wlan_objmgr_vdev *vdev,
			  struct qdf_mac_addr *mpsta_bssid);

struct iterate_info {
	enum wlan_objmgr_obj_type obj_type;
	void *iterate_arg;
	void (*cb_func)(struct wlan_objmgr_psoc *psoc,
			void *obj,
			void *arg);
};

struct s_ssid_info {
	struct wlan_objmgr_vdev *vdev;
	u8 *ssid;
	u8 ssid_len;
	bool son_enabled;
	bool ssid_match;
};

struct extender_msg {
	struct wlan_objmgr_vdev *vdev;
	u8 *frm;
	u8 apvaps_cnt;
	u8 stavaps_cnt;
	enum QDF_OPMODE opmode;
};

struct ext_connection_info {
	bool disconnect_rptr_clients;
	struct wlan_objmgr_vdev *disconnect_sta_vdev;
};

void
wlan_rptr_psoc_iterate_list(struct wlan_objmgr_psoc *psoc,
			    void *msg, uint8_t index)
{
	struct iterate_info *iterate_msg = (struct iterate_info *)msg;

	wlan_objmgr_iterate_obj_list(psoc, iterate_msg->obj_type,
				     iterate_msg->cb_func,
				     iterate_msg->iterate_arg, false, WLAN_MLME_NB_ID);
}

/**
 * wlan_rptr_vdev_set_feat_cap() - set feature caps
 * @vdev: vdev object
 * @cap: capabilities to be set
 *
 * api to set repeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_vdev_set_feat_cap(struct wlan_objmgr_vdev *vdev,
				 uint32_t cap)
{
	wlan_rptr_core_vdev_set_feat_cap(vdev, cap);
}

qdf_export_symbol(wlan_rptr_vdev_set_feat_cap);

/**
 * wlan_rptr_vdev_clear_feat_cap() - clear feature caps
 * @vdev: vdev object
 * @cap: capabilities to be cleared
 *
 * api to clear rapeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_vdev_clear_feat_cap(struct wlan_objmgr_vdev *vdev,
				   uint32_t cap)
{
	wlan_rptr_core_vdev_clear_feat_cap(vdev, cap);
}

qdf_export_symbol(wlan_rptr_vdev_clear_feat_cap);

/**
 * wlan_rptr_vdev_is_feat_cap_set() - get feature caps
 * @vdev: vdev object
 * @cap: capabilities to be checked
 *
 * api to know repeater feature capability is set or not
 *
 * return: 1 if capabilities is  set or else 0
 */
uint8_t wlan_rptr_vdev_is_feat_cap_set(struct wlan_objmgr_vdev *vdev,
				    uint32_t cap)
{
	return wlan_rptr_core_vdev_is_feat_cap_set(vdev, cap);
}

qdf_export_symbol(wlan_rptr_vdev_is_feat_cap_set);

/**
 * wlan_rptr_pdev_set_feat_cap() - set feature caps
 * @pdev: pdev object
 * @cap: capabilities to be set
 *
 * api to set repeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_pdev_set_feat_cap(struct wlan_objmgr_pdev *pdev,
				 uint32_t cap)
{
	wlan_rptr_core_pdev_set_feat_cap(pdev, cap);
}

qdf_export_symbol(wlan_rptr_pdev_set_feat_cap);

/**
 * wlan_rptr_pdev_clear_feat_cap() - clear feature caps
 * @pdev: pdev object
 * @cap: capabilities to be cleared
 *
 * api to clear rapeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_pdev_clear_feat_cap(struct wlan_objmgr_pdev *pdev,
				   uint32_t cap)
{
	wlan_rptr_core_pdev_clear_feat_cap(pdev, cap);
}

qdf_export_symbol(wlan_rptr_pdev_clear_feat_cap);

/**
 * wlan_rptr_pdev_is_feat_cap_set() - get feature caps
 * @vdev: pdev object
 * @cap: capabilities to be checked
 *
 * api to know repeater feature capability is set or not
 *
 * return: 1 if capabilities is  set or else 0
 */
uint8_t wlan_rptr_pdev_is_feat_cap_set(struct wlan_objmgr_pdev *pdev,
				    uint32_t cap)
{
	return wlan_rptr_core_pdev_is_feat_cap_set(pdev, cap);
}

qdf_export_symbol(wlan_rptr_pdev_is_feat_cap_set);

/**
 * wlan_rptr_psoc_set_feat_cap() - set feature caps
 * @psoc: psoc object
 * @cap: capabilities to be set
 *
 * api to set repeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_psoc_set_feat_cap(struct wlan_objmgr_psoc *psoc,
				 uint32_t cap)
{
	wlan_rptr_core_psoc_set_feat_cap(psoc, cap);
}

qdf_export_symbol(wlan_rptr_psoc_set_feat_cap);

/**
 * wlan_rptr_psoc_clear_feat_cap() - clear feature caps
 * @pdev: psoc object
 * @cap: capabilities to be cleared
 *
 * api to clear rapeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_psoc_clear_feat_cap(struct wlan_objmgr_psoc *psoc,
				   uint32_t cap)
{
	wlan_rptr_core_psoc_clear_feat_cap(psoc, cap);
}

qdf_export_symbol(wlan_rptr_psoc_clear_feat_cap);

/**
 * wlan_rptr_psoc_is_feat_cap_set() - get feature caps
 * @vdev: psoc object
 * @cap: capabilities to be checked
 *
 * api to know repeater feature capability is set or not
 *
 * return: 1 if capabilities is  set or else 0
 */
uint8_t wlan_rptr_psoc_is_feat_cap_set(struct wlan_objmgr_psoc *psoc,
				    uint32_t cap)
{
	return wlan_rptr_core_psoc_is_feat_cap_set(psoc, cap);
}

qdf_export_symbol(wlan_rptr_psoc_is_feat_cap_set);

/**
 * wlan_rptr_global_set_feat_cap() - set feature caps
 * @cap: capabilities to be set
 *
 * api to set repeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_global_set_feat_cap(uint32_t cap)
{
	wlan_rptr_core_global_set_feat_cap(cap);
}

qdf_export_symbol(wlan_rptr_global_set_feat_cap);

/**
 * wlan_rptr_global_clear_feat_cap() - clear feature caps
 * @cap: capabilities to be cleared
 *
 * api to clear rapeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_global_clear_feat_cap(uint32_t cap)
{
	wlan_rptr_core_global_clear_feat_cap(cap);
}

qdf_export_symbol(wlan_rptr_global_clear_feat_cap);

/**
 * wlan_rptr_global_is_feat_cap_set() - get feature caps
 * @cap: capabilities to be checked
 *
 * api to know repeater feature capability is set or not
 *
 * return: 1 if capabilities is  set or else 0
 */
uint8_t wlan_rptr_global_is_feat_cap_set(uint32_t cap)
{
	return wlan_rptr_core_global_is_feat_cap_set(cap);
}

qdf_export_symbol(wlan_rptr_global_is_feat_cap_set);

/**
 * wlan_rptr_reset_flags() - reset global flags
 *
 * api to reset repeater flags
 *
 * return: void
 */
void wlan_rptr_reset_flags(struct wlan_objmgr_pdev *pdev)
{
	wlan_rptr_core_reset_pdev_flags(pdev);
	wlan_rptr_core_reset_global_flags();
}

qdf_export_symbol(wlan_rptr_reset_flags);

void
wlan_rptr_peer_disconnect_cb(struct wlan_objmgr_psoc *psoc,
			     void *obj, void *arg)
{
	struct wlan_objmgr_peer *peer;
	struct wlan_rptr_peer_priv *peer_priv = NULL;
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;

	peer = (struct wlan_objmgr_peer *)obj;

	if (!peer)
		return;

	peer_priv = wlan_rptr_get_peer_priv(peer);
	if (!peer_priv)
		return;

	if (!g_priv)
		return;

	if (peer_priv->is_extender_client)
		ext_cb->peer_disassoc(peer);
}

void
wlan_rptr_pdev_update_beacon_cb(struct wlan_objmgr_psoc *psoc,
				void *obj, void *arg)
{
	struct wlan_objmgr_pdev *pdev;
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;

	pdev = (struct wlan_objmgr_pdev *)obj;

	if (!pdev)
		return;

	if (!g_priv)
		return;

	ext_cb->pdev_update_beacon(pdev);
}

/**
 * wlan_rptr_vdev_set_params() - Set vdev params
 * @vdev: vdev object manager
 * api to set vdev params
 *
 * return: void
 */
void wlan_rptr_vdev_set_params(struct wlan_objmgr_vdev *vdev)
{
	/*disable DA war & dp_lag param not set*/
	cdp_config_param_type val = {0};
	enum QDF_OPMODE opmode;
	u8 vdev_id;
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_psoc *psoc = NULL;
	bool psta, mpsta, wrap;

	psoc = wlan_vdev_get_psoc(vdev);
	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	opmode = wlan_vdev_mlme_get_opmode(vdev);
	vdev_id = wlan_vdev_get_id(vdev);
	wlan_rptr_vdev_get_qwrap_cflags(vdev, &psta, &mpsta, &wrap);

#if ATH_SUPPORT_WRAP
#if !WLAN_QWRAP_LEGACY
	if (psta) {
		val.cdp_vdev_param_proxysta = 1;
		cdp_txrx_set_vdev_param(soc_txrx_handle, vdev_id,
					CDP_ENABLE_PROXYSTA, val);
	}
	if (psta && dp_wrap_pdev_get_isolation(
						wlan_vdev_get_pdev(vdev))) {
		struct wlan_objmgr_vdev *mpsta_vdev = dp_wrap_get_mpsta_vdev(
						wlan_vdev_get_pdev(vdev));

		val.cdp_vdev_param_qwrap_isolation = 1;
		cdp_txrx_set_vdev_param(soc_txrx_handle,
					wlan_vdev_get_id(vdev),
					CDP_ENABLE_QWRAP_ISOLATION, val);
		if (mpsta_vdev) {
			cdp_txrx_set_vdev_param(soc_txrx_handle,
						wlan_vdev_get_id(mpsta_vdev),
						CDP_ENABLE_QWRAP_ISOLATION, val);
		}
	}
#endif
#endif
	if (opmode == QDF_SAP_MODE) {
		if (wrap) {
			val.cdp_vdev_param_wds = 0;
			cdp_txrx_set_vdev_param(soc_txrx_handle,
						wlan_vdev_get_id(vdev),
						CDP_ENABLE_WDS, val);
		} else {
			val.cdp_vdev_param_wds = 1;
			cdp_txrx_set_vdev_param(soc_txrx_handle,
						wlan_vdev_get_id(vdev),
						CDP_ENABLE_WDS, val);
		}
	}
	if (opmode == QDF_STA_MODE) {
		if (psta) {
			val.cdp_vdev_param_wds = 0;
			cdp_txrx_set_vdev_param(soc_txrx_handle,
						wlan_vdev_get_id(vdev),
						CDP_ENABLE_WDS, val);
		} else {
			val.cdp_vdev_param_wds = 1;
			cdp_txrx_set_vdev_param(soc_txrx_handle,
						wlan_vdev_get_id(vdev),
						CDP_ENABLE_WDS, val);
		}
	}
}

qdf_export_symbol(wlan_rptr_vdev_set_params);

/**
 * wlan_rptr_vdev_create_complete() - Set vdev params on vdev create
 * complete
 * @vdev: vdev object manager
 * @dev: net device
 * api to set vdev params on vdev create complete
 *
 * return: void
 */
#if ATH_SUPPORT_WRAP
void
wlan_rptr_vdev_create_complete(struct wlan_objmgr_vdev *vdev,
			       struct net_device *dev)
{
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_rptr_vdev_priv *vdev_priv = NULL;
#if !WLAN_QWRAP_LEGACY
#if DBDC_REPEATER_SUPPORT
	u8 *oma_addr = NULL;
	u8 *vma_addr = NULL;
	struct net_device *wrap_dev;
#endif
#endif

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	vdev_priv = wlan_rptr_get_vdev_priv(vdev);
	if (!pdev_priv || !vdev_priv)
		return;

#if !WLAN_QWRAP_LEGACY
	if (wlan_rptr_vdev_is_wrap(vdev) || wlan_rptr_vdev_is_psta(vdev))
		dp_wrap_vdev_set_netdev(vdev, dev);
#endif
	if (wlan_rptr_vdev_is_psta(vdev)) {
#if !WLAN_QWRAP_LEGACY
		dp_wrap_dev_add(vdev);
#if DBDC_REPEATER_SUPPORT
		if (dp_wrap_get_vdev(pdev) && !wlan_rptr_vdev_is_mpsta(vdev) &&
		    !wlan_rptr_vdev_is_wired_psta(vdev)) {
			wrap_dev = dp_wrap_vdev_get_netdev(
					dp_wrap_get_vdev(pdev));
			if (wrap_dev) {
				oma_addr = dp_wrap_vdev_get_oma(vdev);
				vma_addr = dp_wrap_vdev_get_vma(vdev);
				if (wrap_dev && oma_addr && vma_addr) {
					qca_multi_link_tbl_add_or_refresh_entry
						(wrap_dev,
						 vma_addr,
						 QCA_MULTI_LINK_ENTRY_STATIC);
				}
			}
		}
#endif
#endif

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
		ext_cb->vdev_nss_ol_psta_add(vdev);
#endif
		ext_cb->vdev_set_powersave(vdev, IEEE80211_PWRSAVE_NONE);
	}
	RPTR_LOGI("RPTR vdev flags:%x pdev flags:%x vdev_id:%d pdev_id:%d\n",
		  vdev_priv->vdev_feature_caps, pdev_priv->pdev_feature_caps,
		  wlan_vdev_get_id(vdev), wlan_objmgr_pdev_get_pdev_id(pdev));
}

/**
 * wlan_rptr_vdev_delete_start() - repeater code on vdev delete
 * @vdev: vdev object manager
 * api to execute repeater code on vdev delete
 *
 * return: void
 */
void
wlan_rptr_vdev_delete_start(struct wlan_objmgr_vdev *vdev)
{
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;
#endif
	enum QDF_OPMODE opmode;
#if !WLAN_QWRAP_LEGACY
#if DBDC_REPEATER_SUPPORT
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	u8 *oma_addr = NULL;
	u8 *vma_addr = NULL;
	struct net_device *wrap_dev;
#endif
#endif

	opmode = wlan_vdev_mlme_get_opmode(vdev);
	if (wlan_rptr_vdev_is_psta(vdev)) {
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
		ext_cb->vdev_nss_ol_psta_delete(vdev);
#endif
#if !WLAN_QWRAP_LEGACY
#if DBDC_REPEATER_SUPPORT
		if (dp_wrap_get_vdev(pdev) && !wlan_rptr_vdev_is_mpsta(vdev) &&
		    !wlan_rptr_vdev_is_wired_psta(vdev)) {
			wrap_dev = dp_wrap_vdev_get_netdev(
						dp_wrap_get_vdev(pdev));
			if (wrap_dev) {
				oma_addr = dp_wrap_vdev_get_oma(vdev);
				vma_addr = dp_wrap_vdev_get_vma(vdev);
				if (wrap_dev && oma_addr && vma_addr) {
					qca_multi_link_tbl_delete_entry(
							wrap_dev, vma_addr);
				}
			}
		}
#endif
		dp_wrap_dev_remove(vdev);
		dp_wrap_dev_remove_vma(vdev);
#endif
	}
	if ((opmode == QDF_STA_MODE) && wlan_rptr_vdev_is_extap(vdev)) {
		dp_extap_disable(vdev);
		dp_extap_mitbl_purge(dp_get_extap_handle(vdev));
	}
}
#endif

#if ATH_SUPPORT_WRAP

/**
 * wlan_rptr_vdev_is_key_set_allowed - check if key set is allowed for vdev
 * @vdev- vdev object manager
 * @flags – key flags
 * return true if valid; otherwise false
 */
bool
wlan_rptr_vdev_is_key_set_allowed(struct wlan_objmgr_vdev *vdev, uint16_t flags)
{
#if WLAN_QWRAP_LEGACY
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;

	if (ext_cb->vdev_is_psta(vdev) && !(ext_cb->vdev_is_mpsta(vdev)) &&
	    (flags & WLAN_CRYPTO_KEY_GROUP)) {
#else
		if (wlan_rptr_vdev_is_psta(vdev) && !(wlan_rptr_vdev_is_mpsta
		    (vdev)) && (flags & WLAN_CRYPTO_KEY_GROUP)) {
#endif
			return 0;
		}
	return 1;
}

qdf_export_symbol(wlan_rptr_vdev_is_key_set_allowed);
#endif

/**
 * wlan_rptr_vdev_ucfg_config - API for repeater vdev configuration
 * @vdev- vdev object manager
 * @param- config param
 * @value – config value
 * Return: QDF_STATUS_SUCCESS - for success and non-zero for failure
 */
QDF_STATUS
wlan_rptr_vdev_ucfg_config(struct wlan_objmgr_vdev *vdev, int param,
			   u8 value)
{
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;
	enum QDF_OPMODE opmode;
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	int retv = QDF_STATUS_SUCCESS;
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_psoc *psoc = NULL;
	u8 vdev_id;
	cdp_config_param_type val = {0};

	psoc = wlan_vdev_get_psoc(vdev);
	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	vdev_id = wlan_vdev_get_id(vdev);

	opmode = wlan_vdev_mlme_get_opmode(vdev);
	switch (param) {
	case IEEE80211_PARAM_EXTAP:
#ifdef QCA_SUPPORT_WDS_EXTENDED
		if (wlan_psoc_nif_feat_cap_get(wlan_vdev_get_psoc(vdev),
					       WLAN_SOC_F_WDS_EXTENDED)) {
			RPTR_LOGE("RPTR EXTAP can't coexist with WDSEXT mode");
			break;
		}
#endif
		if (value) {
			if (value == 3 /* dbg */) {
				dp_extap_mitbl_dump(dp_get_extap_handle
						    (vdev));
				break;
			}
			if (value == 2 /* dbg */) {
				dp_extap_disable(vdev);
				dp_extap_mitbl_purge(dp_get_extap_handle
						     (vdev));
			}
			dp_extap_enable(vdev);
			/*
			 * Set the auto assoc feature for Ext Station
			*/
			ext_cb->vdev_set_param(vdev,
					       IEEE80211_AUTO_ASSOC, 1);
			/*
			 * DISABLE DA LEARN at SOC level if extap is
			 *  enabled on any VAP
			*/
			val.cdp_vdev_param_da_war = 0;
			cdp_txrx_set_vdev_param(soc_txrx_handle,
						vdev_id,
						CDP_ENABLE_DA_WAR, val);
			if (opmode == QDF_STA_MODE) {
				ext_cb->vdev_set_powersave(vdev,
							   IEEE80211_PWRSAVE_NONE);
				ext_cb->vdev_pwrsave_force_sleep(vdev, 0);
			/*
			 * Enable EIR mode for EXTAP
			*/
				wlan_rptr_pdev_set_eir(pdev);
			}
		} else {
			dp_extap_disable(vdev);
			val.cdp_vdev_param_da_war = 1;
			cdp_txrx_set_vdev_param(soc_txrx_handle,
						vdev_id,
						CDP_ENABLE_DA_WAR, val);
			if (opmode == QDF_STA_MODE)
				wlan_rptr_pdev_clear_eir(pdev);
		}
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
		ext_cb->vdev_nss_ol_set_cfg(vdev,
					    OSIF_NSS_VDEV_EXTAP_CONFIG);
#endif
		break;
	case IEEE80211_PARAM_WDS:
		if (value) {
			wlan_rptr_vdev_set_wds(vdev);
			ext_cb->vdev_set_param(vdev,
					       IEEE80211_AUTO_ASSOC, 1);
			if (opmode == QDF_STA_MODE) {
				ext_cb->vdev_set_powersave(vdev, IEEE80211_PWRSAVE_NONE);
				ext_cb->vdev_pwrsave_force_sleep(vdev, 0);
			}
		} else {
			wlan_rptr_vdev_clear_wds(vdev);
		}

		retv = ext_cb->send_wds_cmd(vdev, value);
		if (retv == EOK) {
			/*
			 * For AP mode, keep WDS always enabled
			 */
			if (opmode != QDF_SAP_MODE) {
				cdp_config_param_type val = {0};

				val.cdp_vdev_param_wds = value;
				cdp_txrx_set_vdev_param(soc_txrx_handle, vdev_id,
								CDP_ENABLE_WDS, val);
				val.cdp_vdev_param_mec = value;
				cdp_txrx_set_vdev_param(soc_txrx_handle, vdev_id,
								CDP_ENABLE_MEC, val);

				/* DA_WAR is enabled by default
				 * within DP in AP mode,
				 * for Hawkeye v1.x
				 */
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
				ext_cb->vdev_nss_ol_set_cfg(vdev,
							    OSIF_NSS_VDEV_WDS_CFG);
#endif
			}
		}
		break;
	case IEEE80211_PARAM_VAP_ENHIND:
		if (value)
			wlan_rptr_pdev_set_eir(pdev);
		else
			wlan_rptr_pdev_clear_eir(pdev);
		break;
	}
	return retv;
}

/**
 * wlan_rptr_vdev_get_qwrap_cflags - get qwrap cflags value
 * param: vdev: vdev object mgr
 * psta: psta var ptr
 * mpsta: mpsta var ptr
 * wrap: wrap var ptr
 * ret: void
 */
void
wlan_rptr_vdev_get_qwrap_cflags(struct wlan_objmgr_vdev *vdev, bool *psta,
				bool *mpsta, bool *wrap)
{
	u32 flags;
	enum QDF_OPMODE opmode;

	if (!vdev) {
		RPTR_LOGE("VDEV is null");
		return;
	}

	flags = vdev->vdev_objmgr.c_flags;
	opmode = wlan_vdev_mlme_get_opmode(vdev);

	*psta = ((opmode == QDF_STA_MODE) &&
			(flags & IEEE80211_CLONE_MACADDR)) ? 1 : 0;

	*mpsta = ((opmode == QDF_STA_MODE) &&
			(flags & IEEE80211_CLONE_MACADDR) &&
			!(flags & IEEE80211_WRAP_NON_MAIN_STA)) ? 1 : 0;

	*wrap = ((opmode == QDF_SAP_MODE) &&
			(flags & IEEE80211_WRAP_VAP)) ? 1 : 0;
}
qdf_export_symbol(wlan_rptr_vdev_get_qwrap_cflags);

void
wlan_rptr_pdev_pref_uplink_set_cb(struct wlan_objmgr_psoc *psoc,
		void *obj,
		void *arg)
{
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_objmgr_pdev *tmp_pdev = NULL;

	tmp_pdev = (struct wlan_objmgr_pdev *)obj;
	if (!tmp_pdev) {
		RPTR_LOGE("PDEV is null");
		return;
	}

	pdev = (struct wlan_objmgr_pdev *)arg;

	if (pdev == tmp_pdev) {
		wlan_rptr_core_pdev_pref_uplink_set(tmp_pdev, 1);
	} else {
		wlan_rptr_core_pdev_pref_uplink_set(tmp_pdev, 0);
	}
	return;
}

/**
 * wlan_rptr_pdev_ucfg_config_set - NB call into RE component during set
 * param: pdev: pdev object manager
 * param: feature specific param
 * value: set value
 * ret: QDF_STATUS_SUCCESS in case of success and non-zero in case of failure
 */
QDF_STATUS
wlan_rptr_pdev_ucfg_config_set(struct wlan_objmgr_pdev *pdev,
			       int param, u8 value)
{
	struct wlan_rptr_global_priv *g_priv = NULL;
	struct rptr_ext_cbacks *ext_cb = NULL;
	struct wlan_objmgr_vdev *sta_vdev = NULL;
	struct iterate_info iterate_msg;
	int retv = QDF_STATUS_SUCCESS;
#if DBDC_REPEATER_SUPPORT
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_psoc *psoc = NULL;
	uint8_t pdev_id;
	cdp_config_param_type val = {0};
	struct pdev_osif_priv *pdev_ospriv = NULL;
	struct wiphy *wiphy = NULL;
	struct wiphy *primary_wiphy = NULL;
#endif

	g_priv = wlan_rptr_get_global_ctx();
	ext_cb = &g_priv->ext_cbacks;
#if DBDC_REPEATER_SUPPORT
	psoc = wlan_pdev_get_psoc(pdev);
	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	pdev_id = wlan_objmgr_pdev_get_pdev_id(pdev);
	pdev_ospriv = wlan_pdev_get_ospriv(pdev);
	wiphy = pdev_ospriv->wiphy;
#endif

	switch (param) {
#if DBDC_REPEATER_SUPPORT
	case OL_ATH_PARAM_PRIMARY_RADIO:
		if (wiphy) {
			if (value) {
				qca_multi_link_set_primary_radio(wiphy);
			} else {
				primary_wiphy = qca_multi_link_get_primary_radio();
				if (primary_wiphy == wiphy)
					qca_multi_link_set_primary_radio(NULL);
			}
		}
		RPTR_LOGI("multi_link set primary radio flag as %d for pdev_id: %d\n",
			  value, pdev_id);

		val.cdp_pdev_param_primary_radio = value;
		cdp_txrx_set_pdev_param(soc_txrx_handle, pdev_id,
					CDP_CONFIG_PRIMARY_RADIO, val);
		break;
	case OL_ATH_PARAM_DBDC_ENABLE:
		if (value)
			qca_multi_link_set_dbdc_enable(true);
		else
			qca_multi_link_set_dbdc_enable(false);

		RPTR_LOGI("multi_link set dbdc_enable flag as %d\n", value);
		break;
	case OL_ATH_PARAM_CLIENT_MCAST:
		if (value)
			qca_multi_link_set_force_client_mcast(true);
		else
			qca_multi_link_set_force_client_mcast(false);

		RPTR_LOGI("multi_link set force_client_mcast flag as %d\n",
			  value);
		break;
#endif
	case OL_ATH_PARAM_ALWAYS_PRIMARY:
#if DBDC_REPEATER_SUPPORT
		if (value)
			qca_multi_link_set_always_primary(true);
		else
			qca_multi_link_set_always_primary(false);

		RPTR_LOGI("multi_link set always primary flag as %d\n", value);
#endif
		break;
	case OL_ATH_PARAM_FAST_LANE:
#if DBDC_REPEATER_SUPPORT
		if (value)
			qca_multi_link_add_fastlane_radio(wiphy);
		else
			qca_multi_link_remove_fastlane_radio(wiphy);

		RPTR_LOGI("multi_link set fastlane flag as %d for pdev_id: %d\n",
			  value, pdev_id);
#endif
		break;
	case OL_ATH_PARAM_PREFERRED_UPLINK:
		sta_vdev = ext_cb->get_stavap(pdev);
		if (!sta_vdev) {
			RPTR_LOGI("Radio not configured on repeater mode");
			retv = QDF_STATUS_E_INVAL;
			break;
		}

		if (value != 1) {
			RPTR_LOGI("Value should be given as 1 to set as preferred uplink");
			retv = QDF_STATUS_E_INVAL;
			break;
		}

		iterate_msg.obj_type = WLAN_PDEV_OP;
		iterate_msg.iterate_arg = pdev;
		iterate_msg.cb_func = wlan_rptr_pdev_pref_uplink_set_cb;

		wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
					      &iterate_msg, WLAN_MLME_NB_ID);
		break;
	case OL_ATH_PARAM_SAME_SSID_DISABLE:
		wlan_rptr_core_global_same_ssid_disable(value);
		break;
	case OL_ATH_PARAM_DISCONNECTION_TIMEOUT:
		wlan_rptr_core_global_disconnect_timeout_set(value);
		break;
	case OL_ATH_PARAM_RECONFIGURATION_TIMEOUT:
		wlan_rptr_core_global_reconfig_timeout_set(value);
		break;
	}
	return retv;
}

qdf_export_symbol(wlan_rptr_pdev_ucfg_config_set);

/**
 * wlan_rptr_pdev_ucfg_config_get - NB call into RE component during get
 * param: pdev: pdev object manager
 * param: feature specific param
 * ret: value
 */
uint32_t
wlan_rptr_pdev_ucfg_config_get(struct wlan_objmgr_pdev *pdev, int param)
{
	u32 value;

	switch (param) {
	case OL_ATH_PARAM_PREFERRED_UPLINK:
		wlan_rptr_core_pdev_pref_uplink_get(pdev, &value);
		break;
	case OL_ATH_PARAM_DISCONNECTION_TIMEOUT:
		wlan_rptr_core_global_disconnect_timeout_get(&value);
		break;
	case OL_ATH_PARAM_RECONFIGURATION_TIMEOUT:
		wlan_rptr_core_global_reconfig_timeout_get(&value);
		break;
	}
	return value;
}
qdf_export_symbol(wlan_rptr_pdev_ucfg_config_get);

/**
 * wlan_rptr_vdev_is_scan_allowed - check if scan is allowed for vdev
 * @vdev- vdev object manager
 * return true if valid; otherwise false
 */
bool wlan_rptr_vdev_is_scan_allowed(struct wlan_objmgr_vdev *vdev)
{
	if (wlan_rptr_vdev_is_no_event(vdev))
		return 0;

	RPTR_LOGD("RPTR scan allowed on vdev:%d\n", wlan_vdev_get_id(vdev));
	return 1;
}

#if ATH_SUPPORT_WRAP
/**
 * wlan_rptr_set_psta_bssid_filter - set bssid filter for PSTA
 * @vdev- vdev object manager
 * @des_bssid_set - flag for bssid set
 * @filter - scan filter
 * return QDF_STATUS_SUCCESS if bssid set; otherwise non-zero value
 */
QDF_STATUS
wlan_rptr_set_psta_bssid_filter(struct wlan_objmgr_vdev *vdev,
				bool *des_bssid_set, struct scan_filter *filter)
{
#if WLAN_QWRAP_LEGACY
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;
#endif
	struct qdf_mac_addr mpsta_bssid;
	int retv = QDF_STATUS_SUCCESS;
#if WLAN_QWRAP_LEGACY
	if (ext_cb->vdev_is_psta(vdev) && !ext_cb->vdev_is_mpsta(vdev)) {
#else
	if (wlan_rptr_vdev_is_psta(vdev) && !wlan_rptr_vdev_is_mpsta(vdev)) {
#endif
		retv = wlan_rptr_get_mpsta_bssid(vdev, &mpsta_bssid);
		if (retv == QDF_STATUS_SUCCESS) {
			/* For PSTA, desired BSSID should be MPSTA BSSID */
			filter->num_of_bssid = 1;
			qdf_mem_copy(&filter->bssid_list[0], mpsta_bssid.bytes,
				     QDF_NET_ETH_LEN);
			*des_bssid_set = true;
		} else {
			qdf_mem_free(filter);
			RPTR_LOGE("RPTR WRAP mode, staVap not available");
			return QDF_STATUS_E_INVAL;
		}
	}
	return QDF_STATUS_SUCCESS;
}

/**
 * wlan_rptr_psta_validate_chan - validate psta channel
 * @vdev- vdev object manager
 * @freq - PSTA freq
 * return QDF_STATUS_SUCCESS if channel is correct; otherwise non-zero value
 */
QDF_STATUS
wlan_rptr_psta_validate_chan(struct wlan_objmgr_vdev *vdev, uint16_t freq)
{
	struct wlan_objmgr_vdev *mpsta_vdev = NULL;
	struct wlan_channel *chan;
	u16 mpsta_freq;
#if WLAN_QWRAP_LEGACY
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;

	if (ext_cb->vdev_is_psta(vdev)) {
		if (ext_cb->vdev_is_mpsta(vdev)) {
#else
	if (wlan_rptr_vdev_is_psta(vdev)) {
		if (wlan_rptr_vdev_is_mpsta(vdev)) {
#endif
			wlan_cm_set_max_connect_attempts(vdev, 1);
		} else {
#if WLAN_QWRAP_LEGACY
			mpsta_vdev = ext_cb->get_mpsta_vdev(vdev);
#else
			mpsta_vdev = dp_wrap_get_mpsta_vdev(wlan_vdev_get_pdev(
									vdev));
#endif
			if (mpsta_vdev) {
				chan = wlan_vdev_mlme_get_bss_chan(mpsta_vdev);
				mpsta_freq = chan->ch_freq;
			}
			if (mpsta_freq != freq) {
				RPTR_LOGE("RPTR psta freq %d mpsta_freq %d not matching\n",
					  freq, mpsta_freq);
				return QDF_STATUS_E_FAILURE;
			}
		}
	}
	return QDF_STATUS_SUCCESS;
}
#endif

/**
 * wlan_rptr_get_qwrap_vdevs_for_pdev_id() - Get total number of qwrap vdevs
 * from cfg private context of psoc for given pdev id.
 * @psoc: PSOC object
 * @pdev_id : Pdev id
 *
 * This API can be called only before WMI_INIT_CMDID sent from host to FW.
 * After WMI_READY_EVENTID, call either wlan_pdev_get_max_vdev_count()/
 * wlan_psoc_get_max_vdev_count().
 *
 * Return: Number of qwrap vdevs for given pdev_id in max client mode
 */
uint8_t wlan_rptr_get_qwrap_vdevs_for_pdev_id(struct wlan_objmgr_psoc *psoc,
					      uint16_t pdev_id)
{
	switch (pdev_id) {
	case 0:
		return cfg_get(psoc, CFG_OL_QWRAP_VDEVS_PDEV0);
	case 1:
		return cfg_get(psoc, CFG_OL_QWRAP_VDEVS_PDEV1);
	case 2:
		return cfg_get(psoc, CFG_OL_QWRAP_VDEVS_PDEV2);
	default:
		return 0;
	}
}

qdf_export_symbol(wlan_rptr_get_qwrap_vdevs_for_pdev_id);

/**
 * wlan_rptr_get_qwrap_peers_for_pdev_id() - Get total number of qwrap peers
 * from cfg private context of psoc for given pdev id.
 * @psoc: PSOC object
 * @pdev_id : Pdev id
 *
 * This API can be called only before WMI_INIT_CMDID sent from host to FW.
 * After WMI_READY_EVENTID, call either wlan_pdev_get_max_vdev_count()/
 * wlan_psoc_get_max_vdev_count().
 *
 * Return: Number of qwrap peers for given pdev_id in max client mode
 */

uint8_t wlan_rptr_get_qwrap_peers_for_pdev_id(struct wlan_objmgr_psoc *psoc,
					      uint16_t pdev_id)
{
	switch (pdev_id) {
	case 0:
		return cfg_get(psoc, CFG_OL_QWRAP_PEERS_PDEV0);
	case 1:
		return cfg_get(psoc, CFG_OL_QWRAP_PEERS_PDEV1);
	case 2:
		return cfg_get(psoc, CFG_OL_QWRAP_PEERS_PDEV2);
	default:
		return 0;
	}
}

qdf_export_symbol(wlan_rptr_get_qwrap_peers_for_pdev_id);

#if DBDC_REPEATER_SUPPORT
void
wlan_rptr_conn_up_dbdc_process(struct wlan_objmgr_vdev *vdev,
			       struct net_device *dev)
{
	struct rptr_ext_cbacks *ext_cb = NULL;
	cdp_config_param_type val = {0};
	u8 vdev_id;
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_rptr_global_priv *g_priv = NULL;
	struct wiphy *wiphy = NULL;
	struct pdev_osif_priv *pdev_ospriv = NULL;
	struct dbdc_flags flags;
	u8 disconnect_rptr_clients = 0;
	struct iterate_info iterate_msg;

	g_priv = wlan_rptr_get_global_ctx();
	ext_cb = &g_priv->ext_cbacks;
	psoc = wlan_vdev_get_psoc(vdev);
	pdev = wlan_vdev_get_pdev(vdev);

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	pdev_ospriv = wlan_pdev_get_ospriv(pdev);
	vdev_id = wlan_vdev_get_id(vdev);
	wiphy = pdev_ospriv->wiphy;

	ext_cb->legacy_dbdc_flags_get(pdev, &flags);

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	g_priv->num_stavaps_up++;
	RPTR_LOGI("Number of STA VAPs connected:%d", g_priv->num_stavaps_up);
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);

	if (wiphy && dev)
		qca_multi_link_add_station_vap(wiphy, dev);

	qca_multi_link_append_num_sta(true);

	if (g_priv->global_feature_caps & wlan_rptr_global_f_s_ssid)
		wlan_rptr_s_ssid_vdev_connection_up(vdev, &disconnect_rptr_clients);

	val.cdp_vdev_param_da_war = 0;
	cdp_txrx_set_vdev_param(soc_txrx_handle, vdev_id, CDP_ENABLE_DA_WAR,
			val);

	ext_cb->max_pri_stavap_process_up(vdev);

	if (qca_multi_link_get_num_sta() > 1) {
		qca_multi_link_set_drop_sec_mcast(true);

		if (flags.dbdc_process_enable) {
			qca_multi_link_set_dbdc_enable(true);

			/*Disable legacy DBDC model when multi link dbdb is enabled.*/
			if (ext_cb->target_lithium(pdev))
				dp_lag_soc_enable(pdev, 0);

		}
	}

#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
	ext_cb->nss_prep_mac_db_store_stavap(vdev, qca_multi_link_get_num_sta());
#endif

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	if (g_priv->num_stavaps_up > 1) {
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
		if (flags.delay_stavap_connection) {
			RPTR_LOGI("\nSetting drop_secondary_mcast and starting timer");
			dp_lag_soc_set_drop_secondary_mcast(pdev, 1);
			ext_cb->delay_stavap_conn_process_up(vdev);
		}
		if (flags.dbdc_process_enable) {
			ext_cb->dbdc_process_mac_db_up(vdev, qca_multi_link_get_num_sta());
		}
	} else {
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}

	ext_cb->act_update_force_cli_mcast_process_up(vdev);

	if ((g_priv->global_feature_caps & wlan_rptr_global_f_s_ssid) &&
	    disconnect_rptr_clients) {
		/* Disconnect only repeater clients*/
		iterate_msg.obj_type = WLAN_PEER_OP;
		iterate_msg.iterate_arg = NULL;
		iterate_msg.cb_func = wlan_rptr_peer_disconnect_cb;

		wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
					      &iterate_msg, WLAN_MLME_NB_ID);
	}
	return;

}

void
wlan_rptr_conn_down_dbdc_process(struct wlan_objmgr_vdev *vdev,
				 struct net_device *dev)
{
	cdp_config_param_type val = {0};
	u8 vdev_id;
	ol_txrx_soc_handle soc_txrx_handle;
	struct wlan_objmgr_psoc *psoc = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	struct wlan_rptr_global_priv *g_priv = NULL;
	struct rptr_ext_cbacks *ext_cb = NULL;
	struct wiphy *wiphy = NULL;
	struct pdev_osif_priv *pdev_ospriv = NULL;
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	wlan_rptr_same_ssid_feature_t *ss_info = NULL;
	bool max_priority_stavap_disconnected = 0;
	struct dbdc_flags flags;

	g_priv = wlan_rptr_get_global_ctx();
	ss_info = &g_priv->ss_info;
	ext_cb = &g_priv->ext_cbacks;
	psoc = wlan_vdev_get_psoc(vdev);
	pdev = wlan_vdev_get_pdev(vdev);

	soc_txrx_handle = wlan_psoc_get_dp_handle(psoc);
	pdev_ospriv = wlan_pdev_get_ospriv(pdev);
	vdev_id = wlan_vdev_get_id(vdev);
	wiphy = pdev_ospriv->wiphy;
	pdev_priv = wlan_rptr_get_pdev_priv(pdev);

	ext_cb->legacy_dbdc_flags_get(pdev, &flags);

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	g_priv->num_stavaps_up--;
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);

	if (wiphy && dev)
		qca_multi_link_remove_station_vap(wiphy);

	qca_multi_link_append_num_sta(false);

	wlan_rptr_s_ssid_vdev_connection_down(vdev);

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	if (g_priv->num_stavaps_up == 1) {
		ext_cb->legacy_dbdc_rootap_set(pdev, 0);
	} else if (g_priv->num_stavaps_up == 0) {
		val.cdp_vdev_param_da_war = 1;
		cdp_txrx_set_vdev_param(soc_txrx_handle, vdev_id, CDP_ENABLE_DA_WAR,
				val);
	}
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);

	ext_cb->max_pri_stavap_process_down(vdev, &max_priority_stavap_disconnected);

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	if (g_priv->num_stavaps_up == 1) {
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
		dp_lag_soc_enable(pdev, 0);
		qca_multi_link_set_dbdc_enable(false);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
		ext_cb->nss_dbdc_process_mac_db_down(vdev);
#endif
		if (flags.delay_stavap_connection) {
			RPTR_LOGI("\nclearing drop_secondary_mcast and starting timer");
			qca_multi_link_set_drop_sec_mcast(false);
			dp_lag_soc_set_drop_secondary_mcast(pdev, 0);
			ext_cb->delay_stavap_conn_process_down(vdev);
		}
	} else {
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	}

	ext_cb->force_cli_mcast_process_down(vdev, &max_priority_stavap_disconnected);
}
#endif

void
wlan_rptr_vdev_s_ssid_check_cb(struct wlan_objmgr_psoc *psoc,
			       void *obj, void *arg)
{
	struct wlan_objmgr_vdev *vdev = NULL;
	struct s_ssid_info *s_ssid_msg;
	struct wlan_objmgr_pdev *pdev;
	u8 ssid[WLAN_SSID_MAX_LEN + 1] = {0};
	u8 len = 0;

	vdev = (struct wlan_objmgr_vdev *)obj;
	if (!vdev) {
		RPTR_LOGE("RPTR VDEV is null");
		return;
	}
	if (QDF_STATUS_SUCCESS !=
			wlan_vdev_mlme_get_ssid(vdev, ssid, &len))
		return;

	pdev = wlan_vdev_get_pdev(vdev);

	s_ssid_msg = ((struct s_ssid_info *)arg);
	if (wlan_son_is_pdev_enabled(pdev)) {
		s_ssid_msg->son_enabled = 1;
		return;
	}

	if (wlan_vdev_mlme_get_opmode(vdev) != wlan_vdev_mlme_get_opmode(
							s_ssid_msg->vdev)) {
		if (wlan_rptr_is_psta_vdev(vdev))
			return;

		if (s_ssid_msg->ssid_len != len ||
		    qdf_mem_cmp(s_ssid_msg->ssid, ssid, len)) {
			return;
		}
		s_ssid_msg->ssid_match = 1;
	}
}

/**
 * wlan_rptr_same_ssid_check - Check same SSID feature support
 * @vdev- vdev object manager
 * @ssid - ssid
 * @ssid_len- ssid length
 * return void
 */
void
wlan_rptr_same_ssid_check(struct wlan_objmgr_vdev *vdev,
			  u8 *ssid, u32 ssid_len)
{
	struct wlan_objmgr_pdev *pdev = wlan_vdev_get_pdev(vdev);
	struct iterate_info iterate_msg;
	struct s_ssid_info s_ssid_msg;
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	wlan_rptr_same_ssid_feature_t   *ss_info;

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	ss_info = &g_priv->ss_info;
	if (ss_info->same_ssid_disable) {
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
		return;
	}

	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	if (wlan_son_is_pdev_enabled(pdev))
		return;

	if (wlan_rptr_is_psta_vdev(vdev))
		return;

	iterate_msg.obj_type = WLAN_VDEV_OP;
	iterate_msg.iterate_arg = &s_ssid_msg;
	iterate_msg.cb_func = wlan_rptr_vdev_s_ssid_check_cb;

	s_ssid_msg.vdev = vdev;
	s_ssid_msg.ssid = ssid;
	s_ssid_msg.ssid_len = ssid_len;
	s_ssid_msg.son_enabled = 0;
	s_ssid_msg.ssid_match = 0;

	wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
				      &iterate_msg, WLAN_MLME_NB_ID);
	if (s_ssid_msg.ssid_match) {
		if (!s_ssid_msg.son_enabled) {
			wlan_rptr_global_set_s_ssid();
			RPTR_LOGI("RPTR Enable same_ssid_support ssid:%s g_caps:%x\n",
				  ssid, g_priv->global_feature_caps);
		}
	}
}

qdf_export_symbol(wlan_rptr_same_ssid_check);

/**
 * wlan_rptr_ss_parse_scan_entries - parse scan entries
 * @vdev- vdev object manager
 * @event - scan event
 * return void
 */
void
wlan_rptr_ss_parse_scan_entries(struct wlan_objmgr_vdev *vdev,
				struct scan_event *event)
{
	wlan_rptr_core_ss_parse_scan_entries(vdev, event);
}

qdf_export_symbol(wlan_rptr_ss_parse_scan_entries);

/**
 * wlan_rptr_validate_stavap_bssid - validate stavap bssid
 * when same ssid feature is enabled
 * @vdev- vdev object manager
 * @bssid - stavap bssid
 * return QDF_STATUS_SUCCESS if bssid is correct; otherwise non-zero value
 */
QDF_STATUS
wlan_rptr_validate_stavap_bssid(struct wlan_objmgr_vdev *vdev,
				u_int8_t *bssid)
{
	return wlan_rptr_core_validate_stavap_bssid(vdev, bssid);
}

qdf_export_symbol(wlan_rptr_validate_stavap_bssid);

void
wlan_rptr_vdev_extender_info_cb(struct wlan_objmgr_psoc *psoc,
				void *obj, void *arg)
{
	struct wlan_objmgr_vdev *vdev = NULL;
	struct extender_msg *ext_info;
	enum QDF_OPMODE opmode;

	vdev = (struct wlan_objmgr_vdev *)obj;

	if (!vdev) {
		mlme_err("VDEV is null");
		return;
	}
	opmode = wlan_vdev_mlme_get_opmode(vdev);
	ext_info = ((struct extender_msg *)arg);

	if (opmode == ext_info->opmode) {
		if (opmode == QDF_SAP_MODE) {
			WLAN_ADDR_COPY(ext_info->frm, wlan_vdev_mlme_get_macaddr(vdev));
			ext_info->frm += QDF_MAC_ADDR_SIZE;
			ext_info->apvaps_cnt++;
		}
		if (opmode == QDF_STA_MODE) {
			/*Copy only MPSTA mac address, not PSTA mac address*/
			if (wlan_rptr_is_psta_vdev(vdev))
				return;

			WLAN_ADDR_COPY(ext_info->frm, wlan_vdev_mlme_get_macaddr(vdev));
			ext_info->frm += QDF_MAC_ADDR_SIZE;
			ext_info->stavaps_cnt++;
		}
	}
}

/**
 * wlan_rptr_add_extender_ie - Add extender IE
 * @vdev- vdev object manager
 * @ftype- frame type
 * @frm - frame pointer
 * return frame pointer
 */
u_int8_t *
wlan_rptr_add_extender_ie(struct wlan_objmgr_vdev *vdev,
			  ieee80211_frame_type ftype, u8 *frm)
{
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	wlan_rptr_same_ssid_feature_t   *ss_info;
	u8 extender_info;
	struct iterate_info iterate_msg;
	struct extender_msg ext_info;
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_objmgr_pdev *pdev = NULL;
	static const u8 oui[4] = {
		(QCA_OUI & 0xff), ((QCA_OUI >> 8) & 0xff),
		((QCA_OUI >> 16) & 0xff), QCA_OUI_EXTENDER_TYPE
	};
	u8 *pos1, *pos2;
	u8 *ie_len;

	pdev = wlan_vdev_get_pdev(vdev);
	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!g_priv)
		return frm;

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	ss_info = &g_priv->ss_info;
	extender_info = ss_info->extender_info;
	*frm++ = IEEE80211_ELEMID_VENDOR;
	ie_len = frm;
	*frm++ = sizeof(oui) + IE_CONTENT_SIZE;
	OS_MEMCPY(frm, oui, sizeof(oui));
	frm += sizeof(oui);
	*frm++ = extender_info;
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);

	if (ftype == IEEE80211_FRAME_TYPE_ASSOCRESP) {
		pos1 = frm++;
		pos2 = frm++;
		*ie_len += 2;

		iterate_msg.obj_type = WLAN_VDEV_OP;
		iterate_msg.iterate_arg = &ext_info;
		iterate_msg.cb_func = wlan_rptr_vdev_extender_info_cb;

		ext_info.vdev = vdev;
		ext_info.frm = frm;
		ext_info.apvaps_cnt = 0;
		ext_info.stavaps_cnt = 0;

		ext_info.opmode = QDF_SAP_MODE;
		wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
					      &iterate_msg, WLAN_MLME_NB_ID);

		ext_info.opmode = QDF_STA_MODE;
		wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
					      &iterate_msg, WLAN_MLME_NB_ID);

		*pos1 = ext_info.apvaps_cnt;
		*pos2 = ext_info.stavaps_cnt;
		*ie_len += ((ext_info.apvaps_cnt + ext_info.stavaps_cnt) *
			    QDF_MAC_ADDR_SIZE);
		frm = ext_info.frm;
	}
	return frm;
}

qdf_export_symbol(wlan_rptr_add_extender_ie);

/**
 * wlan_rptr_process_extender_ie - Process extender IE
 * @peer- peer object manager
 * @ie - extender IE pointer
 * @ftype- frame type
 * return void
 */
void
wlan_rptr_process_extender_ie(struct wlan_objmgr_peer *peer,
			      const u8 *ie, ieee80211_frame_type ftype)
{
	u8 ie_len, i;
	u8 *mac_list;
	u8 extender_ie_content, extender_ie_type;
	u8 apvaps_cnt = 0, stavaps_cnt = 0;
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_rptr_peer_priv *peer_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	struct wlan_objmgr_pdev *pdev;

	pdev = wlan_vdev_get_pdev(wlan_peer_get_vdev(peer));

	if (!pdev)
		return;

	if (!g_priv)
		return;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!pdev_priv)
		return;

	peer_priv = wlan_rptr_get_peer_priv(peer);
	if (!peer_priv)
		return;

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	ss_info = &g_priv->ss_info;
	ie++; /*to get ie len*/
	ie_len = *ie;
	ie += 4; /*to get extender ie content*/
	extender_ie_type = *ie++;
	extender_ie_content = *ie++;
	ie_len -= 5;
	if (ftype == IEEE80211_FRAME_TYPE_ASSOCREQ) {
		peer_priv->is_extender_client = 1;
		ss_info->num_rptr_clients++;
	} else if (ftype == IEEE80211_FRAME_TYPE_ASSOCRESP) {
		pdev_priv->extender_connection = ext_connection_type_no_root_access;
		if (g_priv->num_stavaps_up == 0) {
			apvaps_cnt = *ie++;
			stavaps_cnt = *ie++;
			mac_list = (u_int8_t *)ss_info->preferred_bssid_list;
			for (i = 0; ((i < apvaps_cnt) && (i < MAX_RADIO_CNT) &&
				     (ie_len > 0)); i++) {
				WLAN_ADDR_COPY((mac_list + (i * QDF_MAC_ADDR_SIZE)),
					       ie);
				qdf_info("Preferred mac[%d]:%s", i,
					 ether_sprintf(mac_list + (i * QDF_MAC_ADDR_SIZE)));
				ie += QDF_MAC_ADDR_SIZE;
				ie_len -= QDF_MAC_ADDR_SIZE;
			}
			mac_list = (u_int8_t *)ss_info->denied_client_list;
			for (i = 0; ((i < stavaps_cnt) && (i < MAX_RADIO_CNT) &&
				     (ie_len > 0)); i++) {
				WLAN_ADDR_COPY((mac_list + (i * QDF_MAC_ADDR_SIZE)),
						ie);
				qdf_info("Denied mac[%d]:%s", i,
					 ether_sprintf(mac_list + (i * QDF_MAC_ADDR_SIZE)));
				ie += QDF_MAC_ADDR_SIZE;
				ie_len -= QDF_MAC_ADDR_SIZE;
			}

			ss_info->ap_preference = ap_preference_type_rptr;
		}
		if ((extender_ie_content & ROOTAP_ACCESS_MASK) ==
							ROOTAP_ACCESS_MASK) {
			/*If connecting RE has RootAP access*/
			pdev_priv->extender_connection = ext_connection_type_root_access;
		}
	}
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
}

qdf_export_symbol(wlan_rptr_process_extender_ie);

/**
 * wlan_rptr_allow_client_conn - Check for client connection
 * @vdev- vdev object manager
 * @bssid- client mac address
 * return 1 to allow or 0 to not allow client connection
 */
bool
wlan_rptr_allow_client_conn(struct wlan_objmgr_vdev *vdev, uint8_t *bssid)
{
	struct wlan_rptr_global_priv *g_priv;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	int i;

	g_priv = wlan_rptr_get_global_ctx();
	if (!g_priv)
		return 1;

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	ss_info = &g_priv->ss_info;
	for (i = 0; i < RPTR_MAX_RADIO_CNT; i++) {
		if (OS_MEMCMP(bssid, &ss_info->denied_client_list[i][0],
			      QDF_MAC_ADDR_SIZE) == 0) {
			RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
			return 0;
		}
	}
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	return 1;
}

qdf_export_symbol(wlan_rptr_allow_client_conn);

void
wlan_rptr_pdev_ext_connection_cb(struct wlan_objmgr_psoc *psoc,
				 void *obj, void *arg)
{
	struct wlan_objmgr_pdev *pdev;
	struct ext_connection_info *ext_con_msg;
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;

	pdev = (struct wlan_objmgr_pdev *)obj;

	if (!pdev) {
		mlme_err("PDEV is null");
		return;
	}
	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!pdev_priv)
		return;

	if (!g_priv)
		return;

	ext_con_msg = ((struct ext_connection_info *)arg);
	if (pdev_priv->extender_connection) {
		ext_con_msg->disconnect_rptr_clients = 1;
		ext_con_msg->disconnect_sta_vdev = ext_cb->get_stavap(pdev);
	}
}

/**
 * wlan_rptr_validate_stavap_connection - Validate stavap connection
 * @vdev- vdev object manager
 * @bssid- stavap bssid
 * return QDF_STATUS_SUCCESS to allow connection; otherwise non-zero value
 */
int
wlan_rptr_validate_stavap_connection(struct wlan_objmgr_vdev *vdev,
				     u_int8_t *bssid)
{
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	struct wlan_objmgr_pdev *pdev;
	int i, flag = QDF_STATUS_SUCCESS;
	struct iterate_info iterate_msg;
	struct ext_connection_info ext_con_msg;

	pdev = wlan_vdev_get_pdev(vdev);

	if (!pdev)
		return QDF_STATUS_E_INVAL;

	if (!g_priv)
		return QDF_STATUS_E_INVAL;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!pdev_priv)
		return QDF_STATUS_E_INVAL;

	if (!IS_NULL_ADDR(pdev_priv->preferred_bssid) && (OS_MEMCMP(
	    pdev_priv->preferred_bssid, bssid, QDF_MAC_ADDR_SIZE) != 0)) {
		return QDF_STATUS_E_INVAL;
	}
	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	ss_info = &g_priv->ss_info;
	if (((ss_info->extender_info & ROOTAP_ACCESS_MASK) !=
	    ROOTAP_ACCESS_MASK) && (g_priv->num_stavaps_up > 1)) {
		RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
		return QDF_STATUS_E_INVAL;
	} else {
		if (!pdev_priv->extender_connection) {
			/* When stavap connected to RootAP, clear preferred and
			 *  denied mac list
			*/
			for (i = 0; i < MAX_RADIO_CNT; i++) {
				OS_MEMZERO(&ss_info->preferred_bssid_list[i][0],
					   QDF_MAC_ADDR_SIZE);
				OS_MEMZERO(&ss_info->denied_client_list[i][0],
					   QDF_MAC_ADDR_SIZE);
			}
		}
		if (ss_info->ap_preference == ap_preference_type_root) {
			/*make sure connecting to RootAP only*/
			if (pdev_priv->extender_connection) {
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
				return QDF_STATUS_E_INVAL;
			}
		} else if (ss_info->ap_preference == ap_preference_type_rptr) {
			/* When first stavap connected to Repeater,
			 * and second stavap connected to RootAP,
			 * Then, disconnect first stavap,
			 * so that it will connect to RootAP
			*/
			if (!pdev_priv->extender_connection) {
				ss_info->ap_preference = ap_preference_type_root;
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
				iterate_msg.obj_type = WLAN_PDEV_OP;
				iterate_msg.iterate_arg = &ext_con_msg;
				iterate_msg.cb_func = wlan_rptr_pdev_ext_connection_cb;

				ext_con_msg.disconnect_rptr_clients = 0;
				ext_con_msg.disconnect_sta_vdev = NULL;
				wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
							      &iterate_msg, WLAN_MLME_NB_ID);
				if (ext_con_msg.disconnect_sta_vdev) {
					if (wlan_cm_is_vdev_connected(ext_con_msg.disconnect_sta_vdev)) {
						wlan_cm_disconnect(ext_con_msg.disconnect_sta_vdev, CM_SB_DISCONNECT,
								   REASON_DISASSOC_NETWORK_LEAVING, NULL);
					}
				}
				if (ext_con_msg.disconnect_rptr_clients) {
					iterate_msg.obj_type = WLAN_PEER_OP;
					iterate_msg.iterate_arg = NULL;
					iterate_msg.cb_func = wlan_rptr_peer_disconnect_cb;

					wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
								      &iterate_msg, WLAN_MLME_NB_ID);
				}
				return QDF_STATUS_SUCCESS;
			}
			/*make sure connecting to RE whose mac present in preferred bssid*/
			flag = QDF_STATUS_E_INVAL;
			for (i = 0; i < MAX_RADIO_CNT; i++) {
				if (OS_MEMCMP(bssid, &ss_info->preferred_bssid_list[i][0],
					      QDF_MAC_ADDR_SIZE) == 0) {
					flag = QDF_STATUS_SUCCESS;
					break;
				}
			}
		}
	}
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	return flag;
}

qdf_export_symbol(wlan_rptr_validate_stavap_connection);

/**
 * wlan_rptr_s_ssid_vdev_connection_up - update same ssid feature info during
 * stavap connection up
 * @vdev- vdev object manager
 * @disconnect_rptr_clients- flag pointer to disconnect repeater clients
 * return void
 */
void
wlan_rptr_s_ssid_vdev_connection_up(struct wlan_objmgr_vdev *vdev,
				    u8 *disconnect_rptr_clients)
{
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	struct wlan_objmgr_pdev *pdev;
	struct iterate_info iterate_msg;
	u8 update_beacon = 0;

	pdev = wlan_vdev_get_pdev(vdev);

	if (!pdev)
		return;

	if (!g_priv)
		return;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!pdev_priv)
		return;

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	ss_info = &g_priv->ss_info;

	if (g_priv->num_stavaps_up == 1) {
		ss_info->extender_info |= STAVAP_CONNECTION_MASK;
		update_beacon = 1;
		if ((pdev_priv->extender_connection == ext_connection_type_init) ||
		    (pdev_priv->extender_connection == ext_connection_type_root_access)) {
			*disconnect_rptr_clients = 1;
		}
		if (pdev_priv->extender_connection == ext_connection_type_init)
			ss_info->ap_preference = ap_preference_type_root;
	}
	if ((pdev_priv->extender_connection == ext_connection_type_init) ||
	    (pdev_priv->extender_connection == ext_connection_type_root_access)) {
		ss_info->extender_info |= ROOTAP_ACCESS_MASK;
		update_beacon = 1;
	}
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
	if (update_beacon) {
		iterate_msg.obj_type = WLAN_PDEV_OP;
		iterate_msg.iterate_arg = NULL;
		iterate_msg.cb_func = wlan_rptr_pdev_update_beacon_cb;

		wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
					      &iterate_msg, WLAN_MLME_NB_ID);
	}
	RPTR_LOGI("AP preference:%d Extender connection:%d Extender info:0x%x",
		 ss_info->ap_preference, pdev_priv->extender_connection,
		 ss_info->extender_info);

}
qdf_export_symbol(wlan_rptr_s_ssid_vdev_connection_up);

/**
 * wlan_rptr_s_ssid_vdev_connection_down - update same ssid feature info during
 * stavap connection down
 * @vdev- vdev object manager
 * return void
 */
void
wlan_rptr_s_ssid_vdev_connection_down(struct wlan_objmgr_vdev *vdev)
{
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	struct wlan_objmgr_pdev *pdev;
	struct iterate_info iterate_msg;
	u8 i = 0;

	pdev = wlan_vdev_get_pdev(vdev);

	if (!pdev)
		return;

	if (!g_priv)
		return;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!pdev_priv)
		return;

	RPTR_PDEV_LOCK(&pdev_priv->rptr_pdev_lock);
	pdev_priv->extender_connection = ext_connection_type_init;
	qdf_mem_zero(pdev_priv->preferred_bssid, QDF_MAC_ADDR_SIZE);
	RPTR_PDEV_UNLOCK(&pdev_priv->rptr_pdev_lock);

	RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
	ss_info = &g_priv->ss_info;
	if (g_priv->num_stavaps_up == 0) {
		ss_info->ap_preference = ap_preference_type_init;
		ss_info->extender_info = 0;
		ss_info->rootap_access_downtime = qdf_get_system_timestamp();
		for (i = 0; i < RPTR_MAX_RADIO_CNT; i++) {
			qdf_mem_zero(&ss_info->preferred_bssid_list[i][0],
					QDF_MAC_ADDR_SIZE);
			qdf_mem_zero(&ss_info->denied_client_list[i][0],
					QDF_MAC_ADDR_SIZE);
		}
	}
	RPTR_LOGI("AP preference:%d Extender connection:%d Extender info:0x%x",
		 ss_info->ap_preference, pdev_priv->extender_connection,
		 ss_info->extender_info);
	RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);

	iterate_msg.obj_type = WLAN_PDEV_OP;
	iterate_msg.iterate_arg = NULL;
	iterate_msg.cb_func = wlan_rptr_pdev_update_beacon_cb;

	wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
				      &iterate_msg, WLAN_MLME_NB_ID);
}

qdf_export_symbol(wlan_rptr_s_ssid_vdev_connection_down);

void
wlan_rptr_disconnect_sec_stavap_cb(struct wlan_objmgr_psoc *psoc,
				   void *obj, void *arg)
{
#if DBDC_REPEATER_SUPPORT
	struct wlan_objmgr_pdev *pdev;
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;
	struct wlan_objmgr_vdev *sta_vdev;
	struct pdev_osif_priv *pdev_ospriv;
	struct wiphy *wiphy;

	pdev = (struct wlan_objmgr_pdev *)obj;

	if (!pdev)
		return;

	pdev_ospriv = wlan_pdev_get_ospriv(pdev);
	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	sta_vdev = ext_cb->get_stavap(pdev);
	wiphy = pdev_ospriv->wiphy;
	if (wlan_cm_is_vdev_connected(sta_vdev) &&
	    !(qca_multi_link_is_primary_radio(wiphy))) {
		wlan_cm_disconnect(sta_vdev, CM_SB_DISCONNECT,
				   REASON_DISASSOC_NETWORK_LEAVING, NULL);
	}
#endif
}

/**
 * wlan_rptr_pdev_extd_ioctl - NB api for extd ioctl call
 * param: pdev: pdev object manager
 * param: feature specific param
 * data: ptr to buffer
 * ret: 0 in case of success and negative in case of failure
 */
int
wlan_rptr_pdev_extd_ioctl(struct wlan_objmgr_pdev *pdev, int param, char *data)
{
	struct wlan_rptr_global_priv *g_priv = NULL;
	struct rptr_ext_cbacks *ext_cb = NULL;
	struct wlan_objmgr_vdev *sta_vdev = NULL;
	struct iface_config_t *iface_config = NULL;
	int retv = 0;

	g_priv = wlan_rptr_get_global_ctx();
	ext_cb = &g_priv->ext_cbacks;
	iface_config = (struct iface_config_t *)data;

	switch (param) {
	case EXTENDED_SUBIOCTL_IFACE_MGR_STATUS:
#if ATH_SUPPORT_WRAP && DBDC_REPEATER_SUPPORT
		if ((iface_config->iface_mgr_up) == 1) {
			wlan_rptr_global_set_g_wds();
			RPTR_LOGI("setting iface_mgr_up to 1");
		} else {
			wlan_rptr_global_clear_g_wds();
			RPTR_LOGI("setting iface_mgr_up to 0");
		}
#endif
		break;
	case EXTENDED_SUBIOCTL_GET_STAVAP_CONNECTION:
		{
			u8 stavap_up;

		sta_vdev = ext_cb->get_stavap(pdev);

			if (!sta_vdev) {
				RPTR_LOGI("stavap not up for pdev_id:%d",
					  wlan_objmgr_pdev_get_pdev_id(pdev));
				return -EINVAL;
			}

			if (wlan_vdev_is_up(sta_vdev) == QDF_STATUS_SUCCESS)
				stavap_up = 1;
			else
				stavap_up = 0;

			iface_config->stavap_up = stavap_up;
		}
		break;
	case EXTENDED_SUBIOCTL_GET_DISCONNECTION_TIMEOUT:
		{
#if DBDC_REPEATER_SUPPORT
			u16 timeout;
			u32 disconnect_timeout;
			u32 reconfiguration_timeout;

			wlan_rptr_core_global_disconnect_timeout_get(&disconnect_timeout);
			wlan_rptr_core_global_reconfig_timeout_get(&reconfiguration_timeout);
			if (disconnect_timeout >= reconfiguration_timeout) {
				timeout = disconnect_timeout;
				RPTR_LOGI("disconnect_timeout:%d",
					  disconnect_timeout);
			} else {
				timeout = reconfiguration_timeout;
				RPTR_LOGI("reconfiguration_timeout:%d",
					  reconfiguration_timeout);
			}

			iface_config->timeout = timeout;
#endif
		}
		break;
	case EXTENDED_SUBIOCTL_GET_PREF_UPLINK:
		{
#if DBDC_REPEATER_SUPPORT
			int preferredUplink;
			u32 pref_uplink;

			wlan_rptr_core_pdev_pref_uplink_get(pdev, &pref_uplink);
			if (pref_uplink)
				preferredUplink = 1;
			else
				preferredUplink = 0;

			iface_config->is_preferredUplink = preferredUplink;
#endif
		}
		break;
	}
	return retv;
}

qdf_export_symbol(wlan_rptr_pdev_extd_ioctl);

/**
 * wlan_rptr_update_extender_info - update extender info
 * @vdev- vdev object manager
 * @scan_entry- scan entry
 * @disconnect_sec_stavap- flag pointer to disconnect secondary stavap
 * @bssid- AP bssid
 * return update_beacon value
 */
uint8_t
wlan_rptr_update_extender_info(struct wlan_objmgr_vdev *vdev,
			       ieee80211_scan_entry_t scan_entry,
			       u8 *disconnect_sec_stavap, u8 *bssid)
{
	struct wlan_rptr_global_priv *g_priv = wlan_rptr_get_global_ctx();
	struct rptr_ext_cbacks *ext_cb = &g_priv->ext_cbacks;
	struct wlan_rptr_pdev_priv *pdev_priv = NULL;
	wlan_rptr_same_ssid_feature_t   *ss_info;
	struct wlan_objmgr_pdev *pdev;
	struct iterate_info iterate_msg;
	struct wlan_objmgr_vdev *sta_vdev;
	u8 update_beacon = 0;
	struct qdf_mac_addr stavap_bssid;
	struct ieee80211_ie_extender *se_extender = NULL;

	pdev = wlan_vdev_get_pdev(vdev);

	if (!pdev)
		return 0;

	if (!g_priv)
		return 0;

	pdev_priv = wlan_rptr_get_pdev_priv(pdev);
	if (!pdev_priv)
		return 0;

	se_extender = (struct ieee80211_ie_extender *)
			util_scan_entry_extenderie(scan_entry);
	if (se_extender) {
		sta_vdev = ext_cb->get_stavap(pdev);
		if ((vdev == sta_vdev) && wlan_cm_is_vdev_connected(sta_vdev)) {
			wlan_vdev_get_bss_peer_mac(sta_vdev, &stavap_bssid);
			if (WLAN_ADDR_EQ(&stavap_bssid, bssid)) {
				RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
				ss_info = &g_priv->ss_info;
				if ((ss_info->extender_info & ROOTAP_ACCESS_MASK) != ((se_extender->extender_info) & ROOTAP_ACCESS_MASK)) {
					ss_info->extender_info &= ~ROOTAP_ACCESS_MASK;
					ss_info->extender_info |= (se_extender->extender_info & ROOTAP_ACCESS_MASK);
					update_beacon = 1;
					if ((ss_info->extender_info & ROOTAP_ACCESS_MASK) == 0x0) {
						ss_info->rootap_access_downtime = OS_GET_TIMESTAMP();
						if (!ext_cb->target_lithium(pdev)) {
							*disconnect_sec_stavap = 1;
						} else {
							RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
							iterate_msg.obj_type = WLAN_PDEV_OP;
							iterate_msg.iterate_arg = NULL;
							iterate_msg.cb_func = wlan_rptr_disconnect_sec_stavap_cb;

							wlan_objmgr_iterate_psoc_list(wlan_rptr_psoc_iterate_list,
									&iterate_msg, WLAN_MLME_NB_ID);
							RPTR_GLOBAL_LOCK(&g_priv->rptr_global_lock);
						}
					}
				}
				RPTR_GLOBAL_UNLOCK(&g_priv->rptr_global_lock);
			}
		}
	}
	return update_beacon;
}

qdf_export_symbol(wlan_rptr_update_extender_info);
