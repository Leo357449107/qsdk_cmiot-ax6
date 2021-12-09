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

#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/mutex.h>
#include <linux/soc/qcom/qmi.h>
#include <linux/of.h>

#include "bus.h"
#include "debug.h"
#include "main.h"
#include "qmi.h"
#include "pci.h"

#define WLFW_SERVICE_INS_ID_V01		1
#define WLFW_CLIENT_ID			0x4b4e454c
#define MAX_BDF_FILE_NAME		64
#define BDF_FILE_NAME_PREFIX		"bdwlan"
#define ELF_BDF_FILE_NAME		"bdwlan.elf"
#define ELF_BDF_FILE_NAME_PREFIX	"bdwlan.e"
#define BIN_BDF_FILE_NAME		"bdwlan.bin"
#define BIN_BDF_FILE_NAME_PREFIX	"bdwlan.b"
#define DEFAULT_BDF_FILE_NAME		"bdwlan.bin"
#define BDF_WIN_FILE_NAME_PREFIX	"bdwlan.b"
#define REGDB_FILE_NAME			"regdb.bin"
#define DUMMY_BDF_FILE_NAME		"bdwlan.dmy"
#define HDS_FILE_NAME			"hds.bin"

#define DEFAULT_CAL_FILE_NAME		"caldata.bin"
#define CAL_FILE_NAME_PREFIX		"caldata.b"
#define DEFAULT_CAL_FILE_PREFIX         "caldata_"
#define DEFAULT_CAL_FILE_SUFFIX         ".bin"
#define MAX_HW_LINKS			2

#ifdef CONFIG_CNSS2_DEBUG
static unsigned int qmi_timeout = 5000;
module_param(qmi_timeout, uint, 0600);
MODULE_PARM_DESC(qmi_timeout, "Timeout for QMI message in milliseconds");
EXPORT_SYMBOL(qmi_timeout);

#define QMI_WLFW_TIMEOUT_MS		qmi_timeout
#else
#define QMI_WLFW_TIMEOUT_MS		(plat_priv->ctrl_params.qmi_timeout)
#endif

#define QMI_WLFW_TIMEOUT_JF		msecs_to_jiffies(QMI_WLFW_TIMEOUT_MS)
#define COEX_TIMEOUT			QMI_WLFW_TIMEOUT_JF
#define IMS_TIMEOUT                     QMI_WLFW_TIMEOUT_JF

#define QMI_WLFW_MAX_RECV_BUF_SIZE	SZ_8K
unsigned int qca8074_fw_mem_mode = 0xFF;
module_param(qca8074_fw_mem_mode, uint, 0600);
MODULE_PARM_DESC(qca8074_fw_mem_mode, "qca8074_fw_mem_mode");

#ifdef CONFIG_CNSS2_DEBUG
static bool bdf_bypass = true;
module_param(bdf_bypass, bool, 0600);
MODULE_PARM_DESC(bdf_bypass, "If BDF is not found, send dummy BDF to FW");
#endif

unsigned int num_wlan_clients;
module_param(num_wlan_clients, uint, 0600);
MODULE_PARM_DESC(num_wlan_clients, "num_wlan_clients");

unsigned int num_wlan_vaps;
module_param(num_wlan_vaps, uint, 0600);
MODULE_PARM_DESC(num_wlan_vaps, "num_wlan_vaps");

unsigned int enable_mlo_support;
module_param(enable_mlo_support, uint, 0600);
MODULE_PARM_DESC(enable_mlo_support, "enable_mlo_support");

struct qmi_history qmi_log[QMI_HISTORY_SIZE];
int qmi_history_index;
DEFINE_SPINLOCK(qmi_log_spinlock);
struct wlfw_request_mem_ind_msg_v01 ind_message = {0};

void cnss_dump_qmi_history(void)
{
	int i;

	pr_err("qmi_history_index [%d]\n", ((qmi_history_index - 1) &
		(QMI_HISTORY_SIZE - 1)));
	for (i = 0; i < QMI_HISTORY_SIZE; i++) {
		if (qmi_log[i].msg_id)
			pr_err(
			"qmi_history[%d]:timestamp[%llu] instance_id [0x%X] msg_id[0x%X] err[%d] resp_err[%d]\n",
			i, qmi_log[i].timestamp,
			qmi_log[i].instance_id,
			qmi_log[i].msg_id,
			qmi_log[i].error_msg,
			qmi_log[i].resp_err_msg);
	}
}
EXPORT_SYMBOL(cnss_dump_qmi_history);

void qmi_record(u8 instance_id, u16 msg_id, s8 error_msg, s8 resp_err_msg)
{
	spin_lock(&qmi_log_spinlock);
	qmi_log[qmi_history_index].instance_id = instance_id;
	qmi_log[qmi_history_index].msg_id = msg_id;

	if (error_msg < 0 || resp_err_msg != 0)
		qmi_log[qmi_history_index].error_msg = error_msg;
	else
		qmi_log[qmi_history_index].error_msg = 0;

	qmi_log[qmi_history_index].resp_err_msg = resp_err_msg;
	qmi_log[qmi_history_index++].timestamp = ktime_to_ms(ktime_get());

	qmi_history_index &= (QMI_HISTORY_SIZE - 1);
	spin_unlock(&qmi_log_spinlock);
}

static char *cnss_qmi_mode_to_str(enum cnss_driver_mode mode)
{
	switch (mode) {
	case CNSS_MISSION:
		return "MISSION";
	case CNSS_FTM:
		return "FTM";
	case CNSS_EPPING:
		return "EPPING";
	case CNSS_WALTEST:
		return "WALTEST";
	case CNSS_OFF:
		return "OFF";
	case CNSS_CCPM:
		return "CCPM";
	case CNSS_QVIT:
		return "QVIT";
	case CNSS_CALIBRATION:
		return "COLDBOOT CALIBRATION";
	case QMI_WLFW_FTM_CALIBRATION_V01:
		return "FTM COLDBOOT CALIBRATION";
	default:
		return "UNKNOWN";
	}
};

static int cnss_wlfw_ind_register_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_ind_register_req_msg_v01 *req;
	struct wlfw_ind_register_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;
	int resp_error_msg = 0;

	cnss_pr_dbg("Sending indication register message, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->client_id_valid = 1;
	req->client_id = WLFW_CLIENT_ID;
	req->fw_ready_enable_valid = 1;
	req->fw_ready_enable = 1;
	req->request_mem_enable_valid = 1;
	req->request_mem_enable = 1;
	req->fw_mem_ready_enable_valid = 1;
	req->fw_mem_ready_enable = 1;
	req->fw_init_done_enable_valid = 1;
	req->fw_init_done_enable = 1;
	req->pin_connect_result_enable_valid = 0;
	req->pin_connect_result_enable = 0;
	req->cal_done_enable_valid = 1;
	req->cal_done_enable = 1;
	req->qdss_trace_req_mem_enable_valid = 1;
	req->qdss_trace_req_mem_enable = 1;
	req->qdss_trace_save_enable_valid = 1;
	req->qdss_trace_save_enable = 1;
	req->qdss_trace_free_enable_valid = 1;
	req->qdss_trace_free_enable = 1;
	req->m3_dump_upload_req_enable_valid = 1;
	req->m3_dump_upload_req_enable = 1;

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_IND_REGISTER_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_ind_register_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for indication register request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_IND_REGISTER_REQ_V01,
			       WLFW_IND_REGISTER_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_ind_register_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send indication register request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of indication register request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Indication register request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

	if (resp->fw_status_valid) {
		if (resp->fw_status & QMI_WLFW_ALREADY_REGISTERED_V01) {
			ret = -EALREADY;
			goto qmi_registered;
		}
	}

	kfree(req);
	kfree(resp);
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_IND_REGISTER_REQ_V01, ret, resp_error_msg);
	return 0;
out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_IND_REGISTER_REQ_V01, ret, resp_error_msg);
	CNSS_ASSERT(0);

qmi_registered:
	kfree(req);
	kfree(resp);
	return ret;
}

