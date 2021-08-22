// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * Inspired by dwc3-of-simple.c
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/kernel.h>
#if defined(CONFIG_IPQ_DWC3_QTI_EXTCON)
#include <linux/extcon.h>
#endif
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/phy/phy.h>
#include <linux/usb/of.h>
#include <linux/reset.h>
#include <linux/iopoll.h>
#include <linux/usb/msm_hsusb.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

#include "core.h"
#include "gadget.h"
#include "dbm.h"

/* USB QSCRATCH Hardware registers */
#define QSCRATCH_HS_PHY_CTRL			0x10
#define UTMI_OTG_VBUS_VALID			BIT(20)
#define SW_SESSVLD_SEL				BIT(28)

#define QSCRATCH_SS_PHY_CTRL			0x30
#define LANE0_PWR_PRESENT			BIT(24)

#define QSCRATCH_GENERAL_CFG			0x08
#define PIPE_UTMI_CLK_SEL			BIT(0)
#define PIPE3_PHYSTATUS_SW			BIT(3)
#define PIPE_UTMI_CLK_DIS			BIT(8)

#define PWR_EVNT_IRQ_STAT_REG			0x58
#define PWR_EVNT_LPM_IN_L2_MASK			BIT(4)
#define PWR_EVNT_LPM_OUT_L2_MASK		BIT(5)

#define DSTS_CONNECTSPD_SS			0x4

struct dwc3_qcom {
	struct device		*dev;
	void __iomem		*dwc3_base;
	void __iomem		*qscratch_base;
	struct platform_device	*dwc3;
	struct clk		**clks;
	int			num_clocks;
	struct reset_control	*resets;

	int			hs_phy_irq;
	int			dp_hs_phy_irq;
	int			dm_hs_phy_irq;
	int			ss_phy_irq;

#if defined(CONFIG_IPQ_DWC3_QTI_EXTCON)
	struct extcon_dev	*edev;
	struct extcon_dev	*host_edev;
	struct notifier_block	vbus_nb;
	struct notifier_block	host_nb;

	bool			vbus_status;
	bool			id_status;
	struct work_struct	vbus_work;
	struct work_struct	host_work;
	struct workqueue_struct	*dwc3_wq;
#endif

	enum usb_dr_mode	mode;
	bool			is_suspended;
	bool			pm_suspended;

	struct dbm		*dbm;
	const struct usb_ep_ops *original_ep_ops[DWC3_ENDPOINTS_NUM];
	struct list_head req_complete_list;
	bool			phy_mux;
	struct regmap		*phy_mux_map;
	u32			phy_mux_reg;

	struct gpio_desc *device_power_gpio;
};

struct dwc3_msm_req_complete {
	struct list_head list_item;
	struct usb_request *req;
	void (*orig_complete)(struct usb_ep *ep,
			      struct usb_request *req);
};

static inline void dwc3_qcom_setbits(void __iomem *base, u32 offset, u32 val)
{
	u32 reg;

	reg = readl(base + offset);
	reg |= val;
	writel(reg, base + offset);

	/* ensure that above write is through */
	readl(base + offset);
}

static inline void dwc3_qcom_clrbits(void __iomem *base, u32 offset, u32 val)
{
	u32 reg;

	reg = readl(base + offset);
	reg &= ~val;
	writel(reg, base + offset);

	/* ensure that above write is through */
	readl(base + offset);
}

static void dwc3_qcom_vbus_overrride_enable(struct dwc3_qcom *qcom, bool enable)
{
	if (enable) {
		if (qcom->device_power_gpio)
			gpiod_set_value(qcom->device_power_gpio, 1);

		dwc3_qcom_setbits(qcom->qscratch_base, QSCRATCH_SS_PHY_CTRL,
				  LANE0_PWR_PRESENT);
		dwc3_qcom_setbits(qcom->qscratch_base, QSCRATCH_HS_PHY_CTRL,
				  UTMI_OTG_VBUS_VALID | SW_SESSVLD_SEL);
	} else {
		if (qcom->device_power_gpio)
			gpiod_set_value(qcom->device_power_gpio, 0);

		dwc3_qcom_clrbits(qcom->qscratch_base, QSCRATCH_SS_PHY_CTRL,
				  LANE0_PWR_PRESENT);
		dwc3_qcom_clrbits(qcom->qscratch_base, QSCRATCH_HS_PHY_CTRL,
				  UTMI_OTG_VBUS_VALID | SW_SESSVLD_SEL);
	}
}

#if defined(CONFIG_IPQ_DWC3_QTI_EXTCON)
static void dwc3_otg_start_peripheral(struct work_struct *w)
{
	struct dwc3_qcom *qcom = container_of(w, struct dwc3_qcom, vbus_work);
	struct dwc3 *dwc = platform_get_drvdata(qcom->dwc3);
	int ret;

	dwc3_qcom_vbus_overrride_enable(qcom, qcom->vbus_status);
	if (qcom->vbus_status) {
		dev_dbg(qcom->dev, "turn on dwc3 gadget\n");
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_DEVICE);
		ret = usb_gadget_vbus_connect(&dwc->gadget);
		if (ret)
			dev_err(qcom->dev, "%s: vbus connect  failed\n",
								__func__);
	} else {
		dev_dbg(qcom->dev, "turn off dwc3 gadget\n");
		ret = usb_gadget_vbus_disconnect(&dwc->gadget);
		if (ret)
			dev_err(qcom->dev, "%s: vbus disconnect failed\n",
								__func__);
	}
}

static void dwc3_otg_start_host(struct work_struct *w)
{
	struct dwc3_qcom *qcom = container_of(w, struct dwc3_qcom, host_work);
	struct dwc3 *dwc = platform_get_drvdata(qcom->dwc3);
	int ret;

	dwc3_qcom_vbus_overrride_enable(qcom, qcom->id_status);
	if (qcom->id_status) {
		dev_dbg(qcom->dev, "turn on dwc3 host\n");
		dwc3_set_mode(dwc, DWC3_GCTL_PRTCAP_HOST);
		ret = dwc3_host_init(dwc);
		if (ret)
			dev_err(dwc->dev, "failed to register xHCI device\n");

	} else {
		dev_dbg(qcom->dev, "turn off dwc3 host\n");
		dwc3_host_exit(dwc);
	}
}


