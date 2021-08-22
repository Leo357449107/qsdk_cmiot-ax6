/*
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __QTISECLIB_CB_INTERFACE_H__
#define __QTISECLIB_CB_INTERFACE_H__

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <qtiseclib_defs.h>

/* Standard Library API's */
void *qtiseclib_cb_memcpy(void *dst, const void *src, size_t len);

#define QTISECLIB_CB_ERROR(...)		qtiseclib_cb_log(QTISECLIB_LOG_LEVEL_ERROR, __VA_ARGS__)
#define QTISECLIB_CB_NOTICE(...)	qtiseclib_cb_log(QTISECLIB_LOG_LEVEL_NOTICE, __VA_ARGS__)
#define QTISECLIB_CB_WARN(...)		qtiseclib_cb_log(QTISECLIB_LOG_LEVEL_WARNING, __VA_ARGS__)
#define QTISECLIB_CB_INFO(...)		qtiseclib_cb_log(QTISECLIB_LOG_LEVEL_INFO, __VA_ARGS__)

void qtiseclib_cb_log(unsigned int loglvl, const char *fmt, ...);

void qtiseclib_cb_spin_lock(qtiseclib_cb_spinlock_t * lock);
void qtiseclib_cb_spin_unlock(qtiseclib_cb_spinlock_t * lock);

unsigned int qtiseclib_cb_plat_my_core_pos(void);
int qtiseclib_cb_plat_core_pos_by_mpidr(u_register_t mpidr);
unsigned int qtiseclib_cb_plat_my_cluster_pos(void);

void qtiseclib_cb_clear_interrupt_pending(unsigned int id);
void qtiseclib_cb_set_interrupt_pending(unsigned int irq);
uintptr_t qtiseclib_cb_get_warmboot_entry_addr(void);
uintptr_t qtiseclib_cb_get_entry_addr(void);

int qtiseclib_cb_mmap_add_dynamic_region(unsigned long long base_pa,
					 uintptr_t base_va, size_t size,
					 qtiseclib_mmap_attr_t attr);

void qtiseclib_cb_inv_dcache_range(uintptr_t addr, size_t size);

void qtiseclib_cb_tlbialle3(void);

void qtiseclib_cb_flush_dcache_range(uintptr_t addr, size_t size);
void qtiseclib_cb_flush_dcache_all(void);

int qtiseclib_cb_mmap_remove_dynamic_region(uintptr_t base_va, size_t size);

/* GIC platform wrappers */
void qtiseclib_cb_gic_pcpu_init(void);
void qtiseclib_cb_gic_cpuif_enable(void);
void qtiseclib_cb_gic_cpuif_disable(void);
void qtiseclib_cb_ic_raise_sgi(int sgi_num, u_register_t target);
void qtiseclib_cb_set_spi_routing(unsigned int id, unsigned int irm, u_register_t target);

void qtiseclib_cb_get_ns_ctx(qtiseclib_dbg_a64_ctxt_regs_type *ns_ctx);

/* Crash reporting api's wrappers */
int qtiseclib_cb_crash_console_init(void);
int qtiseclib_cb_crash_console_flush(void);
uintptr_t qti_page_align(uintptr_t value, unsigned dir);
#if QTI_5018_PLATFORM
int qtiseclib_qfprom_fuse_secdat(uint32_t*);
#endif
bool qtiseclib_is_overlap_atf_rg(unsigned long long addr, size_t size);
#endif /* __QTISECLIB_CB_INTERFACE_H__ */
