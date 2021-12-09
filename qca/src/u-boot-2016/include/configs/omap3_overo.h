/*
 * Configuration settings for the Gumstix Overo board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_NR_DRAM_BANKS	2	/* CS1 may or may not be populated */
#define CONFIG_NAND

#include <configs/ti_omap3_common.h>
#undef CONFIG_SPL_MAX_SIZE
#define CONFIG_SPL_MAX_SIZE	(64*1024)
#undef CONFIG_SPL_TEXT_BASE
#define CONFIG_SPL_TEXT_BASE	0x40200000

#define CONFIG_BCH

/* Display CPU and Board information */
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DISPLAY_BOARDINFO

/* call misc_init_r */
#define CONFIG_MISC_INIT_R

/* pass the revision tag */
#define CONFIG_REVISION_TAG

/* override size of malloc() pool */
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_ENV_SIZE		(128 << 10)	/* 128 KiB sector */
/* Shift 128 << 15 provides 4 MiB heap to support UBI commands.
 * Shift 128 << 10 provides 128 KiB heap for limited-memory devices. */
#define CONFIG_SYS_MALLOC_LEN	(CONFIG_ENV_SIZE + (128 << 15))

/* I2C Support */
#define CONFIG_SYS_I2C_OMAP34XX

/* TWL4030 LED */
#define CONFIG_TWL4030_LED

/* USB EHCI */
#define CONFIG_USB_EHCI
#define CONFIG_USB_EHCI_OMAP
#define CONFIG_USB_STORAGE
#define CONFIG_OMAP_EHCI_PHY1_RESET_GPIO	183
#define CONFIG_SYS_USB_EHCI_MAX_ROOT_PORTS	3

/* Initialize GPIOs by default */
#define CONFIG_OMAP3_GPIO_2	/* GPIO32..63 is in GPIO Bank 2 */
#define CONFIG_OMAP3_GPIO_3	/* GPIO64..95 is in GPIO Bank 3 */
#define CONFIG_OMAP3_GPIO_4	/* GPIO96..127 is in GPIO Bank 4 */
#define CONFIG_OMAP3_GPIO_5	/* GPIO128..159 is in GPIO Bank 5 */
#define CONFIG_OMAP3_GPIO_6	/* GPIO160..191 is in GPIO Bank 6 */

/* commands to include */
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_USB

#ifdef CONFIG_NAND
#define CONFIG_CMD_UBI		/* UBI-formated MTD partition support */
#define CONFIG_CMD_UBIFS	/* Read-only UBI volume operations */

#define CONFIG_RBTREE		/* required by CONFIG_CMD_UBI */
#define CONFIG_LZO		/* required by CONFIG_CMD_UBIFS */

#define CONFIG_MTD_PARTITIONS	/* required for UBI partition support */

/* NAND block size is 128 KiB.  Synchronize these values with
 * overo_nand_partitions in mach-omap2/board-overo.c in Linux:
 *  xloader              4 * NAND_BLOCK_SIZE = 512 KiB
 *  uboot               14 * NAND_BLOCK_SIZE = 1792 KiB
 *  uboot environtment   2 * NAND_BLOCK_SIZE = 256 KiB
 *  linux               64 * NAND_BLOCK_SIZE = 8 MiB
 *  rootfs              remainder
 */
#define MTDIDS_DEFAULT "nand0=omap2-nand.0"
#define MTDPARTS_DEFAULT "mtdparts=omap2-nand.0:"	\
	"512k(xloader),"				\
	"1792k(u-boot),"				\
	"256k(environ),"				\
	"8m(linux),"					\
	"-(rootfs)"
#else /* CONFIG_NAND */
#define MTDPARTS_DEFAULT
#endif /* CONFIG_NAND */

/* Board NAND Info. */
#define CONFIG_SYS_NAND_ADDR		NAND_BASE	/* physical address */
							/* to access nand */
