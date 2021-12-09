/*
 * Freescale i.MX23 EVK board config
 *
 * Copyright (C) 2013 Otavio Salvador <otavio@ossystems.com.br>
 * on behalf of O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __CONFIGS_MX23EVK_H__
#define __CONFIGS_MX23EVK_H__

/* System configurations */
#define CONFIG_MX23				/* i.MX23 SoC */
#define CONFIG_MACH_TYPE	MACH_TYPE_MX23EVK

/* U-Boot Commands */
#define CONFIG_SYS_NO_FLASH
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DOS_PARTITION

#define CONFIG_CMD_CACHE
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT
#define CONFIG_CMD_MMC
#define CONFIG_CMD_USB
#define CONFIG_CMD_BOOTZ
#define CONFIG_VIDEO

/* Memory configuration */
#define CONFIG_NR_DRAM_BANKS		1		/* 1 bank of DRAM */
#define PHYS_SDRAM_1			0x40000000	/* Base address */
#define PHYS_SDRAM_1_SIZE		0x08000000	/* Max 128 MB RAM */
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1

/* Environment */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_ENV_IS_IN_MMC

/* Environment is in MMC */
#if defined(CONFIG_CMD_MMC) && defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET	(256 * 1024)
#define CONFIG_ENV_SIZE		(16 * 1024)
#define CONFIG_SYS_MMC_ENV_DEV	0
#endif

/* USB */
#ifdef	CONFIG_CMD_USB
#define CONFIG_EHCI_MXS_PORT0
#define CONFIG_USB_MAX_CONTROLLER_COUNT 1
#define CONFIG_USB_STORAGE
#endif

/* Framebuffer support */
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_LOGO
#define CONFIG_SPLASH_SCREEN
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(512 << 10)
#endif

/* Boot Linux */
#define CONFIG_BOOTDELAY	1
#define CONFIG_BOOTFILE		"uImage"
#define CONFIG_LOADADDR		0x42000000
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR

/* Extra Environments */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"update_sd_firmware_filename=u-boot.sd\0" \
	"update_sd_firmware="		/* Update the SD firmware partition */ \
		"if mmc rescan ; then "	\
		"if tftp ${update_sd_firmware_filename} ; then " \
		"setexpr fw_sz ${filesize} / 0x200 ; "	/* SD block size */ \
		"setexpr fw_sz ${fw_sz} + 1 ; "	\
		"mmc write ${loadaddr} 0x800 ${fw_sz} ; " \
		"fi ; "	\
		"fi\0" \
	"script=boot.scr\0"	\
	"image=zImage\0" \
	"console=ttyAMA0\0" \
	"fdt_file=imx23-evk.dtb\0" \
	"fdt_addr=0x41000000\0" \
	"boot_fdt=try\0" \
	"mmcdev=0\0" \
	"mmcpart=2\0" \
	"mmcroot=/dev/mmcblk0p3 rw rootwait\0" \
	"mmcargs=setenv bootargs console=${console},${baudrate} " \
		"root=${mmcroot}\0" \
	"loadbootscript="  \
		"fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${script};\0" \
	"bootscript=echo Running bootscript from mmc ...; "	\
		"source\0" \
	"loadimage=fatload mmc ${mmcdev}:${mmcpart} ${loadaddr} ${image}\0" \
	"loadfdt=fatload mmc ${mmcdev}:${mmcpart} ${fdt_addr} ${fdt_file}\0" \
	"mmcboot=echo Booting from mmc ...; " \
		"run mmcargs; " \
		"if test ${boot_fdt} = yes || test ${boot_fdt} = try; then " \
			"if run loadfdt; then " \
				"bootz ${loadaddr} - ${fdt_addr}; " \
			"else " \
				"if test ${boot_fdt} = try; then " \
					"bootz; " \
				"else " \
					"echo WARN: Cannot load the DT; " \
				"fi; " \
			"fi; " \
		"else " \
			"bootz; " \
		"fi;\0"

#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev}; if mmc rescan; then " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"else " \
			"if run loadimage; then " \
				"run mmcboot; " \
			"else " \
				"echo ERR: Fail to boot from MMC; " \
			"fi; " \
		"fi; " \
	"else exit; fi"

/* The rest of the configuration is shared */
#include <configs/mxs.h>

#endif /* __CONFIGS_MX23EVK_H__ */
