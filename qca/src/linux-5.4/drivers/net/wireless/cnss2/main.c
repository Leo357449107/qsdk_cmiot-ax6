/* Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
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

#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_wakeup.h>
#include <linux/rwsem.h>
#include <linux/suspend.h>
#include <linux/coresight.h>
#include <soc/qcom/ramdump.h>
#include <linux/remoteproc/qcom_rproc.h>
#include <linux/remoteproc.h>
#include <linux/of_address.h>
#ifdef CONFIG_CNSS2_QCA9574_SUPPORT
#include <soc/qcom/socinfo.h>
#endif

#include "main.h"
#include "debug.h"
#include "pci.h"
#include "qmi.h"
#include "bus.h"
#include "genl.h"

#define CNSS_DUMP_FORMAT_VER		0x11
#define CNSS_DUMP_FORMAT_VER_V2		0x22
#define CNSS_DUMP_MAGIC_VER_V2		0x42445953
#define CNSS_DUMP_NAME			"CNSS_WLAN"
#define CNSS_DUMP_DESC_SIZE		0x1000
#define CNSS_DUMP_SEG_VER		0x1
#define WLAN_RECOVERY_DELAY		1000
#define FILE_SYSTEM_READY		1
#define FW_ASSERT_TIMEOUT		5000
#define CNSS_EVENT_PENDING		2989

#define CNSS_QUIRKS_DEFAULT		0
#ifdef CONFIG_CNSS_EMULATION
#define CNSS_MHI_TIMEOUT_DEFAULT	3600
#else
#define CNSS_MHI_TIMEOUT_DEFAULT	60
#endif
#define CNSS_QMI_TIMEOUT_DEFAULT	10000
#define CNSS_BDF_TYPE_DEFAULT		CNSS_BDF_ELF
#define CNSS_TIME_SYNC_PERIOD_DEFAULT	900000
#define QCN9000_DEFAULT_FW_FILE_NAME	"qcn9000/amss.bin"
#define QCN9224_DEFAULT_FW_FILE_NAME	"qcn9224/amss.bin"
#define QCN9224_MLO_MIN_LINKS 2

#define MAX_NUMBER_OF_SOCS 4
struct cnss_plat_data *plat_env[MAX_NUMBER_OF_SOCS];
int plat_env_index;

#ifdef CONFIG_CNSS2_PM
static DECLARE_RWSEM(cnss_pm_sem);
#endif

bool ramdump_enabled;
module_param(ramdump_enabled, bool, 0600);
MODULE_PARM_DESC(ramdump_enabled, "ramdump_enabled");

static int bdf_integrated;
module_param(bdf_integrated, int, 0644);
MODULE_PARM_DESC(bdf_integrated, "bdf_integrated");

static int bdf_pci0;
module_param(bdf_pci0, int, 0644);
MODULE_PARM_DESC(bdf_pci0, "bdf_pci0");

static int bdf_pci1;
module_param(bdf_pci1, int, 0644);
MODULE_PARM_DESC(bdf_pci1, "bdf_pci1");

int timeout_factor = 1;
module_param(timeout_factor, int, 0644);
MODULE_PARM_DESC(timeout_factor, "timeout_factor");

static unsigned int driver_mode;
module_param(driver_mode, uint, 0644);
MODULE_PARM_DESC(driver_mode, "Global driver mode");

static int skip_cnss;
module_param(skip_cnss, int, 0644);
MODULE_PARM_DESC(skip_cnss, "skip_cnss");

static int skip_radio_bmap;
module_param(skip_radio_bmap, int, 0644);
MODULE_PARM_DESC(skip_radio_bmap, "Bitmap to skip device probe");

static int disable_caldata_bmap;
module_param(disable_caldata_bmap, int, 0644);
MODULE_PARM_DESC(disable_caldata_bmap, "Bitmap to Disable Caldata download");

static int disable_regdb_bmap = 0xF;
module_param(disable_regdb_bmap, int, 0644);
MODULE_PARM_DESC(disable_regdb_bmap, "Bitmap to Disable RegDB download");

#define FW_READY_DELAY	100  /* in msecs */
#ifdef CONFIG_KASAN
static int fw_ready_timeout = 60;
static int cold_boot_cal_timeout = 180;
#else
static int fw_ready_timeout = 15;
static int cold_boot_cal_timeout = 60;
#endif
module_param(fw_ready_timeout, int, 0644);
MODULE_PARM_DESC(fw_ready_timeout, "fw ready timeout in seconds");

module_param(cold_boot_cal_timeout, int, 0644);
MODULE_PARM_DESC(cold_boot_cal_timeout, "Cold boot cal timeout in seconds");

static int soc_version_major;
module_param(soc_version_major, int, 0444);
MODULE_PARM_DESC(soc_version_major, "SOC Major Version");

static int enable_qcn9224_support;
module_param(enable_qcn9224_support, int, 0444);
MODULE_PARM_DESC(enable_qcn9224_support, "Enable QCN9224 support");

enum skip_cnss_options {
	CNSS_SKIP_NONE,
	CNSS_SKIP_ALL,
	CNSS_SKIP_AHB,
	CNSS_SKIP_PCI
};

#define SKIP_INTEGRATED		0x1
#define SKIP_PCI_0		0x2
#define SKIP_PCI_1		0x4
#define SKIP_PCI_2		0x8
#define SKIP_PCI_3		0x10

static struct cnss_fw_files FW_FILES_QCA6174_FW_3_0 = {
	"qwlan30.bin", "bdwlan30.bin", "otp30.bin", "utf30.bin",
	"utfbd30.bin", "epping30.bin", "evicted30.bin"
};

static struct cnss_fw_files FW_FILES_DEFAULT = {
	"qwlan.bin", "bdwlan.bin", "otp.bin", "utf.bin",
	"utfbd.bin", "epping.bin", "evicted.bin"
};

struct cnss_driver_event {
	struct list_head list;
	enum cnss_driver_event_type type;
	bool sync;
	struct completion complete;
	int ret;
	void *data;
};

/* M3 Dump related global structures/variables */
static int m3_dump_major;
static struct class *m3_dump_class;

static void cnss_set_plat_priv(struct platform_device *plat_dev,
			       struct cnss_plat_data *plat_priv)
{
	if (plat_env_index >= MAX_NUMBER_OF_SOCS) {
		pr_err("ERROR: No space to allocate save the plat_priv\n");
		return;
	}
	plat_env[plat_env_index++] = plat_priv;
}

void *cnss_get_pci_dev_by_device_id(int device_id)
{
	int i;

	for (i = 0; i < plat_env_index; i++) {
		if (plat_env[i]->device_id == device_id)
			return plat_env[i]->pci_dev;
	}
	return NULL;
}
EXPORT_SYMBOL(cnss_get_pci_dev_by_device_id);

void *cnss_get_plat_priv_dev_by_pci_dev(void *pci_dev)
{
	int i;

	for (i = 0; i < plat_env_index; i++) {
		if (plat_env[i]->pci_dev == pci_dev)
			return plat_env[i];
	}
	return NULL;
}

struct cnss_plat_data *cnss_get_plat_priv_by_qrtr_node_id(int node_id)
{
	int i;

	for (i = 0; i < plat_env_index; i++) {
		if (plat_env[i]->qrtr_node_id == node_id)
			return plat_env[i];
	}
	return NULL;
}

struct cnss_plat_data *cnss_get_plat_priv_by_instance_id(int instance_id)
{
	int i;

	for (i = 0; i < plat_env_index; i++) {
		if (plat_env[i]->wlfw_service_instance_id == instance_id)
			return plat_env[i];
	}
	return NULL;
}

struct cnss_plat_data *cnss_get_plat_priv_by_device_id(int id)
{
	int i;

	for (i = 0; i < plat_env_index; i++) {
		if (plat_env[i]->device_id == id &&
		    !plat_env[i]->pci_dev)
			return plat_env[i];
	}
	return NULL;
}

struct cnss_plat_data *cnss_get_plat_priv(struct platform_device
						 *plat_dev)
{
	int i;

	if (!plat_dev)
		return NULL;

	for (i = 0; i < plat_env_index; i++) {
		if (plat_env[i]->plat_dev == plat_dev)
			return plat_env[i];
	}
	return NULL;
}

int cnss_get_plat_env_index_from_plat_priv(struct cnss_plat_data *plat_priv)
{
	int i;

	if (!plat_priv)
		return -EINVAL;

	for (i = 0; i < plat_env_index; i++) {
		if (plat_env[i] == plat_priv)
			return i;
	}

	return -EINVAL;
}

#ifdef CONFIG_CNSS2_PM
static int cnss_pm_notify(struct notifier_block *b,
			  unsigned long event, void *p)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
		down_write(&cnss_pm_sem);
		break;
	case PM_POST_SUSPEND:
		up_write(&cnss_pm_sem);
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block cnss_pm_notifier = {
	.notifier_call = cnss_pm_notify,
};

static void cnss_pm_stay_awake(struct cnss_plat_data *plat_priv)
{
	if (atomic_inc_return(&plat_priv->pm_count) != 1)
		return;

	cnss_pr_dbg("PM stay awake, state: 0x%lx, count: %d\n",
		    plat_priv->driver_state,
		    atomic_read(&plat_priv->pm_count));
	pm_stay_awake(&plat_priv->plat_dev->dev);
}

static void cnss_pm_relax(struct cnss_plat_data *plat_priv)
{
	int r = atomic_dec_return(&plat_priv->pm_count);

	WARN_ON(r < 0);

	if (r != 0)
		return;

	cnss_pr_dbg("PM relax, state: 0x%lx, count: %d\n",
		    plat_priv->driver_state,
		    atomic_read(&plat_priv->pm_count));
	pm_relax(&plat_priv->plat_dev->dev);
}
void cnss_lock_pm_sem(struct device *dev)
{
	down_read(&cnss_pm_sem);
}
EXPORT_SYMBOL(cnss_lock_pm_sem);

void cnss_release_pm_sem(struct device *dev)
{
	up_read(&cnss_pm_sem);
}
EXPORT_SYMBOL(cnss_release_pm_sem);
#endif

int cnss_get_fw_files_for_target(struct device *dev,
				 struct cnss_fw_files *pfw_files,
				 u32 target_type, u32 target_version)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return -EINVAL;

	if (!pfw_files)
		return -ENODEV;

	switch (target_version) {
	case QCA6174_REV3_VERSION:
	case QCA6174_REV3_2_VERSION:
		memcpy(pfw_files, &FW_FILES_QCA6174_FW_3_0, sizeof(*pfw_files));
		break;
	default:
		memcpy(pfw_files, &FW_FILES_DEFAULT, sizeof(*pfw_files));
		cnss_pr_err("Unknown target version, type: 0x%X, version: 0x%X",
			    target_type, target_version);
		break;
	}

	return 0;
}
EXPORT_SYMBOL(cnss_get_fw_files_for_target);

int cnss_request_bus_bandwidth(struct device *dev, int bandwidth)
{
#ifdef CONFIG_MSM_PCI
	int ret;
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	struct cnss_bus_bw_info *bus_bw_info;

	if (!plat_priv)
		return -ENODEV;

	bus_bw_info = &plat_priv->bus_bw_info;
	if (!bus_bw_info->bus_client)
		return -EINVAL;

	switch (bandwidth) {
	case CNSS_BUS_WIDTH_NONE:
	case CNSS_BUS_WIDTH_IDLE:
	case CNSS_BUS_WIDTH_LOW:
	case CNSS_BUS_WIDTH_MEDIUM:
	case CNSS_BUS_WIDTH_HIGH:
	case CNSS_BUS_WIDTH_VERY_HIGH:
		ret = msm_bus_scale_client_update_request
			(bus_bw_info->bus_client, bandwidth);
		if (!ret)
			bus_bw_info->current_bw_vote = bandwidth;
		else
			cnss_pr_err("Could not set bus bandwidth: %d, err = %d\n",
				    bandwidth, ret);
		break;
	default:
		cnss_pr_err("Invalid bus bandwidth: %d", bandwidth);
		ret = -EINVAL;
	}

	return ret;
#endif
	return 0;
}
EXPORT_SYMBOL(cnss_request_bus_bandwidth);

int cnss_get_platform_cap(struct device *dev, struct cnss_platform_cap *cap)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return -ENODEV;

	if (cap)
		*cap = plat_priv->cap;

	return 0;
}
EXPORT_SYMBOL(cnss_get_platform_cap);

void cnss_request_pm_qos(struct device *dev, u32 qos_val)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return;

	pm_qos_add_request(&plat_priv->qos_request, PM_QOS_CPU_DMA_LATENCY,
			   qos_val);
}
EXPORT_SYMBOL(cnss_request_pm_qos);

void cnss_remove_pm_qos(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return;

	pm_qos_remove_request(&plat_priv->qos_request);
}
EXPORT_SYMBOL(cnss_remove_pm_qos);

int cnss_wlan_enable(struct device *dev,
		     struct cnss_wlan_enable_cfg *config,
		     enum cnss_driver_mode mode,
		     const char *host_version)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	int ret;

	if (!plat_priv)
		return 0;

	if (plat_priv->device_id == QCA6174_DEVICE_ID)
		return 0;

	if (test_bit(QMI_BYPASS, &plat_priv->ctrl_params.quirks))
		return 0;

	if (mode == CNSS_CALIBRATION || mode == CNSS_WALTEST ||
	    mode == CNSS_FTM_CALIBRATION)
		goto skip_cfg;

	if (!config || !host_version) {
		cnss_pr_err("Invalid config or host_version pointer\n");
		return -EINVAL;
	}

	/* Set wmi diag logging */
	if (!(plat_priv->device_id == QCA8074_DEVICE_ID ||
	      plat_priv->device_id == QCA8074V2_DEVICE_ID ||
	      plat_priv->device_id == QCA6018_DEVICE_ID))
		cnss_set_fw_log_mode(dev, 1);

	cnss_pr_dbg("Mode: %d, config: %pK, host_version: %s\n",
		    mode, config, host_version);

	if (mode == CNSS_WALTEST || mode == CNSS_CCPM)
		goto skip_cfg;

	if (mode != CNSS_CALIBRATION && mode != CNSS_FTM_CALIBRATION) {
		ret = cnss_wlfw_wlan_cfg_send_sync(plat_priv, config,
						   host_version);
		if (ret)
			goto out;
	}

skip_cfg:

	if (mode == CNSS_CALIBRATION || mode == CNSS_FTM_CALIBRATION)
		set_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state);

	ret = cnss_wlfw_wlan_mode_send_sync(plat_priv, mode);
out:
	return ret;
}
EXPORT_SYMBOL(cnss_wlan_enable);

int cnss_wlan_disable(struct device *dev, enum cnss_driver_mode mode)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return 0;

	if (plat_priv->device_id == QCA6174_DEVICE_ID)
		return 0;

	if (test_bit(QMI_BYPASS, &plat_priv->ctrl_params.quirks))
		return 0;

	return cnss_wlfw_wlan_mode_send_sync(plat_priv, CNSS_OFF);
}
EXPORT_SYMBOL(cnss_wlan_disable);

int cnss_athdiag_read(struct device *dev, u32 offset, u32 mem_type,
		      u32 data_len, u8 *output)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	int ret = 0;

	if (!plat_priv) {
		pr_err("plat_priv is NULL!\n");
		return -EINVAL;
	}

	if (plat_priv->device_id == QCA6174_DEVICE_ID)
		return 0;

	if (!output || data_len == 0 || data_len > QMI_WLFW_MAX_DATA_SIZE_V01) {
		cnss_pr_err("Inv param athdiag read: output %p, data_len %u\n",
			    output, data_len);
		ret = -EINVAL;
		goto out;
	}

	if (!test_bit(CNSS_FW_READY, &plat_priv->driver_state)) {
		cnss_pr_err("Invalid state for athdiag read: 0x%lx\n",
			    plat_priv->driver_state);
		ret = -EINVAL;
		goto out;
	}

	ret = cnss_wlfw_athdiag_read_send_sync(plat_priv, offset, mem_type,
					       data_len, output);

out:
	return ret;
}
EXPORT_SYMBOL(cnss_athdiag_read);

int cnss_athdiag_write(struct device *dev, u32 offset, u32 mem_type,
		       u32 data_len, u8 *input)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	int ret = 0;

	if (!plat_priv) {
		pr_err("plat_priv is NULL!\n");
		return -EINVAL;
	}

	if (plat_priv->device_id == QCA6174_DEVICE_ID)
		return 0;

	if (!input || data_len == 0 || data_len > QMI_WLFW_MAX_DATA_SIZE_V01) {
		cnss_pr_err("Inv param athdiag write: input %p, data_len %u\n",
			    input, data_len);
		ret = -EINVAL;
		goto out;
	}

	if (!test_bit(CNSS_FW_READY, &plat_priv->driver_state)) {
		cnss_pr_err("Invalid state for athdiag write: 0x%lx\n",
			    plat_priv->driver_state);
		ret = -EINVAL;
		goto out;
	}

	ret = cnss_wlfw_athdiag_write_send_sync(plat_priv, offset, mem_type,
						data_len, input);

out:
	return ret;
}
EXPORT_SYMBOL(cnss_athdiag_write);

int cnss_set_fw_log_mode(struct device *dev, u8 fw_log_mode)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return -EINVAL;

	if (plat_priv->device_id == QCA6174_DEVICE_ID)
		return 0;

	return cnss_wlfw_ini_send_sync(plat_priv, fw_log_mode);
}
EXPORT_SYMBOL(cnss_set_fw_log_mode);

