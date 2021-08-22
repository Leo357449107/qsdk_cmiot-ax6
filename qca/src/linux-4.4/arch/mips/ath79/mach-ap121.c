/*
 *  Atheros AP121 board support
 *
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-spi.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define AP121_GPIO_LED_WLAN		0
#define AP121_GPIO_LED_USB		1

#define AP121_GPIO_BTN_JUMPSTART	11
#define AP121_GPIO_BTN_RESET		12

#define AP121_KEYS_POLL_INTERVAL	20	/* msecs */
#define AP121_KEYS_DEBOUNCE_INTERVAL	(3 * AP121_KEYS_POLL_INTERVAL)

#define AP121_MAC0_OFFSET		0x0000
#define AP121_MAC1_OFFSET		0x0006
#define AP121_CALDATA_OFFSET		0x1000
#define AP121_WMAC_MAC_OFFSET		0x1002

#define AP121_MINI_GPIO_LED_WLAN	0
#define AP121_MINI_GPIO_BTN_JUMPSTART	12
#define AP121_MINI_GPIO_BTN_RESET	11

static struct gpio_led ap121_leds_gpio[] __initdata = {
	{
		.name		= "ap121:green:usb",
		.gpio		= AP121_GPIO_LED_USB,
		.active_low	= 0,
	},
	{
		.name		= "ap121:green:wlan",
		.gpio		= AP121_GPIO_LED_WLAN,
		.active_low	= 0,
	},
};

static struct gpio_keys_button ap121_gpio_keys[] __initdata = {
	{
		.desc		= "jumpstart button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = AP121_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= AP121_GPIO_BTN_JUMPSTART,
		.active_low	= 1,
	},
	{
		.desc		= "reset button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = AP121_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= AP121_GPIO_BTN_RESET,
		.active_low	= 1,
	}
};

static struct gpio_led ap121_mini_leds_gpio[] __initdata = {
	{
		.name		= "ap121:green:wlan",
		.gpio		= AP121_MINI_GPIO_LED_WLAN,
		.active_low	= 0,
	},
};

static struct gpio_keys_button ap121_mini_gpio_keys[] __initdata = {
	{
		.desc		= "jumpstart button",
		.type		= EV_KEY,
		.code		= KEY_WPS_BUTTON,
		.debounce_interval = AP121_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= AP121_MINI_GPIO_BTN_JUMPSTART,
		.active_low	= 1,
	},
	{
		.desc		= "reset button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = AP121_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= AP121_MINI_GPIO_BTN_RESET,
		.active_low	= 1,
	}
};

static void __init ap121_common_setup(void)
{
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	ath79_register_m25p80(NULL);
	ath79_register_wmac(art + AP121_CALDATA_OFFSET,
			    art + AP121_WMAC_MAC_OFFSET);

	ath79_init_mac(ath79_eth0_data.mac_addr, art + AP121_MAC0_OFFSET, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, art + AP121_MAC1_OFFSET, 0);

	ath79_register_mdio(0, 0x0);

	/* LAN ports */
	ath79_register_eth(1);

	/* WAN port */
	ath79_register_eth(0);
}

static void __init ap121_setup(void)
{
	ap121_common_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(ap121_leds_gpio),
				 ap121_leds_gpio);
	ath79_register_gpio_keys_polled(-1, AP121_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(ap121_gpio_keys),
					ap121_gpio_keys);
	ath79_register_usb();
}

MIPS_MACHINE(ATH79_MACH_AP121, "AP121", "Atheros AP121 reference board",
	     ap121_setup);

static void __init ap121_mini_setup(void)
{
	ap121_common_setup();

	ath79_register_leds_gpio(-1, ARRAY_SIZE(ap121_mini_leds_gpio),
				 ap121_mini_leds_gpio);
	ath79_register_gpio_keys_polled(-1, AP121_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(ap121_mini_gpio_keys),
					ap121_mini_gpio_keys);
}

MIPS_MACHINE(ATH79_MACH_AP121_MINI, "AP121-MINI", "Atheros AP121-MINI",
	     ap121_mini_setup);
