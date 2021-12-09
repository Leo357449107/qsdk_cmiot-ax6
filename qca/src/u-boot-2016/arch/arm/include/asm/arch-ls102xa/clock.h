/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 */

#ifndef __ASM_ARCH_LS102XA_CLOCK_H_
#define __ASM_ARCH_LS102XA_CLOCK_H_

#include <common.h>

enum mxc_clock {
	MXC_ARM_CLK = 0,
	MXC_UART_CLK,
	MXC_ESDHC_CLK,
	MXC_I2C_CLK,
	MXC_DSPI_CLK,
};

unsigned int mxc_get_clock(enum mxc_clock clk);

#endif /* __ASM_ARCH_LS102XA_CLOCK_H_ */
