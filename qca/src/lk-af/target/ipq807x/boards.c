
/*
 * Copyright (c) 2012-2018 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <debug.h>
#include <lib/ptable.h>
#include <smem.h>
#include <platform/iomap.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <reg.h>
#include <app.h>
#include <gsbi.h>
#include <target.h>
#include <platform.h>
#include <uart_dm.h>
#include <crypto_hash.h>
#include <board.h>
#include <target/board.h>
#include <partition_parser.h>
#include <stdlib.h>
#include <libfdt.h>
#include <mmc_wrapper.h>
#include <err.h>

board_ipq807x_params_t *gboard_param;

gpio_func_data_t uart4_gpio_hk01[] = {
	{
		.gpio = 23,
		.func = 2,
		.pull = GPIO_PULL_DOWN,
		.oe = GPIO_OE_ENABLE
	},
	{
		.gpio = 24,
		.func = 2,
		.pull = GPIO_NO_PULL,
		.oe = GPIO_OE_ENABLE
	},
};

uart_cfg_t uart4_console_uart_hk01 = {
	.base           = GSBI_1,
	.gsbi_base      = 0,
	.uart_dm_base = UART4_DM_BASE,
	.dbg_uart_gpio = uart4_gpio_hk01,
};

/* Board specific parameter Array */
board_ipq807x_params_t board_params[] = {
	{
		.machid = MACH_TYPE_IPQ807X_AP_HK01_1_C1,
		.console_uart_cfg = &uart4_console_uart_hk01,
		.dtb_config_name = { "config@hk01" },
	},
	{
		.machid = MACH_TYPE_IPQ807X_AP_HK01_1_C2,
		.console_uart_cfg = &uart4_console_uart_hk01,
		.dtb_config_name = { "config@hk01.c2" },
	},
	{
		.machid = MACH_TYPE_IPQ807X_AP_HK01_1_C3,
		.console_uart_cfg = &uart4_console_uart_hk01,
		.dtb_config_name = { "config@hk01.c3" },
	},
	{
		.machid = MACH_TYPE_IPQ807X_AP_HK01_1_C4,
		.console_uart_cfg = &uart4_console_uart_hk01,
		.dtb_config_name = { "config@hk01.c4" },
	},
	{
		.machid = MACH_TYPE_IPQ807X_AP_HK01_1_C5,
		.console_uart_cfg = &uart4_console_uart_hk01,
		.dtb_config_name = { "config@hk01.c5" },
	},
};

#define NUM_IPQ807X_BOARDS	ARRAY_SIZE(board_params)

board_ipq807x_params_t *get_board_param(unsigned int machid)
{
	unsigned int index = 0;

	if (gboard_param)
		return gboard_param;

	for (index = 0; index < NUM_IPQ807X_BOARDS; index++) {
		if (machid == board_params[index].machid) {
			gboard_param = &board_params[index];
			return &board_params[index];
		}
	}

	printf("cdp: Invalid machine id 0x%x\n", machid);

	for (;;);

	return NULL;
}

static inline int
valid_mac_addr(const unsigned char *mac)
{
	if (!mac ||
            (mac[0] & 1) ||	/* broadcast/multicast addresses */
	    ((mac[0] == 0) && (mac[1] == 0) && (mac[2] == 0) &&
             (mac[3] == 0) && (mac[4] == 0) && (mac[5] == 0)))
		return 0;	/* Invalid */

	return 1;		/* Valid */
}

#define IPQ_GMAC_COUNT		2

#define MAC_ADDR_FMT		"%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ADDR_ARG(x)		(x)[0], (x)[1], (x)[2], (x)[3], (x)[4], (x)[5]

void update_mac_addrs(void *fdt)
{
	int i, j, index;
	unsigned long long off;
	unsigned char *mac;
	char eth[] = { "ethernetX" };

	index = partition_get_index("0:ART");
	if (index == INVALID_PTN) {
		critical("ART partition not found, can't get MAC addresses\n");
		return;
	}

	off = partition_get_offset(index);
	if (off == 0ull) {
		critical("ART partition offset invalid\n");
		return;
	}

	mac = memalign(BLOCK_SIZE, BLOCK_SIZE);
	if (mac == NULL) {
		critical("Could not allocate sufficient memory to read MAC information\n");
		return;
	}
	if (mmc_read(off, (unsigned int *)mac, BLOCK_SIZE)) {
		critical("Could not read ART partition\n");
		return;
	}

	for (i = j = 0; i < IPQ_GMAC_COUNT; i++) {
		unsigned char *p = &mac[j * 6];

		snprintf(eth, sizeof(eth), "ethernet%d", i);

		if (!valid_mac_addr(p)) {
			critical("Ignoring " MAC_ADDR_FMT " for %s\n",
					MAC_ADDR_ARG(p), eth);
			j++;
			continue;
		}

		index = fdt_path_offset(fdt, eth);
		if (index < 0) {
			info("Skipping %s\n", eth);
			continue;
		}

		info("Setting " MAC_ADDR_FMT " for %s\n", MAC_ADDR_ARG(p), eth);

		if (fdt_setprop(fdt, index, "local-mac-address", p, 6) < 0) {
			critical("DT update [" MAC_ADDR_FMT "] failed for %s\n",
					MAC_ADDR_ARG(p), eth);
			continue;
		}
		j++;
	}

	free(mac);
}

