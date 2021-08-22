/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_wakeup.h>
#include <linux/rwsem.h>
#include <linux/suspend.h>
#include <soc/qcom/ramdump.h>
#include <soc/qcom/subsystem_notif.h>
#include <soc/qcom/subsystem_restart.h>
#include <linux/platform_device.h>
#include <linux/remoteproc.h>
#include <linux/debugfs.h>

/*
 * To call subsystem get & put for WCSS alone,
 * 1) insmod testssr.ko (test_id by default is set to 1)
 * 2) rmmod testssr
 * To call subsystem get & put for LPASS alone,
 * 1) insmod testssr.ko test_id=2
 * 2) rmmod testssr
 * To call subsystem get & put for both WCSS and LPASS,
 * 1) insmod testssr.ko test_id=3
 * 2) rmmod testssr
 * Same as default case, just directly using rproc APIs
 * used for open source/profile
 * 1)insmod testssr.ko test_id=4
 * 2)rmmod testssr.ko
 */

static void *wcss_notif_handle;
static void *wcss_subsys_handle;
static void *adsp_notif_handle;
static void *adsp_subsys_handle;
static void *subsys_handle;

static struct notifier_block nb;
static struct notifier_block atomic_nb;

#define TEST_SSR_WCSS "qcom_q6v5_wcss"
#define TEST_SSR_LPASS "q6v6-adsp"
#define WCSS_RPROC	"cd00000.qcom_q6v5_wcss"
static int test_id = 1;
module_param(test_id, int, S_IRUGO | S_IWUSR | S_IWGRP);
static ulong  user_pd;
module_param(user_pd, ulong, S_IRUGO | S_IWUSR | S_IWGRP);

#define MAX_USERPD_CNT	3

struct userpd_info {
	char name[20];
	struct rproc *rproc;
	/*0-stop, 1-start*/
	bool pd_curr_state;
	int pd_id;
	void *subsys_hdl;
	void *notif_hdl;
	struct notifier_block nb;
	struct notifier_block atomic_nb;
};

struct userpd_dbgfs_info {
	struct dentry *userpd;
	/*0-stop, 1-start*/
	struct dentry *start_stop;
	struct userpd_info userpd_hdl;
};

static struct dentry *q6_userpd_debug;
static struct userpd_dbgfs_info *userpd_dbgfs_hdl[MAX_USERPD_CNT];

static const char *action_to_string(enum subsys_notif_type action)
{
	switch (action) {

	case	SUBSYS_BEFORE_SHUTDOWN:
		return __stringify(SUBSYS_BEFORE_SHUTDOWN);

	case	SUBSYS_AFTER_SHUTDOWN:
		return __stringify(SUBSYS_AFTER_SHUTDOWN);

	case	SUBSYS_BEFORE_POWERUP:
		return __stringify(SUBSYS_BEFORE_POWERUP);

	case	SUBSYS_AFTER_POWERUP:
		return __stringify(SUBSYS_AFTER_POWERUP);

	case	SUBSYS_RAMDUMP_NOTIFICATION:
		return __stringify(SUBSYS_RAMDUMP_NOTIFICATION);

	case	SUBSYS_POWERUP_FAILURE:
		return __stringify(SUBSYS_POWERUP_FAILURE);

	case	SUBSYS_PROXY_VOTE:
		return __stringify(SUBSYS_PROXY_VOTE);

	case	SUBSYS_PROXY_UNVOTE:
		return __stringify(SUBSYS_PROXY_UNVOTE);

	case	SUBSYS_SOC_RESET:
		return __stringify(SUBSYS_SOC_RESET);

	case	SUBSYS_PREPARE_FOR_FATAL_SHUTDOWN:
		return __stringify(SUBSYS_PREPARE_FOR_FATAL_SHUTDOWN);

	default:
		return "unknown";
	}
}

