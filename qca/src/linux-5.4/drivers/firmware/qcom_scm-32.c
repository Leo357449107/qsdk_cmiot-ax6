// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2010,2015, The Linux Foundation. All rights reserved.
 * Copyright (C) 2015 Linaro Ltd.
 */

#include <linux/slab.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/qcom_scm.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

#include "qcom_scm.h"

#define SCM_SVC_ID_SHIFT		0xA

#define QCOM_SCM_FLAG_COLDBOOT_CPU0	0x00
#define QCOM_SCM_FLAG_COLDBOOT_CPU1	0x01
#define QCOM_SCM_FLAG_COLDBOOT_CPU2	0x08
#define QCOM_SCM_FLAG_COLDBOOT_CPU3	0x20

#define QCOM_SCM_FLAG_WARMBOOT_CPU0	0x04
#define QCOM_SCM_FLAG_WARMBOOT_CPU1	0x02
#define QCOM_SCM_FLAG_WARMBOOT_CPU2	0x10
#define QCOM_SCM_FLAG_WARMBOOT_CPU3	0x40

#define N_EXT_SCM_ARGS		7
#define FIRST_EXT_ARG_IDX	3
#define N_REGISTER_ARGS		(MAX_QCOM_SCM_ARGS - N_EXT_SCM_ARGS + 1)

struct qcom_scm_entry {
	int flag;
	void *entry;
};

static struct qcom_scm_entry qcom_scm_wb[] = {
	{ .flag = QCOM_SCM_FLAG_WARMBOOT_CPU0 },
	{ .flag = QCOM_SCM_FLAG_WARMBOOT_CPU1 },
	{ .flag = QCOM_SCM_FLAG_WARMBOOT_CPU2 },
	{ .flag = QCOM_SCM_FLAG_WARMBOOT_CPU3 },
};

static DEFINE_MUTEX(qcom_scm_lock);

/**
 * struct qcom_scm_command - one SCM command buffer
 * @len: total available memory for command and response
 * @buf_offset: start of command buffer
 * @resp_hdr_offset: start of response buffer
 * @id: command to be executed
 * @buf: buffer returned from qcom_scm_get_command_buffer()
 *
 * An SCM command is laid out in memory as follows:
 *
 *	------------------- <--- struct qcom_scm_command
 *	| command header  |
 *	------------------- <--- qcom_scm_get_command_buffer()
 *	| command buffer  |
 *	------------------- <--- struct qcom_scm_response and
 *	| response header |      qcom_scm_command_to_response()
 *	------------------- <--- qcom_scm_get_response_buffer()
 *	| response buffer |
 *	-------------------
 *
 * There can be arbitrary padding between the headers and buffers so
 * you should always use the appropriate qcom_scm_get_*_buffer() routines
 * to access the buffers in a safe manner.
 */
struct qcom_scm_command {
	__le32 len;
	__le32 buf_offset;
	__le32 resp_hdr_offset;
	__le32 id;
	__le32 buf[0];
};

/**
 * struct qcom_scm_response - one SCM response buffer
 * @len: total available memory for response
 * @buf_offset: start of response data relative to start of qcom_scm_response
 * @is_complete: indicates if the command has finished processing
 */
struct qcom_scm_response {
	__le32 len;
	__le32 buf_offset;
	__le32 is_complete;
};

/**
 * qcom_scm_command_to_response() - Get a pointer to a qcom_scm_response
 * @cmd: command
 *
 * Returns a pointer to a response for a command.
 */
static inline struct qcom_scm_response *qcom_scm_command_to_response(
		const struct qcom_scm_command *cmd)
{
	return (void *)cmd + le32_to_cpu(cmd->resp_hdr_offset);
}

/**
 * qcom_scm_get_command_buffer() - Get a pointer to a command buffer
 * @cmd: command
 *
 * Returns a pointer to the command buffer of a command.
 */
static inline void *qcom_scm_get_command_buffer(const struct qcom_scm_command *cmd)
{
	return (void *)cmd->buf;
}

/**
 * qcom_scm_get_response_buffer() - Get a pointer to a response buffer
 * @rsp: response
 *
 * Returns a pointer to a response buffer of a response.
 */
static inline void *qcom_scm_get_response_buffer(const struct qcom_scm_response *rsp)
{
	return (void *)rsp + le32_to_cpu(rsp->buf_offset);
}

static u32 smc(u32 cmd_addr)
{
	int context_id;
	register u32 r0 asm("r0") = 1;
	register u32 r1 asm("r1") = (u32)&context_id;
	register u32 r2 asm("r2") = cmd_addr;
	do {
		asm volatile(
			__asmeq("%0", "r0")
			__asmeq("%1", "r0")
			__asmeq("%2", "r1")
			__asmeq("%3", "r2")
#ifdef REQUIRES_SEC
			".arch_extension sec\n"
#endif
			"smc	#0	@ switch to secure world\n"
			: "=r" (r0)
			: "r" (r0), "r" (r1), "r" (r2)
			: "r3", "r12");
	} while (r0 == QCOM_SCM_INTERRUPTED);

	return r0;
}

/**
 * qcom_scm_call() - Send an SCM command
 * @dev: struct device
 * @svc_id: service identifier
 * @cmd_id: command identifier
 * @cmd_buf: command buffer
 * @cmd_len: length of the command buffer
 * @resp_buf: response buffer
 * @resp_len: length of the response buffer
 *
 * Sends a command to the SCM and waits for the command to finish processing.
 *
 * A note on cache maintenance:
 * Note that any buffers that are expected to be accessed by the secure world
 * must be flushed before invoking qcom_scm_call and invalidated in the cache
 * immediately after qcom_scm_call returns. Cache maintenance on the command
 * and response buffers is taken care of by qcom_scm_call; however, callers are
 * responsible for any other cached buffers passed over to the secure world.
 */
