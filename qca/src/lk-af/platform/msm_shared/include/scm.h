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

#ifndef __SCM_H__
#define __SCM_H__

/* 8 Byte SSD magic number (LE) */
#define DECRYPT_MAGIC_0 0x73737A74
#define DECRYPT_MAGIC_1 0x676D6964
#define ENCRYPT_MAGIC_0 0x6B647373
#define ENCRYPT_MAGIC_1 0x676D6973
#define SSD_HEADER_MAGIC_SIZE  8
#define SSD_HEADER_XML_SIZE    2048

typedef unsigned int uint32;

typedef struct {
	uint32 len;
	uint32 buf_offset;
	uint32 resp_hdr_offset;
	uint32 id;
} scm_command;

typedef struct {
	uint32 len;
	uint32 buf_offset;
	uint32 is_complete;
} scm_response;

typedef struct {
	uint32 *img_ptr;
	uint32 *img_len_ptr;
} img_req;

#define SCM_SVC_INFO            0x6
#define SCM_SVC_SSD                 7
#define SCM_SVC_IO_ACCESS	0x5
#define SSD_DECRYPT_ID              0x01
#define SSD_ENCRYPT_ID              0x02
#define TZBSP_BUILD_VER_QUERY_CMD	0x4

#define SCM_VAL		0x0
#define SCM_IO_READ     0x1
#define SCM_IO_WRITE    0x2

#define SCM_SVC_BOOT                    0x1
#define SCM_SVC_WR                      0x10
#define SCM_SVC_RD                      0x12
#define QFPROM_IS_AUTHENTICATE_CMD      0x7
#ifndef PLATFORM_IPQ6018
#define SCM_CMD_SEC_AUTH                0x15
#else
#define SCM_CMD_SEC_AUTH                0x1F
#endif
#define SCM_SVC_ID_SHIFT                0xA
#define SCM_SVC_RESETTYPE_CMD		0x18

static uint32 smc(uint32 cmd_addr);

int decrypt_scm(uint32_t ** img_ptr, uint32_t * img_len_ptr);
int encrypt_scm(uint32_t ** img_ptr, uint32_t * img_len_ptr);
int scm_call_atomic1(uint32_t svc, uint32_t cmd, uint32_t arg1);
int scm_call_atomic2(uint32_t svc, uint32_t cmd, uint32_t arg1, uint32_t arg2);
int scm_call(uint32_t svc_id, uint32_t cmd_id, const void *cmd_buf,
			size_t cmd_len, void *resp_buf, size_t resp_len);


#define SCM_SVC_FUSE                0x08
#define FUSEPROV_SUCCESS		0x0
#define FUSEPROV_INVALID_HASH		0x09
#define FUSEPROV_SECDAT_LOCK_BLOWN	0xB
#define TZ_BLOW_FUSE_SECDAT		0x20
#define SCM_BLOW_SW_FUSE_ID         0x01
#define SCM_IS_SW_FUSE_BLOWN_ID     0x02

#define HLOS_IMG_TAMPER_FUSE        0


#define SCM_SVC_CE_CHN_SWITCH_ID    0x04
#define SCM_CE_CHN_SWITCH_ID        0x02

enum ap_ce_channel_type {
AP_CE_REGISTER_USE = 0,
AP_CE_ADM_USE = 1,
AP_CE_BAM_USE = 2
};

/* Apps CE resource. */
#define TZ_RESOURCE_CE_AP  1

uint8_t switch_ce_chn_cmd(enum ap_ce_channel_type channel);

int fuseipq(uint32_t address);
void set_tamper_fuse_cmd();

/**
 * struct scm_command - one SCM command buffer
 * @len: total available memory for command and response
 * @buf_offset: start of command buffer
 * @resp_hdr_offset: start of response buffer
 * @id: command to be executed
 * @buf: buffer returned from scm_get_command_buffer()
 *
 * An SCM command is layed out in memory as follows:
 *
 *	------------------- <--- struct scm_command
 *	| command header  |
 *	------------------- <--- scm_get_command_buffer()
 *	| command buffer  |
 *	------------------- <--- struct scm_response and
 *	| response header |      scm_command_to_response()
 *	------------------- <--- scm_get_response_buffer()
 *	| response buffer |
 *	-------------------
 *
 * There can be arbitrary padding between the headers and buffers so
 * you should always use the appropriate scm_get_*_buffer() routines
 * to access the buffers in a safe manner.
 */
