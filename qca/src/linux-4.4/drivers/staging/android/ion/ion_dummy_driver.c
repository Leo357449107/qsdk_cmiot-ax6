/*
 * drivers/gpu/ion/ion_dummy_driver.c
 *
 * Copyright (C) 2013 Linaro, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/bootmem.h>
#include <linux/memblock.h>
#include <linux/sizes.h>
#include <linux/io.h>
#include "ion.h"
#include "ion_priv.h"

static struct ion_device *idev;
static struct ion_heap **heaps;

static void *carveout_ptr;
static void *chunk_ptr;

static struct ion_platform_heap dummy_heaps[] = {
		{
			.id	= ION_HEAP_TYPE_SYSTEM,
			.type	= ION_HEAP_TYPE_SYSTEM,
			.name	= "system",
		},
		{
			.id	= ION_HEAP_TYPE_SYSTEM_CONTIG,
			.type	= ION_HEAP_TYPE_SYSTEM_CONTIG,
			.name	= "system contig",
		},
		{
			.id	= ION_HEAP_TYPE_CARVEOUT,
			.type	= ION_HEAP_TYPE_CARVEOUT,
			.name	= "carveout",
			.size	= SZ_4M,
		},
		{
			.id	= ION_HEAP_TYPE_CHUNK,
			.type	= ION_HEAP_TYPE_CHUNK,
			.name	= "chunk",
			.size	= SZ_4M,
			.align	= SZ_16K,
		},
};

static struct ion_platform_data dummy_ion_pdata = {
	.nr = ARRAY_SIZE(dummy_heaps),
	.heaps = dummy_heaps,
};

static int __init ion_dummy_init(struct platform_device *pdev)
{
	int i, err;

	idev = ion_device_create(NULL);
	heaps = kcalloc(dummy_ion_pdata.nr, sizeof(struct ion_heap *),
			GFP_KERNEL);
	if (!heaps)
		return -ENOMEM;


	/* Allocate a dummy carveout heap */
	carveout_ptr = alloc_pages_exact(
				dummy_heaps[ION_HEAP_TYPE_CARVEOUT].size,
				GFP_KERNEL);
	if (carveout_ptr)
		dummy_heaps[ION_HEAP_TYPE_CARVEOUT].base =
						virt_to_phys(carveout_ptr);
	else
		pr_err("ion_dummy: Could not allocate carveout\n");

	/* Allocate a dummy chunk heap */
	chunk_ptr = alloc_pages_exact(
				dummy_heaps[ION_HEAP_TYPE_CHUNK].size,
				GFP_KERNEL);
	if (chunk_ptr)
		dummy_heaps[ION_HEAP_TYPE_CHUNK].base = virt_to_phys(chunk_ptr);
	else
		pr_err("ion_dummy: Could not allocate chunk\n");

	for (i = 0; i < dummy_ion_pdata.nr; i++) {
		struct ion_platform_heap *heap_data = &dummy_ion_pdata.heaps[i];

		heap_data->priv = &pdev->dev;

		if (heap_data->type == ION_HEAP_TYPE_CARVEOUT &&
							!heap_data->base)
			continue;

		if (heap_data->type == ION_HEAP_TYPE_CHUNK && !heap_data->base)
			continue;

		heaps[i] = ion_heap_create(heap_data);
		if (IS_ERR_OR_NULL(heaps[i])) {
			err = PTR_ERR(heaps[i]);
			if (err == -EINVAL) {
				pr_info("%s: Heap type is disabled: %d\n",
					__func__, heap_data->type);
				continue;
			} else {
				goto err;
			}
		}
		ion_device_add_heap(idev, heaps[i]);
	}
	return 0;
err:
	for (i = 0; i < dummy_ion_pdata.nr; ++i)
		ion_heap_destroy(heaps[i]);
	kfree(heaps);

	if (carveout_ptr) {
		free_pages_exact(carveout_ptr,
				dummy_heaps[ION_HEAP_TYPE_CARVEOUT].size);
		carveout_ptr = NULL;
	}
	if (chunk_ptr) {
		free_pages_exact(chunk_ptr,
				dummy_heaps[ION_HEAP_TYPE_CHUNK].size);
		chunk_ptr = NULL;
	}
	return err;
}

static int __exit ion_dummy_exit(void)
{
	int i;

	ion_device_destroy(idev);

	for (i = 0; i < dummy_ion_pdata.nr; i++)
		ion_heap_destroy(heaps[i]);
	kfree(heaps);

	if (carveout_ptr) {
		free_pages_exact(carveout_ptr,
				dummy_heaps[ION_HEAP_TYPE_CARVEOUT].size);
		carveout_ptr = NULL;
	}
	if (chunk_ptr) {
		free_pages_exact(chunk_ptr,
				dummy_heaps[ION_HEAP_TYPE_CHUNK].size);
		chunk_ptr = NULL;
	}

	return 0;
}

static const struct of_device_id qcom_ion_dt_match[] = {
	{ .compatible = "ipq60xx-ion-dummy",},
	{}
};
MODULE_DEVICE_TABLE(of, qcom_ion_dt_match);

static struct platform_driver qcom_ion_dummy_driver = {
	.probe = ion_dummy_init,
	.remove = ion_dummy_exit,
	.driver = {
		.name   = KBUILD_MODNAME,
		.of_match_table = qcom_ion_dt_match,
	},
};
module_platform_driver(qcom_ion_dummy_driver);
