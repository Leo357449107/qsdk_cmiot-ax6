/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <common/debug.h>
#include <context.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <common/runtime_svc.h>
#include <tools_share/uuid.h>
#include <smccc_helpers.h>
#include <lib/utils_def.h>
#include <plat_qti.h>
#include <qti_secure_io_cfg.h>
#include <qtiseclib_interface.h>
#include <lib/el3_runtime/context_mgmt.h>

/*----------------------------------------------------------------------------
 * SIP service - SMC function IDs for SiP Service queries
 * -------------------------------------------------------------------------*/
#define	QTI_SIP_SVC_CALL_COUNT_ID			U(0x0200ff00)
#define	QTI_SIP_SVC_UID_ID					U(0x0200ff01)
/*							0x8200ff02 is reserved */
#define	QTI_SIP_SVC_VERSION_ID				U(0x0200ff03)

/*
 * Syscall's to allow Non Secure world accessing peripheral/IO memory
 * those are secure/proteced BUT not required to be secure.
 */
#define	QTI_SIP_SVC_SECURE_IO_READ_ID		U(0x02000501)
#define	QTI_SIP_SVC_SECURE_IO_WRITE_ID		U(0x02000502)

#define QTI_SIP_IS_SMC_AVAILABLE_ID			U(0x02000601)
#define QTI_INFO_GET_DIAG_ID			U(0x02000602)
#define QTI_SIP_SVC_AUTH_CHECK_ID               U(0x02000807)
#define QTI_DUMP_SET_CPU_CTX_BUF_ID             U(0x02000302)

#define QTI_SIP_DO_HLOS_MODE_SWITCH             U(0x0200010F)
#define QTI_SIP_PROTECT_MEM_SUBSYS_ID           U(0x0200020C)
#define QTI_SIP_CLEAR_MEM_SUBSYS_ID             U(0x0200020D)

#ifdef QTI_5018_PLATFORM
#define QTI_SIP_BLOW_FUSE_SEC_DAT_ID		U(0x02000820)
#endif
/*
 * Syscall's to assigns a list of intermediate PAs from a
 * source Virtual Machine (VM) to a destination VM.
 */
#define	QTI_SIP_SVC_MEM_ASSIGN_ID			U(0x02000C16)
#define	QTI_SIP_SVC_RESET_DEBUG_ID				U(0x02000109)

/*
 * Syscall for PIL
 */
#define QTI_SIP_SVC_PIL_INIT_ID             U(0x02000201)
#define QTI_SIP_SVC_PIL_AUTH_RESET_ID       U(0x02000205)
#define QTI_SIP_SVC_PIL_UNLOCK_XPU_ID       U(0x02000206)
#define QTI_SIP_SVC_PIL_WCSS_BREAK_DEBUG_ID U(0x02000214)
#define QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_ID   U(0x02000216)
#define QTI_SIP_SVC_PIL_USERPD1_BRINGUP_ID   U(0x02000217)
#define QTI_SIP_SVC_PIL_USERPD1_TEARDOWN_ID  U(0x02000218)
#define QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_V2_ID   U(0x02000219)

/*
 * TFTF Syscall's
 */
#define QTI_TEST_XPU_ERR_COUNT_ID               U(0x0200FE90)
#define QTI_TEST_XPU_ERR_COUNT_CLEAR_ID         U(0x0200FE91)
#define QTI_TEST_STACK_PROTECTION_ID            U(0x0200FD09)

#define QTI_SIP_SVC_AUTH_CHECK_PARAM_ID         U(0x12)
#define	QTI_SIP_SVC_SECURE_IO_READ_PARAM_ID		U(0x1)
#define	QTI_SIP_SVC_SECURE_IO_WRITE_PARAM_ID	U(0x2)
#define	QTI_SIP_SVC_RESET_DEBUG_PARAM_ID	U(0x2)
#define	QTI_SIP_SVC_MEM_ASSIGN_PARAM_ID			U(0x1117)
#define QTI_SIP_PROTECT_MEM_SUBSYS_ID_PARAM_ID	U(0x3)
#define QTI_SIP_CLEAR_MEM_SUBSYS_ID_PARAM_ID	U(0x4)
#define	QTI_SIP_SVC_CALL_COUNT			U(0x3)
#define QTI_SIP_SVC_VERSION_MAJOR		U(0x0)
#define	QTI_SIP_SVC_VERSION_MINOR		U(0x0)
#define SMC_ARMV8                             ULL(0x1)
#define QTI_SIP_SVC_PIL_INIT_PARAM_ID             U(0x82)
#define QTI_SIP_SVC_PIL_AUTH_RESET_PARAM_ID       U(0x1)
#define QTI_SIP_SVC_PIL_UNLOCK_XPU_PARAM_ID       U(0x1)
#define QTI_SIP_SVC_PIL_WCSS_BREAK_DEBUG_PARAM_ID U(0x1)
#define QTI_SIP_SVC_PIL_USERPD1_BRINGUP_PARAM_ID   U(0x1)
#define QTI_SIP_SVC_PIL_USERPD1_TEARDOWN_PARAM_ID  U(0x1)
#define QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_PARAM_ID   U(0x204)
#define QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_V2_PARAM_ID   U(0x204)