static int qcom_scm_call(struct device *dev, u32 svc_id, u32 cmd_id,
			 const void *cmd_buf, size_t cmd_len, void *resp_buf,
			 size_t resp_len)
{
	int ret;
	struct qcom_scm_command *cmd;
	struct qcom_scm_response *rsp;
	size_t alloc_len = sizeof(*cmd) + cmd_len + sizeof(*rsp) + resp_len;
	dma_addr_t cmd_phys;

	cmd = kzalloc(PAGE_ALIGN(alloc_len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd->len = cpu_to_le32(alloc_len);
	cmd->buf_offset = cpu_to_le32(sizeof(*cmd));
	cmd->resp_hdr_offset = cpu_to_le32(sizeof(*cmd) + cmd_len);

	cmd->id = cpu_to_le32((svc_id << SCM_SVC_ID_SHIFT) | cmd_id);
	if (cmd_buf)
		memcpy(qcom_scm_get_command_buffer(cmd), cmd_buf, cmd_len);

	rsp = qcom_scm_command_to_response(cmd);

	cmd_phys = dma_map_single(dev, cmd, alloc_len, DMA_TO_DEVICE);
	if (dma_mapping_error(dev, cmd_phys)) {
		kfree(cmd);
		return -ENOMEM;
	}

	mutex_lock(&qcom_scm_lock);
	ret = smc(cmd_phys);
	if (ret < 0)
		ret = qcom_scm_remap_error(ret);
	mutex_unlock(&qcom_scm_lock);
	if (ret)
		goto out;

	do {
		dma_sync_single_for_cpu(dev, cmd_phys + sizeof(*cmd) + cmd_len,
					sizeof(*rsp), DMA_FROM_DEVICE);
	} while (!rsp->is_complete);

	if (resp_buf) {
		dma_sync_single_for_cpu(dev, cmd_phys + sizeof(*cmd) + cmd_len +
					le32_to_cpu(rsp->buf_offset),
					resp_len, DMA_FROM_DEVICE);
		memcpy(resp_buf, qcom_scm_get_response_buffer(rsp),
		       resp_len);
	}
out:
	dma_unmap_single(dev, cmd_phys, alloc_len, DMA_TO_DEVICE);
	kfree(cmd);
	return ret;
}

#define SCM_CLASS_REGISTER	(0x2 << 8)
#define SCM_MASK_IRQS		BIT(5)
#define SCM_ATOMIC(svc, cmd, n) (((((svc) << 10)|((cmd) & 0x3ff)) << 12) | \
				SCM_CLASS_REGISTER | \
				SCM_MASK_IRQS | \
				(n & 0xf))

/**
 * qcom_scm_call_atomic1() - Send an atomic SCM command with one argument
 * @svc_id: service identifier
 * @cmd_id: command identifier
 * @arg1: first argument
 *
 * This shall only be used with commands that are guaranteed to be
 * uninterruptable, atomic and SMP safe.
 */
static s32 qcom_scm_call_atomic1(u32 svc, u32 cmd, u32 arg1)
{
	int context_id;

	register u32 r0 asm("r0") = SCM_ATOMIC(svc, cmd, 1);
	register u32 r1 asm("r1") = (u32)&context_id;
	register u32 r2 asm("r2") = arg1;

	asm volatile(
			__asmeq("%0", "r0")
			__asmeq("%1", "r0")
			__asmeq("%2", "r1")
			__asmeq("%3", "r2")
#ifdef REQUIRES_SEC
			".arch_extension sec\n"
#endif
			"smc    #0      @ switch to secure world\n"
			: "=r" (r0)
			: "r" (r0), "r" (r1), "r" (r2)
			: "r3", "r12");
	return r0;
}

/**
 * qcom_scm_call_atomic2() - Send an atomic SCM command with two arguments
 * @svc_id:	service identifier
 * @cmd_id:	command identifier
 * @arg1:	first argument
 * @arg2:	second argument
 *
 * This shall only be used with commands that are guaranteed to be
 * uninterruptable, atomic and SMP safe.
 */
static s32 qcom_scm_call_atomic2(u32 svc, u32 cmd, u32 arg1, u32 arg2)
{
	int context_id;

	register u32 r0 asm("r0") = SCM_ATOMIC(svc, cmd, 2);
	register u32 r1 asm("r1") = (u32)&context_id;
	register u32 r2 asm("r2") = arg1;
	register u32 r3 asm("r3") = arg2;

	asm volatile(
			__asmeq("%0", "r0")
			__asmeq("%1", "r0")
			__asmeq("%2", "r1")
			__asmeq("%3", "r2")
			__asmeq("%4", "r3")
#ifdef REQUIRES_SEC
			".arch_extension sec\n"
#endif
			"smc    #0      @ switch to secure world\n"
			: "=r" (r0)
			: "r" (r0), "r" (r1), "r" (r2), "r" (r3)
			: "r12");
	return r0;
}

#define R0_STR "r0"
#define R1_STR "r1"
#define R2_STR "r2"
#define R3_STR "r3"
#define R4_STR "r4"
#define R5_STR "r5"
#define R6_STR "r6"

static int __scm_call_armv8_32(u32 w0, u32 w1, u32 w2, u32 w3, u32 w4, u32 w5,
				u64 *ret1, u64 *ret2, u64 *ret3)
{
	register u32 r0 asm("r0") = w0;
	register u32 r1 asm("r1") = w1;
	register u32 r2 asm("r2") = w2;
	register u32 r3 asm("r3") = w3;
	register u32 r4 asm("r4") = w4;
	register u32 r5 asm("r5") = w5;
	register u32 r6 asm("r6") = 0;

	do {
		asm volatile(
			__asmeq("%0", R0_STR)
			__asmeq("%1", R1_STR)
			__asmeq("%2", R2_STR)
			__asmeq("%3", R3_STR)
			__asmeq("%4", R0_STR)
			__asmeq("%5", R1_STR)
			__asmeq("%6", R2_STR)
			__asmeq("%7", R3_STR)
			__asmeq("%8", R4_STR)
			__asmeq("%9", R5_STR)
			__asmeq("%10", R6_STR)
#ifdef REQUIRES_SEC
			".arch_extension sec\n"
#endif
			"smc	#0\n"
			: "=r" (r0), "=r" (r1), "=r" (r2), "=r" (r3)
			: "r" (r0), "r" (r1), "r" (r2), "r" (r3), "r" (r4),
			 "r" (r5), "r" (r6));

	} while (r0 == QCOM_SCM_INTERRUPTED);

	if (ret1)
		*ret1 = r1;
	if (ret2)
		*ret2 = r2;
	if (ret3)
		*ret3 = r3;

	return r0;
}

static enum scm_interface_version {
	SCM_UNKNOWN,
	SCM_LEGACY,
	SCM_ARMV8_32,
} scm_version = SCM_UNKNOWN;

/* This function is used to find whether TZ is in AARCH64 mode.
 * If this function returns 1, then its in AARCH64 mode and
 * calling conventions for AARCH64 TZ is different, we need to
 * use them.
 */
bool is_scm_armv8(void)
{
	int ret;
	u64 ret1, x0;

	if (likely(scm_version != SCM_UNKNOWN))
		return (scm_version == SCM_ARMV8_32);

	/* Try SMC32 call */
	ret1 = 0;
	x0 = SCM_SIP_FNID(QCOM_SCM_SVC_INFO, QCOM_IS_CALL_AVAIL_CMD) |
			QTI_SMC_ATOMIC_MASK;

	ret = __scm_call_armv8_32(x0, SCM_ARGS(1), x0, 0, 0, 0,
				  &ret1, NULL, NULL);
	if (ret || !ret1)
		scm_version = SCM_LEGACY;
	else
		scm_version = SCM_ARMV8_32;

	pr_debug("scm_call: scm version is %x\n", scm_version);

	return (scm_version == SCM_ARMV8_32);
}

/**
 * qti_scm_call2() - Invoke a syscall in the secure world
 * @dev: struct device
 * @fn_id: The function ID for this syscall
 * @desc: Descriptor structure containing arguments and return values
 *
 * Sends a command to the SCM and waits for the command to finish processing.
 * This should *only* be called in pre-emptible context.
 *
 */
static int qti_scm_call2(struct device *dev, u32 fn_id, struct scm_desc *desc)
{
	int arglen = desc->arginfo & 0xf;
	int ret = 0;
	int retry_count = 0;
	int i = 0;
	u64 x0 = fn_id;
	dma_addr_t args_phy = 0;
	u32 *args_virt = NULL;
	size_t alloc_len = 0;

	desc->x5 = desc->args[FIRST_EXT_ARG_IDX];

	if (unlikely(!is_scm_armv8()))
		return -ENODEV;

	if (unlikely(arglen > N_REGISTER_ARGS)) {
		alloc_len = N_EXT_SCM_ARGS * sizeof(u32);
		args_virt = kzalloc(PAGE_ALIGN(alloc_len), GFP_KERNEL);

		if (!args_virt)
			return -ENOMEM;

		desc->extra_arg_buf = args_virt;

		for (i = 0; i < N_EXT_SCM_ARGS; i++)
			args_virt[i] = cpu_to_le32(desc->args[i +
						      FIRST_EXT_ARG_IDX]);

		args_phy = dma_map_single(dev, args_virt, alloc_len,
					   DMA_TO_DEVICE);

		if (dma_mapping_error(dev, args_phy)) {
			kfree(args_virt);
			return -ENOMEM;
		}

		desc->x5 = args_phy;
	}

	do {
		mutex_lock(&qcom_scm_lock);

		desc->ret[0] = desc->ret[1] = desc->ret[2] = 0;

		ret = __scm_call_armv8_32(x0, desc->arginfo,
					  desc->args[0], desc->args[1],
					  desc->args[2], desc->x5,
					  &desc->ret[0], &desc->ret[1],
					  &desc->ret[2]);
		mutex_unlock(&qcom_scm_lock);

		if (ret == QCOM_SCM_V2_EBUSY)
			msleep(QCOM_SCM_EBUSY_WAIT_MS);
	}  while (ret == QCOM_SCM_V2_EBUSY &&
			(retry_count++ < QCOM_SCM_EBUSY_MAX_RETRY));

	if (args_virt) {
		dma_unmap_single(dev, args_phy, alloc_len, DMA_TO_DEVICE);
		kfree(args_virt);
	}

	if (ret < 0)
		pr_err("scm_call failed: func id %#llx ret: %d syscall returns: %#llx, %#llx, %#llx\n",
			x0, ret, desc->ret[0], desc->ret[1], desc->ret[2]);

	if (ret < 0)
		return qcom_scm_remap_error(ret);

	return 0;
}

u32 qcom_scm_get_version(void)
{
	int context_id;
	static u32 version = -1;
	register u32 r0 asm("r0");
	register u32 r1 asm("r1");

	if (version != -1)
		return version;

	mutex_lock(&qcom_scm_lock);

	r0 = 0x1 << 8;
	r1 = (u32)&context_id;
	do {
		asm volatile(
			__asmeq("%0", "r0")
			__asmeq("%1", "r1")
			__asmeq("%2", "r0")
			__asmeq("%3", "r1")
#ifdef REQUIRES_SEC
			".arch_extension sec\n"
#endif
			"smc	#0	@ switch to secure world\n"
			: "=r" (r0), "=r" (r1)
			: "r" (r0), "r" (r1)
			: "r2", "r3", "r12");
	} while (r0 == QCOM_SCM_INTERRUPTED);

	version = r1;
	mutex_unlock(&qcom_scm_lock);

	return version;
}
EXPORT_SYMBOL(qcom_scm_get_version);

/**
 * qcom_scm_set_cold_boot_addr() - Set the cold boot address for cpus
 * @entry: Entry point function for the cpus
 * @cpus: The cpumask of cpus that will use the entry point
 *
 * Set the cold boot address of the cpus. Any cpu outside the supported
 * range would be removed from the cpu present mask.
 */
int __qcom_scm_set_cold_boot_addr(void *entry, const cpumask_t *cpus)
{
	int flags = 0;
	int cpu;
	int scm_cb_flags[] = {
		QCOM_SCM_FLAG_COLDBOOT_CPU0,
		QCOM_SCM_FLAG_COLDBOOT_CPU1,
		QCOM_SCM_FLAG_COLDBOOT_CPU2,
		QCOM_SCM_FLAG_COLDBOOT_CPU3,
	};

	if (!cpus || (cpus && cpumask_empty(cpus)))
		return -EINVAL;

	for_each_cpu(cpu, cpus) {
		if (cpu < ARRAY_SIZE(scm_cb_flags))
			flags |= scm_cb_flags[cpu];
		else
			set_cpu_present(cpu, false);
	}

	return qcom_scm_call_atomic2(QCOM_SCM_SVC_BOOT, QCOM_SCM_BOOT_ADDR,
				    flags, virt_to_phys(entry));
}

/**
 * qcom_scm_set_warm_boot_addr() - Set the warm boot address for cpus
 * @entry: Entry point function for the cpus
 * @cpus: The cpumask of cpus that will use the entry point
 *
 * Set the Linux entry point for the SCM to transfer control to when coming
 * out of a power down. CPU power down may be executed on cpuidle or hotplug.
 */
int __qcom_scm_set_warm_boot_addr(struct device *dev, void *entry,
				  const cpumask_t *cpus)
{
	int ret;
	int flags = 0;
	int cpu;
	struct {
		__le32 flags;
		__le32 addr;
	} cmd;

	/*
	 * Reassign only if we are switching from hotplug entry point
	 * to cpuidle entry point or vice versa.
	 */
	for_each_cpu(cpu, cpus) {
		if (entry == qcom_scm_wb[cpu].entry)
			continue;
		flags |= qcom_scm_wb[cpu].flag;
	}

	/* No change in entry function */
	if (!flags)
		return 0;

	cmd.addr = cpu_to_le32(virt_to_phys(entry));
	cmd.flags = cpu_to_le32(flags);
	ret = qcom_scm_call(dev, QCOM_SCM_SVC_BOOT, QCOM_SCM_BOOT_ADDR,
			    &cmd, sizeof(cmd), NULL, 0);
	if (!ret) {
		for_each_cpu(cpu, cpus)
			qcom_scm_wb[cpu].entry = entry;
	}

	return ret;
}

/**
 * qcom_scm_cpu_power_down() - Power down the cpu
 * @flags - Flags to flush cache
 *
 * This is an end point to power down cpu. If there was a pending interrupt,
 * the control would return from this function, otherwise, the cpu jumps to the
 * warm boot entry point set for this cpu upon reset.
 */
void __qcom_scm_cpu_power_down(u32 flags)
{
	qcom_scm_call_atomic1(QCOM_SCM_SVC_BOOT, QCOM_SCM_CMD_TERMINATE_PC,
			flags & QCOM_SCM_FLUSH_FLAG_MASK);
}

int __qti_scm_qseecom_notify(struct device *dev,
			     struct qsee_notify_app *request, size_t req_size,
			     struct qseecom_command_scm_resp *response,
			     size_t resp_size)
{
	int ret = 0;
	uint32_t smc_id = 0;
	struct scm_desc desc = {0};

	if (!is_scm_armv8())
		return -ENOTSUPP;

	smc_id = QTI_SYSCALL_CREATE_SMC_ID(QTI_OWNER_QSEE_OS,
					 QTI_SVC_APP_MGR,
					 QTI_CMD_NOTIFY_REGION_ID);

	desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = request->applications_region_addr;
	desc.args[1] = request->applications_region_size;

	ret = qti_scm_call2(dev, smc_id, &desc);

	response->result = desc.ret[0];
	response->resp_type = desc.ret[1];
	response->data = desc.ret[2];

	return ret;
}

int __qti_scm_qseecom_load(struct device *dev, uint32_t smc_id,
			   uint32_t cmd_id, union qseecom_load_ireq *request,
			   size_t req_size,
			   struct qseecom_command_scm_resp *response,
			   size_t resp_size)
{
	int ret = 0;
	struct scm_desc desc = {0};

	if (!is_scm_armv8())
		return -ENOTSUPP;

	desc.arginfo = SCM_ARGS(3, QCOM_SCM_VAL, QCOM_SCM_VAL,
					QCOM_SCM_VAL);
	desc.args[0] = request->load_lib_req.mdt_len;
	desc.args[1] = request->load_lib_req.img_len;
	desc.args[2] = request->load_lib_req.phy_addr;

	ret = qti_scm_call2(dev, smc_id, &desc);

	response->result = desc.ret[0];
	response->resp_type = desc.ret[1];
	response->data = desc.ret[2];

	return ret;
}

int __qti_scm_qseecom_send_data(struct device *dev,
				union qseecom_client_send_data_ireq *request,
				size_t req_size,
				struct qseecom_command_scm_resp *response,
				size_t resp_size)
{
	int ret = 0;
	uint32_t smc_id = 0;
	struct scm_desc desc = {0};

	if (!is_scm_armv8())
		return -ENOTSUPP;

	smc_id = QTI_SYSCALL_CREATE_SMC_ID(QTI_OWNER_TZ_APPS,
					 QTI_SVC_APP_ID_PLACEHOLDER,
					 QTI_CMD_SEND_DATA_ID);

	desc.arginfo = SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_RW, QCOM_SCM_VAL,
			       QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = request->v1.app_id;
	desc.args[1] = request->v1.req_ptr;
	desc.args[2] = request->v1.req_len;
	desc.args[3] = request->v1.rsp_ptr;
	desc.args[4] = request->v1.rsp_len;

	ret = qti_scm_call2(dev, smc_id, &desc);

	response->result = desc.ret[0];
	response->resp_type = desc.ret[1];
	response->data = desc.ret[2];

	return ret;
}

int __qti_scm_qseecom_unload(struct device *dev,
			     uint32_t smc_id, uint32_t cmd_id,
			     struct qseecom_unload_ireq *request,
			     size_t req_size,
			     struct qseecom_command_scm_resp *response,
			     size_t resp_size)
{
	int ret = 0;
	struct scm_desc desc = {0};

	if (!is_scm_armv8())
		return -ENOTSUPP;

	if (cmd_id == QSEOS_APP_SHUTDOWN_COMMAND) {
		desc.arginfo = SCM_ARGS(1);
		desc.args[0] = request->app_id;
	}

	ret = qti_scm_call2(dev, smc_id, &desc);

	response->result = desc.ret[0];
	response->resp_type = desc.ret[1];
	response->data = desc.ret[2];

	return ret;
}

int __qti_scm_register_log_buf(struct device *dev,
				  struct qsee_reg_log_buf_req *request,
				  size_t req_size,
				  struct qseecom_command_scm_resp *response,
				  size_t resp_size)
{
	int ret = 0;
	uint32_t smc_id = 0;
	struct scm_desc desc = {0};

	if (!is_scm_armv8())
		return -ENOTSUPP;

	smc_id = QTI_SYSCALL_CREATE_SMC_ID(QTI_OWNER_QSEE_OS,
					 QTI_SVC_APP_MGR,
					 QTI_CMD_REGISTER_LOG_BUF);

	desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = request->phy_addr;
	desc.args[1] = request->len;

	ret = qti_scm_call2(dev, smc_id, &desc);

	response->result = desc.ret[0];
	response->resp_type = desc.ret[1];
	response->data = desc.ret[2];

	return ret;
}

int __qti_scm_tls_hardening(struct device *dev, uint32_t req_addr,
			    uint32_t req_size, uint32_t resp_addr,
			    uint32_t resp_size, u32 cmd_id)
{
	int ret = 0;
	__le32 scm_ret;
	struct scm_desc desc = {0};

	if (!is_scm_armv8())
		return -ENOTSUPP;

	desc.arginfo = SCM_ARGS(4, QCOM_SCM_RW, QCOM_SCM_VAL,
				QCOM_SCM_RW, QCOM_SCM_VAL);
	desc.args[0] = (u64)req_addr;
	desc.args[1] = req_size;
	desc.args[2] = (u64)resp_addr;
	desc.args[3] = resp_size;

	ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SVC_CRYPTO, cmd_id), &desc);
	scm_ret = desc.ret[0];
	if (!ret)
		return le32_to_cpu(scm_ret);

	return ret;
}