static int cnss_wlfw_host_cap_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_host_cap_req_msg_v01 *req;
	struct wlfw_host_cap_resp_msg_v01 *resp;
	struct wlfw_host_mlo_chip_info_s_v01 *info;
	struct qmi_txn txn;
	int ret = 0, i;
	int resp_error_msg = 0;
	const char *model = NULL;
	struct device_node *root, *mlo_config = NULL, *chipnp;
	struct device *dev = &plat_priv->plat_dev->dev;
	struct device *bus_dev;
	struct pci_dev *pcidev;
	int chip_id;

	cnss_pr_dbg("Sending host capability message, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}
	req->num_clients_valid = 1;
	if (test_bit(ENABLE_DAEMON_SUPPORT,
		     &plat_priv->ctrl_params.quirks))
		req->num_clients = 2;
	else
		req->num_clients = 1;

	req->num_clients = 1;
	if (plat_priv->daemon_support)
		req->num_clients = 2;

	cnss_pr_dbg("Number of clients is %d\n", req->num_clients);

	req->mem_cfg_mode = plat_priv->tgt_mem_cfg_mode;
	req->mem_cfg_mode_valid = 1;
	cnss_pr_info("device_id : %lu mem mode : [%d]\n",
		     plat_priv->device_id,
		     plat_priv->tgt_mem_cfg_mode);

	req->bdf_support_valid = 1;
	req->bdf_support = 1;

	req->m3_support_valid = 0;
	req->m3_support = 0;

	req->m3_cache_support_valid = 0;
	req->m3_cache_support = 0;

	req->cal_done_valid = 1;
	req->cal_done = plat_priv->cal_done;
	cnss_pr_dbg("Calibration done is %d\n", plat_priv->cal_done);

	root = of_find_node_by_path("/");
	if (root) {
		model = of_get_property(root, "model", NULL);
		if (model) {
			req->platform_name_valid = 1;
			strlcpy(req->platform_name, model,
				QMI_WLFW_MAX_PLATFORM_NAME_LEN_V01);
			cnss_pr_info("platform name: %s", req->platform_name);
		}
	}

	if (!of_property_read_u32(dev->of_node, "gpios-len", &req->gpios_len)) {
		if (req->gpios_len > QMI_WLFW_MAX_NUM_GPIO_V01) {
			cnss_pr_err("Invalid GPIOs array length %d\n",
				    req->gpios_len);
			ret = -EINVAL;
			goto err;
		}

		if (of_property_read_u32_array(dev->of_node, "gpios",
					       req->gpios, req->gpios_len)) {
			cnss_pr_err("Failed to get gpios from device tree\n");
			ret = -EINVAL;
			goto err;
		}

		req->gpios_valid = 1;
		cnss_pr_info("Sending %d GPIO entries in Host Capabilities\n",
			     req->gpios_len);
	}

	/* update MLO configuration */
	if (plat_priv->device_id == QCN9224_DEVICE_ID)
		mlo_config = of_find_node_by_name(NULL, "mlo_group0");

	if (enable_mlo_support && mlo_config) {
		pcidev = (struct pci_dev *)plat_priv->pci_dev;
		bus_dev = &pcidev->dev;

		req->mlo_capable_valid = 1;
		req->mlo_capable = 1;

		chip_id = cnss_get_mlo_chip_id(bus_dev);
		if (ret < 0) {
			cnss_pr_err("Unable to get chip id\n");
			ret = -EINVAL;
			goto err;
		}
		req->mlo_chip_id = (u16)chip_id;
		req->mlo_chip_id_valid = 1;

		req->mlo_group_id = 0;
		req->mlo_group_id_valid = 1;

		if (of_property_read_u16(mlo_config,
					 "mlo_max_num_peer",
					 &req->max_mlo_peer)) {
			cnss_pr_err("mlo_max_num_peer is not configured\n");
			ret = -EINVAL;
			goto err;
		}
		req->max_mlo_peer_valid = 1;

		if (of_property_read_u8(mlo_config,
					"mlo_num_chips",
					&req->mlo_num_chips)) {
			cnss_pr_err("mlo_num_chips is not configured\n");
			ret = -EINVAL;
			goto err;
		}
		req->mlo_num_chips_valid = 1;

		req->mlo_chip_info_valid = 1;
		for (i = 0; i < req->mlo_num_chips; i++) {
			chipnp = of_parse_phandle(mlo_config, "chips", i);
			if (!chipnp) {
				cnss_pr_err("chip info is null. num chips %d\n",
					    req->mlo_num_chips);
				ret = -EINVAL;
				goto err;
			}
			info = &req->mlo_chip_info[i];
			if (of_property_read_u8(chipnp, "chip_id",
						&info->chip_id)) {
				cnss_pr_err("chip_id is not configured\n");
				ret = -EINVAL;
				goto err_chip_info;
			}

			if (of_property_read_u8(chipnp, "num_local_links",
						&info->num_local_links)) {
				cnss_pr_err("num_local_links is missing\n");
				ret = -EINVAL;
				goto err_chip_info;
			}

			if (of_property_read_u8_array(chipnp, "hw_link_ids",
						      &info->hw_link_id[0],
						      MAX_HW_LINKS)) {
				cnss_pr_err("hw_link_ids is not configured\n");
				ret = -EINVAL;
				goto err_chip_info;
			}

			if (of_property_read_u8_array(chipnp,
						      "valid_mlo_link_ids",
						      &info->valid_mlo_link_id[0],
						      MAX_HW_LINKS)) {
				cnss_pr_err("valid_mlo_link_ids read error\n");
				ret = -EINVAL;
				goto err_chip_info;
			}
			of_node_put(chipnp);
		}
	}

	if (num_wlan_clients) {
		req->num_wlan_clients_valid = 1;
		req->num_wlan_clients = num_wlan_clients;
		cnss_pr_info("Sending %d Number of WLAN clients in Host Capabilities\n",
			     req->num_wlan_clients);
	} else if (!of_property_read_u16(dev->of_node, "num_wlan_clients",
					 &req->num_wlan_clients)) {
		req->num_wlan_clients_valid = 1;
		cnss_pr_info("Sending %d Number of WLAN clients in Host Capabilities\n",
			     req->num_wlan_clients);
	}

	if (num_wlan_vaps) {
		req->num_wlan_vaps_valid = 1;
		req->num_wlan_vaps = num_wlan_vaps;
		cnss_pr_info("Sending %d Number of WLAN Vaps in Host Capabilities\n",
			     req->num_wlan_vaps);
	} else if (!of_property_read_u8(dev->of_node, "num_wlan_vaps",
					&req->num_wlan_vaps)) {
		req->num_wlan_vaps_valid = 1;
		cnss_pr_info("Sending %d Number of WLAN vaps in Host Capabilities\n",
			     req->num_wlan_vaps);
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_HOST_CAP_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_host_cap_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for host capability request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_HOST_CAP_REQ_V01,
			       WLFW_HOST_CAP_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_host_cap_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send host capability request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of host capability request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Host capability request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_HOST_CAP_REQ_V01, ret, resp_error_msg);

	kfree(req);
	kfree(resp);
	return 0;

err_chip_info:
	of_node_put(chipnp);
err:
	kfree(req);
	kfree(resp);
out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_HOST_CAP_REQ_V01, ret, resp_error_msg);
	CNSS_ASSERT(0);

	return ret;
}

int cnss_wlfw_respond_mem_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_respond_mem_req_msg_v01 *req;
	struct wlfw_respond_mem_resp_msg_v01 *resp;
	struct qmi_txn txn;
	struct cnss_fw_mem *fw_mem = plat_priv->fw_mem;
	int ret = 0, i;
	int resp_error_msg = 0;

	cnss_pr_dbg("Sending respond memory message, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->mem_seg_len = plat_priv->fw_mem_seg_len;
	for (i = 0; i < req->mem_seg_len; i++) {
		if (plat_priv->cold_boot_support &&
		    (!fw_mem[i].pa || !fw_mem[i].size)) {
			if (fw_mem[i].type == 0) {
				cnss_pr_err("Invalid memory for FW type, segment = %d\n",
					    i);
				ret = -EINVAL;
				goto out;
			}
			cnss_pr_err("Memory for FW is not available for type: %u\n",
				    fw_mem[i].type);
			ret = -ENOMEM;
			goto out;
		}

		cnss_pr_dbg("Memory for FW, va: 0x%pK, pa: %pa, size: 0x%zx, type: %u\n",
			    fw_mem[i].va, &fw_mem[i].pa,
			    fw_mem[i].size, fw_mem[i].type);

		req->mem_seg[i].addr = fw_mem[i].pa;
		req->mem_seg[i].size = fw_mem[i].size;
		req->mem_seg[i].type = fw_mem[i].type;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_RESPOND_MEM_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_respond_mem_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for respond memory request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_RESPOND_MEM_REQ_V01,
			       WLFW_RESPOND_MEM_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_respond_mem_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send respond memory request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of respond memory request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Respond memory request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_RESPOND_MEM_REQ_V01, ret, resp_error_msg);

	kfree(req);
	kfree(resp);
	return 0;
out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_RESPOND_MEM_REQ_V01, ret, resp_error_msg);
	CNSS_ASSERT(0);
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_tgt_cap_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_cap_req_msg_v01 *req;
	struct wlfw_cap_resp_msg_v01 *resp;
	struct qmi_txn txn;
	char *fw_build_timestamp;
	int i, ret = 0;
	int resp_error_msg = 0;

	cnss_pr_dbg("Sending target capability message, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_CAP_REQ_V01, ret, resp_error_msg);

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_cap_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for target capability request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_CAP_REQ_V01,
			       WLFW_CAP_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_cap_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send respond target capability request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of target capability request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Target capability request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_CAP_RESP_V01, ret, resp_error_msg);
	if (resp->chip_info_valid) {
		plat_priv->chip_info.chip_id = resp->chip_info.chip_id;
		plat_priv->chip_info.chip_family = resp->chip_info.chip_family;
	}
	if (resp->board_info_valid)
		plat_priv->board_info.board_id = resp->board_info.board_id;
	else
		plat_priv->board_info.board_id = 0xFF;
	if (resp->soc_info_valid)
		plat_priv->soc_info.soc_id = resp->soc_info.soc_id;
	if (resp->fw_version_info_valid) {
		plat_priv->fw_version_info.fw_version =
			resp->fw_version_info.fw_version;
		fw_build_timestamp = resp->fw_version_info.fw_build_timestamp;
		fw_build_timestamp[QMI_WLFW_MAX_TIMESTAMP_LEN] = '\0';
		strlcpy(plat_priv->fw_version_info.fw_build_timestamp,
			resp->fw_version_info.fw_build_timestamp,
			QMI_WLFW_MAX_TIMESTAMP_LEN + 1);
	}
	if (resp->time_freq_hz_valid) {
		plat_priv->device_freq_hz = resp->time_freq_hz;
		cnss_pr_dbg("Device frequency is %d HZ\n",
			    plat_priv->device_freq_hz);
	}
#ifdef CNSS2_CPR
	if (resp->voltage_mv_valid) {
		plat_priv->cpr_info.voltage = resp->voltage_mv;
		cnss_pr_dbg("Voltage for CPR: %dmV\n",
			    plat_priv->cpr_info.voltage);
		cnss_update_cpr_info(plat_priv);
	}
	if (resp->otp_version_valid)
		plat_priv->otp_version = resp->otp_version;
#endif

	if (resp->eeprom_caldata_read_timeout_valid)
		plat_priv->eeprom_caldata_read_timeout =
			resp->eeprom_caldata_read_timeout;

	if (resp->dev_mem_info_valid) {
		for (i = 0; i < QMI_WLFW_MAX_DEV_MEM_NUM_V01; i++) {
			plat_priv->dev_mem_info[i].start =
				resp->dev_mem_info[i].start;
			plat_priv->dev_mem_info[i].size =
				resp->dev_mem_info[i].size;
			cnss_pr_info("Device memory info[%d]: start = 0x%llx, size = 0x%llx\n",
				     i, plat_priv->dev_mem_info[i].start,
				     plat_priv->dev_mem_info[i].size);
		}
	}

	cnss_pr_info("Target capability: chip_id: 0x%x, chip_family: 0x%x, board_id: 0x%x, soc_id: 0x%x, fw_version: 0x%x, fw_build_timestamp: %s, otp_version: 0x%x eeprom_caldata_read_timeout %ds\n",
		     plat_priv->chip_info.chip_id,
		     plat_priv->chip_info.chip_family,
		     plat_priv->board_info.board_id, plat_priv->soc_info.soc_id,
		     plat_priv->fw_version_info.fw_version,
		     plat_priv->fw_version_info.fw_build_timestamp,
		     plat_priv->otp_version,
		     plat_priv->eeprom_caldata_read_timeout);

	kfree(req);
	kfree(resp);
	return 0;

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_CAP_RESP_V01, ret, resp_error_msg);
	CNSS_ASSERT(0);
	kfree(req);
	kfree(resp);
	return ret;
}

