/*
 * Copyright (c) 2016,2018 The Linux Foundation. All rights reserved.
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

#include <linux/device.h>
#include <linux/sysfs.h>

#include "wil6210.h"
#include "wmi.h"
#include "txrx.h"

static ssize_t
wil_ftm_txrx_offset_sysfs_show(struct device *dev,
			       struct device_attribute *attr,
			       char *buf)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	struct wil6210_vif *vif = ndev_to_vif(wil->main_ndev);
	struct {
		struct wmi_cmd_hdr wmi;
		struct wmi_tof_get_tx_rx_offset_event evt;
	} __packed reply;
	int rc;
	ssize_t len;

	if (!test_bit(WMI_FW_CAPABILITY_FTM, wil->fw_capabilities))
		return -EOPNOTSUPP;

	memset(&reply, 0, sizeof(reply));
	rc = wmi_call(wil, WMI_TOF_GET_TX_RX_OFFSET_CMDID, vif->mid, NULL, 0,
		      WMI_TOF_GET_TX_RX_OFFSET_EVENTID,
		      &reply, sizeof(reply), 100);
	if (rc < 0)
		return rc;
	if (reply.evt.status) {
		wil_err(wil, "get_tof_tx_rx_offset failed, error %d\n",
			reply.evt.status);
		return -EIO;
	}
	len = snprintf(buf, PAGE_SIZE, "%u %u\n",
		       le32_to_cpu(reply.evt.tx_offset),
		       le32_to_cpu(reply.evt.rx_offset));
	return len;
}

int wil_ftm_offset_set(struct wil6210_priv *wil, const char *buf)
{
	wil->ftm_txrx_offset.enabled = 0;
	if (sscanf(buf, "%u %u", &wil->ftm_txrx_offset.tx_offset,
		   &wil->ftm_txrx_offset.tx_offset) != 2)
		return -EINVAL;

	wil->ftm_txrx_offset.enabled = 1;
	return 0;
}

static ssize_t
wil_ftm_txrx_offset_sysfs_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	int rc;

	rc = wil_ftm_offset_set(wil, buf);
	if (rc < 0)
		return rc;

	rc = wmi_set_tof_tx_rx_offset(wil, wil->ftm_txrx_offset.tx_offset,
				      wil->ftm_txrx_offset.rx_offset);
	if (rc < 0)
		return rc;

	return count;
}

static DEVICE_ATTR(ftm_txrx_offset, 0644,
		   wil_ftm_txrx_offset_sysfs_show,
		   wil_ftm_txrx_offset_sysfs_store);

static ssize_t
board_file_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);

	wil_get_board_file(wil, buf, PAGE_SIZE);
	strlcat(buf, "\n", PAGE_SIZE);
	return strlen(buf);
}

int wil_board_file_set(struct wil6210_priv *wil, const char *buf,
		       size_t count)
{
	size_t len;

	if (count == 0)
		return -EINVAL;

	mutex_lock(&wil->mutex);

	kfree(wil->board_file);
	wil->board_file = NULL;

	len = count;
	if (buf[count - 1] == '\n')
		len--;
	len = strnlen(buf, len);

	if (len > len + 1) { /* check wraparound */
		mutex_unlock(&wil->mutex);
		return -EINVAL;
	}

	if (len > 0) {
		wil->board_file = kmalloc(len + 1, GFP_KERNEL);
		if (!wil->board_file) {
			mutex_unlock(&wil->mutex);
			return -ENOMEM;
		}
		strlcpy(wil->board_file, buf, len + 1);
	}
	mutex_unlock(&wil->mutex);

	return 0;
}

static ssize_t
board_file_store(struct device *dev,
		 struct device_attribute *attr,
		 const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	int rc;

	rc = wil_board_file_set(wil, buf, count);
	if (rc < 0)
		return rc;

	return count;
}

static DEVICE_ATTR_RW(board_file);