static int cnss_fw_mem_ready_hdlr(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	set_bit(CNSS_FW_MEM_READY, &plat_priv->driver_state);

	ret = cnss_wlfw_tgt_cap_send_sync(plat_priv);
	if (ret)
		goto out;

	if (plat_priv->device_id == QCN6122_DEVICE_ID) {
		ret = cnss_wlfw_device_info_send_sync(plat_priv);
		if (ret) {
			cnss_pr_err("Device info msg failed. ret %d\n", ret);
			goto out;
		}
	}

	if (plat_priv->hds_support) {
		ret = cnss_wlfw_bdf_dnld_send_sync(plat_priv, CNSS_BDF_HDS);
		if (ret) {
			cnss_pr_err("hds load failed. ret %d\n", ret);
			goto out;
		}
	}

	if (plat_priv->regdb_support) {
		ret = cnss_wlfw_bdf_dnld_send_sync(plat_priv,
						   CNSS_BDF_REGDB);
		if (ret) {
			cnss_pr_err("regdb load failed. ret %d\n", ret);
			goto out;
		}
	}

	ret = cnss_wlfw_bdf_dnld_send_sync(plat_priv, CNSS_BDF_WIN);
	if (ret) {
		cnss_pr_err("bdf load failed. ret %d\n", ret);
		goto out;
	}

	if (plat_priv->caldata_support) {
		ret = cnss_wlfw_bdf_dnld_send_sync(plat_priv, CNSS_CALDATA_WIN);
		if (ret) {
			cnss_pr_err("caldata load failed. ret %d\n", ret);
			goto out;
		}
	}

	if (plat_priv->device_id == QCN9000_DEVICE_ID ||
	    plat_priv->device_id == QCN9224_DEVICE_ID) {
		ret = cnss_bus_load_m3(plat_priv);
		if (ret) {
			cnss_pr_err("m3 load failed. ret %d\n", ret);
			goto out;
		}
	}

	ret = cnss_wlfw_m3_dnld_send_sync(plat_priv);
	if (ret) {
		cnss_pr_err("m3 dnld failed. ret %d\n", ret);
		goto out;
	}

	return 0;
out:
	return ret;
}

static int cnss_request_antenna_sharing(struct cnss_plat_data *plat_priv)
{
#ifdef CNSS2_COEX
	int ret = 0;

	if (!plat_priv->antenna) {
		ret = cnss_wlfw_antenna_switch_send_sync(plat_priv);
		if (ret)
			goto out;
	}

	if (test_bit(CNSS_COEX_CONNECTED, &plat_priv->driver_state)) {
		ret = coex_antenna_switch_to_wlan_send_sync_msg(plat_priv);
		if (ret)
			goto out;
	}

	ret = cnss_wlfw_antenna_grant_send_sync(plat_priv);
	if (ret)
		goto out;

	return 0;

out:
	return ret;
#endif
	return 0;
}

static void cnss_release_antenna_sharing(struct cnss_plat_data *plat_priv)
{
#ifdef CNSS2_COEX
	if (test_bit(CNSS_COEX_CONNECTED, &plat_priv->driver_state))
		coex_antenna_switch_to_mdm_send_sync_msg(plat_priv);
#endif
}

void cnss_get_ramdump_device_name(struct device *dev,
				  char *ramdump_dev_name,
				  size_t ramdump_dev_name_len)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	bool multi_pd_arch = false;
	const char *subsys_name;

	if (!plat_priv || !ramdump_dev_name)
		return;

	switch (plat_priv->device_id) {
	case QCA8074_DEVICE_ID:
	case QCA8074V2_DEVICE_ID:
	case QCA6018_DEVICE_ID:
	case QCA9574_DEVICE_ID:
		snprintf(ramdump_dev_name, ramdump_dev_name_len, "q6mem");
		break;
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
		snprintf(ramdump_dev_name, ramdump_dev_name_len, "ramdump_%s",
			 plat_priv->device_name);
		break;
	case QCA5018_DEVICE_ID:
	case QCN6122_DEVICE_ID:
		multi_pd_arch = of_property_read_bool(dev->of_node,
						      "qcom,multipd_arch");
		if (multi_pd_arch) {
			of_property_read_string(dev->of_node,
						"qcom,userpd-subsys-name",
						&subsys_name);
			snprintf(ramdump_dev_name, ramdump_dev_name_len,
				 "%s_mem", subsys_name);
		} else {
			snprintf(ramdump_dev_name, ramdump_dev_name_len,
				 "q6mem");
		}
		break;
	default:
		cnss_pr_info("%s: Unknown device_id 0x%lx",
			     __func__, plat_priv->device_id);
	}
	cnss_pr_dbg("Ramdump device name %s for device 0x%lx\n",
		    ramdump_dev_name, plat_priv->device_id);
}
EXPORT_SYMBOL(cnss_get_ramdump_device_name);

int cnss_get_mlo_chip_id(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	u8 mlo_chip_id = 0;
	struct device *bus_dev;
	struct device_node *mlo_chip_node = NULL;
	phandle mlo_chip_phandle;

	if (!plat_priv)
		return -EINVAL;

	if (plat_priv->device_id != QCN9224_DEVICE_ID)
		return -EINVAL;

	bus_dev = &plat_priv->plat_dev->dev;

	if (of_property_read_u32(bus_dev->of_node, "mlo_chip_info",
				 &mlo_chip_phandle)) {
		cnss_pr_err("could not get mlo_chip_phandle\n");
		return -EINVAL;
	}

	mlo_chip_node = of_find_node_by_phandle(mlo_chip_phandle);
	if (!mlo_chip_node) {
		cnss_pr_err("could not get mlo_chip_node\n");
		return -EINVAL;
	}
	if (of_property_read_u8(mlo_chip_node,
				"chip_id",
				&mlo_chip_id)) {
		cnss_pr_err("Error: No MLO CHIP ID present\n");
		of_node_put(mlo_chip_node);
		return -EINVAL;
	}
	of_node_put(mlo_chip_node);
	return (int)mlo_chip_id;
}
EXPORT_SYMBOL(cnss_get_mlo_chip_id);

bool cnss_get_mlo_capable(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	bool mlo_capable = false;
	struct device *bus_dev;
	phandle mlo_group_phandle = 0;

	if (!plat_priv)
		return false;

	if (plat_priv->device_id != QCN9224_DEVICE_ID)
		return false;

	bus_dev = &plat_priv->plat_dev->dev;

	if (of_property_read_u32(bus_dev->of_node, "mlo_group_info",
				 &mlo_group_phandle)) {
		/* no entry found, disable mlo capability */
		mlo_capable = false;
	}
	if (mlo_group_phandle)
		mlo_capable = true;

	return mlo_capable;
}
EXPORT_SYMBOL(cnss_get_mlo_capable);

int cnss_get_mlo_global_config_region_info(struct device *dev,
					   void **bar,
					   int *num_bytes)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	struct device_node *mlo_global_mem_node;
	struct resource mlo_mem;
	struct cnss_fw_mem *fw_mem;
	int i;

	if (!plat_priv)
		return -EINVAL;

	if (plat_priv->device_id != QCN9224_DEVICE_ID)
		return -EINVAL;

	mlo_global_mem_node = of_find_node_by_name(NULL, "mlo_global_mem0");
	if (!mlo_global_mem_node) {
		cnss_pr_err("could not get mlo_global_mem_node\n");
		return -EINVAL;
	}

	if (of_address_to_resource(mlo_global_mem_node, 0, &mlo_mem)) {
		cnss_pr_err("%s: Unable to read mlo_mem", __func__);
		of_node_put(mlo_global_mem_node);
		return -EINVAL;
	}
	of_node_put(mlo_global_mem_node);

	*bar = 0;
	fw_mem = plat_priv->fw_mem;
	for (i = 0; i < plat_priv->fw_mem_seg_len; i++) {
		if (fw_mem[i].va) {
			if (fw_mem[i].type == QMI_WLFW_MLO_GLOBAL_MEM_V01)
				*bar = plat_priv->fw_mem[i].va;
		}
	}
	if (!*bar) {
		cnss_pr_err("%s: no mlo mem found", __func__);
		return -EINVAL;
	}
	*num_bytes = resource_size(&mlo_mem);
	return 0;
}
EXPORT_SYMBOL(cnss_get_mlo_global_config_region_info);

int cnss_get_num_mlo_links(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	u8 num_local_links = 0;
	struct device *bus_dev;
	struct device_node *mlo_chip_node = NULL;
	phandle mlo_chip_phandle;

	if (!plat_priv)
		return -EINVAL;

	if (plat_priv->device_id != QCN9224_DEVICE_ID)
		return -EINVAL;

	bus_dev = &plat_priv->plat_dev->dev;

	if (of_property_read_u32(bus_dev->of_node, "mlo_chip_info",
				 &mlo_chip_phandle)) {
		cnss_pr_err("could not get mlo_chip_phandle\n");
		return -EINVAL;
	}

	mlo_chip_node = of_find_node_by_phandle(mlo_chip_phandle);
	if (!mlo_chip_node) {
		cnss_pr_err("could not get mlo_chip_node\n");
		return -EINVAL;
	}
	if (of_property_read_u8(mlo_chip_node,
				"num_local_links",
				&num_local_links)) {
		of_node_put(mlo_chip_node);
		cnss_pr_err("Error: No num_local_links is present\n");
		return -EINVAL;
	}
	of_node_put(mlo_chip_node);
	return (int)num_local_links;
}
EXPORT_SYMBOL(cnss_get_num_mlo_links);

int cnss_get_num_mlo_capable_devices(unsigned int *device_id, int num_elements)
{
	struct cnss_plat_data *plat_priv = NULL;
	struct device *dev;
	int num_capable = 0;
	int i;
	int device_count = 0;
	phandle mlo_group_phandle;

	for (i = 0; i < MAX_NUMBER_OF_SOCS; i++) {
		plat_priv = plat_env[i];
		if (!plat_priv)
			break;

		dev = &plat_priv->plat_dev->dev;

		mlo_group_phandle = 0;
		if (of_property_read_u32(dev->of_node, "mlo_group_info",
					 &mlo_group_phandle))
			continue;

		if (mlo_group_phandle) {
			num_capable++;
			if (device_id && device_count < num_elements)
				device_id[device_count++] =
						plat_priv->device_id;
		}
	}
	return num_capable;
}
EXPORT_SYMBOL(cnss_get_num_mlo_capable_devices);

int cnss_get_dev_link_ids(struct device *dev, u8 *link_ids, int max_elements)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	struct device *bus_dev;
	struct device_node *mlo_chip_node = NULL;
	phandle mlo_chip_phandle;
	int num_elements;

	if (!plat_priv)
		return -EINVAL;

	if (plat_priv->device_id != QCN9224_DEVICE_ID)
		return -EINVAL;

	if (!link_ids) {
		cnss_pr_err("link_ids buffer is null\n");
		return -ENOMEM;
	}

	if (max_elements < QCN9224_MLO_MIN_LINKS) {
		cnss_pr_err("link ids size is less %d\n", max_elements);
		return -EINVAL;
	}

	bus_dev = &plat_priv->plat_dev->dev;

	if (of_property_read_u32(bus_dev->of_node, "mlo_chip_info",
				 &mlo_chip_phandle)) {
		cnss_pr_err("could not get mlo_chip_phandle\n");
		return -EINVAL;
	}

	mlo_chip_node = of_find_node_by_phandle(mlo_chip_phandle);
	if (!mlo_chip_node) {
		cnss_pr_err("could not get mlo_chip_node\n");
		return -EINVAL;
	}
	memset(link_ids, 0, max_elements);

	num_elements = of_property_read_variable_u8_array(mlo_chip_node,
							  "hw_link_ids",
							  link_ids,
							  QCN9224_MLO_MIN_LINKS,
							  0);
	of_node_put(mlo_chip_node);
	if (num_elements < 0) {
		cnss_pr_err("Error: couldn't read hw_ids(%d)\n", num_elements);
		return -EINVAL;
	}
	return num_elements;
}
EXPORT_SYMBOL(cnss_get_dev_link_ids);

void cnss_wait_for_fw_ready(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	int count = 0;

	if (!plat_priv)
		return;

	if (plat_priv->device_id == QCA8074_DEVICE_ID ||
	    plat_priv->device_id == QCA8074V2_DEVICE_ID ||
	    plat_priv->device_id == QCA6018_DEVICE_ID ||
	    plat_priv->device_id == QCA5018_DEVICE_ID ||
	    plat_priv->device_id == QCN6122_DEVICE_ID ||
	    plat_priv->device_id == QCN9224_DEVICE_ID ||
	    plat_priv->device_id == QCA9574_DEVICE_ID ||
	    plat_priv->device_id == QCN9000_DEVICE_ID) {
		cnss_pr_info("Waiting for FW ready. Device: 0x%lx, FW ready timeout: %d seconds\n",
			     plat_priv->device_id, fw_ready_timeout);
		while (!test_bit(CNSS_FW_READY, &plat_priv->driver_state)) {
			msleep(FW_READY_DELAY);
			if (count++ > fw_ready_timeout * 10) {
				cnss_pr_err("FW ready timed-out %d seconds\n",
					    fw_ready_timeout);
				CNSS_ASSERT(0);
			}
		}
		cnss_pr_info("FW ready received for device 0x%lx\n",
			     plat_priv->device_id);
	}
}
EXPORT_SYMBOL(cnss_wait_for_fw_ready);

void cnss_wait_for_cold_boot_cal_done(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	int count = 0;

	if (!plat_priv)
		return;

	if (plat_priv->device_id == QCA8074_DEVICE_ID ||
	    plat_priv->device_id == QCA8074V2_DEVICE_ID ||
	    plat_priv->device_id == QCA6018_DEVICE_ID ||
	    plat_priv->device_id == QCN6122_DEVICE_ID ||
	    plat_priv->device_id == QCN9224_DEVICE_ID ||
	    plat_priv->device_id == QCA5018_DEVICE_ID ||
	    plat_priv->device_id == QCN9000_DEVICE_ID ||
	    plat_priv->device_id == QCA9574_DEVICE_ID) {
		/* Cold boot Calibration is done parallely for multiple devices
		 * Check if this device has already completed cold boot cal
		 * If already completed, we need not wait
		 */
		if (!test_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state))
			return;

		cnss_pr_info("Coldboot Calbration wait started for Device: 0x%lx, timeout: %d seconds\n",
			     plat_priv->device_id, cold_boot_cal_timeout);
		while (test_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state)) {
			msleep(FW_READY_DELAY);
			if (count++ > cold_boot_cal_timeout * 10) {
				cnss_pr_err("Coldboot calibration timed out %d seconds\n",
					    cold_boot_cal_timeout);
				CNSS_ASSERT(0);
			}
		}
		cnss_pr_info("Coldboot Calibration wait ended for device 0x%lx\n",
			     plat_priv->device_id);
	}
}
EXPORT_SYMBOL(cnss_wait_for_cold_boot_cal_done);

void cnss_set_ramdump_enabled(struct device *dev, bool enabled)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv) {
		pr_err("%s: Failed to get plat_priv", __func__);
		return;
	}

	/* This is temporarily same as cnss_set_recovery_enabled until the
	 * wifi driver switches to use cnss_set_recovery_enabled.
	 */
	plat_priv->recovery_enabled = enabled;
	cnss_pr_dbg("Setting recovery_enabled to %d for %s\n",
		    plat_priv->recovery_enabled,
		    plat_priv->device_name);
}
EXPORT_SYMBOL(cnss_set_ramdump_enabled);

void cnss_set_recovery_enabled(struct device *dev, bool enabled)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv) {
		pr_err("%s: Failed to get plat_priv", __func__);
		return;
	}

	plat_priv->recovery_enabled = enabled;
	cnss_pr_dbg("Setting recovery_enabled to %d for %s\n",
		    plat_priv->recovery_enabled,
		    plat_priv->device_name);
}
EXPORT_SYMBOL(cnss_set_recovery_enabled);

static int cnss_fw_ready_hdlr(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	del_timer(&plat_priv->fw_boot_timer);
	set_bit(CNSS_FW_READY, &plat_priv->driver_state);
	clear_bit(CNSS_DEV_ERR_NOTIFY, &plat_priv->driver_state);

	if (test_bit(CNSS_FW_BOOT_RECOVERY, &plat_priv->driver_state)) {
		clear_bit(CNSS_FW_BOOT_RECOVERY, &plat_priv->driver_state);
		clear_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state);
	}

	if (test_bit(ENABLE_WALTEST, &plat_priv->ctrl_params.quirks)) {
		ret = cnss_wlfw_wlan_mode_send_sync(plat_priv,
						    CNSS_WALTEST);
	} else if (test_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state)) {
		cnss_request_antenna_sharing(plat_priv);
		ret = cnss_wlfw_wlan_mode_send_sync(plat_priv,
						    CNSS_CALIBRATION);
	} else if (test_bit(CNSS_DRIVER_LOADING, &plat_priv->driver_state) ||
		   test_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state)) {
	} else {
		ret = cnss_bus_call_driver_probe(plat_priv);
		complete(&plat_priv->power_up_complete);
	}

	if (ret && test_bit(CNSS_DEV_ERR_NOTIFY, &plat_priv->driver_state))
		goto out;
	else if (ret)
		goto shutdown;
#ifdef CNSS2_VREG
	cnss_vreg_unvote_type(plat_priv, CNSS_VREG_PRIM);
#endif

	return 0;

shutdown:
	cnss_bus_dev_shutdown(plat_priv);

	clear_bit(CNSS_FW_READY, &plat_priv->driver_state);
	clear_bit(CNSS_FW_MEM_READY, &plat_priv->driver_state);

out:
	cnss_pr_err("%s:%d ret %d\n", __func__, __LINE__, ret);
	return ret;
}

static char *cnss_driver_event_to_str(enum cnss_driver_event_type type)
{
	switch (type) {
	case CNSS_DRIVER_EVENT_SERVER_ARRIVE:
		return "SERVER_ARRIVE";
	case CNSS_DRIVER_EVENT_SERVER_EXIT:
		return "SERVER_EXIT";
	case CNSS_DRIVER_EVENT_REQUEST_MEM:
		return "REQUEST_MEM";
	case CNSS_DRIVER_EVENT_FW_MEM_READY:
		return "FW_MEM_READY";
	case CNSS_DRIVER_EVENT_FW_READY:
		return "FW_READY";
	case CNSS_DRIVER_EVENT_COLD_BOOT_CAL_START:
		return "COLD_BOOT_CAL_START";
	case CNSS_DRIVER_EVENT_COLD_BOOT_CAL_DONE:
		return "COLD_BOOT_CAL_DONE";
	case CNSS_DRIVER_EVENT_REGISTER_DRIVER:
		return "REGISTER_DRIVER";
	case CNSS_DRIVER_EVENT_UNREGISTER_DRIVER:
		return "UNREGISTER_DRIVER";
	case CNSS_DRIVER_EVENT_RECOVERY:
		return "RECOVERY";
	case CNSS_DRIVER_EVENT_FORCE_FW_ASSERT:
		return "FORCE_FW_ASSERT";
	case CNSS_DRIVER_EVENT_POWER_UP:
		return "POWER_UP";
	case CNSS_DRIVER_EVENT_POWER_DOWN:
		return "POWER_DOWN";
	case CNSS_DRIVER_EVENT_IDLE_RESTART:
		return "IDLE_RESTART";
	case CNSS_DRIVER_EVENT_IDLE_SHUTDOWN:
		return "IDLE_SHUTDOWN";
	case CNSS_DRIVER_EVENT_QDSS_TRACE_REQ_MEM:
		return "QDSS_TRACE_REQ_MEM";
	case CNSS_DRIVER_EVENT_QDSS_TRACE_SAVE:
		return "QDSS_TRACE_SAVE";
	case CNSS_DRIVER_EVENT_QDSS_TRACE_FREE:
		return "QDSS_TRACE_FREE";
	case CNSS_DRIVER_EVENT_M3_DUMP_UPLOAD_REQ:
		return "M3_DUMP_UPLOAD_REQ";
	case CNSS_DRIVER_EVENT_MAX:
		return "EVENT_MAX";
	}

	return "UNKNOWN";
};