static int cnss_wlfw_load_bdf(struct wlfw_bdf_download_req_msg_v01 *req,
			      struct cnss_plat_data *plat_priv,
			      unsigned int remaining,
			      uint8_t bdf_type)
{
	int ret;
	char filename[30];
	const struct firmware *fw;
	char *bdf_addr, *folder;
	unsigned int bdf_addr_pa, location[MAX_TGT_MEM_MODES], board_id;
	int size;
	struct device *dev;

	dev = &plat_priv->plat_dev->dev;

	switch (plat_priv->device_id) {
	case QCA6018_DEVICE_ID:
		folder = "IPQ6018/";
		break;
	case QCA5018_DEVICE_ID:
		folder = "IPQ5018/";
		break;
	case QCN6122_DEVICE_ID:
		folder = "qcn6122/";
		break;
	case QCA9574_DEVICE_ID:
		folder = "IPQ9574/";
		break;
	default:
		folder = "IPQ8074/";
		break;
	}

	switch (bdf_type) {
	case BDF_TYPE_GOLDEN:
		if (plat_priv->board_info.board_id_override) {
			cnss_pr_info("Using Boardid from bootargs:0x%02x\n",
				     plat_priv->board_info.board_id_override);
			snprintf(filename, sizeof(filename),
				 "%s" BDF_WIN_FILE_NAME_PREFIX "%02x", folder,
				 plat_priv->board_info.board_id_override);
		} else if (!of_property_read_u32(dev->of_node, "qcom,board_id",
					  &board_id)) {
			if ((board_id == 0xFF) &&
			    (plat_priv->board_info.board_id == 0xFF)) {
				snprintf(filename, sizeof(filename),
					 "%s" DEFAULT_BDF_FILE_NAME, folder);
			} else if (board_id == 0xFF) {
				snprintf(filename, sizeof(filename),
					 "%s" BDF_WIN_FILE_NAME_PREFIX "%02x",
					 folder,
					 plat_priv->board_info.board_id);
			} else {
				if (board_id != plat_priv->board_info.board_id)
					cnss_pr_info(
					"Boardid from dts:%02x,FW:%02x\n",
					board_id,
					plat_priv->board_info.board_id);

				snprintf(filename, sizeof(filename),
					 "%s" BDF_WIN_FILE_NAME_PREFIX "%02x",
					 folder, board_id);
			}
		} else {
			cnss_pr_info("No board_id entry in device tree\n");
			if (plat_priv->board_info.board_id == 0xFF)
				snprintf(filename, sizeof(filename),
					 "%s" DEFAULT_BDF_FILE_NAME, folder);
			else
				snprintf(filename, sizeof(filename),
					 "%s" BDF_WIN_FILE_NAME_PREFIX "%02x",
					 folder,
					 plat_priv->board_info.board_id);
		}
		break;
	case BDF_TYPE_CALDATA:
		if (plat_priv->device_id == QCN6122_DEVICE_ID) {
			snprintf(filename, sizeof(filename),
				 "%s" DEFAULT_CAL_FILE_PREFIX
				 "%d" DEFAULT_CAL_FILE_SUFFIX,
				 folder, plat_priv->userpd_id);
		} else {
			snprintf(filename, sizeof(filename),
				 "%s" DEFAULT_CAL_FILE_NAME, folder);
		}
		break;
	case BDF_TYPE_HDS:
		snprintf(filename, sizeof(filename),
			 "%s" HDS_FILE_NAME, folder);
		break;
	case BDF_TYPE_REGDB:
		snprintf(filename, sizeof(filename),
			 "%s" REGDB_FILE_NAME, folder);
		break;
	default:
		return -EINVAL;
	}

	ret = request_firmware(&fw, filename, &plat_priv->plat_dev->dev);
	if (ret) {
		cnss_pr_err("Failed to get BDF file %s (%d)", filename, ret);
		return ret;
	}
	size = fw->size;
	if (of_property_read_u32_array(dev->of_node, "qcom,bdf-addr", location,
				       ARRAY_SIZE(location))) {
		pr_err("Error: No bdf_addr in device_tree\n");
		CNSS_ASSERT(0);
		goto out;
	}
	CNSS_ASSERT(plat_priv->tgt_mem_cfg_mode < ARRAY_SIZE(location));
	bdf_addr_pa = location[plat_priv->tgt_mem_cfg_mode];
	bdf_addr = ioremap(bdf_addr_pa, BDF_MAX_SIZE);
	if (!bdf_addr) {
		cnss_pr_err("ERROR. not able to ioremap BDF location\n");
		ret = -EIO;
		CNSS_ASSERT(0);
	}
	if (size != 0 && size <= BDF_MAX_SIZE) {
		if (bdf_type == BDF_TYPE_GOLDEN ||
		    bdf_type == BDF_TYPE_HDS ||
		    bdf_type == BDF_TYPE_REGDB) {
			cnss_pr_info("BDF location : 0x%x\n", bdf_addr_pa);
			cnss_pr_info("BDF %s size %d\n",
				     filename, (unsigned int)fw->size);
			memcpy(bdf_addr, fw->data, fw->size);
		}
		if (bdf_type == BDF_TYPE_CALDATA) {
			cnss_pr_info("per device BDF location : 0x%x\n",
				     CALDATA_OFFSET(bdf_addr_pa));
			memcpy(CALDATA_OFFSET(bdf_addr), fw->data, fw->size);
			cnss_pr_info("CALDATA %s size %d offset 0x%x\n",
				     filename, (unsigned int)fw->size,
				     CALDATA_OFFSET(0));
		}
		req->total_size_valid = 1;
		req->total_size = size;
		req->data_valid = 0;
		req->end_valid = 1;
		req->end = 1;
		req->data_len = remaining;
		req->bdf_type = bdf_type;
		req->bdf_type_valid = 1;
	} else {
		cnss_pr_info("bdf size %d > segsz %d\n", size, BDF_MAX_SIZE);
		req->data_len = remaining;
		req->end = 1;
	}
	iounmap(bdf_addr);
out:
	if (fw)
		release_firmware(fw);
	return ret;
}

int cnss_wlfw_bdf_dnld_send_sync(struct cnss_plat_data *plat_priv,
				 u32 bdf_type)
{
	struct qmi_txn txn;
	char filename[MAX_BDF_FILE_NAME];
	const struct firmware *fw_entry = NULL;
	const u8 *temp;
	char *folder;
	struct device *dev;
	unsigned int remaining, id = 0;
	struct wlfw_bdf_download_req_msg_v01 *req;
	struct wlfw_bdf_download_resp_msg_v01 *resp;
	int ret = 0, node_id_base;
	int resp_error_msg = 0;
	u8 fw_bdf_type = BDF_TYPE_GOLDEN;

	cnss_pr_dbg("Sending BDF download message, state: 0x%lx, type: %d\n",
		    plat_priv->driver_state, bdf_type);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	folder = (plat_priv->device_id == QCN9000_DEVICE_ID) ? "qcn9000/" :
		 (plat_priv->device_id == QCN9224_DEVICE_ID) ? "qcn9224/" : "";
	switch (bdf_type) {
	case CNSS_BDF_ELF:
		if (plat_priv->board_info.board_id == 0xFF)
			snprintf(filename, sizeof(filename), ELF_BDF_FILE_NAME);
		else if (plat_priv->board_info.board_id < 0xFF)
			snprintf(filename, sizeof(filename),
				 "%s" ELF_BDF_FILE_NAME_PREFIX "%02x",
				 folder, plat_priv->board_info.board_id);
		else
			snprintf(filename, sizeof(filename),
				 "%s" BDF_FILE_NAME_PREFIX "%02x.e%02x", folder,
				 plat_priv->board_info.board_id >> 8 & 0xFF,
				 plat_priv->board_info.board_id & 0xFF);
		break;
	case CNSS_BDF_BIN:
		if (plat_priv->board_info.board_id == 0xFF)
			snprintf(filename, sizeof(filename), BIN_BDF_FILE_NAME);
		else if (plat_priv->board_info.board_id < 0xFF)
			snprintf(filename, sizeof(filename),
				 "%s" BIN_BDF_FILE_NAME_PREFIX "%02x", folder,
				 plat_priv->board_info.board_id);
		else
			snprintf(filename, sizeof(filename),
				 "%s" BDF_FILE_NAME_PREFIX "%02x.b%02x", folder,
				 plat_priv->board_info.board_id >> 8 & 0xFF,
				 plat_priv->board_info.board_id & 0xFF);
		break;
	case CNSS_BDF_DUMMY:
		cnss_pr_dbg("CNSS_BDF_DUMMY is set, sending dummy BDF\n");
		snprintf(filename, sizeof(filename), DUMMY_BDF_FILE_NAME);
		temp = DUMMY_BDF_FILE_NAME;
		remaining = MAX_BDF_FILE_NAME;
		goto bypass_bdf;
	case CNSS_BDF_WIN:
		if ((plat_priv->device_id == QCN9000_DEVICE_ID ||
		     plat_priv->device_id == QCN9224_DEVICE_ID) &&
		    !plat_priv->board_info.board_id_override) {
			dev = &plat_priv->plat_dev->dev;
			if (!of_property_read_u32(dev->of_node, "board_id",
						  &id)) {
				plat_priv->board_info.board_id_override = id;
			}
		}

		if ((plat_priv->device_id == QCN9000_DEVICE_ID ||
		     plat_priv->device_id == QCN9224_DEVICE_ID) &&
		    plat_priv->board_info.board_id_override)
			snprintf(filename, sizeof(filename),
				 "%s" BDF_WIN_FILE_NAME_PREFIX "%02x", folder,
				 plat_priv->board_info.board_id_override);
		else if (plat_priv->board_info.board_id == 0xFF)
			snprintf(filename, sizeof(filename),
				 "%s" DEFAULT_BDF_FILE_NAME, folder);
		else
			snprintf(filename, sizeof(filename),
				 "%s" BDF_WIN_FILE_NAME_PREFIX "%02x", folder,
				 plat_priv->board_info.board_id);

		if (plat_priv->device_id == QCA8074_DEVICE_ID ||
		    plat_priv->device_id == QCA8074V2_DEVICE_ID ||
		    plat_priv->device_id == QCA5018_DEVICE_ID ||
		    plat_priv->device_id == QCN6122_DEVICE_ID ||
		    plat_priv->device_id == QCA6018_DEVICE_ID ||
		    plat_priv->device_id == QCA9574_DEVICE_ID) {
			temp = filename;
			remaining = MAX_BDF_FILE_NAME;
			goto bypass_bdf;
		}
		break;
	case CNSS_CALDATA_WIN:
		fw_bdf_type = BDF_TYPE_CALDATA;
		if (plat_priv->device_id == QCN9000_DEVICE_ID ||
		    plat_priv->device_id == QCN9224_DEVICE_ID) {
			if (plat_priv->device_id == QCN9224_DEVICE_ID)
				node_id_base = QCN9224_NODE_ID_BASE;
			else
				node_id_base = QCN9000_NODE_ID_BASE;

			snprintf(filename, sizeof(filename),
				 "%s" DEFAULT_CAL_FILE_PREFIX
				 "%d" DEFAULT_CAL_FILE_SUFFIX,
				 folder,
				 (plat_priv->wlfw_service_instance_id -
				  (node_id_base - 1)));
		} else {
			snprintf(filename, sizeof(filename),
				 "%s" DEFAULT_CAL_FILE_NAME, folder);
		}

		if (plat_priv->device_id == QCA8074_DEVICE_ID ||
		    plat_priv->device_id == QCA8074V2_DEVICE_ID ||
		    plat_priv->device_id == QCA5018_DEVICE_ID ||
		    plat_priv->device_id == QCN6122_DEVICE_ID ||
		    plat_priv->device_id == QCA6018_DEVICE_ID ||
		    plat_priv->device_id == QCA9574_DEVICE_ID) {
			temp = filename;
			remaining = MAX_BDF_FILE_NAME;
			goto bypass_bdf;
		}

		if (plat_priv->eeprom_caldata_read_timeout &&
		    (plat_priv->device_id == QCN9000_DEVICE_ID ||
		     plat_priv->device_id == QCN9224_DEVICE_ID)) {
			fw_bdf_type = BDF_TYPE_EEPROM;
			temp = filename;
			remaining = MAX_BDF_FILE_NAME;
			goto bypass_bdf;
		}
		break;
	case CNSS_BDF_REGDB:
		fw_bdf_type = BDF_TYPE_REGDB;
		snprintf(filename, sizeof(filename),
			 "%s" REGDB_FILE_NAME, folder);
		if (plat_priv->bus_type == CNSS_BUS_AHB) {
			temp = filename;
			remaining = MAX_BDF_FILE_NAME;
			goto bypass_bdf;
		}
		break;
	case CNSS_BDF_HDS:
		fw_bdf_type = BDF_TYPE_HDS;
		snprintf(filename, sizeof(filename),
			 "%s" HDS_FILE_NAME, folder);
		if (plat_priv->bus_type == CNSS_BUS_AHB) {
			temp = filename;
			remaining = MAX_BDF_FILE_NAME;
			goto bypass_bdf;
		}
		break;
	default:
		cnss_pr_err("Invalid BDF type: %d\n",
			    plat_priv->ctrl_params.bdf_type);
		ret = -EINVAL;
		goto out;
	}

	ret = request_firmware(&fw_entry, filename, &plat_priv->plat_dev->dev);
	if (ret) {
		if (bdf_type == CNSS_CALDATA_WIN) {
			cnss_pr_warn("WARNING: Caldata not present. Skipping caldata download: %s\n",
				     filename);
			ret = 0;
			resp_error_msg = -ENOENT;
			goto err_req_fw;
		} else if (bdf_type == CNSS_BDF_HDS) {
			/* HDS bin download is not mandatory */
			ret = 0;
			goto out;
		} else if (bdf_type == CNSS_BDF_REGDB) {
			if (plat_priv->device_id == QCN9224_DEVICE_ID) {
				/* Reg DB bin download is mandatory for QCN9224,
				 * hence if the file is not found, we assert.
				 */
				cnss_pr_info("Failed to load RegDB %s\n",
					     filename);
				goto out;
			} else {
				ret = 0;
				goto out;
			}
		} else {
			/* BDF download is mandatory for all targets */
			cnss_pr_err("Failed to load BDF: %s\n", filename);
			goto out;
		}
	}

	temp = fw_entry->data;
	remaining = fw_entry->size;

	cnss_pr_info("Downloading BDF: %s, size: %u\n", filename, remaining);
bypass_bdf:

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_BDF_DOWNLOAD_REQ_V01, ret, resp_error_msg);
	while (remaining) {
		req->valid = 1;
		req->file_id_valid = 1;
		req->file_id = plat_priv->board_info.board_id;
		req->total_size_valid = 1;
		req->total_size = remaining;
		req->seg_id_valid = 1;
		req->data_valid = 1;
		req->end_valid = 1;
		req->bdf_type_valid = 1;
		req->bdf_type = fw_bdf_type;

		if (remaining > QMI_WLFW_MAX_DATA_SIZE_V01) {
			req->data_len = QMI_WLFW_MAX_DATA_SIZE_V01;
		} else {
			req->data_len = remaining;
			req->end = 1;
		}
		if (plat_priv->device_id == QCA8074_DEVICE_ID ||
		    plat_priv->device_id == QCA8074V2_DEVICE_ID ||
		    plat_priv->device_id == QCA5018_DEVICE_ID ||
		    plat_priv->device_id == QCN6122_DEVICE_ID ||
		    plat_priv->device_id == QCA6018_DEVICE_ID ||
		    plat_priv->device_id == QCA9574_DEVICE_ID) {
			ret = cnss_wlfw_load_bdf(req, plat_priv,
						 MAX_BDF_FILE_NAME,
						 fw_bdf_type);
			if (ret) {
				if (bdf_type == CNSS_CALDATA_WIN) {
					cnss_pr_warn("WARNING: Caldata not present. Skipping caldata download: %s\n",
						     filename);
					ret = 0;
					resp_error_msg = -ENOENT;
					goto err_req_fw;
				} else if (bdf_type == CNSS_BDF_HDS ||
					   bdf_type == CNSS_BDF_REGDB) {
					/* HDS is not mandatory and REGDB is
					 * is mandatory only for QCN9224
					 */
					ret = 0;
					goto err_req_fw;
				} else {
					/* BDF download is mandatory for
					 * all targets.
					 */
					cnss_pr_err("Failed to load BDF: %s\n",
						    filename);
					goto err_req_fw;
				}
			}
		}

		memcpy(req->data, temp, req->data_len);

		ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
				   wlfw_bdf_download_resp_msg_v01_ei, resp);
		if (ret < 0) {
			cnss_pr_err("Failed to initialize txn for BDF download request, err: %d\n",
				    ret);
			goto err_send;
		}

		ret = qmi_send_request
			(&plat_priv->qmi_wlfw, NULL, &txn,
			 QMI_WLFW_BDF_DOWNLOAD_REQ_V01,
			 WLFW_BDF_DOWNLOAD_REQ_MSG_V01_MAX_MSG_LEN,
			 wlfw_bdf_download_req_msg_v01_ei, req);
		if (ret < 0) {
			qmi_txn_cancel(&txn);
			cnss_pr_err("Failed to send respond BDF download request, err: %d\n",
				    ret);
			goto err_send;
		}

		if (fw_bdf_type == BDF_TYPE_EEPROM) {
			cnss_pr_info("EEPROM READ WAIT STARTED: %d seconds",
				     plat_priv->eeprom_caldata_read_timeout);
			ret = qmi_txn_wait(&txn,
					   msecs_to_jiffies(
					   plat_priv->
					   eeprom_caldata_read_timeout * 1000));
		} else {
			ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
		}

		if (ret < 0) {
			cnss_pr_err("Failed to wait for response of BDF download request, err: %d\n",
				    ret);
			goto err_send;
		}

		if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
			cnss_pr_err("BDF download request failed, result: %d, err: %d\n",
				    resp->resp.result, resp->resp.error);
			ret = -resp->resp.result;
			resp_error_msg = resp->resp.error;
			goto err_send;
		}
		remaining -= req->data_len;
		temp += req->data_len;
		req->seg_id++;
	}

	if (fw_entry)
		release_firmware(fw_entry);

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_BDF_DOWNLOAD_REQ_V01, ret, resp_error_msg);
	kfree(req);
	kfree(resp);
	return 0;