static int dwc3_qcom_vbus_notifier(struct notifier_block *nb,
				   unsigned long event, void *ptr)
{
	struct dwc3_qcom *qcom = container_of(nb, struct dwc3_qcom, vbus_nb);

	if (qcom->vbus_status == event)
		return NOTIFY_DONE;

	qcom->vbus_status = event;
	queue_work(qcom->dwc3_wq, &qcom->vbus_work);

	qcom->mode = event ? USB_DR_MODE_PERIPHERAL : USB_DR_MODE_HOST;

	return NOTIFY_DONE;
}

static int dwc3_qcom_host_notifier(struct notifier_block *nb,
				   unsigned long event, void *ptr)
{
	struct dwc3_qcom *qcom = container_of(nb, struct dwc3_qcom, host_nb);

	if (qcom->id_status == event)
		return NOTIFY_DONE;

	qcom->id_status = event;
	queue_work(qcom->dwc3_wq, &qcom->host_work);

	qcom->mode = event ? USB_DR_MODE_HOST : USB_DR_MODE_PERIPHERAL;

	return NOTIFY_DONE;
}

static int dwc3_qcom_register_extcon(struct dwc3_qcom *qcom)
{
	struct device		*dev = qcom->dev;
	struct extcon_dev	*host_edev;
	int			ret;

	if (!of_property_read_bool(dev->of_node, "extcon"))
		return 0;

	qcom->edev = extcon_get_edev_by_phandle(dev, 0);
	if (IS_ERR(qcom->edev))
		return PTR_ERR(qcom->edev);

	qcom->vbus_nb.notifier_call = dwc3_qcom_vbus_notifier;

	qcom->host_edev = extcon_get_edev_by_phandle(dev, 1);
	if (IS_ERR(qcom->host_edev))
		qcom->host_edev = NULL;

	ret = extcon_register_notifier(qcom->edev, EXTCON_USB, &qcom->vbus_nb);
	if (ret < 0) {
		dev_err(dev, "VBUS notifier register failed\n");
		return ret;
	}

	if (qcom->host_edev)
		host_edev = qcom->host_edev;
	else
		host_edev = qcom->edev;

	qcom->host_nb.notifier_call = dwc3_qcom_host_notifier;
	ret = extcon_register_notifier(host_edev, EXTCON_USB_HOST,
				       &qcom->host_nb);
	if (ret < 0) {
		dev_err(dev, "Host notifier register failed\n");
		return ret;
	}

	/* Update initial VBUS override based on extcon state */
	if (extcon_get_cable_state_(qcom->edev, EXTCON_USB) ||
	    !extcon_get_cable_state_(host_edev, EXTCON_USB_HOST)) {
		dwc3_qcom_host_notifier(&qcom->host_nb, false, host_edev);
		dwc3_qcom_vbus_notifier(&qcom->vbus_nb, true, qcom->edev);
	} else {
		dwc3_qcom_vbus_notifier(&qcom->vbus_nb, false, qcom->edev);
		dwc3_qcom_host_notifier(&qcom->host_nb, true, host_edev);
	}

	return 0;
}
#endif

static void dwc3_qcom_disable_interrupts(struct dwc3_qcom *qcom)
{
	if (qcom->hs_phy_irq) {
		disable_irq_wake(qcom->hs_phy_irq);
		disable_irq_nosync(qcom->hs_phy_irq);
	}

	if (qcom->dp_hs_phy_irq) {
		disable_irq_wake(qcom->dp_hs_phy_irq);
		disable_irq_nosync(qcom->dp_hs_phy_irq);
	}

	if (qcom->dm_hs_phy_irq) {
		disable_irq_wake(qcom->dm_hs_phy_irq);
		disable_irq_nosync(qcom->dm_hs_phy_irq);
	}

	if (qcom->ss_phy_irq) {
		disable_irq_wake(qcom->ss_phy_irq);
		disable_irq_nosync(qcom->ss_phy_irq);
	}
}

static void dwc3_qcom_enable_interrupts(struct dwc3_qcom *qcom)
{
	if (qcom->hs_phy_irq) {
		enable_irq(qcom->hs_phy_irq);
		enable_irq_wake(qcom->hs_phy_irq);
	}

	if (qcom->dp_hs_phy_irq) {
		enable_irq(qcom->dp_hs_phy_irq);
		enable_irq_wake(qcom->dp_hs_phy_irq);
	}

	if (qcom->dm_hs_phy_irq) {
		enable_irq(qcom->dm_hs_phy_irq);
		enable_irq_wake(qcom->dm_hs_phy_irq);
	}

	if (qcom->ss_phy_irq) {
		enable_irq(qcom->ss_phy_irq);
		enable_irq_wake(qcom->ss_phy_irq);
	}
}

static int dwc3_qcom_suspend(struct dwc3_qcom *qcom)
{
	u32 val;
	int i;

	if (qcom->is_suspended)
		return 0;

	val = readl(qcom->qscratch_base + PWR_EVNT_IRQ_STAT_REG);
	if (!(val & PWR_EVNT_LPM_IN_L2_MASK))
		dev_err(qcom->dev, "HS-PHY not in L2\n");

	for (i = qcom->num_clocks - 1; i >= 0; i--)
		clk_disable_unprepare(qcom->clks[i]);

	qcom->is_suspended = true;
	dwc3_qcom_enable_interrupts(qcom);

	return 0;
}

static int dwc3_qcom_resume(struct dwc3_qcom *qcom)
{
	int ret;
	int i;

	if (!qcom->is_suspended)
		return 0;

	dwc3_qcom_disable_interrupts(qcom);

	for (i = 0; i < qcom->num_clocks; i++) {
		ret = clk_prepare_enable(qcom->clks[i]);
		if (ret < 0) {
			while (--i >= 0)
				clk_disable_unprepare(qcom->clks[i]);
			return ret;
		}
	}

	/* Clear existing events from PHY related to L2 in/out */
	dwc3_qcom_setbits(qcom->qscratch_base, PWR_EVNT_IRQ_STAT_REG,
			  PWR_EVNT_LPM_IN_L2_MASK | PWR_EVNT_LPM_OUT_L2_MASK);

	qcom->is_suspended = false;

	return 0;
}