int cnss_driver_event_post(struct cnss_plat_data *plat_priv,
			   enum cnss_driver_event_type type,
			   u32 flags, void *data)
{
	struct cnss_driver_event *event;
	unsigned long irq_flags;
	int gfp = GFP_KERNEL;
	int ret = 0;

	if (!plat_priv)
		return -ENODEV;

	cnss_pr_dbg("Posting event[%p]: %s(%d)%s, state: 0x%lx flags: 0x%0x\n",
		    plat_priv,
		    cnss_driver_event_to_str(type), type,
		    flags ? "-sync" : "", plat_priv->driver_state, flags);

	if (type >= CNSS_DRIVER_EVENT_MAX) {
		cnss_pr_err("Invalid Event type: %d, can't post", type);
		return -EINVAL;
	}

	if (in_interrupt() || irqs_disabled())
		gfp = GFP_ATOMIC;

	event = kzalloc(sizeof(*event), gfp);
	if (!event)
		return -ENOMEM;

#ifdef CONFIG_CNSS2_PM
	cnss_pm_stay_awake(plat_priv);
#endif

	event->type = type;
	event->data = data;
	init_completion(&event->complete);
	event->ret = CNSS_EVENT_PENDING;
	event->sync = !!(flags & CNSS_EVENT_SYNC);

	spin_lock_irqsave(&plat_priv->event_lock, irq_flags);
	list_add_tail(&event->list, &plat_priv->event_list);
	spin_unlock_irqrestore(&plat_priv->event_lock, irq_flags);

	queue_work(plat_priv->event_wq, &plat_priv->event_work);

	if (!(flags & CNSS_EVENT_SYNC))
		goto out;

	printk(KERN_INFO "Waiting for Event(%s) to complete\n",
		cnss_driver_event_to_str(type));

	if (flags & CNSS_EVENT_UNINTERRUPTIBLE)
		wait_for_completion(&event->complete);
	else
		ret = wait_for_completion_interruptible(&event->complete);

	cnss_pr_dbg("Completed event: %s(%d), state: 0x%lx, ret: %d/%d\n",
		    cnss_driver_event_to_str(type), type,
		    plat_priv->driver_state, ret, event->ret);
	spin_lock_irqsave(&plat_priv->event_lock, irq_flags);
	if (ret == -ERESTARTSYS && event->ret == CNSS_EVENT_PENDING) {
		event->sync = false;
		spin_unlock_irqrestore(&plat_priv->event_lock, irq_flags);
		ret = -EINTR;
		goto out;
	}
	spin_unlock_irqrestore(&plat_priv->event_lock, irq_flags);

	ret = event->ret;
	kfree(event);

out:
#ifdef CONFIG_CNSS2_PM
	cnss_pm_relax(plat_priv);
#endif
	return ret;
}

unsigned int cnss_get_driver_mode(void)
{
	return driver_mode;
}
EXPORT_SYMBOL(cnss_get_driver_mode);

int cnss_set_driver_mode(unsigned int mode)
{
	switch (mode) {
	/* Fall through for all valid modes */
	case CNSS_MISSION:
	case CNSS_FTM:
	case CNSS_EPPING:
	case CNSS_WALTEST:
	case CNSS_OFF:
	case CNSS_CCPM:
	case CNSS_QVIT:
	case CNSS_CALIBRATION:
	case CNSS_FTM_CALIBRATION:
		driver_mode = mode;
		break;
	default:
		pr_err("%s: Invalid driver mode %d", __func__, mode);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(cnss_set_driver_mode);

unsigned int cnss_get_boot_timeout(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv) {
		cnss_pr_err("plat_priv is NULL\n");
		return 0;
	}

	return cnss_get_qmi_timeout(plat_priv);
}
EXPORT_SYMBOL(cnss_get_boot_timeout);

int cnss_power_up(struct device *dev)
{
	int ret = 0;
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	unsigned int timeout;

	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL\n");
		return -ENODEV;
	}

	cnss_pr_dbg("Powering up device\n");

	ret = cnss_driver_event_post(plat_priv,
				     CNSS_DRIVER_EVENT_POWER_UP,
				     CNSS_EVENT_SYNC, NULL);
	if (ret)
		goto out;

	if (plat_priv->device_id == QCA6174_DEVICE_ID)
		goto out;

	timeout = cnss_get_boot_timeout(dev);

	reinit_completion(&plat_priv->power_up_complete);
	ret = wait_for_completion_timeout(&plat_priv->power_up_complete,
					  msecs_to_jiffies(timeout) << 2);
	if (!ret) {
		cnss_pr_err("Timeout waiting for power up to complete\n");
		ret = -EAGAIN;
		goto out;
	}

	return 0;

out:
	return ret;
}
EXPORT_SYMBOL(cnss_power_up);

int cnss_power_down(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL\n");
		return -ENODEV;
	}

	cnss_pr_dbg("Powering down device\n");

	return cnss_driver_event_post(plat_priv,
				      CNSS_DRIVER_EVENT_POWER_DOWN,
				      CNSS_EVENT_SYNC, NULL);
}
EXPORT_SYMBOL(cnss_power_down);

int cnss_idle_restart(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	unsigned int timeout;
	int ret = 0;

	if (!plat_priv) {
		cnss_pr_err("plat_priv is NULL\n");
		return -ENODEV;
	}

	cnss_pr_dbg("Doing idle restart\n");

	ret = cnss_driver_event_post(plat_priv,
				     CNSS_DRIVER_EVENT_IDLE_RESTART,
				     CNSS_EVENT_SYNC_UNINTERRUPTIBLE, NULL);
	if (ret)
		goto out;

	if (plat_priv->device_id == QCA6174_DEVICE_ID) {
		ret = cnss_bus_call_driver_probe(plat_priv);
		goto out;
	}

	timeout = cnss_get_boot_timeout(dev);

	reinit_completion(&plat_priv->power_up_complete);
	ret = wait_for_completion_timeout(&plat_priv->power_up_complete,
					  msecs_to_jiffies(timeout) << 2);
	if (!ret) {
		cnss_pr_err("Timeout waiting for idle restart to complete\n");
		ret = -EAGAIN;
		goto out;
	}

	return 0;

out:
	return ret;
}
EXPORT_SYMBOL(cnss_idle_restart);

int cnss_idle_shutdown(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	int ret;

	if (!plat_priv) {
		cnss_pr_err("plat_priv is NULL\n");
		return -ENODEV;
	}

	if (test_bit(CNSS_IN_SUSPEND_RESUME, &plat_priv->driver_state)) {
		cnss_pr_dbg("System suspend or resume in progress, ignore idle shutdown\n");
		return -EAGAIN;
	}

	cnss_pr_dbg("Doing idle shutdown\n");

	if (!test_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state) &&
	    !test_bit(CNSS_DEV_ERR_NOTIFY, &plat_priv->driver_state))
		goto skip_wait;

	reinit_completion(&plat_priv->recovery_complete);
	ret = wait_for_completion_timeout(&plat_priv->recovery_complete,
					  RECOVERY_TIMEOUT);
	if (!ret) {
		cnss_pr_err("Timeout waiting for recovery to complete\n");
		CNSS_ASSERT(0);
	}

skip_wait:
	return cnss_driver_event_post(plat_priv,
				      CNSS_DRIVER_EVENT_IDLE_SHUTDOWN,
				      CNSS_EVENT_SYNC_UNINTERRUPTIBLE, NULL);
}
EXPORT_SYMBOL(cnss_idle_shutdown);

static int cnss_get_resources(struct cnss_plat_data *plat_priv)
{
#ifdef CNSS2_NOCLOCK
	int ret;

	ret = cnss_get_vreg_type(plat_priv, CNSS_VREG_PRIM);
	if (ret) {
		cnss_pr_err("Failed to get vreg, err = %d\n", ret);
		goto out;
	}

	ret = cnss_get_clk(plat_priv);
	if (ret) {
		cnss_pr_err("Failed to get clocks, err = %d\n", ret);
		goto put_vreg;
	}

	ret = cnss_get_pinctrl(plat_priv);
	if (ret) {
		cnss_pr_err("Failed to get pinctrl, err = %d\n", ret);
		goto put_clk;
	}

	return 0;

put_clk:
	cnss_put_clk(plat_priv);
put_vreg:
	cnss_put_vreg_type(plat_priv, CNSS_VREG_PRIM);
out:
	return ret;
#endif
	return 0;
}

static void cnss_put_resources(struct cnss_plat_data *plat_priv)
{
#ifdef CNSS2_NOCLOCK
	cnss_put_clk(plat_priv);
	cnss_put_vreg_type(plat_priv, CNSS_VREG_PRIM);
#endif
}
int cnss_unregister_qca8074_cb(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	if (plat_priv->modem_nb.notifier_call) {
		ret = rproc_unregister_subsys_notifier(
				plat_priv->subsys_info.subsys_desc.name,
				&plat_priv->modem_nb, &plat_priv->modem_atomic_nb);
		if (ret) {
			cnss_pr_err("%s: failed to unregister ret %d\n",__func__,ret);
			return ret;
		}
		memset(&plat_priv->modem_nb, 0, sizeof(struct notifier_block));
		memset(&plat_priv->modem_atomic_nb, 0, sizeof(struct notifier_block));
	}
	return 0;
}

int cnss_unregister_qcn9000_cb(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	if (plat_priv->modem_nb.notifier_call) {
		ret = rproc_unregister_subsys_notifier(
				plat_priv->subsys_info.subsys_desc.name,
				&plat_priv->modem_nb, &plat_priv->modem_atomic_nb);
		if (ret) {
			cnss_pr_err("%s: failed to unregister ret %d\n",__func__,ret);
			return ret;
		}
		memset(&plat_priv->modem_nb, 0, sizeof(struct notifier_block));
		memset(&plat_priv->modem_atomic_nb, 0, sizeof(struct notifier_block));
	}
	return 0;

}


static int cnss_qcn9000_notifier_atomic_nb(struct notifier_block *nb,
					   unsigned long code,
					   void *ss_handle)
{
	/* Fatal Notification to driver already sent as soon as
	 * MHI FATAL_ERR or SYS_ERR is received in cnss_mhi_notify_status
	 */
	return NOTIFY_OK;
}

static int cnss_qca8074_notifier_atomic_nb(struct notifier_block *nb,
	unsigned long code,
	void *ss_handle)
{
	struct cnss_plat_data *plat_priv =
		container_of(nb, struct cnss_plat_data, modem_atomic_nb);
	struct cnss_wlan_driver *driver_ops;

	driver_ops = plat_priv->driver_ops;

	if (code == SUBSYS_PREPARE_FOR_FATAL_SHUTDOWN)
		cnss_pr_err("XXX TARGET ASSERTED XXX\n");
		cnss_pr_err("XXX TARGET %s instance_id 0x%x plat_env idx %d XXX\n",
			    plat_priv->device_name,
			    plat_priv->wlfw_service_instance_id,
			    cnss_get_plat_env_index_from_plat_priv(plat_priv));
		plat_priv->target_asserted = 1;
		plat_priv->target_assert_timestamp = ktime_to_ms(ktime_get());
		driver_ops->fatal((struct pci_dev *)plat_priv->plat_dev,
				  (const struct pci_device_id *)
				  plat_priv->plat_dev_id);

	return NOTIFY_OK;
}

static int cnss_qcn9000_notifier_nb(struct notifier_block *nb,
				    unsigned long code,
				    void *ss_handle)
{
	struct cnss_plat_data *plat_priv =
		container_of(nb, struct cnss_plat_data, modem_nb);
	struct cnss_wlan_driver *driver_ops;

	driver_ops = plat_priv->driver_ops;

	if (code == SUBSYS_AFTER_POWERUP) {
		driver_ops->probe((struct pci_dev *)plat_priv->plat_dev,
				  (const struct pci_device_id *)
				  plat_priv->plat_dev_id);
		clear_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state);
		clear_bit(CNSS_DRIVER_LOADING, &plat_priv->driver_state);
		set_bit(CNSS_DRIVER_PROBED, &plat_priv->driver_state);
	} else if (code == SUBSYS_BEFORE_SHUTDOWN) {
		driver_ops->remove((struct pci_dev *)plat_priv->plat_dev);
		clear_bit(CNSS_DRIVER_PROBED, &plat_priv->driver_state);
		clear_bit(CNSS_DEV_ERR_NOTIFY, &plat_priv->driver_state);
	} else if (code == SUBSYS_RAMDUMP_NOTIFICATION) {
		coresight_abort();
		driver_ops->reinit((struct pci_dev *)plat_priv->plat_dev,
				   (const struct pci_device_id *)
				   plat_priv->plat_dev_id);
		clear_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state);
		return NOTIFY_DONE;
	} else {
		driver_ops->update_status((struct pci_dev *)plat_priv->plat_dev,
					  (const struct pci_device_id *)
					  plat_priv->plat_dev_id, code);
	}

	return NOTIFY_OK;
}

void *cnss_register_qcn9000_cb(struct cnss_plat_data *plat_priv)
{
	struct cnss_subsys_info *subsys_info;
	void *ss_handle = NULL;
	int ret = 0;

	subsys_info = &plat_priv->subsys_info;
	subsys_info->subsys_desc.name = plat_priv->device_name;

	plat_priv->modem_nb.notifier_call = cnss_qcn9000_notifier_nb;
	plat_priv->modem_atomic_nb.notifier_call =
				cnss_qcn9000_notifier_atomic_nb;
	ret = rproc_register_subsys_notifier(subsys_info->subsys_desc.name,
			&plat_priv->modem_nb, &plat_priv->modem_atomic_nb);
	if (ret) {
		cnss_pr_err("%s: failed to register rproc ret %d\n", __func__, ret);
		return NULL;
	}
	ss_handle = subsys_info;

	return ss_handle;

}

static int cnss_qca8074_notifier_nb(struct notifier_block *nb,
				  unsigned long code,
				  void *ss_handle)
{
	struct cnss_plat_data *plat_priv =
		container_of(nb, struct cnss_plat_data, modem_nb);
	struct cnss_wlan_driver *driver_ops;

	driver_ops = plat_priv->driver_ops;

	if (code == SUBSYS_AFTER_POWERUP) {
		driver_ops->probe((struct pci_dev *)plat_priv->plat_dev,
				  (const struct pci_device_id *)
				  plat_priv->plat_dev_id);
	} else if (code == SUBSYS_BEFORE_SHUTDOWN) {
		cnss_bus_free_fw_mem(plat_priv);
		cnss_bus_free_qdss_mem(plat_priv);
		driver_ops->remove((struct pci_dev *)plat_priv->plat_dev);
	} else if (code == SUBSYS_RAMDUMP_NOTIFICATION) {
		coresight_abort();
		driver_ops->reinit((struct pci_dev *)plat_priv->plat_dev,
				   (const struct pci_device_id *)
				   plat_priv->plat_dev_id);
		return NOTIFY_DONE;
	} else {
		if (code == SUBSYS_AFTER_SHUTDOWN) {
			clear_bit(CNSS_FW_READY, &plat_priv->driver_state);
			clear_bit(CNSS_FW_MEM_READY, &plat_priv->driver_state);
		}
		driver_ops->update_status((struct pci_dev *)plat_priv->plat_dev,
					  (const struct pci_device_id *)
					  plat_priv->plat_dev_id, code);
		cnss_free_soc_info(plat_priv);
	}

	return NOTIFY_OK;
}

void *cnss_register_qca8074_cb(struct cnss_plat_data *plat_priv)
{
	struct cnss_subsys_info *subsys_info;
	void *ss_handle = NULL;
	int ret = 0;

	subsys_info = &plat_priv->subsys_info;
	plat_priv->modem_nb.notifier_call = cnss_qca8074_notifier_nb;
	plat_priv->modem_atomic_nb.notifier_call =
					cnss_qca8074_notifier_atomic_nb;
	ret = rproc_register_subsys_notifier(subsys_info->subsys_desc.name,
			&plat_priv->modem_nb, &plat_priv->modem_atomic_nb);
	if (ret) {
		cnss_pr_err("%s: failed to register rproc ret %d\n", __func__, ret);
		return NULL;
	}

	ss_handle = subsys_info;
	return ss_handle;
}

void *cnss_register_notifier_cb(struct cnss_plat_data *plat_priv)
{
	switch (plat_priv->device_id) {
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
		return cnss_register_qcn9000_cb(plat_priv);
	case QCA8074_DEVICE_ID:
	case QCA8074V2_DEVICE_ID:
	case QCA6018_DEVICE_ID:
	case QCA5018_DEVICE_ID:
	case QCN6122_DEVICE_ID:
	case QCA9574_DEVICE_ID:
		return cnss_register_qca8074_cb(plat_priv);
	default:
		cnss_pr_err("Invalid device ID: 0x%lx", plat_priv->device_id);
	}
	return NULL;
}

int cnss_unregister_notifier_cb(struct cnss_plat_data *plat_priv)
{
	switch (plat_priv->device_id) {
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
		return cnss_unregister_qcn9000_cb(plat_priv);
	case QCA8074_DEVICE_ID:
	case QCA8074V2_DEVICE_ID:
	case QCA6018_DEVICE_ID:
	case QCA5018_DEVICE_ID:
	case QCN6122_DEVICE_ID:
	case QCA9574_DEVICE_ID:
		return cnss_unregister_qca8074_cb(plat_priv);
	default:
		cnss_pr_err("Invalid device ID: 0x%lx", plat_priv->device_id);
	}
	return 0;
}

