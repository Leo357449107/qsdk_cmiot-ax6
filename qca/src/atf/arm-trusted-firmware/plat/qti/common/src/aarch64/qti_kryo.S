/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <cpu_macros.S>
#include <qti_cpu.h>

#define CPU_IMPL_PN_MASK	(MIDR_IMPL_MASK << MIDR_IMPL_SHIFT) | \
				(MIDR_PN_MASK << MIDR_PN_SHIFT)

	.p2align 3

/* -------------------------------------------------
 * API to execute CPU specific cache maintenance
 * operation before power collapse.
 * qti_cpu_cm_at_pc function passes argument to determine
 * low power mode and argument X0 will be used to detect
 * L2 flush based on the cache flush
 * -------------------------------------------------
 */
	.global qti_cpu_cm_at_pc
func qti_cpu_cm_at_pc
	mov x19, x30

	/*
	 * Apply cache maintenance work around for kryo3 gold core.
	 * Ideally cache maintenance for FSM enabled core taken care at h/w.
	 * h/w cache maintenance disabled for gold core due to some errata.
	 */

	/* Read the MIDR_EL1 */
	mrs	x0, midr_el1
	mov_imm	x1, CPU_IMPL_PN_MASK

	/* Retain only the implementation and part number using mask */
	and	x0, x0, x1

	/* Check if the core is Kryo3 Gold core. */
	mov_imm	x1, QTI_KRYO3_GOLD_IMPL_PN
	cmp	x0, x1
	bne	qti_cpu_cm_at_pc_exit

	/* Disable data caches. */
	mrs	x1, sctlr_el3
	bic	x1, x1, #SCTLR_C_BIT
	msr	sctlr_el3, x1
	isb

	/* Flush L1 caches. */
	mov	x0, #DCCISW
	bl	dcsw_op_level1

	/* Flush L2 caches. */
	mov	x0, #DCCISW
	bl	dcsw_op_level2

qti_cpu_cm_at_pc_exit:
	mov	x30, x19
	ret

endfunc qti_cpu_cm_at_pc