err_send:
	if (plat_priv->ctrl_params.bdf_type != CNSS_BDF_DUMMY)
		release_firmware(fw_entry);
err_req_fw:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_BDF_DOWNLOAD_REQ_V01, ret, resp_error_msg);
out:
	kfree(req);
	kfree(resp);

	if (ret)
		CNSS_ASSERT(0);
	else
		ret = 0;
	return ret;
}

int cnss_wlfw_m3_dnld_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_m3_info_req_msg_v01 *req;
	struct wlfw_m3_info_resp_msg_v01 *resp;
	struct qmi_txn txn;
	struct cnss_fw_mem *m3_mem = &plat_priv->m3_mem;
	int ret = 0;
	int resp_error_msg = 0;

	cnss_pr_dbg("Sending M3 information message, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}
	if ((plat_priv->device_id == QCN9000_DEVICE_ID ||
	     plat_priv->device_id == QCN9224_DEVICE_ID) &&
	    (!m3_mem->pa || !m3_mem->size)) {
		cnss_pr_err("Memory for M3 is not available\n");
		ret = -ENOMEM;
		goto out;
	}

	cnss_pr_dbg("M3 memory, va: 0x%pK, pa: %pa, size: 0x%zx\n",
		    m3_mem->va, &m3_mem->pa, m3_mem->size);

	req->addr = plat_priv->m3_mem.pa;
	req->size = plat_priv->m3_mem.size;

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_M3_INFO_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_m3_info_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for M3 information request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_M3_INFO_REQ_V01,
			       WLFW_M3_INFO_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_m3_info_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send M3 information request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of M3 information request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("M3 information request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_M3_INFO_REQ_V01, ret, resp_error_msg);
	kfree(req);
	kfree(resp);
	return 0;

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_M3_INFO_REQ_V01, ret, resp_error_msg);
	CNSS_ASSERT(0);
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_wlan_mode_send_sync(struct cnss_plat_data *plat_priv,
				  enum cnss_driver_mode mode)
{
	struct wlfw_wlan_mode_req_msg_v01 *req;
	struct wlfw_wlan_mode_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;
	int resp_error_msg = 0;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_info("Sending mode message, mode: %s(%d), state: 0x%lx\n",
		     cnss_qmi_mode_to_str(mode), mode, plat_priv->driver_state);

	if (mode == CNSS_OFF &&
	    test_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state)) {
		cnss_pr_dbg("Recovery is in progress, ignore mode off request\n");
		return 0;
	}

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->mode = (enum wlfw_driver_mode_enum_v01)mode;
	req->hw_debug_valid = 1;
	req->hw_debug = 0;
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_WLAN_MODE_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_wlan_mode_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for mode request, mode: %s(%d), err: %d\n",
			    cnss_qmi_mode_to_str(mode), mode, ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_WLAN_MODE_REQ_V01,
			       WLFW_WLAN_MODE_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_wlan_mode_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send mode request, mode: %s(%d), err: %d\n",
			    cnss_qmi_mode_to_str(mode), mode, ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of mode request, mode: %s(%d), err: %d\n",
			    cnss_qmi_mode_to_str(mode), mode, ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Mode request failed, mode: %s(%d), result: %d, err: %d\n",
			    cnss_qmi_mode_to_str(mode), mode, resp->resp.result,
			    resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_WLAN_MODE_REQ_V01, ret, resp_error_msg);

	kfree(req);
	kfree(resp);
	return 0;

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_WLAN_MODE_REQ_V01, ret, resp_error_msg);
	if (mode == CNSS_OFF) {
		cnss_pr_dbg("WLFW service is disconnected while sending mode off request\n");
		ret = 0;
	} else {
		CNSS_ASSERT(0);
	}
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_wlan_cfg_send_sync(struct cnss_plat_data *plat_priv,
				 struct cnss_wlan_enable_cfg *config,
				 const char *host_version)
{
	struct wlfw_wlan_cfg_req_msg_v01 *req;
	struct wlfw_wlan_cfg_resp_msg_v01 *resp;
	struct qmi_txn txn;
	u32 i;
	int ret = 0;
	int resp_error_msg = 0;

	cnss_pr_dbg("Sending WLAN config message, state: 0x%lx\n",
		    plat_priv->driver_state);

	if (!plat_priv)
		return -ENODEV;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->host_version_valid = 1;
	strlcpy(req->host_version, host_version,
		QMI_WLFW_MAX_STR_LEN_V01 + 1);

	req->tgt_cfg_valid = 1;
	if (config->num_ce_tgt_cfg > QMI_WLFW_MAX_NUM_CE_V01)
		req->tgt_cfg_len = QMI_WLFW_MAX_NUM_CE_V01;
	else
		req->tgt_cfg_len = config->num_ce_tgt_cfg;
	for (i = 0; i < req->tgt_cfg_len; i++) {
		req->tgt_cfg[i].pipe_num = config->ce_tgt_cfg[i].pipe_num;
		req->tgt_cfg[i].pipe_dir = config->ce_tgt_cfg[i].pipe_dir;
		req->tgt_cfg[i].nentries = config->ce_tgt_cfg[i].nentries;
		req->tgt_cfg[i].nbytes_max = config->ce_tgt_cfg[i].nbytes_max;
		req->tgt_cfg[i].flags = config->ce_tgt_cfg[i].flags;
	}

	req->svc_cfg_valid = 1;
	if (config->num_ce_svc_pipe_cfg > QMI_WLFW_MAX_NUM_SVC_V01)
		req->svc_cfg_len = QMI_WLFW_MAX_NUM_SVC_V01;
	else
		req->svc_cfg_len = config->num_ce_svc_pipe_cfg;
	for (i = 0; i < req->svc_cfg_len; i++) {
		req->svc_cfg[i].service_id = config->ce_svc_cfg[i].service_id;
		req->svc_cfg[i].pipe_dir = config->ce_svc_cfg[i].pipe_dir;
		req->svc_cfg[i].pipe_num = config->ce_svc_cfg[i].pipe_num;
	}

	req->shadow_reg_v2_valid = 1;
	if (config->num_shadow_reg_v2_cfg >
	    QMI_WLFW_MAX_NUM_SHADOW_REG_V2_V01)
		req->shadow_reg_v2_len = QMI_WLFW_MAX_NUM_SHADOW_REG_V2_V01;
	else
		req->shadow_reg_v2_len = config->num_shadow_reg_v2_cfg;

	memcpy(req->shadow_reg_v2, config->shadow_reg_v2_cfg,
	       sizeof(struct wlfw_shadow_reg_v2_cfg_s_v01)
	       * req->shadow_reg_v2_len);

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_WLAN_CFG_REQ_V01, ret, resp_error_msg);

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_wlan_cfg_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for WLAN config request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_WLAN_CFG_REQ_V01,
			       WLFW_WLAN_CFG_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_wlan_cfg_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send WLAN config request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of WLAN config request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("WLAN config request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_WLAN_CFG_REQ_V01, ret, resp_error_msg);

	kfree(req);
	kfree(resp);
	return 0;

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_WLAN_CFG_REQ_V01, ret, resp_error_msg);
	CNSS_ASSERT(0);
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_athdiag_read_send_sync(struct cnss_plat_data *plat_priv,
				     u32 offset, u32 mem_type,
				     u32 data_len, u8 *data)
{
	struct wlfw_athdiag_read_req_msg_v01 *req;
	struct wlfw_athdiag_read_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	if (!data || data_len == 0 || data_len > QMI_WLFW_MAX_DATA_SIZE_V01) {
		cnss_pr_err("Invalid parameters for athdiag read: data %p, data_len %u\n",
			    data, data_len);
		return -EINVAL;
	}

	cnss_pr_dbg("athdiag read: state 0x%lx, offset %x, mem_type %x, data_len %u\n",
		    plat_priv->driver_state, offset, mem_type, data_len);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->offset = offset;
	req->mem_type = mem_type;
	req->data_len = data_len;

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_athdiag_read_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for athdiag read request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_ATHDIAG_READ_REQ_V01,
			       WLFW_ATHDIAG_READ_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_athdiag_read_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send athdiag read request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of athdiag read request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Athdiag read request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		goto out;
	}

	if (!resp->data_valid || resp->data_len != data_len) {
		cnss_pr_err("athdiag read data is invalid, data_valid = %u, data_len = %u\n",
			    resp->data_valid, resp->data_len);
		ret = -EINVAL;
		goto out;
	}

	memcpy(data, resp->data, resp->data_len);

	kfree(req);
	kfree(resp);
	return 0;