int cnss_wlan_register_driver(struct cnss_wlan_driver *driver_ops)
{
	int ret, i;
	struct cnss_plat_data *plat_priv;

	for (i = 0; i < plat_env_index; i++) {
		plat_priv = plat_env[i];

		if (!plat_priv)
			continue;

		if (!plat_priv->cold_boot_support &&
		    (driver_mode == CNSS_CALIBRATION ||
		     driver_mode == CNSS_FTM_CALIBRATION)) {
			cnss_pr_info("Skipping driver register for device 0x%lx for mode %d\n",
				     plat_priv->device_id, driver_mode);
			continue;
		}

		plat_priv->target_asserted = 0;
		plat_priv->target_assert_timestamp = 0;
		plat_priv->driver_status = CNSS_LOAD_UNLOAD;

		if ((plat_priv->device_id == QCA8074_DEVICE_ID ||
		     plat_priv->device_id == QCA8074V2_DEVICE_ID ||
		     plat_priv->device_id == QCA5018_DEVICE_ID ||
		     plat_priv->device_id == QCN6122_DEVICE_ID ||
		     plat_priv->device_id == QCA6018_DEVICE_ID ||
		     plat_priv->device_id == QCA9574_DEVICE_ID) &&
		    (strcmp(driver_ops->name, "pld_ahb") == 0)) {
			plat_priv->driver_ops = driver_ops;
			ret = cnss_register_subsys(plat_priv);
			if (ret)
				goto reset_ctx;
		}
		/*
		 * Skip cnss_wlan_driver_register for PCI if pci device is not
		 * connected to board. If PCI device is connected, probe_basic
		 * function will initialize plat_priv->pci_dev
		 */
		if ((plat_priv->device_id == QCN9000_DEVICE_ID ||
		     plat_priv->device_id == QCN9224_DEVICE_ID) &&
		    plat_priv->pci_dev &&
		    (strcmp(driver_ops->name, "pld_pcie") == 0)) {
			cnss_pci_init(plat_priv);
			plat_priv->driver_ops = driver_ops;
			set_bit(CNSS_DRIVER_LOADING, &plat_priv->driver_state);
			ret = cnss_register_subsys(plat_priv);
			if (ret)
				goto reset_ctx;
		}
		plat_priv->driver_status = CNSS_INITIALIZED;
	}

	return 0;

reset_ctx:
	cnss_pr_err("Failed to get subsystem, err = %d\n", ret);
	plat_priv->driver_status = CNSS_UNINITIALIZED;
	plat_priv->driver_ops = NULL;
	return ret;
}
EXPORT_SYMBOL(cnss_wlan_register_driver);

bool cnss_is_dev_initialized(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return NULL;

	return (plat_priv->driver_status == CNSS_INITIALIZED) ? 1 : 0;
}
EXPORT_SYMBOL(cnss_is_dev_initialized);

void *cnss_get_pci_dev_from_plat_dev(void *pdev)
{
	struct platform_device *plat_dev = (struct platform_device *)pdev;
	struct cnss_plat_data *plat_priv;

	if (!plat_dev)
		return NULL;

	plat_priv = platform_get_drvdata(plat_dev);

	if (!plat_priv)
		return NULL;

	return plat_priv->pci_dev;
}
EXPORT_SYMBOL(cnss_get_pci_dev_from_plat_dev);

void *cnss_get_pci_dev_id_from_plat_dev(void *pdev)
{
	struct platform_device *plat_dev = (struct platform_device *)pdev;
	struct cnss_plat_data *plat_priv;

	if (!plat_dev)
		return NULL;

	plat_priv = platform_get_drvdata(plat_dev);

	if (!plat_priv)
		return NULL;

	return plat_priv->pci_dev_id;
}
EXPORT_SYMBOL(cnss_get_pci_dev_id_from_plat_dev);

void cnss_wlan_unregister_driver(struct cnss_wlan_driver *driver_ops)
{
	struct cnss_plat_data *plat_priv;
	struct cnss_subsys_info *subsys_info;
	struct cnss_wlan_driver *ops;
	int i;

	for (i = 0; i < plat_env_index; i++) {
		plat_priv = plat_env[i];

		if (!plat_priv) {
			printk(KERN_ERR "%s plat_priv is NULL!\n", __func__);
			return;
		}

		if (!plat_priv->cold_boot_support &&
		    (driver_mode == CNSS_CALIBRATION ||
		     driver_mode == CNSS_FTM_CALIBRATION)) {
			cnss_pr_info("Skipping driver unregister for device 0x%lx for mode %d\n",
				     plat_priv->device_id, driver_mode);
			continue;
		}

		plat_priv->driver_status = CNSS_LOAD_UNLOAD;
		ops = plat_priv->driver_ops;

		if ((plat_priv->device_id == QCA8074_DEVICE_ID ||
		     plat_priv->device_id == QCA8074V2_DEVICE_ID ||
		     plat_priv->device_id == QCA5018_DEVICE_ID ||
		     plat_priv->device_id == QCN6122_DEVICE_ID ||
		     plat_priv->device_id == QCA9574_DEVICE_ID ||
		     plat_priv->device_id == QCA6018_DEVICE_ID) && ops &&
			(strcmp(driver_ops->name, "pld_ahb") == 0)) {
			subsys_info = &plat_priv->subsys_info;
			if (subsys_info->subsys_handle &&
			    !subsys_info->subsystem_put_in_progress) {
				subsys_info->subsystem_put_in_progress = true;
				rproc_shutdown(subsys_info->subsys_handle);
				subsys_info->subsystem_put_in_progress = false;
			} else {
				ops->remove((struct pci_dev *)
					    plat_priv->plat_dev);
			}

			cnss_unregister_notifier_cb(plat_priv);
			subsys_info->subsys_handle = NULL;
			plat_priv->driver_ops = NULL;
			plat_priv->driver_status = CNSS_UNINITIALIZED;
			plat_priv->driver_state = 0;
		}

		if ((plat_priv->device_id == QCN9000_DEVICE_ID ||
		     plat_priv->device_id == QCN9224_DEVICE_ID) && ops &&
		    (strcmp(driver_ops->name, "pld_pcie") == 0)) {
			set_bit(CNSS_DRIVER_UNLOADING, &plat_priv->driver_state);
			subsys_info = &plat_priv->subsys_info;
			if (subsys_info->subsys_handle &&
			    !subsys_info->subsystem_put_in_progress) {
				subsys_info->subsystem_put_in_progress = true;
				rproc_shutdown(subsys_info->subsys_handle);
				subsys_info->subsys_handle = NULL;
				subsys_info->subsystem_put_in_progress = false;
			} else {
				if (plat_priv->pci_dev)
					ops->remove((struct pci_dev *)plat_priv->plat_dev);
			}
			cnss_unregister_subsys(plat_priv);
			cnss_unregister_notifier_cb(plat_priv);
			plat_priv->driver_ops = NULL;
			plat_priv->driver_status = CNSS_UNINITIALIZED;
			plat_priv->driver_state = 0;
		}
	}
}
EXPORT_SYMBOL(cnss_wlan_unregister_driver);

void  *cnss_subsystem_get(struct device *dev, int device_id)
{
	struct cnss_plat_data *plat_priv;
	struct cnss_subsys_info *subsys_info;
	struct pci_dev *pcidev;

	if (cnss_get_bus_type(device_id) == CNSS_BUS_AHB) {
		plat_priv = cnss_bus_dev_to_plat_priv(dev);
	} else {
		pcidev = container_of(dev, struct pci_dev, dev);
		plat_priv = cnss_get_plat_priv_dev_by_pci_dev(pcidev);
	}

	if (!plat_priv) {
		return NULL;
	}

	plat_priv->target_asserted = 0;
	plat_priv->target_assert_timestamp = 0;
	subsys_info = &plat_priv->subsys_info;

	if (subsys_info->subsys_handle) {
		cnss_pr_err("%s: error: subsys handle %p is not NULL ",
			    __func__, subsys_info->subsys_handle);
		return NULL;
	}

	if (!plat_priv->rproc_handle) {
		cnss_pr_err("%s: rproc_handle is NULL for %s\n",
			    __func__, plat_priv->device_name);
		return NULL;
	}

	subsys_info->subsys_handle = plat_priv->rproc_handle;
	if (rproc_boot(subsys_info->subsys_handle)) {
		cnss_pr_err("%s: error: rproc_boot failed for %s\n",
			    __func__, plat_priv->device_name);
		CNSS_ASSERT(0);
		return NULL;
	}
	return subsys_info->subsys_handle;
}
EXPORT_SYMBOL(cnss_subsystem_get);

void cnss_subsystem_put(struct device *dev)
{
	struct cnss_plat_data *plat_priv;
	struct cnss_subsys_info *subsys_info;

	plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv)
		return;

	subsys_info = &plat_priv->subsys_info;

	if (!subsys_info->subsys_handle) {
		cnss_pr_err("%s: error: subsys handle is NULL", __func__);
		return;
	}
	set_bit(CNSS_DRIVER_UNLOADING, &plat_priv->driver_state);

	if (!subsys_info->subsystem_put_in_progress) {
		subsys_info->subsystem_put_in_progress = true;
		rproc_shutdown(subsys_info->subsys_handle);
		subsys_info->subsystem_put_in_progress = false;
		subsys_info->subsys_handle = NULL;
		plat_priv->driver_state = 0;
	}
}
EXPORT_SYMBOL(cnss_subsystem_put);

#ifdef CONFIG_CNSS2_PM
static int cnss_modem_notifier_nb(struct notifier_block *nb,
				  unsigned long code,
				  void *ss_handle)
{
	struct cnss_plat_data *plat_priv =
		container_of(nb, struct cnss_plat_data, modem_nb);
	struct cnss_esoc_info *esoc_info;

	cnss_pr_dbg("Modem notifier: event %lu\n", code);

	if (!plat_priv)
		return NOTIFY_DONE;

	esoc_info = &plat_priv->esoc_info;

	if (code == SUBSYS_AFTER_POWERUP)
		esoc_info->modem_current_status = 1;
	else if (code == SUBSYS_BEFORE_SHUTDOWN)
		esoc_info->modem_current_status = 0;
	else
		return NOTIFY_DONE;

	if (!cnss_bus_call_driver_modem_status(plat_priv,
					       esoc_info->modem_current_status))
		return NOTIFY_DONE;

	return NOTIFY_OK;
}

static int cnss_register_esoc(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct device *dev;
	struct cnss_esoc_info *esoc_info;
	struct esoc_desc *esoc_desc;
	const char *client_desc;

	dev = &plat_priv->plat_dev->dev;
	esoc_info = &plat_priv->esoc_info;

	esoc_info->notify_modem_status =
		of_property_read_bool(dev->of_node,
				      "qcom,notify-modem-status");

	if (!esoc_info->notify_modem_status)
		goto out;

	ret = of_property_read_string_index(dev->of_node, "esoc-names", 0,
					    &client_desc);
	if (ret) {
		cnss_pr_dbg("esoc-names is not defined in DT, skip!\n");
	} else {
		esoc_desc = devm_register_esoc_client(dev, client_desc);
		if (IS_ERR_OR_NULL(esoc_desc)) {
			ret = PTR_RET(esoc_desc);
			cnss_pr_err("Failed to register esoc_desc, err = %d\n",
				    ret);
			goto out;
		}
		esoc_info->esoc_desc = esoc_desc;
	}

	plat_priv->modem_nb.notifier_call = cnss_modem_notifier_nb;
	esoc_info->modem_current_status = 0;
	ret = rproc_register_subsys_notifier(esoc_info->esoc_desc ?
					 esoc_info->esoc_desc->name :
					 "modem", &plat_priv->modem_nb, NULL);
	if (ret) {
		cnss_pr_err("%s: Failed register rproc. ret %d\n",__func__, ret);
		return ret
	}
	return 0;
unreg_esoc:
	if (esoc_info->esoc_desc)
		devm_unregister_esoc_client(dev, esoc_info->esoc_desc);
out:
	return ret;
}

static void cnss_unregister_esoc(struct cnss_plat_data *plat_priv)
{
	struct cnss_esoc_info *esoc_info;
	int ret = 0;

	esoc_info = &plat_priv->esoc_info;
	ret = rproc_unregister_subsys_notifier("modem", &plat_priv->modem_nb, NULL);
	if (ret) {
		cnss_pr_err("%s: Failed to unregister rproc. ret %d\n",__func__, ret);
		return;
	}
}
#endif

static int cnss_subsys_powerup(struct rproc *subsys_desc)
{
	int ret = 0;
	struct cnss_plat_data *plat_priv;
	plat_priv = dev_get_drvdata(subsys_desc->dev.parent);
	if (!plat_priv)
		return -ENODEV;

	plat_priv->target_asserted = 0;
	plat_priv->target_assert_timestamp = 0;
	set_bit(CNSS_DRIVER_LOADING, &plat_priv->driver_state);
	ret = cnss_pci_probe(plat_priv->pci_dev,
			     plat_priv->pci_dev_id,
			     plat_priv);
	if (ret) {
		pr_err("ERROR : %s:%d ret %d\n", __func__, __LINE__, ret);
		return -ENODEV;
	}

	if (!plat_priv->driver_state) {
		cnss_pr_dbg("Powerup is ignored\n");
		return 0;
	}

	return cnss_bus_dev_powerup(plat_priv);
}

static int cnss_subsys_shutdown(struct rproc *subsys_desc)
{
	struct cnss_plat_data *plat_priv;
	plat_priv = dev_get_drvdata(subsys_desc->dev.parent);
	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL!\n");
		return -ENODEV;
	}

	if (!plat_priv->driver_state) {
		cnss_pr_dbg("shutdown is ignored\n");
		return 0;
	}

	return cnss_bus_dev_shutdown(plat_priv);
}

static int cnss_subsys_dummy_load(struct rproc *subsys_desc, const struct firmware *fw)
{
	/*no firmware load it will be taken care by pci and mhi*/
	return 0;
}

void cnss_device_crashed(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	struct cnss_subsys_info *subsys_info;

	if (!plat_priv)
		return;

	subsys_info = &plat_priv->subsys_info;
	if (subsys_info->subsys_handle) {
		set_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state);
		rproc_report_crash(subsys_info->subsys_handle,
						RPROC_FATAL_ERROR);
	}
}
EXPORT_SYMBOL(cnss_device_crashed);

static void cnss_subsys_crash_shutdown(struct rproc *subsys_desc)
{
	struct cnss_plat_data *plat_priv = dev_get_drvdata(subsys_desc->dev.parent);

	if (!plat_priv) {
		cnss_pr_err("plat_priv is NULL\n");
		return;
	}

	cnss_bus_dev_crash_shutdown(plat_priv);
}

static void cnss_subsys_ramdump(struct rproc *subsys_desc, struct rproc_dump_segment *segment,
					void  *dest)
{
	struct cnss_plat_data *plat_priv = dev_get_drvdata(subsys_desc->dev.parent);

	if (!plat_priv) {
		cnss_pr_err("plat_priv is NULL\n");
		return;
	}

	cnss_bus_dev_ramdump(plat_priv);
}

static int cnss_subsys_add_ramdump_callback(struct rproc *subsys_desc,
		const struct firmware *firmware)
{
	struct cnss_plat_data *plat_priv = dev_get_drvdata(subsys_desc->dev.parent);
	int ret = 0;
	if (!plat_priv) {
		cnss_pr_err("%s: plat_priv is NULL\n",__func__);
		return -1;
	}
	ret = rproc_coredump_add_custom_segment(subsys_desc, 0, 0,
						cnss_subsys_ramdump, NULL);
        if (ret) {
               cnss_pr_err("%s: Failed to add custom segment ret %d\n",__func__, ret);
               return ret;
        }
        return ret;
}

void *cnss_get_virt_ramdump_mem(struct device *dev, unsigned long *size)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	struct cnss_ramdump_info *ramdump_info;

	if (!plat_priv)
		return NULL;

	ramdump_info = &plat_priv->ramdump_info;
	*size = ramdump_info->ramdump_size;

	return ramdump_info->ramdump_va;
}
EXPORT_SYMBOL(cnss_get_virt_ramdump_mem);

static const char *cnss_recovery_reason_to_str(enum cnss_recovery_reason reason)
{
	switch (reason) {
	case CNSS_REASON_DEFAULT:
		return "DEFAULT";
	case CNSS_REASON_LINK_DOWN:
		return "LINK_DOWN";
	case CNSS_REASON_RDDM:
		return "RDDM";
	case CNSS_REASON_TIMEOUT:
		return "TIMEOUT";
	}

	return "UNKNOWN";
};

static int cnss_do_recovery(struct cnss_plat_data *plat_priv,
			    enum cnss_recovery_reason reason)
{
	struct cnss_subsys_info *subsys_info =
		&plat_priv->subsys_info;

	plat_priv->recovery_count++;

	if (plat_priv->device_id == QCA6174_DEVICE_ID)
		goto self_recovery;

	if (test_bit(SKIP_RECOVERY, &plat_priv->ctrl_params.quirks)) {
		cnss_pr_dbg("Skip device recovery\n");
		return 0;
	}

	switch (reason) {
	case CNSS_REASON_LINK_DOWN:
		if (test_bit(LINK_DOWN_SELF_RECOVERY,
			     &plat_priv->ctrl_params.quirks))
			goto self_recovery;
		break;
	case CNSS_REASON_RDDM:
		cnss_bus_collect_dump_info(plat_priv, false);
		break;
	case CNSS_REASON_DEFAULT:
	case CNSS_REASON_TIMEOUT:
		break;
	default:
		cnss_pr_err("Unsupported recovery reason: %s(%d)\n",
			    cnss_recovery_reason_to_str(reason), reason);
		break;
	}

	/* If recovery is enabled, fatal notification is sent to wifi driver
	 * as soon as MHI notifies error.
	 * If recovery is disabled, send fatal notification here after RDDM
	 * is done so that we have the RDDM information before the assert
	 * is triggered from the wifi driver.
	 */
	if (!plat_priv->recovery_enabled) {
		/* This is a special case where ramdump file is uploaded to
		 * TFTP and then host assert happens. It is disabled by default.
		 */
		if (ramdump_enabled)
			cnss_bus_dev_ramdump(plat_priv);

		cnss_pci_update_status(plat_priv->bus_priv, CNSS_FW_DOWN);
	}

	if (!subsys_info->subsys_handle)
		return 0;
	rproc_report_crash(subsys_info->subsys_handle, RPROC_FATAL_ERROR);
	return 0;

self_recovery:
	cnss_bus_dev_shutdown(plat_priv);
	cnss_bus_dev_powerup(plat_priv);

	return 0;
}

