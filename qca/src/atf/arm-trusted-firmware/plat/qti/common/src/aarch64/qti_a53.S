/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <cpu_macros.S>
#include <qti_cpu.h>

#define CORTEX_A53_ECTLR_SMP_BIT                     (U(1) << 6)
#define CORTEX_A53_ECTLR_EL1                         S3_1_C15_C2_1
#define A53_L2_FLUSH                              U(5)

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
	mov x18, x0

	/*
	 * Apply cache maintenance work around for kryo3 gold core.
	 * Ideally cache maintenance for FSM enabled core taken care at h/w.
	 * h/w cache maintenance disabled for gold core due to some errata.
	 */


	/* Disable data caches. */
	mrs	x1, sctlr_el3
	bic	x1, x1, #SCTLR_C_BIT
	msr	sctlr_el3, x1
	isb

	/* Flush L1 caches. */
	mov	x0, #DCCISW
	bl	dcsw_op_level1

	cmp     x18, #A53_L2_FLUSH
	bne     qti_cpu_cm_at_pc_exit

	/* Flush L2 caches. */
	mov	x0, #DCCISW
	bl	dcsw_op_level2

qti_cpu_cm_at_pc_exit:
        mrs     x0, CORTEX_A53_ECTLR_EL1
	bic     x0, x0, #CORTEX_A53_ECTLR_SMP_BIT
	msr     CORTEX_A53_ECTLR_EL1, x0
	mov	x30, x19
	isb
	dsb sy
	ret

endfunc qti_cpu_cm_at_pc
