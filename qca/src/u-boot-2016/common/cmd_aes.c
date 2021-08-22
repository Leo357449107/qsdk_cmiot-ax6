/*
 * Copyright (C) 2014 Marek Vasut <marex@denx.de>
 *
 * Command for en/de-crypting block of memory with AES-128-CBC cipher.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <aes.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <linux/compiler.h>
#include <asm/arch-qca-common/scm.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_CMD_AES_256
enum tz_crypto_service_aes_cmd_t {
	TZ_CRYPTO_SERVICE_AES_ENC_ID = 0x7,
	TZ_CRYPTO_SERVICE_AES_DEC_ID = 0x8,
};

enum tz_crypto_service_aes_type_t {
	TZ_CRYPTO_SERVICE_AES_SHK = 0x1,
	TZ_CRYPTO_SERVICE_AES_PHK = 0x2,
	TZ_CRYPTO_SERVICE_AES_TYPE_MAX,

};

enum tz_crypto_service_aes_mode_t {
       TZ_CRYPTO_SERVICE_AES_ECB = 0x0,
       TZ_CRYPTO_SERVICE_AES_CBC = 0x1,
	TZ_CRYPTO_SERVICE_AES_MODE_MAX,
};

struct crypto_aes_req_data_t {
	uint64_t type;
	uint64_t mode;
        uint64_t req_buf;
        uint64_t req_len;
        uint64_t ivdata;
        uint64_t iv_len;
        uint64_t resp_buf;
        uint64_t resp_len;
};

/**
 * do_aes_256() - Handle the "aes" command-line command
 * @cmdtp:	Command data struct pointer
 * @flag:	Command flag
 * @argc:	Command-line argument count
 * @argv:	Array of command-line arguments
 *
 * Returns zero on success, CMD_RET_USAGE in case of misuse and negative
 * on error.
 */
static int do_aes_256(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	uint64_t src_addr, dst_addr, ivdata;
	uint64_t req_len, iv_len, resp_len, type, mode;
	struct crypto_aes_req_data_t *req_ptr = NULL;
	int cmd_id = -1;
	int ret = CMD_RET_USAGE;

	if (argc != 10)
		return ret;

	if (!strncmp(argv[1], "enc", 3))
		cmd_id = TZ_CRYPTO_SERVICE_AES_ENC_ID;
	else if (!strncmp(argv[1], "dec", 3))
		cmd_id = TZ_CRYPTO_SERVICE_AES_DEC_ID;
	else
		return ret;

	type = simple_strtoul(argv[2], NULL, 16);
	if (type >= TZ_CRYPTO_SERVICE_AES_TYPE_MAX) {
		printf("unkown type specified, use 0x1 - SHK, 0x2 - PHK\n");
		goto err;
	}

	mode = simple_strtoul(argv[3], NULL, 16);
        if (mode >= TZ_CRYPTO_SERVICE_AES_MODE_MAX) {
                printf("unkown mode specified, use 0x0 - ECB, 0x1 - CBC\n");
                goto err;
        }

	src_addr = simple_strtoull(argv[4], NULL, 16);
	req_len = simple_strtoul(argv[5], NULL, 16);
	if (req_len <= 0 || (req_len % 16) != 0) {
		printf("Invalid request buffer length, length "
			"should be multiple of AES block size (16)\n");
		goto err;
	}

	ivdata = simple_strtoull(argv[6], NULL, 16);
	iv_len = simple_strtoul(argv[7], NULL, 16);
	if (iv_len != 16) {
		printf("Error: iv length should be equal to AES block size (16)\n");
		goto err;
	}

	dst_addr = simple_strtoull(argv[8], NULL, 16);
	resp_len =  simple_strtoul(argv[9], NULL, 16);
	if (resp_len < req_len) {
		printf("Error: response buffer cannot be less then request buffer\n");
		goto err;
	}

	req_ptr = (struct crypto_aes_req_data_t *)memalign(ARCH_DMA_MINALIGN,
					sizeof(struct crypto_aes_req_data_t));
	if (!req_ptr) {
		printf("Error allocating memory");
		return -ENOMEM;
	}

	req_ptr->type = type;
	req_ptr->mode = mode;
	req_ptr->req_buf = (uint64_t)src_addr;
	req_ptr->req_len = req_len;
	req_ptr->ivdata = (mode == TZ_CRYPTO_SERVICE_AES_CBC) ? (uint64_t)ivdata : 0;
	req_ptr->iv_len = iv_len;
	req_ptr->resp_buf = (uint64_t)dst_addr;
	req_ptr->resp_len = resp_len;
	ret = qca_scm_crypto(cmd_id, (void *)req_ptr,
					sizeof(struct crypto_aes_req_data_t));

	if (req_ptr)
		free(req_ptr);

	if (ret) {
		printf("Scm call failed with error code: %d\n", ret);
		return ret;
	}
	return 0;

err:
	if (req_ptr)
		free(req_ptr);
	return CMD_RET_USAGE;
}

