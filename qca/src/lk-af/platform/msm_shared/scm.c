/* Copyright (c) 2011-2012, 2019 The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <arch/ops.h>
#include <libfdt.h>
#include "scm.h"

#pragma GCC optimize ("O0")

/* From Linux Kernel asm/system.h */
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"

#ifndef offsetof
#  define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

struct tz_log_read {
        uint32_t log_buf;
        uint32_t buf_size;
};

/**
 * alloc_scm_command() - Allocate an SCM command
 * @cmd_size: size of the command buffer
 * @resp_size: size of the response buffer
 *
 * Allocate an SCM command, including enough room for the command
 * and response headers as well as the command and response buffers.
 *
 * Returns a valid &scm_command on success or %NULL if the allocation fails.
 */
static struct scm_command *alloc_scm_command(size_t cmd_size, size_t resp_size)
{
	struct scm_command *cmd;
	size_t len = sizeof(*cmd) + sizeof(struct scm_response) + cmd_size +
	    resp_size;

	cmd = memalign(CACHE_LINE, ROUNDUP(len, CACHE_LINE));
	if (cmd) {
		cmd->len = len;
		cmd->buf_offset = offsetof(struct scm_command, buf);
		cmd->resp_hdr_offset = cmd->buf_offset + cmd_size;
	}
	return cmd;
}

/**
 * free_scm_command() - Free an SCM command
 * @cmd: command to free
 *
 * Free an SCM command.
 */
static inline void free_scm_command(struct scm_command *cmd)
{
	free(cmd);
}

/**
 * scm_command_to_response() - Get a pointer to a scm_response
 * @cmd: command
 *
 * Returns a pointer to a response for a command.
 */
static inline struct scm_response *scm_command_to_response(const struct
							   scm_command *cmd)
{
	return (void *)cmd + cmd->resp_hdr_offset;
}

/**
 * scm_get_command_buffer() - Get a pointer to a command buffer
 * @cmd: command
 *
 * Returns a pointer to the command buffer of a command.
 */
static inline void *scm_get_command_buffer(const struct scm_command *cmd)
{
	return (void *)cmd->buf;
}

/**
 * scm_get_response_buffer() - Get a pointer to a response buffer
 * @rsp: response
 *
 * Returns a pointer to a response buffer of a response.
 */
static inline void *scm_get_response_buffer(const struct scm_response *rsp)
{
	return (void *)rsp + rsp->buf_offset;
}

static uint32_t smc(uint32_t cmd_addr)
{
	uint32_t context_id;
	register uint32_t r0 __asm__("r0") = 1;
	register uint32_t r1 __asm__("r1") = (uint32_t) & context_id;
	register uint32_t r2 __asm__("r2") = cmd_addr;
 __asm__("1:smc	#0	@ switch to secure world\n" "cmp	r0, #1				\n" "beq	1b				\n": "=r"(r0): "r"(r0), "r"(r1), "r"(r2):"r3", "cc");
	return r0;
}

#define SCM_CLASS_REGISTER	(0x2 << 8)
#define SCM_MASK_IRQS		(1<<5)
#define SCM_ATOMIC(svc, cmd, n) (((((svc) << 10)|((cmd) & 0x3ff)) << 12) | \
				SCM_CLASS_REGISTER | \
				SCM_MASK_IRQS | \
				(n & 0xf))

/**
 * scm_call_atomic1() - Send an atomic SCM command with one argument
 * @svc_id: service identifier
 * @cmd_id: command identifier
 * @arg1: first argument
 *
 * This shall only be used with commands that are guaranteed to be
 * uninterruptable, atomic and SMP safe.
 */
int scm_call_atomic1(uint32_t svc, uint32_t cmd, uint32_t arg1)
{
	int context_id;
	register uint32_t r0 __asm__("r0") = SCM_ATOMIC(svc, cmd, 1);
	register uint32_t r1 __asm__("r1") = (uint32_t)&context_id;
	register uint32_t r2 __asm__("r2") = arg1;

	__asm__ volatile(
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
		: "r3");
	return r0;
}

