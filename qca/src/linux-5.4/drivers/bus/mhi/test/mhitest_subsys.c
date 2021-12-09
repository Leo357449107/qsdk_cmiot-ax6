/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include "commonmhitest.h"
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/sched.h>

#define DEFAULT_FW_FILE_NAME	"qcn9000/amss.bin"

int mhitest_ss_powerup(struct rproc *subsys_desc)
{
	int ret;
	struct mhitest_platform *temp;

	MHITEST_LOG("Enter\n");

	temp = subsys_desc->priv;
	if (!temp) {
		MHITEST_ERR("Error not dev data\n");
		return -EINVAL;
	}
	MHITEST_VERB("temp:[%p]\n", temp);

	if (!temp->pci_dev) {
		pr_mhitest2("temp->pci_Dev is NULL\n");
		return -ENODEV;
	}
	snprintf(temp->fw_name, sizeof(temp->fw_name),
						DEFAULT_FW_FILE_NAME);

	ret = mhitest_prepare_pci_mhi_msi(temp);
	if (ret) {
		MHITEST_ERR("Error prep. pci_mhi_msi  ret:%d\n", ret);
		return ret;
	}

	ret = mhitest_prepare_start_mhi(temp);
	if (ret) {
		MHITEST_ERR("Error preapare start mhi  ret:%d\n", ret);
		return ret;
	}

	MHITEST_LOG("Exit\n");

	return 0;
}

int mhitest_ss_shutdown(struct rproc *subsys_desc)
{

	struct mhitest_platform *mplat;

	MHITEST_VERB("Going for shutdown\n");

	mplat = subsys_desc->priv;

	mhitest_pci_set_mhi_state(mplat, MHI_POWER_OFF);
	mhitest_pci_set_mhi_state(mplat, MHI_DEINIT);
	mhitest_pci_remove_all(mplat);

	return 0;
}

void mhitest_ss_crash_shutdown(struct rproc *subsys_desc)
{
	int crash_d_instance;
	int ret;
	struct mhitest_platform *temp;

	if (!strncmp(subsys_desc->name, "mhitest-ss-0",
		     strlen("mhitest-ss-0"))) {
		crash_d_instance = 0;
	} else if (!strncmp(subsys_desc->name, "mhitest-ss-1",
			  strlen("mhitest-ss-1"))) {
		crash_d_instance = 1;
	} else {
		MHITEST_ERR("Error: subsys desc name: %s is not matching with any subsystem\n",
			    subsys_desc->name);
		return;
	}

	temp = get_mhitest_mplat(crash_d_instance);

	MHITEST_LOG("Going for shutdown temp:%p\n", temp);

	if (!temp)
		return;

	if ((strcmp(temp->mhitest_ss_desc_name, subsys_desc->name))) {
		MHITEST_ERR("Error: not the same subsystem\n");
		return;
	}

	ret = mhitest_dump_info(temp, true);
	if (ret) {
		MHITEST_ERR("Error: ret = %d\n", ret);
		return;
	}
}

int mhitest_ss_dummy_load(struct rproc *subsys_desc,
					const struct firmware *fw)
{
	/* no firmware load it will taken care by pci and mhi */
	return 0;
}

int mhitest_ss_add_ramdump_callback(struct rproc *subsys_desc,
			const struct firmware *firmware)
{
	MHITEST_LOG("Dummy callback, returning 0...\n");
	return 0;
}

const struct rproc_ops mhitest_rproc_ops = {
	.start = mhitest_ss_powerup,
	.stop = mhitest_ss_shutdown,
	.load = mhitest_ss_dummy_load,
	.parse_fw = mhitest_ss_add_ramdump_callback,
	.report_panic = mhitest_ss_crash_shutdown,
};

int mhitest_subsystem_register(struct mhitest_platform *mplat)
{
	MHITEST_VERB("Going for ss_reg.\n");

	if (mplat->d_instance == 0)
		mplat->mhitest_ss_desc_name = "mhitest-ss-0";
	else
		mplat->mhitest_ss_desc_name = "mhitest-ss-1";

	MHITEST_VERB("SS name :%s\n", mplat->mhitest_ss_desc_name);

	MHITEST_VERB("Doing rproc alloc..\n");
	mplat->subsys_handle = rproc_alloc(&mplat->plat_dev->dev,
					  mplat->mhitest_ss_desc_name,
					  &mhitest_rproc_ops,
					  DEFAULT_FW_FILE_NAME, 0);

	if (!mplat->subsys_handle) {
		MHITEST_ERR("rproc_alloc returned NULL..\n");
		return -EINVAL;
	}

	mplat->subsys_handle->priv = mplat;
	mplat->subsys_handle->auto_boot = false;

	MHITEST_VERB("Doing rproc add..\n");
	if (rproc_add(mplat->subsys_handle))
		return -EINVAL;

	if (!mplat->subsys_handle->dev.parent) {
		MHITEST_ERR("dev is null\n");
		return -ENODEV;
	}

	MHITEST_VERB("Doing rproc boot..\n");
	if (rproc_boot(mplat->subsys_handle))
		return -EINVAL;

	return 0;
}

void mhitest_subsystem_unregister(struct mhitest_platform *mplat)
{
	if (!mplat) {
		MHITEST_ERR("mplat is null, no subsystem unregister\n");
		return;
	}

	if (mplat->subsys_handle)
		rproc_shutdown(mplat->subsys_handle);
	else
		MHITEST_ERR("mplat->subsys_handle is NULL\n");

	mplat->subsys_handle = NULL;
}
