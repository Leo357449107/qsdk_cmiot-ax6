/*
 * Copyright (c) 2015 Google, Inc
 * Written by Simon Glass <sjg@chromium.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <led.h>
#include <asm/gpio.h>
#include <dm/lists.h>

DECLARE_GLOBAL_DATA_PTR;

struct led_gpio_priv {
	struct gpio_desc gpio;
};

static int gpio_led_set_on(struct udevice *dev, int on)
{
	struct led_gpio_priv *priv = dev_get_priv(dev);

	if (!dm_gpio_is_valid(&priv->gpio))
		return -EREMOTEIO;

	return dm_gpio_set_value(&priv->gpio, on);
}

static int led_gpio_probe(struct udevice *dev)
{
	struct led_uclass_plat *uc_plat = dev_get_uclass_platdata(dev);
	struct led_gpio_priv *priv = dev_get_priv(dev);

	/* Ignore the top-level LED node */
	if (!uc_plat->label)
		return 0;
	return gpio_request_by_name(dev, "gpios", 0, &priv->gpio, GPIOD_IS_OUT);
}

static int led_gpio_remove(struct udevice *dev)
{
	/*
	 * The GPIO driver may have already been removed. We will need to
	 * address this more generally.
	 */
#ifndef CONFIG_SANDBOX
	struct led_gpio_priv *priv = dev_get_priv(dev);

	if (dm_gpio_is_valid(&priv->gpio))
		dm_gpio_free(dev, &priv->gpio);
#endif

	return 0;
}

static int led_gpio_bind(struct udevice *parent)
{
	const void *blob = gd->fdt_blob;
	struct udevice *dev;
	int node;
	int ret;

	for (node = fdt_first_subnode(blob, parent->of_offset);
	     node > 0;
	     node = fdt_next_subnode(blob, node)) {
		struct led_uclass_plat *uc_plat;
		const char *label;

		label = fdt_getprop(blob, node, "label", NULL);
		if (!label) {
			debug("%s: node %s has no label\n", __func__,
			      fdt_get_name(blob, node, NULL));
			return -EINVAL;
		}
		ret = device_bind_driver_to_node(parent, "gpio_led",
						 fdt_get_name(blob, node, NULL),
						 node, &dev);
		if (ret)
			return ret;
		uc_plat = dev_get_uclass_platdata(dev);
		uc_plat->label = label;
	}

	return 0;
}

static const struct led_ops gpio_led_ops = {
	.set_on		= gpio_led_set_on,
};

static const struct udevice_id led_gpio_ids[] = {
	{ .compatible = "gpio-leds" },
	{ }
};

U_BOOT_DRIVER(led_gpio) = {
	.name	= "gpio_led",
	.id	= UCLASS_LED,
	.of_match = led_gpio_ids,
	.ops	= &gpio_led_ops,
	.priv_auto_alloc_size = sizeof(struct led_gpio_priv),
	.bind	= led_gpio_bind,
	.probe	= led_gpio_probe,
	.remove	= led_gpio_remove,
};