/**
 * scm_call_atomic2() - Send an atomic SCM command with two arguments
 * @svc_id: service identifier
 * @cmd_id: command identifier
 * @arg1: first argument
 * @arg2: second argument
 *
 * This shall only be used with commands that are guaranteed to be
 * uninterruptable, atomic and SMP safe.
 */
int scm_call_atomic2(uint32_t svc, uint32_t cmd, uint32_t arg1, uint32_t arg2)
{
	int context_id;
	register uint32_t r0 __asm__("r0") = SCM_ATOMIC(svc, cmd, 2);
	register uint32_t r1 __asm__("r1") = (uint32_t)&context_id;
	register uint32_t r2 __asm__("r2") = arg1;
	register uint32_t r3 __asm__("r3") = arg2;

	__asm__ volatile(
		__asmeq("%0", "r0")
		__asmeq("%1", "r0")
		__asmeq("%2", "r1")
		__asmeq("%3", "r2")
		__asmeq("%4", "r3")
#ifdef REQUIRES_SEC
			".arch_extension sec\n"
#endif
		"smc	#0	@ switch to secure world\n"
		: "=r" (r0)
		: "r" (r0), "r" (r1), "r" (r2), "r" (r3));
	return r0;
}

/**
 * scm_call() - Send an SCM command
 * @svc_id: service identifier
 * @cmd_id: command identifier
 * @cmd_buf: command buffer
 * @cmd_len: length of the command buffer
 * @resp_buf: response buffer
 * @resp_len: length of the response buffer
 *
 * Sends a command to the SCM and waits for the command to finish processing.
 */
int
scm_call(uint32_t svc_id, uint32_t cmd_id, const void *cmd_buf,
	 size_t cmd_len, void *resp_buf, size_t resp_len)
{
	int ret;
	struct scm_command *cmd;
	struct scm_response *rsp;
	uint8_t *resp_ptr;

	cmd = alloc_scm_command(cmd_len, resp_len);
	if (!cmd)
		return ERR_NO_MEMORY;

	cmd->id = (svc_id << 10) | cmd_id;
	if (cmd_buf)
		memcpy(scm_get_command_buffer(cmd), cmd_buf, cmd_len);

	/* Flush command to main memory for TZ */
	arch_clean_invalidate_cache_range((addr_t) cmd, cmd->len);

	ret = smc((uint32_t) cmd);

	if (ret)
		goto out;

	if (resp_len) {
		rsp = scm_command_to_response(cmd);

		do
		{
			/* Need to invalidate before each check since TZ will update
			 * the response complete flag in main memory.
			 */
			arch_clean_invalidate_cache_range((addr_t) rsp, sizeof(*rsp));
		} while (!rsp->is_complete);


		resp_ptr = scm_get_response_buffer(rsp);

		/* Invalidate any cached response data */
		arch_clean_invalidate_cache_range((addr_t) resp_ptr, resp_len);

		if (resp_buf)
			memcpy(resp_buf, resp_ptr, resp_len);
	}
 out:
	free_scm_command(cmd);
	return ret;
}

/* SCM Encrypt Command */
int encrypt_scm(uint32_t ** img_ptr, uint32_t * img_len_ptr)
{
	int ret;
	img_req cmd;

	cmd.img_ptr     = (uint32*) img_ptr;
	cmd.img_len_ptr = img_len_ptr;

	/* Image data is operated upon by TZ, which accesses only the main memory.
	 * It must be flushed/invalidated before and after TZ call.
	 */
	arch_clean_invalidate_cache_range((addr_t) *img_ptr, *img_len_ptr);

	ret = scm_call(SCM_SVC_SSD, SSD_ENCRYPT_ID, &cmd, sizeof(cmd), NULL, 0);

	/* Values at img_ptr and img_len_ptr are updated by TZ. Must be invalidated
	 * before we use them.
	 */
	arch_clean_invalidate_cache_range((addr_t) img_ptr, sizeof(img_ptr));
	arch_clean_invalidate_cache_range((addr_t) img_len_ptr, sizeof(img_len_ptr));

	/* Invalidate the updated image data */
	arch_clean_invalidate_cache_range((addr_t) *img_ptr, *img_len_ptr);

	return ret;
}