int __qti_scm_aes(struct device *dev, uint32_t req_addr, uint32_t req_size,
		  uint32_t resp_addr, uint32_t resp_size, u32 cmd_id)
{
	int ret = 0;
	__le32 scm_ret;
	struct scm_desc desc = {0};

	if (!is_scm_armv8())
		return -ENOTSUPP;

	desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);

	desc.args[0] = (u64)req_addr;
	desc.args[1] = req_size;

	ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SVC_CRYPTO, cmd_id), &desc);
	scm_ret = desc.ret[0];
	if (!ret)
		return le32_to_cpu(scm_ret);

	return ret;
}

int __qcom_scm_is_call_available(struct device *dev, u32 svc_id, u32 cmd_id)
{
	int ret;

	if (!is_scm_armv8()) {
		__le32 svc_cmd = cpu_to_le32((svc_id << SCM_SVC_ID_SHIFT) |
								cmd_id);
		__le32 ret_val = 0;

		ret = qcom_scm_call(dev, QCOM_SCM_SVC_INFO,
				QCOM_IS_CALL_AVAIL_CMD, &svc_cmd,
				sizeof(svc_cmd), &ret_val, sizeof(ret_val));

		if (!ret)
			return le32_to_cpu(ret_val);
	} else {
		__le32 scm_ret;
		struct scm_desc desc = {0};

		desc.args[0] = SCM_SIP_FNID(svc_id, cmd_id);
		desc.arginfo = SCM_ARGS(1);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_INFO,
					QCOM_IS_CALL_AVAIL_CMD), &desc);
		scm_ret = desc.ret[0];

		if (!ret)
			return le32_to_cpu(scm_ret);
	}

	return ret;
}

