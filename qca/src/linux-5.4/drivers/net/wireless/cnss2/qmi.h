/* Copyright (c) 2015-2018, The Linux Foundation. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*/

#ifndef _CNSS_QMI_H
#define _CNSS_QMI_H
#define CONFIG_CNSS2_QMI 1
#include "wlan_firmware_service_v01.h"

#define WLFW_SERVICE_INS_ID_V01_QCA8074		2
#define WLFW_SERVICE_INS_ID_V01_QCN6122		0x40
#define BDF_MAX_SIZE (256 * 1024)
#define BDF_TYPE_GOLDEN 0
#define BDF_TYPE_CALDATA 2
#define BDF_TYPE_EEPROM 3
#define BDF_TYPE_REGDB 4
#define BDF_TYPE_HDS 6
#define CALDATA_OFFSET(addr) (addr + (128 * 1024))
#define Q6_QDSS_ETR_SIZE_QCN9000 0x100000
#define Q6_M3_DUMP_SIZE_QCN9000 0x100000
#define QMI_HISTORY_SIZE 128

/* userpd_id for QCN6122 in multi pd arch */
#define QCN6122_0	1
#define QCN6122_1	2

/* node_id for QCN9000 */
#define QCN9000_0	0x20
#define QCN9000_1	0x21
#define QCN9000_2	0x22
#define QCN9000_3	0x23

#define QCN9224_0	0x30
#define QCN9224_1	0x31
#define QCN9224_2	0x32
#define QCN9224_3	0x33

/*NODE_ID_BASE is derived by qrtr_node_id in DTS + FW base node id 7 */
#define QCN9000_NODE_ID_BASE 0x27
#define QCN9224_NODE_ID_BASE 0x37
#define FW_ID_BASE 7

struct qmi_history {
	u16  msg_id;
	s8  error_msg;
	s8  resp_err_msg;
	u8  instance_id;
	u8  reserved[3];
	u64 timestamp;
};

extern struct qmi_history qmi_log[];
extern int qmi_history_index;


struct cnss_plat_data;
struct cnss_qmi_event_server_arrive_data {
	unsigned int node;
	unsigned int port;
};

#define QDSS_TRACE_SEG_LEN_MAX 32
#define QDSS_TRACE_FILE_NAME_MAX 16

struct cnss_mem_seg {
	u64 addr;
	u32 size;
};

struct cnss_qmi_event_qdss_trace_save_data {
	u32 total_size;
	u32 mem_seg_len;
	struct cnss_mem_seg mem_seg[QDSS_TRACE_SEG_LEN_MAX];
	char file_name[QDSS_TRACE_FILE_NAME_MAX + 1];
};

struct cnss_qmi_event_m3_dump_upload_req_data {
	u32 pdev_id;
	u64 addr;
	u64 size;
};

#ifdef CONFIG_CNSS2_QMI
#include "wlan_firmware_service_v01.h"
#include "coexistence_service_v01.h"
#include "ip_multimedia_subsystem_private_service_v01.h"

void cnss_dump_qmi_history(void);
int cnss_qmi_init(struct cnss_plat_data *plat_priv);
void cnss_qmi_deinit(struct cnss_plat_data *plat_priv);
unsigned int cnss_get_qmi_timeout(struct cnss_plat_data *plat_priv);
int cnss_wlfw_server_arrive(struct cnss_plat_data *plat_priv, void *data);
int cnss_wlfw_server_exit(struct cnss_plat_data *plat_priv);
int cnss_wlfw_respond_mem_send_sync(struct cnss_plat_data *plat_priv);
int cnss_wlfw_tgt_cap_send_sync(struct cnss_plat_data *plat_priv);
int cnss_wlfw_bdf_dnld_send_sync(struct cnss_plat_data *plat_priv,
				 u32 bdf_type);
int cnss_wlfw_m3_dnld_send_sync(struct cnss_plat_data *plat_priv);
int cnss_wlfw_wlan_mode_send_sync(struct cnss_plat_data *plat_priv,
				  enum cnss_driver_mode mode);
int cnss_wlfw_wlan_cfg_send_sync(struct cnss_plat_data *plat_priv,
				 struct cnss_wlan_enable_cfg *config,
				 const char *host_version);
int cnss_wlfw_athdiag_read_send_sync(struct cnss_plat_data *plat_priv,
				     u32 offset, u32 mem_type,
				     u32 data_len, u8 *data);