/* SCM Decrypt Command */
int decrypt_scm(uint32_t ** img_ptr, uint32_t * img_len_ptr)
{
	int ret;
	img_req cmd;

	cmd.img_ptr     = (uint32*) img_ptr;
	cmd.img_len_ptr = img_len_ptr;

	/* Image data is operated upon by TZ, which accesses only the main memory.
	 * It must be flushed/invalidated before and after TZ call.
	 */
	arch_clean_invalidate_cache_range((addr_t) *img_ptr, *img_len_ptr);

	ret = scm_call(SCM_SVC_SSD, SSD_DECRYPT_ID, &cmd, sizeof(cmd), NULL, 0);

	/* Values at img_ptr and img_len_ptr are updated by TZ. Must be invalidated
	 * before we use them.
	 */
	arch_clean_invalidate_cache_range((addr_t) img_ptr, sizeof(img_ptr));
	arch_clean_invalidate_cache_range((addr_t) img_len_ptr, sizeof(img_len_ptr));

	/* Invalidate the updated image data */
	arch_clean_invalidate_cache_range((addr_t) *img_ptr, *img_len_ptr);

	return ret;
}


void set_tamper_fuse_cmd()
{
	uint32_t svc_id;
	uint32_t cmd_id;
	void *cmd_buf;
	size_t cmd_len;
	void *resp_buf = NULL;
	size_t resp_len = 0;

	uint32_t fuse_id = HLOS_IMG_TAMPER_FUSE;
	cmd_buf = (void *)&fuse_id;
	cmd_len = sizeof(fuse_id);

	/*no response */
	resp_buf = NULL;
	resp_len = 0;

	svc_id = SCM_SVC_FUSE;
	cmd_id = SCM_BLOW_SW_FUSE_ID;

	scm_call(svc_id, cmd_id, cmd_buf, cmd_len, resp_buf, resp_len);
	return;
}

uint8_t get_tamper_fuse_cmd()
{
	uint32_t svc_id;
	uint32_t cmd_id;
	void *cmd_buf;
	size_t cmd_len;
	size_t resp_len = 0;
	uint8_t resp_buf;

	uint32_t fuse_id = HLOS_IMG_TAMPER_FUSE;
	cmd_buf = (void *)&fuse_id;
	cmd_len = sizeof(fuse_id);

	/*response */
	resp_len = sizeof(resp_buf);

	svc_id = SCM_SVC_FUSE;
	cmd_id = SCM_IS_SW_FUSE_BLOWN_ID;

	scm_call(svc_id, cmd_id, cmd_buf, cmd_len, &resp_buf, resp_len);
	return resp_buf;
}

/*
 * Switches the CE1 channel between ADM and register usage.
 * channel : AP_CE_REGISTER_USE, CE1 uses register interface
 *         : AP_CE_ADM_USE, CE1 uses ADM interface
 */
uint8_t switch_ce_chn_cmd(enum ap_ce_channel_type channel)
{
	uint32_t svc_id;
	uint32_t cmd_id;
	void *cmd_buf;
	size_t cmd_len;
	size_t resp_len = 0;
	uint8_t resp_buf;

	struct {
		uint32_t resource;
		uint32_t chn_id;
		}__PACKED switch_ce_chn_buf;

	switch_ce_chn_buf.resource = TZ_RESOURCE_CE_AP;
	switch_ce_chn_buf.chn_id = channel;
	cmd_buf = (void *)&switch_ce_chn_buf;
	cmd_len = sizeof(switch_ce_chn_buf);

	/*response */
	resp_len = sizeof(resp_buf);

	svc_id = SCM_SVC_CE_CHN_SWITCH_ID;
	cmd_id = SCM_CE_CHN_SWITCH_ID;

	scm_call(svc_id, cmd_id, cmd_buf, cmd_len, &resp_buf, resp_len);
	return resp_buf;
}

