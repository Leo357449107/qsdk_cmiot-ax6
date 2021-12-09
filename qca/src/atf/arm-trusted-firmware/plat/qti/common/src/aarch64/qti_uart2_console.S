/*
 * Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <asm_macros.S>
#include <console_macros.S>
#include <qti_uart_console.h>
#include <platform_def.h>

/*
 * This driver implements console logging into a ring buffer.
 */

	.globl qti_console_uart_register

	/* -----------------------------------------------
	 * int qti_console_uart_register(qti_console_uart_t *console,
	 *				 uintptr_t uart_base_addr)
	 * Registers uart console instance.
	 * In:  x0 - pointer to empty qti_console_uart_t struct
	 *      x1 - start address of uart block.
	 * Out: x0 - 1 to indicate success
	 * Clobber list: x0, x1, x14
	 * -----------------------------------------------
	 */
func qti_console_uart_register
	str	x1, [x0, #QTI_CONSOLE_T_UART_BASE_OFF]		/* Save UART base. */
	finish_console_register uart
endfunc qti_console_uart_register

	/* -----------------------------------------------
	 * int qti_console_uart_puts(int c, qti_console_uart_t *console)
	 * Writes a character to the UART console.
	 * The character must be preserved in x0.
	 * In: x0 - character to be stored
	 *     x1 - pointer to qti_console_uart_t struct
	 * Clobber list: x1, x2
	 * now this is a stub function.
	 * -----------------------------------------------
	 */
func console_uart_putc
	ret
endfunc	console_uart_putc

	/* -----------------------------------------------
	 * int qti_console_uart_flush(qti_console_uart_t *console)
	 * In:  x0 - pointer to qti_console_uart_t struct
	 * Out: x0 - 0 for success
	 * Clobber list: x0, x1
	 * now this is a stub function.
	 * -----------------------------------------------
	 */
func console_uart_flush
	mov	x0, #0
	ret
endfunc console_uart_flush
