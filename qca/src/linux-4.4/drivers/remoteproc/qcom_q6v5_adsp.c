// SPDX-License-Identifier: GPL-2.0
/*
 * Qualcomm Technology Inc. ADSP Peripheral Image Loader for SDM845.
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_domain.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>
#include <linux/reset.h>
#include <linux/qcom_scm.h>
#include <linux/soc/qcom/mdt_loader.h>
#include <linux/soc/qcom/smem.h>
#include <linux/soc/qcom/smem_state.h>

#include "qcom_common.h"
#include "qcom_q6v5.h"
#include "remoteproc_internal.h"

/* time out value */
#define ACK_TIMEOUT			1000
#define BOOT_FSM_TIMEOUT		10000
/* mask values */
#define EVB_MASK			GENMASK(27, 4)
/*QDSP6SS register offsets*/
#define RST_EVB_REG			0x10
#define CORE_START_REG			0x400
#define BOOT_CMD_REG			0x404
#define BOOT_STATUS_REG			0x408
#define RET_CFG_REG			0x1C
/*TCSR register offsets*/
#define LPASS_MASTER_IDLE_REG		0x8
#define LPASS_HALTACK_REG		0x4
#define LPASS_PWR_ON_REG		0x10
#define LPASS_HALTREQ_REG		0x0

#define ADSP_PAS_ID		1
#define ADSP_RESET_CMD_ID	0x18
#define DEBUG_ADSP_BREAK_AT_START	1

static int debug_adsp;
/* list of clocks required by ADSP PIL */
static const char * const adsp_clk_id[] = {
	"sway_cbcr", "lpass_aon", "lpass_ahbs_aon_cbcr", "lpass_ahbm_aon_cbcr",
	"qdsp6ss_xo", "qdsp6ss_sleep", "qdsp6ss_core",
};

struct qcom_adsp {
	struct device *dev;
	struct rproc *rproc;

	struct qcom_q6v5 q6v5;

	struct clk *xo;

	int num_clks;
	struct clk *clks[ARRAY_SIZE(adsp_clk_id)];

	void __iomem *qdsp6ss_base;

	struct reset_control *pdc_sync_reset;
	struct reset_control *cc_lpass_restart;

	struct regmap *halt_map;
	unsigned int halt_lpass;

	int crash_reason_smem;

	struct completion start_done;
	struct completion stop_done;

	phys_addr_t mem_phys;
	phys_addr_t mem_reloc;
	void *mem_region;
	size_t mem_size;

	struct qcom_rproc_glink glink_subdev;
	struct qcom_rproc_ssr ssr_subdev;
	struct qcom_sysmon *sysmon;
};

struct adsp_pil_data {
	int crash_reason_smem;
	const char *firmware_name;

	const char *ssr_name;
	const char *sysmon_name;
	int ssctl_id;
	int (*init_clock)(struct qcom_adsp *);
	int (*init_reset)(struct qcom_adsp *);
	int (*init_mmio)(struct qcom_adsp *, struct platform_device *);
	void (*handover)(struct qcom_q6v5 *);
	const struct rproc_ops *adsp_ops;
};

#ifdef CONFIG_CNSS2
static int start_q6(const struct subsys_desc *subsys)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);
	struct rproc *rproc = q6v5->rproc;
	int ret = 0;

	ret = rproc_boot(rproc);
	if (ret)
		pr_err("couldn't boot q6v5: %d\n", ret);
	else
		q6v5->running = true;

	return ret;
}

static int stop_q6(const struct subsys_desc *subsys, bool force_stop)
{
	struct qcom_q6v5 *q6v5 = subsys_to_pdata(subsys);
	struct rproc *rproc = q6v5->rproc;
	int ret = 0;

	rproc_shutdown(rproc);

	q6v5->running = false;
	return ret;
}
#endif

static void adsp_clk_bulk_disable_unprepare(int count, struct qcom_adsp *adsp)
{
	int i;

	for (i = 0; i < count; i++) {
		clk_disable_unprepare(adsp->clks[i]);
		devm_clk_put(adsp->dev, adsp->clks[i]);
	}
}

