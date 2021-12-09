/*
 *  (C) Copyright 2010-2012
 *  NVIDIA Corporation <www.nvidia.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _TEGRA_COMMON_H_
#define _TEGRA_COMMON_H_
#include <linux/sizes.h>
#include <linux/stringify.h>

/*
 * High Level Configuration Options
 */
#define CONFIG_ARMCORTEXA9		/* This is an ARM V7 CPU core */
#define CONFIG_SYS_L2CACHE_OFF		/* No L2 cache */

#include <asm/arch/tegra.h>		/* get chip and board defs */

/* Use the Tegra US timer on ARMv7, but the architected timer on ARMv8. */
#ifndef CONFIG_ARM64
#define CONFIG_SYS_TIMER_RATE		1000000
#define CONFIG_SYS_TIMER_COUNTER	NV_PA_TMRUS_BASE
#endif

/*
 * Display CPU and Board information
 */
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_CMDLINE_TAG		/* enable passing of ATAGs */

/* Environment */
#define CONFIG_ENV_VARS_UBOOT_CONFIG
#define CONFIG_ENV_SIZE			0x2000	/* Total Size Environment */

/*
 * NS16550 Configuration
 */
#define CONFIG_SYS_NS16550_CLK		V_NS16550_CLK

/*
 * Common HW configuration.
 * If this varies between SoCs later, move to tegraNN-common.h
 * Note: This is number of devices, not max device ID.
 */
#define CONFIG_SYS_MMC_MAX_DEVICE 4

/*
 * select serial console configuration
 */
#define CONFIG_CONS_INDEX	1

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_BAUDRATE			115200

/* turn on command-line edit/hist/auto */
#define CONFIG_COMMAND_HISTORY

/* turn on commonly used storage-related commands */
#define CONFIG_PARTITION_UUIDS
#define CONFIG_CMD_PART

#define CONFIG_SYS_NO_FLASH

#define CONFIG_CONSOLE_MUX
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#ifndef CONFIG_SPL_BUILD
#define CONFIG_SYS_STDIO_DEREGISTER
#endif

/*
 * Increasing the size of the IO buffer as default nfsargs size is more
 *  than 256 and so it is not possible to edit it
 */
#define CONFIG_SYS_CBSIZE		(256 * 2) /* Console I/O Buffer Size */
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE + \
					sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_MAXARGS		32	/* max number of command args */
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		(CONFIG_SYS_CBSIZE)

#define CONFIG_SYS_MEMTEST_START	(NV_PA_SDRC_CS0 + 0x600000)
#define CONFIG_SYS_MEMTEST_END		(CONFIG_SYS_MEMTEST_START + 0x100000)

#ifndef CONFIG_ARM64
#ifndef CONFIG_SPL_BUILD
#define CONFIG_USE_ARCH_MEMCPY
#endif
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	2
#define PHYS_SDRAM_1		NV_PA_SDRC_CS0
#define PHYS_SDRAM_1_SIZE	0x20000000	/* 512M */

#define CONFIG_SYS_UBOOT_START	CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_SDRAM_BASE	PHYS_SDRAM_1

#define CONFIG_SYS_BOOTMAPSZ	(256 << 20)	/* 256M */

#define CONFIG_SYS_INIT_RAM_ADDR	CONFIG_STACKBASE
#define CONFIG_SYS_INIT_RAM_SIZE	CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SYS_INIT_RAM_ADDR + \
						CONFIG_SYS_INIT_RAM_SIZE - \
						GENERATED_GBL_DATA_SIZE)

#define CONFIG_TEGRA_GPIO
#define CONFIG_CMD_ENTERRCM

/* Defines for SPL */
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_RAM_DEVICE
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_NAND_SIMPLE
#define CONFIG_SPL_MAX_FOOTPRINT	(CONFIG_SYS_TEXT_BASE - \
						CONFIG_SPL_TEXT_BASE)
#define CONFIG_SYS_SPL_MALLOC_SIZE	0x00010000

#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_BOARD_LATE_INIT

/* Misc utility code */
#define CONFIG_BOUNCE_BUFFER
#define CONFIG_CRC32_VERIFY

#ifndef CONFIG_SPL_BUILD
#include <config_distro_defaults.h>
#define CONFIG_CMD_EXT4_WRITE
#define CONFIG_FAT_WRITE
#endif

#define CONFIG_OF_SYSTEM_SETUP

#endif /* _TEGRA_COMMON_H_ */
