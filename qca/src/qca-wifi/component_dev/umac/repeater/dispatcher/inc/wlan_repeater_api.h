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

#ifndef _WLAN_REPEATER_API_H_
#define _WLAN_REPEATER_API_H_

#include <wlan_objmgr_cmn.h>
#include <wlan_repeater_types.h>
#include <ieee80211_defines.h>
#include <wlan_scan_utils_api.h>
#include <wlan_scan.h>

/* REPEATER global Feature flags */
/* SAME SSID feature*/
#define wlan_rptr_global_f_s_ssid     0x00000001
/* GLOBAL_WDS*/
#define wlan_rptr_global_f_g_wds      0x00000002

/* REPEATER psoc Feature flags */
/* QWRAP max client feature*/
#define wlan_rptr_psoc_f_max_qwrap  0x00000001

/* REPEATER PDEV Feature flags */
/* PDEV is EIR mode*/
#define wlan_rptr_pdev_f_eir          0x00000001
/* PDEV is QWRAP*/
#define wlan_rptr_pdev_f_qwrap        0x00000002

/* REPEATER VDEV Feature flags */
/* VDEV is WDS*/
#define wlan_rptr_vdev_f_wds          0x00000001
/* VDEV is EXTAP*/
#define wlan_rptr_vdev_f_extap        0x00000002
/* VDEV is MPSTA*/
#define wlan_rptr_vdev_f_mpsta        0x00000004
/* VDEV is PSTA*/
#define wlan_rptr_vdev_f_psta         0x00000008
/* VDEV is WRAP*/
#define wlan_rptr_vdev_f_wrap         0x00000010
/* VDEV is WIRED PSTA*/
#define wlan_rptr_vdev_f_wired_psta   0x00000020
/* VDEV is MAT*/
#define wlan_rptr_vdev_f_mat          0x00000040
/* NO event handlers for vdev*/
#define wlan_rptr_vdev_f_no_event     0x00000080

/**
 * @brief init function for WLAN REPEATER
 *
 * @param [in] void
 *
 * @return QDF_STATUS_SUCCESS for success otherwise
 *  QDF_STATUS_FAIL
 */
QDF_STATUS wlan_repeater_init(void);

/**
 * @brief Deinit REPEATER infrasturcture.
 *
 * @param [in] void
 * @return QDF_STATUS_SUCCESS for success otherwise
 *  QDF_STATUS_FAIL
 * @return
 */
QDF_STATUS wlan_repeater_deinit(void);

#if DBDC_REPEATER_SUPPORT
/**
 * struct dbdc_flags - legacy dbdc flags used by rep comp
 * @dbdc_process_enable:            dbdc_process_enable value
 * @delay_stavap_connection:        delay_stavap_connection value
 */
struct dbdc_flags {
	bool dbdc_process_enable;
	u8 delay_stavap_connection;
};
#endif

/**
 * struct rptr_ext_cbacks - Repeater extended callbacks
 * @vdev_set_powersave:             Set powersave
 * @vdev_set_param:                 set vdev param
 * @vdev_pwrsave_force_sleep:       set powersave force sleep
 * @send_wds_cmd:                   send wds command
 * @get_stavap:                     get stavap pointer
 * @peer_disassoc:                  disconnect peer
 * @pdev_update_beacon:             update beacon
 * @target_lithium:                 check target type
 * @dessired_ssid_found:            desired ssid found
 * @legacy_dbdc_flags_get:          Get legacy dbdc flag
 * @max_pri_stavap_process_up:      Max pri stavap up
 * @delay_stavap_conn_process_up:   Delay stavap conn up
 * @dbdc_process_mac_db_up:         dbdc mac db up
 * @act_update_force_cli_mcast_process_up: Activity update force client mcast up
 * @legacy_dbdc_rootap_set:         Set legacy dbdc rootap flag
 * @max_pri_stavap_process_down:    Max pri stavap down
 * @delay_stavap_conn_process_down: Delay stavap conn down
 * @force_cli_mcast_process_down:   force client mcast down
 * @rptr_send_event:                Send rptr specific event
 * @vdev_nss_ol_psta_add:           NSS OL API to add PSTA
 * @vdev_nss_ol_psta_delete:        NSS OL API to delete PSTA
 * @vdev_nss_ol_set_cfg:            NSS OL API to set cfg
 * @nss_dbdc_process_mac_db_down:   NSS OL API for dbdc mac db down
 * @nss_prep_mac_db_store_stavap:   NSS OL API for prep macdb store stavap
 * @vdev_is_psta:                   check vdev is PSTA
 * @vdev_is_mpsta:                  check vdev is MPSTA
 * @get_mpsta_vdev:                 get MPSTA vdev pointer
 */
