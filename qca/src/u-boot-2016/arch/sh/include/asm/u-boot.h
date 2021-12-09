/*
 * SPDX-License-Identifier:	GPL-2.0+
 *
 ********************************************************************
 * NOTE: This header file defines an interface to U-Boot. Including
 * this (unmodified) header file in another file is considered normal
 * use of U-Boot, and does *not* fall under the heading of "derived
 * work".
 ********************************************************************
 */

#ifndef __ASM_SH_U_BOOT_H_
#define __ASM_SH_U_BOOT_H_

typedef struct bd_info {
	unsigned long   bi_memstart;    /* start of DRAM memory */
	phys_size_t	bi_memsize;     /* size  of DRAM memory in bytes */
	unsigned long   bi_flashstart;  /* start of FLASH memory */
	unsigned long   bi_flashsize;   /* size  of FLASH memory */
	unsigned long   bi_flashoffset; /* reserved area for startup monitor */
	unsigned long   bi_sramstart;   /* start of SRAM memory */
	unsigned long   bi_sramsize;    /* size  of SRAM memory */
	unsigned long	bi_boot_params; /* where this board expects params */
} bd_t;

/* For image.h:image_check_target_arch() */
#define IH_ARCH_DEFAULT IH_ARCH_SH

#endif