static int tssr_notifier(struct notifier_block *nb, unsigned long action,
								void *data)
{
	struct notif_data *notif = (struct notif_data *)data;

	pr_emerg("%s:\tReceived [%s] notification from [%s]subsystem\n",
		THIS_MODULE->name, action_to_string(action), notif->pdev->name);

	return NOTIFY_DONE;
}

static void *test_subsys_notif_register(const char *subsys_name)
{
	int ret;

	subsys_handle = subsys_notif_register_notifier(subsys_name, &nb);
	if (IS_ERR_OR_NULL(subsys_handle)) {
		pr_emerg("Subsystem notif reg failed\n");
		return NULL;
	}

	ret = subsys_notif_register_atomic_notifier(subsys_handle, &atomic_nb);
	if (ret < 0) {
		pr_emerg("Subsystem atomic notif reg failed\n");

		ret = subsys_notif_unregister_notifier(subsys_handle, &nb);
		if (ret < 0)
			pr_emerg("Can't unregister subsys notifier\n");

		return NULL;
	}
	return subsys_handle;
}

static void *test_subsystem_get(const char *subsys_name)
{
	int ret;

	pr_info("%s: invoking subsystem_get for %s\n", __func__, subsys_name);
	subsys_handle = subsystem_get(subsys_name);
	if (IS_ERR_OR_NULL(subsys_handle)) {
		pr_emerg("Subsystem get failed\n");

		subsys_handle = subsys_notif_add_subsys(subsys_name);
		if (IS_ERR_OR_NULL(subsys_handle)) {
			pr_err("Subsystem not found\n");
			return subsys_handle;
		}

		ret = subsys_notif_unregister_notifier(subsys_handle, &nb);
		if (ret < 0)
			pr_emerg("Can't unregister subsys notifier\n");

		ret = subsys_notif_unregister_atomic_notifier(subsys_handle,
								&atomic_nb);
		if (ret < 0)
			pr_emerg("Can't unregister subsys notifier\n");

		return NULL;
	}
	return subsys_handle;
}

#if defined(CONFIG_QCOM_Q6V5_WCSS)
static struct rproc * get_rproc_from_phandle(void) {
		struct device_node *of_np;
		phandle rproc_phandle;
		struct rproc *q6rproc;

       of_np = of_find_node_with_property(NULL, "qcom,rproc");
		if (!of_np) {
			pr_err("no node with qcom,rproc NULLi\n");
			return NULL;
		}
		if (of_property_read_u32(of_np, "qcom,rproc", &rproc_phandle)) {
			printk("could not get rproc phandle\n");
			return  NULL;
		}
		q6rproc  = rproc_get_by_phandle(rproc_phandle);
		if(!q6rproc) {
			pr_err("could not get the rproc handle\n");
			return NULL;
		}
		return q6rproc;
}
#endif

static ssize_t show_start_stop(struct file *file, char __user *user_buf,
			size_t count, loff_t *ppos)
{
	struct userpd_dbgfs_info *hdl = file->private_data;
	char _buf[16] = {0};
	int ret;

	snprintf(_buf, sizeof(_buf), "%d\n", hdl->userpd_hdl.pd_curr_state);
	ret = simple_read_from_buffer(user_buf, count, ppos, _buf,
						strnlen(_buf, 16));
	return ret;
}

