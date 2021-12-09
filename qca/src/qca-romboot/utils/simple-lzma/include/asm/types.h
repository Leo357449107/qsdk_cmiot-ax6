/* $Id: //depot/sw/qca_main/components/bootloaders/rom-boot-drv/1.0/utils/simple-lzma/include/asm/types.h#2 $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 1995, 1996, 1999 by Ralf Baechle
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 */
#ifndef _ASM_TYPES_H
#define _ASM_TYPES_H

typedef unsigned short umode_t;

/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;
typedef unsigned short __le16;

typedef __signed__ int __s32;
typedef unsigned int __u32;
typedef unsigned int uint_32  ;

#if (_MIPS_SZLONG == 64)

typedef __signed__ long __s64;
typedef unsigned long __u64;

#else

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif

#endif

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
#ifdef __KERNEL__

typedef __signed char s8;
typedef unsigned char u8;

typedef __signed short s16;
typedef unsigned short u16;

typedef __signed int s32;
typedef unsigned int u32;

#if (_MIPS_SZLONG == 64)

typedef __signed__ long s64;
typedef unsigned long u64;

#else

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __signed__ long long s64;
typedef unsigned long long u64;
#endif

#endif

#define BITS_PER_LONG _MIPS_SZLONG

typedef unsigned long dma_addr_t;

#endif /* __KERNEL__ */

#endif /* _ASM_TYPES_H */
