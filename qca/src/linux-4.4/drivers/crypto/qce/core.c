/*
 * Copyright (c) 2010-2014, The Linux Foundation. All rights reserved.
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

#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <crypto/algapi.h>
#include <crypto/internal/hash.h>
#include <crypto/sha.h>
#include <linux/qcom_scm.h>

#include <linux/debugfs.h>
#include "core.h"
#include "cipher.h"
#include "sha.h"

#define QCE_MAJOR_VERSION5	0x05
#define QCE_QUEUE_LENGTH	1

bool use_fixed_key = false;

#define to_qce_obj(x) container_of(x, struct qce_device, kobj)
#define to_qce_attr(x) container_of(x, struct qce_attribute, attr)

static const struct qce_algo_ops *qce_ops[] = {
	&ablkcipher_ops,
	&ahash_ops,
};

static void qce_unregister_algs(struct qce_device *qce)
{
	const struct qce_algo_ops *ops;
	int i;

	for (i = 0; i < ARRAY_SIZE(qce_ops); i++) {
		ops = qce_ops[i];
		ops->unregister_algs(qce);
	}
}

static int qce_register_algs(struct qce_device *qce)
{
	const struct qce_algo_ops *ops;
	int i, ret = -ENODEV;

	for (i = 0; i < ARRAY_SIZE(qce_ops); i++) {
		ops = qce_ops[i];
		ret = ops->register_algs(qce);
		if (ret)
			break;
	}

	return ret;
}

static int qce_handle_request(struct crypto_async_request *async_req)
{
	int ret = -EINVAL, i;
	const struct qce_algo_ops *ops;
	u32 type = crypto_tfm_alg_type(async_req->tfm);

	for (i = 0; i < ARRAY_SIZE(qce_ops); i++) {
		ops = qce_ops[i];
		if (type != ops->type)
			continue;
		ret = ops->async_req_handle(async_req);
		break;
	}

	return ret;
}

static int qce_handle_queue(struct qce_device *qce,
			    struct crypto_async_request *req)
{
	struct crypto_async_request *async_req, *backlog;
	unsigned long flags;
	int ret = 0, err;

	spin_lock_irqsave(&qce->lock, flags);

	if (req)
		ret = crypto_enqueue_request(&qce->queue, req);

	/* busy, do not dequeue request */
	if (qce->req) {
		spin_unlock_irqrestore(&qce->lock, flags);
		return ret;
	}

	backlog = crypto_get_backlog(&qce->queue);
	async_req = crypto_dequeue_request(&qce->queue);
	if (async_req)
		qce->req = async_req;

	spin_unlock_irqrestore(&qce->lock, flags);

	if (!async_req)
		return ret;

	if (backlog) {
		spin_lock_bh(&qce->lock);
		backlog->complete(backlog, -EINPROGRESS);
		spin_unlock_bh(&qce->lock);
	}

	err = qce_handle_request(async_req);
	if (err) {
		qce->result = err;
		tasklet_schedule(&qce->done_tasklet);
	}

	return ret;
}

static void qce_tasklet_req_done(unsigned long data)
{
	struct qce_device *qce = (struct qce_device *)data;
	struct crypto_async_request *req;
	unsigned long flags;

	spin_lock_irqsave(&qce->lock, flags);
	req = qce->req;
	qce->req = NULL;
	spin_unlock_irqrestore(&qce->lock, flags);

	if (req)
		req->complete(req, qce->result);

	qce_handle_queue(qce, NULL);
}

static int qce_async_request_enqueue(struct qce_device *qce,
				     struct crypto_async_request *req)
{
	struct qce_stat *pstat;
	const char *cra_drv_name;
	int ablk_flags = 0;
	struct qce_cipher_reqctx *rctx;

