/**
 * dwc3-of-simple.c - OF glue layer for simple integrations
 *
 * Copyright (c) 2015 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * This is a combination of the old dwc3-qcom.c by Ivan T. Ivanov
 * <iivanov@mm-sol.com> and the original patch adding support for Xilinx' SoC
 * by Subbaraya Sundeep Bhatta <subbaraya.sundeep.bhatta@xilinx.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/reset.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/usb/msm_hsusb.h>
#include "gadget.h"
#include "dbm.h"

struct dwc3_of_simple {
	struct device		*dev;
	struct clk		**clks;
	int			num_clocks;
	struct reset_control	*mstr_rst;

	void __iomem		*qscratch_base;
	void __iomem		*dwc3_base;
	struct dbm		*dbm;
	const struct usb_ep_ops *original_ep_ops[DWC3_ENDPOINTS_NUM];
	struct list_head req_complete_list;
};

#define USB30_QSCRATCH_GENERAL_CFG_OFFSET    (0x08)
#define PIPE_UTMI_CLK_SEL			BIT(0)
#define PIPE3_PHYSTATUS_SW			BIT(3)
#define PIPE_UTMI_CLK_DIS			BIT(8)
#define DSTS_CONNECTSPD_SS              0x4
struct dwc3_msm_req_complete {
	struct list_head list_item;
	struct usb_request *req;
	void (*orig_complete)(struct usb_ep *ep,
			      struct usb_request *req);
};

static void dwc3_qcom_disable_ss(struct dwc3_of_simple *qcom)
{
	/* Configure dwc3 to use UTMI clock as PIPE clock not present */
	writel((readl(qcom->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET) | PIPE_UTMI_CLK_DIS),
			qcom->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET);

	usleep_range(100, 1000);

	writel((readl(qcom->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET) | PIPE_UTMI_CLK_SEL |
		PIPE3_PHYSTATUS_SW),qcom->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET);

	usleep_range(100, 1000);

	writel((readl(qcom->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET) & ~PIPE_UTMI_CLK_DIS),
			qcom->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET);
}