static irqreturn_t qcom_dwc3_resume_irq(int irq, void *data)
{
	struct dwc3_qcom *qcom = data;
	struct dwc3	*dwc = platform_get_drvdata(qcom->dwc3);

	/* If pm_suspended then let pm_resume take care of resuming h/w */
	if (qcom->pm_suspended)
		return IRQ_HANDLED;

	if (dwc->xhci)
		pm_runtime_resume(&dwc->xhci->dev);

	return IRQ_HANDLED;
}

static void dwc3_qcom_select_utmi_clk(struct dwc3_qcom *qcom)
{
	/* Configure dwc3 to use UTMI clock as PIPE clock not present */
	dwc3_qcom_setbits(qcom->qscratch_base, QSCRATCH_GENERAL_CFG,
			  PIPE_UTMI_CLK_DIS);

	usleep_range(100, 1000);

	dwc3_qcom_setbits(qcom->qscratch_base, QSCRATCH_GENERAL_CFG,
			  PIPE_UTMI_CLK_SEL | PIPE3_PHYSTATUS_SW);

	usleep_range(100, 1000);

	dwc3_qcom_clrbits(qcom->qscratch_base, QSCRATCH_GENERAL_CFG,
			  PIPE_UTMI_CLK_DIS);
}

static int dwc3_qcom_setup_irq(struct platform_device *pdev)
{
	struct dwc3_qcom *qcom = platform_get_drvdata(pdev);
	int irq, ret;

	irq = platform_get_irq_byname(pdev, "hs_phy_irq");
	if (irq > 0) {
		/* Keep wakeup interrupts disabled until suspend */
		irq_set_status_flags(irq, IRQ_NOAUTOEN);
		ret = devm_request_threaded_irq(qcom->dev, irq, NULL,
					qcom_dwc3_resume_irq,
					IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					"qcom_dwc3 HS", qcom);
		if (ret) {
			dev_err(qcom->dev, "hs_phy_irq failed: %d\n", ret);
			return ret;
		}
		qcom->hs_phy_irq = irq;
	}

	irq = platform_get_irq_byname(pdev, "dp_hs_phy_irq");
	if (irq > 0) {
		irq_set_status_flags(irq, IRQ_NOAUTOEN);
		ret = devm_request_threaded_irq(qcom->dev, irq, NULL,
					qcom_dwc3_resume_irq,
					IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					"qcom_dwc3 DP_HS", qcom);
		if (ret) {
			dev_err(qcom->dev, "dp_hs_phy_irq failed: %d\n", ret);
			return ret;
		}
		qcom->dp_hs_phy_irq = irq;
	}

	irq = platform_get_irq_byname(pdev, "dm_hs_phy_irq");
	if (irq > 0) {
		irq_set_status_flags(irq, IRQ_NOAUTOEN);
		ret = devm_request_threaded_irq(qcom->dev, irq, NULL,
					qcom_dwc3_resume_irq,
					IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					"qcom_dwc3 DM_HS", qcom);
		if (ret) {
			dev_err(qcom->dev, "dm_hs_phy_irq failed: %d\n", ret);
			return ret;
		}
		qcom->dm_hs_phy_irq = irq;
	}

	irq = platform_get_irq_byname(pdev, "ss_phy_irq");
	if (irq > 0) {
		irq_set_status_flags(irq, IRQ_NOAUTOEN);
		ret = devm_request_threaded_irq(qcom->dev, irq, NULL,
					qcom_dwc3_resume_irq,
					IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
					"qcom_dwc3 SS", qcom);
		if (ret) {
			dev_err(qcom->dev, "ss_phy_irq failed: %d\n", ret);
			return ret;
		}
		qcom->ss_phy_irq = irq;
	}

	return 0;
}

static int dwc3_qcom_clk_init(struct dwc3_qcom *qcom, int count)
{
	struct device		*dev = qcom->dev;
	struct device_node	*np = dev->of_node;
	int			i;

	qcom->num_clocks = count;

	if (!count)
		return 0;

	qcom->clks = devm_kcalloc(dev, qcom->num_clocks,
				  sizeof(struct clk *), GFP_KERNEL);
	if (!qcom->clks)
		return -ENOMEM;

	for (i = 0; i < qcom->num_clocks; i++) {
		struct clk	*clk;
		int		ret;

		clk = of_clk_get(np, i);
		if (IS_ERR(clk)) {
			while (--i >= 0)
				clk_put(qcom->clks[i]);
			return PTR_ERR(clk);
		}

		ret = clk_prepare_enable(clk);
		if (ret < 0) {
			while (--i >= 0) {
				clk_disable_unprepare(qcom->clks[i]);
				clk_put(qcom->clks[i]);
			}
			clk_put(clk);

			return ret;
		}

		qcom->clks[i] = clk;
	}

	return 0;
}

static int dwc3_qcom_phy_sel(struct dwc3_qcom *qcom)
{
	struct of_phandle_args args;
	int ret;

	ret = of_parse_phandle_with_fixed_args(qcom->dev->of_node,
			"qcom,phy-mux-regs", 1, 0, &args);
	if (ret) {
		dev_err(qcom->dev, "failed to parse qcom,phy-mux-regs\n");
		return ret;
	}

	qcom->phy_mux_map = syscon_node_to_regmap(args.np);
	of_node_put(args.np);
	if (IS_ERR(qcom->phy_mux_map)) {
		pr_err("phy mux regs map failed:%ld\n",
						PTR_ERR(qcom->phy_mux_map));
		return PTR_ERR(qcom->phy_mux_map);
	}

	qcom->phy_mux_reg = args.args[0];
	/*usb phy mux sel*/
	ret = regmap_write(qcom->phy_mux_map, qcom->phy_mux_reg, 0x1);
	if (ret) {
		dev_err(qcom->dev,
			"Not able to configure phy mux selection:%d\n", ret);
		return ret;
	}
	return 0;
}