/* armv8 support */
int __qca_scm_call_armv8_32(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t x4, uint32_t x5,
				uint32_t *ret1, uint32_t *ret2, uint32_t *ret3)
{
	register uint32_t r0 __asm__("r0") = x0;
	register uint32_t r1 __asm__("r1") = x1;
	register uint32_t r2 __asm__("r2") = x2;
	register uint32_t r3 __asm__("r3") = x3;
	register uint32_t r4 __asm__("r4") = x4;
	register uint32_t r5 __asm__("r5") = x5;

	__asm__ volatile(
		__asmeq("%0", "r0")
		__asmeq("%1", "r1")
		__asmeq("%2", "r2")
		__asmeq("%3", "r3")
		__asmeq("%4", "r0")
		__asmeq("%5", "r1")
		__asmeq("%6", "r2")
		__asmeq("%7", "r3")
		__asmeq("%8", "r4")
		__asmeq("%9", "r5")
		".arch_extension sec\n"
		"smc    #0\n"
		: "=r" (r0), "=r" (r1), "=r" (r2), "=r" (r3)
		: "r" (r0), "r" (r1), "r" (r2), "r" (r3), "r" (r4),
		"r" (r5));

	if (ret1)
		*ret1 = r1;
	if (ret2)
		*ret2 = r2;
	if (ret3)
		*ret3 = r3;

	return r0;
}


/**
 * scm_call_64() - Invoke a syscall in the secure world
 *  <at> svc_id: service identifier
 *  <at> cmd_id: command identifier
 *  <at> fn_id: The function ID for this syscall
 *  <at> desc: Descriptor structure containing arguments and return values
 *
 * Sends a command to the SCM and waits for the command to finish processing.
 *
 */
static int scm_call_64(uint32_t svc_id, uint32_t cmd_id, struct qca_scm_desc *desc)
{
	int arglen = desc->arginfo & 0xf;
	int ret;
	uint32_t fn_id = QCA_SCM_SIP_FNID(svc_id, cmd_id);

	if (arglen > QCA_MAX_ARG_LEN) {
		dprintf(INFO, "Error Extra args not supported\n");
		for(;;);
	}

	desc->ret[0] = desc->ret[1] = desc->ret[2] = 0;

	arch_clean_cache_range(BASE_ADDR, MEMBASE + MEMSIZE - BASE_ADDR);

	ret = __qca_scm_call_armv8_32(fn_id, desc->arginfo,
				      desc->args[0], desc->args[1],
				      desc->args[2], desc->x5,
				      &desc->ret[0], &desc->ret[1],
				      &desc->ret[2]);

	return ret;
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
        uint32_t ret1, x0;

        if (likely(scm_version != SCM_UNKNOWN))
                return (scm_version == SCM_ARMV8_32);

        /* Try SMC32 call */
        ret1 = 0;
        x0 = QCA_SCM_SIP_FNID(SCM_SVC_INFO, QCA_IS_CALL_AVAIL_CMD) |
                        QCA_SMC_ATOMIC_MASK;

	arch_clean_cache_range(BASE_ADDR, MEMBASE + MEMSIZE - BASE_ADDR);

	ret = __qca_scm_call_armv8_32(x0, QCA_SCM_ARGS(1), x0, 0, 0, 0,
				      &ret1, NULL, NULL);
        if (ret || !ret1)
                scm_version = SCM_LEGACY;
        else
                scm_version = SCM_ARMV8_32;

        dprintf(SPEW, "scm_call: scm version is %x\n", scm_version);

        return (scm_version == SCM_ARMV8_32);
}

