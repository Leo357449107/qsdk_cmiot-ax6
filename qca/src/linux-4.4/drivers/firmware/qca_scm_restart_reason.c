/* Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
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
 *
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <soc/qcom/rpm-glink.h>
#include <asm/system_misc.h>
#include "qcom_scm.h"

static int dload_dis;
static int enable_coldboot;
static void __iomem *restart_settings_regs;

#define WARM_RST_OFFSET 0x10
#define RPM_RESET_RESOURCE	0x747372	/*'rst' in little endian format*/
#define RPM_COLD_RESET_KEY	0x646c6f63	/*'cold' in little endian format*/

static void scm_restart_dload_mode_enable(void)
{
	if (!dload_dis) {
		unsigned int magic_cookie = SET_MAGIC;
		qcom_scm_dload(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_FORCE_DLOAD_ID,
				&magic_cookie);
	}
}

static void scm_restart_reason_rpm_cold_restart (enum reboot_mode reboot_mode,
			const char *cmd)
{
	struct msm_rpm_request *rpm_req;
	uint32_t key, resource, value, msg_id;
	int ret;

	/*'rst' in little endian format*/
	resource = RPM_RESET_RESOURCE;
	rpm_req = msm_rpm_create_request(MSM_RPM_CTX_ACTIVE_SET,
			resource, 0, 1);
	if (IS_ERR_OR_NULL(rpm_req)) {
		pr_err("%s RPM create request failed %p\n", __func__, rpm_req);
		return;
	}

	/*'cold' as we are sending cold reset req*/
	key = RPM_COLD_RESET_KEY;
	value = 1;
	ret = msm_rpm_add_kvp_data(rpm_req, key, (const uint8_t *)&value,
							(int)4);
	if (ret) {
		pr_err("%s unable to add kvp data for device shutdown\n",
				__func__);
		return;
	}

	msg_id = msm_rpm_send_request(rpm_req);
	if(!msg_id) {
		pr_err("%s unable to send rpm message\n", __func__);
		return;
	}

	ret = msm_rpm_wait_for_ack(msg_id);
	if (ret)
		pr_err("%s wait for rpm ack failed %d\n", __func__, ret);

	/* Give a grace period for failure to restart of 1s */
	mdelay(1000);
}

static void scm_restart_dload_mode_disable(void)
{
	unsigned int magic_cookie = CLEAR_MAGIC;

	qcom_scm_dload(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_FORCE_DLOAD_ID,
			&magic_cookie);
};

static void scm_restart_sdi_disable(void)
{
	qcom_scm_sdi(QCOM_SCM_SVC_BOOT, SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID);
}

static int scm_restart_panic(struct notifier_block *this,
	unsigned long event, void *data)
{
	scm_restart_dload_mode_enable();
	scm_restart_sdi_disable();

	return NOTIFY_DONE;
}

static struct notifier_block panic_nb = {
	.notifier_call = scm_restart_panic,
};

static int scm_restart_reason_reboot(struct notifier_block *nb,
				unsigned long action, void *data)
{
	scm_restart_sdi_disable();
	scm_restart_dload_mode_disable();

	if (enable_coldboot) {
		if (IS_ERR_OR_NULL(restart_settings_regs))
			pr_err("%s unable to get tcsr regs\n", __func__);
		else if (!readl(restart_settings_regs + WARM_RST_OFFSET))
			arm_pm_restart = scm_restart_reason_rpm_cold_restart;
	}

	return NOTIFY_DONE;
}

static struct notifier_block reboot_nb = {
	.notifier_call = scm_restart_reason_reboot,
	.priority = INT_MIN,
};

static const struct of_device_id scm_restart_reason_match_table[] = {
	{ .compatible = "qca,scm_restart_reason",
	  .data = (void *)CLEAR_MAGIC,
	},
	{ .compatible = "qca_ipq6018,scm_restart_reason",
	  .data = (void *)ABNORMAL_MAGIC,
	},
	{}
};
MODULE_DEVICE_TABLE(of, scm_restart_reason_match_table);

static int scm_restart_reason_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	int ret, dload_dis_sec;
	struct device_node *np;
	struct resource *resource;
	unsigned int magic_cookie = SET_MAGIC_WARMRESET;
	unsigned int dload_warm_reset = 0;

	np = of_node_get(pdev->dev.of_node);
	if (!np)
		return 0;

	id = of_match_device(scm_restart_reason_match_table, &pdev->dev);
	if (!id)
		return -ENODEV;

	ret = of_property_read_u32(np, "dload_status", &dload_dis);
	if (ret)
		dload_dis = 0;

	ret = of_property_read_u32(np, "dload_warm_reset", &dload_warm_reset);
	if (ret)
		dload_warm_reset = 0;

	ret = of_property_read_u32(np, "dload_sec_status", &dload_dis_sec);
	if (ret)
		dload_dis_sec = 0;

	if (!of_compat_cmp(id->compatible, "qca_ipq6018,scm_restart_reason",
			   strlen(id->compatible))) {
		enable_coldboot = of_property_read_bool(np, "qca,coldreboot-enabled");
		if (enable_coldboot) {
			resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
			restart_settings_regs =
				devm_ioremap_resource(&pdev->dev, resource);
		}
	}

	if (dload_dis_sec) {
		qcom_scm_dload(QCOM_SCM_SVC_BOOT,
			SCM_CMD_TZ_SET_DLOAD_FOR_SECURE_BOOT, NULL);
	}

	/* Ensure Disable before enabling the dload and sdi bits
	 * to make sure they are disabled during boot */
	if (dload_dis) {
		scm_restart_sdi_disable();
		if (!dload_warm_reset)
			magic_cookie = (uintptr_t)id->data;
		qcom_scm_dload(QCOM_SCM_SVC_BOOT,
			       SCM_CMD_TZ_FORCE_DLOAD_ID,
			       &magic_cookie);
	} else {
		scm_restart_dload_mode_enable();
	}

	ret = atomic_notifier_chain_register(&panic_notifier_list, &panic_nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to setup download mode\n");
		goto fail;
	}

	ret = register_reboot_notifier(&reboot_nb);
	if (ret) {
		dev_err(&pdev->dev, "failed to setup reboot handler\n");
		atomic_notifier_chain_unregister(&panic_notifier_list,
								&panic_nb);
		goto fail;
	}

fail:
	return ret;
}

static int scm_restart_reason_remove(struct platform_device *pdev)
{
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_nb);
	unregister_reboot_notifier(&reboot_nb);
	return 0;
}

static struct platform_driver scm_restart_reason_driver = {
	.probe      = scm_restart_reason_probe,
	.remove     = scm_restart_reason_remove,
	.driver     = {
		.name = "qca_scm_restart_reason",
		.of_match_table = scm_restart_reason_match_table,
	},
};

module_platform_driver(scm_restart_reason_driver);

MODULE_DESCRIPTION("QCA SCM Restart Reason Driver");