out:
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_athdiag_write_send_sync(struct cnss_plat_data *plat_priv,
				      u32 offset, u32 mem_type,
				      u32 data_len, u8 *data)
{
	struct wlfw_athdiag_write_req_msg_v01 *req;
	struct wlfw_athdiag_write_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	if (!data || data_len == 0 || data_len > QMI_WLFW_MAX_DATA_SIZE_V01) {
		cnss_pr_err("Invalid parameters for athdiag write: data %p, data_len %u\n",
			    data, data_len);
		return -EINVAL;
	}

	cnss_pr_dbg("athdiag write: state 0x%lx, offset %x, mem_type %x, data_len %u, data %p\n",
		    plat_priv->driver_state, offset, mem_type, data_len, data);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->offset = offset;
	req->mem_type = mem_type;
	req->data_len = data_len;
	memcpy(req->data, data, data_len);

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_athdiag_write_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for athdiag write request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_ATHDIAG_WRITE_REQ_V01,
			       WLFW_ATHDIAG_WRITE_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_athdiag_write_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send athdiag write request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of athdiag write request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Athdiag write request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		goto out;
	}

	kfree(req);
	kfree(resp);
	return 0;

out:
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_ini_send_sync(struct cnss_plat_data *plat_priv,
			    u8 fw_log_mode)
{
	struct wlfw_ini_req_msg_v01 *req;
	struct wlfw_ini_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;
	int resp_error_msg = 0;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_dbg("Sending ini sync request, state: 0x%lx, fw_log_mode: %d\n",
		    plat_priv->driver_state, fw_log_mode);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->enablefwlog_valid = 1;
	req->enablefwlog = fw_log_mode;

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_INI_REQ_V01, ret, resp_error_msg);

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_ini_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for ini request, fw_log_mode: %d, err: %d\n",
			    fw_log_mode, ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_INI_REQ_V01,
			       WLFW_INI_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_ini_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send ini request, fw_log_mode: %d, err: %d\n",
			    fw_log_mode, ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of ini request, fw_log_mode: %d, err: %d\n",
			    fw_log_mode, ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Ini request failed, fw_log_mode: %d, result: %d, err: %d\n",
			    fw_log_mode, resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_INI_REQ_V01, ret, resp_error_msg);

	kfree(req);
	kfree(resp);
	return 0;

out:
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_antenna_switch_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_antenna_switch_req_msg_v01 *req;
	struct wlfw_antenna_switch_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;
	int resp_error_msg = 0;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_dbg("Sending antenna switch sync request, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_ANTENNA_SWITCH_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_antenna_switch_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for antenna switch request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_ANTENNA_SWITCH_REQ_V01,
			       WLFW_ANTENNA_SWITCH_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_antenna_switch_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send antenna switch request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of antenna switch request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Antenna switch request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

	if (resp->antenna_valid)
		plat_priv->antenna = resp->antenna;

	cnss_pr_dbg("Antenna valid: %u, antenna 0x%llx\n",
		    resp->antenna_valid, resp->antenna);

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_ANTENNA_SWITCH_REQ_V01, ret, resp_error_msg);
	kfree(req);
	kfree(resp);
	return 0;

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_ANTENNA_SWITCH_REQ_V01, ret, resp_error_msg);
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_antenna_grant_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_antenna_grant_req_msg_v01 *req;
	struct wlfw_antenna_grant_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_dbg("Sending antenna grant sync request, state: 0x%lx, grant 0x%llx\n",
		    plat_priv->driver_state, plat_priv->grant);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->grant_valid = 1;
	req->grant = plat_priv->grant;

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_antenna_grant_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize txn for antenna grant request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_ANTENNA_GRANT_REQ_V01,
			       WLFW_ANTENNA_GRANT_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_antenna_grant_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Failed to send antenna grant request, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for response of antenna grant request, err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Antenna grant request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		goto out;
	}

	kfree(req);
	kfree(resp);
	return 0;

out:
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_qdss_trace_mem_info_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_qdss_trace_mem_info_req_msg_v01 *req;
	struct wlfw_qdss_trace_mem_info_resp_msg_v01 *resp;
	struct qmi_txn txn;
	struct cnss_fw_mem *qdss_mem = plat_priv->qdss_mem;
	int ret = 0;
	int i;
	int resp_error_msg = 0;

	cnss_pr_dbg("Sending QDSS trace mem info, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->mem_seg_len = plat_priv->qdss_mem_seg_len;
	for (i = 0; i < req->mem_seg_len; i++) {
		cnss_pr_dbg("Memory for FW, pa: 0x%x, size: 0x%x, type: %u\n",
			    (unsigned int)qdss_mem[i].pa,
			    (unsigned int)qdss_mem[i].size,
			    qdss_mem[i].type);
		req->mem_seg[i].addr = qdss_mem[i].pa;
		req->mem_seg[i].size = qdss_mem[i].size;
		req->mem_seg[i].type = qdss_mem[i].type;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_QDSS_TRACE_MEM_INFO_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_qdss_trace_mem_info_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Fail to initialize txn for QDSS trace mem request: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_QDSS_TRACE_MEM_INFO_REQ_V01,
			       WLFW_QDSS_TRACE_MEM_INFO_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_qdss_trace_mem_info_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Fail to send QDSS trace mem info request: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Fail to wait for response of QDSS trace mem info request, err %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("QDSS trace mem info request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_QDSS_TRACE_MEM_INFO_REQ_V01, ret, resp_error_msg);
	kfree(req);
	kfree(resp);
	return 0;

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_QDSS_TRACE_MEM_INFO_REQ_V01, ret, resp_error_msg);
	kfree(req);
	kfree(resp);
	return ret;
}

#ifdef CNSS2_IMS
static int cnss_wlfw_wfc_call_status_send_sync(struct cnss_plat_data *plat_priv,
					       u32 data_len, const void *data)
{
	struct wlfw_wfc_call_status_req_msg_v01 *req;
	struct wlfw_wfc_call_status_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;

	cnss_pr_dbg("Sending WFC call status: state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->wfc_call_status_len = data_len;
	memcpy(req->wfc_call_status, data, req->wfc_call_status_len);

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_wfc_call_status_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Fail to initialize txn for WFC call status request: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_WFC_CALL_STATUS_REQ_V01,
			       WLFW_WFC_CALL_STATUS_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_wfc_call_status_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Fail to send WFC call status request: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Fail to wait for response of WFC call status request, err %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("WFC call status request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		goto out;
	}

	kfree(req);
	kfree(resp);
	return 0;

out:
	kfree(req);
	kfree(resp);
	return ret;
}
#endif

int cnss_wlfw_dynamic_feature_mask_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_dynamic_feature_mask_req_msg_v01 *req;
	struct wlfw_dynamic_feature_mask_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;

	cnss_pr_dbg("Sending dynamic feature mask 0x%llx, state: 0x%lx\n",
		    plat_priv->dynamic_feature,
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->mask_valid = 1;
	req->mask = plat_priv->dynamic_feature;

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_dynamic_feature_mask_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Fail to initialize txn for dynamic feature mask request: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request
		(&plat_priv->qmi_wlfw, NULL, &txn,
		 QMI_WLFW_DYNAMIC_FEATURE_MASK_REQ_V01,
		 WLFW_DYNAMIC_FEATURE_MASK_REQ_MSG_V01_MAX_MSG_LEN,
		 wlfw_dynamic_feature_mask_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Fail to send dynamic feature mask request: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Fail to wait for response of dynamic feature mask request, err %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Dynamic feature mask request failed, result: %d, err: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		goto out;
	}

out:
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_m3_dump_upload_done_send_sync(struct cnss_plat_data *plat_priv,
					    u32 pdev_id, int status)
{
	struct wlfw_m3_dump_upload_done_req_msg_v01 *req;
	struct wlfw_m3_dump_upload_done_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;
	int resp_error_msg = 0;

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	cnss_pr_dbg("Sending M3 Upload done req, pdev %d, status %d\n",
		    pdev_id, status);

	req->pdev_id = pdev_id;
	req->status = status;

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_M3_DUMP_UPLOAD_DONE_REQ_V01, ret, resp_error_msg);
	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_m3_dump_upload_done_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Fail to initialize txn for M3 dump upload done req: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_M3_DUMP_UPLOAD_DONE_REQ_V01,
			       WLFW_M3_DUMP_UPLOAD_DONE_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_m3_dump_upload_done_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Fail to send M3 dump upload done request: err %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Fail to wait for response of M3 dump upload done request, err %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("M3 Dump Upload Done Req failed, result: %d, err: 0x%X\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_M3_DUMP_UPLOAD_DONE_REQ_V01, ret, resp_error_msg);
	kfree(req);
	kfree(resp);
	return ret;
}

int cnss_wlfw_device_info_send_sync(struct cnss_plat_data *plat_priv)
{
	struct wlfw_device_info_req_msg_v01 *req;
	struct wlfw_device_info_resp_msg_v01 *resp;
	struct qmi_txn txn;
	int ret = 0;
	int resp_error_msg = 0;

	cnss_pr_dbg("Sending Device Info request message, state: 0x%lx\n",
		    plat_priv->driver_state);

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_DEVICE_INFO_REQ_V01, ret, resp_error_msg);

	ret = qmi_txn_init(&plat_priv->qmi_wlfw, &txn,
			   wlfw_device_info_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Fail to init txn for Device Info req, err: %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request(&plat_priv->qmi_wlfw, NULL, &txn,
			       QMI_WLFW_DEVICE_INFO_REQ_V01,
			       WLFW_DEVICE_INFO_REQ_MSG_V01_MAX_MSG_LEN,
			       wlfw_device_info_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Fail to send device info req %d\n", ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, QMI_WLFW_TIMEOUT_JF);
	if (ret < 0) {
		cnss_pr_err("Failed to wait for device info response err: %d\n",
			    ret);
		goto out;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Device info request failed, result: %d error: %d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		resp_error_msg = resp->resp.error;
		goto out;
	}

	if (!resp->bar_addr_valid || !resp->bar_size_valid) {
		cnss_pr_err("bar addr(%d) or bar size(%d) not received\n",
			    resp->bar_addr_valid, resp->bar_size_valid);
		ret = -EINVAL;
		goto out;
	}

	if (!resp->bar_addr ||
	    (resp->bar_size != QCN6122_DEVICE_BAR_SIZE)) {
		cnss_pr_err("Invalid bar addr(0x%llx) or bar size (0x%x)\n",
			    resp->bar_addr, resp->bar_size);
		ret = -EINVAL;
		goto out;
	}

	plat_priv->qcn6122.bar_addr_pa = resp->bar_addr;
	plat_priv->qcn6122.bar_size = resp->bar_size;

	plat_priv->qcn6122.bar_addr_va =
				ioremap_nocache(plat_priv->qcn6122.bar_addr_pa,
						plat_priv->qcn6122.bar_size);

	if (!plat_priv->qcn6122.bar_addr_va) {
		cnss_pr_err("Ioremap failed for bar address\n");
		plat_priv->qcn6122.bar_addr_pa = 0;
		plat_priv->qcn6122.bar_size = 0;
		ret = -EIO;
		goto out;
	}

	cnss_pr_info("Device BAR Info pa: 0x%llx, va: 0x%p, size: 0x%x\n",
		     plat_priv->qcn6122.bar_addr_pa,
		     plat_priv->qcn6122.bar_addr_va,
		     plat_priv->qcn6122.bar_size);

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_DEVICE_INFO_RESP_V01, ret, resp_error_msg);

	kfree(resp);
	kfree(req);
	return 0;

out:
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_DEVICE_INFO_RESP_V01, ret, resp_error_msg);

	kfree(resp);
	kfree(req);
	if (ret < 0)
		CNSS_ASSERT(0);

	return ret;
}

unsigned int cnss_get_qmi_timeout(struct cnss_plat_data *plat_priv)
{
	cnss_pr_dbg("QMI timeout is %u ms\n", QMI_WLFW_TIMEOUT_MS);

	return QMI_WLFW_TIMEOUT_MS;
}
EXPORT_SYMBOL(cnss_get_qmi_timeout);

static void cnss_wlfw_request_mem_ind_cb(struct qmi_handle *qmi_wlfw,
					 struct sockaddr_qrtr *sq,
					 struct qmi_txn *txn, const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	const struct wlfw_request_mem_ind_msg_v01 *ind_msg = data;
	int i;

	cnss_pr_dbg("Received QMI WLFW request memory indication\n");

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_REQUEST_MEM_IND_V01, 0, 0);
	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}
	if (ind_msg->mem_seg_len == 0 ||
		ind_msg->mem_seg_len > QMI_WLFW_MAX_NUM_MEM_SEG_V01)
		cnss_pr_err("Invalid memory segment length: %u\n",
			    ind_msg->mem_seg_len);

	cnss_pr_dbg("FW memory segment count is %u\n", ind_msg->mem_seg_len);
	plat_priv->fw_mem_seg_len = ind_msg->mem_seg_len;

	for (i = 0; i < plat_priv->fw_mem_seg_len; i++) {
		cnss_pr_dbg("FW requests for memory, size: 0x%x, type: %u\n",
			    ind_msg->mem_seg[i].size, ind_msg->mem_seg[i].type);
		plat_priv->fw_mem[i].type = ind_msg->mem_seg[i].type;
		plat_priv->fw_mem[i].size = ind_msg->mem_seg[i].size;
	}

	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_REQUEST_MEM,
			       0, NULL);
}