static int dwc3_qcom_probe(struct platform_device *pdev)
{
	struct device_node	*np = pdev->dev.of_node, *dwc3_np;
	struct device		*dev = &pdev->dev;
	struct dwc3_qcom	*qcom;
	struct resource		*res;
	int			ret, i;
	bool			ignore_pipe_clk;

	qcom = devm_kzalloc(&pdev->dev, sizeof(*qcom), GFP_KERNEL);
	if (!qcom)
		return -ENOMEM;

	platform_set_drvdata(pdev, qcom);
	qcom->dev = &pdev->dev;

	qcom->device_power_gpio = devm_gpiod_get_optional(dev, "device-power",
							  GPIOD_OUT_HIGH);

	qcom->phy_mux = device_property_read_bool(dev,
				"qcom,multiplexed-phy");
	if (qcom->phy_mux)
		dwc3_qcom_phy_sel(qcom);

#if defined(CONFIG_IPQ_DWC3_QTI_EXTCON)
	INIT_WORK(&qcom->vbus_work, dwc3_otg_start_peripheral);
	INIT_WORK(&qcom->host_work, dwc3_otg_start_host);

	qcom->dwc3_wq = alloc_ordered_workqueue("dwc3_wq", 0);
	if (!qcom->dwc3_wq) {
		pr_err("%s: Unable to create workqueue dwc3_wq\n", __func__);
		return -ENOMEM;
	}
#endif

	qcom->resets = devm_reset_control_get(dev, "usb30_mstr_rst");
	if (IS_ERR(qcom->resets)) {
		ret = PTR_ERR(qcom->resets);
		dev_dbg(dev, "failed to get resets, err=%d\n", ret);
	} else {
		ret = reset_control_assert(qcom->resets);
		if (ret) {
			dev_err(dev, "failed to assert resets, err=%d\n", ret);
			return ret;
		}

		usleep_range(10, 1000);

		ret = reset_control_deassert(qcom->resets);
		if (ret) {
			dev_err(dev, "failed to deassert resets, err=%d\n", ret);
			goto reset_assert;
		}
	}

	ret = dwc3_qcom_clk_init(qcom, of_count_phandle_with_args(np,
						"clocks", "#clock-cells"));
	if (ret) {
		dev_err(dev, "failed to get clocks\n");
		goto reset_assert;
	}

	if (of_get_property(pdev->dev.of_node, "qcom,usb-dbm", NULL)) {
		qcom->dbm = usb_get_dbm_by_phandle(&pdev->dev, "qcom,usb-dbm");
		if (IS_ERR(qcom->dbm)) {
			dev_err(&pdev->dev, "unable to get dbm device\n");
			ret = -EPROBE_DEFER;
			goto clk_disable;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	qcom->qscratch_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(qcom->qscratch_base)) {
		dev_err(dev, "failed to map qscratch, err=%d\n", ret);
		ret = PTR_ERR(qcom->qscratch_base);
		goto clk_disable;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"dwc3_base");
	if (res) {
		qcom->dwc3_base = devm_ioremap_nocache(dev, res->start,
							resource_size(res));
		if (IS_ERR(qcom->dwc3_base)) {
			dev_err(dev, "couldn't ioremap dwc3_base\n");
			ret = -ENODEV;
			goto clk_disable;
		}
	}

	INIT_LIST_HEAD(&qcom->req_complete_list);

	ret = dwc3_qcom_setup_irq(pdev);
	if (ret)
		goto clk_disable;

	dwc3_np = of_get_child_by_name(np, "dwc3");
	if (!dwc3_np) {
		dev_err(dev, "failed to find dwc3 core child\n");
		ret = -ENODEV;
		goto clk_disable;
	}

	/*
	 * Disable pipe_clk requirement if specified. Used when dwc3
	 * operates without SSPHY and only HS/FS/LS modes are supported.
	 */
	ignore_pipe_clk = device_property_read_bool(dev,
				"qcom,select-utmi-as-pipe-clk");
	if (ignore_pipe_clk)
		dwc3_qcom_select_utmi_clk(qcom);

	ret = of_platform_populate(np, NULL, NULL, dev);
	if (ret) {
		dev_err(dev, "failed to register dwc3 core - %d\n", ret);
		goto clk_disable;
	}

	qcom->dwc3 = of_find_device_by_node(dwc3_np);
	if (!qcom->dwc3) {
		dev_err(&pdev->dev, "failed to get dwc3 platform device\n");
		ret = -ENODEV;
		goto depopulate;
	}

	qcom->mode = usb_get_dr_mode(&qcom->dwc3->dev);

	/* enable vbus override for device mode */
	if (qcom->mode == USB_DR_MODE_PERIPHERAL)
		dwc3_qcom_vbus_overrride_enable(qcom, true);
	else if (qcom->device_power_gpio)
		gpiod_set_value(qcom->device_power_gpio, 0);

#if defined(CONFIG_IPQ_DWC3_QTI_EXTCON)
	/* register extcon to override sw_vbus on Vbus change later */
	ret = dwc3_qcom_register_extcon(qcom);
	if (ret)
		goto depopulate;
#endif

	device_init_wakeup(&pdev->dev, 1);
	qcom->is_suspended = false;
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_forbid(dev);

	return 0;

depopulate:
	of_platform_depopulate(&pdev->dev);
clk_disable:
	for (i = qcom->num_clocks - 1; i >= 0; i--) {
		clk_disable_unprepare(qcom->clks[i]);
		clk_put(qcom->clks[i]);
	}
reset_assert:
	if (!IS_ERR(qcom->resets))
		reset_control_assert(qcom->resets);

#if defined(CONFIG_IPQ_DWC3_QTI_EXTCON)
	destroy_workqueue(qcom->dwc3_wq);
#endif
	return ret;
}

static int dwc3_qcom_remove(struct platform_device *pdev)
{
	struct dwc3_qcom *qcom = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	int i;

#if defined(CONFIG_IPQ_DWC3_QTI_EXTCON)
	extcon_unregister_notifier(qcom->edev, EXTCON_USB, &qcom->vbus_nb);
	extcon_unregister_notifier(qcom->host_edev, EXTCON_USB_HOST,
				   &qcom->host_nb);
	destroy_workqueue(qcom->dwc3_wq);
#endif
	of_platform_depopulate(dev);

	for (i = qcom->num_clocks - 1; i >= 0; i--) {
		clk_disable_unprepare(qcom->clks[i]);
		clk_put(qcom->clks[i]);
	}
	qcom->num_clocks = 0;

	if (!IS_ERR(qcom->resets))
		reset_control_assert(qcom->resets);

	if (qcom->phy_mux) {
		/*usb phy mux deselection*/
		int ret = regmap_write(qcom->phy_mux_map, qcom->phy_mux_reg, 0x0);
		if (ret)
			dev_err(qcom->dev,
				"Not able to configure phy mux selection:%d\n", ret);
	}

	pm_runtime_allow(dev);
	pm_runtime_disable(dev);

	return 0;
}

static inline dma_addr_t dwc3_trb_dma_offset(struct dwc3_ep *dep,
					     struct dwc3_trb *trb)
{
	u32 offset = (char *) trb - (char *) dep->trb_pool;

	return dep->trb_pool_dma + offset;
}

/**
 *
 * Read register with debug info.
 *
 * @base - DWC3 base virtual address.
 * @offset - register offset.
 *
 * @return u32
 */
static inline u32 dwc3_msm_read_reg(void *base, u32 offset)
{
	u32 val = ioread32(base + offset);
	return val;
}

static inline bool dwc3_msm_is_dev_superspeed(struct dwc3_qcom *mdwc)
{
	u8 speed;

	speed = dwc3_msm_read_reg(mdwc->dwc3_base, DWC3_DSTS) & DWC3_DSTS_CONNECTSPD;
	return !!(speed & DSTS_CONNECTSPD_SS);
}

static inline void dwc3_msm_write_reg_field(void *base, u32 offset,
					    const unsigned long mask, u32 val)
{
	u32 shift = find_first_bit((void *)&mask, 32);
	u32 tmp = ioread32(base + offset);

	tmp &= ~mask;           /* clear written bits */
	val = tmp | (val << shift);
	iowrite32(val, base + offset);
}

static inline void dwc3_msm_writel(void __iomem *base, u32 offset, u32 value)
{
	u32 offs = offset - DWC3_GLOBALS_REGS_START;

	writel(value, base + offs);
}

static inline u32 dwc3_msm_readl(void __iomem *base, u32 offset)
{
	u32 offs = offset - DWC3_GLOBALS_REGS_START;
	u32 value;

	value = readl(base + offs);
	return value;
}

/**
 * Configure the DBM with the BAM's data fifo.
 * This function is called by the USB BAM Driver
 * upon initialization.
 *
 * @ep - pointer to usb endpoint.
 * @addr - address of data fifo.
 * @size - size of data fifo.
 *
 */
int msm_data_fifo_config(struct usb_ep *ep, phys_addr_t addr,
			 u32 size, u8 dst_pipe_idx)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_qcom *mdwc = dev_get_drvdata(dwc->dev->parent);

	dev_dbg(mdwc->dev, "%s\n", __func__);

	return  dbm_data_fifo_config(mdwc->dbm, dep->number, addr, size,
				     dst_pipe_idx);
}
EXPORT_SYMBOL(msm_data_fifo_config);

