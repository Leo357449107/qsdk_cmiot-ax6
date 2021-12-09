/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <cpu_macros.S>
#include <plat_macros.S>
#include <qti_cpu.h>

	/* ---------------------------------------------
	 * Clean RW sections used by ATF
	 * This includes 'data' and 'bss'
	 * ---------------------------------------------
	 */
	.p2align 3
.type qti_a53_clean_atf, %function
qti_a53_clean_atf:
	mov	x17, x30
	// clean .data section till POC
	ldr	x0, =__DATA_START__
	ldr	x1, =__DATA_END__
	sub	x1, x1, x0
	bl	clean_dcache_range
	// clean .bss section till POC
	ldr	x0, =__BSS_START__
	ldr	x1, =__BSS_END__
	sub	x1, x1, x0
	bl	clean_dcache_range
	mov	x30, x17
	ret	// End of function

	/* ---------------------------------------------
	 * Disable L1 data cache and unified L2 cache
	 * ---------------------------------------------
	 */
	.p2align 3
func qti_a53_disable_dcache
	mrs	x1, sctlr_el3
	bic	x1, x1, #SCTLR_C_BIT
	msr	sctlr_el3, x1
	isb
	ret
endfunc qti_a53_disable_dcache

/* -------------------------------------------------
 * The CPU Ops reset function for Kryo-3 Gold
 * -------------------------------------------------
 */
func qti_a53_reset_func

	mov x19, x30

	bl qtiseclib_a53_cluster_reset_func

	mov	x30, x19
	ret

endfunc qti_a53_reset_func

/* ----------------------------------------------------
 * The CPU Ops core power down function for Kryo-3 Gold
 * ----------------------------------------------------
 */
func qti_a53_core_pwr_dwn
	ret
endfunc qti_a53_core_pwr_dwn

/* -------------------------------------------------------
 * The CPU Ops cluster power down function for Kryo-3 Gold
 * -------------------------------------------------------
 */
func qti_a53_cluster_pwr_dwn
	ret
endfunc qti_a53_cluster_pwr_dwn

func qti_a53_errata_report
	ret
endfunc qti_a53_errata_report

/* ---------------------------------------------
 * This function provides kryo3_gold specific
 * register information for crash reporting.
 * It needs to return with x6 pointing to
 * a list of register names in ascii and
 * x8 - x15 having values of registers to be
 * reported.
 * ---------------------------------------------
 */
.section .rodata.qti_a53_regs, "aS"
qti_a53_regs:  /* The ascii list of register names to be reported */
	.asciz	""

func qti_a53_cpu_reg_dump
	adr	x6, qti_a53_regs
	ret
endfunc qti_a53_cpu_reg_dump

declare_cpu_ops	qti_a53, QTI_A53_MIDR,	\
		qti_a53_reset_func,		\
		qti_a53_core_pwr_dwn,	\
		qti_a53_cluster_pwr_dwn
