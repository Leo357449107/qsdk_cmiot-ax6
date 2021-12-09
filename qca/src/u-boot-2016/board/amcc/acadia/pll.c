/*
 * (C) Copyright 2007
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/processor.h>
#include <asm/ppc405.h>

/* test-only: move into cpu directory!!! */

#if defined(PLLMR0_200_133_66)
void board_pll_init_f(void)
{
	/*
	 * set PLL clocks based on input sysclk is 33M
	 *
	 * ----------------------------------
	 * | CLK   | FREQ (MHz) | DIV RATIO |
	 * ----------------------------------
	 * | CPU   |  200.0     |   4 (0x02)|
	 * | PLB   |  133.3     |   6 (0x06)|
	 * | OPB   |   66.6     |  12 (0x0C)|
	 * | EBC   |   66.6     |  12 (0x0C)|
	 * | SPI   |   66.6     |  12 (0x0C)|
	 * | UART0 |   10.0     |  40 (0x28)|
	 * | UART1 |   10.0     |  40 (0x28)|
	 * | DAC   |    2.0     | 200 (0xC8)|
	 * | ADC   |    2.0     | 200 (0xC8)|
	 * | PWM   |  100.0     |   4 (0x04)|
	 * | EMAC  |   25.0     |  16 (0x10)|
	 * -----------------------------------
	 */

	/* Initialize PLL */
	mtcpr(CPR0_PLLC, 0x0000033c);
	mtcpr(CPR0_PLLD, 0x0c010200);
	mtcpr(CPR0_PRIMAD, 0x04060c0c);
	mtcpr(CPR0_PERD0, 0x000c0000);	/* SPI clk div. eq. OPB clk div. */
	mtcpr(CPR0_CLKUPD, 0x40000000);
}

#elif defined(PLLMR0_266_160_80)

void board_pll_init_f(void)
{
	/*
	 * set PLL clocks based on input sysclk is 33M
	 *
	 * ----------------------------------
	 * | CLK   | FREQ (MHz) | DIV RATIO |
	 * ----------------------------------
	 * | CPU   |  266.64    |   3       |
	 * | PLB   |  159.98    |   5 (0x05)|
	 * | OPB   |   79.99    |  10 (0x0A)|
	 * | EBC   |   79.99    |  10 (0x0A)|
	 * | SPI   |   79.99    |  10 (0x0A)|
	 * | UART0 |   28.57    |   7 (0x07)|
	 * | UART1 |   28.57    |   7 (0x07)|
	 * | DAC   |   28.57    |   7 (0xA7)|
	 * | ADC   |    4       |  50 (0x32)|
	 * | PWM   |   28.57    |   7 (0x07)|
	 * | EMAC  |    4       |  50 (0x32)|
	 * -----------------------------------
	 */

	/* Initialize PLL */
	mtcpr(CPR0_PLLC, 0x20000238);
	mtcpr(CPR0_PLLD, 0x03010400);
	mtcpr(CPR0_PRIMAD, 0x03050a0a);
	mtcpr(CPR0_PERC0, 0x00000000);
	mtcpr(CPR0_PERD0, 0x070a0707);	/* SPI clk div. eq. OPB clk div. */
	mtcpr(CPR0_PERD1, 0x07323200);
	mtcpr(CPR0_CLKUP, 0x40000000);
}

#elif defined(PLLMR0_333_166_83)

void board_pll_init_f(void)
{
	/*
	 * set PLL clocks based on input sysclk is 33M
	 *
	 * ----------------------------------
	 * | CLK   | FREQ (MHz) | DIV RATIO |
	 * ----------------------------------
	 * | CPU   |  333.33    |   2       |
	 * | PLB   |  166.66    |   4 (0x04)|
	 * | OPB   |   83.33    |   8 (0x08)|
	 * | EBC   |   83.33    |   8 (0x08)|
	 * | SPI   |   83.33    |   8 (0x08)|
	 * | UART0 |   16.66    |   5 (0x05)|
	 * | UART1 |   16.66    |   5 (0x05)|
	 * | DAC   |   ????     | 166 (0xA6)|
	 * | ADC   |   ????     | 166 (0xA6)|
	 * | PWM   |   41.66    |   3 (0x03)|
	 * | EMAC  |   ????     |   3 (0x03)|
	 * -----------------------------------
	 */

	/* Initialize PLL */
	mtcpr(CPR0_PLLC, 0x0000033C);
	mtcpr(CPR0_PLLD, 0x0a010000);
	mtcpr(CPR0_PRIMAD, 0x02040808);
	mtcpr(CPR0_PERD0, 0x02080505);	/* SPI clk div. eq. OPB clk div. */
	mtcpr(CPR0_PERD1, 0xA6A60300);
	mtcpr(CPR0_CLKUP, 0x40000000);
}

#elif defined(PLLMR0_100_100_12)

void board_pll_init_f(void)
{
	/*
	 * set PLL clocks based on input sysclk is 33M
	 *
	 * ----------------------
	 * | CLK   | FREQ (MHz) |
	 * ----------------------
	 * | CPU   |  100.00    |
	 * | PLB   |  100.00    |
	 * | OPB   |   12.00    |
	 * | EBC   |   49.00    |
	 * ----------------------
	 */

	/* Initialize PLL */
	mtcpr(CPR0_PLLC, 0x000003BC);
	mtcpr(CPR0_PLLD, 0x06060600);
	mtcpr(CPR0_PRIMAD, 0x02020004);
	mtcpr(CPR0_PERD0, 0x04002828);	/* SPI clk div. eq. OPB clk div. */
	mtcpr(CPR0_PERD1, 0xC8C81600);
	mtcpr(CPR0_CLKUP, 0x40000000);
}
#endif				/* CPU_<speed>_405EZ */