struct rptr_ext_cbacks {
	void (*vdev_set_powersave)(struct wlan_objmgr_vdev *vdev, int val);
	void (*vdev_set_param)(struct wlan_objmgr_vdev *vdev,
			       ieee80211_param param, u_int32_t val);
	void (*vdev_pwrsave_force_sleep)(struct wlan_objmgr_vdev *vdev,
					 bool enable);
	int (*send_wds_cmd)(struct wlan_objmgr_vdev *vdev, uint8_t value);
	struct wlan_objmgr_vdev *(*get_stavap)(struct wlan_objmgr_pdev *pdev);
	void (*peer_disassoc)(struct wlan_objmgr_peer *peer);
	void (*pdev_update_beacon)(struct wlan_objmgr_pdev *pdev);
	bool (*target_lithium)(struct wlan_objmgr_pdev *pdev);
	bool (*dessired_ssid_found)(struct wlan_objmgr_vdev *vdev,
				    u8 *ssid, u8 ssid_len);
#if DBDC_REPEATER_SUPPORT
	void (*legacy_dbdc_flags_get)(struct wlan_objmgr_pdev *pdev,
				      struct dbdc_flags *flags);
	void (*max_pri_stavap_process_up)(struct wlan_objmgr_vdev *vdev);
	void (*delay_stavap_conn_process_up)(struct wlan_objmgr_vdev *vdev);
	void (*dbdc_process_mac_db_up)(struct wlan_objmgr_vdev *vdev,
				       u8 num_sta);
	void (*act_update_force_cli_mcast_process_up)(struct wlan_objmgr_vdev *vdev);
	void (*legacy_dbdc_rootap_set)(struct wlan_objmgr_pdev *pdev, int value);
	void (*max_pri_stavap_process_down)(struct wlan_objmgr_vdev *vdev,
					    bool *max_priority_stavap_disconnected);
	void (*delay_stavap_conn_process_down)(struct wlan_objmgr_vdev *vdev);
	void (*force_cli_mcast_process_down)(struct wlan_objmgr_vdev *vdev,
					     bool *max_priority_stavap_disconnected);
#endif
	void (*rptr_send_event)(struct wlan_objmgr_vdev *vdev,
				u8 *ev_data, u32 ev_len, u8 ev_flags);
#ifdef QCA_NSS_WIFI_OFFLOAD_SUPPORT
	void (*vdev_nss_ol_psta_add)(struct wlan_objmgr_vdev *vdev);
	void (*vdev_nss_ol_psta_delete)(struct wlan_objmgr_vdev *vdev);
	void (*vdev_nss_ol_set_cfg)(struct wlan_objmgr_vdev *vdev, int cmd);
	void (*nss_dbdc_process_mac_db_down)(struct wlan_objmgr_vdev *vdev);
	void (*nss_prep_mac_db_store_stavap)(struct wlan_objmgr_vdev *vdev,
					     u8 num_sta);
#endif
#if ATH_SUPPORT_WRAP
#if WLAN_QWRAP_LEGACY
	bool (*vdev_is_psta)(struct wlan_objmgr_vdev *vdev);
	bool (*vdev_is_mpsta)(struct wlan_objmgr_vdev *vdev);
	struct wlan_objmgr_vdev *
		(*get_mpsta_vdev)(struct wlan_objmgr_vdev *vdev);
#endif
#endif
};

/**
 * wlan_rptr_register_ext_cb() - Register repeater legacy callbacks
 * commands on repeater parameters
 * @psoc    - the physical device object
 * @ext_cbacks - Reference to struct rptr_ext_cbacks from which
 * function pointers need to be copied
 *
 * API to register repeater related extended callbacks
 *
 * Return: QDF_STATUS_SUCCESS upon successful registration,
 *         QDF_STATUS_E_FAILURE upon failure
 */
QDF_STATUS wlan_rptr_register_ext_cb(
		struct wlan_objmgr_psoc *psoc,
		struct rptr_ext_cbacks *ext_cbacks);

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
				 uint32_t cap);

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
				   uint32_t cap);

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
				    uint32_t cap);

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
				 uint32_t cap);

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
				   uint32_t cap);

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
				    uint32_t cap);
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
				 uint32_t cap);

