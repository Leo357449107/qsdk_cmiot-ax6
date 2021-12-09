/*
 * Copyright (c) 2012-2019 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

board_ipq6018_params_t *gboard_param;

gpio_func_data_t uart2_gpio_cp02[] = {
	{
		.gpio = 44,
		.func = 1,
		.pull = GPIO_NO_PULL,
		.oe = GPIO_OE_ENABLE
	},
	{
		.gpio = 45,
		.func = 1,
		.pull = GPIO_NO_PULL,
		.oe = GPIO_OE_ENABLE
	},
};

uart_cfg_t uart2_console_uart_cp02 = {
	.base           = GSBI_1,
	.gsbi_base      = 0,
	.uart_dm_base = UART2_DM_BASE,
	.dbg_uart_gpio = uart2_gpio_cp02,
};

/* Board specific parameter Array */
board_ipq6018_params_t board_params[] = {
	{
		.machid = MACH_TYPE_IPQ6018_AP_CP02_1_C1,
		.console_uart_cfg = &uart2_console_uart_cp02,
		.dtb_config_name = { "config@cp02-c1" },
	},
};

#define NUM_IPQ6018_BOARDS	ARRAY_SIZE(board_params)

board_ipq6018_params_t *get_board_param(unsigned int machid)
{
	unsigned int index = 0;

	if (gboard_param)
		return gboard_param;

	for (index = 0; index < NUM_IPQ6018_BOARDS; index++) {
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

void update_usb_mode(void *fdt)
{
	return;
}

#define DLOAD_DISABLE	0x1
void fdt_fixup_atf(void *fdt)
{
	uint32_t value;
	int offset, ret;
	uint32_t dload = htonl(DLOAD_DISABLE);

	/* Set the dload_status to DLOAD_DISABLE */
	offset = fdt_path_offset(fdt, "/soc/qca,scm_restart_reason");
	if (offset < 0) {
		offset = fdt_path_offset(fdt, "/qti,scm_restart_reason");
	}

	if (offset >= 0) {
		ret = fdt_setprop(fdt, offset, "dload_status",
					&dload, sizeof(dload));
		if (ret) {
			dprintf(CRITICAL, "unable to set dload_status\n");
		}
	}

	value = readl(ATF_FUSE);
	if (value & ATF_ENABLED) {
		offset = fdt_path_offset(fdt, "/soc/q6v5_wcss@CD00000");
		ret = fdt_delprop((void *)fdt, offset, "qca,secure");
		if (ret) {
			dprintf(CRITICAL, "%s: Unable to delete the property\n", __func__);
			return;
		}
		value = 1;
		offset = fdt_path_offset(fdt, "/soc/qca,scm_restart_reason");
		ret = fdt_setprop((void *)fdt, offset, "qca,coldreboot-enabled", &value, sizeof(value));
		if (ret) {
			dprintf(CRITICAL, "%s: Unable to set the property\n", __func__);
			return;
		}
	}
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