static ssize_t
wil_fst_link_loss_sysfs_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	ssize_t len = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(wil->sta); i++)
		if (wil->sta[i].status == wil_sta_connected)
			len += snprintf(buf + len, PAGE_SIZE - len,
					"[%d] %pM %s\n", i, wil->sta[i].addr,
					wil->sta[i].fst_link_loss ?
					"On" : "Off");

	return len;
}

static ssize_t
wil_fst_link_loss_sysfs_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	u8 addr[ETH_ALEN];
	char *token, *dupbuf, *tmp;
	int rc = -EINVAL;
	bool fst_link_loss;

	tmp = kmemdup(buf, count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	tmp[count] = '\0';
	dupbuf = tmp;

	token = strsep(&dupbuf, " ");
	if (!token)
		goto out;

	/* mac address */
	if (sscanf(token, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		   &addr[0], &addr[1], &addr[2],
		   &addr[3], &addr[4], &addr[5]) != 6)
		goto out;

	/* On/Off */
	if (strtobool(dupbuf, &fst_link_loss))
		goto out;

	wil_dbg_misc(wil, "set [%pM] with %d\n", addr, fst_link_loss);

	rc = wmi_link_maintain_cfg_write(wil, addr, fst_link_loss);
	if (!rc)
		rc = count;

out:
	kfree(tmp);
	return rc;
}

static DEVICE_ATTR(fst_link_loss, 0644,
		   wil_fst_link_loss_sysfs_show,
		   wil_fst_link_loss_sysfs_store);