/**
 * Cleanups for msm endpoint on request complete.
 *
 * Also call original request complete.
 *
 * @usb_ep - pointer to usb_ep instance.
 * @request - pointer to usb_request instance.
 *
 * @return int - 0 on success, negative on error.
 */
static void dwc3_msm_req_complete_func(struct usb_ep *ep,
				       struct usb_request *request)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_qcom *mdwc = dev_get_drvdata(dwc->dev->parent);
	struct dwc3_msm_req_complete *req_complete = NULL;


	/* Find original request complete function and remove it from list */
	list_for_each_entry(req_complete, &mdwc->req_complete_list, list_item) {
		if (req_complete->req == request)
			break;
	}
	if (!req_complete || req_complete->req != request) {
		dev_err(dep->dwc->dev, "%s: could not find the request\n",
			__func__);
		return;
	}
	list_del(&req_complete->list_item);

	/*
	 * Release another one TRB to the pool since DBM queue took 2 TRBs
	 * (normal and link), and the dwc3/gadget.c :: dwc3_gadget_giveback
	 * released only one.
	 */
	dep->busy_slot++;

	/* Unconfigure dbm ep */
	dbm_ep_unconfig(mdwc->dbm, dep->number);

	/*
	 * If this is the last endpoint we unconfigured, than reset also
	 * the event buffers; unless unconfiguring the ep due to lpm,
	 * in which case the event buffer only gets reset during the
	 * block reset.
	 */
	if ((dbm_get_num_of_eps_configured(mdwc->dbm) == 0) &&
	    !dbm_reset_ep_after_lpm(mdwc->dbm))
		dbm_event_buffer_config(mdwc->dbm, 0, 0, 0);

	/*
	 * Call original complete function, notice that dwc->lock is already
	 * taken by the caller of this function (dwc3_gadget_giveback()).
	 */
	request->complete = req_complete->orig_complete;
	if (request->complete)
		request->complete(ep, request);

	kfree(req_complete);
}

/**
 * Helper function
 *
 * Reset  DBM endpoint.
 *
 * @mdwc - pointer to dwc3_msm instance.
 * @dep - pointer to dwc3_ep instance.
 *
 * @return int - 0 on success, negative on error.
 */
static int __dwc3_msm_dbm_ep_reset(struct dwc3_qcom *mdwc, struct dwc3_ep *dep)
{
	int ret;

	dev_dbg(mdwc->dev, "Resetting dbm endpoint %d\n", dep->number);

	/* Reset the dbm endpoint */
	ret = dbm_ep_soft_reset(mdwc->dbm, dep->number, true);
	if (ret) {
		dev_err(mdwc->dev, "%s: failed to assert dbm ep reset\n",
			__func__);
		return ret;
	}

	/*
	 * The necessary delay between asserting and deasserting the dbm ep
	 * reset is based on the number of active endpoints. If there is more
	 * than one endpoint, a 1 msec delay is required. Otherwise, a shorter
	 * delay will suffice.
	 */
	if (dbm_get_num_of_eps_configured(mdwc->dbm) > 1)
		usleep_range(1000, 1200);
	else
		udelay(10);
	ret = dbm_ep_soft_reset(mdwc->dbm, dep->number, false);
	if (ret) {
		dev_err(mdwc->dev, "%s: failed to deassert dbm ep reset\n",
			__func__);
		return ret;
	}

	return 0;
}

