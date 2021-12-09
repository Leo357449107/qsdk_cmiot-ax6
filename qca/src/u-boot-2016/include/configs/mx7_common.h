/*
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 *
 * Configuration settings for the Freescale i.MX7.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MX7_COMMON_H
#define __MX7_COMMON_H

#include <linux/sizes.h>
#include <asm/arch/imx-regs.h>
#include <asm/imx-common/gpio.h>

#ifndef CONFIG_MX7
#define CONFIG_MX7
#endif

/* Timer settings */
#define CONFIG_MXC_GPT_HCLK
#define CONFIG_SYSCOUNTER_TIMER
#define CONFIG_SC_TIMER_CLK 8000000 /* 8Mhz */
#define CONFIG_SYS_FSL_CLK

/* Enable iomux-lpsr support */
#define CONFIG_IOMUX_LPSR
#define CONFIG_IMX_FIXED_IVT_OFFSET

/* Size of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN           (32 * SZ_1M)

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT

#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_FSL_CLK

#define CONFIG_LOADADDR                 0x80800000
#define CONFIG_SYS_TEXT_BASE            0x87800000

#ifndef CONFIG_BOOTDELAY
#define CONFIG_BOOTDELAY                3
#endif

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX               1
#define CONFIG_BAUDRATE                 115200

/* Filesystems and image support */
#define CONFIG_OF_LIBFDT
#define CONFIG_CMD_BOOTZ
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_CMD_FAT

/* Miscellaneous configurable options */
#undef CONFIG_CMD_IMLS
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_CMDLINE_EDITING
#define CONFIG_AUTO_COMPLETE
#define CONFIG_SYS_CBSIZE		512
#define CONFIG_SYS_MAXARGS		32
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#ifndef CONFIG_SYS_DCACHE_OFF
#define CONFIG_CMD_CACHE
#endif

/* GPIO */
#define CONFIG_MXC_GPIO

/* UART */
#define CONFIG_MXC_UART
#define CONFIG_MXC_UART_BASE            UART1_IPS_BASE_ADDR

/* MMC */
#define CONFIG_MMC
#define CONFIG_CMD_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_FSL_ESDHC
#define CONFIG_FSL_USDHC

/* Fuses */
#define CONFIG_CMD_FUSE
#define CONFIG_MXC_OCOTP

/*
 * Default boot linux kernel in no secure mode.
 * If want to boot kernel in secure mode, please define CONFIG_MX7_SEC
 */
#ifndef CONFIG_MX7_SEC
#define CONFIG_ARMV7_NONSEC
#define CONFIG_ARMV7_PSCI
#define CONFIG_ARMV7_PSCI_NR_CPUS	2
#define CONFIG_ARMV7_SECURE_BASE	0x00900000
#endif

#endif