/* Environment information */
#define CONFIG_EXTRA_ENV_SETTINGS \
	DEFAULT_LINUX_BOOT_ENV \
	"bootdir=/boot\0" \
	"bootfile=zImage\0" \
	"usbtty=cdc_acm\0" \
	"console=ttyO2,115200n8\0" \
	"mpurate=auto\0" \
	"optargs=\0" \
	"vram=12M\0" \
	"dvimode=1024x768MR-16@60\0" \
	"defaultdisplay=dvi\0" \
	"mmcdev=0\0" \
	"mmcroot=/dev/mmcblk0p2 rw\0" \
	"mmcrootfstype=ext4 rootwait\0" \
	"nandroot=ubi0:rootfs ubi.mtd=4\0" \
	"nandrootfstype=ubifs\0" \
	"mtdparts=" MTDPARTS_DEFAULT "\0" \
	"mmcargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"mpurate=${mpurate} " \
		"vram=${vram} " \
		"omapfb.mode=dvi:${dvimode} " \
		"omapdss.def_disp=${defaultdisplay} " \
		"root=${mmcroot} " \
		"rootfstype=${mmcrootfstype}\0" \
	"nandargs=setenv bootargs console=${console} " \
		"${optargs} " \
		"mpurate=${mpurate} " \
		"vram=${vram} " \
		"omapfb.mode=dvi:${dvimode} " \
		"omapdss.def_disp=${defaultdisplay} " \
		"root=${nandroot} " \
		"rootfstype=${nandrootfstype}\0" \
	"loadbootscript=load mmc ${mmcdev} ${loadaddr} boot.scr\0" \
	"bootscript=echo Running boot script from mmc ...; " \
		"source ${loadaddr}\0" \
	"loadbootenv=load mmc ${mmcdev} ${loadaddr} uEnv.txt\0" \
	"importbootenv=echo Importing environment from mmc ...; " \
		"env import -t ${loadaddr} ${filesize}\0" \
	"loaduimage=load mmc ${mmcdev} ${loadaddr} uImage\0" \
	"mmcboot=echo Booting from mmc...; " \
		"run mmcargs; " \
		"bootm ${loadaddr}\0" \
	"loadzimage=load mmc ${mmcdev}:2 ${loadaddr} ${bootdir}/${bootfile}\0" \
	"loadfdt=load mmc ${mmcdev}:2 ${fdtaddr} ${bootdir}/${fdtfile}\0" \
	"loadubizimage=ubifsload ${loadaddr} ${bootdir}/${bootfile}\0" \
	"loadubifdt=ubifsload ${fdtaddr} ${bootdir}/${fdtfile}\0" \
	"mmcbootfdt=echo Booting with DT from mmc ...; " \
		"run mmcargs; " \
		"bootz ${loadaddr} - ${fdtaddr}\0" \
	"nandboot=echo Booting from nand ...; " \
		"run nandargs; " \
		"if nand read ${loadaddr} linux; then " \
			"bootm ${loadaddr};" \
		"fi;\0" \
	"nanddtsboot=echo Booting from nand with DTS...; " \
		"run nandargs; " \
		"ubi part rootfs; "\
		"ubifsmount ubi0:rootfs; "\
		"run loadubifdt; "\
		"run loadubizimage; "\
		"bootz ${loadaddr} - ${fdtaddr}\0" \

#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev}; if mmc rescan; then " \
		"if run loadbootscript; then " \
			"run bootscript; " \
		"fi;" \
		"if run loadbootenv; then " \
			"echo Loaded environment from ${bootenv};" \
			"run importbootenv;" \
		"fi;" \
		"if test -n $uenvcmd; then " \
			"echo Running uenvcmd ...;" \
			"run uenvcmd;" \
		"fi;" \
		"if run loaduimage; then " \
			"run mmcboot;" \
		"fi;" \
		"if run loadzimage; then " \
			"if test -z \"${fdtfile}\"; then " \
				"setenv fdtfile omap3-${boardname}-${expansionname}.dtb;" \
			"fi;" \
			"if run loadfdt; then " \
				"run mmcbootfdt;" \
			"fi;" \
		"fi;" \
	"fi;" \
	"run nandboot; " \
	"if test -z \"${fdtfile}\"; then "\
		"setenv fdtfile omap3-${boardname}-${expansionname}.dtb;" \
	"fi;" \
	"run nanddtsboot; " \