static int adsp_clk_bulk_prepare_enable(int count, struct qcom_adsp *adsp)
{
	int i, ret;

	for (i = 0; i < count; i++) {
		adsp->clks[i] = devm_clk_get(adsp->dev, adsp_clk_id[i]);
		if (IS_ERR(adsp->clks[i])) {
			pr_err("%s unable to get clk %s\n",
					__func__, adsp_clk_id[i]);
			ret = PTR_ERR(adsp->clks[i]);
			goto disable_clk;
		}
		ret = clk_prepare_enable(adsp->clks[i]);
		if (ret) {
			pr_err("%s unable to enable clk %s\n",
					__func__, adsp_clk_id[i]);
			goto put_disable_clk;
		}
	}

	return 0;

put_disable_clk:
	devm_clk_put(adsp->dev, adsp->clks[i]);
disable_clk:
	adsp_clk_bulk_disable_unprepare(i, adsp);
	return ret;
}

static int qcom_adsp_shutdown(struct qcom_adsp *adsp)
{
	unsigned long timeout;
	unsigned int val;
	int ret;

	/* Reset the retention logic */
	val = readl(adsp->qdsp6ss_base + RET_CFG_REG);
	val |= 0x1;
	writel(val, adsp->qdsp6ss_base + RET_CFG_REG);

	adsp_clk_bulk_disable_unprepare(adsp->num_clks, adsp);

	/* QDSP6 master port needs to be explicitly halted */
	ret = regmap_read(adsp->halt_map,
			adsp->halt_lpass + LPASS_PWR_ON_REG, &val);
	if (ret || !val)
		goto reset;

	ret = regmap_read(adsp->halt_map,
			adsp->halt_lpass + LPASS_MASTER_IDLE_REG,
			&val);
	if (ret || val)
		goto reset;

	regmap_write(adsp->halt_map,
			adsp->halt_lpass + LPASS_HALTREQ_REG, 1);

	/* Wait for halt ACK from QDSP6 */
	timeout = jiffies + msecs_to_jiffies(ACK_TIMEOUT);
	for (;;) {
		ret = regmap_read(adsp->halt_map,
			adsp->halt_lpass + LPASS_HALTACK_REG, &val);
		if (ret || val || time_after(jiffies, timeout))
			break;

		usleep_range(1000, 1100);
	}

	ret = regmap_read(adsp->halt_map,
			adsp->halt_lpass + LPASS_MASTER_IDLE_REG, &val);
	if (ret || !val)
		dev_err(adsp->dev, "port failed halt\n");

reset:
	/* Assert the LPASS PDC Reset */
	reset_control_assert(adsp->pdc_sync_reset);
	/* Place the LPASS processor into reset */
	reset_control_assert(adsp->cc_lpass_restart);
	/* wait after asserting subsystem restart from AOSS */
	usleep_range(200, 300);

	/* Clear the halt request for the AXIM and AHBM for Q6 */
	regmap_write(adsp->halt_map, adsp->halt_lpass + LPASS_HALTREQ_REG, 0);

	/* De-assert the LPASS PDC Reset */
	reset_control_deassert(adsp->pdc_sync_reset);
	/* Remove the LPASS reset */
	reset_control_deassert(adsp->cc_lpass_restart);
	/* wait after de-asserting subsystem restart from AOSS */
	usleep_range(200, 300);

	return 0;
}

static int ipq6018_adsp_load(struct rproc *rproc, const struct firmware *fw)
{
	struct qcom_adsp *adsp = (struct qcom_adsp *)rproc->priv;
	int ret;

	ret = qcom_scm_pas_init_image(ADSP_PAS_ID, fw->data, fw->size);
	if (ret) {
		dev_err(adsp->dev, "image auth failed\n");
		return ret;
	}

	return qcom_mdt_load_no_init(adsp->dev, fw, rproc->firmware, 0,
			     adsp->mem_region, adsp->mem_phys, adsp->mem_size,
			     &adsp->mem_reloc);
}

static int adsp_load(struct rproc *rproc, const struct firmware *fw)
{
	struct qcom_adsp *adsp = (struct qcom_adsp *)rproc->priv;

	return qcom_mdt_load_no_init(adsp->dev, fw, rproc->firmware, 0,
			     adsp->mem_region, adsp->mem_phys, adsp->mem_size,
			     &adsp->mem_reloc);
}

static int ipq6018_adsp_start(struct rproc *rproc)
{
	struct qcom_adsp *adsp = (struct qcom_adsp *)rproc->priv;
	int ret;

	ret = qcom_scm_pas_auth_and_reset(ADSP_PAS_ID,
		(debug_adsp & DEBUG_ADSP_BREAK_AT_START),
			ADSP_RESET_CMD_ID);
	if (ret) {
		dev_err(adsp->dev, "q6-adsp reset failed\n");
		return ret;
	}

	ret = qcom_q6v5_wait_for_start(&adsp->q6v5, msecs_to_jiffies(5 * HZ));
	if (ret == -ETIMEDOUT) {
		dev_err(adsp->dev, "start timed out\n");
		return ret;
	}
	return 0;
}