	pstat = &qce->qce_stat;
	if (req) {
		cra_drv_name = crypto_tfm_alg_driver_name(req->tfm);
		rctx = ablkcipher_request_ctx((void *)req);
		if (rctx)
			ablk_flags = rctx->flags;

		if (!strcmp(cra_drv_name, "sha1-qce"))
			pstat->sha1_digest++;
		else if (!strcmp(cra_drv_name, "sha256-qce"))
			pstat->sha256_digest++;
		else if (!strcmp(cra_drv_name, "hmac-sha256-qce"))
			pstat->sha256_hmac_digest++;
		else if (!strcmp(cra_drv_name, "hmac-sha1-qce"))
			pstat->sha1_hmac_digest++;
		else if (IS_AES(ablk_flags) && (ablk_flags & QCE_ENCRYPT))
			pstat->ablk_cipher_aes_enc++;
		else if (IS_AES(ablk_flags) && (ablk_flags & QCE_DECRYPT))
			pstat->ablk_cipher_aes_dec++;
		else if (IS_DES(ablk_flags) && (ablk_flags & QCE_ENCRYPT))
			pstat->ablk_cipher_des_enc++;
		else if (IS_DES(ablk_flags) && (ablk_flags & QCE_DECRYPT))
			pstat->ablk_cipher_des_dec++;
		else if (IS_3DES(ablk_flags) && (ablk_flags & QCE_ENCRYPT))
			pstat->ablk_cipher_3des_enc++;
		else if (IS_3DES(ablk_flags) && (ablk_flags & QCE_DECRYPT))
			pstat->ablk_cipher_3des_dec++;
	}

	return qce_handle_queue(qce, req);
}

static void qce_async_request_done(struct qce_device *qce, int ret)
{
	u32 type;
	struct qce_stat *pstat;

	qce->result = ret;
	pstat = &qce->qce_stat;
	if (qce->req) {
		type = crypto_tfm_alg_type(qce->req->tfm);

		if (ret && (type == CRYPTO_ALG_TYPE_AHASH))
			pstat->ahash_op_fail++;
		if (!ret && (type == CRYPTO_ALG_TYPE_AHASH))
			pstat->ahash_op_success++;

		if (ret && (type == CRYPTO_ALG_TYPE_ABLKCIPHER))
			pstat->ablk_cipher_op_fail++;
		if (!ret && (type == CRYPTO_ALG_TYPE_ABLKCIPHER))
			pstat->ablk_cipher_op_success++;
	}
	tasklet_schedule(&qce->done_tasklet);
}

static int qce_check_version(struct qce_device *qce)
{
	u32 major, minor, step;

	qce_get_version(qce, &major, &minor, &step);

	/*
	 * the driver does not support v5 with minor 0 because it has special
	 * alignment requirements.
	 */
	if (major != QCE_MAJOR_VERSION5 || minor == 0)
		return -ENODEV;

	qce->burst_size = QCE_BAM_BURST_SIZE;
	qce->pipe_pair_id = 1;

	dev_dbg(qce->dev, "Crypto device found, version %d.%d.%d\n",
		major, minor, step);

	return 0;
}

