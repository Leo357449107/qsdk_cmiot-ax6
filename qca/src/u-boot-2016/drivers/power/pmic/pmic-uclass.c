/*
 * Copyright (C) 2014-2015 Samsung Electronics
 * Przemyslaw Marczak <p.marczak@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <fdtdec.h>
#include <errno.h>
#include <dm.h>
#include <vsprintf.h>
#include <dm/lists.h>
#include <dm/device-internal.h>
#include <dm/uclass-internal.h>
#include <power/pmic.h>
#include <linux/ctype.h>

DECLARE_GLOBAL_DATA_PTR;

int pmic_bind_children(struct udevice *pmic, int offset,
		       const struct pmic_child_info *child_info)
{
	const struct pmic_child_info *info;
	const void *blob = gd->fdt_blob;
	struct driver *drv;
	struct udevice *child;
	const char *node_name;
	int bind_count = 0;
	int node;
	int prefix_len;
	int ret;

	debug("%s for '%s' at node offset: %d\n", __func__, pmic->name,
	      pmic->of_offset);

	for (node = fdt_first_subnode(blob, offset);
	     node > 0;
	     node = fdt_next_subnode(blob, node)) {
		node_name = fdt_get_name(blob, node, NULL);

		debug("* Found child node: '%s' at offset:%d\n", node_name,
								 node);

		child = NULL;
		for (info = child_info; info->prefix && info->driver; info++) {
			debug("  - compatible prefix: '%s'\n", info->prefix);

			prefix_len = strlen(info->prefix);
			if (strncmp(info->prefix, node_name, prefix_len))
				continue;

			drv = lists_driver_lookup_name(info->driver);
			if (!drv) {
				debug("  - driver: '%s' not found!\n",
				      info->driver);
				continue;
			}

			debug("  - found child driver: '%s'\n", drv->name);

			ret = device_bind(pmic, drv, node_name, NULL,
					  node, &child);
			if (ret) {
				debug("  - child binding error: %d\n", ret);
				continue;
			}

			debug("  - bound child device: '%s'\n", child->name);

			child->driver_data = trailing_strtol(node_name);

			debug("  - set 'child->driver_data': %lu\n",
			      child->driver_data);
			break;
		}

		if (child)
			bind_count++;
		else
			debug("  - compatible prefix not found\n");
	}

	debug("Bound: %d childs for PMIC: '%s'\n", bind_count, pmic->name);
	return bind_count;
}

int pmic_get(const char *name, struct udevice **devp)
{
	return uclass_get_device_by_name(UCLASS_PMIC, name, devp);
}

int pmic_reg_count(struct udevice *dev)
{
	const struct dm_pmic_ops *ops = dev_get_driver_ops(dev);

	if (!ops || !ops->reg_count)
		return -ENOSYS;

	return ops->reg_count(dev);
}

int pmic_read(struct udevice *dev, uint reg, uint8_t *buffer, int len)
{
	const struct dm_pmic_ops *ops = dev_get_driver_ops(dev);

	if (!buffer)
		return -EFAULT;

	if (!ops || !ops->read)
		return -ENOSYS;

	return ops->read(dev, reg, buffer, len);
}

int pmic_write(struct udevice *dev, uint reg, const uint8_t *buffer, int len)
{
	const struct dm_pmic_ops *ops = dev_get_driver_ops(dev);

	if (!buffer)
		return -EFAULT;

	if (!ops || !ops->write)
		return -ENOSYS;

	return ops->write(dev, reg, buffer, len);
}

int pmic_reg_read(struct udevice *dev, uint reg)
{
	u8 byte;
	int ret;

	ret = pmic_read(dev, reg, &byte, 1);
	debug("%s: reg=%x, value=%x\n", __func__, reg, byte);

	return ret ? ret : byte;
}

int pmic_reg_write(struct udevice *dev, uint reg, uint value)
{
	u8 byte = value;

	debug("%s: reg=%x, value=%x\n", __func__, reg, value);
	return pmic_write(dev, reg, &byte, 1);
}

int pmic_clrsetbits(struct udevice *dev, uint reg, uint clr, uint set)
{
	u8 byte;
	int ret;

	ret = pmic_reg_read(dev, reg);
	if (ret < 0)
		return ret;
	byte = (ret & ~clr) | set;

	return pmic_reg_write(dev, reg, byte);
}

UCLASS_DRIVER(pmic) = {
	.id		= UCLASS_PMIC,
	.name		= "pmic",
};