static int cnss_driver_recovery_hdlr(struct cnss_plat_data *plat_priv,
				     void *data)
{
	struct cnss_recovery_data *recovery_data = data;
	int ret = 0;

	cnss_pr_dbg("Driver recovery is triggered with reason: %s(%d)\n",
		    cnss_recovery_reason_to_str(recovery_data->reason),
		    recovery_data->reason);

	if (!plat_priv->driver_state) {
		cnss_pr_err("Improper driver state, ignore recovery\n");
		ret = -EINVAL;
		goto out;
	}

	if (test_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state)) {
		cnss_pr_err("Recovery is already in progress, state 0x%lx\n",
			    plat_priv->driver_state);
		ret = -EINVAL;
		goto out;
	}

	if (test_bit(CNSS_DRIVER_UNLOADING, &plat_priv->driver_state) ||
	    test_bit(CNSS_DRIVER_IDLE_SHUTDOWN, &plat_priv->driver_state)) {
		cnss_pr_err("Driver unload or idle shutdown is in progress, ignore recovery\n");
		CNSS_ASSERT(0);
		ret = -EINVAL;
		goto out;
	}

	switch (plat_priv->device_id) {
	case QCA6174_DEVICE_ID:
		if (test_bit(CNSS_DRIVER_LOADING, &plat_priv->driver_state) ||
		    test_bit(CNSS_DRIVER_IDLE_RESTART,
			     &plat_priv->driver_state)) {
			cnss_pr_err("Driver load or idle restart is in progress, ignore recovery\n");
			ret = -EINVAL;
			goto out;
		}
		break;
	default:
		if (!test_bit(CNSS_FW_READY, &plat_priv->driver_state)) {
			set_bit(CNSS_FW_BOOT_RECOVERY,
				&plat_priv->driver_state);
		}
		break;
	}

	set_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state);
	ret = cnss_do_recovery(plat_priv, recovery_data->reason);

out:
	kfree(data);
	return ret;
}

int cnss_self_recovery(struct device *dev,
		       enum cnss_recovery_reason reason)
{
	cnss_schedule_recovery(dev, reason);
	return 0;
}
EXPORT_SYMBOL(cnss_self_recovery);

void cnss_schedule_recovery(struct device *dev,
			    enum cnss_recovery_reason reason)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	struct cnss_recovery_data *data;
	int gfp = GFP_KERNEL;

	if (test_bit(CNSS_DRIVER_UNLOADING, &plat_priv->driver_state) ||
	    test_bit(CNSS_DRIVER_IDLE_SHUTDOWN, &plat_priv->driver_state)) {
		cnss_pr_dbg("Driver unload or idle shutdown is in progress, ignore schedule recovery\n");
		CNSS_ASSERT(0);
		return;
	}

	if (in_interrupt() || irqs_disabled())
		gfp = GFP_ATOMIC;

	data = kzalloc(sizeof(*data), gfp);
	if (!data)
		return;

	data->reason = reason;
	cnss_driver_event_post(plat_priv,
			       CNSS_DRIVER_EVENT_RECOVERY,
			       0, data);
}
EXPORT_SYMBOL(cnss_schedule_recovery);

int cnss_force_fw_assert(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);

	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL\n");
		return -ENODEV;
	}

	if (plat_priv->device_id == QCA6174_DEVICE_ID) {
		cnss_pr_info("Forced FW assert is not supported\n");
		return -EOPNOTSUPP;
	}

	if (cnss_pci_is_device_down(dev)) {
		cnss_pr_info("Device is already in bad state, ignore force assert\n");
		return 0;
	}

	if (test_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state)) {
		cnss_pr_info("Recovery is already in progress, ignore forced FW assert\n");
		return 0;
	}

	if (in_interrupt() || irqs_disabled())
		cnss_driver_event_post(plat_priv,
				       CNSS_DRIVER_EVENT_FORCE_FW_ASSERT,
				       0, NULL);
	else
		cnss_bus_force_fw_assert_hdlr(plat_priv);

	return 0;
}
EXPORT_SYMBOL(cnss_force_fw_assert);

int cnss_force_collect_rddm(struct device *dev)
{
	struct cnss_plat_data *plat_priv = cnss_bus_dev_to_plat_priv(dev);
	int ret = 0;

	if (!plat_priv) {
		cnss_pr_err("plat_priv is NULL\n");
		return -ENODEV;
	}

	if (plat_priv->device_id == QCA6174_DEVICE_ID) {
		cnss_pr_info("Force collect rddm is not supported\n");
		return -EOPNOTSUPP;
	}

	if (cnss_pci_is_device_down(dev)) {
		cnss_pr_info("Device is already in bad state, ignore force collect rddm\n");
		return 0;
	}

	if (test_bit(CNSS_DRIVER_RECOVERY, &plat_priv->driver_state)) {
		cnss_pr_info("Recovery is already in progress, ignore forced collect rddm\n");
		return 0;
	}

	if (test_bit(CNSS_DRIVER_LOADING, &plat_priv->driver_state) ||
	    test_bit(CNSS_DRIVER_UNLOADING, &plat_priv->driver_state) ||
	    test_bit(CNSS_DRIVER_IDLE_RESTART, &plat_priv->driver_state) ||
	    test_bit(CNSS_DRIVER_IDLE_SHUTDOWN, &plat_priv->driver_state)) {
		cnss_pr_info("Loading/Unloading/idle restart/shutdown is in progress, ignore forced collect rddm\n");
		return 0;
	}

	ret = cnss_bus_force_fw_assert_hdlr(plat_priv);
	if (ret)
		return ret;

	reinit_completion(&plat_priv->rddm_complete);
	ret = wait_for_completion_timeout
		(&plat_priv->rddm_complete,
		 msecs_to_jiffies(CNSS_RDDM_TIMEOUT_MS));
	if (!ret)
		ret = -ETIMEDOUT;

	return ret;
}
EXPORT_SYMBOL(cnss_force_collect_rddm);

static int cnss_cold_boot_cal_start_hdlr(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	if (test_bit(CNSS_FW_READY, &plat_priv->driver_state) ||
	    test_bit(CNSS_DRIVER_LOADING, &plat_priv->driver_state) ||
	    test_bit(CNSS_DRIVER_PROBED, &plat_priv->driver_state)) {
		cnss_pr_dbg("Device is already active, ignore calibration\n");
		goto out;
	}

	set_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state);
	reinit_completion(&plat_priv->cal_complete);
	ret = cnss_bus_dev_powerup(plat_priv);
	if (ret) {
		complete(&plat_priv->cal_complete);
		clear_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state);
	}

out:
	return ret;
}

static int cnss_cold_boot_cal_done_hdlr(struct cnss_plat_data *plat_priv,
					void *data)
{
	struct cnss_cal_info *cal_info = data;

	if (!test_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state))
		goto out;

	switch (cal_info->cal_status) {
	case CNSS_CAL_DONE:
		cnss_pr_info("Coldboot Calibration completed successfully for device 0x%lx\n",
			     plat_priv->device_id);
		plat_priv->cal_done = true;
		break;
	case CNSS_CAL_TIMEOUT:
		cnss_pr_dbg("Calibration timed out, force shutdown\n");
		break;
	default:
		cnss_pr_err("Unknown calibration status: %u\n",
			    cal_info->cal_status);
		break;
	}

	cnss_release_antenna_sharing(plat_priv);
	complete(&plat_priv->cal_complete);
	clear_bit(CNSS_COLD_BOOT_CAL, &plat_priv->driver_state);

out:
	kfree(data);
	return 0;
}

static int cnss_power_up_hdlr(struct cnss_plat_data *plat_priv)
{
	int ret;

	ret = cnss_bus_dev_powerup(plat_priv);
	if (ret)
		clear_bit(CNSS_DRIVER_IDLE_RESTART, &plat_priv->driver_state);

	return ret;
}

static int cnss_power_down_hdlr(struct cnss_plat_data *plat_priv)
{
	cnss_bus_dev_shutdown(plat_priv);

	return 0;
}

static int cnss_qdss_trace_req_mem_hdlr(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	ret = cnss_bus_alloc_qdss_mem(plat_priv);
	if (ret < 0)
		return ret;

	return cnss_wlfw_qdss_trace_mem_info_send_sync(plat_priv);
}

void *cnss_qdss_trace_pa_to_va(struct cnss_plat_data *plat_priv,
			       u64 pa, u32 size, int *seg_id)
{
	int i = 0;
	struct cnss_fw_mem *qdss_mem = plat_priv->qdss_mem;
	u64 offset = 0;
	void *va = NULL;
	u64 local_pa;
	u32 local_size;

	for (i = 0; i < plat_priv->qdss_mem_seg_len; i++) {
		local_pa = (u64)qdss_mem[i].pa;
		local_size = (u32)qdss_mem[i].size;
		if (pa == local_pa && size <= local_size) {
			va = qdss_mem[i].va;
			break;
		}
		if (pa > local_pa &&
		    pa < local_pa + local_size &&
		    pa + size <= local_pa + local_size) {
			offset = pa - local_pa;
			va = qdss_mem[i].va + offset;
			break;
		}
	}

	*seg_id = i;
	return va;
}

static int cnss_qdss_trace_save_hdlr(struct cnss_plat_data *plat_priv,
				     void *data)
{
	struct cnss_qmi_event_qdss_trace_save_data *event_data = data;
	struct cnss_fw_mem *qdss_mem = plat_priv->qdss_mem;
	int ret = 0;
	int i;
	char *file_suffix = NULL;
	char file_prefix[QDSS_TRACE_FILE_NAME_MAX] = {};
	char file_name[CNSS_GENL_STR_LEN_MAX];

	if (!plat_priv->qdss_mem_seg_len) {
		cnss_pr_err("Memory for QDSS trace is not available\n");
		return -ENOMEM;
	}

	file_suffix = strnstr(event_data->file_name, ".bin",
			      QDSS_TRACE_FILE_NAME_MAX);

	if (file_suffix) {
		strlcpy(file_prefix, event_data->file_name,
			(file_suffix - &event_data->file_name[0]) + 1);

		if (plat_priv->device_id == QCN6122_DEVICE_ID)
			snprintf(file_name, sizeof(file_name),
				 "%s_qcn6122_%d%s", file_prefix,
				 (plat_priv->userpd_id - QCN6122_0),
				 file_suffix);

		if (plat_priv->device_id == QCN9000_DEVICE_ID)
			snprintf(file_name, sizeof(file_name),
				 "%s_qcn9000_pci%d%s", file_prefix,
				 (plat_priv->wlfw_service_instance_id -
				 QCN9000_NODE_ID_BASE), file_suffix);

		if (plat_priv->device_id == QCN9224_DEVICE_ID)
			snprintf(file_name, sizeof(file_name),
				 "%s_qcn9224_pci%d%s", file_prefix,
				 (plat_priv->wlfw_service_instance_id -
				 QCN9224_NODE_ID_BASE), file_suffix);
	} else {
		if (plat_priv->device_id == QCN6122_DEVICE_ID)
			snprintf(file_name, sizeof(file_name),
				 "%s_qcn6122_%d",
				 event_data->file_name,
				 (plat_priv->userpd_id - QCN6122_0));

		if (plat_priv->device_id == QCN9000_DEVICE_ID)
			snprintf(file_name, sizeof(file_name),
				 "%s_qcn9000_pci%d",
				 event_data->file_name,
				 (plat_priv->wlfw_service_instance_id -
				 QCN9000_NODE_ID_BASE));

		if (plat_priv->device_id == QCN9224_DEVICE_ID)
			snprintf(file_name, sizeof(file_name),
				 "%s_qcn9224_pci%d",
				 event_data->file_name,
				 (plat_priv->wlfw_service_instance_id -
				 QCN9224_NODE_ID_BASE));
	}

	if (event_data->mem_seg_len == 0) {
		for (i = 0; i < plat_priv->qdss_mem_seg_len; i++) {
			ret = cnss_genl_send_msg(qdss_mem[i].va,
						 CNSS_GENL_MSG_TYPE_QDSS,
						 file_name,
						 qdss_mem[i].size);
			if (ret < 0) {
				cnss_pr_err("Fail to save QDSS data: %d\n",
					    ret);
				break;
			}
		}
	} else if ((event_data->mem_seg_len == 1) &&
		   (event_data->mem_seg[0].addr == qdss_mem[0].pa) &&
		   (event_data->mem_seg[0].size <= qdss_mem[0].size)) {
		ret = cnss_genl_send_msg(qdss_mem[0].va,
					 CNSS_GENL_MSG_TYPE_QDSS,
					 file_name,
					 event_data->mem_seg[0].size);
		if (ret < 0) {
			cnss_pr_err("Fail to save QDSS data: %d\n", ret);
			goto out;
		}
		cnss_pr_info("QDSS Data saved in /data/vendor/wifi/%s",
			     file_name);
	} else if (event_data->mem_seg_len == 2) {
		/* FW sends the 2 segments in below format, we need to send
		 * segment 0 first then segment 1
		 *
		 *  QDSS ETR Memory - 1MB
		 * +---------------------+
		 * |   segment 1 start   |
		 * |                     |
		 * |                     |
		 * |                     |
		 * |   segment 1 end     |
		 * +---------------------+
		 * |   segment 0 start   |
		 * |                     |
		 * |                     |
		 * |   segment 0 end     |
		 * +---------------------+
		 */
		if (event_data->mem_seg[1].addr != qdss_mem[0].pa) {
			cnss_pr_err("Invalid seg 0 addr 0x%llx",
				    event_data->mem_seg[1].addr);
			goto out;
		}
		if (event_data->mem_seg[0].size + event_data->mem_seg[1].size !=
		    qdss_mem[0].size) {
			cnss_pr_err("Invalid total size 0x%x 0x%x",
				    event_data->mem_seg[0].size,
				    event_data->mem_seg[1].size);
			goto out;
		}

		ret = cnss_genl_send_msg((qdss_mem[0].va +
					 event_data->mem_seg[1].size),
					 CNSS_GENL_MSG_TYPE_QDSS,
					 file_name,
					 event_data->mem_seg[0].size);
		if (ret < 0) {
			cnss_pr_err("Fail to save QDSS data 0: %d\n", ret);
			goto out;
		}
		ret = cnss_genl_send_msg((qdss_mem[0].va),
					 CNSS_GENL_MSG_TYPE_QDSS,
					 file_name,
					 event_data->mem_seg[1].size);
		if (ret < 0) {
			cnss_pr_err("Fail to save QDSS data 1: %d\n", ret);
			goto out;
		}
		cnss_pr_info("QDSS Data saved in /data/vendor/wifi/%s",
			     file_name);
	} else {
		cnss_pr_err("Inavalid mem seg len %d", event_data->mem_seg_len);
	}

out:
	kfree(data);
	return ret;
}

static int cnss_qdss_trace_free_hdlr(struct cnss_plat_data *plat_priv)
{
	cnss_bus_free_qdss_mem(plat_priv);

	return 0;
}

static void m3_dump_open_timeout_func(struct timer_list *timer)
{
	struct m3_dump *m3_dump_data =
			from_timer(m3_dump_data, timer, open_timer);

	if (!m3_dump_data) {
		pr_err("%s: Invalid m3_dump_data from timer\n", __func__);
		return;
	}

	atomic_set(&m3_dump_data->open_timedout, 1);
	complete(&m3_dump_data->open_complete);
	pr_err("M3 dump open failed\n");
}

static void m3_dump_read_timeout_func(struct timer_list *timer)
{
	struct m3_dump *m3_dump_data =
			from_timer(m3_dump_data, timer, read_timer);

	if (!m3_dump_data) {
		pr_err("%s: Invalid m3_dump_data from timer\n", __func__);
		return;
	}

	if (m3_dump_data->task)
		send_sig(SIGKILL, m3_dump_data->task, 0);

	atomic_set(&m3_dump_data->read_timedout, 1);
	complete(&m3_dump_data->read_complete);
	pr_err("M3 dump collection failed\n");
}

static int m3_dump_open(struct inode *inode, struct file *file)
{
	struct cnss_plat_data *plat_priv;
	struct m3_dump *m3_dump_data;

	plat_priv = cnss_get_plat_priv_by_instance_id(iminor(inode));

	if (!plat_priv) {
		cnss_pr_err("%s: Failed to get plat_priv for instance_id 0x%x",
			    __func__, iminor(inode));
		return -ENODEV;
	}

	m3_dump_data = &plat_priv->m3_dump_data;
	if (m3_dump_data->file_open)
		return -EBUSY;

	del_timer_sync(&m3_dump_data->open_timer);
	if (atomic_read(&m3_dump_data->open_timedout) == 1)
		return -ENODEV;

	m3_dump_data->file_open = true;
	m3_dump_data->task = current;
	file->private_data = m3_dump_data;
	nonseekable_open(inode, file);

	init_completion(&m3_dump_data->read_complete);
	timer_setup(&m3_dump_data->read_timer, m3_dump_read_timeout_func, 0);
	mod_timer(&m3_dump_data->read_timer,
		  jiffies + msecs_to_jiffies(M3_DUMP_READ_TIMER_TIMEOUT));

	complete(&m3_dump_data->open_complete);
	return 0;
}

static ssize_t m3_dump_read(struct file *file, char __user *data, size_t len,
			    loff_t *ppos)
{
	struct m3_dump *m3_dump_data = (struct m3_dump *)file->private_data;
	char *bufp;

	if (!m3_dump_data) {
		pr_err("%s: m3_dump_data invalid", __func__);
		return -EFAULT;
	}

	bufp = (char *)m3_dump_data->dump_addr + *ppos;
	mod_timer(&m3_dump_data->read_timer,
		  jiffies + msecs_to_jiffies(M3_DUMP_READ_TIMER_TIMEOUT));

	if (*ppos + len > m3_dump_data->size)
		len = m3_dump_data->size - *ppos;

	if (copy_to_user(data, bufp, len)) {
		pr_err("%s: copy_to_user failed\n", __func__);
		return -EFAULT;
	}

	*ppos += len;

	return len;
}

static int m3_dump_release(struct inode *inode, struct file *file)
{
	struct m3_dump *m3_dump_data = (struct m3_dump *)file->private_data;
	int dump_minor = iminor(inode);
	int dump_major = imajor(inode);

	if (!m3_dump_data) {
		pr_err("%s: m3_dump_data invalid", __func__);
		return -EFAULT;
	}

	file->private_data = NULL;
	device_destroy(m3_dump_class, MKDEV(dump_major, dump_minor));

	complete(&m3_dump_data->read_complete);
	m3_dump_data->file_open = false;
	m3_dump_data->task = 0;
	return 0;
}

