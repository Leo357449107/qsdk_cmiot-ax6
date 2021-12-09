/*
 * Common configuration settings for IGEP technology based boards
 *
 * (C) Copyright 2012
 * ISEE 2007 SL, <www.iseebcn.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __IGEP00X0_H
#define __IGEP00X0_H

#ifdef CONFIG_BOOT_NAND
#define CONFIG_NAND
#endif

#define CONFIG_NR_DRAM_BANKS            2

#include <configs/ti_omap3_common.h>
#include <asm/mach-types.h>

#undef CONFIG_BOOTDELAY

/*
 * Display CPU and Board information
 */
#define CONFIG_DISPLAY_CPUINFO		1
#define CONFIG_DISPLAY_BOARDINFO	1

#define CONFIG_MISC_INIT_R

#define CONFIG_REVISION_TAG		1

/* Status LED available for IGEP0020 and IGEP0030 but not IGEP0032 */
#if (CONFIG_MACH_TYPE != MACH_TYPE_IGEP0032)
#define CONFIG_STATUS_LED
#define CONFIG_BOARD_SPECIFIC_LED
#define CONFIG_GPIO_LED
#if (CONFIG_MACH_TYPE == MACH_TYPE_IGEP0020)
#define RED_LED_GPIO 27
#elif (CONFIG_MACH_TYPE == MACH_TYPE_IGEP0030)
#define RED_LED_GPIO 16
#else
#error "status LED not defined for this machine."
#endif
#define RED_LED_DEV				0
#define STATUS_LED_BIT			RED_LED_GPIO
#define STATUS_LED_STATE		STATUS_LED_ON
#define STATUS_LED_PERIOD		(CONFIG_SYS_HZ / 2)
#define STATUS_LED_BOOT			RED_LED_DEV
#endif

/* GPIO banks */
#define CONFIG_OMAP3_GPIO_3		/* GPIO64 .. 95 is in GPIO bank 3 */
#define CONFIG_OMAP3_GPIO_5		/* GPIO128..159 is in GPIO bank 5 */
#define CONFIG_OMAP3_GPIO_6		/* GPIO160..191 is in GPIO bank 6 */

/* USB */
#define CONFIG_USB_MUSB_UDC			1
#define CONFIG_USB_OMAP3		1
#define CONFIG_TWL4030_USB		1

/* USB device configuration */
#define CONFIG_USB_DEVICE		1
#define CONFIG_USB_TTY			1
#define CONFIG_SYS_CONSOLE_IS_IN_ENV	1

/* Change these to suit your needs */
#define CONFIG_USBD_VENDORID		0x0451
#define CONFIG_USBD_PRODUCTID		0x5678
#define CONFIG_USBD_MANUFACTURER	"Texas Instruments"
#define CONFIG_USBD_PRODUCT_NAME	"IGEP"

#define CONFIG_CMD_CACHE
#ifdef CONFIG_BOOT_ONENAND
#define CONFIG_CMD_ONENAND	/* ONENAND support		*/
#endif
#if (CONFIG_MACH_TYPE == MACH_TYPE_IGEP0020) || \
    (CONFIG_MACH_TYPE == MACH_TYPE_IGEP0032)
#endif
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_PING

/*#undef CONFIG_ENV_IS_NOWHERE*/

#ifndef CONFIG_SPL_BUILD

#include <config_distro_defaults.h>

/* Environment */
#define ENV_DEVICE_SETTINGS \
	"stdin=serial\0" \
	"stdout=serial\0" \
	"stderr=serial\0"

#define MEM_LAYOUT_SETTINGS \
	DEFAULT_LINUX_BOOT_ENV \
	"scriptaddr=0x87E00000\0" \
	"pxefile_addr_r=0x87F00000\0"

#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0)

#include <config_distro_bootcmd.h>


#define CONFIG_EXTRA_ENV_SETTINGS \
	ENV_DEVICE_SETTINGS \
	MEM_LAYOUT_SETTINGS \
	BOOTENV

#endif

/*
 * FLASH and environment organization
 */

#ifdef CONFIG_BOOT_ONENAND
#define CONFIG_SYS_ONENAND_BASE		ONENAND_MAP

#define ONENAND_ENV_OFFSET		0x260000 /* environment starts here */

#define CONFIG_ENV_IS_IN_ONENAND	1
#define CONFIG_ENV_SIZE			(512 << 10) /* Total Size Environment */
#define CONFIG_ENV_ADDR			ONENAND_ENV_OFFSET
#endif

#ifdef CONFIG_NAND
#define CONFIG_ENV_OFFSET		0x260000 /* environment starts here */
#define CONFIG_ENV_IS_IN_NAND	        1
#define CONFIG_ENV_SIZE			(512 << 10) /* Total Size Environment */
#define CONFIG_ENV_ADDR			NAND_ENV_OFFSET
#endif

/*
 * SMSC911x Ethernet
 */
#if defined(CONFIG_CMD_NET)
#define CONFIG_SMC911X
#define CONFIG_SMC911X_32_BIT
#define CONFIG_SMC911X_BASE	0x2C000000
#endif /* (CONFIG_CMD_NET) */

/* OneNAND boot config */
#ifdef CONFIG_BOOT_ONENAND
#define CONFIG_SPL_ONENAND_SUPPORT
#define CONFIG_SYS_ONENAND_U_BOOT_OFFS  0x80000
#define CONFIG_SYS_ONENAND_PAGE_SIZE	2048
#define CONFIG_SPL_ONENAND_LOAD_ADDR    0x80000
#define CONFIG_SPL_ONENAND_LOAD_SIZE    \
	(512 * 1024 - CONFIG_SPL_ONENAND_LOAD_ADDR)

#endif

/* NAND boot config */
#ifdef CONFIG_NAND
#define CONFIG_SYS_NAND_BUSWIDTH_16BIT
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_PAGE_COUNT	64
#define CONFIG_SYS_NAND_PAGE_SIZE	2048
#define CONFIG_SYS_NAND_OOBSIZE		64
#define CONFIG_SYS_NAND_BLOCK_SIZE	(128*1024)
#define CONFIG_SYS_NAND_BAD_BLOCK_POS	NAND_LARGE_BADBLOCK_POS
#define CONFIG_SYS_NAND_ECCPOS		{ 2,  3,  4,  5,  6,  7,  8,  9, \
					 10, 11, 12, 13, 14, 15, 16, 17, \
					 18, 19, 20, 21, 22, 23, 24, 25, \
					 26, 27, 28, 29, 30, 31, 32, 33, \
					 34, 35, 36, 37, 38, 39, 40, 41, \
					 42, 43, 44, 45, 46, 47, 48, 49, \
					 50, 51, 52, 53, 54, 55, 56, 57, }
#define CONFIG_SYS_NAND_ECCSIZE		512
#define CONFIG_SYS_NAND_ECCBYTES	14
#define CONFIG_NAND_OMAP_ECCSCHEME	OMAP_ECC_BCH8_CODE_HW_DETECTION_SW
#define CONFIG_NAND_OMAP_GPMC
#define CONFIG_BCH

#define CONFIG_SYS_NAND_U_BOOT_OFFS	0x80000
/* NAND: SPL falcon mode configs */
#ifdef CONFIG_SPL_OS_BOOT
#define CONFIG_CMD_SPL_NAND_OFS		0x240000
#define CONFIG_SYS_NAND_SPL_KERNEL_OFFS	0x280000
#define CONFIG_CMD_SPL_WRITE_SIZE	0x2000
#endif
#endif

#endif /* __IGEP00X0_H */
