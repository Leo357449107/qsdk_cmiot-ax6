/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef __QTI_SECURE_IO_CFG_H__
#define __QTI_SECURE_IO_CFG_H__

#include <stdint.h>

/*
 * List of peripheral/IO memory areas that are protected from
 * non-secure world but not required to be secure.
 */

static const uintptr_t qti_secure_io_allowed_regs[] = {
	0x193d100,
};

#endif /* __QTI_SECURE_IO_CFG_H__ */