static void cnss_wlfw_fw_mem_ready_ind_cb(struct qmi_handle *qmi_wlfw,
					  struct sockaddr_qrtr *sq,
					  struct qmi_txn *txn, const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);

	cnss_pr_dbg("Received QMI WLFW FW memory ready indication\n");

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_FW_MEM_READY_IND_V01, 0, 0);
	/* WAR Conditional check of driver state to hinder processing */
	/* FW_mem_ready msg i.e. received twice from QMI stack occasionally */
	if (!test_bit(CNSS_FW_MEM_READY, &plat_priv->driver_state))
		cnss_driver_event_post(plat_priv,
			CNSS_DRIVER_EVENT_FW_MEM_READY, 0, NULL);
	else
		cnss_pr_err("FW_Mem_Ready_Ind received twice\n");

}

static void cnss_wlfw_fw_ready_ind_cb(struct qmi_handle *qmi_wlfw,
				      struct sockaddr_qrtr *sq,
				      struct qmi_txn *txn, const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	struct cnss_cal_info *cal_info;

	cnss_pr_dbg("Received QMI WLFW FW ready indication\n");

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}

	cal_info = kzalloc(sizeof(*cal_info), GFP_KERNEL);
	if (!cal_info)
		return;

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_FW_READY_IND_V01, 0, 0);
	cal_info->cal_status = CNSS_CAL_DONE;
	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_COLD_BOOT_CAL_DONE,
			       0, cal_info);
}

static void cnss_wlfw_fw_init_done_ind_cb(struct qmi_handle *qmi_wlfw,
					  struct sockaddr_qrtr *sq,
					  struct qmi_txn *txn, const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);

	cnss_pr_dbg("Received QMI WLFW FW initialization done indication\n");

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_FW_INIT_DONE_IND_V01, 0, 0);
	cnss_driver_event_post(plat_priv,
				CNSS_DRIVER_EVENT_FW_READY,
				0, NULL);
}

static void cnss_wlfw_pin_result_ind_cb(struct qmi_handle *qmi_wlfw,
					struct sockaddr_qrtr *sq,
					struct qmi_txn *txn, const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	const struct wlfw_pin_connect_result_ind_msg_v01 *ind_msg = data;

	cnss_pr_dbg("Received QMI WLFW pin connect result indication\n");

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_PIN_CONNECT_RESULT_IND_V01, 0, 0);
	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}

	if (ind_msg->pwr_pin_result_valid)
		plat_priv->pin_result.fw_pwr_pin_result =
		    ind_msg->pwr_pin_result;
	if (ind_msg->phy_io_pin_result_valid)
		plat_priv->pin_result.fw_phy_io_pin_result =
		    ind_msg->phy_io_pin_result;
	if (ind_msg->rf_pin_result_valid)
		plat_priv->pin_result.fw_rf_pin_result = ind_msg->rf_pin_result;

	cnss_pr_dbg("Pin connect Result: pwr_pin: 0x%x phy_io_pin: 0x%x rf_io_pin: 0x%x\n",
		    ind_msg->pwr_pin_result, ind_msg->phy_io_pin_result,
		    ind_msg->rf_pin_result);
}

static void cnss_wlfw_cal_done_ind_cb(struct qmi_handle *qmi_wlfw,
				      struct sockaddr_qrtr *sq,
				      struct qmi_txn *txn, const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	struct cnss_cal_info *cal_info;

	cnss_pr_dbg("Received QMI WLFW calibration done indication\n");

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}

	cal_info = kzalloc(sizeof(*cal_info), GFP_KERNEL);
	if (!cal_info)
		return;

	cal_info->cal_status = CNSS_CAL_DONE;
	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_COLD_BOOT_CAL_DONE,
			       0, cal_info);
}

static void cnss_wlfw_qdss_trace_req_mem_ind_cb(struct qmi_handle *qmi_wlfw,
						struct sockaddr_qrtr *sq,
						struct qmi_txn *txn,
						const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	const struct wlfw_qdss_trace_req_mem_ind_msg_v01 *ind_msg = data;
	int i;

	cnss_pr_dbg("Received QMI WLFW QDSS trace request mem indication\n");
	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_QDSS_TRACE_REQ_MEM_IND_V01, 0, 0);

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}

	if (plat_priv->qdss_mem_seg_len) {
		cnss_pr_err("Ignore double allocation for QDSS trace, current len %u\n",
			    plat_priv->qdss_mem_seg_len);
		return;
	}
	plat_priv->qdss_mem_seg_len = ind_msg->mem_seg_len;
	if (ind_msg->mem_seg_len > 1) {
		cnss_pr_dbg("%s: FW requests %d segments, overwriting it with 1",
			    __func__, ind_msg->mem_seg_len);
		plat_priv->qdss_mem_seg_len = 1;
	}

	for (i = 0; i < plat_priv->qdss_mem_seg_len; i++) {
		cnss_pr_dbg("QDSS requests for memory, size: 0x%x, type: %u\n",
			    ind_msg->mem_seg[i].size, ind_msg->mem_seg[i].type);
		plat_priv->qdss_mem[i].type = ind_msg->mem_seg[i].type;
		plat_priv->qdss_mem[i].size = ind_msg->mem_seg[i].size;
	}

	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_QDSS_TRACE_REQ_MEM,
			       0, NULL);
}

static void cnss_wlfw_qdss_trace_save_ind_cb(struct qmi_handle *qmi_wlfw,
					     struct sockaddr_qrtr *sq,
					     struct qmi_txn *txn,
					     const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	const struct wlfw_qdss_trace_save_ind_msg_v01 *ind_msg = data;
	struct cnss_qmi_event_qdss_trace_save_data *event_data;
	int i = 0;

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_QDSS_TRACE_SAVE_IND_V01, 0, 0);
	cnss_pr_dbg("Received QMI WLFW QDSS trace save indication\n");

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}

	cnss_pr_dbg("QDSS_trace_save info: source %u, total_size %u, file_name_valid %u, file_name %s\n",
		    ind_msg->source, ind_msg->total_size,
		    ind_msg->file_name_valid, ind_msg->file_name);

	if (ind_msg->source == 1)
		return;

	event_data = kzalloc(sizeof(*event_data), GFP_KERNEL);
	if (!event_data)
		return;

	if (ind_msg->mem_seg_valid) {
		if (ind_msg->mem_seg_len > QDSS_TRACE_SEG_LEN_MAX) {
			cnss_pr_err("Invalid seg len %u\n",
				    ind_msg->mem_seg_len);
			goto free_event_data;
		}
		cnss_pr_dbg("QDSS_trace_save seg len %u\n",
			    ind_msg->mem_seg_len);
		event_data->mem_seg_len = ind_msg->mem_seg_len;
		for (i = 0; i < ind_msg->mem_seg_len; i++) {
			event_data->mem_seg[i].addr = ind_msg->mem_seg[i].addr;
			event_data->mem_seg[i].size = ind_msg->mem_seg[i].size;
			cnss_pr_dbg("seg-%d: addr 0x%llx size 0x%x\n",
				    i, ind_msg->mem_seg[i].addr,
				    ind_msg->mem_seg[i].size);
		}
	}

	event_data->total_size = ind_msg->total_size;

	if (ind_msg->file_name_valid)
		strlcpy(event_data->file_name, ind_msg->file_name,
			QDSS_TRACE_FILE_NAME_MAX + 1);
	else
		strlcpy(event_data->file_name, "qdss_trace",
			QDSS_TRACE_FILE_NAME_MAX + 1);

	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_QDSS_TRACE_SAVE,
			       0, event_data);

	return;

