/* Copyright (c) 2018 The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/firmware.h>
#include <linux/bitops.h>
#include "remoteproc_internal.h"
#include <linux/qcom_scm.h>
#include <linux/elf.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include "rproc_mdt_loader.h"

#define subsys_to_pdata(d) container_of(d, struct m3_rproc_pdata, subsys_desc)

struct m3_rproc_pdata {
	struct rproc *rproc;
	struct subsys_device *subsys;
	struct subsys_desc subsys_desc;
	int missing_m3;
	int secure;
};

static struct m3_rproc_pdata *m3_rproc_pdata;

struct m3_rtable {
	struct resource_table rtable;
	struct fw_rsc_hdr last_hdr;
};

static struct m3_rtable m3_rtable = {
	.rtable = {
		.ver = 1,
		.num = 0,
	},
	.last_hdr = {
		.type = RSC_LAST,
	},
};


static struct resource_table *m3_find_loaded_rsc_table(struct rproc *rproc,
	const struct firmware *fw)
{
	return &(m3_rtable.rtable);
}

static void *m3_da_to_va(struct rproc *rproc, u64 da, int len)
{
	unsigned long addr = (__force unsigned long)(da & 0xFFFFFFFF);

	return ioremap(addr, len);
}

static int m3_rproc_start(struct rproc *rproc)
{
	return 0;
}

static int m3_rproc_stop(struct rproc *rproc)
{
	return 0;
}

static int m3_load(struct rproc *rproc, const struct firmware *fw)
{
	pr_info("Sanity check passed for M3 image\n");

	return mdt_load(rproc, fw);
}

static int start_m3(const struct subsys_desc *subsys)
{
	struct m3_rproc_pdata *pdata = subsys_to_pdata(subsys);
	struct rproc *rproc = pdata->rproc;
	int ret = 0;

	if (pdata->missing_m3) {
		pr_emerg("m3_fw.mdt missing, assuming combined image\n");
		return 0;
	}

	ret = rproc_boot(rproc);
	if (ret) {
		pdata->missing_m3 = 1;
		pr_emerg("m3_fw.mdt missing, assuming combined image\n");
		return 0;
	}

	return ret;
}

static int stop_m3(const struct subsys_desc *subsys, bool force_stop)
{
	struct m3_rproc_pdata *pdata = subsys_to_pdata(subsys);
	struct rproc *rproc = pdata->rproc;

	if (pdata->missing_m3)
		return 0;

	rproc_shutdown(rproc);
	return 0;
}

static struct rproc_ops m3_rproc_ops = {
	.start          = m3_rproc_start,
	.da_to_va       = m3_da_to_va,
	.stop           = m3_rproc_stop,
	.find_loaded_rsc_table = m3_find_loaded_rsc_table,
	.load = m3_load,
};

static int m3_rproc_probe(struct platform_device *pdev)
{
	const char *firmware_name;
	struct rproc *rproc;
	int ret;

	ret = dma_set_coherent_mask(&pdev->dev,
			DMA_BIT_MASK(sizeof(dma_addr_t) * 8));
	if (ret) {
		dev_err(&pdev->dev, "dma_set_coherent_mask: %d\n", ret);
		return ret;
	}

	ret = of_property_read_string(pdev->dev.of_node, "firmware",
			&firmware_name);
	if (ret) {
		dev_err(&pdev->dev, "couldn't read firmware name: %d\n", ret);
		return ret;
	}

	rproc = rproc_alloc(&pdev->dev, "q6v5-m3", &m3_rproc_ops,
				firmware_name, sizeof(*m3_rproc_pdata));
	if (unlikely(!rproc))
		return -ENOMEM;

	m3_rproc_pdata = rproc->priv;
	m3_rproc_pdata->rproc = rproc;

	m3_rproc_pdata->secure = of_property_read_bool(pdev->dev.of_node,
					"qca,secure");

	rproc->has_iommu = false;

	m3_rproc_pdata->subsys_desc.is_not_loadable = 0;
	m3_rproc_pdata->subsys_desc.name = "q6v5-m3";
	m3_rproc_pdata->subsys_desc.dev = &pdev->dev;
	m3_rproc_pdata->subsys_desc.owner = THIS_MODULE;
	m3_rproc_pdata->subsys_desc.shutdown = stop_m3;
	m3_rproc_pdata->subsys_desc.powerup = start_m3;

	m3_rproc_pdata->subsys = subsys_register(
					&m3_rproc_pdata->subsys_desc);
	if (IS_ERR(m3_rproc_pdata->subsys)) {
		ret = PTR_ERR(m3_rproc_pdata->subsys);
		goto free_rproc;
	}

	rproc->auto_boot = false;
	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	return 0;

free_rproc:

	rproc_free(rproc);
	return -EIO;
}

static int m3_rproc_remove(struct platform_device *pdev)
{
	struct m3_rproc_pdata *pdata;
	struct rproc *rproc;

	pdata = platform_get_drvdata(pdev);
	rproc = m3_rproc_pdata->rproc;

	rproc_del(rproc);
	rproc_free(rproc);

	subsys_unregister(pdata->subsys);

	return 0;
}


static const struct of_device_id m3_match_table[] = {
	{ .compatible = "qca,q6v5-m3-rproc" },
	{}
};

static struct platform_driver m3_rproc_driver = {
	.probe = m3_rproc_probe,
	.remove = m3_rproc_remove,
	.driver = {
		.name = "q6v5-m3",
		.of_match_table = m3_match_table,
		.owner = THIS_MODULE,
	},
};

module_platform_driver(m3_rproc_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("QCA Remote Processor control driver");