void jump_kernel64(void *kernel_entry,
				void *fdt_addr)
{
	struct qca_scm_desc desc = {0};
	int ret = 0;
	kernel_params param = {0};

	param.reg_x0 = (uint32_t)fdt_addr;
	param.kernel_start = (uint32_t)kernel_entry;

	desc.arginfo = QCA_SCM_ARGS(2, SCM_READ_OP);
	desc.args[0] = (uint32_t) &param;
	desc.args[1] = sizeof(param);

	if (!is_scm_armv8()) {
		dprintf(INFO, "Can't boot kernel: %d\n", ret);
		for(;;);
	}

	dprintf(INFO, "Jumping to AARCH64 kernel via monitor\n");
	ret = scm_call_64(SCM_ARCH64_SWITCH_ID, SCM_EL1SWITCH_CMD_ID,
		&desc);

	dprintf(INFO, "Can't boot kernel: %d\n", ret);
	for(;;);
}

int qca_scm_call_write(uint32_t svc_id,
		       uint32_t cmd_id, uint32_t *addr, uint32_t val)
{
	int ret = 0;

	struct qca_scm_desc desc = {0};

	/*
	 * In ipq807x, this SCM call is called as a Fast
	 * SCM call which means it will get executed in
	 * EL3 monitor mode itself without jumping to QSEE.
	 * But, In ipq6018, We need to jump into QSEE which
	 * will execute the SCM call, as we do not have
	 * support for Fast SCM call in ipq6018.
	 */

	desc.arginfo = QCA_SCM_ARGS(2, SCM_VAL, SCM_VAL);
	desc.args[0] = (uint32_t)addr;
	desc.args[1] = val;
	ret = scm_call_64(svc_id, cmd_id, &desc);

	return ret;
}

int qca_scm_sdi_v8(uint32_t dump_id)
{
	struct qca_scm_desc desc = {0};
	int ret;

	desc.args[0] = 1ul;    /* Disable wdog debug */
	desc.args[1] = 0ul;    /* SDI Enable */
	desc.arginfo = QCA_SCM_ARGS(2, SCM_VAL, SCM_VAL);
	ret = scm_call_64(SCM_SVC_BOOT, dump_id, &desc);

	if (ret)
		return ret;

	return desc.ret[0];
}

#ifndef CONFIG_SYS_CACHELINE_SIZE
#define CONFIG_SYS_CACHELINE_SIZE       128
#endif
static uint8_t tz_buf[CONFIG_SYS_CACHELINE_SIZE] __attribute__ ((aligned(CONFIG_SYS_CACHELINE_SIZE)));

int qca_scm_call(uint32_t svc_id, uint32_t cmd_id, void *buf, size_t len)
{
	int ret = 0;

	if (is_scm_armv8())
	{
		struct qca_scm_desc desc = {0};
		memcpy(tz_buf, buf, len);

		desc.arginfo = QCA_SCM_ARGS(2, SCM_READ_OP);
		desc.args[0] = (uint32_t)tz_buf;
		desc.args[1] = len;
		ret = scm_call_64(svc_id, cmd_id, &desc);
		/* qca_scm_call is called with len 1, hence tz_buf is enough */
		arch_invalidate_cache_range((addr_t)tz_buf,
					    CONFIG_SYS_CACHELINE_SIZE);
		memcpy(buf, tz_buf, len);
	}
	else
	{
		ret = scm_call(svc_id, cmd_id, NULL, 0, buf, len);
	}
	return ret;
}

int is_scm_sec_auth_available(uint32_t svc_id, uint32_t cmd_id)
{
	int ret;
	uint32_t scm_ret;

	if (is_scm_armv8())
	{
		struct qca_scm_desc desc = {0};

		desc.arginfo = QCA_SCM_ARGS(1);
		desc.args[0] = QCA_SCM_SIP_FNID(svc_id, cmd_id);

		ret = scm_call_64(SCM_SVC_INFO, QCA_IS_CALL_AVAIL_CMD, &desc);
		scm_ret = desc.ret[0];
	}
	else
	{
		uint32_t svc_cmd = cpu_to_fdt32((svc_id << SCM_SVC_ID_SHIFT) | cmd_id);

		ret = scm_call(SCM_SVC_INFO, QCA_IS_CALL_AVAIL_CMD, &svc_cmd,
				sizeof(svc_cmd), &scm_ret, sizeof(scm_ret));
	}

	if (!ret)
		return fdt32_to_cpu(scm_ret);

	return ret;
}

