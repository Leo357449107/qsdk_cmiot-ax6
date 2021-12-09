/* Copyright (c) 2012, 2014 The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
 *
 */

#ifndef __BOARD_H
#define __BOARD_H

#include <smem.h>

#define BOARD_SOC_VERSION1 0x10000
#define BOARD_SOC_VERSION2 0x20000
#define BOARD_SOC_VERSION2P5 0x30000
#define LINUX_MACHTYPE_UNKNOWN 0
#define MAX_PMIC_DEVICES       SMEM_MAX_PMIC_DEVICES

struct board_pmic_data {
	uint32_t pmic_type;
	uint32_t pmic_version;
	uint32_t pmic_target;
};

struct board_data {
	uint32_t platform;
	uint32_t foundry_id;
	uint32_t platform_version;
	uint32_t platform_hw;
	uint32_t platform_subtype;
	uint32_t target;
	uint32_t baseband;
	struct board_pmic_data pmic_info[MAX_PMIC_DEVICES];
	uint32_t platform_hlos_subtype;
};

void board_init();
void target_detect(struct board_data *);
void target_baseband_detect(struct board_data *);
uint32_t board_platform_id();
uint32_t board_target_id();
uint32_t board_baseband();
uint32_t board_hardware_id();
uint32_t board_subtype_id();
uint32_t board_platform_ver();
uint32_t board_platform_ver_minor();
void fdt_fixup_atf(void *);
#endif
