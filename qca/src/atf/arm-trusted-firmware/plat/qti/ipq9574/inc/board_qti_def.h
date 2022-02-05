/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __BOARD_QTI_DEF_H__
#define __BOARD_QTI_DEF_H__

/*
 * Required platform porting definitions common to all ARM
 * development platforms
 */

/* Size of cacheable stacks */
#define PLATFORM_STACK_SIZE	0x800

/*
 * PLAT_QTI_MMAP_ENTRIES depends on the number of entries in the
 * plat_qti_mmap array defined for each BL stage.
 */
#define PLAT_QTI_MMAP_ENTRIES	12

/*
 * Platform specific page table and MMU setup constants
 */
#define MAX_XLAT_TABLES	12

#endif /* __BOARD_QTI_DEF_H__ */
