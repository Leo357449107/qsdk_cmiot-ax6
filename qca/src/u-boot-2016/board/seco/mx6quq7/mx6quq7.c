/*
 * Copyright (C) 2013 Freescale Semiconductor, Inc.
 * Copyright (C) 2015 ECA Sinters
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 * Modified by: Boris Brezillon <boris.brezillon@free-electrons.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <asm/errno.h>
#include <asm/gpio.h>
#include <asm/imx-common/iomux-v3.h>
#include <asm/imx-common/boot_mode.h>
#include <malloc.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/mxc_hdmi.h>
#include <asm/arch/crm_regs.h>
#include <linux/fb.h>
#include <ipu_pixfmt.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <micrel.h>
#include <asm/imx-common/mxc_i2c.h>
#include <i2c.h>

#include "../common/mx6.h"

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	gd->ram_size = get_ram_size((void *)PHYS_SDRAM, PHYS_SDRAM_SIZE);

	return 0;
}

int board_early_init_f(void)
{
	seco_mx6_setup_uart_iomux();

	return 0;
}

int board_phy_config(struct phy_device *phydev)
{
	seco_mx6_rgmii_rework(phydev);
	if (phydev->drv->config)
		phydev->drv->config(phydev);

	return 0;
}

int board_eth_init(bd_t *bis)
{
	uint32_t base = IMX_FEC_BASE;
	struct mii_dev *bus = NULL;
	struct phy_device *phydev = NULL;
	int ret = 0;

	seco_mx6_setup_enet_iomux();

#ifdef CONFIG_FEC_MXC
	bus = fec_get_miibus(base, -1);
	if (!bus)
		return -ENOMEM;

	/* scan phy 4,5,6,7 */
	phydev = phy_find_by_mask(bus, (0xf << 4), PHY_INTERFACE_MODE_RGMII);
	if (!phydev) {
		free(bus);
		return -ENOMEM;
	}

	printf("using phy at %d\n", phydev->addr);
	ret  = fec_probe(bis, -1, base, bus, phydev);
	if (ret) {
		free(phydev);
		free(bus);
		printf("FEC MXC: %s:failed\n", __func__);
	}
#endif

	return ret;
}

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC3_BASE_ADDR},
	{USDHC2_BASE_ADDR},
};

int board_mmc_init(bd_t *bis)
{
	u32 index = 0;
	int ret;

	/*
	 * Following map is done:
	 * (U-boot device node)    (Physical Port)
	 * mmc0                    eMMC on Board
	 * mmc1                    Ext SD
	 */
	for (index = 0; index < CONFIG_SYS_FSL_USDHC_NUM; ++index) {
		switch (index) {
		case 0:
			seco_mx6_setup_usdhc_iomux(3);
			usdhc_cfg[0].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			usdhc_cfg[0].max_bus_width = 4;
			break;
		case 1:
			seco_mx6_setup_usdhc_iomux(4);
			usdhc_cfg[1].sdhc_clk = mxc_get_clock(MXC_ESDHC4_CLK);
			usdhc_cfg[1].max_bus_width = 4;
			break;

		default:
			printf("Warning: %d exceed maximum number of SD ports %d\n",
			       index + 1, CONFIG_SYS_FSL_USDHC_NUM);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[index]);
		if (ret)
			return ret;
	}

	return 0;
}

int board_init(void)
{
	/* address of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM + 0x100;

	imx_iomux_v3_setup_pad(MX6_PAD_NANDF_D4__GPIO2_IO04 |
			       MUX_PAD_CTRL(NO_PAD_CTRL));

	gpio_direction_output(IMX_GPIO_NR(2, 4), 0);

	/* Set Low */
	gpio_set_value(IMX_GPIO_NR(2, 4), 0);
	udelay(1000);

	/* Set High */
	gpio_set_value(IMX_GPIO_NR(2, 4), 1);

	return 0;
}

int checkboard(void)
{
	puts("Board: SECO uQ7\n");

	return 0;
}
