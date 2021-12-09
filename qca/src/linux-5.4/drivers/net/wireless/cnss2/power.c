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

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#ifdef CNSS2_CPR
#include <soc/qcom/cmd-db.h>
#endif

#include "main.h"
#include "debug.h"

#ifdef CNSS2_VREG
static struct cnss_vreg_cfg cnss_vreg_list[] = {
	{"vdd-wlan-core", 1300000, 1300000, 0, 0, 0},
	{"vdd-wlan-io", 1800000, 1800000, 0, 0, 0},
	{"vdd-wlan-xtal-aon", 0, 0, 0, 0, 0},
	{"vdd-wlan-xtal", 1800000, 1800000, 0, 2, 0},
	{"vdd-wlan", 0, 0, 0, 0, 0},
	{"vdd-wlan-ctrl1", 0, 0, 0, 0, 0},
	{"vdd-wlan-ctrl2", 0, 0, 0, 0, 0},
	{"vdd-wlan-sp2t", 2700000, 2700000, 0, 0, 0},
	{"wlan-ant-switch", 1800000, 1800000, 0, 0, 0},
	{"wlan-soc-swreg", 1200000, 1200000, 0, 0, 0},
	{"vdd-wlan-aon", 950000, 950000, 0, 0, 0},
	{"vdd-wlan-dig", 950000, 952000, 0, 0, 0},
	{"vdd-wlan-rfa1", 1900000, 1900000, 0, 0, 0},
	{"vdd-wlan-rfa2", 1350000, 1350000, 0, 0, 0},
	{"vdd-wlan-en", 0, 0, 0, 10, 0},
};

static struct cnss_clk_cfg cnss_clk_list[] = {
	{"rf_clk", 0, 0},
};

#define CNSS_VREG_INFO_SIZE		ARRAY_SIZE(cnss_vreg_list)
#define CNSS_CLK_INFO_SIZE		ARRAY_SIZE(cnss_clk_list)
#define MAX_PROP_SIZE			32
#endif

#define BOOTSTRAP_GPIO			"qcom,enable-bootstrap-gpio"
#define BOOTSTRAP_ACTIVE		"bootstrap_active"
#define WLAN_EN_GPIO			"wlan-en-gpio"
#define WLAN_EN_ACTIVE			"wlan_en_active"
#define WLAN_EN_SLEEP			"wlan_en_sleep"

#define BOOTSTRAP_DELAY			1000
#define WLAN_ENABLE_DELAY		1000

#define TCS_CMD_DATA_ADDR_OFFSET	0x4
#define TCS_OFFSET			0xC8
#define TCS_CMD_OFFSET			0x10
#define MAX_TCS_NUM			8
#define MAX_TCS_CMD_NUM			5
#define BT_CXMX_VOLTAGE_MV		950
#ifdef CNSS2_VREG
static int cnss_get_vreg_single(struct cnss_plat_data *plat_priv,
				struct cnss_vreg_info *vreg)
{
	int ret = 0;
	struct device *dev;
	struct regulator *reg;
	const __be32 *prop;
	char prop_name[MAX_PROP_SIZE] = {0};
	int len;

	dev = &plat_priv->plat_dev->dev;
	reg = devm_regulator_get_optional(dev, vreg->cfg.name);
	if (IS_ERR(reg)) {
		ret = PTR_ERR(reg);
		if (ret == -ENODEV)
			return ret;
		else if (ret == -EPROBE_DEFER)
			cnss_pr_info("EPROBE_DEFER for regulator: %s\n",
				     vreg->cfg.name);
		else
			cnss_pr_err("Failed to get regulator %s, err = %d\n",
				    vreg->cfg.name, ret);
		return ret;
	}

	vreg->reg = reg;

	snprintf(prop_name, MAX_PROP_SIZE, "qcom,%s-config",
		 vreg->cfg.name);

	prop = of_get_property(dev->of_node, prop_name, &len);
	if (!prop || len != (5 * sizeof(__be32))) {
		cnss_pr_dbg("Property %s %s, use default\n", prop_name,
			    prop ? "invalid format" : "doesn't exist");
	} else {
		vreg->cfg.min_uv = be32_to_cpup(&prop[0]);
		vreg->cfg.max_uv = be32_to_cpup(&prop[1]);
		vreg->cfg.load_ua = be32_to_cpup(&prop[2]);
		vreg->cfg.delay_us = be32_to_cpup(&prop[3]);
		vreg->cfg.need_unvote = be32_to_cpup(&prop[4]);
	}