/**
 * wlan_rptr_psoc_clear_feat_cap() - clear feature caps
 * @psoc: psoc object
 * @cap: capabilities to be cleared
 *
 * api to clear rapeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_psoc_clear_feat_cap(struct wlan_objmgr_psoc *psoc,
				   uint32_t cap);

/**
 * wlan_rptr_psoc_is_feat_cap_set() - get feature caps
 * @psoc: psoc object
 * @cap: capabilities to be checked
 *
 * api to know repeater feature capability is set or not
 *
 * return: 1 if capabilities is  set or else 0
 */
uint8_t wlan_rptr_psoc_is_feat_cap_set(struct wlan_objmgr_psoc *psoc,
				    uint32_t cap);

/**
 * wlan_rptr_global_set_feat_cap() - set feature caps
 * @cap: capabilities to be set
 *
 * api to set repeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_global_set_feat_cap(uint32_t cap);

/**
 * wlan_rptr_global_clear_feat_cap() - clear feature caps
 * @cap: capabilities to be cleared
 *
 * api to clear rapeater feature capabilities
 *
 * return: void
 */
void wlan_rptr_global_clear_feat_cap(uint32_t cap);

/**
 * wlan_rptr_global_is_feat_cap_set() - get feature caps
 * @cap: capabilities to be checked
 *
 * api to know repeater feature capability is set or not
 *
 * return: 1 if capabilities is  set or else 0
 */
uint8_t wlan_rptr_global_is_feat_cap_set(uint32_t cap);

/**
 * wlan_rptr_reset_flags() - reset global flags
 *
 * api to reset repeater flags
 *
 * return: void
 */
void wlan_rptr_reset_flags(struct wlan_objmgr_pdev *pdev);

#define WLAN_RPTR_VDEV_FLAG_FUNCS(xyz) \
static INLINE void \
wlan_rptr_vdev_set_##xyz(struct wlan_objmgr_vdev *_vdev) { \
wlan_rptr_vdev_set_feat_cap(_vdev, wlan_rptr_vdev_f_##xyz); \
return; \
} \
static INLINE int \
wlan_rptr_vdev_is_##xyz(struct wlan_objmgr_vdev *_vdev) { \
return wlan_rptr_vdev_is_feat_cap_set(_vdev, wlan_rptr_vdev_f_##xyz); \
} \
static INLINE void \
wlan_rptr_vdev_clear_##xyz(struct wlan_objmgr_vdev *_vdev) { \
wlan_rptr_vdev_clear_feat_cap(_vdev, wlan_rptr_vdev_f_##xyz); \
return; \
}

WLAN_RPTR_VDEV_FLAG_FUNCS(wds)
WLAN_RPTR_VDEV_FLAG_FUNCS(extap)
WLAN_RPTR_VDEV_FLAG_FUNCS(mpsta)
WLAN_RPTR_VDEV_FLAG_FUNCS(wrap)
WLAN_RPTR_VDEV_FLAG_FUNCS(psta)
WLAN_RPTR_VDEV_FLAG_FUNCS(wired_psta)
WLAN_RPTR_VDEV_FLAG_FUNCS(mat)
WLAN_RPTR_VDEV_FLAG_FUNCS(no_event)

#define WLAN_RPTR_PDEV_FLAG_FUNCS(xyz) \
static INLINE void \
wlan_rptr_pdev_set_##xyz(struct wlan_objmgr_pdev *_pdev) { \
wlan_rptr_pdev_set_feat_cap(_pdev, wlan_rptr_pdev_f_##xyz); \
return; \
} \
static INLINE int \
wlan_rptr_pdev_is_##xyz(struct wlan_objmgr_pdev *_pdev) { \
return wlan_rptr_pdev_is_feat_cap_set(_pdev, wlan_rptr_pdev_f_##xyz); \
} \
static INLINE void \
wlan_rptr_pdev_clear_##xyz(struct wlan_objmgr_pdev *_pdev) { \
wlan_rptr_pdev_clear_feat_cap(_pdev, wlan_rptr_pdev_f_##xyz); \
return; \
}

WLAN_RPTR_PDEV_FLAG_FUNCS(eir)
WLAN_RPTR_PDEV_FLAG_FUNCS(qwrap)

#define WLAN_RPTR_PSOC_FLAG_FUNCS(xyz) \
static INLINE void \
wlan_rptr_psoc_set_##xyz(struct wlan_objmgr_psoc *_psoc) { \
wlan_rptr_psoc_set_feat_cap(_psoc, wlan_rptr_psoc_f_##xyz); \
return; \
} \
static INLINE int \
wlan_rptr_psoc_is_##xyz(struct wlan_objmgr_psoc *_psoc) { \
return wlan_rptr_psoc_is_feat_cap_set(_psoc, wlan_rptr_psoc_f_##xyz); \
} \
static INLINE void \
wlan_rptr_psoc_clear_##xyz(struct wlan_objmgr_psoc *_psoc) { \
wlan_rptr_psoc_clear_feat_cap(_psoc, wlan_rptr_psoc_f_##xyz); \
return; \
}

