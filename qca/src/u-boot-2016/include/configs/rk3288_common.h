/*
 * (C) Copyright 2015 Google, Inc
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __CONFIG_RK3288_COMMON_H
#define __CONFIG_RK3288_COMMON_H

#include <asm/arch/hardware.h>

#define CONFIG_SYS_NO_FLASH
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_ENV_SIZE			0x2000
#define CONFIG_SYS_MAXARGS		16
#define CONFIG_BAUDRATE			115200
#define CONFIG_SYS_MALLOC_LEN		(32 << 20)
#define CONFIG_SYS_CBSIZE		1024
#define CONFIG_SKIP_LOWLEVEL_INIT
#define CONFIG_SYS_THUMB_BUILD
#define CONFIG_OF_LIBFDT
#define CONFIG_DISPLAY_BOARDINFO

#define CONFIG_SYS_TIMER_RATE		(24 * 1000 * 1000)
#define	CONFIG_SYS_TIMER_BASE		0xff810020 /* TIMER7 */
#define CONFIG_SYS_TIMER_COUNTER	(CONFIG_SYS_TIMER_BASE + 8)

#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SYS_NS16550_MEM32
#define CONFIG_SPL_BOARD_INIT

#define CONFIG_SYS_TEXT_BASE		0x00100000
#define CONFIG_SYS_INIT_SP_ADDR		0x00100000
#define CONFIG_SYS_LOAD_ADDR		0x00800800
#define CONFIG_SPL_STACK		0xff718000
#define CONFIG_SPL_TEXT_BASE		0xff704004

#define CONFIG_ROCKCHIP_COMMON
#define CONFIG_SPL_ROCKCHIP_COMMON

/* MMC/SD IP block */
#define CONFIG_MMC
#define CONFIG_GENERIC_MMC
#define CONFIG_CMD_MMC
#define CONFIG_SDHCI
#define CONFIG_DWMMC
#define CONFIG_BOUNCE_BUFFER

#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_FAT_WRITE
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_EXT4
#define CONFIG_CMD_FS_GENERIC
#define CONFIG_PARTITION_UUIDS
#define CONFIG_CMD_PART

/* RAW SD card / eMMC locations. */
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	256
#define CONFIG_SYS_SPI_U_BOOT_OFFS	(128 << 10)

/* FAT sd card locations. */
#define CONFIG_SYS_MMCSD_FS_BOOT_PARTITION	1
#define CONFIG_SPL_FS_LOAD_PAYLOAD_NAME		"u-boot.img"

#define CONFIG_SPL_PINCTRL_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_RAM_SUPPORT
#define CONFIG_SPL_DRIVERS_MISC_SUPPORT

#define CONFIG_CMD_CACHE
#define CONFIG_CMD_TIME

#define CONFIG_SYS_SDRAM_BASE		0
#define CONFIG_NR_DRAM_BANKS		1
#define SDRAM_BANK_SIZE			(2UL << 30)

#define CONFIG_SPI_FLASH
#define CONFIG_SPI
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI
#define CONFIG_SF_DEFAULT_SPEED 20000000

#define CONFIG_CMD_I2C

#ifndef CONFIG_SPL_BUILD
#include <config_distro_defaults.h>

#define ENV_MEM_LAYOUT_SETTINGS \
	"scriptaddr=0x00000000\0" \
	"pxefile_addr_r=0x00100000\0" \
	"fdt_addr_r=0x01f00000\0" \
	"kernel_addr_r=0x02000000\0" \
	"ramdisk_addr_r=0x04000000\0"

/* First try to boot from SD (index 0), then eMMC (index 1 */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1)

#include <config_distro_bootcmd.h>

/* Linux fails to load the fdt if it's loaded above 512M on a Rock 2 board, so
 * limit the fdt reallocation to that */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"fdt_high=0x1fffffff\0" \
	"initrd_high=0x1fffffff\0" \
	ENV_MEM_LAYOUT_SETTINGS \
	BOOTENV
#endif

#endif