#define	FUNCID_OEN_NUM_MASK  ((FUNCID_OEN_MASK << FUNCID_OEN_SHIFT)\
				|(FUNCID_NUM_MASK << FUNCID_NUM_SHIFT) )

/* QTI SiP Service UUID */
DEFINE_SVC_UUID2(qti_sip_svc_uid,
		0x43864748, 0x217f, 0x41ad, 0xaa, 0x5a,
		0xba, 0xe7, 0x0f, 0xa5, 0x52, 0xaf);

static bool qti_is_smc_available(uint32_t smc_id)
{
/**
SMC Masks :
     Bit 31      - IRQ / Atomic / Fast SMC
     Bit 30      - ARCH
     Bit (29-24) - Owner ID
     Bit (23-16) - Reserved
     Bit (15-0)  - Func ID
**/
/*Skipping IRQ and ARCH*/
smc_id &= 0x3FFFFFFF;
	switch(smc_id)
	{
		case QTI_SIP_SVC_SECURE_IO_READ_ID:
		case QTI_SIP_SVC_SECURE_IO_WRITE_ID:
		case QTI_SIP_IS_SMC_AVAILABLE_ID:
		case QTI_INFO_GET_DIAG_ID:
		case QTI_SIP_SVC_AUTH_CHECK_ID:
		case QTI_DUMP_SET_CPU_CTX_BUF_ID:
		case QTI_SIP_DO_HLOS_MODE_SWITCH:
		case QTI_SIP_PROTECT_MEM_SUBSYS_ID:
		case QTI_SIP_CLEAR_MEM_SUBSYS_ID:
		case QTI_SIP_SVC_MEM_ASSIGN_ID:
		case QTI_SIP_SVC_RESET_DEBUG_ID:
		case QTI_TEST_XPU_ERR_COUNT_ID:
		case QTI_TEST_XPU_ERR_COUNT_CLEAR_ID:
		case QTI_TEST_STACK_PROTECTION_ID:
#ifdef QTI_5018_PLATFORM
		case QTI_SIP_BLOW_FUSE_SEC_DAT_ID:
		case QTI_SIP_SVC_PIL_INIT_ID:
		case QTI_SIP_SVC_PIL_AUTH_RESET_ID:
		case QTI_SIP_SVC_PIL_UNLOCK_XPU_ID:
		case QTI_SIP_SVC_PIL_WCSS_BREAK_DEBUG_ID:
		case QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_ID:
		case QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_V2_ID:
		case QTI_SIP_SVC_PIL_USERPD1_BRINGUP_ID:
		case QTI_SIP_SVC_PIL_USERPD1_TEARDOWN_ID:
#endif
			return true;
		default:
			return false;
	}
}

static bool qti_is_secure_io_access_allowed(u_register_t addr)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(qti_secure_io_allowed_regs); i++)
	{
		if ((uintptr_t)addr == qti_secure_io_allowed_regs[i])
		{
			return true;
		}
	}

  return false;
}

