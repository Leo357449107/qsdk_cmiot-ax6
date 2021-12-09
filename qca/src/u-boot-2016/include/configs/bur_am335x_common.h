/*
 * bur_am335x_common.h
 *
 * common parts used by B&R AM335x based boards
 *
 * Copyright (C) 2013 Hannes Schmelzer <oe5hpm@oevsv.at> -
 * Bernecker & Rainer Industrieelektronik GmbH - http://www.br-automation.com
 *
 * SPDX-License-Identifier:        GPL-2.0+
 */

#ifndef __BUR_AM335X_COMMON_H__
#define __BUR_AM335X_COMMON_H__
/* ------------------------------------------------------------------------- */
#define BUR_COMMON_ENV \
"usbscript=usb start && fatload usb 0 0x80000000 usbscript.img && source\0" \
"brdefaultip=if test -r ${ipaddr}; then; else" \
" setenv ipaddr 192.168.60.1; setenv serverip 192.168.60.254;" \
" setenv gatewayip 192.168.60.254; setenv netmask 255.255.255.0; fi;\0" \
"netconsole=echo switching to network console ...; " \
"if dhcp; then; else run brdefaultip; fi; setenv ncip ${serverip}; " \
"setcurs 1 9; lcdputs myip; setcurs 10 9; lcdputs ${ipaddr};" \
"setcurs 1 10;lcdputs serverip; setcurs 10 10; lcdputs ${serverip};" \
"setenv stdout nc;setenv stdin nc;setenv stderr nc\0"

#define CONFIG_PREBOOT			"run brdefaultip"
#define CONFIG_CMD_TIME


#define CONFIG_AM33XX
#define CONFIG_OMAP
#define CONFIG_OMAP_COMMON
#define CONFIG_BOARD_LATE_INIT
#define CONFIG_SYS_CACHELINE_SIZE	64
#define CONFIG_MAX_RAM_BANK_SIZE	(1024 << 20)	/* 1GB */

/* Timer information */
#define CONFIG_SYS_PTV			2	/* Divisor: 2^(PTV+1) => 8 */
#define CONFIG_SYS_TIMERBASE		0x48040000	/* Use Timer2 */
#define CONFIG_SPL_AM33XX_ENABLE_RTC32K_OSC	/* enable 32kHz OSC at bootime */
#define CONFIG_SPL_POWER_SUPPORT
#define CONFIG_POWER_TPS65217

#define CONFIG_SYS_NO_FLASH		/* have no NOR-flash */

#include <asm/arch/omap.h>

/* NS16550 Configuration */
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_REG_SIZE	(-4)
#define CONFIG_SYS_NS16550_CLK		48000000
#define CONFIG_SYS_NS16550_COM1		0x44e09000	/* UART0 */
#define CONFIG_BAUDRATE			115200

/* Network defines */
#define CONFIG_CMD_DHCP
#define CONFIG_BOOTP_DNS		/* Configurable parts of CMD_DHCP */
#define CONFIG_BOOTP_SEND_HOSTNAME
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_NET_RETRY_COUNT		2
#define CONFIG_CMD_PING
#define CONFIG_DRIVER_TI_CPSW		/* Driver for IP block */
#define CONFIG_MII			/* Required in net/eth.c */
#define CONFIG_SPL_ETH_SUPPORT
#define CONFIG_PHYLIB
#define CONFIG_PHY_NATSEMI
#define CONFIG_SPL_NET_SUPPORT
#define CONFIG_SPL_ENV_SUPPORT		/* used for a fetching MAC-Address */
#define CONFIG_SPL_NET_VCI_STRING	"AM335x U-Boot SPL"
/* Network console */
#define CONFIG_NETCONSOLE			1
#define CONFIG_BOOTP_MAY_FAIL		/* if we don't have DHCP environment */
/*
 * SPL related defines.  The Public RAM memory map the ROM defines the
 * area between 0x402F0400 and 0x4030B800 as a download area and
 * 0x4030B800 to 0x4030CE00 as a public stack area.  The ROM also
 * supports X-MODEM loading via UART, and we leverage this and then use
 * Y-MODEM to load u-boot.img, when booted over UART.
 */
#define CONFIG_SPL_TEXT_BASE		0x402F0400
#define CONFIG_SPL_MAX_SIZE		(0x4030B800 - CONFIG_SPL_TEXT_BASE)

/*
 * Since SPL did pll and ddr initialization for us,
 * we don't need to do it twice.
 */