	cnss_pr_dbg("Got regulator: %s, min_uv: %u, max_uv: %u, load_ua: %u, delay_us: %u, need_unvote: %u\n",
		    vreg->cfg.name, vreg->cfg.min_uv,
		    vreg->cfg.max_uv, vreg->cfg.load_ua,
		    vreg->cfg.delay_us, vreg->cfg.need_unvote);

	return 0;
}

static void cnss_put_vreg_single(struct cnss_plat_data *plat_priv,
				 struct cnss_vreg_info *vreg)
{
	struct device *dev = &plat_priv->plat_dev->dev;

	cnss_pr_dbg("Put regulator: %s\n", vreg->cfg.name);
	devm_regulator_put(vreg->reg);
	devm_kfree(dev, vreg);
}

static int cnss_vreg_on_single(struct cnss_vreg_info *vreg)
{
	int ret = 0;

	if (vreg->enabled) {
		cnss_pr_dbg("Regulator %s is already enabled\n",
			    vreg->cfg.name);
		return 0;
	}

	cnss_pr_dbg("Regulator %s is being enabled\n", vreg->cfg.name);

	if (vreg->cfg.min_uv != 0 && vreg->cfg.max_uv != 0) {
		ret = regulator_set_voltage(vreg->reg,
					    vreg->cfg.min_uv,
					    vreg->cfg.max_uv);

		if (ret) {
			cnss_pr_err("Failed to set voltage for regulator %s, min_uv: %u, max_uv: %u, err = %d\n",
				    vreg->cfg.name, vreg->cfg.min_uv,
				    vreg->cfg.max_uv, ret);
			goto out;
		}
	}

	if (vreg->cfg.load_ua) {
		ret = regulator_set_load(vreg->reg,
					 vreg->cfg.load_ua);

		if (ret < 0) {
			cnss_pr_err("Failed to set load for regulator %s, load: %u, err = %d\n",
				    vreg->cfg.name, vreg->cfg.load_ua,
				    ret);
			goto out;
		}
	}

	if (vreg->cfg.delay_us)
		udelay(vreg->cfg.delay_us);

	ret = regulator_enable(vreg->reg);
	if (ret) {
		cnss_pr_err("Failed to enable regulator %s, err = %d\n",
			    vreg->cfg.name, ret);
		goto out;
	}
	vreg->enabled = true;

out:
	return ret;
}

static int cnss_vreg_unvote_single(struct cnss_vreg_info *vreg)
{
	int ret = 0;

	if (!vreg->enabled) {
		cnss_pr_dbg("Regulator %s is already disabled\n",
			    vreg->cfg.name);
		return 0;
	}

	cnss_pr_dbg("Removing vote for Regulator %s\n", vreg->cfg.name);

	if (vreg->cfg.load_ua) {
		ret = regulator_set_load(vreg->reg, 0);
		if (ret < 0)
			cnss_pr_err("Failed to set load for regulator %s, err = %d\n",
				    vreg->cfg.name, ret);
	}

	if (vreg->cfg.min_uv != 0 && vreg->cfg.max_uv != 0) {
		ret = regulator_set_voltage(vreg->reg, 0,
					    vreg->cfg.max_uv);
		if (ret)
			cnss_pr_err("Failed to set voltage for regulator %s, err = %d\n",
				    vreg->cfg.name, ret);
	}

	return ret;
}

static int cnss_vreg_off_single(struct cnss_vreg_info *vreg)
{
	int ret = 0;

	if (!vreg->enabled) {
		cnss_pr_dbg("Regulator %s is already disabled\n",
			    vreg->cfg.name);
		return 0;
	}

	cnss_pr_dbg("Regulator %s is being disabled\n",
		    vreg->cfg.name);

	ret = regulator_disable(vreg->reg);
	if (ret)
		cnss_pr_err("Failed to disable regulator %s, err = %d\n",
			    vreg->cfg.name, ret);

	if (vreg->cfg.load_ua) {
		ret = regulator_set_load(vreg->reg, 0);
		if (ret < 0)
			cnss_pr_err("Failed to set load for regulator %s, err = %d\n",
				    vreg->cfg.name, ret);
	}

	if (vreg->cfg.min_uv != 0 && vreg->cfg.max_uv != 0) {
		ret = regulator_set_voltage(vreg->reg, 0,
					    vreg->cfg.max_uv);
		if (ret)
			cnss_pr_err("Failed to set voltage for regulator %s, err = %d\n",
				    vreg->cfg.name, ret);
	}
	vreg->enabled = false;

	return ret;
}