static uintptr_t qti_sip_mem_assign(void *handle, uint32_t smc_cc,
									u_register_t x1,
									u_register_t x2,
									u_register_t x3,
									u_register_t x4)
{
	uintptr_t dyn_map_start, dyn_map_end;
	size_t dyn_map_size;
	u_register_t x6, x7, x8;
	int ret = SMC_UNK;
	u_register_t x5 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5);

	do {
		if (SMC_32 == smc_cc) {
			x5 = (uint32_t)x5;
		}
		/* Validate input arg count & retrieve arg3-6 from NS Buffer. */
		if ((QTI_SIP_SVC_MEM_ASSIGN_PARAM_ID != x1) &&
			(0x0 == x5)) {
			break;
		}

		/* Map NS Buffer. */
		dyn_map_start = x5;
		dyn_map_size = (SMC_32 == smc_cc) ? (sizeof(uint32_t) * 4) : (sizeof(uint64_t) * 4);
		if (0 != qti_mmap_add_dynamic_region(dyn_map_start, dyn_map_start,
					dyn_map_size, (MT_NS | MT_RO | MT_EXECUTE_NEVER))) {
			break;
		}
		/* Retrieve indirect args. */
		if (SMC_32 == smc_cc) {
			x6 = *((uint32_t *)x5 + 1);
			x7 = *((uint32_t *)x5 + 2);
			x8 = *((uint32_t *)x5 + 3);
			x5 = *(uint32_t *)x5;
		} else {
			x6 = *((uint64_t *)x5 + 1);
			x7 = *((uint64_t *)x5 + 2);
			x8 = *((uint32_t *)x5 + 3);
			x5 = *(uint64_t *)x5;
		}

		/* Un-Map NS Buffer. */
		if (0 != qti_mmap_remove_dynamic_region(dyn_map_start, dyn_map_size)) {
			break;
		}

		/* Map NS Buffers.
		   arg0,2,4 points to buffers & arg1,3,5 hold sizes.
		   MAP api's fail to map if it's already mapped. Let's
		   find lowest start & highest end address, then map once. */
		dyn_map_start = MIN(x2, x4);
		dyn_map_start = MIN(dyn_map_start, x6);
		dyn_map_end = MAX((x2 + x3), (x4 + x5));
		dyn_map_end = MAX(dyn_map_end, (x6 + x7));
		dyn_map_size = dyn_map_end - dyn_map_start;

		if (0 != qti_mmap_add_dynamic_region(dyn_map_start, dyn_map_start,
					 dyn_map_size, (MT_NS | MT_RO | MT_EXECUTE_NEVER))) {
			break;
		}

		/* Invoke API lib api. */
		ret = qtiseclib_mem_assign(x2, x3, x4, x5, x6, x7, x8);

		/* Un-Map NS Buffers. */
		if (0 != qti_mmap_remove_dynamic_region(dyn_map_start, dyn_map_size)) {
			break;
		}

		if (0 == ret) {
			SMC_RET2(handle, SMC_OK, ret);
		}

	}while(0);

	SMC_RET2(handle, SMC_UNK, ret);
}

/*
 * This function does a mode switch to Aarch64 HLOS
 */
static uintptr_t qti_sip_hlos_mode_switch(void *handle, hlos_boot_params_t* hlos_boot_params)
{
	void *ns_ctx;
	ns_ctx = cm_get_context(NON_SECURE);
	/* Map the buffer passed by Lower EL*/
	qti_mmap_add_dynamic_region((uintptr_t)hlos_boot_params, (uintptr_t)hlos_boot_params, sizeof(hlos_boot_params_t), (MT_NS | MT_RO | MT_EXECUTE_NEVER));

	/* zero out all the register values in non secure context*/
	memset(ns_ctx, 0 , sizeof(qtiseclib_dbg_a64_ctxt_regs_type));
	/*switch to aarch64*/
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X0, hlos_boot_params->el1_x0);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X1, hlos_boot_params->el1_x1);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X2, hlos_boot_params->el1_x2);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X3, hlos_boot_params->el1_x3);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X4, hlos_boot_params->el1_x4);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X5, hlos_boot_params->el1_x5);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X6, hlos_boot_params->el1_x6);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X7, hlos_boot_params->el1_x7);
	write_ctx_reg(get_gpregs_ctx(ns_ctx), CTX_GPREG_X8, hlos_boot_params->el1_x8);

	write_ctx_reg(get_el3state_ctx(ns_ctx), CTX_SPSR_EL3, (SPSR_D_BIT | SPSR_I_BIT | SPSR_MODE_EL1h));
	write_ctx_reg(get_el3state_ctx(ns_ctx), CTX_ELR_EL3, hlos_boot_params->el1_elr);
	write_ctx_reg(get_el3state_ctx(ns_ctx),CTX_SCR_EL3, (SCR_FIQ_BIT | SCR_RW_BIT | SCR_NS_BIT));
	write_hcr_el2(read_hcr_el2() | HCR_RW_BIT);

	/* Unmap the buffer passed by Lower EL*/
	qti_mmap_remove_dynamic_region((uintptr_t)hlos_boot_params, sizeof(hlos_boot_params_t));

	SMC_RET0(handle);
}

/*
 * This function handles QTI specific syscalls. Currently only SiP calls are present.
 * Both FAST & YIELD type call land here.
 */