static const struct file_operations m3_dump_fops = {
	.owner          = THIS_MODULE,
	.open		= m3_dump_open,
	.read		= m3_dump_read,
	.release	= m3_dump_release,
};

static int cnss_init_m3_dump_class(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	if (m3_dump_class) {
		cnss_pr_dbg("m3_dump_class already initialized");
		goto out;
	}

	m3_dump_major = register_chrdev(UNNAMED_MAJOR, "dump",
					&m3_dump_fops);
	if (m3_dump_major < 0) {
		cnss_pr_err("%s: Unable to allocate a major number err = %d",
			    __func__, m3_dump_major);
		ret = m3_dump_major;
		goto out;
	}

	m3_dump_class = class_create(THIS_MODULE, "dump");
	if (IS_ERR(m3_dump_class)) {
		cnss_pr_err("%s: Unable to create class = %ld",
			    __func__, PTR_ERR(m3_dump_class));
		unregister_chrdev(m3_dump_major, "dump");
		m3_dump_major = 0;
		m3_dump_class = NULL;
		ret = -ENODEV;
	}

out:
	return ret;
}

static void cnss_deinit_m3_dump_class(void)
{
	if (m3_dump_class)
		class_destroy(m3_dump_class);

	if (m3_dump_major)
		unregister_chrdev(m3_dump_major, "dump");

	m3_dump_class = NULL;
	m3_dump_major = 0;
}

static int cnss_do_m3_dump_upload(struct cnss_plat_data *plat_priv,
				  const char *dump_file_name)
{
	int ret = 0;
	struct device *dump_dev = NULL;
	struct m3_dump *m3_dump_data = &plat_priv->m3_dump_data;

	init_completion(&m3_dump_data->open_complete);
	atomic_set(&m3_dump_data->open_timedout, 0);
	atomic_set(&m3_dump_data->read_timedout, 0);

	if (!m3_dump_class) {
		cnss_pr_err("M3 dump class not initialized.");
		return -ENODEV;
	}

	dump_dev = device_create(m3_dump_class, NULL,
				 MKDEV(m3_dump_major,
				       plat_priv->wlfw_service_instance_id),
				 NULL, dump_file_name);
	if (IS_ERR(dump_dev)) {
		ret = PTR_ERR(dump_dev);
		cnss_pr_err("%s: Unable to create device = %d",
			    __func__, ret);
		return ret;
	}

	/* This avoids race condition between the scheduled timer and the opened
	 * file discriptor during delay in user space app execution.
	 */
	timer_setup(&m3_dump_data->open_timer, m3_dump_open_timeout_func,0);

	mod_timer(&m3_dump_data->open_timer,
		  jiffies + msecs_to_jiffies(M3_DUMP_OPEN_TIMEOUT));

	ret = wait_for_completion_timeout(&m3_dump_data->open_complete,
					  msecs_to_jiffies(
					  M3_DUMP_OPEN_COMPLETION_TIMEOUT));
	if (!ret || (atomic_read(&m3_dump_data->open_timedout) == 1)) {
		ret = -ETIMEDOUT;
		cnss_pr_err("%s: Failed to open M3 dump", __func__);
		goto dump_dev_failed;
	}

	ret = wait_for_completion_timeout(&m3_dump_data->read_complete,
					  msecs_to_jiffies(
					  M3_DUMP_COMPLETION_TIMEOUT));
	if (!ret || (atomic_read(&m3_dump_data->read_timedout) == 1)) {
		ret = -ETIMEDOUT;
		cnss_pr_err("%s: Failed to collect M3 dump", __func__);
	} else {
		/* completed before timeout */
		ret = 0;
	}

	del_timer_sync(&m3_dump_data->read_timer);
	return ret;

dump_dev_failed:
	device_destroy(m3_dump_class,
		       MKDEV(m3_dump_major,
			     plat_priv->wlfw_service_instance_id));

	return ret;
}

static int cnss_m3_dump_upload_req_hdlr(struct cnss_plat_data *plat_priv,
					void *data)
{
	struct cnss_qmi_event_m3_dump_upload_req_data *event_data = data;
	struct cnss_fw_mem *m3_mem = NULL;
	char dump_file_name[30];
	struct m3_dump *m3_dump_data = &plat_priv->m3_dump_data;
	int i, ret = 0;

	cnss_pr_dbg("%s: %d pdev_id %d addr 0x%llx size %llu",
		    __func__, __LINE__,
		    event_data->pdev_id, event_data->addr, event_data->size);

	for (i = 0; i < plat_priv->fw_mem_seg_len; i++) {
		if (plat_priv->fw_mem[i].type == M3_DUMP_REGION_TYPE) {
			m3_mem = &plat_priv->fw_mem[i];
			break;
		}
	}

	if (!m3_mem) {
		cnss_pr_err("M3 dump memory not allocated\n");
		ret = -ENOMEM;
		goto send_resp;
	}

	if ((event_data->addr != m3_mem->pa) ||
	    (event_data->size > m3_mem->size)) {
		cnss_pr_err("Invalid M3 dump info from FW: addr: %llx, size: %lld; in plat_priv: addr:%pa size: %zd\n",
			    event_data->addr, event_data->size,
			    &m3_mem->pa, m3_mem->size);
		ret = -EINVAL;
		goto send_resp;
	}

	if (!m3_mem->va) {
		cnss_pr_err("M3 mem not remapped!\n");
		ret = -ENOMEM;
		goto send_resp;
	}

	memset(m3_dump_data, 0, sizeof(struct m3_dump));

	m3_dump_data->dump_addr = m3_mem->va;
	m3_dump_data->size = event_data->size;
	m3_dump_data->pdev_id = event_data->pdev_id;
	m3_dump_data->timestamp = ktime_to_ms(ktime_get());
	cnss_pr_dbg("%s: %d: pdev_id: %d va 0x%p size %d\n",
		    __func__, __LINE__,
		    m3_dump_data->pdev_id, m3_dump_data->dump_addr,
		    m3_dump_data->size);

	if (plat_priv->device_id == QCN9000_DEVICE_ID)
		snprintf(dump_file_name, sizeof(dump_file_name),
			 "m3_dump_qcn9000_pci%d.bin",
			 (plat_priv->wlfw_service_instance_id -
			  QCN9000_NODE_ID_BASE));
	else if (plat_priv->device_id == QCN9224_DEVICE_ID)
		snprintf(dump_file_name, sizeof(dump_file_name),
			 "m3_dump_qcn9224_pci%d.bin",
			 (plat_priv->wlfw_service_instance_id -
			  QCN9224_NODE_ID_BASE));
	else
		snprintf(dump_file_name, sizeof(dump_file_name),
			 "m3_dump_wifi%d.bin", m3_dump_data->pdev_id);

	ret = cnss_do_m3_dump_upload(plat_priv, (const char *)dump_file_name);
	if (ret)
		cnss_pr_err("M3 Dump upload failed with ret %d", ret);

send_resp:
	cnss_wlfw_m3_dump_upload_done_send_sync(plat_priv,
						event_data->pdev_id,
						ret);

	return ret;
}

static void cnss_driver_event_work(struct work_struct *work)
{
	struct cnss_plat_data *plat_priv =
		container_of(work, struct cnss_plat_data, event_work);
	struct cnss_driver_event *event;
	unsigned long flags;
	int ret = 0;

	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL!\n");
		return;
	}

#ifdef CONFIG_CNSS2_PM
	cnss_pm_stay_awake(plat_priv);
#endif

	spin_lock_irqsave(&plat_priv->event_lock, flags);

	while (!list_empty(&plat_priv->event_list)) {
		event = list_first_entry(&plat_priv->event_list,
					 struct cnss_driver_event, list);
		list_del(&event->list);
		spin_unlock_irqrestore(&plat_priv->event_lock, flags);

		cnss_pr_dbg("Processing driver event: %s%s(%d), state: 0x%lx\n",
			    cnss_driver_event_to_str(event->type),
			    event->sync ? "-sync" : "", event->type,
			    plat_priv->driver_state);

		switch (event->type) {
		case CNSS_DRIVER_EVENT_SERVER_ARRIVE:
			ret = cnss_wlfw_server_arrive(plat_priv, event->data);
			break;
		case CNSS_DRIVER_EVENT_SERVER_EXIT:
			ret = cnss_wlfw_server_exit(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_REQUEST_MEM:
			ret = cnss_bus_alloc_fw_mem(plat_priv);
			if (ret)
				break;
			ret = cnss_wlfw_respond_mem_send_sync(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_FW_MEM_READY:
			ret = cnss_fw_mem_ready_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_FW_READY:
			ret = cnss_fw_ready_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_COLD_BOOT_CAL_START:
			ret = cnss_cold_boot_cal_start_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_COLD_BOOT_CAL_DONE:
			ret = cnss_cold_boot_cal_done_hdlr(plat_priv,
							   event->data);
			break;
		case CNSS_DRIVER_EVENT_REGISTER_DRIVER:
			ret = cnss_bus_register_driver_hdlr(plat_priv,
							    event->data);
			break;
		case CNSS_DRIVER_EVENT_UNREGISTER_DRIVER:
			ret = cnss_bus_unregister_driver_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_RECOVERY:
			ret = cnss_driver_recovery_hdlr(plat_priv,
							event->data);
			break;
		case CNSS_DRIVER_EVENT_FORCE_FW_ASSERT:
			ret = cnss_bus_force_fw_assert_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_IDLE_RESTART:
			set_bit(CNSS_DRIVER_IDLE_RESTART,
				&plat_priv->driver_state);
			/* fall through */
		case CNSS_DRIVER_EVENT_POWER_UP:
			ret = cnss_power_up_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_IDLE_SHUTDOWN:
			set_bit(CNSS_DRIVER_IDLE_SHUTDOWN,
				&plat_priv->driver_state);
			/* fall through */
		case CNSS_DRIVER_EVENT_POWER_DOWN:
			ret = cnss_power_down_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_QDSS_TRACE_REQ_MEM:
			ret = cnss_qdss_trace_req_mem_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_QDSS_TRACE_SAVE:
			ret = cnss_qdss_trace_save_hdlr(plat_priv,
							event->data);
			break;
		case CNSS_DRIVER_EVENT_QDSS_TRACE_FREE:
			ret = cnss_qdss_trace_free_hdlr(plat_priv);
			break;
		case CNSS_DRIVER_EVENT_M3_DUMP_UPLOAD_REQ:
			ret = cnss_m3_dump_upload_req_hdlr(plat_priv,
							   event->data);
			break;
		default:
			cnss_pr_err("Invalid driver event type: %d",
				    event->type);
			kfree(event);
			spin_lock_irqsave(&plat_priv->event_lock, flags);
			continue;
		}

		spin_lock_irqsave(&plat_priv->event_lock, flags);
		if (event->sync) {
			event->ret = ret;
			complete(&event->complete);
			continue;
		}
		spin_unlock_irqrestore(&plat_priv->event_lock, flags);

		kfree(event);

		spin_lock_irqsave(&plat_priv->event_lock, flags);
	}
	spin_unlock_irqrestore(&plat_priv->event_lock, flags);

#ifdef CONFIG_CNSS2_PM
	cnss_pm_relax(plat_priv);
#endif
}

const struct rproc_ops cnss_rproc_ops = {
	.start = cnss_subsys_powerup,
	.stop = cnss_subsys_shutdown,
	.load = cnss_subsys_dummy_load,
	.parse_fw = cnss_subsys_add_ramdump_callback,
	.report_panic = cnss_subsys_crash_shutdown,
};

int cnss_register_subsys(struct cnss_plat_data *plat_priv)
{
	struct cnss_subsys_info *subsys_info = &plat_priv->subsys_info;
	struct device *dev = &plat_priv->plat_dev->dev;
	phandle rproc_node;
	int ret = 0;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_AHB:
		if (!plat_priv->rproc_handle) {
			/* rproc_handle for AHB targets are allocated inside
			 * remoteproc driver and is never freed.
			 * So get the rproc_handle once during first wifi load
			 * and reuse it for every subsequent loads
			 */
			if (of_property_read_u32(dev->of_node, "qcom,rproc",
						 &rproc_node)) {
				return -ENODEV;
			}

			plat_priv->rproc_handle =
				rproc_get_by_phandle(rproc_node);
			if (IS_ERR_OR_NULL(plat_priv->rproc_handle)) {
				cnss_pr_err("%s: Failed to get rproc handle %ld for device %s\n",
					    __func__,
					    PTR_ERR(plat_priv->rproc_handle),
					    plat_priv->device_name);
				return -EINVAL;
			}
		}

		subsys_info->subsys_desc.name = plat_priv->rproc_handle->name;

		break;
	case CNSS_BUS_PCI:
		if (!plat_priv->pci_dev)
			return -ENODEV;

		if (!plat_priv->rproc_handle) {
			/* Should never happen as rproc_handle is allocated
			 * during cnss_probe for PCI targets
			 */
			cnss_pr_err("Invalid rproc_handle for %s",
				    plat_priv->device_name);
			return -ENODEV;
		}
		subsys_info->subsys_desc.name = plat_priv->device_name;
		break;
	default:
		cnss_pr_err("%s: Invalid bus type for device 0x%lx\n",
			    __func__, plat_priv->device_id);
		return -ENODEV;

	}

	/* plat_priv->rproc_handle is never freed but subsys_handle is set here
	 * and is reset to NULL everytime target is shutdown.
	 */
	subsys_info->subsys_handle = plat_priv->rproc_handle;
	plat_priv->esoc_info.modem_notify_handler =
				cnss_register_notifier_cb(plat_priv);

	ret = rproc_boot(subsys_info->subsys_handle);
	if (ret) {
		cnss_pr_err("%s: Failed to boot device %s (%d)\n",
			    __func__, plat_priv->device_name, ret);
		CNSS_ASSERT(0);
		cnss_unregister_notifier_cb(plat_priv);
	}

	return ret;
}

void cnss_unregister_subsys(struct cnss_plat_data *plat_priv)
{
	struct cnss_subsys_info *subsys_info;
	struct cnss_pci_data *pci_priv;

	pci_priv = plat_priv->bus_priv;

	if (plat_priv->device_id == QCA8074_DEVICE_ID ||
	    plat_priv->device_id == QCA8074V2_DEVICE_ID ||
	    plat_priv->device_id == QCA5018_DEVICE_ID ||
	    plat_priv->device_id == QCN6122_DEVICE_ID ||
	    plat_priv->device_id == QCA6018_DEVICE_ID ||
	    plat_priv->device_id == QCA9574_DEVICE_ID) {
		return;
	}

	subsys_info = &plat_priv->subsys_info;
	if (subsys_info->subsys_handle)
		rproc_shutdown(subsys_info->subsys_handle);
	subsys_info->subsys_handle = NULL;
}

static int cnss_init_dump_entry(struct cnss_plat_data *plat_priv)
{
	struct cnss_ramdump_info *ramdump_info;
	struct msm_dump_entry dump_entry;

	ramdump_info = &plat_priv->ramdump_info;
	ramdump_info->dump_data.addr = ramdump_info->ramdump_pa;
	ramdump_info->dump_data.len = ramdump_info->ramdump_size;
	ramdump_info->dump_data.version = CNSS_DUMP_FORMAT_VER;
	ramdump_info->dump_data.magic = CNSS_DUMP_MAGIC_VER_V2;
	strlcpy(ramdump_info->dump_data.name, CNSS_DUMP_NAME,
		sizeof(ramdump_info->dump_data.name));
	dump_entry.id = MSM_DUMP_DATA_CNSS_WLAN;
	dump_entry.addr = virt_to_phys(&ramdump_info->dump_data);

#ifdef NOMINIDUMP
	return msm_dump_data_register_nominidump(MSM_DUMP_TABLE_APPS,
						&dump_entry);
#else
	return 0;
#endif
}

static int cnss_register_ramdump_v1(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct device *dev;
	struct cnss_subsys_info *subsys_info;
	struct cnss_ramdump_info *ramdump_info;
	u32 ramdump_size = 0;

	dev = &plat_priv->plat_dev->dev;
	subsys_info = &plat_priv->subsys_info;
	ramdump_info = &plat_priv->ramdump_info;

	if (of_property_read_u32(dev->of_node, "qcom,wlan-ramdump-dynamic",
				 &ramdump_size) == 0) {
		ramdump_info->ramdump_va =
			dma_alloc_coherent(dev, ramdump_size,
					   &ramdump_info->ramdump_pa,
					   GFP_KERNEL);

		if (ramdump_info->ramdump_va)
			ramdump_info->ramdump_size = ramdump_size;
	}

	cnss_pr_dbg("ramdump va: %pK, pa: %pa\n",
		    ramdump_info->ramdump_va, &ramdump_info->ramdump_pa);

	if (ramdump_info->ramdump_size == 0) {
		cnss_pr_info("Ramdump will not be collected");
		goto out;
	}

	ret = cnss_init_dump_entry(plat_priv);
	if (ret) {
		cnss_pr_err("Failed to setup dump table, err = %d\n", ret);
		goto free_ramdump;
	}

	ramdump_info->ramdump_dev =
		create_ramdump_device(subsys_info->subsys_desc.name,
				      subsys_info->subsys_desc.dev);
	if (!ramdump_info->ramdump_dev) {
		cnss_pr_err("Failed to create ramdump device!");
		ret = -ENOMEM;
		goto free_ramdump;
	}

	return 0;
free_ramdump:
	dma_free_coherent(dev, ramdump_info->ramdump_size,
			  ramdump_info->ramdump_va, ramdump_info->ramdump_pa);
out:
	return ret;
}

static void cnss_unregister_ramdump_v1(struct cnss_plat_data *plat_priv)
{
	struct device *dev;
	struct cnss_ramdump_info *ramdump_info;

	dev = &plat_priv->plat_dev->dev;
	ramdump_info = &plat_priv->ramdump_info;

	if (ramdump_info->ramdump_dev)
		destroy_ramdump_device(ramdump_info->ramdump_dev);

	if (ramdump_info->ramdump_va)
		dma_free_coherent(dev, ramdump_info->ramdump_size,
				  ramdump_info->ramdump_va,
				  ramdump_info->ramdump_pa);
}