static struct cnss_vreg_cfg *get_vreg_list(u32 *vreg_list_size,
					   enum cnss_vreg_type type)
{
	switch (type) {
	case CNSS_VREG_PRIM:
		*vreg_list_size = CNSS_VREG_INFO_SIZE;
		return cnss_vreg_list;
	default:
		cnss_pr_err("Unsupported vreg type 0x%x\n", type);
		*vreg_list_size = 0;
		return NULL;
	}
}
int cnss_get_vreg(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	int i;
	struct cnss_vreg_info *vreg_info;
	struct device *dev;
	struct regulator *reg;
	const __be32 *prop;
	char prop_name[MAX_PROP_SIZE];
	int len;

	dev = &plat_priv->plat_dev->dev;

	plat_priv->vreg_info = devm_kzalloc(dev, sizeof(cnss_vreg_info),
					    GFP_KERNEL);
	if (!plat_priv->vreg_info) {
		ret = -ENOMEM;
		goto out;
	}

	memcpy(plat_priv->vreg_info, cnss_vreg_info, sizeof(cnss_vreg_info));

	for (i = 0; i < CNSS_VREG_INFO_SIZE; i++) {
		vreg_info = &plat_priv->vreg_info[i];
		reg = devm_regulator_get_optional(dev, vreg_info->name);
		if (IS_ERR(reg)) {
			ret = PTR_ERR(reg);
			if (ret == -ENODEV)
				continue;
			else if (ret == -EPROBE_DEFER)
				cnss_pr_info("EPROBE_DEFER for regulator: %s\n",
					     vreg_info->name);
			else
				cnss_pr_err("Failed to get regulator %s, err = %d\n",
					    vreg_info->name, ret);
			goto out;
		}

		vreg_info->reg = reg;

		snprintf(prop_name, MAX_PROP_SIZE, "qcom,%s-info",
			 vreg_info->name);

		prop = of_get_property(dev->of_node, prop_name, &len);
		cnss_pr_dbg("Got regulator info, name: %s, len: %d\n",
			    prop_name, len);

		if (!prop || len != (4 * sizeof(__be32))) {
			cnss_pr_dbg("Property %s %s, use default\n", prop_name,
				    prop ? "invalid format" : "doesn't exist");
		} else {
			vreg_info->min_uv = be32_to_cpup(&prop[0]);
			vreg_info->max_uv = be32_to_cpup(&prop[1]);
			vreg_info->load_ua = be32_to_cpup(&prop[2]);
			vreg_info->delay_us = be32_to_cpup(&prop[3]);
		}

		cnss_pr_dbg("Got regulator: %s, min_uv: %u, max_uv: %u, load_ua: %u, delay_us: %u\n",
			    vreg_info->name, vreg_info->min_uv,
			    vreg_info->max_uv, vreg_info->load_ua,
			    vreg_info->delay_us);
	}

	return 0;
out:
	return ret;
}
#endif

