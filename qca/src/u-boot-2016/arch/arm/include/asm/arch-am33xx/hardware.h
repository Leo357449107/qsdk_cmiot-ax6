/*
 * hardware.h
 *
 * hardware specific header
 *
 * Copyright (C) 2013, Texas Instruments, Incorporated - http://www.ti.com/
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __AM33XX_HARDWARE_H
#define __AM33XX_HARDWARE_H

#include <config.h>
#include <asm/arch/omap.h>
#ifdef CONFIG_AM33XX
#include <asm/arch/hardware_am33xx.h>
#elif defined(CONFIG_TI816X)
#include <asm/arch/hardware_ti816x.h>
#elif defined(CONFIG_TI814X)
#include <asm/arch/hardware_ti814x.h>
#elif defined(CONFIG_AM43XX)
#include <asm/arch/hardware_am43xx.h>
#endif

/*
 * Common hardware definitions
 */

/* DM Timer base addresses */
#define DM_TIMER0_BASE			0x4802C000
#define DM_TIMER1_BASE			0x4802E000
#define DM_TIMER2_BASE			0x48040000
#define DM_TIMER3_BASE			0x48042000
#define DM_TIMER4_BASE			0x48044000
#define DM_TIMER5_BASE			0x48046000
#define DM_TIMER6_BASE			0x48048000
#define DM_TIMER7_BASE			0x4804A000

/* GPIO Base address */
#define GPIO0_BASE			0x48032000
#define GPIO1_BASE			0x4804C000

/* BCH Error Location Module */
#define ELM_BASE			0x48080000

/* EMIF Base address */
#define EMIF4_0_CFG_BASE		0x4C000000
#define EMIF4_1_CFG_BASE		0x4D000000

/* DDR Base address */
#define DDR_CTRL_ADDR			0x44E10E04
#define DDR_CONTROL_BASE_ADDR		0x44E11404

/* UART */
#define DEFAULT_UART_BASE		UART0_BASE

/* GPMC Base address */
#define GPMC_BASE			0x50000000

/* CPSW Config space */
#define CPSW_BASE			0x4A100000

int clk_get(int clk);
#endif /* __AM33XX_HARDWARE_H */
