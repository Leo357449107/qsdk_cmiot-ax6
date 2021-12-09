/*
 * Copyright (C) 2015
 * Bhuvanchandra DV, Toradex, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <fdtdec.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/io.h>
#include <malloc.h>

DECLARE_GLOBAL_DATA_PTR;

struct vybrid_gpios {
	unsigned int chip;
	struct vybrid_gpio_regs *reg;
};

static int vybrid_gpio_direction_input(struct udevice *dev, unsigned gpio)
{
	const struct vybrid_gpios *gpios = dev_get_priv(dev);

	gpio = gpio + (gpios->chip * VYBRID_GPIO_COUNT);
	imx_iomux_gpio_set_direction(gpio, VF610_GPIO_DIRECTION_IN);

	return 0;
}

static int vybrid_gpio_direction_output(struct udevice *dev, unsigned gpio,
					 int value)
{
	const struct vybrid_gpios *gpios = dev_get_priv(dev);

	gpio = gpio + (gpios->chip * VYBRID_GPIO_COUNT);
	gpio_set_value(gpio, value);
	imx_iomux_gpio_set_direction(gpio, VF610_GPIO_DIRECTION_OUT);

	return 0;
}

static int vybrid_gpio_get_value(struct udevice *dev, unsigned gpio)
{
	const struct vybrid_gpios *gpios = dev_get_priv(dev);

	return ((readl(&gpios->reg->gpio_pdir) & (1 << gpio))) ? 1 : 0;
}

static int vybrid_gpio_set_value(struct udevice *dev, unsigned gpio,
				  int value)
{
	const struct vybrid_gpios *gpios = dev_get_priv(dev);
	if (value)
		writel((1 << gpio), &gpios->reg->gpio_psor);
	else
		writel((1 << gpio), &gpios->reg->gpio_pcor);

	return 0;
}

static int vybrid_gpio_get_function(struct udevice *dev, unsigned gpio)
{
	const struct vybrid_gpios *gpios = dev_get_priv(dev);
	u32 g_state = 0;

	gpio = gpio + (gpios->chip * VYBRID_GPIO_COUNT);

	imx_iomux_gpio_get_function(gpio, &g_state);

	if (((g_state & (0x07 << PAD_MUX_MODE_SHIFT)) >> PAD_MUX_MODE_SHIFT) > 0)
		return GPIOF_FUNC;
	if (g_state & PAD_CTL_OBE_ENABLE)
		return GPIOF_OUTPUT;
	if (g_state & PAD_CTL_IBE_ENABLE)
		return GPIOF_INPUT;
	if (!(g_state & PAD_CTL_OBE_IBE_ENABLE))
		return GPIOF_UNUSED;

	return GPIOF_UNKNOWN;
}

static const struct dm_gpio_ops gpio_vybrid_ops = {
	.direction_input	= vybrid_gpio_direction_input,
	.direction_output	= vybrid_gpio_direction_output,
	.get_value		= vybrid_gpio_get_value,
	.set_value		= vybrid_gpio_set_value,
	.get_function		= vybrid_gpio_get_function,
};

static int vybrid_gpio_probe(struct udevice *dev)
{
	struct vybrid_gpios *gpios = dev_get_priv(dev);
	struct vybrid_gpio_platdata *plat = dev_get_platdata(dev);
	struct gpio_dev_priv *uc_priv = dev_get_uclass_priv(dev);

	uc_priv->bank_name = plat->port_name;
	uc_priv->gpio_count = VYBRID_GPIO_COUNT;
	gpios->reg = (struct vybrid_gpio_regs *)plat->base;
	gpios->chip = plat->chip;

	return 0;
}

static int vybrid_gpio_bind(struct udevice *dev)
{
	struct vybrid_gpio_platdata *plat = dev->platdata;
	fdt_addr_t base_addr;

	if (plat)
		return 0;

	base_addr = dev_get_addr(dev);
	if (base_addr == FDT_ADDR_T_NONE)
		return -ENODEV;

	/*
	* TODO:
	* When every board is converted to driver model and DT is
	* supported, this can be done by auto-alloc feature, but
	* not using calloc to alloc memory for platdata.
	*/
	plat = calloc(1, sizeof(*plat));
	if (!plat)
		return -ENOMEM;

	plat->base = base_addr;
	plat->chip = dev->req_seq;
	plat->port_name = fdt_get_name(gd->fdt_blob, dev->of_offset, NULL);
	dev->platdata = plat;

	return 0;
}

#if !CONFIG_IS_ENABLED(OF_CONTROL)
static const struct vybrid_gpio_platdata vybrid_gpio[] = {
	{0, GPIO0_BASE_ADDR, "GPIO0 "},
	{1, GPIO1_BASE_ADDR, "GPIO1 "},
	{2, GPIO2_BASE_ADDR, "GPIO2 "},
	{3, GPIO3_BASE_ADDR, "GPIO3 "},
	{4, GPIO4_BASE_ADDR, "GPIO4 "},
};

U_BOOT_DEVICES(vybrid_gpio) = {
	{ "gpio_vybrid", &vybrid_gpio[0] },
	{ "gpio_vybrid", &vybrid_gpio[1] },
	{ "gpio_vybrid", &vybrid_gpio[2] },
	{ "gpio_vybrid", &vybrid_gpio[3] },
	{ "gpio_vybrid", &vybrid_gpio[4] },
};
#endif

static const struct udevice_id vybrid_gpio_ids[] = {
	{ .compatible = "fsl,vf610-gpio" },
	{ }
};

U_BOOT_DRIVER(gpio_vybrid) = {
	.name	= "gpio_vybrid",
	.id	= UCLASS_GPIO,
	.ops	= &gpio_vybrid_ops,
	.probe	= vybrid_gpio_probe,
	.priv_auto_alloc_size = sizeof(struct vybrid_gpios),
	.of_match = vybrid_gpio_ids,
	.bind	= vybrid_gpio_bind,
};
