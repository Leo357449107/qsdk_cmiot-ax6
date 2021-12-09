/*
 * (C) Copyright 2012
 * Texas Instruments, <www.ti.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef	_SPL_H_
#define	_SPL_H_

/* Platform-specific defines */
#include <linux/compiler.h>
#include <asm/spl.h>

/* Value in r0 indicates we booted from U-Boot */
#define UBOOT_NOT_LOADED_FROM_SPL	0x13578642

/* Boot type */
#define MMCSD_MODE_UNDEFINED	0
#define MMCSD_MODE_RAW		1
#define MMCSD_MODE_FS		2
#define MMCSD_MODE_EMMCBOOT	3

struct spl_image_info {
	const char *name;
	u8 os;
	u32 load_addr;
	u32 entry_point;
	u32 size;
	u32 flags;
};

#define SPL_COPY_PAYLOAD_ONLY	1

extern struct spl_image_info spl_image;

/* SPL common functions */
void preloader_console_init(void);
u32 spl_boot_device(void);
u32 spl_boot_mode(void);
void spl_set_header_raw_uboot(void);
void spl_parse_image_header(const struct image_header *header);
void spl_board_prepare_for_linux(void);
void __noreturn jump_to_image_linux(void *arg);
int spl_start_uboot(void);
void spl_display_print(void);

/* NAND SPL functions */
int spl_nand_load_image(void);

/* OneNAND SPL functions */
int spl_onenand_load_image(void);

/* NOR SPL functions */
int spl_nor_load_image(void);

/* MMC SPL functions */
int spl_mmc_load_image(u32 boot_device);

/* YMODEM SPL functions */
int spl_ymodem_load_image(void);

/* SPI SPL functions */
int spl_spi_load_image(void);

/* Ethernet SPL functions */
int spl_net_load_image(const char *device);

/* USB SPL functions */
int spl_usb_load_image(void);

/* SATA SPL functions */
int spl_sata_load_image(void);

/* SPL FAT image functions */
int spl_load_image_fat(block_dev_desc_t *block_dev, int partition, const char *filename);
int spl_load_image_fat_os(block_dev_desc_t *block_dev, int partition);

void __noreturn jump_to_image_no_args(struct spl_image_info *spl_image);

/* SPL EXT image functions */
int spl_load_image_ext(block_dev_desc_t *block_dev, int partition, const char *filename);
int spl_load_image_ext_os(block_dev_desc_t *block_dev, int partition);

/**
 * spl_init() - Set up device tree and driver model in SPL if enabled
 *
 * Call this function in board_init_f() if you want to use device tree and
 * driver model early, before board_init_r() is called. This function will
 * be called from board_init_r() if not called earlier.
 *
 * If this is not called, then driver model will be inactive in SPL's
 * board_init_f(), and no device tree will be available.
 */
int spl_init(void);

#ifdef CONFIG_SPL_BOARD_INIT
void spl_board_init(void);
#endif

/**
 * spl_was_boot_source() - check if U-Boot booted from SPL
 *
 * This will normally be true, but if U-Boot jumps to second U-Boot, it will
 * be false. This should be implemented by board-specific code.
 *
 * @return true if U-Boot booted from SPL, else false
 */
bool spl_was_boot_source(void);

#endif