int __qcom_scm_hdcp_req(struct device *dev, struct qcom_scm_hdcp_req *req,
			u32 req_cnt, u32 *resp)
{
	if (req_cnt > QCOM_SCM_HDCP_MAX_REQ_CNT)
		return -ERANGE;

	return qcom_scm_call(dev, QCOM_SCM_SVC_HDCP, QCOM_SCM_CMD_HDCP,
		req, req_cnt * sizeof(*req), resp, sizeof(*resp));
}

void __qcom_scm_init(void)
{
}

bool __qcom_scm_pas_supported(struct device *dev, u32 peripheral)
{
	__le32 out;
	__le32 in;
	int ret;

	in = cpu_to_le32(peripheral);
	ret = qcom_scm_call(dev, QCOM_SCM_SVC_PIL,
			    QCOM_SCM_PAS_IS_SUPPORTED_CMD,
			    &in, sizeof(in),
			    &out, sizeof(out));

	return ret ? false : !!out;
}

int __qcom_scm_pas_init_image(struct device *dev, u32 peripheral,
			      dma_addr_t metadata_phys)
{
	__le32 scm_ret;
	int ret;
	struct {
		__le32 proc;
		__le32 image_addr;
	} request;
	struct scm_desc desc = {0};

	if (!is_scm_armv8()) {
		request.proc = cpu_to_le32(peripheral);
		request.image_addr = cpu_to_le32(metadata_phys);

		ret = qcom_scm_call(dev, QCOM_SCM_SVC_PIL,
				    QCOM_SCM_PAS_INIT_IMAGE_CMD,
				    &request, sizeof(request),
					&scm_ret, sizeof(scm_ret));
	} else {
		desc.args[0] = peripheral;
		desc.args[1] = metadata_phys;
		desc.arginfo = SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_RW);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_PIL,
					QCOM_SCM_PAS_INIT_IMAGE_CMD), &desc);
		scm_ret = desc.ret[0];
	}

	return ret ? : le32_to_cpu(scm_ret);
}

