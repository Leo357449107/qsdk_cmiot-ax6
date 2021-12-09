/* QTI Secure Execution Environment Communicator (QSEECOM) driver
 *
 * Copyright (c) 2012, 2015, 2017-2018 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _qseecom_h
#define _qseecom_h

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/highuid.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/qcom_scm.h>
#include <linux/sysfs.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/random.h>
#include <linux/of_reserved_mem.h>

#define CLIENT_CMD_CRYPTO_AES_DECRYPT	8
#define CLIENT_CMD_CRYPTO_AES_ENCRYPT	7
#define CLIENT_CMD_CRYPTO_AES_64	6
#define CLIENT_CMD_CRYPTO_RSA_64	5
#define CLIENT_CMD_CRYPTO_RSA		3
#define CLIENT_CMD_CRYPTO_AES		2
#define CLIENT_CMD1_BASIC_DATA		1
#define CLIENT_CMD8_RUN_CRYPTO_TEST	3
#define CLIENT_CMD8_RUN_CRYPTO_ENCRYPT	8
#define CLIENT_CMD9_RUN_CRYPTO_DECRYPT	9
#define MAX_INPUT_SIZE			4096
#define QSEE_64				64
#define QSEE_32				32
#define AES_BLOCK_SIZE			16

#define MAX_ENCRYPTED_DATA_SIZE  (2072 * sizeof(uint8_t))
#define MAX_PLAIN_DATA_SIZE	 (2048 * sizeof(uint8_t))
#define MAX_RSA_PLAIN_DATA_SIZE  (1792 * sizeof(uint8_t))

#define ENCRYPTED_DATA_HEADER \
	(MAX_ENCRYPTED_DATA_SIZE - MAX_PLAIN_DATA_SIZE)

#define KEY_BLOB_SIZE		(56 * sizeof(uint8_t))
#define KEY_SIZE		(32 * sizeof(uint8_t))

#define RSA_KEY_SIZE_MAX	((528) * sizeof(uint8_t))
#define RSA_IV_LENGTH		(16 * sizeof(uint8_t))
#define RSA_HMAC_LENGTH		(32 * sizeof(uint8_t))
#define RSA_MODULUS_LEN		(2048 * sizeof(uint8_t))
#define RSA_PUBLIC_EXPONENT	(0x10001)
#define RSA_PUB_EXP_SIZE_MAX	(5 * sizeof(uint8_t))
#define RSA_KEY_MATERIAL_SIZE	((528 + 2 + 5 + 1 + 528 + 2) * sizeof(uint8_t))
#define MAX_RSA_SIGN_DATA_SIZE	(2048 * sizeof(uint8_t))
#define RSA_PARAM_LEN		(5 * sizeof(uint8_t))

#define QSEE_LOG_BUF_SIZE		0x1000

static int app_state;
static int app_libs_state;
struct qseecom_props *props;

enum qti_crypto_service_aes_cmd_t {
	QTI_CRYPTO_SERVICE_AES_ENC_ID = 0x1,
	QTI_CRYPTO_SERVICE_AES_DEC_ID = 0x2,
};

enum qti_crypto_service_aes_type_t {
	QTI_CRYPTO_SERVICE_AES_TYPE_SHK = 1,
	QTI_CRYPTO_SERVICE_AES_TYPE_PHK,
	QTI_CRYPTO_SERVICE_AES_TYPE_MAX,
};

enum qti_crypto_service_aes_mode_t {
	QTI_CRYPTO_SERVICE_AES_MODE_ECB = 0,
	QTI_CRYPTO_SERVICE_AES_MODE_CBC,
	QTI_CRYPTO_SERVICE_AES_MODE_MAX,
};

enum qti_storage_service_cmd_t {
	QTI_STOR_SVC_GENERATE_KEY = 0x00000001,
	QTI_STOR_SVC_SEAL_DATA	 = 0x00000002,
	QTI_STOR_SVC_UNSEAL_DATA	 = 0x00000003,
	QTI_STOR_SVC_IMPORT_KEY	 = 0x00000004,
};

enum qti_storage_service_rsa_cmd_t {
	QTI_STOR_SVC_RSA_GENERATE_KEY      = 0x00000001,
	QTI_STOR_SVC_RSA_EXPORT_PUBLIC_KEY = 0x00000002,
	QTI_STOR_SVC_RSA_SIGN_DATA         = 0x00000003,
	QTI_STOR_SVC_RSA_VERIFY_SIGNATURE  = 0x00000004,
	QTI_STOR_SVC_RSA_IMPORT_KEY        = 0x00000005,
};

enum qti_storage_service_digest_pad_algo_t {
	QTI_STOR_SVC_RSA_DIGEST_PAD_NONE             = 0x00000000,
	QTI_STOR_SVC_RSA_DIGEST_PAD_PKCS115_SHA2_256 = 0x00000001,
	QTI_STOR_SVC_RSA_DIGEST_PAD_PSS_SHA2_256     = 0x00000002,
};

struct qti_storage_service_key_blob_t {
	uint64_t key_material;
	uint32_t key_material_len;
};

struct qti_storage_service_import_key_cmd_t {
	enum qti_storage_service_cmd_t cmd_id;
	struct qti_storage_service_key_blob_t key_blob;
	uint64_t input_key;
	uint32_t input_key_len;
};

struct qti_storage_service_gen_key_cmd_t {
	enum qti_storage_service_cmd_t cmd_id;
	struct qti_storage_service_key_blob_t key_blob;
};

struct qti_storage_service_gen_key_resp_t {
	enum qti_storage_service_cmd_t cmd_id;
	int32_t status;
	uint32_t key_blob_size;
};

struct qti_storage_service_seal_data_cmd_t {
	enum qti_storage_service_cmd_t cmd_id;
	struct qti_storage_service_key_blob_t key_blob;
	uint64_t plain_data;
	uint32_t plain_data_len;
	uint64_t output_buffer;
	uint32_t output_len;
};

struct qti_storage_service_seal_data_resp_t {
	enum qti_storage_service_cmd_t cmd_id;
	int32_t status;
	uint32_t sealed_data_len;
};

struct qti_storage_service_unseal_data_cmd_t {
	enum qti_storage_service_cmd_t cmd_id;
	struct qti_storage_service_key_blob_t key_blob;
	uint64_t sealed_data;
	uint32_t sealed_dlen;
	uint64_t output_buffer;
	uint32_t output_len;
};

struct qti_storage_service_unseal_data_resp_t {
	enum qti_storage_service_cmd_t cmd_id;
	int32_t status;
	uint32_t unsealed_data_len;
};

struct qti_crypto_service_encrypt_data_cmd_t {
	uint64_t  type;
	uint64_t mode;
	uint64_t plain_data;
	uint64_t plain_data_len;
	uint64_t iv;
	uint64_t iv_len;
	uint64_t output_buffer;
	uint64_t output_len;
};

struct qti_crypto_service_decrypt_data_cmd_t {
	uint64_t  type;
	uint64_t mode;
	uint64_t encrypted_data;
	uint64_t encrypted_dlen;
	uint64_t iv;
	uint64_t iv_len;
	uint64_t output_buffer;
	uint64_t output_len;
};

struct qti_storage_service_rsa_key_t {
	uint32_t magic_num;
	uint32_t version;
	enum qti_storage_service_digest_pad_algo_t pad_algo;
	uint8_t modulus[RSA_KEY_SIZE_MAX];
	uint32_t modulus_len;
	uint8_t public_exponent[RSA_KEY_SIZE_MAX];
	uint32_t public_exponent_len;
	uint8_t iv[RSA_IV_LENGTH];
	uint8_t pvt_exponent[RSA_KEY_SIZE_MAX];
	uint32_t pvt_exponent_len;
	uint8_t hmac[RSA_HMAC_LENGTH];
};

struct qti_storage_service_rsa_key_blob_t {
	uint64_t key_material;
	uint32_t key_material_len;
};

#define RSA_KEY_BLOB_SIZE sizeof(struct qti_storage_service_rsa_key_t)

struct qti_storage_service_rsa_keygen_params_t {
	uint32_t modulus_size;
	uint64_t public_exponent;
	enum qti_storage_service_digest_pad_algo_t pad_algo;
};

struct qti_storage_service_rsa_gen_key_cmd_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	struct qti_storage_service_rsa_key_blob_t key_blob;
	struct qti_storage_service_rsa_keygen_params_t rsa_params;
};

struct qti_storage_service_rsa_gen_key_resp_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	int32_t status;
	uint32_t key_blob_size;
};

struct qti_storage_service_rsa_import_key_cmd_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	uint8_t modulus[RSA_KEY_SIZE_MAX];
	uint32_t modulus_len;
	uint8_t public_exponent[RSA_PUB_EXP_SIZE_MAX];
	uint32_t public_exponent_len;
	uint8_t pvt_exponent[RSA_KEY_SIZE_MAX];
	uint32_t pvt_exponent_len;
	uint32_t digest_pad_type;
	struct qti_storage_service_rsa_key_blob_t key_blob;
};

struct qti_storage_service_rsa_import_key_resp_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	int32_t status;
};

struct qti_storage_service_rsa_sign_data_cmd_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	struct qti_storage_service_rsa_key_blob_t key_blob;
	uint64_t plain_data;
	uint32_t plain_data_len;
	uint64_t output_buffer;
	uint32_t output_len;
};

struct qti_storage_service_rsa_sign_data_resp_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	uint32_t sealed_data_len;
	int32_t status;
};

struct qti_storage_service_rsa_verify_data_cmd_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	struct qti_storage_service_rsa_key_blob_t key_blob;
	uint64_t data;
	uint32_t data_len;
	uint64_t signed_data;
	uint32_t signed_dlen;
};

struct qti_storage_service_rsa_verify_data_resp_t {
	enum qti_storage_service_rsa_cmd_t cmd_id;
	int32_t status;
};

struct qsee_64_send_cmd {
	uint32_t cmd_id;
	uint64_t data;
	uint64_t data2;
	uint32_t len;
	uint32_t start_pkt;
	uint32_t end_pkt;
	uint32_t test_buf_size;
};

struct qsee_send_cmd_rsp {
	uint32_t data;
	int32_t status;
};

enum qseecom_qceos_cmd_status {
	QSEOS_RESULT_SUCCESS	= 0,
	QSEOS_RESULT_INCOMPLETE,
	QSEOS_RESULT_FAILURE	= 0xFFFFFFFF
};

static uint32_t qsee_app_id;
static void *qsee_sbuffer;
static unsigned long basic_output;
static int basic_data_len;
static int mdt_size;
static int seg_size;
static uint8_t *mdt_file;
static uint8_t *seg_file;

static struct kobject *sec_kobj;
static uint8_t *key;
static size_t key_len;
static uint8_t *key_blob;
static size_t key_blob_len;
static uint8_t *sealed_buf;
static size_t seal_len;
static uint8_t *unsealed_buf;
static size_t unseal_len;
static uint64_t encrypted_len;
static uint64_t decrypted_len;
static uint8_t *ivdata;
static uint64_t type;
static uint64_t mode;
static uint64_t ivdata_len;

static struct kobject *rsa_sec_kobj;
static uint8_t *rsa_import_modulus;
static size_t rsa_import_modulus_len;
static uint8_t *rsa_import_public_exponent;
static size_t rsa_import_public_exponent_len;
static uint8_t *rsa_import_pvt_exponent;
static size_t rsa_import_pvt_exponent_len;
static uint8_t *rsa_key_blob;
static size_t rsa_key_blob_len;
static uint8_t *rsa_sign_data_buf;
static size_t rsa_sign_data_len;
static uint8_t *rsa_plain_data_buf;
static size_t rsa_plain_data_len;

struct kobject *qtiapp_kobj;
struct attribute_group qtiapp_attr_grp;

static struct qtidbg_log_t *g_qsee_log;

static struct device *qdev;

#define MUL		0x1
#define CRYPTO		0x8
#define AES_SEC_KEY	0x20
#define RSA_SEC_KEY	0x40

static ssize_t show_qsee_app_log_buf(struct device *dev,
				    struct device_attribute *attr, char *buf);

static ssize_t generate_key_blob(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t store_key(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count);

static ssize_t import_key_blob(struct device *dev,
			      struct device_attribute *attr, char *buf);

static ssize_t store_key_blob(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);

static ssize_t store_unsealed_data(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

static ssize_t show_sealed_data(struct device *dev,
			       struct device_attribute *attr, char *buf);

static ssize_t store_sealed_data(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

static ssize_t show_unsealed_data(struct device *dev,
				 struct device_attribute *attr, char *buf);

static ssize_t store_aes_type(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

static ssize_t store_aes_mode(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

static ssize_t store_iv_data(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

static ssize_t store_encrypted_data(struct device *dev,
                                struct device_attribute *attr,
                                const char *buf, size_t count);

static ssize_t show_encrypted_data(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t store_decrypted_data(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

static ssize_t show_decrypted_data(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t generate_rsa_key_blob(struct device *dev,
				    struct device_attribute *attr,
				    char *buf);

static ssize_t store_rsa_key(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count);

static ssize_t import_rsa_key_blob(struct device *dev,
				  struct device_attribute *attr, char *buf);

static ssize_t store_rsa_key_blob(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count);

static ssize_t store_rsa_plain_data(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count);

static ssize_t show_rsa_signed_data(struct device *dev,
				   struct device_attribute *attr, char *buf);

static ssize_t store_rsa_signed_data(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count);

static ssize_t verify_rsa_signed_data(struct device *dev,
				     struct device_attribute *attr, char *buf);

static ssize_t mdt_write(struct file *filp, struct kobject *kobj,
			struct bin_attribute *bin_attr,
			char *buf, loff_t pos, size_t count);

static ssize_t seg_write(struct file *filp, struct kobject *kobj,
			struct bin_attribute *bin_attr,
			char *buf, loff_t pos, size_t count);

static ssize_t store_load_start(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count);

static ssize_t show_basic_output(struct device *dev,
				struct device_attribute *attr, char *buf);

static ssize_t store_basic_input(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count);

static ssize_t store_crypto_input(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count);

static DEVICE_ATTR(log_buf, 0644, show_qsee_app_log_buf, NULL);
static DEVICE_ATTR(load_start, S_IWUSR, NULL, store_load_start);
static DEVICE_ATTR(basic_data, 0644, show_basic_output, store_basic_input);
static DEVICE_ATTR(crypto, 0644, NULL, store_crypto_input);

static DEVICE_ATTR(generate, 0644, generate_key_blob, NULL);
static DEVICE_ATTR(import, 0644, import_key_blob, store_key);
static DEVICE_ATTR(key_blob, 0644, NULL, store_key_blob);
static DEVICE_ATTR(seal, 0644, show_sealed_data, store_unsealed_data);
static DEVICE_ATTR(unseal, 0644, show_unsealed_data, store_sealed_data);
static DEVICE_ATTR(aes_encrypt, 0644, show_encrypted_data, store_decrypted_data);
static DEVICE_ATTR(aes_decrypt, 0644, show_decrypted_data, store_encrypted_data);
static DEVICE_ATTR(aes_ivdata, 0644, NULL, store_iv_data);
static DEVICE_ATTR(aes_type, 0644, NULL, store_aes_type);
static DEVICE_ATTR(aes_mode, 0644, NULL, store_aes_mode);

static DEVICE_ATTR(rsa_generate, 0644, generate_rsa_key_blob, NULL);
static DEVICE_ATTR(rsa_key_blob, 0644, NULL, store_rsa_key_blob);
static DEVICE_ATTR(rsa_import, 0644, import_rsa_key_blob, store_rsa_key);
static DEVICE_ATTR(rsa_sign, 0644, show_rsa_signed_data,
		   store_rsa_plain_data);
static DEVICE_ATTR(rsa_verify, 0644, verify_rsa_signed_data,
		   store_rsa_signed_data);

static struct attribute *sec_key_attrs[] = {
	&dev_attr_generate.attr,
	&dev_attr_import.attr,
	&dev_attr_key_blob.attr,
	&dev_attr_seal.attr,
	&dev_attr_unseal.attr,
	&dev_attr_aes_encrypt.attr,
	&dev_attr_aes_decrypt.attr,
	&dev_attr_aes_ivdata.attr,
	&dev_attr_aes_type.attr,
	&dev_attr_aes_mode.attr,
	NULL,
};

static struct attribute *rsa_sec_key_attrs[] = {
	&dev_attr_rsa_generate.attr,
	&dev_attr_rsa_key_blob.attr,
	&dev_attr_rsa_import.attr,
	&dev_attr_rsa_sign.attr,
	&dev_attr_rsa_verify.attr,
	NULL,
};

static struct attribute_group sec_key_attr_grp = {
	.attrs = sec_key_attrs,
};

static struct attribute_group rsa_sec_key_attr_grp = {
	.attrs = rsa_sec_key_attrs,
};

struct bin_attribute mdt_attr = {
	.attr = {.name = "mdt_file", .mode = 0666},
	.write = mdt_write,
};

struct bin_attribute seg_attr = {
	.attr = {.name = "seg_file", .mode = 0666},
	.write = seg_write,
};

#endif