int wil_qos_weights_set(struct wil6210_priv *wil, const char *buf,
			size_t count)
{
	int rc = -EINVAL, i;
	u8 weights[WMI_QOS_NUM_OF_PRIORITY - 1];
	char *token, *dupbuf, *tmp;

	tmp = kmemdup(buf, count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	tmp[count] = '\0';
	dupbuf = tmp;

	wil->qos_weights_set = 0;
	for (i = 0; i < ARRAY_SIZE(weights); i++) {
		token = strsep(&dupbuf, " ");
		if (!token) {
			wil_err(wil, "missing prio_weight[%d]\n", i);
			goto out;
		}
		if (kstrtou8(token, 0, &weights[i])) {
			wil_err(wil, "unrecognized prio_weight[%d]\n", i);
			goto out;
		}

		if (weights[i] < WMI_QOS_MIN_DEFAULT_WEIGHT ||
		    weights[i] > WMI_QOS_MAX_WEIGHT) {
			wil_err(wil, "invalid prio_weight[%d] %d\n",
				i, weights[i]);
			goto out;
		}
		if (i > 0 && weights[i] < weights[i - 1]) {
			wil_err(wil, "invalid (descending) prio_weight[%d] %d\n",
				i, weights[i]);
			goto out;
		}
	}

	wil_dbg_misc(wil, "qos weights:\n");
	for (i = 0; i < ARRAY_SIZE(weights); i++) {
		wil_dbg_misc(wil, "[%d] %d\n", i + 1, weights[i]);
		wil->qos_weights[i] = weights[i];
	}
	wil->qos_weights_set = 1;

	rc = 0;

out:
	kfree(tmp);
	return rc;
}

static ssize_t
wil_qos_weights_sysfs_store(struct device *dev,
			    struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	struct wil6210_vif *vif = ndev_to_vif(wil->main_ndev);
	int rc;

	rc = wil_qos_weights_set(wil, buf, count);
	if (rc)
		return rc;

	rc = wil_wmi_ring_priority_weight(vif, ARRAY_SIZE(wil->qos_weights),
					  wil->qos_weights);
	if (!rc)
		rc = count;

	return rc;
}

static DEVICE_ATTR(qos_weights, 0220,
		   NULL,
		   wil_qos_weights_sysfs_store);

static ssize_t
wil_qos_link_prio_sysfs_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	struct wil6210_vif *vif;
	int rc = -EINVAL;
	u8 mid, priority;
	u8 mac[ETH_ALEN];
	bool mac_found;
	char *token, *dupbuf, *tmp;

	/* specify either peer MAC or VIF index (MID) followed by
	 * priority (0-3).
	 */
	tmp = kmemdup(buf, count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	tmp[count] = '\0';
	dupbuf = tmp;

	token = strsep(&dupbuf, " ");
	if (!token) {
		wil_err(wil, "invalid qos priority input\n");
		goto out;
	}

	mac_found = mac_pton(token, mac);
	if (!mac_found) {
		/* look for VIF index */
		rc = kstrtou8(token, 0, &mid);
		if (rc) {
			wil_err(wil, "unrecognized qos priority VIF index\n");
			goto out;
		}
	}

	rc = kstrtou8(dupbuf, 0, &priority);
	if (rc) {
		wil_err(wil, "unrecognized qos priority\n");
		goto out;
	}

	rc = -EINVAL;
	if (priority < 0 || priority >= WMI_QOS_NUM_OF_PRIORITY) {
		wil_err(wil, "invalid qos priority %d\n", priority);
		goto out;
	}

	if (mac_found) {
		/* set priority for peer's tx rings */
		int ring_idx, cid;

		vif = ndev_to_vif(wil->main_ndev);

		for (cid = 0; cid < ARRAY_SIZE(wil->sta); cid++) {
			if (wil->sta[cid].status != wil_sta_unused &&
			    ether_addr_equal(wil->sta[cid].addr, mac))
				break;
		}

		if (cid == ARRAY_SIZE(wil->sta)) {
			wil_err(wil, "invalid qos priority peer %pM\n", mac);
			goto out;
		}

		for (ring_idx = wil_get_min_tx_ring_id(wil);
		     ring_idx < WIL6210_MAX_TX_RINGS; ring_idx++) {
			if (wil->ring2cid_tid[ring_idx][0] != cid ||
			    !wil->ring_tx[ring_idx].va)
				continue;

			wil_dbg_misc(wil, "set ring %d qos priority %d\n",
				     ring_idx, priority);
			rc = wil_wmi_ring_priority(vif, ring_idx, priority);
			if (rc)
				break;
		}
	} else {
		/* set VIF's default priority */
		mutex_lock(&wil->vif_mutex);
		if (mid >= wil->max_vifs) {
			wil_err(wil, "invalid qos priority VIF %d\n", mid);
			mutex_unlock(&wil->vif_mutex);
			goto out;
		}
		vif = wil->vifs[mid];
		if (!vif) {
			wil_err(wil, "qos priority VIF %d unused\n", mid);
			mutex_unlock(&wil->vif_mutex);
			goto out;
		}

		wil_dbg_misc(wil, "set qos priority %d for mid %d\n",
			     priority, vif->mid);
		rc = wil_wmi_ring_priority(vif, WIL6210_MAX_TX_RINGS,
					   priority);
		mutex_unlock(&wil->vif_mutex);
	}

	if (!rc)
		rc = count;

out:
	kfree(tmp);
	return rc;
}

static DEVICE_ATTR(qos_link_prio, 0220,
		   NULL,
		   wil_qos_link_prio_sysfs_store);

static ssize_t
thermal_throttling_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	ssize_t len;
	struct wmi_tt_data tt_data;
	int i, rc;

	rc = wmi_get_tt_cfg(wil, &tt_data);
	if (rc)
		return rc;

	len = snprintf(buf, PAGE_SIZE, "    high      max       critical\n");

	len += snprintf(buf + len, PAGE_SIZE - len, "bb: ");
	if (tt_data.bb_enabled)
		for (i = 0; i < WMI_NUM_OF_TT_ZONES; ++i)
			len += snprintf(buf + len, PAGE_SIZE - len,
					"%03d-%03d   ",
					tt_data.bb_zones[i].temperature_high,
					tt_data.bb_zones[i].temperature_low);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "* disabled *");
	len += snprintf(buf + len, PAGE_SIZE - len, "\nrf: ");
	if (tt_data.rf_enabled)
		for (i = 0; i < WMI_NUM_OF_TT_ZONES; ++i)
			len += snprintf(buf + len, PAGE_SIZE - len,
					"%03d-%03d   ",
					tt_data.rf_zones[i].temperature_high,
					tt_data.rf_zones[i].temperature_low);
	else
		len += snprintf(buf + len, PAGE_SIZE - len, "* disabled *");
	len += snprintf(buf + len, PAGE_SIZE - len, "\n");

	return len;
}