#ifdef CONFIG_CNSS2_PM
static int cnss_vreg_on(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct cnss_vreg_info *vreg_info;
	int i;

	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL!\n");
		return -ENODEV;
	}

	for (i = 0; i < CNSS_VREG_INFO_SIZE; i++) {
		vreg_info = &plat_priv->vreg_info[i];

		if (!vreg_info->reg)
			continue;

		cnss_pr_dbg("Regulator %s is being enabled\n", vreg_info->name);

		if (vreg_info->min_uv != 0 && vreg_info->max_uv != 0) {
			ret = regulator_set_voltage(vreg_info->reg,
						    vreg_info->min_uv,
						    vreg_info->max_uv);

			if (ret) {
				cnss_pr_err("Failed to set voltage for regulator %s, min_uv: %u, max_uv: %u, err = %d\n",
					    vreg_info->name, vreg_info->min_uv,
					    vreg_info->max_uv, ret);
				break;
			}
		}

		if (vreg_info->load_ua) {
			ret = regulator_set_load(vreg_info->reg,
							 vreg_info->load_ua);

			if (ret < 0) {
				cnss_pr_err("Failed to set load for regulator %s, load: %u, err = %d\n",
					    vreg_info->name, vreg_info->load_ua,
					    ret);
				break;
			}
		}

		if (vreg_info->delay_us)
			udelay(vreg_info->delay_us);

		ret = regulator_enable(vreg_info->reg);
		if (ret) {
			cnss_pr_err("Failed to enable regulator %s, err = %d\n",
				    vreg_info->name, ret);
			break;
		}
	}

	if (ret) {
		for (; i >= 0; i--) {
			vreg_info = &plat_priv->vreg_info[i];

			if (!vreg_info->reg)
				continue;

			regulator_disable(vreg_info->reg);
			if (vreg_info->load_ua)
				regulator_set_load(vreg_info->reg, 0);
			if (vreg_info->min_uv != 0 && vreg_info->max_uv != 0)
				regulator_set_voltage(vreg_info->reg, 0,
						      vreg_info->max_uv);
		}

		return ret;
	}

	return 0;
}

static int cnss_vreg_off(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct cnss_vreg_info *vreg_info;
	int i;

	if (!plat_priv) {
		pr_err("plat_priv is NULL!\n");
		return -ENODEV;
	}

	for (i = CNSS_VREG_INFO_SIZE - 1; i >= 0; i--) {
		vreg_info = &plat_priv->vreg_info[i];

		if (!vreg_info->reg)
			continue;

		cnss_pr_dbg("Regulator %s is being disabled\n",
			    vreg_info->name);

		ret = regulator_disable(vreg_info->reg);
		if (ret)
			cnss_pr_err("Failed to disable regulator %s, err = %d\n",
				    vreg_info->name, ret);

		if (vreg_info->load_ua) {
			ret = regulator_set_load(vreg_info->reg, 0);
			if (ret < 0)
				cnss_pr_err("Failed to set load for regulator %s, err = %d\n",
					    vreg_info->name, ret);
		}

		if (vreg_info->min_uv != 0 && vreg_info->max_uv != 0) {
			ret = regulator_set_voltage(vreg_info->reg, 0,
						    vreg_info->max_uv);
			if (ret)
				cnss_pr_err("Failed to set voltage for regulator %s, err = %d\n",
					    vreg_info->name, ret);
		}
	}

	return ret;
}
#endif

int cnss_get_pinctrl(struct cnss_plat_data *plat_priv)
{
	int ret = 0;
	struct device *dev;
	struct cnss_pinctrl_info *pinctrl_info;

	dev = &plat_priv->plat_dev->dev;
	pinctrl_info = &plat_priv->pinctrl_info;

	pinctrl_info->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pinctrl_info->pinctrl)) {
		ret = PTR_ERR(pinctrl_info->pinctrl);
		cnss_pr_err("Failed to get pinctrl, err = %d\n", ret);
		goto out;
	}


	if (of_find_property(dev->of_node, WLAN_EN_GPIO, NULL)) {
		pinctrl_info->wlan_en_active =
			pinctrl_lookup_state(pinctrl_info->pinctrl,
					     WLAN_EN_ACTIVE);
		if (IS_ERR_OR_NULL(pinctrl_info->wlan_en_active)) {
			ret = PTR_ERR(pinctrl_info->wlan_en_active);
			cnss_pr_err("Failed to get wlan_en active state, err = %d\n",
				    ret);
			goto out;
		}

		pinctrl_info->wlan_en_sleep =
			pinctrl_lookup_state(pinctrl_info->pinctrl,
					     WLAN_EN_SLEEP);
		if (IS_ERR_OR_NULL(pinctrl_info->wlan_en_sleep)) {
			ret = PTR_ERR(pinctrl_info->wlan_en_sleep);
			cnss_pr_err("Failed to get wlan_en sleep state, err = %d\n",
				    ret);
			goto out;
		}
	}

	return 0;
out:
	return ret;
}

