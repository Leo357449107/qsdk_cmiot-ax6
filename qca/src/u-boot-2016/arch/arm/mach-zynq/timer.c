/*
 * Copyright (C) 2012 Michal Simek <monstr@monstr.eu>
 * Copyright (C) 2011-2012 Xilinx, Inc. All rights reserved.
 *
 * (C) Copyright 2008
 * Guennadi Liakhovetki, DENX Software Engineering, <lg@denx.de>
 *
 * (C) Copyright 2004
 * Philippe Robin, ARM Ltd. <philippe.robin@arm.com>
 *
 * (C) Copyright 2002-2004
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * (C) Copyright 2003
 * Texas Instruments <www.ti.com>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <div64.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/clk.h>

DECLARE_GLOBAL_DATA_PTR;

struct scu_timer {
	u32 load; /* Timer Load Register */
	u32 counter; /* Timer Counter Register */
	u32 control; /* Timer Control Register */
};

static struct scu_timer *timer_base =
			      (struct scu_timer *)ZYNQ_SCUTIMER_BASEADDR;

#define SCUTIMER_CONTROL_PRESCALER_MASK	0x0000FF00 /* Prescaler */
#define SCUTIMER_CONTROL_PRESCALER_SHIFT	8
#define SCUTIMER_CONTROL_AUTO_RELOAD_MASK	0x00000002 /* Auto-reload */
#define SCUTIMER_CONTROL_ENABLE_MASK		0x00000001 /* Timer enable */

#define TIMER_LOAD_VAL 0xFFFFFFFF
#define TIMER_PRESCALE 255

int timer_init(void)
{
	const u32 emask = SCUTIMER_CONTROL_AUTO_RELOAD_MASK |
			(TIMER_PRESCALE << SCUTIMER_CONTROL_PRESCALER_SHIFT) |
			SCUTIMER_CONTROL_ENABLE_MASK;

	gd->arch.timer_rate_hz = (gd->cpu_clk / 2) / (TIMER_PRESCALE + 1);

	/* Load the timer counter register */
	writel(0xFFFFFFFF, &timer_base->load);

	/*
	 * Start the A9Timer device
	 * Enable Auto reload mode, Clear prescaler control bits
	 * Set prescaler value, Enable the decrementer
	 */
	clrsetbits_le32(&timer_base->control, SCUTIMER_CONTROL_PRESCALER_MASK,
								emask);

	/* Reset time */
	gd->arch.lastinc = readl(&timer_base->counter) /
				(gd->arch.timer_rate_hz / CONFIG_SYS_HZ);
	gd->arch.tbl = 0;

	return 0;
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	return gd->arch.timer_rate_hz;
}
