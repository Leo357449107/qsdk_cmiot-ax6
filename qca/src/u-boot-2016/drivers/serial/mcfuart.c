/*
 * (C) Copyright 2004-2007 Freescale Semiconductor, Inc.
 * TsiChung Liew, Tsi-Chung.Liew@freescale.com.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/*
 * Minimal serial functions needed to use one of the uart ports
 * as serial console interface.
 */

#include <common.h>
#include <serial.h>
#include <linux/compiler.h>

#include <asm/immap.h>
#include <asm/uart.h>

DECLARE_GLOBAL_DATA_PTR;

extern void uart_port_conf(int port);

static int mcf_serial_init(void)
{
	volatile uart_t *uart;
	u32 counter;

	uart = (volatile uart_t *)(CONFIG_SYS_UART_BASE);

	uart_port_conf(CONFIG_SYS_UART_PORT);

	/* write to SICR: SIM2 = uart mode,dcd does not affect rx */
	uart->ucr = UART_UCR_RESET_RX;
	uart->ucr = UART_UCR_RESET_TX;
	uart->ucr = UART_UCR_RESET_ERROR;
	uart->ucr = UART_UCR_RESET_MR;
	__asm__("nop");

	uart->uimr = 0;

	/* write to CSR: RX/TX baud rate from timers */
	uart->ucsr = (UART_UCSR_RCS_SYS_CLK | UART_UCSR_TCS_SYS_CLK);

	uart->umr = (UART_UMR_BC_8 | UART_UMR_PM_NONE);
	uart->umr = UART_UMR_SB_STOP_BITS_1;

	/* Setting up BaudRate */
	counter = (u32) ((gd->bus_clk / 32) + (gd->baudrate / 2));
	counter = counter / gd->baudrate;

	/* write to CTUR: divide counter upper byte */
	uart->ubg1 = (u8) ((counter & 0xff00) >> 8);
	/* write to CTLR: divide counter lower byte */
	uart->ubg2 = (u8) (counter & 0x00ff);

	uart->ucr = (UART_UCR_RX_ENABLED | UART_UCR_TX_ENABLED);

	return (0);
}

static void mcf_serial_putc(const char c)
{
	volatile uart_t *uart = (volatile uart_t *)(CONFIG_SYS_UART_BASE);

	if (c == '\n')
		serial_putc('\r');

	/* Wait for last character to go. */
	while (!(uart->usr & UART_USR_TXRDY)) ;

	uart->utb = c;
}

static int mcf_serial_getc(void)
{
	volatile uart_t *uart = (volatile uart_t *)(CONFIG_SYS_UART_BASE);

	/* Wait for a character to arrive. */
	while (!(uart->usr & UART_USR_RXRDY)) ;
	return uart->urb;
}

static int mcf_serial_tstc(void)
{
	volatile uart_t *uart = (volatile uart_t *)(CONFIG_SYS_UART_BASE);

	return (uart->usr & UART_USR_RXRDY);
}

static void mcf_serial_setbrg(void)
{
	volatile uart_t *uart = (volatile uart_t *)(CONFIG_SYS_UART_BASE);
	u32 counter;

	/* Setting up BaudRate */
	counter = (u32) ((gd->bus_clk / 32) + (gd->baudrate / 2));
	counter = counter / gd->baudrate;

	/* write to CTUR: divide counter upper byte */
	uart->ubg1 = ((counter & 0xff00) >> 8);
	/* write to CTLR: divide counter lower byte */
	uart->ubg2 = (counter & 0x00ff);

	uart->ucr = UART_UCR_RESET_RX;
	uart->ucr = UART_UCR_RESET_TX;

	uart->ucr = UART_UCR_RX_ENABLED | UART_UCR_TX_ENABLED;
}

static struct serial_device mcf_serial_drv = {
	.name	= "mcf_serial",
	.start	= mcf_serial_init,
	.stop	= NULL,
	.setbrg	= mcf_serial_setbrg,
	.putc	= mcf_serial_putc,
	.puts	= default_serial_puts,
	.getc	= mcf_serial_getc,
	.tstc	= mcf_serial_tstc,
};

void mcf_serial_initialize(void)
{
	serial_register(&mcf_serial_drv);
}

__weak struct serial_device *default_serial_console(void)
{
	return &mcf_serial_drv;
}