static int qce_disp_stats(struct qce_device *qce)
{
	struct qce_stat *pstat;
	char *read_buf;
	int len;

	pstat = &qce->qce_stat;
	read_buf = qce->qce_debug_read_buf;
	len = scnprintf(read_buf, DEBUG_MAX_RW_BUF - 1,
			"\nQualcomm crypto accelerator Statistics\n");

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER AES encryption          : %llu\n",
					pstat->ablk_cipher_aes_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER AES decryption          : %llu\n",
					pstat->ablk_cipher_aes_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER DES encryption          : %llu\n",
					pstat->ablk_cipher_des_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER DES decryption          : %llu\n",
					pstat->ablk_cipher_des_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER 3DES encryption         : %llu\n",
					pstat->ablk_cipher_3des_enc);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER 3DES decryption         : %llu\n",
					pstat->ablk_cipher_3des_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER operation success       : %llu\n",
					pstat->ablk_cipher_op_success);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   ABLK CIPHER operation fail          : %llu\n",
					pstat->ablk_cipher_op_fail);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"\n");

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA1-AES encryption            : %llu\n",
					pstat->aead_sha1_aes_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA1-AES decryption            : %llu\n",
					pstat->aead_sha1_aes_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA1-DES encryption            : %llu\n",
					pstat->aead_sha1_des_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA1-DES decryption            : %llu\n",
					pstat->aead_sha1_des_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA1-3DES encryption           : %llu\n",
					pstat->aead_sha1_3des_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA1-3DES decryption           : %llu\n",
					pstat->aead_sha1_3des_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA256-AES encryption          : %llu\n",
					pstat->aead_sha256_aes_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA256-AES decryption          : %llu\n",
					pstat->aead_sha256_aes_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA256-DES encryption          : %llu\n",
					pstat->aead_sha256_des_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA256-DES decryption          : %llu\n",
					pstat->aead_sha256_des_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA256-3DES encryption         : %llu\n",
					pstat->aead_sha256_3des_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD SHA256-3DES decryption         : %llu\n",
					pstat->aead_sha256_3des_dec);

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD CCM-AES encryption             : %llu\n",
					pstat->aead_ccm_aes_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD CCM-AES decryption             : %llu\n",
					pstat->aead_ccm_aes_dec);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD RFC4309-CCM-AES encryption     : %llu\n",
					pstat->aead_rfc4309_ccm_aes_enc);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD RFC4309-CCM-AES decryption     : %llu\n",
					pstat->aead_rfc4309_ccm_aes_dec);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD operation success              : %llu\n",
					pstat->aead_op_success);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD operation fail                 : %llu\n",
					pstat->aead_op_fail);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AEAD bad message                    : %llu\n",
					pstat->aead_bad_msg);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"\n");

	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AHASH SHA1 digest                   : %llu\n",
					pstat->sha1_digest);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AHASH SHA256 digest                 : %llu\n",
					pstat->sha256_digest);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AHASH SHA1 HMAC digest              : %llu\n",
					pstat->sha1_hmac_digest);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AHASH SHA256 HMAC digest            : %llu\n",
					pstat->sha256_hmac_digest);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AHASH operation success             : %llu\n",
					pstat->ahash_op_success);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"   AHASH operation fail                : %llu\n",
					pstat->ahash_op_fail);
	len += scnprintf(read_buf + len, DEBUG_MAX_RW_BUF - len - 1,
			"\n");

	return len;
}

static int qce_debug_stats_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t qce_debug_stats_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	struct qce_device *qce = (struct qce_device *)file->private_data;
	int len;

	if (!qce)
		return 0;
	len = qce_disp_stats(qce);

	return simple_read_from_buffer((void __user *)buf, count,
			ppos, (void *)qce->qce_debug_read_buf, len);
}

static ssize_t qce_debug_stats_write(struct file *file, const char __user *buf,
			size_t count, loff_t *ppos)
{
	struct qce_device *qce = (struct qce_device *)file->private_data;

	if (!qce)
		return 0;
	memset((char *)&qce->qce_stat, 0, sizeof(struct qce_stat));
	return count;
}

static const struct file_operations qce_debug_stats_ops = {
	.open =         qce_debug_stats_open,
	.read =         qce_debug_stats_read,
	.write =        qce_debug_stats_write,
};

static int qce_debug_init(struct qce_device *qce)
{
	int rc;
	char name[DEBUG_MAX_FNAME];
	struct dentry *qce_dent;
	struct dentry *stats_dent;

	qce_dent = qce->qce_debug_dent;
	qce_dent = debugfs_create_dir("qce", NULL);
	if (IS_ERR(qce_dent)) {
		pr_err("qce debugfs_create_dir fail, error %ld\n",
				PTR_ERR(qce_dent));
		return PTR_ERR(qce_dent);
	}

	snprintf(name, DEBUG_MAX_FNAME - 1, "stats");
	stats_dent = debugfs_create_file(name, 0644, qce_dent,
					qce, &qce_debug_stats_ops);
	if (stats_dent == NULL) {
		pr_err("qce debugfs_create_file fail, error %ld\n",
				PTR_ERR(stats_dent));
		rc = PTR_ERR(stats_dent);
		goto err;
	}
	qce->qce_debug_dent = qce_dent;
	return 0;
err:
	debugfs_remove_recursive(qce_dent);
	return rc;
}