WLAN_RPTR_PSOC_FLAG_FUNCS(max_qwrap)

#define WLAN_RPTR_GLOBAL_FLAG_FUNCS(xyz) \
static INLINE void \
wlan_rptr_global_set_##xyz(void) { \
wlan_rptr_global_set_feat_cap(wlan_rptr_global_f_##xyz); \
return; \
} \
static INLINE int \
wlan_rptr_global_is_##xyz(void) { \
return wlan_rptr_global_is_feat_cap_set(wlan_rptr_global_f_##xyz); \
} \
static INLINE void \
wlan_rptr_global_clear_##xyz(void) { \
wlan_rptr_global_clear_feat_cap(wlan_rptr_global_f_##xyz); \
return; \
}

WLAN_RPTR_GLOBAL_FLAG_FUNCS(s_ssid)
WLAN_RPTR_GLOBAL_FLAG_FUNCS(g_wds)

/**
 * wlan_rptr_vdev_set_params() - Set vdev params
 * @vdev: vdev object manager
 * api to set vdev params
 *
 * return: void
 */
void wlan_rptr_vdev_set_params(struct wlan_objmgr_vdev *vdev);

#if DBDC_REPEATER_SUPPORT
/**
 * wlan_rptr_conn_up_dbdc_process() - mlme up dbdc process
 * @vdev: vdev object manager
 * @dev: net device
 *
 * return: void
 */
void wlan_rptr_conn_up_dbdc_process(struct wlan_objmgr_vdev *vdev,
				    struct net_device *dev);

/**
 * wlan_rptr_conn_down_dbdc_process() - mlme down dbdc process
 * @vdev: vdev object manager
 * @dev: net device
 *
 * return: void
 */
void wlan_rptr_conn_down_dbdc_process(struct wlan_objmgr_vdev *vdev,
				      struct net_device *dev);
#endif

/**
 * wlan_rptr_pdev_ucfg_config_get() - Get ucfg pdev param
 * @pdev: pdev object manager
 * @param: pdev param to get
 *
 * return: param value
 */
uint32_t wlan_rptr_pdev_ucfg_config_get(struct wlan_objmgr_pdev *pdev,
					int param);

/**
 * wlan_rptr_pdev_ucfg_config_set() - Set ucfg pdev param
 * @pdev: pdev object manager
 * @param: pdev param to set
 * @value: value to set
 *
 * return: QDF_STATUS_SUCCESS if success and non-zero value on failure
 */
QDF_STATUS wlan_rptr_pdev_ucfg_config_set(struct wlan_objmgr_pdev *pdev,
					  int param, u8 value);

/**
 * wlan_rptr_pdev_extd_ioctl() - Rptr specific extd ioctl
 * @pdev: pdev object manager
 * @param: pdev param to set
 * @data: ptr to buffer
 *
 * return: 0 if success and negative value on failure
 */
int wlan_rptr_pdev_extd_ioctl(struct wlan_objmgr_pdev *pdev, int param,
			      char *data);

/**
 * wlan_rptr_vdev_get_qwrap_cflags() - Get qwrap cflags
 * @vdev: vdev object manager
 * @psta: psta
 * @mpsta: mpsta
 * @wrap: wrap
 *
 * return: void
 */
void wlan_rptr_vdev_get_qwrap_cflags(struct wlan_objmgr_vdev *vdev, bool *psta,
				     bool *mpsta, bool *wrap);

#if ATH_SUPPORT_WRAP
/**
 * wlan_rptr_vdev_create_complete() - Set vdev params on vdev create
 * complete
 * @vdev: vdev object manager
 * @dev: net device
 * api to set vdev params on vdev create complete
 *
 * return: void
 */
void
wlan_rptr_vdev_create_complete(struct wlan_objmgr_vdev *vdev,
			       struct net_device *dev);
#endif

/**
 * wlan_rptr_vdev_delete_start() - repeater code on vdev delete
 * @vdev: vdev object manager
 * api to execute repeater code on vdev delete
 *
 * return: void
 */
void
wlan_rptr_vdev_delete_start(struct wlan_objmgr_vdev *vdev);

/**
 * wlan_rptr_vdev_is_key_set_allowed - check if key set is allowed for vdev
 * @vdev- vdev object manager
 * @flags – key flags
 * return true if valid; otherwise false
 */