int usb_mode_exists(char *str, char *sub, int len)
{
	int i = 0, j = 0, k = 0;
	int sub_size  = strlen(sub);

	while (i < len) {
		while (sub[k] == str[j] && sub[k] != '\0') {
			k++;
			j++;
		}
		if (k == sub_size)
			return 1;
		i++;
		j = i;
		k = 0;
	}
	return 0;
}

void update_usb_mode(void *fdt)
{
	int index, nodeoff, ret;
	unsigned int node;
	unsigned long long off;
	char *appsblenv_part;
	char usb_mode[20] = "usb_mode=peripheral";
	const char *usb_node[] = {"/soc/usb3@8A00000/dwc3@8A00000"};
	const char *usb_dr_mode = "peripheral"; /* Supported mode */
	const char *usb_max_speed = "high-speed";/* Supported speed */

	index = partition_get_index("0:APPSBLENV");
	if (index == INVALID_PTN) {
		critical("APPSBLENV partition not found\n");
		return;
	}

	off = partition_get_offset(index);
	if (off == 0ull) {
		critical("APPSBLENV partition offset invalid\n");
		return;
	}

	appsblenv_part = memalign(BLOCK_SIZE, BLOCK_SIZE * 2);
	if (appsblenv_part == NULL) {
		critical("Could not allocate sufficient memory to read env variables\n");
		return;
	}
	if (mmc_read(off, (unsigned int *)appsblenv_part, BLOCK_SIZE * 2)) {
		critical("Could not read APPSBLENV partition\n");
		return;
	}

	if (usb_mode_exists((char *)appsblenv_part, usb_mode, BLOCK_SIZE * 2)) {
		for (node = 0; node < ARRAY_SIZE(usb_node); node++) {
			nodeoff = fdt_path_offset(fdt, usb_node[node]);
			if (nodeoff < 0) {
				critical("fixup_usb: unable to find node '%s'\n",
						usb_node[node]);
				return;
			}
			ret = fdt_setprop(fdt, nodeoff, "dr_mode",
					usb_dr_mode,
					(strlen(usb_dr_mode) + 1));
			if (ret)
				critical("fixup_usb: 'dr_mode' cannot be set");

			/* if mode is peripheral restricting to high-speed */
			ret = fdt_setprop(fdt, nodeoff, "maximum-speed",
					usb_max_speed,
					(strlen(usb_max_speed) + 1));
			if (ret)
				critical("fixup_usb: 'maximum-speed' cannot be set");
			critical("Setting up USB dr_mode = %s for dt node = %s based \
				 on env variable usb_mode\n", usb_dr_mode, usb_node[node]);
		}
	}
	free(appsblenv_part);
}

void fdt_fixup_version(void *fdt)
{
	int offset, ret;
	char ver[OEM_VERSION_STRING_LENGTH + VERSION_STRING_LENGTH + 1];

	offset = fdt_path_offset(fdt, "/");

	if (!smem_get_build_version(ver, sizeof(ver), BOOT_VERSION)) {
		ret = fdt_setprop((void *)fdt, offset, "boot_version", ver, strlen(ver));
		if (ret)
			dprintf(CRITICAL, "fdt-fixup: Unable to set Boot version\n");
	}

	if (!smem_get_build_version(ver, sizeof(ver), TZ_VERSION)) {
		ret = fdt_setprop((void *)fdt, offset, "tz_version", ver, strlen(ver));
		if (ret)
			dprintf(CRITICAL, "fdt-fixup: Unable to set TZ version\n");
	}

	if (!smem_get_build_version(ver, sizeof(ver), RPM_VERSION)) {
		ret = fdt_setprop((void *)fdt, offset, "rpm_version", ver, strlen(ver));
		if (ret)
			dprintf(CRITICAL, "fdt-fixup: Unable to set rpm version\n");
	}

	return;
}