/**
 * Reset the DBM endpoint which is linked to the given USB endpoint.
 *
 * @usb_ep - pointer to usb_ep instance.
 *
 * @return int - 0 on success, negative on error.
 */
int msm_dwc3_reset_dbm_ep(struct usb_ep *ep)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_qcom *mdwc = dev_get_drvdata(dwc->dev->parent);

	return __dwc3_msm_dbm_ep_reset(mdwc, dep);
}
EXPORT_SYMBOL(msm_dwc3_reset_dbm_ep);

static u32 dwc3_msm_gadget_ep_get_transfer_index(struct dwc3 *dwc, u8 number)
{
	u32 res_id;
	u32 offs = DWC3_DEPCMD(number) - DWC3_GLOBALS_REGS_START;

	res_id = readl(dwc->regs + offs);

	return DWC3_DEPCMD_GET_RSC_IDX(res_id);
}

int dwc3_msm_send_gadget_ep_cmd(struct dwc3 *dwc, unsigned ep, unsigned cmd,
				struct dwc3_gadget_ep_cmd_params *params)
{
	u32 timeout = 500;
	u32 reg;

	dwc3_msm_writel(dwc->regs, DWC3_DEPCMDPAR0(ep), params->param0);
	dwc3_msm_writel(dwc->regs, DWC3_DEPCMDPAR1(ep), params->param1);
	dwc3_msm_writel(dwc->regs, DWC3_DEPCMDPAR2(ep), params->param2);

	dwc3_msm_writel(dwc->regs, DWC3_DEPCMD(ep), cmd | DWC3_DEPCMD_CMDACT);
	do {
		reg = dwc3_msm_readl(dwc->regs, DWC3_DEPCMD(ep));
		if (!(reg & DWC3_DEPCMD_CMDACT)) {
			if (DWC3_DEPCMD_STATUS(reg))
				return -EINVAL;
			return 0;
		}

		timeout--;
		if (!timeout)
			return -ETIMEDOUT;

		udelay(1);
	} while (1);
}

/**
 * Helper function.
 * See the header of the dwc3_msm_ep_queue function.
 *
 * @dwc3_ep - pointer to dwc3_ep instance.
 * @req - pointer to dwc3_request instance.
 *
 * @return int - 0 on success, negative on error.
 */
int __dwc3_msm_ep_queue(struct dwc3_ep *dep, struct dwc3_request *req)
{
	struct dwc3_trb *trb;
	struct dwc3_trb *trb_link;
	struct dwc3_gadget_ep_cmd_params params;
	u32 cmd;
	int ret = 0;


	/* We push the request to the dep->req_queued list to indicate that
	 * this request is issued with start transfer. The request will be out
	 * from this list in 2 cases. The first is that the transfer will be
	 * completed (not if the transfer is endless using a circular TRBs with
	 * with link TRB). The second case is an option to do stop stransfer,
	 * this can be initiated by the function driver when calling dequeue.
	 */
	req->queued = true;
	list_add_tail(&req->list, &dep->req_queued);

	/* First, prepare a normal TRB, point to the fake buffer */
	trb = &dep->trb_pool[dep->free_slot & DWC3_TRB_MASK];
	dep->free_slot++;
	memset(trb, 0, sizeof(*trb));

	req->trb = trb;
	trb->bph = DBM_TRB_BIT | DBM_TRB_DMA | DBM_TRB_EP_NUM(dep->number);
	trb->size = DWC3_TRB_SIZE_LENGTH(req->request.length);
	trb->ctrl = DWC3_TRBCTL_NORMAL | DWC3_TRB_CTRL_HWO |
		DWC3_TRB_CTRL_CHN | (req->direction ? 0 : DWC3_TRB_CTRL_CSP);
	req->trb_dma = dwc3_trb_dma_offset(dep, trb);

	/* Second, prepare a Link TRB that points to the first TRB*/
	trb_link = &dep->trb_pool[dep->free_slot & DWC3_TRB_MASK];
	dep->free_slot++;
	memset(trb_link, 0, sizeof(*trb_link));

	trb_link->bpl = lower_32_bits(req->trb_dma);
	trb_link->bph = DBM_TRB_BIT |
		DBM_TRB_DMA | DBM_TRB_EP_NUM(dep->number);
	trb_link->size = 0;
	trb_link->ctrl = DWC3_TRBCTL_LINK_TRB | DWC3_TRB_CTRL_HWO;
	/*
	 * Now start the transfer
	 */
	memset(&params, 0, sizeof(params));
	params.param0 = 0; /* TDAddr High */
	params.param1 = lower_32_bits(req->trb_dma); /* DAddr Low */

	/* DBM requires IOC to be set */
	cmd = DWC3_DEPCMD_STARTTRANSFER | DWC3_DEPCMD_CMDIOC;
	ret = dwc3_msm_send_gadget_ep_cmd(dep->dwc, dep->number, cmd, &params);
	if (ret < 0) {
		dev_dbg(dep->dwc->dev,
			"%s: failed to send STARTTRANSFER command\n",
			__func__);

		list_del(&req->list);
		return ret;
	}
	dep->flags |= DWC3_EP_BUSY;
	dep->resource_index = dwc3_msm_gadget_ep_get_transfer_index(dep->dwc,
								dep->number);

	return ret;
}

/**
 * Queue a usb request to the DBM endpoint.
 * This function should be called after the endpoint
 * was enabled by the ep_enable.
 *
 * This function prepares special structure of TRBs which
 * is familiar with the DBM HW, so it will possible to use
 * this endpoint in DBM mode.
 *
 * The TRBs prepared by this function, is one normal TRB
 * which point to a fake buffer, followed by a link TRB
 * that points to the first TRB.
 *
 * The API of this function follow the regular API of
 * usb_ep_queue (see usb_ep_ops in include/linuk/usb/gadget.h).
 *
 * @usb_ep - pointer to usb_ep instance.
 * @request - pointer to usb_request instance.
 * @gfp_flags - possible flags.
 *
 * @return int - 0 on success, negative on error.
 */