int __qcom_scm_pas_mem_setup(struct device *dev, u32 peripheral,
			     phys_addr_t addr, phys_addr_t size)
{
	__le32 scm_ret;
	int ret;
	struct scm_desc desc = {0};
	struct {
		__le32 proc;
		__le32 addr;
		__le32 len;
	} request;

	if (!is_scm_armv8()) {
		request.proc = cpu_to_le32(peripheral);
		request.addr = cpu_to_le32(addr);
		request.len = cpu_to_le32(size);

		ret = qcom_scm_call(dev, QCOM_SCM_SVC_PIL,
				QCOM_SCM_PAS_MEM_SETUP_CMD,
				&request, sizeof(request),
				&scm_ret, sizeof(scm_ret));
	} else {
		desc.args[0] = peripheral;
		desc.args[1] = addr;
		desc.args[2] = size;
		desc.arginfo = SCM_ARGS(3, QCOM_SCM_VAL, QCOM_SCM_VAL,
					QCOM_SCM_VAL);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_PIL,
					QCOM_SCM_PAS_MEM_SETUP_CMD), &desc);
		scm_ret = desc.ret[0];
	}

	return ret ? : le32_to_cpu(scm_ret);
}

int __qcom_scm_pas_auth_and_reset(struct device *dev, u32 peripheral,
				u32 debug, u32 reset_cmd_id)
{
	__le32 out;
	__le32 in;
	int ret;
	int break_support = 0;
	struct scm_desc desc = {0};

	if (debug) {
		ret = __qcom_scm_is_call_available(dev, QCOM_SCM_SVC_PIL,
								reset_cmd_id);
		if (ret)
			break_support = 1;
		else
			dev_err(dev, "break at debug not supported\n");
	}

	if (!is_scm_armv8()) {
		if (break_support) {
			in = cpu_to_le32(debug);
			ret = qcom_scm_call(dev, QCOM_SCM_SVC_PIL,
						reset_cmd_id,
						&in, sizeof(in),
						&out, sizeof(out));
			if (ret || le32_to_cpu(out))
				goto end;
		}
		in = cpu_to_le32(peripheral);
		ret = qcom_scm_call(dev, QCOM_SCM_SVC_PIL,
					QCOM_SCM_PAS_AUTH_AND_RESET_CMD,
					&in, sizeof(in),
					&out, sizeof(out));
	} else {
		if (break_support) {
			desc.args[0] = debug;
			desc.arginfo = SCM_ARGS(1);
			ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_PIL,
						reset_cmd_id), &desc);
			out = desc.ret[0];
			if (ret || le32_to_cpu(out))
				goto end;
		}
		desc.args[0] = peripheral;
		desc.arginfo = SCM_ARGS(1);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_PIL,
					QCOM_SCM_PAS_AUTH_AND_RESET_CMD),
					&desc);
		out = desc.ret[0];
	}

end:
	return ret ? : le32_to_cpu(out);
}

int __qcom_scm_pas_shutdown(struct device *dev, u32 peripheral)
{
	__le32 out;
	__le32 in;
	int ret;
	struct scm_desc desc = {0};

	in = cpu_to_le32(peripheral);
	if (!is_scm_armv8()) {
		ret = qcom_scm_call(dev, QCOM_SCM_SVC_PIL,
				    QCOM_SCM_PAS_SHUTDOWN_CMD,
				    &in, sizeof(in),
				    &out, sizeof(out));
	} else {
		desc.args[0] = peripheral;
		desc.arginfo = SCM_ARGS(1);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_PIL,
				     QCOM_SCM_PAS_SHUTDOWN_CMD), &desc);
		out = desc.ret[0];
	}

	return ret ? : le32_to_cpu(out);
}

int __qcom_scm_pas_mss_reset(struct device *dev, bool reset)
{
	__le32 out;
	__le32 in = cpu_to_le32(reset);
	int ret;

	ret = qcom_scm_call(dev, QCOM_SCM_SVC_PIL, QCOM_SCM_PAS_MSS_RESET,
			&in, sizeof(in),
			&out, sizeof(out));

	return ret ? : le32_to_cpu(out);
}

int __qcom_scm_set_dload_mode(struct device *dev, bool enable)
{
	return qcom_scm_call_atomic2(QCOM_SCM_SVC_BOOT, QCOM_SCM_SET_DLOAD_MODE,
				     enable ? QCOM_SCM_SET_DLOAD_MODE : 0, 0);
}

int __qcom_scm_set_remote_state(struct device *dev, u32 state, u32 id)
{
	struct {
		__le32 state;
		__le32 id;
	} req;
	__le32 scm_ret = 0;
	int ret;

	req.state = cpu_to_le32(state);
	req.id = cpu_to_le32(id);

	ret = qcom_scm_call(dev, QCOM_SCM_SVC_BOOT, QCOM_SCM_SET_REMOTE_STATE,
			    &req, sizeof(req), &scm_ret, sizeof(scm_ret));

	return ret ? : le32_to_cpu(scm_ret);
}

int __qcom_scm_assign_mem(struct device *dev, phys_addr_t mem_region,
			  size_t mem_sz, phys_addr_t src, size_t src_sz,
			  phys_addr_t dest, size_t dest_sz)
{
	return -ENODEV;
}

int __qcom_scm_restore_sec_cfg(struct device *dev, u32 device_id,
			       u32 spare)
{
	return -ENODEV;
}

int __qcom_scm_iommu_secure_ptbl_size(struct device *dev, u32 spare,
				      size_t *size)
{
	return -ENODEV;
}

int __qcom_scm_iommu_secure_ptbl_init(struct device *dev, u64 addr, u32 size,
				      u32 spare)
{
	return -ENODEV;
}

int __qcom_scm_io_readl(struct device *dev, phys_addr_t addr,
			unsigned int *val)
{
	int ret;

	ret = qcom_scm_call_atomic1(QCOM_SCM_SVC_IO, QCOM_SCM_IO_READ, addr);
	if (ret >= 0)
		*val = ret;

	return ret < 0 ? ret : 0;
}

int __qcom_scm_io_writel(struct device *dev, phys_addr_t addr, unsigned int val)
{
	return qcom_scm_call_atomic2(QCOM_SCM_SVC_IO, QCOM_SCM_IO_WRITE,
				     addr, val);
}

int __qti_qfprom_show_authenticate(struct device *dev, char *buf)
{
	int ret;

	if (!is_scm_armv8()) {
		ret = qcom_scm_call(dev, QTI_SCM_SVC_FUSE,
					QTI_QFPROM_IS_AUTHENTICATE_CMD, NULL,
					0, buf, sizeof(char));
	} else {
		__le32 scm_ret;
		struct scm_desc desc = {0};
		dma_addr_t auth_phys;
		void *auth_buf;

		auth_buf = dma_alloc_coherent(dev, sizeof(*buf),
						&auth_phys, GFP_KERNEL);
		if (!auth_buf) {
			dev_err(dev, "Allocation for auth buffer failed\n");
			return -ENOMEM;
		}
		desc.args[0] = (u64)auth_phys;
		desc.args[1] = sizeof(char);
		desc.arginfo = SCM_ARGS(2, QCOM_SCM_RO);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SCM_SVC_FUSE,
					QTI_QFPROM_IS_AUTHENTICATE_CMD), &desc);
		scm_ret = desc.ret[0];
		memcpy(buf, auth_buf, sizeof(char));
		dma_free_coherent(dev, sizeof(*buf), auth_buf, auth_phys);

		if (!ret)
			return le32_to_cpu(scm_ret);
	}

	return ret;
}