int qca_scm_secure_authenticate(void *cmd_buf, size_t cmd_len)
{
	int ret = 0;

	if (is_scm_armv8())
	{
		struct qca_scm_desc desc = {0};

		desc.arginfo = QCA_SCM_ARGS(3, SCM_VAL, SCM_VAL, SCM_IO_WRITE);
		/* args[0] has the image SW ID*/
		desc.args[0] = * ((unsigned long *)cmd_buf);
		/* args[1] has the image size */
		desc.args[1] = * (((unsigned long *)cmd_buf) + 1);
		/* args[2] has the load address*/
		desc.args[2] = * (((unsigned long *)cmd_buf) + 2);

		ret = scm_call_64(SCM_SVC_BOOT, SCM_CMD_SEC_AUTH, &desc);
	}
	else
	{
		ret = scm_call(SCM_SVC_BOOT, SCM_CMD_SEC_AUTH, cmd_buf, cmd_len,
									NULL, 0);
	}

	return ret;
}

/*
 * qcom_scm_set_resettype() - configure cold or warm reset
 * @reset type: 0 cold
 *		1 warm
 * Returns 0 on success
 */
int qca_scm_set_resettype(uint32_t reset_type)
{
	uint32_t out;
	int ret = 0;

	if (is_scm_armv8()) {
		struct qca_scm_desc desc = {0};

		desc.args[0] = reset_type;
		desc.arginfo = QCA_SCM_ARGS(1, SCM_VAL);

		ret = scm_call_64(SCM_SVC_BOOT, SCM_SVC_RESETTYPE_CMD, &desc);
		out = desc.ret[0];
	}
	else {
		uint32_t in;

		in = cpu_to_fdt32(reset_type);
		ret = scm_call(SCM_SVC_BOOT, SCM_SVC_RESETTYPE_CMD,
				 &in, sizeof(in), &out, sizeof(out));
	}

	if (!ret)
		ret = fdt32_to_cpu(out);

	return ret;
}

int qca_scm_fuseipq(uint32_t svc_id, uint32_t cmd_id, void *buf, size_t len)
{
	int ret = 0;
	uint32_t *status;
	if (is_scm_armv8())
	{
		struct qca_scm_desc desc = {0};

		desc.arginfo = QCA_SCM_ARGS(1, SCM_READ_OP);
		desc.args[0] = *((unsigned int *)buf);

		ret = scm_call_64(svc_id, cmd_id, &desc);

		status = (uint32_t *)(*(((uint32_t *)buf) + 1));
		*status = desc.ret[0];
	}
	else
	{
		ret = scm_call(svc_id, cmd_id, buf, len, NULL, 0);
	}
	return ret;
}

int qca_scm_tz_log(uint32_t svc_id, uint32_t cmd_id,
			void *ker_buf, uint32_t buf_len)
{
	int ret;
	uint32_t log_buf = (uint32_t) ker_buf;

	if (is_scm_armv8()) {
		struct qca_scm_desc desc = {0};

		desc.args[0] = log_buf;
		desc.args[1] = buf_len;
		desc.arginfo = QCA_SCM_ARGS(2, SCM_IO_WRITE, SCM_VAL);

		ret = scm_call_64(svc_id, cmd_id, &desc);
		if (!ret)
			ret = fdt32_to_cpu(desc.ret[0]);
	}
	else {
		struct tz_log_read tzlog;

		tzlog.log_buf = log_buf;
		tzlog.buf_size = buf_len;

		ret = scm_call(svc_id, cmd_id, &tzlog, sizeof(struct tz_log_read),
								NULL, 0);
	}
	arch_invalidate_cache_range((addr_t)log_buf, buf_len);

	return ret;
}
