/*
 * Copyright (C) 2014-2015 Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <linux/types.h>
#include <linux/io.h>
#include <asm/errno.h>
#include <dm/device.h>
#include <dm/root.h>
#include <i2c.h>
#include <fdtdec.h>
#include <mapmem.h>

DECLARE_GLOBAL_DATA_PTR;

struct uniphier_i2c_regs {
	u32 dtrm;			/* data transmission */
#define I2C_DTRM_STA	(1 << 10)
#define I2C_DTRM_STO	(1 << 9)
#define I2C_DTRM_NACK	(1 << 8)
#define I2C_DTRM_RD	(1 << 0)
	u32 drec;			/* data reception */
#define I2C_DREC_STS	(1 << 12)
#define I2C_DREC_LRB	(1 << 11)
#define I2C_DREC_LAB	(1 << 9)
	u32 myad;			/* slave address */
	u32 clk;			/* clock frequency control */
	u32 brst;			/* bus reset */
#define I2C_BRST_FOEN	(1 << 1)
#define I2C_BRST_BRST	(1 << 0)
	u32 hold;			/* hold time control */
	u32 bsts;			/* bus status monitor */
	u32 noise;			/* noise filter control */
	u32 setup;			/* setup time control */
};

#define IOBUS_FREQ	100000000

struct uniphier_i2c_dev {
	struct uniphier_i2c_regs __iomem *regs;	/* register base */
	unsigned long input_clk;	/* master clock (Hz) */
	unsigned long wait_us;		/* wait for every byte transfer (us) */
};

static int uniphier_i2c_probe(struct udevice *dev)
{
	fdt_addr_t addr;
	fdt_size_t size;
	struct uniphier_i2c_dev *priv = dev_get_priv(dev);

	addr = fdtdec_get_addr_size(gd->fdt_blob, dev->of_offset, "reg", &size);

	priv->regs = map_sysmem(addr, size);

	if (!priv->regs)
		return -ENOMEM;

	priv->input_clk = IOBUS_FREQ;

	/* deassert reset */
	writel(0x3, &priv->regs->brst);

	return 0;
}

static int uniphier_i2c_remove(struct udevice *dev)
{
	struct uniphier_i2c_dev *priv = dev_get_priv(dev);

	unmap_sysmem(priv->regs);

	return 0;
}

static int send_and_recv_byte(struct uniphier_i2c_dev *dev, u32 dtrm)
{
	writel(dtrm, &dev->regs->dtrm);

	/*
	 * This controller only provides interruption to inform the completion
	 * of each byte transfer.  (No status register to poll it.)
	 * Unfortunately, U-Boot does not have a good support of interrupt.
	 * Wait for a while.
	 */
	udelay(dev->wait_us);

	return readl(&dev->regs->drec);
}

static int send_byte(struct uniphier_i2c_dev *dev, u32 dtrm, bool *stop)
{
	int ret = 0;
	u32 drec;

	drec = send_and_recv_byte(dev, dtrm);

	if (drec & I2C_DREC_LAB) {
		debug("uniphier_i2c: bus arbitration failed\n");
		*stop = false;
		ret = -EREMOTEIO;
	}
	if (drec & I2C_DREC_LRB) {
		debug("uniphier_i2c: slave did not return ACK\n");
		ret = -EREMOTEIO;
	}
	return ret;
}

static int uniphier_i2c_transmit(struct uniphier_i2c_dev *dev, uint addr,
				 uint len, const u8 *buf, bool *stop)
{
	int ret;

	debug("%s: addr = %x, len = %d\n", __func__, addr, len);

	ret = send_byte(dev, I2C_DTRM_STA | I2C_DTRM_NACK | addr << 1, stop);
	if (ret < 0)
		goto fail;

	while (len--) {
		ret = send_byte(dev, I2C_DTRM_NACK | *buf++, stop);
		if (ret < 0)
			goto fail;
	}

fail:
	if (*stop)
		writel(I2C_DTRM_STO | I2C_DTRM_NACK, &dev->regs->dtrm);

	return ret;
}

static int uniphier_i2c_receive(struct uniphier_i2c_dev *dev, uint addr,
				uint len, u8 *buf, bool *stop)
{
	int ret;

	debug("%s: addr = %x, len = %d\n", __func__, addr, len);

	ret = send_byte(dev, I2C_DTRM_STA | I2C_DTRM_NACK |
			I2C_DTRM_RD | addr << 1, stop);
	if (ret < 0)
		goto fail;

	while (len--)
		*buf++ = send_and_recv_byte(dev, len ? 0 : I2C_DTRM_NACK);

fail:
	if (*stop)
		writel(I2C_DTRM_STO | I2C_DTRM_NACK, &dev->regs->dtrm);

	return ret;
}

static int uniphier_i2c_xfer(struct udevice *bus, struct i2c_msg *msg,
			     int nmsgs)
{
	int ret = 0;
	struct uniphier_i2c_dev *dev = dev_get_priv(bus);
	bool stop;

	for (; nmsgs > 0; nmsgs--, msg++) {
		/* If next message is read, skip the stop condition */
		stop = nmsgs > 1 && msg[1].flags & I2C_M_RD ? false : true;

		if (msg->flags & I2C_M_RD)
			ret = uniphier_i2c_receive(dev, msg->addr, msg->len,
						   msg->buf, &stop);
		else
			ret = uniphier_i2c_transmit(dev, msg->addr, msg->len,
						    msg->buf, &stop);

		if (ret < 0)
			break;
	}

	return ret;
}

static int uniphier_i2c_set_bus_speed(struct udevice *bus, unsigned int speed)
{
	struct uniphier_i2c_dev *priv = dev_get_priv(bus);

	/* max supported frequency is 400 kHz */
	if (speed > 400000)
		return -EINVAL;

	/* bus reset: make sure the bus is idle when change the frequency */
	writel(0x1, &priv->regs->brst);

	writel((priv->input_clk / speed / 2 << 16) | (priv->input_clk / speed),
	       &priv->regs->clk);

	writel(0x3, &priv->regs->brst);

	/*
	 * Theoretically, each byte can be transferred in
	 * 1000000 * 9 / speed usec.  For safety, wait more than double.
	 */
	priv->wait_us = 20000000 / speed;

	return 0;
}


static const struct dm_i2c_ops uniphier_i2c_ops = {
	.xfer = uniphier_i2c_xfer,
	.set_bus_speed = uniphier_i2c_set_bus_speed,
};

static const struct udevice_id uniphier_i2c_of_match[] = {
	{ .compatible = "socionext,uniphier-i2c" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(uniphier_i2c) = {
	.name = "uniphier-i2c",
	.id = UCLASS_I2C,
	.of_match = uniphier_i2c_of_match,
	.probe = uniphier_i2c_probe,
	.remove = uniphier_i2c_remove,
	.priv_auto_alloc_size = sizeof(struct uniphier_i2c_dev),
	.ops = &uniphier_i2c_ops,
};