free_event_data:
	kfree(event_data);
}

static void cnss_wlfw_qdss_trace_free_ind_cb(struct qmi_handle *qmi_wlfw,
					     struct sockaddr_qrtr *sq,
					     struct qmi_txn *txn,
					     const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_QDSS_TRACE_FREE_IND_V01, 0, 0);
	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_QDSS_TRACE_FREE,
			       0, NULL);
}

static void cnss_wlfw_m3_dump_upload_req_ind_cb(struct qmi_handle *qmi_wlfw,
						struct sockaddr_qrtr *sq,
						struct qmi_txn *txn,
						const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	const struct wlfw_m3_dump_upload_req_ind_msg_v01 *ind_msg = data;
	struct cnss_qmi_event_m3_dump_upload_req_data *event_data;

	qmi_record(plat_priv->wlfw_service_instance_id,
		   QMI_WLFW_M3_DUMP_UPLOAD_REQ_IND_V01, 0, 0);
	cnss_pr_dbg("Received QMI WLFW M3 Dump Upload indication\n");

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}

	cnss_pr_dbg("M3 Dump upload info: pdev_id %d addr: 0x%llx size 0x%llx\n",
		    ind_msg->pdev_id, ind_msg->addr, ind_msg->size);

	event_data = kzalloc(sizeof(*event_data), GFP_KERNEL);
	if (!event_data)
		return;

	event_data->pdev_id = ind_msg->pdev_id;
	event_data->addr = ind_msg->addr;
	event_data->size = ind_msg->size;

	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_M3_DUMP_UPLOAD_REQ,
			       0, event_data);
}

static struct qmi_msg_handler qmi_wlfw_msg_handlers[] = {
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_REQUEST_MEM_IND_V01,
		.ei = wlfw_request_mem_ind_msg_v01_ei,
		.decoded_size = sizeof(struct wlfw_request_mem_ind_msg_v01),
		.fn = cnss_wlfw_request_mem_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_FW_MEM_READY_IND_V01,
		.ei = wlfw_fw_mem_ready_ind_msg_v01_ei,
		.decoded_size = sizeof(struct wlfw_fw_mem_ready_ind_msg_v01),
		.fn = cnss_wlfw_fw_mem_ready_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_FW_READY_IND_V01,
		.ei = wlfw_fw_ready_ind_msg_v01_ei,
		.decoded_size = sizeof(struct wlfw_fw_ready_ind_msg_v01),
		.fn = cnss_wlfw_fw_ready_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_FW_INIT_DONE_IND_V01,
		.ei = wlfw_fw_init_done_ind_msg_v01_ei,
		.decoded_size = sizeof(struct wlfw_fw_init_done_ind_msg_v01),
		.fn = cnss_wlfw_fw_init_done_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_PIN_CONNECT_RESULT_IND_V01,
		.ei = wlfw_pin_connect_result_ind_msg_v01_ei,
		.decoded_size =
			sizeof(struct wlfw_pin_connect_result_ind_msg_v01),
		.fn = cnss_wlfw_pin_result_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_CAL_DONE_IND_V01,
		.ei = wlfw_cal_done_ind_msg_v01_ei,
		.decoded_size = sizeof(struct wlfw_cal_done_ind_msg_v01),
		.fn = cnss_wlfw_cal_done_ind_cb
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_QDSS_TRACE_REQ_MEM_IND_V01,
		.ei = wlfw_qdss_trace_req_mem_ind_msg_v01_ei,
		.decoded_size =
			sizeof(struct wlfw_qdss_trace_req_mem_ind_msg_v01),
		.fn = cnss_wlfw_qdss_trace_req_mem_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_QDSS_TRACE_SAVE_IND_V01,
		.ei = wlfw_qdss_trace_save_ind_msg_v01_ei,
		.decoded_size =
			sizeof(struct wlfw_qdss_trace_save_ind_msg_v01),
		.fn = cnss_wlfw_qdss_trace_save_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_QDSS_TRACE_FREE_IND_V01,
		.ei = wlfw_qdss_trace_free_ind_msg_v01_ei,
		.decoded_size =
			sizeof(struct wlfw_qdss_trace_free_ind_msg_v01),
		.fn = cnss_wlfw_qdss_trace_free_ind_cb,
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_WLFW_M3_DUMP_UPLOAD_REQ_IND_V01,
		.ei = wlfw_m3_dump_upload_req_ind_msg_v01_ei,
		.decoded_size =
			sizeof(struct wlfw_m3_dump_upload_req_ind_msg_v01),
		.fn = cnss_wlfw_m3_dump_upload_req_ind_cb,
	},
	{}
};

static int cnss_wlfw_connect_to_server(struct cnss_plat_data *plat_priv,
				       void *data)
{
	struct cnss_qmi_event_server_arrive_data *event_data = data;
	struct qmi_handle *qmi_wlfw;
	struct sockaddr_qrtr sq = { 0 };
	int ret = 0;

	if (!event_data)
		return -EINVAL;

	if (!plat_priv)
		return -ENODEV;

	qmi_wlfw = &plat_priv->qmi_wlfw;
	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = event_data->node;
	sq.sq_port = event_data->port;

	ret = kernel_connect(qmi_wlfw->sock, (struct sockaddr *)&sq,
			     sizeof(sq), 0);
	if (ret < 0) {
		cnss_pr_err("Failed to connect to QMI WLFW remote service port\n");
		goto out;
	}

	set_bit(CNSS_QMI_WLFW_CONNECTED, &plat_priv->driver_state);

	cnss_pr_info("QMI WLFW service connected, state: 0x%lx\n",
		     plat_priv->driver_state);

	kfree(data);
	return 0;

out:
	CNSS_ASSERT(0);
	kfree(data);
	return ret;
}

int cnss_wlfw_server_arrive(struct cnss_plat_data *plat_priv, void *data)
{
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;
	clear_bit(CNSS_FW_READY, &plat_priv->driver_state);
	clear_bit(CNSS_FW_MEM_READY, &plat_priv->driver_state);

	ret = cnss_wlfw_connect_to_server(plat_priv, data);
	if (ret < 0)
		goto out;

	ret = cnss_wlfw_ind_register_send_sync(plat_priv);
	if (ret < 0) {
		if (ret == -EALREADY)
			ret = 0;
		goto out;
	}

	ret = cnss_wlfw_host_cap_send_sync(plat_priv);
	if (ret < 0)
		goto out;

	return 0;

out:
	return ret;
}

int cnss_wlfw_server_exit(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	clear_bit(CNSS_QMI_WLFW_CONNECTED, &plat_priv->driver_state);

	cnss_pr_info("QMI WLFW service disconnected, state: 0x%lx\n",
		     plat_priv->driver_state);
	return 0;
}

static int wlfw_new_server(struct qmi_handle *qmi_wlfw,
			   struct qmi_service *service)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);
	struct cnss_qmi_event_server_arrive_data *event_data;

	cnss_pr_dbg("WLFW server arriving: node %u port %u\n",
		    service->node, service->port);

	event_data = kzalloc(sizeof(*event_data), GFP_KERNEL);
	if (!event_data)
		return -ENOMEM;

	event_data->node = service->node;
	event_data->port = service->port;

	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_SERVER_ARRIVE,
			       0, event_data);

	return 0;
}

static void wlfw_del_server(struct qmi_handle *qmi_wlfw,
			    struct qmi_service *service)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi_wlfw, struct cnss_plat_data, qmi_wlfw);

	cnss_pr_dbg("WLFW server exiting\n");

	cnss_driver_event_post(plat_priv, CNSS_DRIVER_EVENT_SERVER_EXIT,
			       0, NULL);
}

static struct qmi_ops qmi_wlfw_ops = {
	.new_server = wlfw_new_server,
	.del_server = wlfw_del_server,
};

struct qmi_handle *whandle;

int cnss_qmi_init(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct device *dev;

	dev = &plat_priv->plat_dev->dev;

	if (plat_priv->device_id == QCA8074_DEVICE_ID ||
	    plat_priv->device_id == QCA8074V2_DEVICE_ID ||
	    plat_priv->device_id == QCA5018_DEVICE_ID ||
	    plat_priv->device_id == QCN6122_DEVICE_ID ||
	    plat_priv->device_id == QCA6018_DEVICE_ID ||
	    plat_priv->device_id == QCA9574_DEVICE_ID) {
		if (qca8074_fw_mem_mode != 0xFF) {
			plat_priv->tgt_mem_cfg_mode = qca8074_fw_mem_mode;
			pr_info("Using qca8074_fw_mem_mode 0x%x\n",
				qca8074_fw_mem_mode);
		} else if (of_property_read_u32(dev->of_node,
						"qcom,tgt-mem-mode",
						&plat_priv->tgt_mem_cfg_mode)) {
			pr_info("No qca8074_tgt_mem_mode entry in dev-tree.\n");
			plat_priv->tgt_mem_cfg_mode = 0;
		}
	} else if (plat_priv->device_id == QCN9000_DEVICE_ID ||
		   plat_priv->device_id == QCN9224_DEVICE_ID) {
		if (of_property_read_u32(dev->of_node,
					 "tgt-mem-mode",
					 &plat_priv->tgt_mem_cfg_mode)) {
			pr_info("No tgt-mem-mode entry in dev-tree.\n");
			plat_priv->tgt_mem_cfg_mode = 0;
		}
	}

	ret = qmi_handle_init(&plat_priv->qmi_wlfw,
			      QMI_WLFW_MAX_RECV_BUF_SIZE,
			      &qmi_wlfw_ops, qmi_wlfw_msg_handlers);
	if (ret < 0) {
		cnss_pr_err("Failed to initialize QMI handle, err: %d\n", ret);
		goto out;
	}

	ret = qmi_add_lookup(&plat_priv->qmi_wlfw, plat_priv->service_id,
				WLFW_SERVICE_VERS_V01,
				plat_priv->wlfw_service_instance_id);
	if (ret < 0) {
		cnss_pr_err("Failed to add QMI lookup, err: %d\n", ret);
		return ret;
	}

	whandle = &plat_priv->qmi_wlfw;

out:
	return ret;
}

void cnss_qmi_deinit(struct cnss_plat_data *plat_priv)
{
	qmi_handle_release(&plat_priv->qmi_wlfw);
}