int __qti_qfprom_write_version(struct device *dev, void *wrip, int size)
{
	if (!is_scm_armv8())
		return  qcom_scm_call(dev, QTI_SCM_SVC_FUSE,
						QTI_QFPROM_ROW_WRITE_CMD,
						wrip, size, NULL, 0);
	else
		return -ENOTSUPP;
}

int __qti_qfprom_read_version(struct device *dev, uint32_t sw_type,
			uint32_t value, uint32_t qfprom_ret_ptr)
{
	int ret;

	if (!is_scm_armv8()) {
		struct qfprom_read {
			uint32_t sw_type;
			uint32_t value;
			uint32_t qfprom_ret_ptr;
		} rdip;

		rdip.sw_type = sw_type;
		rdip.value = value;
		rdip.qfprom_ret_ptr = qfprom_ret_ptr;

		ret = qcom_scm_call(dev, QTI_SCM_SVC_FUSE,
			QTI_QFPROM_ROW_READ_CMD, &rdip, sizeof(rdip), NULL, 0);

	} else {
		__le32 scm_ret;
		struct scm_desc desc = {0};

		desc.args[0] = sw_type;
		desc.args[1] = (u64)value;
		desc.args[2] = sizeof(uint32_t);
		desc.args[3] = (u64)qfprom_ret_ptr;
		desc.args[4] = sizeof(uint32_t);

		desc.arginfo = SCM_ARGS(5, QCOM_SCM_VAL, QCOM_SCM_RW,
							QCOM_SCM_VAL,
							QCOM_SCM_RW,
							QCOM_SCM_VAL);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SCM_SVC_FUSE,
					QTI_QFPROM_ROW_READ_CMD), &desc);
		scm_ret = desc.ret[0];

		if (!ret)
			return le32_to_cpu(scm_ret);
	}

	return ret;

}

int __qti_sec_upgrade_auth(struct device *dev, unsigned int scm_cmd_id,
							unsigned int sw_type,
							unsigned int img_size,
							unsigned int load_addr)
{
	int ret;
	struct {
		unsigned type;
		unsigned size;
		unsigned addr;
	} cmd_buf;

	if (!is_scm_armv8()) {
		cmd_buf.type = sw_type;
		cmd_buf.size = img_size;
		cmd_buf.addr = load_addr;
		ret = qcom_scm_call(dev, QCOM_SCM_SVC_BOOT,
					scm_cmd_id, &cmd_buf,
					sizeof(cmd_buf), NULL, 0);
	} else {
		__le32 scm_ret;
		struct scm_desc desc = {0};

		desc.args[0] = sw_type;
		desc.args[1] = img_size;
		desc.args[2] = (u64)load_addr;
		desc.arginfo = SCM_ARGS(3, QCOM_SCM_VAL, QCOM_SCM_VAL,
								QCOM_SCM_RW);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_BOOT,
							scm_cmd_id), &desc);
		scm_ret = desc.ret[0];

		if (!ret)
			return le32_to_cpu(scm_ret);
	}

	return ret;
}

int __qti_fuseipq_scm_call(struct device *dev, u32 svc_id, u32 cmd_id,
			    void *cmd_buf, size_t size)
{
	int ret;
	struct scm_desc desc = {0};
	uint32_t *status;

	if (is_scm_armv8()) {

		desc.arginfo = SCM_ARGS(1, QCOM_SCM_RO);
		desc.args[0] = *((uint32_t *)cmd_buf);

		ret = qti_scm_call2(dev, SCM_SIP_FNID(svc_id, cmd_id), &desc);
		status = (uint32_t *)(((uint32_t *)cmd_buf) + 1);
		*status = desc.ret[0];

	} else {

		return -ENOTSUPP;
	}

	return ret ? : le32_to_cpu(desc.ret[0]);
}

static int __qti_scm_dload_v8(struct device *dev, void *cmd_buf)
{
	struct scm_desc desc = {0};
	int ret;
	unsigned int enable;

	enable = cmd_buf ? *((unsigned int *)cmd_buf) : 0;
	desc.args[0] = TCSR_BOOT_MISC_REG;
	if (enable == SET_MAGIC_WARMRESET)
		desc.args[1] = DLOAD_MODE_ENABLE_WARMRESET;
	else
		desc.args[1] = enable ? DLOAD_MODE_ENABLE : DLOAD_MODE_DISABLE;
	desc.arginfo = SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_IO,
					QCOM_SCM_IO_WRITE), &desc);
	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

static int __qti_scm_wcss_boot_v8(struct device *dev, void *cmd_buf)
{
	struct scm_desc desc = {0};
	int ret;
	unsigned int enable;

	enable = cmd_buf ? *((unsigned int *)cmd_buf) : 0;
	desc.args[0] = TCSR_Q6SS_BOOT_TRIG_REG;
	desc.args[1] = enable;

	desc.arginfo = SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(SCM_SVC_IO_ACCESS,
			    SCM_IO_WRITE), &desc);
	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_wcss_boot(struct device *dev, u32 svc_id, u32 cmd_id,
						 void *cmd_buf)
{
	long ret;

	if (is_scm_armv8())
		return __qti_scm_wcss_boot_v8(dev, cmd_buf);

	if (cmd_buf)
		ret = qcom_scm_call(dev, svc_id, cmd_id, cmd_buf,
				    sizeof(cmd_buf), NULL, 0);
	else
		ret = qcom_scm_call(dev, svc_id, cmd_id, NULL, 0, NULL, 0);

	return ret;
}