int wil_tt_set(struct wil6210_priv *wil, const char *buf,
	       size_t count)
{
	int i, rc = -EINVAL;
	char *token, *dupbuf, *tmp;
	struct wmi_tt_data tt_data = {
		.bb_enabled = 0,
		.rf_enabled = 0,
	};

	tmp = kmemdup(buf, count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	tmp[count] = '\0';
	dupbuf = tmp;

	/* Format for writing is 12 unsigned bytes separated by spaces:
	 * <bb_z1_h> <bb_z1_l> <bb_z2_h> <bb_z2_l> <bb_z3_h> <bb_z3_l> \
	 * <rf_z1_h> <rf_z1_l> <rf_z2_h> <rf_z2_l> <rf_z3_h> <rf_z3_l>
	 * To disable thermal throttling for bb or for rf, use 0 for all
	 * its six set points.
	 */

	/* bb */
	for (i = 0; i < WMI_NUM_OF_TT_ZONES; ++i) {
		token = strsep(&dupbuf, " ");
		if (!token)
			goto out;
		if (kstrtou8(token, 0, &tt_data.bb_zones[i].temperature_high))
			goto out;
		token = strsep(&dupbuf, " ");
		if (!token)
			goto out;
		if (kstrtou8(token, 0, &tt_data.bb_zones[i].temperature_low))
			goto out;

		if (tt_data.bb_zones[i].temperature_high > 0 ||
		    tt_data.bb_zones[i].temperature_low > 0)
			tt_data.bb_enabled = 1;
	}
	/* rf */
	for (i = 0; i < WMI_NUM_OF_TT_ZONES; ++i) {
		token = strsep(&dupbuf, " ");
		if (!token)
			goto out;
		if (kstrtou8(token, 0, &tt_data.rf_zones[i].temperature_high))
			goto out;
		token = strsep(&dupbuf, " ");
		if (!token)
			goto out;
		if (kstrtou8(token, 0, &tt_data.rf_zones[i].temperature_low))
			goto out;

		if (tt_data.rf_zones[i].temperature_high > 0 ||
		    tt_data.rf_zones[i].temperature_low > 0)
			tt_data.rf_enabled = 1;
	}

	wil->tt_data = tt_data;
	wil->tt_data_set = true;
	rc = 0;

out:
	kfree(tmp);
	return rc;
}

static ssize_t
thermal_throttling_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	int rc;

	rc = wil_tt_set(wil, buf, count);
	if (rc)
		return rc;

	rc = wmi_set_tt_cfg(wil, &wil->tt_data);
	if (rc)
		return rc;

	return count;
}

static DEVICE_ATTR_RW(thermal_throttling);

static ssize_t
wil_lo_power_calib_sysfs_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	ssize_t len;

	if (wil->lo_calib == WIL_LO_CALIB_INVALID_INDEX)
		len = snprintf(buf, PAGE_SIZE, "unset\n");
	else
		len = snprintf(buf, PAGE_SIZE, "%u\n", wil->lo_calib);

	return len;
}

static ssize_t
wil_lo_power_calib_sysfs_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	int rc;
	u8 index;

	if (kstrtou8(buf, 0, &index))
		return -EINVAL;

	rc = wmi_lo_power_calib_from_otp(wil, index);
	if (!rc)
		rc = count;

	return rc;
}