/* memtest works on */
#define CONFIG_SYS_MEMTEST_START	(OMAP34XX_SDRC_CS0)
#define CONFIG_SYS_MEMTEST_END		(OMAP34XX_SDRC_CS0 + \
					0x01F00000) /* 31MB */

/* FLASH and environment organization */
#if defined(CONFIG_NAND)
#define CONFIG_SYS_FLASH_BASE		NAND_BASE
#endif

/* Monitor at start of flash */
#define CONFIG_SYS_MONITOR_BASE		CONFIG_SYS_FLASH_BASE
#define CONFIG_SYS_ONENAND_BASE		ONENAND_MAP

#define CONFIG_ENV_IS_IN_NAND
#define ONENAND_ENV_OFFSET		0x240000 /* environment starts here */
#define SMNAND_ENV_OFFSET		0x240000 /* environment starts here */

#define CONFIG_SYS_ENV_SECT_SIZE	(128 << 10)	/* 128 KiB */
#define CONFIG_ENV_OFFSET		SMNAND_ENV_OFFSET
#define CONFIG_ENV_ADDR			SMNAND_ENV_OFFSET

/* Configure SMSC9211 ethernet */
#if defined(CONFIG_CMD_NET)
#define CONFIG_SMC911X
#define CONFIG_SMC911X_32_BIT
#define CONFIG_SMC911X_BASE		0x2C000000
#endif /* (CONFIG_CMD_NET) */

/* Initial RAM setup */
#define CONFIG_SYS_INIT_RAM_ADDR	0x4020f800
#define CONFIG_SYS_INIT_RAM_SIZE	0x800
#define CONFIG_SYS_CACHELINE_SIZE	64

/* NAND boot config */
#define CONFIG_SYS_NAND_BUSWIDTH_16BIT
#define CONFIG_SYS_NAND_MAX_ECCPOS  56
#define CONFIG_SYS_NAND_5_ADDR_CYCLE
#define CONFIG_SYS_NAND_PAGE_COUNT	64
#define CONFIG_SYS_NAND_PAGE_SIZE	2048
#define CONFIG_SYS_NAND_OOBSIZE		64
#define CONFIG_SYS_NAND_BLOCK_SIZE	(128*1024)
#define CONFIG_SYS_NAND_BAD_BLOCK_POS	NAND_LARGE_BADBLOCK_POS
#define CONFIG_SYS_NAND_ECCPOS      {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, \
					13, 14, 16, 17, 18, 19, 20, 21, 22, \
					23, 24, 25, 26, 27, 28, 30, 31, 32, \
					33, 34, 35, 36, 37, 38, 39, 40, 41, \
					42, 44, 45, 46, 47, 48, 49, 50, 51, \
					52, 53, 54, 55, 56}
#define CONFIG_SYS_NAND_ECCSIZE		512
#define CONFIG_SYS_NAND_ECCBYTES	13
#define CONFIG_NAND_OMAP_ECCSCHEME	OMAP_ECC_BCH8_CODE_HW_DETECTION_SW
#define CONFIG_SYS_NAND_U_BOOT_START	CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_NAND_U_BOOT_OFFS	0x80000
/* NAND: SPL falcon mode configs */
#ifdef CONFIG_SPL_OS_BOOT
#define CONFIG_CMD_SPL_NAND_OFS		0x240000
#define CONFIG_SYS_NAND_SPL_KERNEL_OFFS	0x280000
#define CONFIG_CMD_SPL_WRITE_SIZE	0x2000
#endif

#endif				/* __CONFIG_H */