static int __qti_scm_pdseg_memcpy_v2_v8(struct device *dev, u32 peripheral,
					int phno, dma_addr_t dma, int seg_cnt)
{
	struct scm_desc desc = {0};
	int ret;

	desc.args[0] = peripheral;
	desc.args[1] = phno;
	desc.args[2] = dma;
	desc.args[3] = seg_cnt;

	desc.arginfo = SCM_ARGS(4, QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_RW,
			QCOM_SCM_VAL);

	ret = qti_scm_call2(dev, SCM_SIP_FNID(PD_LOAD_SVC_ID,
				PD_LOAD_V2_CMD_ID), &desc);
	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_pdseg_memcpy_v2(struct device *dev, u32 peripheral,
				int phno, dma_addr_t dma, int seg_cnt)
{
	if (is_scm_armv8())
		return __qti_scm_pdseg_memcpy_v2_v8(dev, peripheral,
				phno, dma, seg_cnt);
	else
		return -ENOTSUPP;
}

static int __qti_scm_pdseg_memcpy_v8(struct device *dev, u32 peripheral,
					int phno, dma_addr_t dma, size_t size)
{
	struct scm_desc desc = {0};
	int ret;

	desc.args[0] = peripheral;
	desc.args[1] = phno;
	desc.args[2] = dma;
	desc.args[3] = size;

	desc.arginfo = SCM_ARGS(4, QCOM_SCM_VAL, QCOM_SCM_VAL, QCOM_SCM_RW,
			QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(PD_LOAD_SVC_ID,
				PD_LOAD_CMD_ID), &desc);
	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_pdseg_memcpy(struct device *dev, u32 peripheral,
				int phno, dma_addr_t dma, size_t size)
{
	if (is_scm_armv8())
		return __qti_scm_pdseg_memcpy_v8(dev, peripheral,
				phno, dma, size);
	else
		return -ENOTSUPP;
}

static int __qti_scm_int_radio_powerup_v8(struct device *dev, u32 peripheral)
{
	struct scm_desc desc = {0};
	int ret;

	desc.args[0] = peripheral;

	desc.arginfo = SCM_ARGS(1, QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(PD_LOAD_SVC_ID,
				INT_RAD_PWR_UP_CMD_ID), &desc);
	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_int_radio_powerup(struct device *dev, u32 peripheral)
{
	if (is_scm_armv8())
		return __qti_scm_int_radio_powerup_v8(dev, peripheral);
	else
		return -ENOTSUPP;
}

static int __qti_scm_int_radio_powerdown_v8(struct device *dev,
						u32 peripheral)
{
	struct scm_desc desc = {0};
	int ret;

	desc.args[0] = peripheral;

	desc.arginfo = SCM_ARGS(1, QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(PD_LOAD_SVC_ID,
				INT_RAD_PWR_DN_CMD_ID), &desc);
	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_int_radio_powerdown(struct device *dev, u32 peripheral)
{
	if (is_scm_armv8())
		return __qti_scm_int_radio_powerdown_v8(dev, peripheral);
	else
		return -ENOTSUPP;
}

int __qti_scm_dload(struct device *dev, u32 svc_id, u32 cmd_id, void *cmd_buf)
{
	long ret;

	if (is_scm_armv8())
		return __qti_scm_dload_v8(dev, cmd_buf);

	if (cmd_buf)
		ret = qcom_scm_call(dev, svc_id, cmd_id, cmd_buf,
				sizeof(cmd_buf), NULL, 0);
	else
		ret = qcom_scm_call(dev, svc_id, cmd_id, NULL, 0, NULL, 0);

	return ret;
}

static int __qti_scm_sdi_v8(struct device *dev)
{
	struct scm_desc desc = {0};
	int ret;

	desc.args[0] = 1ull;	/* Disable wdog debug */
	desc.args[1] = 0ull;	/* SDI Enable */
	desc.arginfo = SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_BOOT,
				SCM_CMD_TZ_CONFIG_HW_FOR_RAM_DUMP_ID), &desc);

	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_sdi(struct device *dev, u32 svc_id, u32 cmd_id)
{
	long ret;
	unsigned int clear_info[] = {
		1 /* Disable wdog debug */, 0 /* SDI enable*/, };

	if (is_scm_armv8())
		return __qti_scm_sdi_v8(dev);

	ret = qcom_scm_call(dev, svc_id, cmd_id, &clear_info,
				sizeof(clear_info), NULL, 0);

	return ret;
}

/**
 * __qti_scm_tz_hvc_log() - Get trustzone diag log or hypervisor diag log
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 * ker_buf: kernel buffer to store the diag log
 * buf_len: kernel buffer length
 *
 * This function can be used to get either the trustzone diag log
 * or the hypervisor diag log based on the command id passed to this
 * function. The hypervisor log is only applicable for armv8 and above
 * architectures.
 */
int __qti_scm_tz_hvc_log(struct device *dev, u32 svc_id, u32 cmd_id,
			 void *ker_buf, u32 buf_len)
{
	int ret;
	struct scm_desc desc = {0};
	struct log_read {
		uint32_t dma_buf;
		uint32_t buf_len;
	} rdip;
	dma_addr_t dma_buf;

	dma_buf = dma_map_single(dev, ker_buf, buf_len,
			DMA_FROM_DEVICE);

	ret = dma_mapping_error(dev, dma_buf);
	if (ret != 0) {
		pr_err("%s: DMA Mapping Error : %d\n", __func__, ret);
		return ret;
	}

	if (is_scm_armv8()) {
		desc.args[0] = dma_buf;
		desc.args[1] = buf_len;
		desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);

		ret = qti_scm_call2(dev, SCM_SIP_FNID(svc_id, cmd_id), &desc);

		if (!ret)
			ret = le32_to_cpu(desc.ret[0]);
	} else {
		rdip.buf_len = buf_len;
		rdip.dma_buf = dma_buf;

		ret = qcom_scm_call(dev, svc_id, cmd_id, &rdip,
				sizeof(struct log_read), NULL, 0);
	}

	dma_unmap_single(dev, dma_buf, buf_len, DMA_FROM_DEVICE);

	return ret;
}

/**
 * __qti_scm_get_smmustate () - Get SMMU state
 * @svc_id: SCM service id
 * @cmd_id: SCM command id
 *
 * Returns 0 - SMMU_DISABLE_NONE
 *	   1 - SMMU_DISABLE_S2
 *	   2 - SMMU_DISABLE_ALL on success.
 *	  -1 - Failure
 */
int __qti_scm_get_smmustate(struct device *dev, u32 svc_id, u32 cmd_id)
{
	__le32 out;
	int ret;
	struct scm_desc desc = {0};

	if (!is_scm_armv8()) {
		ret = qcom_scm_call(dev, svc_id, cmd_id, NULL, 0, &out,
				    sizeof(out));
	} else {
		desc.arginfo = SCM_ARGS(0);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(svc_id, cmd_id), &desc);
		out = desc.ret[0];
	}

	return ret ? -1 : le32_to_cpu(out);
}

int __qti_scm_regsave(struct device *dev, u32 svc_id, u32 cmd_id,
				void *scm_regsave, unsigned int buf_size)
{
	long ret;
	struct {
		unsigned addr;
		int len;
	} cmd_buf;

	if (!scm_regsave)
		return -EINVAL;

	if (is_scm_armv8()) {
		__le32 scm_ret;
		struct scm_desc desc = {0};

		desc.args[0] = (u64)virt_to_phys(scm_regsave);
		desc.args[1] = buf_size;
		desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(svc_id, cmd_id), &desc);
		scm_ret = desc.ret[0];
		if (!ret)
			return le32_to_cpu(scm_ret);
	} else {
		cmd_buf.addr = virt_to_phys(scm_regsave);
		cmd_buf.len = buf_size;
		ret = qcom_scm_call(dev, svc_id, cmd_id, &cmd_buf,
						sizeof(cmd_buf), NULL, 0);
	}

	return ret;
}

int __qti_set_qcekey_sec(struct device *dev, void *confBuf, int size)
{
	int ret;

	if (!is_scm_armv8()) {
		ret = qcom_scm_call(dev, QCOM_SCM_QCE_SVC,QCOM_SCM_QCE_CMD,
				NULL, 0, confBuf, size);
	} else {
		__le32 scm_ret;
		struct scm_desc desc = {0};
		dma_addr_t conf_phys;

		conf_phys = dma_map_single(dev, confBuf, size, DMA_TO_DEVICE);

		ret = dma_mapping_error(dev, conf_phys);

		if (ret) {
			dev_err(dev, "Allocation fail for conf buffer\n");
			return -ENOMEM;
		}

		desc.arginfo = SCM_ARGS(1, QCOM_SCM_RO);
		desc.args[0] = (u64)conf_phys;
		desc.args[1] = size;

		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_QCE_CRYPTO_SIP,
						QCOM_SCM_QCE_CMD), &desc);

		scm_ret = desc.ret[0];

		dma_unmap_single(dev, conf_phys, size, DMA_TO_DEVICE);

		if (!ret)
			return le32_to_cpu(scm_ret);
	}

	return ret;
}