bool
wlan_rptr_vdev_is_key_set_allowed(struct wlan_objmgr_vdev *vdev,
				  uint16_t flags);

/**
 * wlan_rptr_vdev_ucfg_config - API for repeater vdev configuration
 * @vdev- vdev object manager
 * @param- config param
 * @value – config value
 * Return: QDF_STATUS_SUCCESS - for success and non-zero for failure
 */
QDF_STATUS
wlan_rptr_vdev_ucfg_config(struct wlan_objmgr_vdev *vdev, int param,
			   uint8_t value);

/**
 * wlan_rptr_vdev_is_scan_allowed - check if scan is allowed for vdev
 * @vdev- vdev object manager
 * return true if valid; otherwise false
 */
bool wlan_rptr_vdev_is_scan_allowed(struct wlan_objmgr_vdev *vdev);
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
				bool *des_bssid_set, struct scan_filter *filter);

/**
 * wlan_rptr_psta_validate_chan - validate psta channel
 * @vdev- vdev object manager
 * @freq - PSTA freq
 * return QDF_STATUS_SUCCESS if channel is correct; otherwise non-zero value
 */
QDF_STATUS
wlan_rptr_psta_validate_chan(struct wlan_objmgr_vdev *vdev, uint16_t freq);
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
					      uint16_t pdev_id);
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
					      uint16_t pdev_id);
#endif

/**
 * wlan_rptr_same_ssid_check - Check same SSID feature support
 * @vdev- vdev object manager
 * @ssid - ssid
 * @ssid_len- ssid length
 * return void
 */
void
wlan_rptr_same_ssid_check(struct wlan_objmgr_vdev *vdev,
			  u8 *ssid, u_int32_t ssid_len);
/**
 * wlan_rptr_ss_parse_scan_entries - parse scan entries
 * @vdev- vdev object manager
 * @event - scan event
 * return void
 */
void
wlan_rptr_ss_parse_scan_entries(struct wlan_objmgr_vdev *vdev,
				struct scan_event *event);
/**
 * wlan_rptr_validate_stavap_bssid - validate stavap bssid
 * when same ssid feature is enabled
 * @vdev- vdev object manager
 * @bssid - stavap bssid
 * return QDF_STATUS_SUCCESS if bssid is correct; otherwise non-zero value
 */
QDF_STATUS
wlan_rptr_validate_stavap_bssid(struct wlan_objmgr_vdev *vdev,
				u_int8_t *bssid);

/**
 * wlan_rptr_add_extender_ie - Add extender IE
 * @vdev- vdev object manager
 * @ftype- frame type
 * @frm - frame pointer
 * return frame pointer
 */
u_int8_t *
wlan_rptr_add_extender_ie(struct wlan_objmgr_vdev *vdev,
			  ieee80211_frame_type ftype, u_int8_t *frm);

/**
 * wlan_rptr_process_extender_ie - Process extender IE
 * @peer- peer object manager
 * @ie - extender IE pointer
 * @ftype- frame type
 * return void
 */
void
wlan_rptr_process_extender_ie(struct wlan_objmgr_peer *peer,
			      const u8 *ie, ieee80211_frame_type ftype);

/**
 * wlan_rptr_allow_client_conn - Check for client connection
 * @vdev- vdev object manager
 * @bssid- client mac address
 * return 1 to allow or 0 to not allow client connection
 */
bool
wlan_rptr_allow_client_conn(struct wlan_objmgr_vdev *vdev, uint8_t *bssid);

/**
 * wlan_rptr_validate_stavap_connection - Validate stavap connection
 * @vdev- vdev object manager
 * @bssid- stavap bssid
 * return 1 to allow or 0 to not allow stavap connection
 */
int
wlan_rptr_validate_stavap_connection(struct wlan_objmgr_vdev *vdev,
				     u_int8_t *bssid);

/**
 * wlan_rptr_s_ssid_vdev_connection_up - update same ssid feature info during
 * stavap connection up
 * @vdev- vdev object manager
 * @disconnect_rptr_clients- flag pointer to disconnect repeater clients
 * return void
 */
void
wlan_rptr_s_ssid_vdev_connection_up(struct wlan_objmgr_vdev *vdev,
				    u_int8_t *disconnect_rptr_clients);

/**
 * wlan_rptr_s_ssid_vdev_connection_down - update same ssid feature info during
 * stavap connection down
 * @vdev- vdev object manager
 * return void
 */
void
wlan_rptr_s_ssid_vdev_connection_down(struct wlan_objmgr_vdev *vdev);

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
			       u8 *disconnect_sec_stavap, u8 *bssid);