#if !defined(CONFIG_SPL_BUILD) && !defined(CONFIG_NOR_BOOT)
#define CONFIG_SKIP_LOWLEVEL_INIT
#endif /* !CONFIG_SPL_BUILD, ... */
/*
 * Our DDR memory always starts at 0x80000000 and U-Boot shall have
 * relocated itself to higher in memory by the time this value is used.
 */
#define CONFIG_SYS_LOAD_ADDR		0x80000000
/*
 * ----------------------------------------------------------------------------
 * DDR information.  We say (for simplicity) that we have 1 bank,
 * always, even when we have more.  We always start at 0x80000000,
 * and we place the initial stack pointer in our SRAM.
 */
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_SDRAM_BASE		0x80000000
#define CONFIG_SYS_INIT_SP_ADDR		(NON_SECURE_SRAM_END - \
					GENERATED_GBL_DATA_SIZE)

/* I2C */
#define CONFIG_SYS_I2C
#define CONFIG_SYS_OMAP24_I2C_SPEED	100000
#define CONFIG_SYS_OMAP24_I2C_SLAVE	1
#define CONFIG_SYS_I2C_OMAP24XX
#define CONFIG_CMD_I2C
/* GPIO */
#define CONFIG_OMAP_GPIO
/*
 * ----------------------------------------------------------------------------
 * The following are general good-enough settings for U-Boot.  We set a
 * large malloc pool as we generally have a lot of DDR, and we opt for
 * function over binary size in the main portion of U-Boot as this is
 * generally easily constrained later if needed.  We enable the config
 * options that give us information in the environment about what board
 * we are on so we do not need to rely on the command prompt.  We set a
 * console baudrate of 115200 and use the default baud rate table.
 */
#define CONFIG_SYS_MALLOC_LEN		(5120 << 10)
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_CONSOLE_INFO_QUIET
#define CONFIG_ENV_OVERWRITE		/* Overwrite ethaddr / serial# */
#define CONFIG_SYS_CONSOLE_IS_IN_ENV
#define CONFIG_SYS_CONSOLE_OVERWRITE_ROUTINE

/* As stated above, the following choices are optional. */
#define CONFIG_SYS_LONGHELP
#define CONFIG_AUTO_COMPLETE
#define CONFIG_CMDLINE_EDITING
#define CONFIG_VERSION_VARIABLE

/* We set the max number of command args high to avoid HUSH bugs. */
#define CONFIG_SYS_MAXARGS		64

/* Console I/O Buffer Size */
#define CONFIG_SYS_CBSIZE		512
/* Print Buffer Size */
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE +\
					sizeof(CONFIG_SYS_PROMPT) + 16)
/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

/*
 * Our platforms make use of SPL to initalize the hardware (primarily
 * memory) enough for full U-Boot to be loaded.  We also support Falcon
 * Mode so that the Linux kernel can be booted directly from SPL
 * instead, if desired.  We make use of the general SPL framework found
 * under common/spl/.  Given our generally common memory map, we set a
 * number of related defaults and sizes here.
 */
#define CONFIG_SPL_FRAMEWORK
/*
 * Place the image at the start of the ROM defined image space.
 * We limit our size to the ROM-defined downloaded image area, and use the
 * rest of the space for stack.  We load U-Boot itself into memory at
 * 0x80800000 for legacy reasons (to not conflict with older SPLs).  We
 * have our BSS be placed 1MiB after this, to allow for the default
 * Linux kernel address of 0x80008000 to work, in the Falcon Mode case.
 * We have the SPL malloc pool at the end of the BSS area.
 *
 * ----------------------------------------------------------------------------
 */
#undef  CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_TEXT_BASE		0x80800000
#define CONFIG_SPL_BSS_START_ADDR	0x80A00000
#define CONFIG_SPL_BSS_MAX_SIZE		0x80000		/* 512 KB */
#define CONFIG_SYS_SPL_MALLOC_START	(CONFIG_SPL_BSS_START_ADDR + \
					CONFIG_SPL_BSS_MAX_SIZE)
#define CONFIG_SYS_SPL_MALLOC_SIZE	CONFIG_SYS_MALLOC_LEN

/* General parts of the framework, required. */
#define CONFIG_SPL_I2C_SUPPORT
#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_YMODEM_SUPPORT
#define CONFIG_SPL_LDSCRIPT		"$(CPUDIR)/am33xx/u-boot-spl.lds"

#endif	/* ! __BUR_AM335X_COMMON_H__ */