static int dwc3_of_simple_probe(struct platform_device *pdev)
{
	struct dwc3_of_simple	*simple;
	struct device		*dev = &pdev->dev;
	struct device_node	*np = dev->of_node;
	struct resource		*res;

	unsigned int		count;
	int			ret;
	int			i;
	bool			ignore_pipe_clk;

	simple = devm_kzalloc(dev, sizeof(*simple), GFP_KERNEL);
	if (!simple)
		return -ENOMEM;

	platform_set_drvdata(pdev, simple);

	count = of_clk_get_parent_count(np);
	if (!count)
		return -ENOENT;

	simple->num_clocks = count;

	simple->clks = devm_kcalloc(dev, simple->num_clocks,
			sizeof(struct clk *), GFP_KERNEL);
	if (!simple->clks)
		return -ENOMEM;

	simple->dev = dev;

	for (i = 0; i < simple->num_clocks; i++) {
		struct clk	*clk;

		clk = of_clk_get(np, i);
		if (IS_ERR(clk)) {
			while (--i >= 0)
				clk_put(simple->clks[i]);
			return PTR_ERR(clk);
		}

		ret = clk_prepare_enable(clk);
		if (ret < 0) {
			while (--i >= 0) {
				clk_disable_unprepare(simple->clks[i]);
				clk_put(simple->clks[i]);
			}
			clk_put(clk);

			return ret;
		}

		simple->clks[i] = clk;
	}

	simple->mstr_rst = devm_reset_control_get(dev, "usb30_mstr_rst");

	if (!IS_ERR(simple->mstr_rst))
		reset_control_deassert(simple->mstr_rst);
	else
		dev_dbg(simple->dev, "cannot get handle for master reset control\n");

	if (of_get_property(pdev->dev.of_node, "qcom,usb-dbm", NULL)) {
		simple->dbm = usb_get_dbm_by_phandle(&pdev->dev, "qcom,usb-dbm");
		if (IS_ERR(simple->dbm)) {
			dev_err(&pdev->dev, "unable to get dbm device\n");
			ret = -EPROBE_DEFER;
			return ret;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"qscratch_base");
	if (res) {
		simple->qscratch_base = devm_ioremap_nocache(dev, res->start,
							resource_size(res));
		if (IS_ERR(simple->qscratch_base)) {
			dev_err(dev, "couldn't ioremap qscratch_base\n");
			simple->qscratch_base = NULL;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"dwc3_base");
	if (res) {
		simple->dwc3_base = devm_ioremap_nocache(dev, res->start,
							resource_size(res));
		if (IS_ERR(simple->dwc3_base)) {
			dev_err(dev, "couldn't ioremap dwc3_base\n");
			simple->dwc3_base = NULL;
		}
	}
	/*
	 * Disable pipe_clk requirement if specified. Used when dwc3
	 * operates without SSPHY and only HS/FS/LS modes are supported.
	 */
	ignore_pipe_clk = device_property_read_bool(dev,
				"qcom,disable-ss");
	if (ignore_pipe_clk)
		dwc3_qcom_disable_ss(simple);

	INIT_LIST_HEAD(&simple->req_complete_list);

	ret = of_platform_populate(np, NULL, NULL, dev);
	if (ret) {
		for (i = 0; i < simple->num_clocks; i++) {
			clk_disable_unprepare(simple->clks[i]);
			clk_put(simple->clks[i]);
		}

		return ret;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_get_sync(dev);

	return 0;
}

static inline dma_addr_t dwc3_trb_dma_offset(struct dwc3_ep *dep,
					     struct dwc3_trb *trb)
{
	u32	offset = (char *) trb - (char *) dep->trb_pool;

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

static inline bool dwc3_msm_is_dev_superspeed(struct dwc3_of_simple *mdwc)
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
	struct dwc3_of_simple *mdwc = dev_get_drvdata(dwc->dev->parent);

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
	struct dwc3_of_simple *mdwc = dev_get_drvdata(dwc->dev->parent);
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
	if (0 == dbm_get_num_of_eps_configured(mdwc->dbm) &&
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
static int __dwc3_msm_dbm_ep_reset(struct dwc3_of_simple *mdwc, struct dwc3_ep *dep)
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
	struct dwc3_of_simple *mdwc = dev_get_drvdata(dwc->dev->parent);

	return __dwc3_msm_dbm_ep_reset(mdwc, dep);
}
EXPORT_SYMBOL(msm_dwc3_reset_dbm_ep);


static u32 dwc3_msm_gadget_ep_get_transfer_index(struct dwc3 *dwc, u8 number)
{
	u32	res_id;
	u32	offs = DWC3_DEPCMD(number) - DWC3_GLOBALS_REGS_START;

	res_id = readl(dwc->regs + offs);

	return DWC3_DEPCMD_GET_RSC_IDX(res_id);
}

int dwc3_msm_send_gadget_ep_cmd(struct dwc3 *dwc, unsigned ep,
				unsigned cmd, struct dwc3_gadget_ep_cmd_params *params)
{
	u32                     timeout = 500;
	u32                     reg;

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
		if (!timeout) {
			return -ETIMEDOUT;
		}

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
	memset(trb_link, 0, sizeof *trb_link);

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
	struct dwc3_of_simple *mdwc = dev_get_drvdata(dwc->dev->parent);
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
	struct dwc3_of_simple *mdwc = dev_get_drvdata(dwc->dev->parent);
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
	writel( (readl(mdwc->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET) | 0x2),
		mdwc->qscratch_base+USB30_QSCRATCH_GENERAL_CFG_OFFSET );
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

	if (!mdwc->dbm || !request /*|| (dep->endpoint.ep_type == EP_TYPE_GSI)*/) {
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

	pr_info("%s: dep->number=0x%x, ioc=0x%x, bam_pipe=0x%x\n",__func__,dep->number, ioc,bam_pipe);

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
	struct dwc3_of_simple *mdwc = dev_get_drvdata(dwc->dev->parent);
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

static int dwc3_of_simple_remove(struct platform_device *pdev)
{
	struct dwc3_of_simple	*simple = platform_get_drvdata(pdev);
	struct device		*dev = &pdev->dev;
	int			i;

	if (!IS_ERR(simple->mstr_rst))
		reset_control_assert(simple->mstr_rst);

	of_platform_depopulate(dev);

	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);

	for (i = (simple->num_clocks-1); i >= 0; i--) {
		clk_disable_unprepare(simple->clks[i]);
		clk_put(simple->clks[i]);
	}

	return 0;
}

#ifdef CONFIG_PM
static int dwc3_of_simple_runtime_suspend(struct device *dev)
{
	struct dwc3_of_simple	*simple = dev_get_drvdata(dev);
	int			i;

	for (i = 0; i < simple->num_clocks; i++)
		clk_disable(simple->clks[i]);

	return 0;
}

static int dwc3_of_simple_runtime_resume(struct device *dev)
{
	struct dwc3_of_simple	*simple = dev_get_drvdata(dev);
	int			ret;
	int			i;

	for (i = 0; i < simple->num_clocks; i++) {
		ret = clk_enable(simple->clks[i]);
		if (ret < 0) {
			while (--i >= 0)
				clk_disable(simple->clks[i]);
			return ret;
		}
	}

	return 0;
}
#endif

static const struct dev_pm_ops dwc3_of_simple_dev_pm_ops = {
	SET_RUNTIME_PM_OPS(dwc3_of_simple_runtime_suspend,
			dwc3_of_simple_runtime_resume, NULL)
};

static const struct of_device_id of_dwc3_simple_match[] = {
	{ .compatible = "qcom,dwc3" },
	{ .compatible = "xlnx,zynqmp-dwc3" },
	{ /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_dwc3_simple_match);

static struct platform_driver dwc3_of_simple_driver = {
	.probe		= dwc3_of_simple_probe,
	.remove		= dwc3_of_simple_remove,
	.driver		= {
		.name	= "dwc3-of-simple",
		.of_match_table = of_dwc3_simple_match,
	},
};

module_platform_driver(dwc3_of_simple_driver);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("DesignWare USB3 OF Simple Glue Layer");
MODULE_AUTHOR("Felipe Balbi <balbi@ti.com>");