static int adsp_start(struct rproc *rproc)
{
	struct qcom_adsp *adsp = (struct qcom_adsp *)rproc->priv;
	int ret;
	unsigned int val;

	qcom_q6v5_prepare(&adsp->q6v5);

	ret = clk_prepare_enable(adsp->xo);
	if (ret)
		goto disable_irqs;

	ret = pm_runtime_get_sync(adsp->dev);
	if (ret)
		goto disable_xo_clk;

	ret = adsp_clk_bulk_prepare_enable(adsp->num_clks, adsp);
	if (ret) {
		dev_err(adsp->dev, "adsp clk_enable failed\n");
		goto disable_power_domain;
	}

	/* Program boot address */
	writel(adsp->mem_phys >> 4, adsp->qdsp6ss_base + RST_EVB_REG);

	/* De-assert QDSP6 stop core. QDSP6 will execute after out of reset */
	writel(0x1, adsp->qdsp6ss_base + CORE_START_REG);

	/* Trigger boot FSM to start QDSP6 */
	writel(0x1, adsp->qdsp6ss_base + BOOT_CMD_REG);

	/* Wait for core to come out of reset */
	ret = readl_poll_timeout(adsp->qdsp6ss_base + BOOT_STATUS_REG,
			val, (val & BIT(0)) != 0, 10, BOOT_FSM_TIMEOUT);
	if (ret) {
		dev_err(adsp->dev, "failed to bootup adsp\n");
		goto disable_adsp_clks;
	}

	ret = qcom_q6v5_wait_for_start(&adsp->q6v5, msecs_to_jiffies(5 * HZ));
	if (ret == -ETIMEDOUT) {
		dev_err(adsp->dev, "start timed out\n");
		goto disable_adsp_clks;
	}

	return 0;

disable_adsp_clks:
	adsp_clk_bulk_disable_unprepare(adsp->num_clks, adsp);
disable_power_domain:
	pm_runtime_put(adsp->dev);
disable_xo_clk:
	clk_disable_unprepare(adsp->xo);
disable_irqs:
	qcom_q6v5_unprepare(&adsp->q6v5);

	return ret;
}

static void qcom_adsp_pil_handover(struct qcom_q6v5 *q6v5)
{
	struct qcom_adsp *adsp = container_of(q6v5, struct qcom_adsp, q6v5);

	clk_disable_unprepare(adsp->xo);
	pm_runtime_put(adsp->dev);
}

static int ipq6018_adsp_stop(struct rproc *rproc)
{
	struct qcom_adsp *adsp = (struct qcom_adsp *)rproc->priv;
	int ret;

	ret = qcom_q6v5_request_stop(&adsp->q6v5);
	if (ret == -ETIMEDOUT)
		dev_err(adsp->dev, "timed out on wait\n");

	ret = qcom_scm_pas_shutdown(ADSP_PAS_ID);
	if (ret)
		dev_err(adsp->dev, "failed to shutdown adsp%d\n", ret);

	return ret;
}

static int adsp_stop(struct rproc *rproc)
{
	struct qcom_adsp *adsp = (struct qcom_adsp *)rproc->priv;
	int handover;
	int ret;

	ret = qcom_q6v5_request_stop(&adsp->q6v5);
	if (ret == -ETIMEDOUT)
		dev_err(adsp->dev, "timed out on wait\n");

	ret = qcom_adsp_shutdown(adsp);
	if (ret)
		dev_err(adsp->dev, "failed to shutdown: %d\n", ret);

	handover = qcom_q6v5_unprepare(&adsp->q6v5);
	if (handover)
		qcom_adsp_pil_handover(&adsp->q6v5);

	return ret;
}

static void *adsp_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct qcom_adsp *adsp = (struct qcom_adsp *)rproc->priv;
	int offset;

	offset = da - adsp->mem_reloc;
	if (offset < 0 || offset + len > adsp->mem_size)
		return NULL;

	return adsp->mem_region + offset;
}

static const struct rproc_ops adsp_ops = {
	.start = adsp_start,
	.stop = adsp_stop,
	.da_to_va = adsp_da_to_va,
	.parse_fw = qcom_register_dump_segments,
	.load = adsp_load,
};

static const struct rproc_ops ipq6018_adsp_ops = {
	.start = ipq6018_adsp_start,
	.stop = ipq6018_adsp_stop,
	.da_to_va = adsp_da_to_va,
	.parse_fw = qcom_register_dump_segments,
	.load = ipq6018_adsp_load,
};