static ssize_t store_start_stop(struct file *file, const char __user *user_buf,
			size_t count, loff_t *ppos)
{
	struct userpd_dbgfs_info *hdl = file->private_data;
	bool state;
	int ret;

	hdl->userpd_hdl.nb.notifier_call = tssr_notifier;
	hdl->userpd_hdl.atomic_nb.notifier_call = tssr_notifier;

	ret = kstrtobool_from_user(user_buf, count, &state);
	if (ret) {
		pr_err("Failed to retrieve userbuf value\n");
		return ret;
	}

	if (state == hdl->userpd_hdl.pd_curr_state)
		return -EINVAL;

	if (state) {
		if (test_id == 1) { /*subsys framework*/
			/*Register notifier*/
			hdl->userpd_hdl.notif_hdl =
				subsys_notif_register_notifier(
						hdl->userpd_hdl.name,
						&hdl->userpd_hdl.nb);
			if (IS_ERR_OR_NULL(hdl->userpd_hdl.notif_hdl)) {
				pr_emerg("Subsystem notif reg failed\n");
				return PTR_ERR(hdl->userpd_hdl.notif_hdl);
			}
			/*Register atomic notifier*/
			ret = subsys_notif_register_atomic_notifier(
					hdl->userpd_hdl.notif_hdl,
					&hdl->userpd_hdl.atomic_nb);
			if (ret < 0) {
				pr_emerg("Subsystem %s atomic notif reg failed\n",
						hdl->userpd_hdl.name);
				ret = subsys_notif_unregister_notifier(
						hdl->userpd_hdl.notif_hdl,
						&hdl->userpd_hdl.nb);
				if (ret < 0)
					pr_emerg("Can't unregister subsys notifier\n");

				return ret;
			}

			hdl->userpd_hdl.subsys_hdl =
				subsystem_get(hdl->userpd_hdl.name);
			if (IS_ERR_OR_NULL(hdl->userpd_hdl.subsys_hdl)) {
				pr_err("could not get %s subsys handle\n",
						hdl->userpd_hdl.name);
				return PTR_ERR(hdl->userpd_hdl.subsys_hdl);
			}
		} else if (test_id == 4) { /*rproc framework*/
			hdl->userpd_hdl.rproc =
				rproc_get_by_name(hdl->userpd_hdl.name);
			if (IS_ERR_OR_NULL(hdl->userpd_hdl.rproc)) {
				pr_err("could not get %s rproc..\n",
						hdl->userpd_hdl.name);
				return PTR_ERR(hdl->userpd_hdl.rproc);
			}
			ret = rproc_boot(hdl->userpd_hdl.rproc);
			if (ret) {
				pr_err("couldn't boot q6v5: %d\n", ret);
				return ret;
			}
		}
		/*userpd started*/
		hdl->userpd_hdl.pd_curr_state = true;
	} else {
		if (test_id == 1) /*subsys framework*/ {
			subsystem_put(hdl->userpd_hdl.subsys_hdl);
			subsys_notif_unregister_notifier(
					hdl->userpd_hdl.notif_hdl,
					&hdl->userpd_hdl.nb);
			subsys_notif_unregister_atomic_notifier(
					hdl->userpd_hdl.notif_hdl,
					&hdl->userpd_hdl.atomic_nb);
		} else if (test_id == 4) /*rproc framework*/
			rproc_shutdown(hdl->userpd_hdl.rproc);
		/*userpd stopped*/
		hdl->userpd_hdl.pd_curr_state = false;
	}
	return count;
}

static const struct file_operations userpd_ops = {
	.open = simple_open,
	.write = store_start_stop,
	.read = show_start_stop,
};