#ifdef CONFIG_CNSS2_PM
static int cnss_select_pinctrl_state(struct cnss_plat_data *plat_priv,
				     bool state)
{
	int ret = 0;
	struct cnss_pinctrl_info *pinctrl_info;

	if (!plat_priv) {
		printk(KERN_ERR "plat_priv is NULL!\n");
		ret = -ENODEV;
		goto out;
	}

	pinctrl_info = &plat_priv->pinctrl_info;

	if (state) {

		if (!IS_ERR_OR_NULL(pinctrl_info->wlan_en_active)) {
			ret = pinctrl_select_state(pinctrl_info->pinctrl,
						   pinctrl_info->
						   wlan_en_active);
			if (ret) {
				cnss_pr_err("Failed to select wlan_en active state, err = %d\n",
					    ret);
				goto out;
			}
			udelay(WLAN_ENABLE_DELAY);
		}
	} else {
		if (!IS_ERR_OR_NULL(pinctrl_info->wlan_en_sleep)) {
			ret = pinctrl_select_state(pinctrl_info->pinctrl,
						   pinctrl_info->wlan_en_sleep);
			if (ret) {
				cnss_pr_err("Failed to select wlan_en sleep state, err = %d\n",
					    ret);
				goto out;
			}
		}
	}

	return 0;
out:
	return ret;
}
#endif