static int cnss_register_ramdump_v2(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct cnss_subsys_info *subsys_info;
	struct cnss_ramdump_info_v2 *info_v2;
	struct cnss_dump_data *dump_data;
	struct msm_dump_entry dump_entry;
	struct device *dev = &plat_priv->plat_dev->dev;
	char ramdump_dev_name[CNSS_RAMDUMP_FILE_NAME_MAX_LEN] = {0};
	u32 ramdump_size = 0;

	subsys_info = &plat_priv->subsys_info;
	info_v2 = &plat_priv->ramdump_info_v2;
	dump_data = &info_v2->dump_data;

	if (of_property_read_u32(dev->of_node, "qcom,wlan-ramdump-dynamic",
				 &ramdump_size) == 0)
		info_v2->ramdump_size = ramdump_size;

	cnss_pr_dbg("Ramdump size 0x%lx\n", info_v2->ramdump_size);

	/* Dump data allocated during previous register needs to be freed
	 * before allocating again
	 */
	kfree(info_v2->dump_data_vaddr);
	info_v2->dump_data_vaddr = NULL;
	info_v2->dump_data_valid = false;

	info_v2->dump_data_vaddr = kzalloc(CNSS_DUMP_DESC_SIZE, GFP_KERNEL);
	if (!info_v2->dump_data_vaddr)
		return -ENOMEM;

	dump_data->paddr = virt_to_phys(info_v2->dump_data_vaddr);
	dump_data->version = CNSS_DUMP_FORMAT_VER_V2;
	dump_data->magic = CNSS_DUMP_MAGIC_VER_V2;
	dump_data->seg_version = CNSS_DUMP_SEG_VER;
	strlcpy(dump_data->name, CNSS_DUMP_NAME,
		sizeof(dump_data->name));
	dump_entry.id = MSM_DUMP_DATA_CNSS_WLAN;
	dump_entry.addr = virt_to_phys(dump_data);

#ifdef NOMINIDUMP
	ret = msm_dump_data_register_nominidump(MSM_DUMP_TABLE_APPS,
						&dump_entry);
#else
	ret = 0;
#endif
	if (ret) {
		cnss_pr_err("Failed to setup dump table, err = %d\n", ret);
		goto free_ramdump;
	}

	snprintf(ramdump_dev_name, sizeof(ramdump_dev_name), "ramdump_%s",
		 plat_priv->device_name);
	info_v2->ramdump_dev =
		create_ramdump_device((const char *)ramdump_dev_name,
				      subsys_info->subsys_desc.dev);
	if (!info_v2->ramdump_dev) {
		cnss_pr_err("Failed to create ramdump device!\n");
		ret = -ENOMEM;
		goto free_ramdump;
	}

	return 0;

free_ramdump:
	kfree(info_v2->dump_data_vaddr);
	info_v2->dump_data_vaddr = NULL;
	return ret;
}

static void cnss_unregister_ramdump_v2(struct cnss_plat_data *plat_priv)
{
	struct cnss_ramdump_info_v2 *info_v2;

	info_v2 = &plat_priv->ramdump_info_v2;

	if (info_v2->ramdump_dev)
		destroy_ramdump_device(info_v2->ramdump_dev);

	/* Freeing the dump data here causes loss of valid dump data in the
	 * below scenario
	 *  1. wifi down is in progress and target asserts after sending
	 *     mode OFF message
	 *  2. MHI notifies target assert, RDDM dump collection happens and
	 *     dump_data_vaddr has valid dump data
	 *  3. cnss_pci_remove is called as part of wifi down and dump data
	 *     with valid contents is now freed.
	 *
	 * To Avoid this scenario, skip freeing dump_data_vaddr here and free
	 * as part of cnss_register_ramdump_v2
	 *
	 * kfree(info_v2->dump_data_vaddr);
	 * info_v2->dump_data_vaddr = NULL;
	 * info_v2->dump_data_valid = false;
	 */
}

int cnss_register_ramdump(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	switch (plat_priv->device_id) {
	case QCA6174_DEVICE_ID:
		ret = cnss_register_ramdump_v1(plat_priv);
		break;
	case QCN9000_EMULATION_DEVICE_ID:
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
	case QCA6390_DEVICE_ID:
	case QCA6490_DEVICE_ID:
		ret = cnss_register_ramdump_v2(plat_priv);
		break;
	default:
		cnss_pr_err("Unknown device ID: 0x%lx\n", plat_priv->device_id);
		ret = -ENODEV;
		break;
	}
	return ret;
}

void cnss_unregister_ramdump(struct cnss_plat_data *plat_priv)
{
	switch (plat_priv->device_id) {
	case QCA6174_DEVICE_ID:
		cnss_unregister_ramdump_v1(plat_priv);
		break;
	case QCN9000_EMULATION_DEVICE_ID:
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
	case QCA6390_DEVICE_ID:
	case QCA6490_DEVICE_ID:
		cnss_unregister_ramdump_v2(plat_priv);
		break;
	default:
		cnss_pr_err("Unknown device ID: 0x%lx\n", plat_priv->device_id);
		break;
	}
}

#ifdef CONFIG_CNSS2_PM
static int cnss_register_bus_scale(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct cnss_bus_bw_info *bus_bw_info;

	bus_bw_info = &plat_priv->bus_bw_info;

	bus_bw_info->bus_scale_table =
		msm_bus_cl_get_pdata(plat_priv->plat_dev);
	if (bus_bw_info->bus_scale_table)  {
		bus_bw_info->bus_client =
			msm_bus_scale_register_client
			(bus_bw_info->bus_scale_table);
		if (!bus_bw_info->bus_client) {
			cnss_pr_err("Failed to register bus scale client!\n");
			ret = -EINVAL;
			goto out;
		}
	}

	return 0;
out:
	return ret;
}

static void cnss_unregister_bus_scale(struct cnss_plat_data *plat_priv)
{
	struct cnss_bus_bw_info *bus_bw_info;

	bus_bw_info = &plat_priv->bus_bw_info;

	if (bus_bw_info->bus_client)
		msm_bus_scale_unregister_client(bus_bw_info->bus_client);
}
#endif

static ssize_t fs_ready_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int fs_ready = 0;
	struct cnss_plat_data *plat_priv = dev_get_drvdata(dev);

	if (sscanf(buf, "%du", &fs_ready) != 1)
		return -EINVAL;

	cnss_pr_dbg("File system is ready, fs_ready is %d, count is %zu\n",
		    fs_ready, count);

	if (test_bit(QMI_BYPASS, &plat_priv->ctrl_params.quirks)) {
		printk(KERN_INFO "QMI is bypassed.\n");
		return count;
	}

	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL!\n");
		return count;
	}

	switch (plat_priv->device_id) {
	case QCN9000_EMULATION_DEVICE_ID:
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
	case QCA6390_DEVICE_ID:
	case QCA6490_DEVICE_ID:
		break;
	default:
		cnss_pr_err("Not supported for device ID 0x%lx\n",
			    plat_priv->device_id);
		return count;
	}

	if (fs_ready == FILE_SYSTEM_READY) {
		cnss_driver_event_post(plat_priv,
				       CNSS_DRIVER_EVENT_COLD_BOOT_CAL_START,
				       CNSS_EVENT_SYNC, NULL);
	}

	return count;
}

static DEVICE_ATTR_WO(fs_ready);

static int cnss_create_sysfs(struct cnss_plat_data *plat_priv)
{
	int ret = 0;

	ret = device_create_file(&plat_priv->plat_dev->dev, &dev_attr_fs_ready);
	if (ret) {
		cnss_pr_err("Failed to create device file, err = %d\n", ret);
		goto out;
	}

	return 0;
out:
	return ret;
}

static void cnss_remove_sysfs(struct cnss_plat_data *plat_priv)
{
	device_remove_file(&plat_priv->plat_dev->dev, &dev_attr_fs_ready);
}

static int cnss_event_work_init(struct cnss_plat_data *plat_priv)
{
	spin_lock_init(&plat_priv->event_lock);
	plat_priv->event_wq = alloc_workqueue("cnss_driver_event",
					      WQ_UNBOUND, 1);
	if (!plat_priv->event_wq) {
		cnss_pr_err("Failed to create event workqueue!\n");
		return -EFAULT;
	}

	INIT_WORK(&plat_priv->event_work, cnss_driver_event_work);
	INIT_LIST_HEAD(&plat_priv->event_list);

	return 0;
}

static void cnss_event_work_deinit(struct cnss_plat_data *plat_priv)
{
	if (plat_priv->event_wq)
		destroy_workqueue(plat_priv->event_wq);
}

static int cnss_misc_init(struct cnss_plat_data *plat_priv)
{
	int ret;

	timer_setup(&plat_priv->fw_boot_timer,
		    cnss_bus_fw_boot_timeout_hdlr, 0);
#ifdef CONFIG_CNSS2_PM
	register_pm_notifier(&cnss_pm_notifier);
#endif

	ret = device_init_wakeup(&plat_priv->plat_dev->dev, true);
	if (ret)
		cnss_pr_err("Failed to init platform device wakeup source, err = %d\n",
			    ret);

	init_completion(&plat_priv->power_up_complete);
	init_completion(&plat_priv->cal_complete);
	init_completion(&plat_priv->rddm_complete);
	init_completion(&plat_priv->recovery_complete);
	mutex_init(&plat_priv->dev_lock);

	return 0;
}

static void cnss_misc_deinit(struct cnss_plat_data *plat_priv)
{
	complete_all(&plat_priv->recovery_complete);
	complete_all(&plat_priv->rddm_complete);
	complete_all(&plat_priv->cal_complete);
	complete_all(&plat_priv->power_up_complete);
	device_init_wakeup(&plat_priv->plat_dev->dev, false);
#ifdef CONFIG_CNSS2_PM
	unregister_pm_notifier(&cnss_pm_notifier);
#endif
	del_timer(&plat_priv->fw_boot_timer);
}

static void cnss_init_control_params(struct cnss_plat_data *plat_priv)
{
	plat_priv->ctrl_params.quirks = CNSS_QUIRKS_DEFAULT;
	plat_priv->ctrl_params.mhi_timeout =
		CNSS_MHI_TIMEOUT_DEFAULT * timeout_factor;
	plat_priv->ctrl_params.qmi_timeout =
		CNSS_QMI_TIMEOUT_DEFAULT * timeout_factor;
	plat_priv->ctrl_params.bdf_type = 0;
	plat_priv->ctrl_params.time_sync_period =
		CNSS_TIME_SYNC_PERIOD_DEFAULT;
}

static const struct platform_device_id cnss_platform_id_table[] = {
	{ .name = "qca6174", .driver_data = QCA6174_DEVICE_ID, },
	{ .name = "qcn9000", .driver_data = QCN9000_DEVICE_ID, },
	{ .name = "qca8074", .driver_data = QCA8074_DEVICE_ID, },
	{ .name = "qca8074v2", .driver_data = QCA8074V2_DEVICE_ID, },
	{ .name = "qca6018", .driver_data = QCA6018_DEVICE_ID, },
	{ .name = "qca5018", .driver_data = QCA5018_DEVICE_ID, },
	{ .name = "qcn6122", .driver_data = QCN6122_DEVICE_ID, },
	{ .name = "qcn9224", .driver_data = QCN9224_DEVICE_ID, },
	{ .name = "qca9574", .driver_data = QCA9574_DEVICE_ID, },
};

static const struct of_device_id cnss_of_match_table[] = {
	{
		.compatible = "qcom,cnss",
		.data = (void *)&cnss_platform_id_table[0]},
	{
		.compatible = "qcom,cnss-qcn9000",
		.data = (void *)&cnss_platform_id_table[1]},
	{
		.compatible = "qcom,cnss-qca8074",
		.data = (void *)&cnss_platform_id_table[2]},
	{
		.compatible = "qcom,cnss-qca8074v2",
		.data = (void *)&cnss_platform_id_table[3]},
	{
		.compatible = "qcom,cnss-qca6018",
		.data = (void *)&cnss_platform_id_table[4]},
	{
		.compatible = "qcom,cnss-qca5018",
		.data = (void *)&cnss_platform_id_table[5]},
	{
		.compatible = "qcom,cnss-qcn6122",
		.data = (void *)&cnss_platform_id_table[6]},
	{
		.compatible = "qcom,cnss-qcn9224",
		.data = (void *)&cnss_platform_id_table[7]},
	{
		.compatible = "qcom,cnss-qca9574",
		.data = (void *)&cnss_platform_id_table[8]},
	{ },
};
MODULE_DEVICE_TABLE(of, cnss_of_match_table);

static const char *
cnss_module_param_feature_to_str(enum cnss_module_param_feature feature)
{
	switch (feature) {
	case CALDATA:
		return "caldata";
	case REGDB:
		return "regdb";
	default:
		return "unknown";
        }
};

static void
cnss_set_mod_param_feature_support(struct cnss_plat_data *plat_priv,
				   enum cnss_module_param_feature feature)
{
	int bmap = 0x0;
	bool *ptr;
	const char *fname = cnss_module_param_feature_to_str(feature);

	switch (feature) {
	case CALDATA:
		bmap = disable_caldata_bmap;
		/* By default caldata support is enabled */
		plat_priv->caldata_support = 1;
		ptr = &plat_priv->caldata_support;
		break;
	case REGDB:
		bmap = disable_regdb_bmap;
		/* By default regdb support should be enabled.
		 * But for now, it is disabled through module param.
		 */
		plat_priv->regdb_support = 1;
		ptr = &plat_priv->regdb_support;
		break;
	default:
		cnss_pr_err("%s: Unknown feature %u %s", __func__, feature,
			    fname);
		return;
	}

	switch (plat_priv->device_id) {
	case QCA8074_DEVICE_ID:
	case QCA8074V2_DEVICE_ID:
	case QCA6018_DEVICE_ID:
	case QCA5018_DEVICE_ID:
	case QCA9574_DEVICE_ID:
		if (bmap & SKIP_INTEGRATED) {
			cnss_pr_info("Disabling %s support for %s", fname,
				     plat_priv->device_name);
			*ptr = 0;
		}
		break;
	case QCN9000_DEVICE_ID:
		if ((plat_priv->qrtr_node_id == QCN9000_0 &&
		     (bmap & SKIP_PCI_0)) ||
		    (plat_priv->qrtr_node_id == QCN9000_1 &&
		     (bmap & SKIP_PCI_1)) ||
		    (plat_priv->qrtr_node_id == QCN9000_2 &&
		     (bmap & SKIP_PCI_2)) ||
		    (plat_priv->qrtr_node_id == QCN9000_3 &&
		     (bmap & SKIP_PCI_3))) {
			*ptr = 0;
			cnss_pr_info("Disabling %s support for %s", fname,
				     plat_priv->device_name);
		}
		break;
	case QCN6122_DEVICE_ID:
		if ((plat_priv->userpd_id == QCN6122_0 &&
		     (bmap & SKIP_PCI_0)) ||
		    (plat_priv->userpd_id == QCN6122_1 &&
		     (bmap & SKIP_PCI_1))) {
			*ptr = 0;
			cnss_pr_info("Disabling %s support for %s", fname,
				     plat_priv->device_name);
		}
		break;
	case QCN9224_DEVICE_ID:
		if ((plat_priv->qrtr_node_id == QCN9224_0 &&
		     (bmap & SKIP_PCI_0)) ||
		    (plat_priv->qrtr_node_id == QCN9224_1 &&
		     (bmap & SKIP_PCI_1)) ||
		    (plat_priv->qrtr_node_id == QCN9224_2 &&
		     (bmap & SKIP_PCI_2)) ||
		    (plat_priv->qrtr_node_id == QCN9224_3 &&
		     (bmap & SKIP_PCI_3))) {
			*ptr = 0;
			cnss_pr_info("Disabling %s support for %s", fname,
				     plat_priv->device_name);
		}
		break;
	default:
		cnss_pr_err("%s: UNKNOWN DEVICE ID", __func__);
		break;
	}
	return;
}

static int cnss_set_device_name(struct cnss_plat_data *plat_priv)
{
	u8 index = 0;

	switch (plat_priv->device_id) {
	case QCN9000_DEVICE_ID:
		index = plat_priv->wlfw_service_instance_id -
				QCN9000_NODE_ID_BASE;
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCN9000_PCI%d", index);
		break;
	case QCN9224_DEVICE_ID:
		index = plat_priv->wlfw_service_instance_id -
				QCN9224_NODE_ID_BASE;
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCN9224_PCI%d", index);
		break;
	case QCA8074_DEVICE_ID:
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCA8074");
		break;
	case QCA8074V2_DEVICE_ID:
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCA8074v2");
		break;
	case QCA6018_DEVICE_ID:
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCA6018");
		break;
	case QCA5018_DEVICE_ID:
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCA5018");
		break;
	case QCN6122_DEVICE_ID:
		index = plat_priv->wlfw_service_instance_id -
						WLFW_SERVICE_INS_ID_V01_QCN6122;
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCN6122_%d", index);
		break;
	case QCA9574_DEVICE_ID:
		snprintf(plat_priv->device_name, sizeof(plat_priv->device_name),
			 "QCA9574");
		break;
	default:
		cnss_pr_err("No such device id 0x%lx\n", plat_priv->device_id);
		return -ENODEV;
	}

	return 0;
}

void cnss_update_platform_feature_support(u8 type, u32 instance_id, u32 value)
{
	struct cnss_plat_data *plat_priv;

	plat_priv = cnss_get_plat_priv_by_instance_id(instance_id);
	if (!plat_priv) {
		cnss_pr_err("Failed to get plat_priv for instance_id 0x%x\n",
			    instance_id);
		return;
	}

	switch (type) {
	case CNSS_GENL_MSG_TYPE_DAEMON_SUPPORT:
		plat_priv->daemon_support = value;
		cnss_pr_info("Setting daemon_support=%d for instance_id 0x%x\n",
			     value, instance_id);
		break;
	case CNSS_GENL_MSG_TYPE_COLD_BOOT_SUPPORT:
		plat_priv->cold_boot_support = value;
		cnss_pr_info("Setting cold_boot_support=%d for instance_id 0x%x\n",
			     value, instance_id);
		break;
	case CNSS_GENL_MSG_TYPE_HDS_SUPPORT:
		plat_priv->hds_support = value;
		cnss_pr_info("Setting hds_support=%d for instance_id 0x%x\n",
			     value, instance_id);
		break;
	case CNSS_GENL_MSG_TYPE_REGDB_SUPPORT:
		plat_priv->regdb_support = value;
		cnss_pr_info("Setting regdb_support=%d for instance_id 0x%x\n",
			     value, instance_id);
		break;
	default:
		cnss_pr_err("Unknown type %d\n", type);
		break;
	}
}

