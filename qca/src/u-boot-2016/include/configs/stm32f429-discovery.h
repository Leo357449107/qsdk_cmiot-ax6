/*
 * (C) Copyright 2015
 * Kamil Lulko, <kamil.lulko@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_STM32F4
#define CONFIG_SYS_THUMB_BUILD
#define CONFIG_STM32F4DISCOVERY

#define CONFIG_OF_LIBFDT

#define CONFIG_BOARD_EARLY_INIT_F
#define CONFIG_MISC_INIT_R

#define CONFIG_SYS_FLASH_BASE		0x08000000

#define CONFIG_SYS_INIT_SP_ADDR		0x10010000
#define CONFIG_SYS_TEXT_BASE		0x08000000

#define CONFIG_SYS_ICACHE_OFF
#define CONFIG_SYS_DCACHE_OFF

/*
 * Configuration of the external SDRAM memory
 */
#define CONFIG_NR_DRAM_BANKS		1
#define CONFIG_SYS_RAM_SIZE		(8 << 20)
#define CONFIG_SYS_RAM_CS		1
#define CONFIG_SYS_RAM_FREQ_DIV		2
#define CONFIG_SYS_RAM_BASE		0xD0000000
#define CONFIG_SYS_SDRAM_BASE		CONFIG_SYS_RAM_BASE
#define CONFIG_SYS_LOAD_ADDR		0xD0400000
#define CONFIG_LOADADDR			0xD0400000

#define CONFIG_SYS_MAX_FLASH_SECT	12
#define CONFIG_SYS_MAX_FLASH_BANKS	2

#define CONFIG_ENV_IS_IN_FLASH
#define CONFIG_ENV_OFFSET		(256 << 10)
#define CONFIG_ENV_SECT_SIZE		(128 << 10)
#define CONFIG_ENV_SIZE			(8 << 10)

#define CONFIG_BOARD_SPECIFIC_LED
#define CONFIG_RED_LED			110
#define CONFIG_GREEN_LED		109

#define CONFIG_STM32_GPIO
#define CONFIG_STM32_SERIAL

#define CONFIG_STM32_HSE_HZ		8000000

#define CONFIG_SYS_CLK_FREQ		180000000 /* 180 MHz */

#define CONFIG_SYS_HZ_CLOCK		1000000	/* Timer is clocked at 1MHz */

#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_INITRD_TAG
#define CONFIG_REVISION_TAG

#define CONFIG_SYS_CBSIZE		1024
#define CONFIG_SYS_PBSIZE		(CONFIG_SYS_CBSIZE \
					+ sizeof(CONFIG_SYS_PROMPT) + 16)

#define CONFIG_SYS_MAXARGS		16

#define CONFIG_SYS_MALLOC_LEN		(2 << 20)

#define CONFIG_STACKSIZE		(64 << 10)

#define CONFIG_BAUDRATE			115200
#define CONFIG_BOOTARGS							\
	"console=ttyS0,115200 earlyprintk consoleblank=0 ignore_loglevel"
#define CONFIG_BOOTCOMMAND						\
	"run bootcmd_romfs"

#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootargs_romfs=uclinux.physaddr=0x08180000 root=/dev/mtdblock0\0" \
	"bootcmd_romfs=setenv bootargs ${bootargs} ${bootargs_romfs};" \
	"bootm 0x08044000 - 0x08042000\0"

#define CONFIG_BOOTDELAY		3
#define CONFIG_AUTOBOOT

/*
 * Command line configuration.
 */
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_AUTO_COMPLETE
#define CONFIG_CMDLINE_EDITING

#define CONFIG_CMD_MEM
#define CONFIG_CMD_TIMER

#endif /* __CONFIG_H */