int dwc3_msm_ep_queue(struct usb_ep *ep,
		      struct usb_request *request, gfp_t gfp_flags)
{
	struct dwc3_request *req = to_dwc3_request(request);
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_qcom *mdwc = dev_get_drvdata(dwc->dev->parent);
	struct dwc3_msm_req_complete *req_complete;
	unsigned long flags;
	int ret = 0, size;
	bool superspeed;
	/*
	 * We must obtain the lock of the dwc3 core driver,
	 * including disabling interrupts, so we will be sure
	 * that we are the only ones that configure the HW device
	 * core and ensure that we queuing the request will finish
	 * as soon as possible so we will release back the lock.
	 */
	spin_lock_irqsave(&dwc->lock, flags);
	if (!dep->endpoint.desc) {
		dev_err(mdwc->dev,
			"%s: trying to queue request %p to disabled ep %s\n",
			__func__, request, ep->name);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EPERM;
	}

	if (!mdwc->original_ep_ops[dep->number]) {
		dev_err(mdwc->dev,
			"ep [%s,%d] was unconfigured as msm endpoint\n",
			ep->name, dep->number);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EINVAL;
	}

	if (!request) {
		dev_err(mdwc->dev, "%s: request is NULL\n", __func__);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EINVAL;
	}

	if (!(request->udc_priv & MSM_SPS_MODE)) {
		dev_err(mdwc->dev, "%s: sps mode is not set\n",
			__func__);

		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EINVAL;
	}

	/* HW restriction regarding TRB size (8KB) */
	if (req->request.length < 0x2000) {
		dev_err(mdwc->dev, "%s: Min TRB size is 8KB\n", __func__);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EINVAL;
	}

	if (dep->number == 0 || dep->number == 1) {
		dev_err(mdwc->dev,
			"%s: trying to queue dbm request %p to control ep %s\n",
			__func__, request, ep->name);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EPERM;
	}

	if (dep->busy_slot != dep->free_slot || !list_empty(&dep->request_list)
	    || !list_empty(&dep->req_queued)) {
		dev_err(mdwc->dev,
			"%s: trying to queue dbm request %p tp ep %s\n",
			__func__, request, ep->name);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EPERM;
	}
	dep->busy_slot = 0;
	dep->free_slot = 0;

	/*
	 * Override req->complete function, but before doing that,
	 * store it's original pointer in the req_complete_list.
	 */
	req_complete = kzalloc(sizeof(*req_complete), gfp_flags);
	if (!req_complete) {
		dev_err(mdwc->dev, "%s: not enough memory\n", __func__);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -ENOMEM;
	}
	req_complete->req = request;
	req_complete->orig_complete = request->complete;
	list_add_tail(&req_complete->list_item, &mdwc->req_complete_list);
	request->complete = dwc3_msm_req_complete_func;

	dev_dbg(dwc->dev, "%s: queing request %pK to ep %s length %d dep->number=0x%x\n",
		  __func__, request, ep->name, request->length, dep->number);
	size = dwc3_msm_read_reg(mdwc->dwc3_base, DWC3_GEVNTSIZ(0));
	dbm_event_buffer_config(mdwc->dbm,
				dwc3_msm_read_reg(mdwc->dwc3_base, DWC3_GEVNTADRLO(0)),
				dwc3_msm_read_reg(mdwc->dwc3_base, DWC3_GEVNTADRHI(0)),
				DWC3_GEVNTSIZ_SIZE(size));

	ret = __dwc3_msm_ep_queue(dep, req);
	if (ret < 0) {
		dev_err(mdwc->dev,
			"error %d after calling __dwc3_msm_ep_queue\n", ret);
		goto err;
	}
	spin_unlock_irqrestore(&dwc->lock, flags);
	superspeed = dwc3_msm_is_dev_superspeed(mdwc);
	dbm_set_speed(mdwc->dbm, (u8)superspeed);
	return 0;

err:
	list_del(&req_complete->list_item);
	spin_unlock_irqrestore(&dwc->lock, flags);
	kfree(req_complete);
	return ret;
}

/**
 * Configure MSM endpoint.
 * This function do specific configurations
 * to an endpoint which need specific implementaion
 * in the MSM architecture.
 *
 * This function should be called by usb function/class
 * layer which need a support from the specific MSM HW
 * which wrap the USB3 core. (like GSI or DBM specific endpoints)
 *
 * @ep - a pointer to some usb_ep instance
 *
 * @return int - 0 on success, negetive on error.
 */
