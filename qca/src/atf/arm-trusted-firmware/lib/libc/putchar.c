/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include <drivers/console.h>

#if QTI_UART_PRINT
int uart_putc(const char ch);
#endif

int putchar(int c)
{
	int res;
#if QTI_UART_PRINT
	if (uart_putc((unsigned char)c) >= 0)
#else
	if (console_putc((unsigned char)c) >= 0)
#endif
		res = c;
	else
		res = EOF;

	return res;
}
