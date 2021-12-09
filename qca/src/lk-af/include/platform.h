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
#ifndef __PLATFORM_H
#define __PLATFORM_H

#include <msm_panel.h>

#define PA(x) platform_get_virt_to_phys_mapping(x)
#define VA(x) platform_get_phys_to_virt_mapping(x)

time_t current_time(void);
bigtime_t current_time_hires(void);

/* super early platform initialization, before almost everything */
void platform_early_init(void);

/* later init, after the kernel has come up */
void platform_init(void);

/* called by the arch init code to get the platform to set up any mmu mappings it may need */
int platform_use_identity_mmu_mappings(void);
void platform_init_mmu_mappings(void);
addr_t platform_get_virt_to_phys_mapping(addr_t virt_addr);
addr_t platform_get_phys_to_virt_mapping(addr_t phys_addr);

void display_init(void);
void display_shutdown(void);
void display_image_on_screen(struct msm_panel_info *pinfo);

unsigned board_machtype(void);
unsigned board_platform_id(void);
unsigned check_reboot_mode(void);
void platform_uninit_timer(void);
void reboot_device(unsigned);
void crashdump_init(void);

#endif
