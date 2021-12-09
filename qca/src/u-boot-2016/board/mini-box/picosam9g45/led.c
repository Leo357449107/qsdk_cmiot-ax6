/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian@popies.net>
 * Lead Tech Design <www.leadtechdesign.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/at91sam9g45.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/gpio.h>

void coloured_LED_init(void)
{
	struct at91_pmc *pmc = (struct at91_pmc *)ATMEL_BASE_PMC;

	/* Enable clock */
	writel(1 << ATMEL_ID_PIODE, &pmc->pcer);

	at91_set_gpio_output(CONFIG_GREEN_LED, 1);

	at91_set_gpio_value(CONFIG_GREEN_LED, 1);
}