#ifdef CNSS_COEX
int coex_antenna_switch_to_wlan_send_sync_msg(struct cnss_plat_data *plat_priv)
{
	int ret;
	struct coex_antenna_switch_to_wlan_req_msg_v01 *req;
	struct coex_antenna_switch_to_wlan_resp_msg_v01 *resp;
	struct qmi_txn txn;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_dbg("Sending coex antenna switch_to_wlan\n");

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->antenna = plat_priv->antenna;

	ret = qmi_txn_init(&plat_priv->coex_qmi, &txn,
			   coex_antenna_switch_to_wlan_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Fail to init txn for coex antenna switch_to_wlan resp %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request
		(&plat_priv->coex_qmi, NULL, &txn,
		 QMI_COEX_SWITCH_ANTENNA_TO_WLAN_REQ_V01,
		 COEX_ANTENNA_SWITCH_TO_WLAN_REQ_MSG_V01_MAX_MSG_LEN,
		 coex_antenna_switch_to_wlan_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Fail to send coex antenna switch_to_wlan req %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, COEX_TIMEOUT);
	if (ret < 0) {
		cnss_pr_err("Coex antenna switch_to_wlan resp wait failed with ret %d\n",
			    ret);
		goto out;
	} else if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Coex antenna switch_to_wlan request rejected, result:%d error:%d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		goto out;
	}

	if (resp->grant_valid)
		plat_priv->grant = resp->grant;

	cnss_pr_dbg("Coex antenna grant: 0x%llx\n", resp->grant);

	kfree(resp);
	kfree(req);
	return 0;

out:
	kfree(resp);
	kfree(req);
	return ret;
}

int coex_antenna_switch_to_mdm_send_sync_msg(struct cnss_plat_data *plat_priv)
{
	int ret;
	struct coex_antenna_switch_to_mdm_req_msg_v01 *req;
	struct coex_antenna_switch_to_mdm_resp_msg_v01 *resp;
	struct qmi_txn txn;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_dbg("Sending coex antenna switch_to_mdm\n");

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	resp = kzalloc(sizeof(*resp), GFP_KERNEL);
	if (!resp) {
		kfree(req);
		return -ENOMEM;
	}

	req->antenna = plat_priv->antenna;

	ret = qmi_txn_init(&plat_priv->coex_qmi, &txn,
			   coex_antenna_switch_to_mdm_resp_msg_v01_ei, resp);
	if (ret < 0) {
		cnss_pr_err("Fail to init txn for coex antenna switch_to_mdm resp %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request
		(&plat_priv->coex_qmi, NULL, &txn,
		 QMI_COEX_SWITCH_ANTENNA_TO_MDM_REQ_V01,
		 COEX_ANTENNA_SWITCH_TO_MDM_REQ_MSG_V01_MAX_MSG_LEN,
		 coex_antenna_switch_to_mdm_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(&txn);
		cnss_pr_err("Fail to send coex antenna switch_to_mdm req %d\n",
			    ret);
		goto out;
	}

	ret = qmi_txn_wait(&txn, COEX_TIMEOUT);
	if (ret < 0) {
		cnss_pr_err("Coex antenna switch_to_mdm resp wait failed with ret %d\n",
			    ret);
		goto out;
	} else if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("Coex antenna switch_to_mdm request rejected, result:%d error:%d\n",
			    resp->resp.result, resp->resp.error);
		ret = -resp->resp.result;
		goto out;
	}

	kfree(resp);
	kfree(req);
	return 0;

out:
	kfree(resp);
	kfree(req);
	return ret;
}

static int coex_new_server(struct qmi_handle *qmi,
			   struct qmi_service *service)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi, struct cnss_plat_data, coex_qmi);
	struct sockaddr_qrtr sq = { 0 };
	int ret = 0;

	cnss_pr_dbg("COEX server arrive: node %u port %u\n",
		    service->node, service->port);

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = service->node;
	sq.sq_port = service->port;
	ret = kernel_connect(qmi->sock, (struct sockaddr *)&sq, sizeof(sq), 0);
	if (ret < 0) {
		cnss_pr_err("Fail to connect to remote service port\n");
		return ret;
	}

	set_bit(CNSS_COEX_CONNECTED, &plat_priv->driver_state);
	cnss_pr_dbg("COEX Server Connected: 0x%lx\n",
		    plat_priv->driver_state);
	return 0;
}

static void coex_del_server(struct qmi_handle *qmi,
			    struct qmi_service *service)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi, struct cnss_plat_data, coex_qmi);

	cnss_pr_dbg("COEX server exit\n");

	clear_bit(CNSS_COEX_CONNECTED, &plat_priv->driver_state);
}

static struct qmi_ops coex_qmi_ops = {
	.new_server = coex_new_server,
	.del_server = coex_del_server,
};

int cnss_register_coex_service(struct cnss_plat_data *plat_priv)
{	int ret;

	ret = qmi_handle_init(&plat_priv->coex_qmi,
			      COEX_SERVICE_MAX_MSG_LEN,
			      &coex_qmi_ops, NULL);
	if (ret < 0)
		return ret;

	ret = qmi_add_lookup(&plat_priv->coex_qmi, COEX_SERVICE_ID_V01,
			     COEX_SERVICE_VERS_V01, 0);
	return ret;
}

void cnss_unregister_coex_service(struct cnss_plat_data *plat_priv)
{
	qmi_handle_release(&plat_priv->coex_qmi);
}
#endif

#ifdef CNSS2_IMS
/* IMS Service */
int ims_subscribe_for_indication_send_async(struct cnss_plat_data *plat_priv)
{
	int ret;
	struct ims_private_service_subscribe_for_indications_req_msg_v01 *req;
	struct qmi_txn *txn;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_dbg("Sending ASYNC ims subscribe for indication\n");

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	req->wfc_call_status_valid = 1;
	req->wfc_call_status = 1;

	txn = &plat_priv->txn;
	ret = qmi_txn_init(&plat_priv->ims_qmi, txn, NULL, NULL);
	if (ret < 0) {
		cnss_pr_err("Fail to init txn for ims subscribe for indication resp %d\n",
			    ret);
		goto out;
	}

	ret = qmi_send_request
	(&plat_priv->ims_qmi, NULL, txn,
	QMI_IMS_PRIVATE_SERVICE_SUBSCRIBE_FOR_INDICATIONS_REQ_V01,
	IMS_PRIVATE_SERVICE_SUBSCRIBE_FOR_INDICATIONS_REQ_MSG_V01_MAX_MSG_LEN,
	ims_private_service_subscribe_for_indications_req_msg_v01_ei, req);
	if (ret < 0) {
		qmi_txn_cancel(txn);
		cnss_pr_err("Fail to send ims subscribe for indication req %d\n",
			    ret);
		goto out;
	}

	kfree(req);
	return 0;

out:
	kfree(req);
	return ret;
}

static void ims_subscribe_for_indication_resp_cb(struct qmi_handle *qmi,
						 struct sockaddr_qrtr *sq,
						 struct qmi_txn *txn,
						 const void *data)
{
	const
	struct ims_private_service_subscribe_for_indications_rsp_msg_v01 *resp =
		data;

	cnss_pr_dbg("Received IMS subscribe indication response\n");

	if (!txn) {
		cnss_pr_err("spurious response\n");
		return;
	}

	if (resp->resp.result != QMI_RESULT_SUCCESS_V01) {
		cnss_pr_err("IMS subscribe for indication request rejected, result:%d error:%d\n",
			    resp->resp.result, resp->resp.error);
		txn->result = -resp->resp.result;
	}
}

static void ims_wfc_call_status_ind_cb(struct qmi_handle *ims_qmi,
				       struct sockaddr_qrtr *sq,
				       struct qmi_txn *txn, const void *data)
{
	struct cnss_plat_data *plat_priv =
		container_of(ims_qmi, struct cnss_plat_data, ims_qmi);
	const
	struct ims_private_service_wfc_call_status_ind_msg_v01 *ind_msg = data;
	u32 data_len = 0;

	cnss_pr_dbg("Received IMS wfc call status indication\n");

	if (!txn) {
		cnss_pr_err("Spurious indication\n");
		return;
	}

	if (!ind_msg) {
		cnss_pr_err("Invalid indication\n");
		return;
	}

	data_len = sizeof(*ind_msg);
	if (data_len > QMI_WLFW_MAX_WFC_CALL_STATUS_DATA_SIZE_V01) {
		cnss_pr_err("Exceed maxinum data len:%u\n", data_len);
		return;
	}

	cnss_wlfw_wfc_call_status_send_sync(plat_priv, data_len, ind_msg);
}

static struct qmi_msg_handler qmi_ims_msg_handlers[] = {
	{
		.type = QMI_RESPONSE,
		.msg_id =
		QMI_IMS_PRIVATE_SERVICE_SUBSCRIBE_FOR_INDICATIONS_REQ_V01,
		.ei =
		ims_private_service_subscribe_for_indications_rsp_msg_v01_ei,
		.decoded_size = sizeof(struct
		ims_private_service_subscribe_for_indications_rsp_msg_v01),
		.fn = ims_subscribe_for_indication_resp_cb
	},
	{
		.type = QMI_INDICATION,
		.msg_id = QMI_IMS_PRIVATE_SERVICE_WFC_CALL_STATUS_IND_V01,
		.ei = ims_private_service_wfc_call_status_ind_msg_v01_ei,
		.decoded_size =
		sizeof(struct ims_private_service_wfc_call_status_ind_msg_v01),
		.fn = ims_wfc_call_status_ind_cb
	},
	{}
};

static int ims_new_server(struct qmi_handle *qmi,
			  struct qmi_service *service)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi, struct cnss_plat_data, ims_qmi);
	struct sockaddr_qrtr sq = { 0 };
	int ret = 0;

	cnss_pr_dbg("IMS server arrive: node %u port %u\n",
		    service->node, service->port);

	sq.sq_family = AF_QIPCRTR;
	sq.sq_node = service->node;
	sq.sq_port = service->port;
	ret = kernel_connect(qmi->sock, (struct sockaddr *)&sq, sizeof(sq), 0);
	if (ret < 0) {
		cnss_pr_err("Fail to connect to remote service port\n");
		return ret;
	}

	set_bit(CNSS_IMS_CONNECTED, &plat_priv->driver_state);
	cnss_pr_dbg("IMS Server Connected: 0x%lx\n",
		    plat_priv->driver_state);

	ret = ims_subscribe_for_indication_send_async(plat_priv);
	return ret;
}

static void ims_del_server(struct qmi_handle *qmi,
			   struct qmi_service *service)
{
	struct cnss_plat_data *plat_priv =
		container_of(qmi, struct cnss_plat_data, ims_qmi);

	cnss_pr_dbg("IMS server exit\n");

	clear_bit(CNSS_IMS_CONNECTED, &plat_priv->driver_state);
}

static struct qmi_ops ims_qmi_ops = {
	.new_server = ims_new_server,
	.del_server = ims_del_server,
};

int cnss_register_ims_service(struct cnss_plat_data *plat_priv)
{	int ret;

	ret = qmi_handle_init(&plat_priv->ims_qmi,
			      IMSPRIVATE_SERVICE_MAX_MSG_LEN,
			      &ims_qmi_ops, qmi_ims_msg_handlers);
	if (ret < 0)
		return ret;

	ret = qmi_add_lookup(&plat_priv->ims_qmi, IMSPRIVATE_SERVICE_ID_V01,
			     IMSPRIVATE_SERVICE_VERS_V01, 0);
	return ret;
}

void cnss_unregister_ims_service(struct cnss_plat_data *plat_priv)
{
	qmi_handle_release(&plat_priv->ims_qmi);
}
#endif
