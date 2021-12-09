/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <err.h>
#include <debug.h>
#include <arch/arm/mmu.h>
#include <platform.h>
#include "platform_p.h"
#include <platform/omap3.h>
#include <dev/i2c.h>
#include <dev/uart.h>
#include <dev/usbc.h>

void platform_init_mmu_mappings(void)
{
	/* do some memory map initialization */
	addr_t addr;
	arm_mmu_map_section(SDRAM_BASE, 0, MMU_FLAG_CACHED|MMU_FLAG_BUFFERED);
	for (addr = SDRAM_BASE; addr < SDRAM_BASE + SDRAM_SIZE; addr += (1024*1024)) {
		arm_mmu_map_section(addr, addr, MMU_FLAG_CACHED|MMU_FLAG_BUFFERED|MMU_FLAG_READWRITE);
	}
}

void platform_early_init(void)
{
	/* initialize the interrupt controller */
	platform_init_interrupts();

	/* initialize the timer block */
	platform_init_timer();

	/* initialize the uart */
	uart_init_early();

	i2c_init_early();
}

void platform_init(void)
{
	i2c_init();

	uart_init();

	usbc_init();
}