static uintptr_t qti_sip_handler(uint32_t smc_fid,
				 u_register_t x1,
				 u_register_t x2,
				 u_register_t x3,
				 u_register_t x4,
				 void *cookie, void *handle, u_register_t flags)
{
	uint32_t l_smc_fid = smc_fid & FUNCID_OEN_NUM_MASK;
	int ret;

	if (SMC_32 == GET_SMC_CC(smc_fid)) {
		x1 = (uint32_t)x1;
		x2 = (uint32_t)x2;
		x3 = (uint32_t)x3;
		x4 = (uint32_t)x4;
	}

	switch (l_smc_fid) {

#if QTI_5018_PLATFORM
	case QTI_SIP_SVC_PIL_INIT_ID:
		{
		if(QTI_SIP_SVC_PIL_INIT_PARAM_ID == x1){
			SMC_RET2(handle, SMC_OK, qtiseclib_pil_init_image_ns(x2, (void *)x3));
			}
		else
			SMC_RET1(handle, SMC_UNK);
		}

	case QTI_SIP_SVC_PIL_UNLOCK_XPU_ID:
	  {
		if(QTI_SIP_SVC_PIL_UNLOCK_XPU_PARAM_ID == x1){
			SMC_RET2(handle, SMC_OK, qtiseclib_pil_unlock_area(x2));
			}
		else
			SMC_RET1(handle, SMC_UNK);
	  }

	case QTI_SIP_SVC_PIL_AUTH_RESET_ID:
		{
		if(QTI_SIP_SVC_PIL_AUTH_RESET_PARAM_ID == x1){
			SMC_RET2(handle, SMC_OK, qtiseclib_pil_auth_reset_ns(x2));
			}
		else
			SMC_RET1(handle, SMC_UNK);
		}

	case QTI_SIP_SVC_PIL_WCSS_BREAK_DEBUG_ID:
		{
		if(QTI_SIP_SVC_PIL_WCSS_BREAK_DEBUG_PARAM_ID == x1){
			SMC_RET2(handle, SMC_OK, pil_wcss_break_start(x2));
			}
		else
			SMC_RET1(handle, SMC_UNK);
		}

	case QTI_SIP_SVC_PIL_USERPD1_BRINGUP_ID:
		{
		if(QTI_SIP_SVC_PIL_USERPD1_BRINGUP_PARAM_ID == x1){
			SMC_RET2(handle, SMC_OK, qtiseclib_pil_userpd1_bringup(x2));
			}
		else
			SMC_RET1(handle, SMC_UNK);
		}

	case QTI_SIP_SVC_PIL_USERPD1_TEARDOWN_ID:
		{
		if(QTI_SIP_SVC_PIL_USERPD1_TEARDOWN_PARAM_ID == x1){
			SMC_RET2(handle, SMC_OK, qtiseclib_pil_userpd1_teardown(x2));
			}
		else
			SMC_RET1(handle, SMC_UNK);
		}

	case QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_ID:
		{
		if(QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_PARAM_ID == x1){
			u_register_t x5 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5);
			SMC_RET2(handle, SMC_OK, qtiseclib_pil_multipd_auth_ns(x2, x3, x4, x5));
			}
		else
			SMC_RET1(handle, SMC_UNK);
		}
	case QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_V2_ID:
		{
		if(QTI_SIP_SVC_PIL_MULTIPD_MEMCPY_V2_PARAM_ID == x1){
			u_register_t x5 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5);
			SMC_RET2(handle, SMC_OK, qtiseclib_pil_multipd_auth_ns_v2(x2, x3, x4, x5));
		}
        else
			SMC_RET1(handle, SMC_UNK);
		}