/* a custom attribute that works just for a struct qce_sysfs*/
struct qce_attribute {
	struct attribute attr;
	ssize_t (*show)(struct qce_device *qce, struct qce_attribute *attr,
				char *buf);
	ssize_t (*store)(struct qce_device *qce, struct qce_attribute *attr,
				const char *buf, size_t count);
};

/*
 * The default show function that must be passed to sysfs.  This will be
 * called by sysfs for whenever a show function is called by the user on a
 * sysfs file associated with the kobjects we have registered.  We need to
 * transpose back from a "default" kobject to our custom struct qce_sysfs and
 * then call the show function for that specific object.
 */
static ssize_t qce_attr_show(struct kobject *kobj, struct attribute *attr,
				char *buf)
{
	struct qce_attribute *attribute;
	struct qce_device *qce;
	attribute = to_qce_attr(attr);
	qce = to_qce_obj(kobj);
	if (!attribute->show)
		return -EIO;
	return attribute->show(qce, attribute, buf);
}

/*
 * Just like the default show function above, but this one is for when the
 * sysfs "store" is requested (when a value is written to a file.
 */
static ssize_t qce_attr_store(struct kobject *kobj,struct attribute *attr,
				const char *buf, size_t len)
{
	struct qce_attribute *attribute;
	struct qce_device *qce;
	attribute = to_qce_attr(attr);
	qce = to_qce_obj(kobj);
	if (!attribute->store)
		return -EIO;
	return attribute->store(qce, attribute, buf, len);
}

/*Our custom sysfs_ops that we will associate with our ktype later on*/
static struct sysfs_ops qce_sysfs_ops = {
	.show = qce_attr_show,
	.store = qce_attr_store,
};

/* The "fixed_sec_key" file where the .fixed_key variable is read from
 * and written to
 */
static ssize_t qce_show(struct qce_device *qce, struct qce_attribute *attr,
				char *buf)
{
	return snprintf(buf, sizeof(int), "%d\n", qce->fixed_key);
}

static ssize_t qce_store(struct qce_device *qce, struct qce_attribute *attr,
				const char *buf, size_t count)
{
	sscanf(buf, "%du", &qce->fixed_key);
	if (qce->fixed_key == 1)
		use_fixed_key = true;
	else if (qce->fixed_key == 0) {
		if (qce_sec_release_xpu_prot())
			pr_err("qce XPU proctection release fail\n");
		use_fixed_key = false;
	}

	return count;
}

static struct qce_attribute qce_attribute = __ATTR(fixed_sec_key, 0644, qce_show,
							qce_store);

/*
 * Create a group of attributes so that we can create and destory them all
 * at once.
 */
static struct attribute *qce_default_attrs[] = {
	&qce_attribute.attr,
	NULL,
};

/*
 * Our own ktype for our kobjects.  Here we specify our sysfs ops, the
 * release function, and the set of default attributes we want created
 * whenever a kobject of this type is registered with the kernel.
 */

static struct kobj_type qce_ktype = {
	.sysfs_ops = &qce_sysfs_ops,
	.default_attrs = qce_default_attrs,
};

static int create_qce_obj(struct qce_device *qce, char *name)
{
	int ret;

	qce->fixed_key = 0;

	/* As we have a kset for this kobject, we need to set
	 * it before calling the kobject core.
	 */
	qce->kobj.kset = qce->parent_kset;

	/*
	 * Initialize and add the kobject to the kernel.  All the default files
	 * will be created here.  As we have already specified a kset for this
	 * kobject, we don't have to set a parent for the kobject, the kobject
	 * will be placed beneath that kset automatically.
	 */
	ret = kobject_init_and_add(&qce->kobj, &qce_ktype, NULL, "%s", name);
	if (ret)
		return -EINVAL;

	return 0;
}

static int qce_sysfs_init(struct qce_device *qce)
{
	int ret;
	/*
	 * Create a simple kset with the name of "crypto",
	 * located under /sys/kernel/
	 */
	qce->parent_kset = kset_create_and_add("crypto", NULL, kernel_kobj);
	if (!qce->parent_kset) {
		pr_err("Error in creating sys entry crypto\n");
		return -ENOMEM;
	}
	/* Create an object and register it with our kset*/
	ret = create_qce_obj(qce, "qce");
	if(ret)
		return -EINVAL;

	return 0;
}