static int adsp_init_clock(struct qcom_adsp *adsp)
{
	int ret;

	adsp->xo = devm_clk_get(adsp->dev, "xo");
	if (IS_ERR(adsp->xo)) {
		ret = PTR_ERR(adsp->xo);
		if (ret != -EPROBE_DEFER)
			dev_err(adsp->dev, "failed to get xo clock");
		return ret;
	}

	adsp->num_clks = ARRAY_SIZE(adsp_clk_id);

	return 0;
}

static int adsp_init_reset(struct qcom_adsp *adsp)
{
	adsp->pdc_sync_reset = devm_reset_control_get(adsp->dev,
			"pdc_sync");
	if (IS_ERR(adsp->pdc_sync_reset)) {
		dev_err(adsp->dev, "failed to acquire pdc_sync reset\n");
		return PTR_ERR(adsp->pdc_sync_reset);
	}

	adsp->cc_lpass_restart = devm_reset_control_get(adsp->dev,
			"cc_lpass");
	if (IS_ERR(adsp->cc_lpass_restart)) {
		dev_err(adsp->dev, "failed to acquire cc_lpass restart\n");
		return PTR_ERR(adsp->cc_lpass_restart);
	}

	return 0;
}

static int adsp_init_mmio(struct qcom_adsp *adsp,
				struct platform_device *pdev)
{
	struct device_node *syscon;
	struct resource *res;
	int ret;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR_OR_NULL(res)) {
		dev_err(adsp->dev, "memory resource not available\n");
		return -EINVAL;
	}

	adsp->qdsp6ss_base = devm_ioremap(&pdev->dev, res->start,
						resource_size(res));
	if (IS_ERR(adsp->qdsp6ss_base)) {
		dev_err(adsp->dev, "failed to map QDSP6SS registers\n");
		return PTR_ERR(adsp->qdsp6ss_base);
	}

	syscon = of_parse_phandle(pdev->dev.of_node, "qcom,halt-regs", 0);
	if (!syscon) {
		dev_err(&pdev->dev, "failed to parse qcom,halt-regs\n");
		return -EINVAL;
	}

	adsp->halt_map = syscon_node_to_regmap(syscon);
	of_node_put(syscon);
	if (IS_ERR(adsp->halt_map))
		return PTR_ERR(adsp->halt_map);

	ret = of_property_read_u32_index(pdev->dev.of_node, "qcom,halt-regs",
			1, &adsp->halt_lpass);
	if (ret < 0) {
		dev_err(&pdev->dev, "no offset in syscon\n");
		return ret;
	}

	return 0;
}

static int adsp_alloc_memory_region(struct qcom_adsp *adsp)
{
	struct device_node *node;
	struct resource r;
	int ret;

	node = of_parse_phandle(adsp->dev->of_node, "memory-region", 0);
	if (!node) {
		dev_err(adsp->dev, "no memory-region specified\n");
		return -EINVAL;
	}

	ret = of_address_to_resource(node, 0, &r);
	if (ret)
		return ret;

	adsp->mem_phys = adsp->mem_reloc = r.start;
	adsp->mem_size = resource_size(&r);
	adsp->mem_region = devm_ioremap_wc(adsp->dev,
				adsp->mem_phys, adsp->mem_size);
	if (!adsp->mem_region) {
		dev_err(adsp->dev, "unable to map memory region: %pa+%zx\n",
			&r.start, adsp->mem_size);
		return -EBUSY;
	}

	return 0;
}