int cnss_power_on_device(struct cnss_plat_data *plat_priv, int device_id)
{
#ifdef CONFIG_CNSS2_PM
	int ret;

	if (!plat_priv) {
		plat_priv = cnss_get_plat_priv_by_device_id(device_id);
		if (!plat_priv) {
			pr_err("%s: plat_priv is NULL: device id 0x%x\n",
			       __func__, device_id);
			return -ENODEV;
		}
	}

	ret = cnss_select_pinctrl_state(plat_priv, true);
	if (ret) {
		cnss_pr_err("Failed to select pinctrl state, err = %d\n", ret);
	}

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(cnss_power_on_device);

int cnss_power_off_device(struct cnss_plat_data *plat_priv, int device_id)
{
#ifdef CONFIG_CNSS2_PM
	int ret;

	if (!plat_priv) {
		plat_priv = cnss_get_plat_priv_by_device_id(device_id);
		if (!plat_priv) {
			pr_err("%s: plat_priv is NULL: device id 0x%x\n",
			       __func__, device_id);
			return -ENODEV;
		}
	}

	ret = cnss_select_pinctrl_state(plat_priv, false);
	if (ret)
		cnss_pr_err("Failed to select pinctrl state, err = %d\n", ret);

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(cnss_power_off_device);

void cnss_set_pin_connect_status(struct cnss_plat_data *plat_priv)
{
	unsigned long pin_status = 0;

	set_bit(CNSS_WLAN_EN, &pin_status);
	set_bit(CNSS_PCIE_TXN, &pin_status);
	set_bit(CNSS_PCIE_TXP, &pin_status);
	set_bit(CNSS_PCIE_RXN, &pin_status);
	set_bit(CNSS_PCIE_RXP, &pin_status);
	set_bit(CNSS_PCIE_REFCLKN, &pin_status);
	set_bit(CNSS_PCIE_REFCLKP, &pin_status);
	set_bit(CNSS_PCIE_RST, &pin_status);

	plat_priv->pin_result.host_pin_result = pin_status;
}

int cnss_get_cpr_info(struct cnss_plat_data *plat_priv)
{
#ifdef CNSS2_CPR
	struct platform_device *plat_dev = plat_priv->plat_dev;
	struct cnss_cpr_info *cpr_info = &plat_priv->cpr_info;
	struct resource *res;
	resource_size_t addr_len;
	void __iomem *tcs_cmd_base_addr;
	const char *cmd_db_name;
	u32 cpr_pmic_addr = 0;
	int ret = 0;

	res = platform_get_resource_byname(plat_dev, IORESOURCE_MEM, "tcs_cmd");
	if (!res) {
		cnss_pr_dbg("TCS CMD address is not present for CPR\n");
		goto out;
	}

	ret = of_property_read_string(plat_dev->dev.of_node,
				      "qcom,cmd_db_name", &cmd_db_name);
	if (ret) {
		cnss_pr_dbg("CommandDB name is not present for CPR\n");
		goto out;
	}

	ret = cmd_db_ready();
	if (ret) {
		cnss_pr_err("CommandDB is not ready\n");
		goto out;
	}

	cpr_pmic_addr = cmd_db_read_addr(cmd_db_name);
	if (cpr_pmic_addr > 0) {
		cpr_info->cpr_pmic_addr = cpr_pmic_addr;
		cnss_pr_dbg("Get CPR PMIC address 0x%x from %s\n",
			    cpr_info->cpr_pmic_addr, cmd_db_name);
	} else {
		cnss_pr_err("CPR PMIC address is not available for %s\n",
			    cmd_db_name);
		ret = -EINVAL;
		goto out;
	}

	cpr_info->tcs_cmd_base_addr = res->start;
	addr_len = resource_size(res);
	cnss_pr_dbg("TCS CMD base address is %pa with length %pa\n",
		    &cpr_info->tcs_cmd_base_addr, &addr_len);

	tcs_cmd_base_addr = devm_ioremap_resource(&plat_dev->dev, res);
	if (IS_ERR(tcs_cmd_base_addr)) {
		ret = PTR_ERR(tcs_cmd_base_addr);
		cnss_pr_err("Failed to map TCS CMD address, err = %d\n",
			    ret);
		goto out;
	}

	cpr_info->tcs_cmd_base_addr_io = tcs_cmd_base_addr;

	return 0;

out:
	return ret;
#endif
	return 0;
}

int cnss_update_cpr_info(struct cnss_plat_data *plat_priv)
{
#ifdef CNSS2_CPR
	struct cnss_cpr_info *cpr_info = &plat_priv->cpr_info;
	u32 pmic_addr, voltage = 0, voltage_tmp, offset;
	void __iomem *tcs_cmd_addr, *tcs_cmd_data_addr;
	int i, j;

	if (cpr_info->tcs_cmd_base_addr == 0) {
		cnss_pr_dbg("CPR is not enabled\n");
		return 0;
	}

	if (cpr_info->voltage == 0 || cpr_info->cpr_pmic_addr == 0) {
		cnss_pr_err("Voltage %dmV or PMIC address 0x%x is not valid\n",
			    cpr_info->voltage, cpr_info->cpr_pmic_addr);
		return -EINVAL;
	}

	if (cpr_info->tcs_cmd_data_addr_io)
		goto update_cpr;

	for (i = 0; i < MAX_TCS_NUM; i++) {
		for (j = 0; j < MAX_TCS_CMD_NUM; j++) {
			offset = i * TCS_OFFSET + j * TCS_CMD_OFFSET;
			tcs_cmd_addr = cpr_info->tcs_cmd_base_addr_io + offset;
			pmic_addr = readl_relaxed(tcs_cmd_addr);
			if (pmic_addr == cpr_info->cpr_pmic_addr) {
				tcs_cmd_data_addr = tcs_cmd_addr +
					TCS_CMD_DATA_ADDR_OFFSET;
				voltage_tmp = readl_relaxed(tcs_cmd_data_addr);
				cnss_pr_dbg("Got voltage %dmV from i: %d, j: %d\n",
					    voltage_tmp, i, j);

				if (voltage_tmp > voltage) {
					voltage = voltage_tmp;
					cpr_info->tcs_cmd_data_addr =
						cpr_info->tcs_cmd_base_addr +
						offset +
						TCS_CMD_DATA_ADDR_OFFSET;
					cpr_info->tcs_cmd_data_addr_io =
						tcs_cmd_data_addr;
				}
			}
		}
	}

	if (!cpr_info->tcs_cmd_data_addr_io) {
		cnss_pr_err("Failed to find proper TCS CMD data address\n");
		return -EINVAL;
	}

update_cpr:
	cpr_info->voltage = cpr_info->voltage > BT_CXMX_VOLTAGE_MV ?
		cpr_info->voltage : BT_CXMX_VOLTAGE_MV;
	cnss_pr_dbg("Update TCS CMD data address %pa with voltage %dmV\n",
		    &cpr_info->tcs_cmd_data_addr, cpr_info->voltage);
	writel_relaxed(cpr_info->voltage, cpr_info->tcs_cmd_data_addr_io);
#endif
	return 0;
}
