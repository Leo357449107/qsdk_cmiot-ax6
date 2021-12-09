/*
 * (C) Copyright 2007-2012
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _SYS_PROTO_H_
#define _SYS_PROTO_H_

#include <linux/types.h>

void sdelay(unsigned long);

/* return_to_fel() - Return to BROM from SPL
 *
 * This returns back into the BROM after U-Boot SPL has performed its initial
 * init. It uses the provided lr and sp to do so.
 *
 * @lr:		BROM link register value (return address)
 * @sp:		BROM stack pointer
 */
void return_to_fel(uint32_t lr, uint32_t sp);

/* Board / SoC level designware gmac init */
int sunxi_gmac_initialize(bd_t *bis);

#endif