static int adsp_probe(struct platform_device *pdev)
{
	const struct adsp_pil_data *desc;
	struct qcom_adsp *adsp;
	struct qcom_q6v5 *q6v5;
	struct rproc *rproc;
	int ret;

	desc = of_device_get_match_data(&pdev->dev);
	if (!desc)
		return -EINVAL;

	rproc = rproc_alloc(&pdev->dev, pdev->name, desc->adsp_ops,
			    desc->firmware_name, sizeof(*adsp));
	if (!rproc) {
		dev_err(&pdev->dev, "unable to allocate remoteproc\n");
		return -ENOMEM;
	}

	adsp = (struct qcom_adsp *)rproc->priv;
	adsp->dev = &pdev->dev;
	adsp->rproc = rproc;
	platform_set_drvdata(pdev, adsp);

	ret = adsp_alloc_memory_region(adsp);
	if (ret)
		goto free_rproc;

	if (desc->init_clock) {
		ret = desc->init_clock(adsp);
		if (ret)
			goto free_rproc;
	}

	pm_runtime_enable(adsp->dev);

	if (desc->init_reset) {
		ret = desc->init_reset(adsp);
		if (ret)
			goto disable_pm;
	}

	if (desc->init_mmio) {
		ret = desc->init_mmio(adsp, pdev);
		if (ret)
			goto disable_pm;
	}

	q6v5 = &adsp->q6v5;
	ret = qcom_q6v5_init(q6v5, pdev, rproc, desc->crash_reason_smem,
			     desc->handover);
	if (ret)
		goto disable_pm;

	qcom_add_glink_subdev(rproc, &adsp->glink_subdev);
	qcom_add_ssr_subdev(rproc, &adsp->ssr_subdev, desc->ssr_name);

	if (desc->ssctl_id)
		adsp->sysmon = qcom_add_sysmon_subdev(rproc,
						      desc->sysmon_name,
						      desc->ssctl_id);
#ifdef CONFIG_CNSS2
	/*
	 * subsys-register
	 */
	q6v5->subsys_desc.is_not_loadable = 0;
	q6v5->subsys_desc.name = "q6v6-adsp";
	q6v5->subsys_desc.dev = &pdev->dev;
	q6v5->subsys_desc.owner = THIS_MODULE;
	q6v5->subsys_desc.shutdown = stop_q6;
	q6v5->subsys_desc.powerup = start_q6;
	q6v5->subsys_desc.ramdump = NULL;
	q6v5->subsys_desc.err_fatal_handler = q6v5_fatal_interrupt;
	q6v5->subsys_desc.stop_ack_handler = q6v5_ready_interrupt;
	q6v5->subsys_desc.wdog_bite_handler = q6v5_wdog_interrupt;

	q6v5->subsys = subsys_register(&q6v5->subsys_desc);
	if (IS_ERR(q6v5->subsys)) {
		dev_err(&pdev->dev, "failed to register with ssr\n");
		ret = PTR_ERR(q6v5->subsys);
		goto free_rproc;
	}
	dev_info(adsp->dev, "ssr registeration success %s\n",
					q6v5->subsys_desc.name);
#endif
	rproc->auto_boot = false;
	ret = rproc_add(rproc);
	if (ret)
		goto disable_pm;

	return 0;

disable_pm:
	pm_runtime_disable(adsp->dev);
free_rproc:
	rproc_free(rproc);

	return ret;
}

static int adsp_remove(struct platform_device *pdev)
{
	struct qcom_adsp *adsp = platform_get_drvdata(pdev);

#ifdef CONFIG_CNSS2
	subsys_unregister(adsp->q6v5.subsys);
#endif
	rproc_del(adsp->rproc);

	qcom_remove_glink_subdev(adsp->rproc, &adsp->glink_subdev);
	qcom_remove_sysmon_subdev(adsp->sysmon);
	qcom_remove_ssr_subdev(adsp->rproc, &adsp->ssr_subdev);
	pm_runtime_disable(adsp->dev);
	rproc_free(adsp->rproc);

	return 0;
}

static const struct adsp_pil_data sdm845_adsp_resource_init = {
	.crash_reason_smem = 423,
	.firmware_name = "adsp.mdt",
	.ssr_name = "lpass",
	.sysmon_name = "adsp",
	.ssctl_id = 0x14,
	.init_clock = adsp_init_clock,
	.init_reset = adsp_init_reset,
	.init_mmio = adsp_init_mmio,
	.handover = qcom_adsp_pil_handover,
	.adsp_ops = &adsp_ops,
};

static const struct adsp_pil_data ipq6018_adsp_resource_init = {
	.crash_reason_smem = 423,
	.firmware_name = "IPQ6018/adsp.mdt",
	.ssr_name = "adsp",
	.adsp_ops = &ipq6018_adsp_ops,
};

static const struct of_device_id adsp_of_match[] = {
	{ .compatible = "qcom,sdm845-adsp-pil", .data = &sdm845_adsp_resource_init },
	{ .compatible = "qcom,ipq6018-adsp-pil", .data = &ipq6018_adsp_resource_init},
	{ },
};
MODULE_DEVICE_TABLE(of, adsp_of_match);

static struct platform_driver adsp_pil_driver = {
	.probe = adsp_probe,
	.remove = adsp_remove,
	.driver = {
		.name = "qcom_q6v5_adsp",
		.of_match_table = adsp_of_match,
	},
};

module_platform_driver(adsp_pil_driver);
MODULE_DESCRIPTION("QTI ADSP Peripheral Image Loader");
MODULE_LICENSE("GPL v2");
module_param(debug_adsp, int, 0644);