/***************************************************/
U_BOOT_CMD(
	aes_256, 10, 1, do_aes_256,
	"AES 256 CBC/ECB encryption/decryption",
	"Encryption: echo enc <type> <mode> <plain data address> <plain data len>"
	"<iv data address> <iv len> <response buf address> <response buf len>"
	"Decryption: echo dec <type> <mode> <Encrypted buf address> <encrypted"
	"buf len> <iv data address> <iv len> <response buf address> <response buf len>"
);
#endif


#ifdef CONFIG_CMD_AES_128
/**
 * do_aes() - Handle the "aes" command-line command
 * @cmdtp:	Command data struct pointer
 * @flag:	Command flag
 * @argc:	Command-line argument count
 * @argv:	Array of command-line arguments
 *
 * Returns zero on success, CMD_RET_USAGE in case of misuse and negative
 * on error.
 */
static int do_aes(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	uint32_t key_addr, src_addr, dst_addr, len;
	uint8_t *key_ptr, *src_ptr, *dst_ptr;
	uint8_t key_exp[AES_EXPAND_KEY_LENGTH];
	uint32_t aes_blocks;
	int enc;

	if (argc != 6)
		return CMD_RET_USAGE;

	if (!strncmp(argv[1], "enc", 3))
		enc = 1;
	else if (!strncmp(argv[1], "dec", 3))
		enc = 0;
	else
		return CMD_RET_USAGE;

	key_addr = simple_strtoul(argv[2], NULL, 16);
	src_addr = simple_strtoul(argv[3], NULL, 16);
	dst_addr = simple_strtoul(argv[4], NULL, 16);
	len = simple_strtoul(argv[5], NULL, 16);

	key_ptr = (uint8_t *)key_addr;
	src_ptr = (uint8_t *)src_addr;
	dst_ptr = (uint8_t *)dst_addr;

	/* First we expand the key. */
	aes_expand_key(key_ptr, key_exp);

	/* Calculate the number of AES blocks to encrypt. */
	aes_blocks = DIV_ROUND_UP(len, AES_KEY_LENGTH);

	if (enc)
		aes_cbc_encrypt_blocks(key_exp, src_ptr, dst_ptr, aes_blocks);
	else
		aes_cbc_decrypt_blocks(key_exp, src_ptr, dst_ptr, aes_blocks);

	return 0;
}

/***************************************************/
#ifdef CONFIG_SYS_LONGHELP
static char aes_help_text[] =
	"enc key src dst len - Encrypt block of data $len bytes long\n"
	"                          at address $src using a key at address\n"
	"                          $key and store the result at address\n"
	"                          $dst. The $len size must be multiple of\n"
	"                          16 bytes and $key must be 16 bytes long.\n"
	"aes dec key src dst len - Decrypt block of data $len bytes long\n"
	"                          at address $src using a key at address\n"
	"                          $key and store the result at address\n"
	"                          $dst. The $len size must be multiple of\n"
	"                          16 bytes and $key must be 16 bytes long.";
#endif

U_BOOT_CMD(
	aes, 6, 1, do_aes,
	"AES 128 CBC encryption",
	aes_help_text
);
#endif