static DEVICE_ATTR(lo_power_calib, 0644,
		   wil_lo_power_calib_sysfs_show,
		   wil_lo_power_calib_sysfs_store);

static ssize_t
snr_thresh_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	ssize_t len = 0;

	if (wil->snr_thresh.enabled)
		len = snprintf(buf, PAGE_SIZE, "omni=%d, direct=%d\n",
			       wil->snr_thresh.omni, wil->snr_thresh.direct);

	return len;
}

int wil_snr_thresh_set(struct wil6210_priv *wil, const char *buf)
{
	wil->snr_thresh.enabled = 0;
	/* to disable snr threshold, set both omni and direct to 0 */
	if (sscanf(buf, "%hd %hd", &wil->snr_thresh.omni,
		   &wil->snr_thresh.direct) != 2)
		return -EINVAL;

	if (wil->snr_thresh.omni != 0 || wil->snr_thresh.direct != 0)
		wil->snr_thresh.enabled = 1;

	return 0;
}

static ssize_t
snr_thresh_store(struct device *dev,
		 struct device_attribute *attr,
		 const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	int rc;

	rc = wil_snr_thresh_set(wil, buf);
	if (rc < 0)
		return rc;

	rc = wmi_set_snr_thresh(wil, wil->snr_thresh.omni,
				wil->snr_thresh.direct);
	if (!rc)
		rc = count;

	return rc;
}

static DEVICE_ATTR_RW(snr_thresh);

static ssize_t
vr_profile_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	ssize_t len;

	len = snprintf(buf, PAGE_SIZE, "%s\n",
		       wil_get_vr_profile_name(wil->vr_profile));

	return len;
}

static ssize_t
vr_profile_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	struct wil6210_priv *wil = dev_get_drvdata(dev);
	u8 profile;
	int rc = 0;

	if (kstrtou8(buf, 0, &profile))
		return -EINVAL;

	if (test_bit(wil_status_fwready, wil->status)) {
		wil_err(wil, "Cannot set VR while interface is up\n");
		return -EIO;
	}

	if (profile == wil->vr_profile) {
		wil_info(wil, "Ignore same VR profile %s\n",
			 wil_get_vr_profile_name(wil->vr_profile));
		return count;
	}

	wil_info(wil, "Sysfs: set VR profile to %s\n",
		 wil_get_vr_profile_name(profile));

	/* Enabling of VR mode is done from wil_reset after FW is ready.
	 * Disabling is done from here.
	 */
	if (profile == WMI_VR_PROFILE_DISABLED) {
		rc = wil_vr_update_profile(wil, profile);
		if (rc)
			return rc;
	}
	wil->vr_profile = profile;

	return count;
}

static DEVICE_ATTR_RW(vr_profile);

static struct attribute *wil6210_sysfs_entries[] = {
	&dev_attr_ftm_txrx_offset.attr,
	&dev_attr_thermal_throttling.attr,
	&dev_attr_board_file.attr,
	&dev_attr_fst_link_loss.attr,
	&dev_attr_qos_weights.attr,
	&dev_attr_qos_link_prio.attr,
	&dev_attr_lo_power_calib.attr,
	&dev_attr_snr_thresh.attr,
	&dev_attr_vr_profile.attr,
	NULL
};

static struct attribute_group wil6210_attribute_group = {
	.name = "wil6210",
	.attrs = wil6210_sysfs_entries,
};

int wil6210_sysfs_init(struct wil6210_priv *wil)
{
	struct device *dev = wil_to_dev(wil);
	int err;

	err = sysfs_create_group(&dev->kobj, &wil6210_attribute_group);
	if (err) {
		wil_err(wil, "failed to create sysfs group: %d\n", err);
		return err;
	}

	return 0;
}

void wil6210_sysfs_remove(struct wil6210_priv *wil)
{
	struct device *dev = wil_to_dev(wil);

	sysfs_remove_group(&dev->kobj, &wil6210_attribute_group);
}