#endif

	case QTI_TEST_XPU_ERR_COUNT_ID:
		{
			SMC_RET1(handle, qtiseclib_test_get_xpu_err_count());
		}
	case QTI_TEST_XPU_ERR_COUNT_CLEAR_ID:
		{
			qtiseclib_test_clear_xpu_err_count();
			SMC_RET1(handle, SMC_OK);
		}
	case QTI_TEST_STACK_PROTECTION_ID:
		{
			SMC_RET1(handle, qti_test_stack_protection());
		}

	case QTI_SIP_SVC_CALL_COUNT_ID:
		{
			SMC_RET1(handle, QTI_SIP_SVC_CALL_COUNT);
		}

	case QTI_SIP_SVC_UID_ID:
		{
			/* Return UID to the caller */
			SMC_UUID_RET(handle, qti_sip_svc_uid);
		}

	case QTI_SIP_SVC_VERSION_ID:
		{
			/* Return the version of current implementation */
			SMC_RET2(handle, QTI_SIP_SVC_VERSION_MAJOR,
					 QTI_SIP_SVC_VERSION_MINOR);
		}
	case QTI_SIP_SVC_SECURE_IO_READ_ID:
		{
			if ((QTI_SIP_SVC_SECURE_IO_READ_PARAM_ID == x1) &&
			    qti_is_secure_io_access_allowed(x2)) {
				SMC_RET2(handle, SMC_OK, *((volatile uint32_t*)x2));
			}
			SMC_RET1(handle, SMC_UNK);
		}
	case QTI_SIP_SVC_SECURE_IO_WRITE_ID:
		{
			if ((QTI_SIP_SVC_SECURE_IO_WRITE_PARAM_ID == x1) &&
			    qti_is_secure_io_access_allowed(x2)) {
				*((volatile uint32_t *)x2) = x3;
				SMC_RET2(handle, SMC_OK, SMC_OK);
			}
			SMC_RET1(handle, SMC_UNK);
		}
	case QTI_SIP_SVC_MEM_ASSIGN_ID:
		{
			return qti_sip_mem_assign(handle, GET_SMC_CC(smc_fid),
									  x1, x2, x3, x4);
		}
        case QTI_SIP_IS_SMC_AVAILABLE_ID:
                {
                        SMC_RET2(handle, SMC_OK, qti_is_smc_available(x2));
                }
	case QTI_SIP_SVC_AUTH_CHECK_ID:
		{
			if (QTI_SIP_SVC_AUTH_CHECK_PARAM_ID == x1){
		#if QTI_5018_PLATFORM
				SMC_RET2(handle, SMC_OK, qtiseclib_secure_boot_check((char *)x2, x3));
		#endif
		#if QTI_6018_PLATFORM
				*((volatile uint32_t *)x2) = 0;
				SMC_RET2(handle, SMC_OK, SMC_OK);
		#endif
				}
			SMC_RET1(handle, SMC_UNK);
		}
	case QTI_INFO_GET_DIAG_ID:
		{
			ret = qtiseclib_get_diag((char *)x2, x3);
			SMC_RET2(handle, SMC_OK, ret);
		}
	case QTI_SIP_SVC_RESET_DEBUG_ID:
		{
			if ((QTI_SIP_SVC_RESET_DEBUG_PARAM_ID == x1))
			    SMC_RET1(handle, qtiseclib_config_reset_debug(x2, x3));
			SMC_RET1(handle, SMC_UNK);
		}
        case QTI_DUMP_SET_CPU_CTX_BUF_ID:
                {
			ret = qtiseclib_set_cpu_ctx_buf(x2, x3);
                        SMC_RET2(handle, SMC_OK, ret);
                }
        case QTI_SIP_DO_HLOS_MODE_SWITCH:
                {
			return qti_sip_hlos_mode_switch(handle, (hlos_boot_params_t *)x2);
                }
	case QTI_SIP_PROTECT_MEM_SUBSYS_ID:
		{
		        if (QTI_SIP_PROTECT_MEM_SUBSYS_ID_PARAM_ID == x1)
			        SMC_RET1(handle, qtiseclib_protect_mem_subsystem(x2, x3, x4));
			SMC_RET1(handle, SMC_UNK);
		}
	case QTI_SIP_CLEAR_MEM_SUBSYS_ID:
		{
		        if (QTI_SIP_CLEAR_MEM_SUBSYS_ID_PARAM_ID == x1) {
				u_register_t x5 = read_ctx_reg(get_gpregs_ctx(handle), CTX_GPREG_X5);
			        SMC_RET1(handle, qtiseclib_clear_protect_mem_subsystem(x2, x3, x4, x5));
			}
			SMC_RET1(handle, SMC_UNK);
		}
#ifdef QTI_5018_PLATFORM
	case QTI_SIP_BLOW_FUSE_SEC_DAT_ID:
		{
			ret = qtiseclib_qfprom_fuse_secdat((uint32_t *)x2);
			SMC_RET2(handle, SMC_OK, ret);
		}
#endif
 	default:
		{
			SMC_RET1(handle, SMC_UNK);
		}
	}
	return (uintptr_t) handle;
}

/* Define a runtime service descriptor for both fast & yield SiP calls */
DECLARE_RT_SVC(qti_sip_fast_svc, OEN_SIP_START,
	       OEN_SIP_END, SMC_TYPE_FAST, NULL, qti_sip_handler);

DECLARE_RT_SVC(qti_sip_yield_svc, OEN_SIP_START,
	       OEN_SIP_END, SMC_TYPE_YIELD, NULL, qti_sip_handler);