static int create_userpd_debugfs(void)
{
	int ret, cnt, tmp;
	char name[20];

	/*Get userpd count*/
	cnt = rproc_get_child_cnt(WCSS_RPROC);
	if (cnt > MAX_USERPD_CNT) {
		pr_err("Current implementation don't support %d userpd's\n",
			cnt);
		return -EINVAL;
	}

	if (!cnt) {
		pr_err("No userpd is registered\n");
		return -EINVAL;
	}

	/*create a sysfs entry to start/stop registered user pd's*/
	q6_userpd_debug = debugfs_create_dir("q6v5_userpd_debug", NULL);
	if (IS_ERR(q6_userpd_debug)) {
		pr_err("Failed to create q6v5 userpd debug directory\n");
		return PTR_ERR(q6_userpd_debug);
	}

	for (tmp = 0; tmp < cnt; tmp++) {
		userpd_dbgfs_hdl[tmp] = kzalloc(sizeof(*userpd_dbgfs_hdl[tmp]),
					GFP_KERNEL);
		if (!userpd_dbgfs_hdl[tmp]) {
			pr_err("Failed to allocate memory\n");
			ret = PTR_ERR(userpd_dbgfs_hdl[tmp]);
			goto err_free_mem;
		}
		snprintf(name, sizeof(name), "q6v5_wcss_userpd%d", (tmp + 1));

		/*sysfs entry for each userpd*/
		userpd_dbgfs_hdl[tmp]->userpd = debugfs_create_dir(name,
							q6_userpd_debug);
		if (IS_ERR(userpd_dbgfs_hdl[tmp]->userpd)) {
			pr_err("Failed to create q6v5 userpd%d debug directory\n",
					(tmp + 1));
			ret = PTR_ERR(userpd_dbgfs_hdl[tmp]->userpd);
			goto err_release_userpd;
		}
		userpd_dbgfs_hdl[tmp]->userpd_hdl.pd_id = (tmp + 1);
		strlcpy(userpd_dbgfs_hdl[tmp]->userpd_hdl.name, name,
			sizeof(userpd_dbgfs_hdl[tmp]->userpd_hdl.name));

		/*sysfs entry for userpd start/stop*/
		userpd_dbgfs_hdl[tmp]->start_stop =
			debugfs_create_file("start_stop",
					0600, userpd_dbgfs_hdl[tmp]->userpd,
					userpd_dbgfs_hdl[tmp],
					&userpd_ops);
		if (IS_ERR(userpd_dbgfs_hdl[tmp]->start_stop)) {
			pr_err("Failed to create q6v5\
				userpd%d start/stop\n", (tmp + 1));
			ret = PTR_ERR(userpd_dbgfs_hdl[tmp]->start_stop);
			goto err_release_userpd_start_stop;
		}
	}
	return 0;

	for (; tmp >= 0; tmp--) {
err_release_userpd_start_stop:
		debugfs_remove(userpd_dbgfs_hdl[tmp]->start_stop);
err_release_userpd:
		debugfs_remove(userpd_dbgfs_hdl[tmp]->userpd);
err_free_mem:
		kfree(userpd_dbgfs_hdl[tmp]);
	}
	debugfs_remove(q6_userpd_debug);
	return ret;
}

static int remove_userpd_debugfs(void)
{
	int tmp, cnt;

	/*Get userpd count*/
	cnt = rproc_get_child_cnt(WCSS_RPROC);
	if (cnt > MAX_USERPD_CNT) {
		pr_err("Current implementation don't support %d userpd's\n",
			cnt);
		return -EINVAL;
	}

	if (!cnt) {
		pr_err("No userpd is registered\n");
		return -EINVAL;
	}

	/*shutdown any userpd's subsequently rootpd
	* also will be shutted down
	*/
	for (tmp = 0; tmp < cnt; tmp++) {
		if (userpd_dbgfs_hdl[tmp]->userpd_hdl.pd_curr_state) {
			if (test_id == 1) {
				subsystem_put(userpd_dbgfs_hdl[tmp]->
						userpd_hdl.subsys_hdl);
				subsys_notif_unregister_notifier(
					userpd_dbgfs_hdl[tmp]->userpd_hdl.notif_hdl,
					&userpd_dbgfs_hdl[tmp]->userpd_hdl.nb);
				subsys_notif_unregister_atomic_notifier(
					userpd_dbgfs_hdl[tmp]->userpd_hdl.notif_hdl,
					&userpd_dbgfs_hdl[tmp]->userpd_hdl.atomic_nb);
			} else if (test_id == 4)
				rproc_shutdown(userpd_dbgfs_hdl[tmp]->
					userpd_hdl.rproc);
		}
	}

	for (tmp = 0; tmp < cnt; tmp++) {
		debugfs_remove(userpd_dbgfs_hdl[tmp]->start_stop);
		debugfs_remove(userpd_dbgfs_hdl[tmp]->userpd);
		kfree(userpd_dbgfs_hdl[tmp]);
	}
	debugfs_remove(q6_userpd_debug);
	return 0;
}

