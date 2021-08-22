/*
 * Copyright (c) 2015-2016, 2018-2020 The Linux Foundation. All rights reserved.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef IPQ5018_CLK_H
#define IPQ5018_CLK_H

#define CLK_ENABLE	0x1
/* I2C clocks configuration */
#ifdef CONFIG_IPQ5018_I2C

#define GCC_BLSP1_QUP3_I2C_APPS_CFG_RCGR		0x1804004
#define GCC_BLSP1_QUP3_I2C_APPS_CFG_RCGR_SRC_SEL	(1 << 8)
#define GCC_BLSP1_QUP3_I2C_APPS_CFG_RCGR_SRC_DIV	(0x1F << 0)

#define GCC_BLSP1_QUP3_I2C_APPS_CMD_RCGR	0x1804000
#define GCC_BLSP1_QUP3_I2C_APPS_CBCR		0x1804010

#define CMD_UPDATE	0x1
#define ROOT_EN		0x2


void i2c_clock_config(void);
#endif

#ifdef CONFIG_IPQ_BT_SUPPORT
#define GCC_BTSS_LPO_CBCR			0x181C004
void enable_btss_lpo_clk(void);
#endif

#endif /*IPQ5018_CLK_H*/