int __qti_qcekey_release_xpu_prot(struct device *dev)
{
	int ret;
	__le32 scm_ret;
	struct scm_desc desc = {0};

	desc.arginfo = SCM_ARGS(0, QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_QCE_CRYPTO_SIP,
				QCOM_SCM_QCE_UNLOCK_CMD), &desc);

	scm_ret = desc.ret[0];
	if (!ret)
		return le32_to_cpu(scm_ret);

	return ret;
}

int __qti_scm_set_resettype(struct device *dev, u32 reset_type)
{
	__le32 out;
	__le32 in;
	int ret;
	struct scm_desc desc = {0};

	if (!is_scm_armv8()) {
		in = cpu_to_le32(reset_type);
		ret = qcom_scm_call(dev, QCOM_SCM_SVC_BOOT,
					QTI_SCM_SVC_RESETTYPE_CMD, &in,
					sizeof(in), &out, sizeof(out));
	} else {
		desc.args[0] = reset_type;
		desc.arginfo = SCM_ARGS(1);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_BOOT,
					QTI_SCM_SVC_RESETTYPE_CMD), &desc);
		out = desc.ret[0];
	}
	return ret ? : le32_to_cpu(out);
}

int __qti_config_ice_sec(struct device *dev, void *conf_buf, int size)
{
	int ret;

	if (!is_scm_armv8()) {
		ret = qcom_scm_call(dev, QTI_SVC_ICE, SCM_ARGS(2,
				QTI_SCM_PARAM_BUF_RO, QTI_SCM_PARAM_VAL),
				NULL, 0, conf_buf, size);
	} else {
		__le32 scm_ret;
		struct scm_desc desc = {0};
		dma_addr_t conf_phys;

		conf_phys = dma_map_single(dev, conf_buf, size, DMA_TO_DEVICE);

		ret = dma_mapping_error(dev, conf_phys);

		if (ret) {
			dev_err(dev, "Allocation fail for conf buffer\n");
			return -ENOMEM;
		}

		desc.arginfo = SCM_ARGS(2, QTI_SCM_PARAM_BUF_RO,
						QTI_SCM_PARAM_VAL);
		desc.args[0] = (u64)conf_phys;
		desc.args[1] = size;

		ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SVC_ICE,
				QTI_SCM_ICE_CMD), &desc);

		scm_ret = desc.ret[0];

		dma_unmap_single(dev, conf_phys, size, DMA_TO_DEVICE);

		if (!ret)
			return le32_to_cpu(scm_ret);
	}

	return ret;
}

int __qti_scm_pshold(struct device *dev)
{
	if (is_scm_armv8())
		return -ENOTSUPP;

	return qcom_scm_call(dev, QCOM_SCM_SVC_BOOT, QTI_SCM_CMD_PSHOLD,
							NULL, 0, NULL, 0);
}

int __qti_scm_extwdt(struct device *dev, u32 svc_id, u32 cmd_id,
				unsigned int regaddr, unsigned int val)
{
	long ret;
	struct {
		unsigned addr;
		int value;
	} cmd_buf;

	if (is_scm_armv8()) {
		__le32 scm_ret;
		struct scm_desc desc = {0};

		desc.args[0] = regaddr;
		desc.args[1] = val;
		desc.arginfo = SCM_ARGS(2, QCOM_SCM_RW, QCOM_SCM_VAL);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(svc_id, cmd_id), &desc);
		scm_ret = desc.ret[0];
		if (!ret)
			return le32_to_cpu(scm_ret);
	} else {
		cmd_buf.addr = regaddr;
		cmd_buf.value = val;
		ret = qcom_scm_call(dev, svc_id, cmd_id, &cmd_buf,
						sizeof(cmd_buf), NULL, 0);
	}

	return ret;
}

int __qti_scm_tcsr_reg_write(struct device *dev, u32 reg_addr, u32 value)
{
	struct scm_desc desc = {0};
	int ret;

	desc.args[0] = reg_addr;
	desc.args[1] = value;

	desc.arginfo = SCM_ARGS(2, QCOM_SCM_VAL, QCOM_SCM_VAL);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_IO,
					QCOM_SCM_IO_WRITE), &desc);
	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_is_tz_log_encrypted(struct device *dev)
{
	int ret;
	__le32 scm_ret;
	struct scm_desc desc = {0};

	desc.arginfo = SCM_ARGS(0);

	ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_BOOT,
					QCOM_SCM_IS_TZ_LOG_ENCRYPTED), &desc);
	scm_ret = desc.ret[0];

	if (!ret)
		return le32_to_cpu(scm_ret);

	return ret;
}

int __qti_scm_get_encrypted_tz_log(struct device *dev, void *ker_buf, u32 buf_len, u32 log_id)
{
	struct scm_desc desc = {0};
	dma_addr_t log_buf;
	int ret;

	log_buf = dma_map_single(dev, ker_buf, buf_len, DMA_FROM_DEVICE);
	ret = dma_mapping_error(dev, log_buf);

	if (ret) {
		dev_err(dev, "DMA mapping error : %d\n", ret);
		return ret;
	}

	desc.args[0] = log_buf;
	desc.args[1] = buf_len;
	desc.args[2] = log_id;
	desc.arginfo = SCM_ARGS(3, QCOM_SCM_RW, QCOM_SCM_VAL, QCOM_SCM_VAL);

	ret = qti_scm_call2(dev, SCM_SIP_FNID(QCOM_SCM_SVC_BOOT,
					QCOM_SCM_GET_TZ_LOG_ENCRYPTED), &desc);

	dma_unmap_single(dev, log_buf, buf_len, DMA_FROM_DEVICE);

	if (ret)
		return ret;

	return le32_to_cpu(desc.ret[0]);
}

int __qti_scm_load_otp(struct device *dev, u32 peripheral)
{
	__le32 out;
	__le32 in;
	int ret;
	struct scm_desc desc = {0};

	if (!is_scm_armv8()) {
		in = cpu_to_le32(peripheral);
		ret = qcom_scm_call(dev, QTI_SCM_SVC_OTP, QTI_SCM_CMD_OTP,
			    &in, sizeof(in),
			    &out, sizeof(out));
	} else {
		desc.args[0] = peripheral;
		desc.arginfo = SCM_ARGS(1);
		ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SCM_SVC_OTP,
						  QTI_SCM_CMD_OTP), &desc);

		out = desc.ret[0];
	}
	return ret ? : le32_to_cpu(out);
}

int __qti_scm_pil_cfg(struct device *dev, u32 peripheral, u32 arg)
{
	int ret;
	struct scm_desc desc = {0};

	desc.args[0] = peripheral;
	desc.args[1] = arg;
	desc.arginfo = SCM_ARGS(2);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SCM_SVC_XO_TCXO,
					  QTI_SCM_CMD_XO_TCXO), &desc);

	return ret ? : le32_to_cpu(desc.ret[0]);
}

int __qti_scm_toggle_bt_eco(struct device *dev, u32 peripheral, u32 arg)
{
	int ret;
	struct scm_desc desc = {0};

	desc.args[0] = peripheral;
	desc.args[1] = arg;
	desc.arginfo = SCM_ARGS(2);
	ret = qti_scm_call2(dev, SCM_SIP_FNID(QTI_SCM_SVC_BT_ECO,
					QTI_SCM_CMD_BT_ECO), &desc);

	return ret ? : le32_to_cpu(desc.ret[0]);
}