static int __init testssr_init(void)
{
#if defined(CONFIG_QCOM_Q6V5_WCSS)
	struct rproc *q6rproc;
	int ret;
#endif
	nb.notifier_call = tssr_notifier;
	atomic_nb.notifier_call = tssr_notifier;

	switch (test_id) {
	case 1:
		if (user_pd) {
			ret = create_userpd_debugfs();
			if (ret) {
				pr_err("Failed to create sysfs entry for\
						userpd\n");
				return ret;
			}
			return 0;
		}
		wcss_notif_handle = test_subsys_notif_register(TEST_SSR_WCSS);
		if (!wcss_notif_handle)
			goto err;
		wcss_subsys_handle = test_subsystem_get(TEST_SSR_WCSS);
		if (!wcss_subsys_handle)
			goto err;
		break;
	case 2:
		adsp_notif_handle = test_subsys_notif_register(TEST_SSR_LPASS);
		if (!adsp_notif_handle)
			goto err;
		adsp_subsys_handle = test_subsystem_get(TEST_SSR_LPASS);
		if (!adsp_subsys_handle)
			goto err;
		break;
	case 3:
		wcss_notif_handle = test_subsys_notif_register(TEST_SSR_WCSS);
		if (!wcss_notif_handle)
			goto err;
		wcss_subsys_handle = test_subsystem_get(TEST_SSR_WCSS);
		if (!wcss_subsys_handle)
			goto err;
		adsp_notif_handle = test_subsys_notif_register(TEST_SSR_LPASS);
		if (!adsp_notif_handle)
			goto err;
		adsp_subsys_handle = test_subsystem_get(TEST_SSR_LPASS);
		if (!adsp_subsys_handle)
			goto err;
		break;
#if defined(CONFIG_QCOM_Q6V5_WCSS)
	case 4:
		if (user_pd) {
#ifndef CONFIG_CNSS2
			ret = create_userpd_debugfs();
			if (ret) {
				pr_err("Failed to create sysfs entry for\
						userpd\n");
				return ret;
			}
#endif
			return 0;
		}
		q6rproc = get_rproc_from_phandle();
		if(!q6rproc) {
			pr_err("could not get rproc..\n");
			return -ENODEV;
		}
		ret = rproc_boot(q6rproc);
		if (ret) {
			pr_err("couldn't boot q6v5: %d\n", ret);
			return ret;
		}
		break;
#endif
	default:
		pr_err("Enter a valid test case id\n");
	}

	return 0;
err:
	return -EIO;
}

static void wcss_test_exit(void)
{
	subsystem_put(wcss_subsys_handle);
	subsys_notif_unregister_notifier(wcss_notif_handle, &nb);
	subsys_notif_unregister_atomic_notifier(wcss_notif_handle,
			&atomic_nb);
}

static void adsp_test_exit(void)
{
	subsystem_put(adsp_subsys_handle);
	subsys_notif_unregister_notifier(adsp_notif_handle, &nb);
	subsys_notif_unregister_atomic_notifier(adsp_notif_handle,
			&atomic_nb);
}

static void __exit testssr_exit(void)
{
#if defined(CONFIG_QCOM_Q6V5_WCSS)
	struct rproc *q6rproc;
#endif
	switch (test_id) {
	case 1:
		if (user_pd)
			remove_userpd_debugfs();
		else
			wcss_test_exit();
		break;
	case 2:
		adsp_test_exit();
		break;
	case 3:
		wcss_test_exit();
		adsp_test_exit();
		break;
#if defined(CONFIG_QCOM_Q6V5_WCSS)
	case 4:
		if (user_pd) {
#ifndef CONFIG_CNSS2
			remove_userpd_debugfs();
#endif
			return;
		}
		q6rproc = get_rproc_from_phandle();
		if (!q6rproc) {
			pr_err("could not get rproc..\n");
				return;
		}
		rproc_shutdown(q6rproc);
		break;
#endif
	default:
		break;
	}
}

module_init(testssr_init);
module_exit(testssr_exit);
MODULE_LICENSE("Dual BSD/GPL");