int cnss_wlfw_athdiag_write_send_sync(struct cnss_plat_data *plat_priv,
				      u32 offset, u32 mem_type,
				      u32 data_len, u8 *data);
int cnss_wlfw_ini_send_sync(struct cnss_plat_data *plat_priv,
			    u8 fw_log_mode);
int cnss_wlfw_antenna_switch_send_sync(struct cnss_plat_data *plat_priv);
int cnss_wlfw_antenna_grant_send_sync(struct cnss_plat_data *plat_priv);
int cnss_wlfw_dynamic_feature_mask_send_sync(struct cnss_plat_data *plat_priv);
int cnss_register_coex_service(struct cnss_plat_data *plat_priv);
void cnss_unregister_coex_service(struct cnss_plat_data *plat_priv);
int coex_antenna_switch_to_wlan_send_sync_msg(struct cnss_plat_data *plat_priv);
int coex_antenna_switch_to_mdm_send_sync_msg(struct cnss_plat_data *plat_priv);
int cnss_wlfw_qdss_trace_mem_info_send_sync(struct cnss_plat_data *plat_priv);
int cnss_register_ims_service(struct cnss_plat_data *plat_priv);
void cnss_unregister_ims_service(struct cnss_plat_data *plat_priv);
int cnss_wlfw_m3_dump_upload_done_send_sync(struct cnss_plat_data *plat_priv,
					    u32 pdev_id, int status);
int cnss_wlfw_device_info_send_sync(struct cnss_plat_data *plat_priv);
#else
#define QMI_WLFW_TIMEOUT_MS		10000

static inline int cnss_qmi_init(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline void cnss_qmi_deinit(struct cnss_plat_data *plat_priv)
{
}

static inline
unsigned int cnss_get_qmi_timeout(struct cnss_plat_data *plat_priv)
{
	return QMI_WLFW_TIMEOUT_MS;
}

static inline int cnss_wlfw_server_arrive(struct cnss_plat_data *plat_priv,
					  void *data)
{
	return 0;
}

static inline int cnss_wlfw_server_exit(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
int cnss_wlfw_respond_mem_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline int cnss_wlfw_tgt_cap_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline int cnss_wlfw_bdf_dnld_send_sync(struct cnss_plat_data *plat_priv,
					       u32 bdf_type)
{
	return 0;
}

static inline int cnss_wlfw_m3_dnld_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
int cnss_wlfw_wlan_mode_send_sync(struct cnss_plat_data *plat_priv,
				  enum cnss_driver_mode mode)
{
	return 0;
}

static inline
int cnss_wlfw_wlan_cfg_send_sync(struct cnss_plat_data *plat_priv,
				 struct cnss_wlan_enable_cfg *config,
				 const char *host_version)
{
	return 0;
}

static inline
int cnss_wlfw_athdiag_read_send_sync(struct cnss_plat_data *plat_priv,
				     u32 offset, u32 mem_type,
				     u32 data_len, u8 *data)
{
	return 0;
}

static inline
int cnss_wlfw_athdiag_write_send_sync(struct cnss_plat_data *plat_priv,
				      u32 offset, u32 mem_type,
				      u32 data_len, u8 *data)
{
	return 0;
}

static inline
int cnss_wlfw_ini_send_sync(struct cnss_plat_data *plat_priv,
			    u8 fw_log_mode)
{
	return 0;
}

static inline
int cnss_wlfw_antenna_switch_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
int cnss_wlfw_antenna_grant_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
int cnss_wlfw_dynamic_feature_mask_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
int cnss_register_coex_service(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
void cnss_unregister_coex_service(struct cnss_plat_data *plat_priv) {}

static inline
int coex_antenna_switch_to_wlan_send_sync_msg(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
int coex_antenna_switch_to_mdm_send_sync_msg(struct cnss_plat_data *plat_priv)

static inline
int cnss_wlfw_qdss_trace_mem_info_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
int cnss_register_ims_service(struct cnss_plat_data *plat_priv)
{
	return 0;
}

static inline
void cnss_unregister_ims_service(struct cnss_plat_data *plat_priv) {}

static inline
int cnss_wlfw_m3_dump_upload_done_send_sync(struct cnss_plat_data *plat_priv,
					    u32 pdev_id, int status)
{
	return 0;
}

static inline
int cnss_wlfw_device_info_send_sync(struct cnss_plat_data *plat_priv)
{
	return 0;
}
#endif /* CONFIG_CNSS2_QMI */

#endif /* _CNSS_QMI_H */