static int qce_crypto_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct qce_device *qce;
	struct resource *res;
	int ret;

	qce = devm_kzalloc(dev, sizeof(*qce), GFP_KERNEL);
	if (!qce)
		return -ENOMEM;

	qce->dev = dev;
	platform_set_drvdata(pdev, qce);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENOMEM;

	qce->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(qce->base))
		return PTR_ERR(qce->base);

	ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (ret < 0)
		return ret;

	if (device_property_read_bool(dev, "qce,use_fixed_hw_key"))
		use_fixed_key = true;

	if (device_property_read_bool(dev, "qce,cmd_desc_support")) {
		qce->qce_cmd_desc_enable = true;
		qce->base_dma = phys_to_dma(dev, (phys_addr_t)res->start);
	}

	qce->core = devm_clk_get(qce->dev, "core");
	if (IS_ERR(qce->core))
		return PTR_ERR(qce->core);

	qce->iface = devm_clk_get(qce->dev, "iface");
	if (IS_ERR(qce->iface))
		return PTR_ERR(qce->iface);

	qce->bus = devm_clk_get(qce->dev, "bus");
	if (IS_ERR(qce->bus))
		return PTR_ERR(qce->bus);

	ret = clk_prepare_enable(qce->core);
	if (ret)
		return ret;

	ret = clk_prepare_enable(qce->iface);
	if (ret)
		goto err_clks_core;

	ret = clk_prepare_enable(qce->bus);
	if (ret)
		goto err_clks_iface;

	ret = qce_dma_request(qce->dev, &qce->dma);
	if (ret)
		goto err_clks;

	ret = qce_check_version(qce);
	if (ret)
		goto err_clks;

	spin_lock_init(&qce->lock);
	tasklet_init(&qce->done_tasklet, qce_tasklet_req_done,
		     (unsigned long)qce);
	crypto_init_queue(&qce->queue, QCE_QUEUE_LENGTH);

	qce->async_req_enqueue = qce_async_request_enqueue;
	qce->async_req_done = qce_async_request_done;

	ret = qce_register_algs(qce);
	if (ret)
		goto err_dma;

	ret = qce_debug_init(qce);
	if (ret)
		goto err_dma;

	ret = qce_sysfs_init(qce);
	if (ret)
		goto err_dma;
	return 0;

err_dma:
	qce_dma_release(&qce->dma);
err_clks:
	clk_disable_unprepare(qce->bus);
err_clks_iface:
	clk_disable_unprepare(qce->iface);
err_clks_core:
	clk_disable_unprepare(qce->core);
	return ret;
}

static int qce_crypto_remove(struct platform_device *pdev)
{
	struct qce_device *qce = platform_get_drvdata(pdev);

	tasklet_kill(&qce->done_tasklet);
	qce_unregister_algs(qce);
	qce_dma_release(&qce->dma);
	clk_disable_unprepare(qce->bus);
	clk_disable_unprepare(qce->iface);
	clk_disable_unprepare(qce->core);
	debugfs_remove_recursive(qce->qce_debug_dent);
	kobject_put(&qce->kobj);
	kset_unregister(qce->parent_kset);
	return 0;
}

static const struct of_device_id qce_crypto_of_match[] = {
	{ .compatible = "qcom,crypto-v5.1", },
	{}
};
MODULE_DEVICE_TABLE(of, qce_crypto_of_match);

static struct platform_driver qce_crypto_driver = {
	.probe = qce_crypto_probe,
	.remove = qce_crypto_remove,
	.driver = {
		.name = KBUILD_MODNAME,
		.of_match_table = qce_crypto_of_match,
	},
};
module_platform_driver(qce_crypto_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Qualcomm crypto engine driver");
MODULE_ALIAS("platform:" KBUILD_MODNAME);
MODULE_AUTHOR("The Linux Foundation");
