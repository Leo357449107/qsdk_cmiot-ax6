/*
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __QTISECLIB_INTERFACE_H__
#define __QTISECLIB_INTERFACE_H__

#include <stdint.h>
#include <stdbool.h>
#include <qtiseclib_defs.h>

/*----------------------------------------------------------------------------
 * QTISECLIB Published API's.
 * -------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Assembly API's
 * -------------------------------------------------------------------------*/

/*
 * CPUSS common reset handler for all CPU wake up (both cold & warm boot).
 * Executes on all core. This API assume serialization across CPU
 * already taken care before invoking.
 *
 * Clobbers: x0 - x17, x30
 */
void qtiseclib_cpuss_reset_asm(uint32_t bl31_cold_boot_state);

/*
 * Execute CPU (Kryo3 gold) specific reset handler / system initialization.
 * This takes care of executing required CPU errata's.
 *
 * Clobbers: x0 - x16
 */
void qtiseclib_kryo3_gold_reset_asm(void);

/*
 * Execute CPU (Kryo3 silver) specific reset handler / system initialization.
 * This takes care of executing required CPU errata's.
 *
 * Clobbers: x0 - x16
 */
void qtiseclib_kryo3_silver_reset_asm(void);

/*----------------------------------------------------------------------------
 * C Api's
 * -------------------------------------------------------------------------*/
void qtiseclib_bl31_platform_setup(void);
void qtiseclib_get_entrypoint_param(uint64_t, entry_point_info_t *);
void qtiseclib_invoke_isr(uint32_t irq, void *handle);
void qtiseclib_panic(void);
int qtiseclib_prng_get_data(uint8_t *out, uint32_t out_len);

int qtiseclib_mem_assign( u_register_t	IPAinfo_hyp,
						  u_register_t	IPAinfolistsize,
						  u_register_t	sourceVMlist_hyp,
						  u_register_t	srcVMlistsize,
						  u_register_t	destVMlist_hyp,
						  u_register_t	dstVMlistsize,
						  u_register_t	spare);

void qtiseclib_oem_register_wifi_interrupt(int irq);
void qtiseclib_Clock_Init(void);
#if !QTI_6018_PLATFORM && !QTI_5018_PLATFORM
void qtiseclib_oem_command_read(qtiseclib_oem_cmd_buf_t *cbuf);
void qtiseclib_oem_command_write(qtiseclib_oem_cmd_buf_t *cbuf);
#endif
int qtiseclib_psci_init(void);
int qtiseclib_psci_node_power_on(u_register_t mpidr);
void qtiseclib_psci_node_on_finish(const uint8_t * states);
void qtiseclib_psci_cpu_standby(uint8_t pwr_state);
void qtiseclib_psci_node_power_off(const uint8_t * states);
void qtiseclib_psci_node_suspend(const uint8_t * states);
void qtiseclib_psci_node_suspend_finish(const uint8_t * states);
void qtiseclib_save_dev_data_addr( void* data);
__attribute__((noreturn)) void qtiseclib_psci_system_off(void);
__attribute__((noreturn)) void qtiseclib_psci_system_reset(void);
void qtiseclib_disable_cluster_coherency(uint8_t state);
int qtiseclib_psci_validate_power_state(unsigned int pwr_state, uint8_t * req_state);
int qtiseclib_config_reset_debug(uint32_t, uint32_t);
int qtiseclib_set_cpu_ctx_buf(uintptr_t addr, uint32_t size);
int qtiseclib_protect_mem_subsystem(uint32_t subsystem_id, uintptr_t phy_base, uint32_t size);
int qtiseclib_clear_protect_mem_subsystem(uint32_t subsystem_id, uintptr_t phy_base, uint32_t size, uint32_t auth_key);
int qtiseclib_get_diag(char* buf, size_t buf_size);
uint64_t qtiseclib_get_ddr_size();
#if QTI_5018_PLATFORM
int qtiseclib_bt_fuse_copy(void);
#endif
#endif /* __QTISECLIB_INTERFACE_H__ */
