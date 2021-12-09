/*
 * (C) Copyright 2008 Semihalf
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FDT_HOST_H__
#define __FDT_HOST_H__

/* Make sure to include u-boot version of libfdt include files */
#include "../include/libfdt.h"
#include "../include/fdt_support.h"

int fit_check_sign(const void *working_fdt, const void *key);

#endif /* __FDT_HOST_H__ */
