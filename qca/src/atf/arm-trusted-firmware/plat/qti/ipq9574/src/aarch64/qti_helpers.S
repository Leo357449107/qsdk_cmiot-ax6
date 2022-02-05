/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <drivers/arm/gicv2.h>
#include <drivers/arm/gicv3.h>
#include <platform_def.h>
#include <drivers/console.h>
#include <console_macros.S>
#include <qti_uart_console.h>

	.globl	plat_my_core_pos
	.globl	plat_qti_core_pos_by_mpidr
	.globl	plat_reset_handler
	.globl	plat_panic_handler
	.globl	qti_diag_register
	.globl  qti_sysdbg_reset_check

	.extern sysdbg_entry_handler
	.extern due_to_resin_tsen_reset

	/* -----------------------------------------------------
	 *  unsigned int plat_qti_core_pos_by_mpidr(uint64_t mpidr)
	 *  Helper function to calculate the core position.
	 *  With this function:
	 *  CorePos = (ClusterId * 4) + CoreId
	 *  - In ARM v8   (MPIDR_EL1[24]=0)
	 *    ClusterId = MPIDR_EL1[15:8]
	 *    CoreId    = MPIDR_EL1[7:0]
	 *  - In ARM v8.1 (MPIDR_EL1[24]=1)
	 *    ClusterId = MPIDR_EL1[23:15]
	 *    CoreId    = MPIDR_EL1[15:8]
	 *  Clobbers: x0 & x1.
	 * -----------------------------------------------------
	 */
func plat_qti_core_pos_by_mpidr
	mrs	x1, mpidr_el1
	tst	x1, #MPIDR_MT_MASK
	beq	plat_qti_core_pos_by_mpidr_no_mt
	/* Right shift mpidr by one affinity level when MT=1. */
	lsr	x0, x0, #MPIDR_AFFINITY_BITS
plat_qti_core_pos_by_mpidr_no_mt:
	and	x1, x0, #MPIDR_CPU_MASK
	and	x0, x0, #MPIDR_CLUSTER_MASK
	add	x0, x1, x0, LSR #6
	ret
endfunc plat_qti_core_pos_by_mpidr

	/* --------------------------------------------------------------------
	 * void plat_panic_handler(void)
	 * calls SDI and reset system
	 * --------------------------------------------------------------------
	 */
func plat_panic_handler
	msr	spsel, #0
	bl	plat_set_my_stack
	b	qtiseclib_panic
endfunc plat_panic_handler

	/* -----------------------------------------------------
	 *  unsigned int plat_my_core_pos(void)
	 *  This function uses the plat_qti_calc_core_pos()
	 *  definition to get the index of the calling CPU
	 *  Clobbers: x0 & x1.
	 * -----------------------------------------------------
	 */
func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	plat_qti_core_pos_by_mpidr
endfunc plat_my_core_pos

func plat_reset_handler
	mov	x18, x30						/* save the lr */

	/* Serialize CPUSS boot setup. Multi core enter simultaneously. */
	ldr	x0, =g_qti_cpuss_boot_lock
	bl	spin_lock

	/* Execuete CPUSS boot set up on every core. */
	ldr	w0, g_qti_bl31_cold_booted		/* pass cold boot status. */
	bl	qtiseclib_cpuss_reset_asm

	ldr	x0, =g_qti_cpuss_boot_lock
	bl	spin_unlock

	mov	x30, x18
	ret
endfunc plat_reset_handler

	/*----------------------------------------------------
	* Register diag region as console. So that we could
	* use console related apis like printf, putchar etc.
	* with diag region alongside UART
	*-----------------------------------------------------
	*/
func qti_diag_register
	str	x1, [x0, #QTI_CONSOLE_T_UART_BASE_OFF]		/* Save UART base. */
	finish_console_register diag putc=1, getc=0, flush=0
endfunc qti_diag_register

func console_diag_putc
	stp     x29, x30, [sp,#-16]!
	mov     x29, sp
	bl log_put
	mov	w0, #0x0
	ldp     x29, x30, [sp],#16
	ret
endfunc	console_diag_putc

#define SDI_DEBUG_STATUS_CHECK_MASK	0x00000028
#define RESIN_DEBUG_STATUS_CHECK_MASK	0x00000013

func_in_section qti_sysdbg_reset_check, QTI_SYSDBG_IMEM_CODE
	movz    x0, #0x181, lsl #16
    add     x0, x0, #0x5000          /*#0x5000 is not a valid #bimm32, orr cannot be used here*/
    add     x0, x0, #0x8             /*add #0x5008 is not valid since exceeds #uimm12 that can be shifted*/
    ldr     x1, [x0]                 /*check for debug reset*/
    ldr     x0, =SDI_DEBUG_STATUS_CHECK_MASK
    and     x0, x0, x1
    cmp     x0, #0
    b.ne    sysdbg_entry_handler
    ldr     x0, =RESIN_DEBUG_STATUS_CHECK_MASK
    and     x0, x0, x1
    cmp     x0, #0
    b.ne    due_to_resin_tsen_reset
    ret
endfunc qti_sysdbg_reset_check
