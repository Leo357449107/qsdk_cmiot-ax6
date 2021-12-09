/*
 * Copyright (c) 2011 The Chromium OS Authors.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __SANDBOX_CACHE_H__
#define __SANDBOX_CACHE_H__

/*
 * For native compilation of the sandbox we should still align
 * the contents of stack buffers to something reasonable.  The
 * GCC macro __BIGGEST_ALIGNMENT__ is defined to be the maximum
 * required alignment for any basic type.  This seems reasonable.
 */
#define ARCH_DMA_MINALIGN	__BIGGEST_ALIGNMENT__

#endif /* __SANDBOX_CACHE_H__ */
