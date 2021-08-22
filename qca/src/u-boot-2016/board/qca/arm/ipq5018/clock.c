/*
 * Copyright (c) 2015-2016, 2018-2019 The Linux Foundation. All rights reserved.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <asm/arch-ipq5018/clk.h>
#include <asm/io.h>
#include <asm/errno.h>

#ifdef CONFIG_IPQ5018_I2C
void i2c_clock_config(void)
{
	int cfg;

	/* Configure qup1_i2c_apps_clk_src */
	cfg = (GCC_BLSP1_QUP3_I2C_APPS_CFG_RCGR_SRC_SEL |
			GCC_BLSP1_QUP3_I2C_APPS_CFG_RCGR_SRC_DIV);
	writel(cfg, GCC_BLSP1_QUP3_I2C_APPS_CFG_RCGR);

	writel(CMD_UPDATE, GCC_BLSP1_QUP3_I2C_APPS_CMD_RCGR);
	mdelay(100);
	writel(ROOT_EN, GCC_BLSP1_QUP3_I2C_APPS_CMD_RCGR);

	/* Configure CBCR */
	writel(CLK_ENABLE, GCC_BLSP1_QUP3_I2C_APPS_CBCR);
}
#endif

#ifdef CONFIG_IPQ_BT_SUPPORT
void enable_btss_lpo_clk(void)
{
	writel(CLK_ENABLE, GCC_BTSS_LPO_CBCR);
}
#endif