struct scm_command {
	uint32_t len;
	uint32_t buf_offset;
	uint32_t resp_hdr_offset;
	uint32_t id;
	uint32_t buf[0];
};

/**
 * struct scm_response - one SCM response buffer
 * @len: total available memory for response
 * @buf_offset: start of response data relative to start of scm_response
 * @is_complete: indicates if the command has finished processing
 */
struct scm_response {
	uint32_t len;
	uint32_t buf_offset;
	uint32_t is_complete;
};

#define MAX_QCA_SCM_RETS		3
#define MAX_QCA_SCM_ARGS		10
#define SCM_READ_OP			1

#define QCA_SCM_ARGS_IMPL(num, a, b, c, d, e, f, g, h, i, j, ...) (\
			(((a) & 0xff) << 4) | \
			(((b) & 0xff) << 6) | \
			(((c) & 0xff) << 8) | \
			(((d) & 0xff) << 10) | \
			(((e) & 0xff) << 12) | \
			(((f) & 0xff) << 14) | \
			(((g) & 0xff) << 16) | \
			(((h) & 0xff) << 18) | \
			(((i) & 0xff) << 20) | \
			(((j) & 0xff) << 22) | \
			(num & 0xffff))

#define QCA_SCM_ARGS(...) QCA_SCM_ARGS_IMPL(__VA_ARGS__, \
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0)

#define QCA_SCM_SIP_FNID(s, c) (((((s) & 0xFF) << 8) | \
			((c) & 0xFF)) | 0x02000000)

#define QCA_SMC_ATOMIC_MASK            0x80000000

#define QCA_MAX_ARG_LEN	5

#define SCM_ARCH64_SWITCH_ID	0x1
#define QCA_IS_CALL_AVAIL_CMD	0x1
#define SCM_EL1SWITCH_CMD_ID	0xf

#define SCM_NULL_OP 0
#define	SCM_RW_OP   2
#define	SCM_BUF_VAL 3


/**
 * struct qca_scm_desc
 *  <at> arginfo: Metadata describi`ng the arguments in args[]
 *  <at> args: The array of arguments for the secure syscall
 *  <at> ret: The values returned by the secure syscall
 *  <at> extra_arg_buf: The buffer containing extra arguments
                        (that don't fit in available registers)
 *  <at> x5: The 4rd argument to the secure syscall or physical address of
                extra_arg_buf
 */
struct qca_scm_desc {
	uint32_t arginfo;
	uint32_t args[MAX_QCA_SCM_ARGS];
	uint32_t ret[MAX_QCA_SCM_RETS];

	/* private */
	void *extra_arg_buf;
	uint64_t x5;
};

typedef struct {
	uint64_t reg_x0;
	uint64_t reg_x1;
	uint64_t reg_x2;
	uint64_t reg_x3;
	uint64_t reg_x4;
	uint64_t reg_x5;
	uint64_t reg_x6;
	uint64_t reg_x7;
	uint64_t reg_x8;
	uint64_t kernel_start;
} kernel_params;

int __qca_scm_call_armv8_32(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
			    uint32_t x4, uint32_t x5, uint32_t *ret1,
			    uint32_t *ret2, uint32_t *ret3);
bool is_scm_armv8(void);
void __attribute__ ((noreturn)) jump_kernel64(void *kernel_entry,
		void *fdt_addr);
int qca_scm_call_write(uint32_t svc_id,
		       uint32_t cmd_id, uint32_t *addr, uint32_t val);
int qca_scm_sdi_v8(uint32_t dump_id);
int qca_scm_call(uint32_t svc_id, uint32_t cmd_id, void *buf, size_t len);
int qca_scm_secure_authenticate(void *cmd_buf, size_t cmd_len);
int is_scm_sec_auth_available(uint32_t svc_id, uint32_t cmd_id);
int qca_scm_set_resettype(uint32_t reset_type);
int qca_scm_tz_log(uint32_t svc_id, uint32_t cmd_id, void *ker_buf, uint32_t buf_len);
int qca_scm_fuseipq(uint32_t svc_id, uint32_t cmd_id, void *buf, size_t len);

#endif
