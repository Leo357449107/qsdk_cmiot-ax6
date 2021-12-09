/*
 * Copyright (c) 2016 ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdint.h>
#include <arch.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <common/runtime_svc.h>
#include <plat/common/platform.h>
#include <tools_share/uuid.h>
#include <oem_svc.h>
#include <qtiseclib_defs.h>
#include <qtiseclib_cb_interface.h>
#include <qtiseclib_interface.h>

int wifi_ac_irqnum;
qtiseclib_oem_cmd_buf_t *oem_cmd_buf;

/* OEM Service UUID */
DEFINE_SVC_UUID2(oem_svc_uid,
   0x2c6f5844, 0x788c, 0x11e9, 0x8f, 0x9e,
   0x2a, 0x86, 0xe4, 0x08, 0x5a, 0x59);

/* Setup OEM Services */
static int32_t oem_svc_setup(void)
{
	/*
	 * Invoke related module setup from here
	 */
	return 0;
}

static uintptr_t qti_oem_register_irq_handler(uint32_t smc_fid,
			u_register_t x1,
			u_register_t x2,
			u_register_t x3,
			u_register_t x4,
			void *cookie,
			void *handle,
			u_register_t flags)
{
	wifi_ac_irqnum = ( int ) x2;
	qtiseclib_oem_register_wifi_interrupt(wifi_ac_irqnum);
	SMC_RET2(handle,SMC_OK,SMC_OK);
}

static uintptr_t qti_oem_read_handler(uint32_t smc_fid,
			u_register_t x1,
			u_register_t x2,
			u_register_t x3,
			u_register_t x4,
			void *cookie,
			void *handle,
			u_register_t flags)
{
	int size = ( int ) x3;
	int retVal = 0;
	retVal = qtiseclib_cb_mmap_add_dynamic_region((unsigned long long)x2, (uintptr_t)x2,
						    x3 , QTISECLIB_MAP_RW_XN_NC_DATA);
	if(retVal != 0)
	{
		WARN("Map failed for Addr: 0x%x Len: 0x%x",(unsigned int)x2,size);
		SMC_RET1(handle, SMC_UNK);
	}
	oem_cmd_buf = ( qtiseclib_oem_cmd_buf_t *)x2 ;
	qtiseclib_oem_command_read(oem_cmd_buf);
	qtiseclib_cb_mmap_remove_dynamic_region((uintptr_t)x2,size);
	SMC_RET2(handle,SMC_OK,SMC_OK);
}

static uintptr_t qti_oem_write_handler(uint32_t smc_fid,
			u_register_t x1,
			u_register_t x2,
			u_register_t x3,
			u_register_t x4,
			void *cookie,
			void *handle,
			u_register_t flags)
{
	int size = ( int ) x3;
	int retVal = 0;
	retVal = qtiseclib_cb_mmap_add_dynamic_region((unsigned long long)x2, (uintptr_t)x2,
						    x3 , QTISECLIB_MAP_RW_XN_NC_DATA);
	if(retVal != 0)
	{
		WARN("Map failed for Addr: 0x%x Len: 0x%x",(unsigned int)x2,size);
		SMC_RET1(handle, SMC_UNK);
	}
	oem_cmd_buf = ( qtiseclib_oem_cmd_buf_t *)x2 ;
	qtiseclib_oem_command_write(oem_cmd_buf);
	qtiseclib_cb_mmap_remove_dynamic_region((uintptr_t)x2,size);
	SMC_RET2(handle,SMC_OK,SMC_OK);
}

/*
 * Top-level OEM Service SMC handler. This handler will in turn dispatch
 * calls to related SMC handler
 */
static uintptr_t qti_oem_svc_smc_handler(uint32_t smc_fid,
			 u_register_t x1,
			 u_register_t x2,
			 u_register_t x3,
			 u_register_t x4,
			 void *cookie,
			 void *handle,
			 u_register_t flags)
{
	/*
	 * Dispatch OEM calls to OEM Common handler and return its return value
	 */
	switch (smc_fid) {
	case OEM_SVC_CALL_COUNT:
		/*
		 * Return the number of OEM Service Calls.
		 */
		SMC_RET1(handle, OEM_SVC_NUM_CALLS);

	case OEM_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, oem_svc_uid);

	case OEM_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, OEM_VERSION_MAJOR, OEM_VERSION_MINOR);

	case OEM_READ_BUFFER:
		/* read buffer from HLOS */
		return qti_oem_read_handler(smc_fid, x1, x2, x3, x4, cookie,
					handle, flags);

	case OEM_WRITE_BUFFER:
		/* read buffer from HLOS */
		return qti_oem_write_handler(smc_fid, x1, x2, x3, x4, cookie,
					handle, flags);

	case OEM_REGISTER_WIFIAC_IRQ:
		/* register WiFi AC irq */
		return qti_oem_register_irq_handler(smc_fid, x1, x2, x3, x4, cookie,
					handle, flags);

	default:
		WARN("Unimplemented OEM Service Call: 0x%x\n", smc_fid);
		SMC_RET1(handle, SMC_UNK);
	}
}

/* Register OEM Service Calls as runtime service */
DECLARE_RT_SVC(
		oem_svc,
		OEN_OEM_START,
		OEN_OEM_END,
		SMC_TYPE_YIELD,
		oem_svc_setup,
		qti_oem_svc_smc_handler
);