int msm_ep_config(struct usb_ep *ep, struct usb_request *request)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_qcom *mdwc = dev_get_drvdata(dwc->dev->parent);
	struct usb_ep_ops *new_ep_ops;
	int ret = 0;
	u8 bam_pipe;
	bool producer;
	bool disable_wb;
	bool internal_mem;
	bool ioc;
	unsigned long flags;

	/* Reset the DBM */
	dbm_soft_reset(mdwc->dbm, 1);
	usleep_range(1000, 1200);
	dbm_soft_reset(mdwc->dbm, 0);

	/*enable DBM*/
	writel((readl(mdwc->qscratch_base + QSCRATCH_GENERAL_CFG) | 0x2),
		mdwc->qscratch_base + QSCRATCH_GENERAL_CFG);
	dbm_enable(mdwc->dbm);

	spin_lock_irqsave(&dwc->lock, flags);
	/* Save original ep ops for future restore*/
	if (mdwc->original_ep_ops[dep->number]) {
		dev_err(mdwc->dev,
			"ep [%s,%d] already configured as msm endpoint\n",
			ep->name, dep->number);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EPERM;
	}
	mdwc->original_ep_ops[dep->number] = ep->ops;

	/* Set new usb ops as we like */
	new_ep_ops = kzalloc(sizeof(struct usb_ep_ops), GFP_ATOMIC);
	if (!new_ep_ops) {
		dev_err(mdwc->dev,
			"%s: unable to allocate mem for new usb ep ops\n",
			__func__);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -ENOMEM;
	}
	(*new_ep_ops) = (*ep->ops);
	new_ep_ops->queue = dwc3_msm_ep_queue;
	ep->ops = new_ep_ops;

	if (!mdwc->dbm || !request
			/*|| (dep->endpoint.ep_type == EP_TYPE_GSI)*/) {
		spin_unlock_irqrestore(&dwc->lock, flags);
		return 0;
	}

	/*
	 * Configure the DBM endpoint if required.
	 */
	bam_pipe = request->udc_priv & MSM_PIPE_ID_MASK;
	producer = ((request->udc_priv & MSM_PRODUCER) ? true : false);
	disable_wb = ((request->udc_priv & MSM_DISABLE_WB) ? true : false);
	internal_mem = ((request->udc_priv & MSM_INTERNAL_MEM) ? true : false);
	ioc = ((request->udc_priv & MSM_ETD_IOC) ? true : false);

	pr_info("%s: dep->number=0x%x, ioc=0x%x, bam_pipe=0x%x\n",
					__func__, dep->number, ioc, bam_pipe);

	ret = dbm_ep_config(mdwc->dbm, dep->number, bam_pipe, producer,
			    disable_wb, internal_mem, ioc);
	if (ret < 0) {
		dev_err(mdwc->dev,
			"error %d after calling dbm_ep_config\n", ret);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return ret;
	}

	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}
EXPORT_SYMBOL(msm_ep_config);

/**
 * Un-configure MSM endpoint.
 * Tear down configurations done in the
 * dwc3_msm_ep_config function.
 *
 * @ep - a pointer to some usb_ep instance
 *
 * @return int - 0 on success, negative on error.
 */
int msm_ep_unconfig(struct usb_ep *ep)
{
	struct dwc3_ep *dep = to_dwc3_ep(ep);
	struct dwc3 *dwc = dep->dwc;
	struct dwc3_qcom *mdwc = dev_get_drvdata(dwc->dev->parent);
	struct usb_ep_ops *old_ep_ops;
	unsigned long flags;

	spin_lock_irqsave(&dwc->lock, flags);
	/* Restore original ep ops */
	if (!mdwc->original_ep_ops[dep->number]) {
		dev_err(mdwc->dev,
			"ep [%s,%d] was not configured as msm endpoint\n",
			ep->name, dep->number);
		spin_unlock_irqrestore(&dwc->lock, flags);
		return -EINVAL;
	}
	old_ep_ops = (struct usb_ep_ops *)ep->ops;
	ep->ops = mdwc->original_ep_ops[dep->number];
	mdwc->original_ep_ops[dep->number] = NULL;
	kfree(old_ep_ops);

	/*
	 * Do HERE more usb endpoint un-configurations
	 * which are specific to MSM.
	 */
	if (!mdwc->dbm /*|| (dep->endpoint.ep_type == EP_TYPE_GSI)*/) {
		spin_unlock_irqrestore(&dwc->lock, flags);
		return 0;
	}

	if (dep->busy_slot == dep->free_slot && list_empty(&dep->request_list)
	    && list_empty(&dep->req_queued)) {
		dev_dbg(mdwc->dev,
			"%s: request is not queued, disable DBM ep for ep %s\n",
			__func__, ep->name);
		/* Unconfigure dbm ep */
		dbm_ep_unconfig(mdwc->dbm, dep->number);

		/*
		 * If this is the last endpoint we unconfigured, than reset also
		 * the event buffers; unless unconfiguring the ep due to lpm,
		 * in which case the event buffer only gets reset during the
		 * block reset.
		 */
		if (dbm_get_num_of_eps_configured(mdwc->dbm) == 0 &&
		    !dbm_reset_ep_after_lpm(mdwc->dbm))
			dbm_event_buffer_config(mdwc->dbm, 0, 0, 0);
	}

	spin_unlock_irqrestore(&dwc->lock, flags);

	return 0;
}
EXPORT_SYMBOL(msm_ep_unconfig);

static int __maybe_unused dwc3_qcom_pm_suspend(struct device *dev)
{
	struct dwc3_qcom *qcom = dev_get_drvdata(dev);
	int ret = 0;

	ret = dwc3_qcom_suspend(qcom);
	if (!ret)
		qcom->pm_suspended = true;

	return ret;
}

static int __maybe_unused dwc3_qcom_pm_resume(struct device *dev)
{
	struct dwc3_qcom *qcom = dev_get_drvdata(dev);
	int ret;

	ret = dwc3_qcom_resume(qcom);
	if (!ret)
		qcom->pm_suspended = false;

	return ret;
}

static int __maybe_unused dwc3_qcom_runtime_suspend(struct device *dev)
{
	struct dwc3_qcom *qcom = dev_get_drvdata(dev);

	return dwc3_qcom_suspend(qcom);
}

static int __maybe_unused dwc3_qcom_runtime_resume(struct device *dev)
{
	struct dwc3_qcom *qcom = dev_get_drvdata(dev);

	return dwc3_qcom_resume(qcom);
}

static const struct dev_pm_ops dwc3_qcom_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(dwc3_qcom_pm_suspend, dwc3_qcom_pm_resume)
	SET_RUNTIME_PM_OPS(dwc3_qcom_runtime_suspend, dwc3_qcom_runtime_resume,
			   NULL)
};

static const struct of_device_id dwc3_qcom_of_match[] = {
	{ .compatible = "qcom,ipq6018-dwc3" },
	{ .compatible = "qcom,ipq5018-dwc3" },
	{ .compatible = "qcom,msm8996-dwc3" },
	{ .compatible = "qcom,sdm845-dwc3" },
	{ }
};
MODULE_DEVICE_TABLE(of, dwc3_qcom_of_match);

static struct platform_driver dwc3_qcom_driver = {
	.probe		= dwc3_qcom_probe,
	.remove		= dwc3_qcom_remove,
	.driver		= {
		.name	= "dwc3-qcom",
		.of_match_table	= dwc3_qcom_of_match,
	},
};

module_platform_driver(dwc3_qcom_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare DWC3 QCOM Glue Driver");