static int platform_get_qcn6122_userpd_id(struct platform_device *plat_dev,
					  uint32_t *userpd_id)
{
	int ret = 0;
	const char *subsys_name;

	ret = of_property_read_string(plat_dev->dev.of_node,
				      "qcom,userpd-subsys-name",
				      &subsys_name);
	if (ret) {
		pr_err("subsys name get failed");
		return -EINVAL;
	}

	if (strcmp(subsys_name, "q6v5_wcss_userpd2") == 0) {
		*userpd_id = QCN6122_0;
		return 0;
	} else if (strcmp(subsys_name,
				"q6v5_wcss_userpd3") == 0) {
		*userpd_id = QCN6122_1;
		return 0;
	}

	pr_err("subsys name %s not found", subsys_name);
	return -EINVAL;
}

static bool
cnss_check_skip_target_probe(const struct platform_device_id *device_id,
			     u32 userpd_id, u32 node_id)
{
	/* skip_cnss based skip target checks */
	if (skip_cnss == CNSS_SKIP_ALL) {
		pr_err("Skipping cnss_probe for device 0x%lx\n",
		       device_id->driver_data);
		return true;
	} else if (skip_cnss == CNSS_SKIP_PCI &&
		   (device_id->driver_data == QCN9000_DEVICE_ID ||
		    device_id->driver_data == QCN9224_DEVICE_ID)) {
		pr_err("Skipping cnss_probe for device 0x%lx\n",
		       device_id->driver_data);
		return true;
	} else if (skip_cnss == CNSS_SKIP_AHB &&
		   (device_id->driver_data == QCA8074_DEVICE_ID ||
		   device_id->driver_data == QCA8074V2_DEVICE_ID ||
		   device_id->driver_data == QCA6018_DEVICE_ID ||
		   device_id->driver_data == QCN6122_DEVICE_ID ||
		   device_id->driver_data == QCA5018_DEVICE_ID ||
		   device_id->driver_data == QCA9574_DEVICE_ID)) {
		pr_err("Skipping cnss_probe for device 0x%lx\n",
		       device_id->driver_data);
		return true;
	}

	/* skip_radio_bmap based skip target checks
	 * SKIP_INTEGRATED - skip integrated radios ie. 5018,8074,8074v2,6018
	 * SKIP_PCI_0 - skip PCI_0 radios ie. first qcn9000/qcn6122 radios
	 * SKIP_PCI_1 - skip PCI_1 radios ie. second qcn9000/qcn6122 radios
	 */
	if ((skip_radio_bmap & SKIP_INTEGRATED) &&
	    ((device_id->driver_data == QCA5018_DEVICE_ID) ||
	    (device_id->driver_data == QCA8074_DEVICE_ID) ||
	    (device_id->driver_data == QCA8074V2_DEVICE_ID) ||
	    (device_id->driver_data == QCA6018_DEVICE_ID) ||
	    (device_id->driver_data == QCA9574_DEVICE_ID))) {
		pr_err("Skipping cnss_probe for device 0x%lx\n",
		       device_id->driver_data);
		return true;
	} else if ((skip_radio_bmap & SKIP_PCI_0) &&
		   (((userpd_id == QCN6122_0) &&
		   (device_id->driver_data == QCN6122_DEVICE_ID)) ||
		   ((node_id == QCN9000_0 || node_id == QCN9224_0) &&
		   (device_id->driver_data == QCN9000_DEVICE_ID ||
		    device_id->driver_data == QCN9224_DEVICE_ID)))) {
		pr_err("Skipping cnss_probe for PCI_0 device 0x%lx\n",
		       device_id->driver_data);
		return true;
	} else if ((skip_radio_bmap & SKIP_PCI_1) &&
		   (((userpd_id == QCN6122_1) &&
		   (device_id->driver_data == QCN6122_DEVICE_ID)) ||
		   ((node_id == QCN9000_1 || node_id == QCN9224_1) &&
		   (device_id->driver_data == QCN9000_DEVICE_ID ||
		    device_id->driver_data == QCN9224_DEVICE_ID)))) {
		pr_err("Skipping cnss_probe for PCI_1 device 0x%lx\n",
		       device_id->driver_data);
		return true;
	} else if ((skip_radio_bmap & SKIP_PCI_2) &&
		   ((node_id == QCN9000_2 || node_id == QCN9224_2) &&
		   (device_id->driver_data == QCN9000_DEVICE_ID ||
		    device_id->driver_data == QCN9224_DEVICE_ID))) {
		pr_err("Skipping cnss_probe for PCI_2 device 0x%lx\n",
		       device_id->driver_data);
		return true;
	} else if ((skip_radio_bmap & SKIP_PCI_3) &&
		   ((node_id == QCN9000_3 || node_id == QCN9224_3) &&
		   (device_id->driver_data == QCN9000_DEVICE_ID ||
		    device_id->driver_data == QCN9224_DEVICE_ID))) {
		pr_err("Skipping cnss_probe for PCI_3 device 0x%lx\n",
		       device_id->driver_data);
		return true;
	}

	return false;
}

static int cnss_rproc_register(struct cnss_plat_data *plat_priv)
{
	struct device *dev = &plat_priv->plat_dev->dev;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_AHB:
		/* rproc alloc is done from qcom_q6v5_wcss code for AHB
		 * plat_priv->rproc_handle will be filled during the first
		 * wifi load from cnss_register_subsys
		 */
		return 0;
	case CNSS_BUS_PCI:
		plat_priv->rproc_handle = rproc_alloc(dev,
						      plat_priv->device_name,
						      &cnss_rproc_ops,
						      plat_priv->firmware_name,
						      0);

		if (IS_ERR_OR_NULL(plat_priv->rproc_handle)) {
			cnss_pr_err("%s: Failed to register rproc %ld for device 0x%s\n",
				    __func__,
				    PTR_ERR(plat_priv->rproc_handle),
				    plat_priv->device_name);
			return -ENODEV;
		}

		plat_priv->rproc_handle->auto_boot = false;
		if (rproc_add(plat_priv->rproc_handle)) {
			cnss_pr_err("%s: Failed to add rproc for device %s\n",
				    __func__, plat_priv->device_name);
			rproc_free(plat_priv->rproc_handle);
			return -EINVAL;
		}
		break;
	default:
		cnss_pr_err("Invalid bus type for device %s\n",
			    plat_priv->device_name);
		return -EINVAL;
	}
	return 0;
}

static void cnss_rproc_unregister(struct cnss_plat_data *plat_priv)
{
	if (plat_priv->rproc_handle) {
		/* For AHB targets, ref count for rproc is taken during first
		 * wifi load from cnss_register_subsys, put the ref count here.
		 * For PCI targets, rproc_handle is allocated during cnss_prove,
		 * delete and free the proc_handle here.
		 */
		if (plat_priv->bus_type == CNSS_BUS_AHB) {
			rproc_put(plat_priv->rproc_handle);
		} else if (plat_priv->bus_type == CNSS_BUS_PCI) {
			rproc_del(plat_priv->rproc_handle);
			rproc_free(plat_priv->rproc_handle);
		}
	}
}

static int cnss_probe(struct platform_device *plat_dev)
{
	int ret = 0;
	struct cnss_plat_data *plat_priv = NULL;
	const struct of_device_id *of_id;
	const struct platform_device_id *device_id;
	u32 node_id = 0, userpd_id = 0, node_id_base;
	const int *soc_version;

	if (cnss_get_plat_priv(plat_dev)) {
		pr_err("Driver is already initialized!\n");
		ret = -EEXIST;
		goto out;
	}
	of_id = of_match_device(cnss_of_match_table, &plat_dev->dev);
	if (!of_id || !of_id->data) {
		pr_err("Failed to find of match device!\n");
		ret = -ENODEV;
		goto out;
	}

	device_id = (const struct platform_device_id *)of_id->data;

	if ((device_id->driver_data == QCN6122_DEVICE_ID) &&
	    (platform_get_qcn6122_userpd_id(plat_dev, &userpd_id))) {
		pr_err("Error: No userpd_id in device_tree\n");
		CNSS_ASSERT(0);
		ret = -ENODEV;
		goto out;
	}

	if (device_id->driver_data == QCN9224_DEVICE_ID &&
	    !enable_qcn9224_support) {
		pr_err("Skipping QCN9224 device support\n");
		ret = -ENODEV;
		goto out;
	}

	if ((device_id->driver_data == QCN9000_DEVICE_ID ||
	     device_id->driver_data == QCN9224_DEVICE_ID) &&
	    (of_property_read_u32(plat_dev->dev.of_node,
				  "qrtr_node_id", &node_id))) {
		pr_err("Error: No qrtr_node_id in device_tree\n");
		CNSS_ASSERT(0);
		ret = -ENODEV;
		goto out;
	}

#ifdef CONFIG_CNSS2_QCA9574_SUPPORT
	/* Check for QCA9574 here and skip probe accordingly */
	if (device_id->driver_data == QCA9574_DEVICE_ID &&
	    !cpu_is_internal_wifi_enabled())
	{
		pr_err("Skipping cnss_probe for device 0x%lx\n",
		       device_id->driver_data);
		ret = -ENODEV;
		goto out;
	}
#endif

	if (cnss_check_skip_target_probe(device_id, userpd_id, node_id))
		goto out;

	soc_version = of_get_property(of_find_node_by_path("/"),
				      "soc_version_major", NULL);
	if (!soc_version) {
		pr_err("Failed to get soc_version_major from device tree");
		CNSS_ASSERT(0);
		ret = -EINVAL;
		goto out;
	}

	soc_version_major = *soc_version;
	soc_version_major = le32_to_cpu(soc_version_major);

	if (device_id->driver_data == QCA8074_DEVICE_ID) {
		if (soc_version_major == 2) {
			pr_err("Skip QCA8074V1 in V2 platform\n");
			ret = -ENODEV;
			goto out;
		}
	}

	if (device_id->driver_data == QCA8074V2_DEVICE_ID) {
		if (soc_version_major == 1) {
			pr_err("Skip QCA8074V2 in V1 platform\n");
			ret = -ENODEV;
			goto out;
		}
	}

	plat_priv = devm_kzalloc(&plat_dev->dev, sizeof(*plat_priv),
				 GFP_KERNEL);
	if (!plat_priv) {
		ret = -ENOMEM;
		goto out;
	}

	plat_priv->plat_dev = plat_dev;
	plat_priv->device_id = device_id->driver_data;
	plat_priv->plat_dev_id = (struct platform_device_id *)device_id;
	plat_priv->service_id = WLFW_SERVICE_ID_V01;

	switch (plat_priv->device_id) {
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
		plat_priv->bus_type = CNSS_BUS_PCI;
		plat_priv->qrtr_node_id = node_id;
		plat_priv->wlfw_service_instance_id = node_id + FW_ID_BASE;

		if (plat_priv->device_id == QCN9224_DEVICE_ID)
			node_id_base = QCN9224_NODE_ID_BASE;
		else
			node_id_base = QCN9000_NODE_ID_BASE;

		if (plat_priv->wlfw_service_instance_id == node_id_base)
			plat_priv->board_info.board_id_override = bdf_pci0;
		else if (plat_priv->wlfw_service_instance_id ==
			 node_id_base + 1)
			plat_priv->board_info.board_id_override = bdf_pci1;

		if (plat_priv->device_id == QCN9224_DEVICE_ID)
			snprintf(plat_priv->firmware_name,
				 sizeof(plat_priv->firmware_name),
				 QCN9224_DEFAULT_FW_FILE_NAME);
		else
			snprintf(plat_priv->firmware_name,
				 sizeof(plat_priv->firmware_name),
				 QCN9000_DEFAULT_FW_FILE_NAME);
		break;
	case QCA8074_DEVICE_ID:
	case QCA8074V2_DEVICE_ID:
	case QCA5018_DEVICE_ID:
	case QCA6018_DEVICE_ID:
	case QCA9574_DEVICE_ID:
		plat_priv->bus_type = CNSS_BUS_AHB;
		plat_priv->wlfw_service_instance_id =
			WLFW_SERVICE_INS_ID_V01_QCA8074;
		plat_priv->board_info.board_id_override = bdf_integrated;
		break;
	case QCN6122_DEVICE_ID:
		plat_priv->bus_type = CNSS_BUS_AHB;
		plat_priv->userpd_id = userpd_id;
		plat_priv->wlfw_service_instance_id =
			WLFW_SERVICE_INS_ID_V01_QCN6122 + userpd_id;
		if (plat_priv->wlfw_service_instance_id ==
			WLFW_SERVICE_INS_ID_V01_QCN6122 + QCN6122_0)
			plat_priv->board_info.board_id_override = bdf_pci0;
		else if (plat_priv->wlfw_service_instance_id ==
			WLFW_SERVICE_INS_ID_V01_QCN6122 + QCN6122_1)
			plat_priv->board_info.board_id_override = bdf_pci1;

		plat_priv->qcn6122.qgic2_msi =
					cnss_qgic2_enable_msi(plat_priv);
		if (!plat_priv->qcn6122.qgic2_msi) {
			cnss_pr_err("qgic2_msi fails: dev 0x%lx userpd id %d\n",
				    plat_priv->device_id, userpd_id);
			ret = -ENODEV;
			goto out;
		}
		break;
	default:
		cnss_pr_err("No such device id %p\n", device_id);
		ret = -ENODEV;
		goto out;
	}
	ret = cnss_set_device_name(plat_priv);
	if (ret)
		goto out;

	ret = cnss_rproc_register(plat_priv);
	if (ret)
		goto out;

	cnss_set_mod_param_feature_support(plat_priv, CALDATA);
	cnss_set_mod_param_feature_support(plat_priv, REGDB);
	cnss_set_plat_priv(plat_dev, plat_priv);
	platform_set_drvdata(plat_dev, plat_priv);
	memset(&qmi_log, 0, sizeof(struct qmi_history) * QMI_HISTORY_SIZE);
	INIT_LIST_HEAD(&plat_priv->vreg_list);
	INIT_LIST_HEAD(&plat_priv->clk_list);

	cnss_init_control_params(plat_priv);

	ret = cnss_get_resources(plat_priv);
	if (ret)
		goto reset_ctx;

	if (!test_bit(SKIP_DEVICE_BOOT, &plat_priv->ctrl_params.quirks)) {
		ret = cnss_power_on_device(plat_priv, plat_priv->device_id);
		if (ret)
			goto free_res;
	}

#ifdef CONFIG_CNSS2_PM
	ret = cnss_register_esoc(plat_priv);
	if (ret)
		goto deinit_bus;

	ret = cnss_register_bus_scale(plat_priv);
	if (ret)
		goto unreg_esoc;
#endif

	ret = cnss_create_sysfs(plat_priv);
	if (ret)
		goto unreg_bus_scale;

	ret = cnss_event_work_init(plat_priv);
	if (ret)
		goto remove_sysfs;

	ret = cnss_qmi_init(plat_priv);
	if (ret)
		goto deinit_event_work;

	ret = cnss_debugfs_create(plat_priv);
	if (ret)
		goto deinit_qmi;

	ret = cnss_misc_init(plat_priv);
	if (ret)
		goto destroy_debugfs;
#if defined(CNSS2_COEX) || defined(CNSS2_IMS)
	cnss_register_coex_service(plat_priv);
	cnss_register_ims_service(plat_priv);
#endif

	ret = cnss_genl_init();
	if (ret < 0)
		cnss_pr_err("CNSS genl init failed %d\n", ret);

	ret = cnss_init_m3_dump_class(plat_priv);
	if (ret)
		goto deinit_genl;

	cnss_pr_info("Platform driver probed successfully. plat %p tgt 0x%lx\n",
		     plat_priv, plat_priv->device_id);

	return 0;

deinit_genl:
	cnss_genl_exit();
destroy_debugfs:
	cnss_debugfs_destroy(plat_priv);
deinit_qmi:
	cnss_qmi_deinit(plat_priv);
deinit_event_work:
	cnss_event_work_deinit(plat_priv);
remove_sysfs:
	cnss_remove_sysfs(plat_priv);
unreg_bus_scale:
#ifdef CONFIG_CNSS2_PM
	cnss_unregister_bus_scale(plat_priv);
unreg_esoc:
	cnss_unregister_esoc(plat_priv);
deinit_bus:
#endif
	if (!test_bit(SKIP_DEVICE_BOOT, &plat_priv->ctrl_params.quirks))
		cnss_bus_deinit(plat_priv);
	if (!test_bit(SKIP_DEVICE_BOOT, &plat_priv->ctrl_params.quirks))
		cnss_power_off_device(plat_priv, plat_priv->device_id);
free_res:
	cnss_put_resources(plat_priv);
reset_ctx:
	platform_set_drvdata(plat_dev, NULL);
	cnss_set_plat_priv(plat_dev, NULL);
	cnss_rproc_unregister(plat_priv);
out:
	return ret;
}

static int cnss_remove(struct platform_device *plat_dev)
{
	struct cnss_plat_data *plat_priv = platform_get_drvdata(plat_dev);

	cnss_deinit_m3_dump_class();
	cnss_qgic2_disable_msi(plat_priv);
	cnss_genl_exit();
#if defined(CNSS2_COEX) || defined(CNSS2_IMS)
	cnss_unregister_ims_service(plat_priv);
	cnss_unregister_coex_service(plat_priv);
#endif
	cnss_misc_deinit(plat_priv);
	cnss_debugfs_destroy(plat_priv);
	cnss_qmi_deinit(plat_priv);
	cnss_event_work_deinit(plat_priv);
	cnss_remove_sysfs(plat_priv);
#ifdef CONFIG_CNSS2_PM
	cnss_unregister_bus_scale(plat_priv);
	cnss_unregister_esoc(plat_priv);
#endif
	cnss_bus_deinit(plat_priv);
	cnss_put_resources(plat_priv);
	cnss_rproc_unregister(plat_priv);
	platform_set_drvdata(plat_dev, NULL);

	return 0;
}

static struct platform_driver cnss_platform_driver = {
	.probe  = cnss_probe,
	.remove = cnss_remove,
	.driver = {
		.name = "cnss2",
		.of_match_table = cnss_of_match_table,
#ifdef CONFIG_CNSS_ASYNC
		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
#endif
	},
};

static int __init cnss_initialize(void)
{
	int ret = 0;

	cnss_debug_init();
	ret = platform_driver_register(&cnss_platform_driver);
	if (ret)
		cnss_debug_deinit();
	cnss_bus_init_by_type(CNSS_BUS_PCI);

	return ret;
}

static void __exit cnss_exit(void)
{
	platform_driver_unregister(&cnss_platform_driver);
	cnss_debug_deinit();
}

module_init(cnss_initialize);
module_exit(cnss_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CNSS2 Platform Driver");
