/*
 * Copyright (C) 2012
 * David Purdy <david.c.purdy@gmail.com>
 *
 * Based on Kirkwood support:
 * (C) Copyright 2009
 * Marvell Semiconductor <www.marvell.com>
 * Written-by: Prafulla Wadaskar <prafulla@marvell.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _CONFIG_POGO_E02_H
#define _CONFIG_POGO_E02_H


/*
 * Machine type definition and ID
 */
#define MACH_TYPE_POGO_E02		3542
#define CONFIG_MACH_TYPE		MACH_TYPE_POGO_E02
#define CONFIG_IDENT_STRING		"\nPogo E02"

/*
 * High Level Configuration Options (easy to change)
 */
#define CONFIG_FEROCEON_88FR131		/* CPU Core subversion */
#define CONFIG_KW88F6281		/* SOC Name */
#define CONFIG_SKIP_LOWLEVEL_INIT	/* disable board lowlevel_init */

/*
 * Commands configuration
 */
#define CONFIG_SYS_NO_FLASH		/* Declare no flash (NOR/SPI) */
#define CONFIG_SYS_MVFS
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_ENV
#define CONFIG_CMD_MII
#define CONFIG_CMD_NAND
#define CONFIG_CMD_PING
#define CONFIG_CMD_USB

/*
 * mv-common.h should be defined after CMD configs since it used them
 * to enable certain macros
 */
#include "mv-common.h"

/*
 *  Environment variables configurations
 */
#ifdef CONFIG_CMD_NAND
#define CONFIG_ENV_IS_IN_NAND
#define CONFIG_ENV_SECT_SIZE		0x20000	/* 128K */
#else
#define CONFIG_ENV_IS_NOWHERE
#endif

#define CONFIG_ENV_SIZE			0x20000	/* 128k */
#define CONFIG_ENV_OFFSET		0x60000	/* env starts here */

/*
 * Default environment variables
 */
#define CONFIG_BOOTCOMMAND \
	"setenv bootargs $(bootargs_console); " \
	"run bootcmd_usb; " \
	"bootm 0x00800000 0x01100000"

#define CONFIG_EXTRA_ENV_SETTINGS \
	"mtdparts=mtdparts=orion_nand:1M(u-boot),4M(uImage)," \
	"32M(rootfs),-(data)\0"\
	"mtdids=nand0=orion_nand\0"\
	"bootargs_console=console=ttyS0,115200\0" \
	"bootcmd_usb=usb start; ext2load usb 0:1 0x00800000 /uImage; " \
	"ext2load usb 0:1 0x01100000 /uInitrd\0"

/*
 * Ethernet Driver configuration
 */
#ifdef CONFIG_CMD_NET
#define CONFIG_MVGBE_PORTS	{1, 0}	/* enable port 0 only */
#define CONFIG_PHY_BASE_ADR	0
#endif /* CONFIG_CMD_NET */

/*
 * File system
 */
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT
#define CONFIG_CMD_JFFS2
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_RBTREE
#define CONFIG_MTD_DEVICE               /* needed for mtdparts commands */
#define CONFIG_MTD_PARTITIONS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_LZO

#endif /* _CONFIG_POGO_E02_H */
