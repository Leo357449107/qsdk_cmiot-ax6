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

/*
 *
 * PCI MHI BHI related stuffs
 *
 *
 */
#include <linux/memblock.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/spinlock.h>
#include <linux/of_device.h>
#include <soc/qcom/ramdump.h>
#include <soc/qcom/subsystem_restart.h>
#include "commonmhitest.h"


static struct mhitest_msi_config msi_config = {
	.total_vectors = 3,
	.total_users = 1,
	.users = (struct mhitest_msi_user[]) {
		{ .name = "MHI-TEST", .num_vectors = 3, .base_vector = 0 },
	},
};

int mhitest_dump_info(struct mhitest_platform *mplat, bool in_panic)
{
	struct mhi_controller *mhi_ctrl;
	struct image_info *rddm_img, *fw_img;
	struct mhitest_dump_data *dump_data;
	struct mhitest_dump_seg *dump_seg;
	int ret, i;
	u16 device_id;

	mhi_ctrl = mplat->mhi_ctrl;
	pci_read_config_word(mplat->pci_dev, PCI_DEVICE_ID, &device_id);
	MHITEST_EMERG("Read config space again, Device_id:0x%x\n", device_id);
	if (device_id != mplat->pci_dev_id->device) {
		MHITEST_ERR("Device Id does not match with Probe ID..\n");
		return -EIO;
	}

	ret = mhi_download_rddm_img(mhi_ctrl, in_panic);
	if (ret) {
		MHITEST_ERR("Error .. not able to dload rddm img ret:%d\n",
									ret);
		return ret;
	}
	MHITEST_LOG("Let's dump some more things...\n");
	mhi_debug_reg_dump(mhi_ctrl);
	/*
	 * pr_mhitest2("fbc_img?:%p - rddm_img?:%p\n",mhi_ctrl->fbc_image,
	 *				mhi_ctrl->rddm_image);
	 */

	rddm_img = mhi_ctrl->rddm_image;
	fw_img = mhi_ctrl->fbc_image;
	dump_data = &mplat->mhitest_rdinfo.dump_data;
	dump_seg = mplat->mhitest_rdinfo.dump_data_vaddr;

	dump_data->nentries = 0;
	MHITEST_EMERG("dump_dname:%s entries:%d\n", dump_data->name,
						dump_data->nentries);
	MHITEST_EMERG("----Collect FW image dump segment, nentries %d----\n",
		    fw_img->entries);

	for (i = 0; i < fw_img->entries; i++) {
		dump_seg->address = fw_img->mhi_buf[i].dma_addr;
		dump_seg->v_address = fw_img->mhi_buf[i].buf;
		dump_seg->size = fw_img->mhi_buf[i].len;
		dump_seg->type = FW_IMAGE;
		MHITEST_EMERG("seg-%d:Address:0x%lx,v_Address %pK, size 0x%lx\n",
				i, dump_seg->address, dump_seg->v_address,
							dump_seg->size);
		dump_seg++;
	}
	dump_data->nentries += fw_img->entries;

	MHITEST_EMERG("----Collect RDDM image dump segment, nentries %d----\n",
		    rddm_img->entries);

	for (i = 0; i < rddm_img->entries; i++) {
		dump_seg->address = rddm_img->mhi_buf[i].dma_addr;
		dump_seg->v_address = rddm_img->mhi_buf[i].buf;
		dump_seg->size = rddm_img->mhi_buf[i].len;
		dump_seg->type = FW_RDDM;
		MHITEST_EMERG("seg-%d: address:0x%lx,v_address %pK,size 0x%lx\n",
				i, dump_seg->address, dump_seg->v_address,
								dump_seg->size);
		dump_seg++;
	}
	dump_data->nentries += rddm_img->entries;
	MHITEST_EMERG("----TODO/not need to Collect remote heap dump segment--\n");
	if (dump_data->nentries > 0)
		mplat->mhitest_rdinfo.dump_data_valid = true;

	return 0;
}

static int mhitest_get_msi_user(struct mhitest_platform *mplat, char *u_name,
		int *num_vectors, u32 *user_base_data, u32 *base_vector)
{
	int idx;
	struct mhitest_msi_config *m_config = mplat->msi_config;

	if (!m_config) {
		MHITEST_ERR("MSI config is NULL\n");
		return -ENODEV;
	}

	for (idx = 0; idx < m_config->total_users; idx++) {
		if (strcmp(u_name, m_config->users[idx].name) == 0) {
			*num_vectors = m_config->users[idx].num_vectors;
			*user_base_data = m_config->users[idx].base_vector
				+ mplat->msi_ep_base_data;
			*base_vector = m_config->users[idx].base_vector;
			MHITEST_VERB("Assign MSI to user:%s,num_vectors:%d,user_base_data:%u, base_vector: %u\n",
				u_name, *num_vectors, *user_base_data,
								*base_vector);

			return 0;
		}
	}
return -ENODEV;
}

static int mhitest_get_msi_irq(struct device  *device, unsigned int vector)
{
	int irq_num;
	struct pci_dev *pci_dev = to_pci_dev(device);

	irq_num = pci_irq_vector(pci_dev, vector);
	MHITEST_VERB("Got irq_num :%d  for vector : %d\n", irq_num, vector);

	return irq_num;
}

int mhitest_suspend_pci_link(struct mhitest_platform *mplat)
{
	/* no suspend resume now return 0*/
	MHITEST_LOG("No suspend resume now return 0\n");
	return 0;
}

int mhitest_resume_pci_link(struct mhitest_platform *mplat)
{
	/* no suspend resume now return 0*/
	MHITEST_LOG("No suspend resume now return 0\n");
	return 0;
}

int  mhitest_power_off_device(struct mhitest_platform *mplat)
{
	/*
	*	Added pinctrl code here if needed !
	*/
	MHITEST_LOG("Powering OFF dummy!\n");
	return 0;
}

int  mhitest_power_on_device(struct mhitest_platform *mplat)
{

	/*
	*	Added pinctrl code here if needed !
	*/
	MHITEST_LOG("Powering ON dummy!\n");
	return 0;
}

int mhitest_pci_get_link_status(struct mhitest_platform *mplat)
{
	u16 link_stat;
	int ret;

	ret = pcie_capability_read_word(mplat->pci_dev, PCI_EXP_LNKSTA,
							&link_stat);
	if (ret) {
		MHITEST_ERR("PCIe link is not active !!ret:%d\n", ret);
		return ret;
	}
	MHITEST_VERB("Get PCI link status register: %u\n", link_stat);

	mplat->def_link_speed = link_stat & PCI_EXP_LNKSTA_CLS;
	mplat->def_link_width =
		(link_stat & PCI_EXP_LNKSTA_NLW) >> PCI_EXP_LNKSTA_NLW_SHIFT;

	MHITEST_VERB("Default PCI link speed is 0x%x, link width is 0x%x\n",
		    mplat->def_link_speed, mplat->def_link_width);

	return ret;
}

int mhitest_pci_get_mhi_msi(struct mhitest_platform *mplat)
{
	int ret, *irq, num_vectors, i;
	u32 user_base_data, base_vector;

	/*right now we have only one user i.e MHI */
	ret = mhitest_get_msi_user(mplat, "MHI-TEST", &num_vectors,
					&user_base_data, &base_vector);
	if (ret) {
		MHITEST_ERR("Not able to get msi user ret:%d\n", ret);
		return ret;
	}

	MHITEST_LOG("MSI user:%s has num_vectore:%d and bas_vectore:%d\n",
					"MHI-TEST", num_vectors, base_vector);

	irq = kcalloc(num_vectors, sizeof(int), GFP_KERNEL);
	if (!irq) {
		MHITEST_ERR("Error not able to allocate vectores\n");
		return -ENOMEM;
	}
	for (i = 0; i < num_vectors; i++)
		irq[i] = mhitest_get_msi_irq(&mplat->pci_dev->dev,
							base_vector + i);

	mplat->mhi_ctrl->irq = irq;
	mplat->mhi_ctrl->msi_allocated = num_vectors;

	MHITEST_VERB("irq:[%p] msi_allocated :%d\n", mplat->mhi_ctrl->irq,
				mplat->mhi_ctrl->msi_allocated);

	return 0;
}

char *mhitest_get_reson_str(enum mhi_callback reason)
{

	switch (reason) {
	case MHI_CB_IDLE:
		return "IDLE";
	case MHI_CB_EE_RDDM:
		return "RDDM";
	case MHI_CB_SYS_ERROR:
		return "SYS_ERROR";
	case MHI_CB_FATAL_ERROR:
		return "FATAL_ERROR";
	default:
		return "UNKNOWN";

	}
}

char *mhitest_event_to_str(enum mhitest_event_type etype)
{
	switch (etype) {
	case MHITEST_RECOVERY_EVENT:
		return "MHITEST_RECOVERY_EVENT";
	default:
		return "UNKNOWN EVENT";
	}
}
int mhitest_post_event(struct mhitest_platform *mplat,
	struct mhitest_recovery_data *data, enum mhitest_event_type etype,
							u32 flags)
{
	struct mhitest_driver_event *event;
	int gfp = GFP_KERNEL;
	unsigned long irq_flags;

	if (in_interrupt() || irqs_disabled())
		gfp = GFP_ATOMIC;

	event = kzalloc(sizeof(*event), gfp);
	if (!event)
		return -ENOMEM;

	event->type = etype;
	event->data = data;
	init_completion(&event->complete);
	event->ret = -1;
	event->sync = !!(flags);

	spin_lock_irqsave(&mplat->event_lock, irq_flags);
	list_add_tail(&event->list, &mplat->event_list);
	spin_unlock_irqrestore(&mplat->event_lock, irq_flags);

	queue_work(mplat->event_wq, &mplat->event_work);
	if (flags) {
		MHITEST_ERR("Waiting here to complete (%s) event ...\n",
			mhitest_event_to_str(etype));
		wait_for_completion(&event->complete);
	}
	MHITEST_LOG("No waiting/Completed (%s) event ...ret:%d\n",
			mhitest_event_to_str(etype), event->ret);

	return 0;
}

void mhitest_sch_do_recovery(struct mhitest_platform *mplat,
				enum mhitest_recovery_reason reason)
{
	int gfp = GFP_KERNEL;
	struct mhitest_recovery_data *data;

	if (in_interrupt() || irqs_disabled())
		gfp = GFP_ATOMIC;

	data = kzalloc(sizeof(*data), gfp);
	if (!data)
		return;

	data->reason = reason;

	mhitest_post_event(mplat, data, MHITEST_RECOVERY_EVENT, 0);
}

int mhitest_mhi_link_status(struct mhi_controller *mhi_ctrl)
{

	MHITEST_LOG("Link status..return with 1\n");
	return 1;
}

void mhitest_mhi_notify_status(struct mhi_controller *mhi_cntrl,
						enum mhi_callback reason)
{

	struct mhitest_platform *temp = mhi_cntrl->priv_data;

	MHITEST_VERB("Enter\n");
	if (reason > MHI_CB_FATAL_ERROR) {
		MHITEST_ERR("Unsupported reason :%d\n", reason);
		return;
	}
	MHITEST_EMERG(":[%s]- %d\n", mhitest_get_reson_str(reason), reason);

	switch (reason) {
	case MHI_CB_IDLE:
	case MHI_CB_SYS_ERROR:
		return;

	case MHI_CB_FATAL_ERROR:
		reason = MHI_DEFAULT;
		return;
	case MHI_CB_EE_RDDM:
		reason = MHI_RDDM;
		break;
	default:
		MHITEST_ERR("Unsupported reason --reason:[%s]-(%d)\n",
				mhitest_get_reson_str(reason), reason);
		return;
	}
	mhitest_sch_do_recovery(temp, reason);
	MHITEST_VERB("Exit\n");
}

int mhitest_mhi_pm_runtime_get(struct mhi_controller *mhi_cntrl)
{

	struct mhitest_platform *temp2 = mhi_cntrl->priv_data;

	if (!temp2)
		return -ENODEV;
	if (!temp2->pci_dev)
		return -ENODEV;
	if (!&temp2->pci_dev->dev)
		return -ENODEV;

	MHITEST_VERB("\n");
	return pm_runtime_get(&temp2->pci_dev->dev);
}

void mhitest_mhi_pm_runtime_put_noidle(struct mhi_controller *mhi_cntrl)
{
	struct mhitest_platform *temp2 = mhi_cntrl->priv_data;

	if (!temp2)
		return;
	if (!temp2->pci_dev)
		return;
	if (!&temp2->pci_dev->dev)
		return;

	MHITEST_VERB("\n");
	pm_runtime_put_noidle(&temp2->pci_dev->dev);
}

int mhitest_pci_register_mhi(struct mhitest_platform *mplat)
{
	struct pci_dev *pci_dev = mplat->pci_dev;
	struct mhi_controller *mhi_ctrl;
	struct device_node *np;
	int ret, len, sw, aw;
	unsigned int *reg, *reg_end;
	unsigned long start, size;

	mhi_ctrl = mhi_alloc_controller(0);
	if (!mhi_ctrl) {
		MHITEST_ERR("Error: not able to allocate mhi_ctrl\n");
		return -EINVAL;
	}
	MHITEST_LOG("MHI CTRL :%p\n", mhi_ctrl);

	mplat->mhi_ctrl = mhi_ctrl;
	mhi_ctrl->priv_data = mplat;
	mhi_ctrl->dev = &pci_dev->dev;
	mhi_ctrl->of_node = (&mplat->plat_dev->dev)->of_node;
	mhi_ctrl->dev_id = mplat->device_id;
	mhi_ctrl->domain = pci_domain_nr(pci_dev->bus);
	mhi_ctrl->bus = pci_dev->bus->number;
	mhi_ctrl->slot = PCI_SLOT(pci_dev->devfn);

	if (!mplat->fw_name) {
		MHITEST_ERR("fw_name is NULLL\n");
		return -EINVAL;
	}
	MHITEST_LOG("mhi_ctrl->of_node-name;%s\n",
						mhi_ctrl->of_node->name);
	mhi_ctrl->fw_image = mplat->fw_name;
	mhi_ctrl->regs = mplat->bar;
	MHITEST_EMERG("BAR start at :%pa\n", &pci_resource_start(pci_dev,
								PCI_BAR_NUM));

	ret  =  mhitest_pci_get_mhi_msi(mplat);
	if (ret) {
		MHITEST_ERR("PCI get MHI MSI Failed ret:%d\n", ret);
		goto out;
	}

	np = of_find_node_by_type(NULL, "memory");
	if (!np) {
		MHITEST_ERR("memory node not found !!\n");
		return 1;
	}

	aw = of_n_addr_cells(np);
	sw = of_n_size_cells(np);

	reg = (unsigned int *)of_get_property(np, "reg", &len);
	if (!reg) {
		pr_mhitest2("Couldn't get reg from mem node\n");
		return -ENOMEM;
	}
	reg_end = reg + len/4;
	do {
		start = of_read_number(reg, aw);
		reg += aw;
		size = of_read_number(reg, sw);
		reg += sw;
	} while (reg < reg_end);


	mhi_ctrl->iova_start = (dma_addr_t)(start + 0x1000000);
	mhi_ctrl->iova_stop = (dma_addr_t)(start + size);

	MHITEST_VERB("iova_start:%x iova_stop:%x\n",
			(unsigned int)mhi_ctrl->iova_start,
				(unsigned int)mhi_ctrl->iova_stop);
	mhi_ctrl->link_status = mhitest_mhi_link_status;
	mhi_ctrl->status_cb =	mhitest_mhi_notify_status;
	mhi_ctrl->runtime_get =	mhitest_mhi_pm_runtime_get;
	mhi_ctrl->runtime_put = mhitest_mhi_pm_runtime_put_noidle;

	mhi_ctrl->rddm_size = mplat->mhitest_rdinfo.ramdump_size;
	mhi_ctrl->sbl_size = SZ_512K;
	mhi_ctrl->seg_len = SZ_512K;
	mhi_ctrl->fbc_download = true;

	/*
	* let's decide log level for mhi controller
	* MHI_MSG_LVL_VERBOSE/MHI_MSG_LVL_INFO
	*
	* mhi_ctrl->klog_lvl = MHI_MSG_LVL_INFO;
	*/

	ret = of_register_mhi_controller(mhi_ctrl);
	if (ret) {
		MHITEST_ERR("Failed to register mhi controller ret:%d\n", ret);
		goto out;
	}
	MHITEST_VERB("GOOD!\n");
	return  0;

out:
	return ret;
}

int mhitest_pci_en_msi(struct mhitest_platform *temp)
{
	struct pci_dev *pci_dev = temp->pci_dev;
	int num_vectors, ret = 0;
	struct msi_desc *msi_desc;

	temp->msi_config = &msi_config;

	if (!temp->msi_config) {
		MHITEST_ERR("MSI config is NULL\n");
		return -EINVAL;
	}

	num_vectors = pci_alloc_irq_vectors(pci_dev,
		temp->msi_config->total_vectors,
			temp->msi_config->total_vectors, PCI_IRQ_NOMSIX);
	if (num_vectors != temp->msi_config->total_vectors) {
		MHITEST_ERR("No Enough MSI vectors req:%d and allocated:%d\n",
				temp->msi_config->total_vectors, num_vectors);
		if (num_vectors >= 0)
			ret = -EINVAL;
		temp->msi_config = NULL;
		goto out;
}
	msi_desc = irq_get_msi_desc(pci_dev->irq);
	if (!msi_desc) {
		MHITEST_ERR("MSI desc is NULL\n");
		goto free_irq_vectors;
	}
	/*
	*comment this for now
	*temp->msi_ep_base_data = msi_desc->msg.data;
	*if (!temp->msi_ep_base_data) {
	*	pr_mhitest2("Got 0 msi base data, Not good\n");
	*	ret = -EINVAL;
	*	goto out;
	*}
	*pr_mhitest2("MSI base data is %d\n", temp->msi_ep_base_data);
	*/
	return 0;
free_irq_vectors:
	pci_free_irq_vectors(pci_dev);
out:
	return ret;
}

int mhitest_pci_enable_bus(struct mhitest_platform *temp)
{
	struct pci_dev *pci_dev = temp->pci_dev;
	u16 device_id;
	int ret;
	u32 pci_dma_mask = PCI_DMA_MASK_64_BIT;

	MHITEST_VERB("Going for PCI Enable bus\n");
	pci_read_config_word(pci_dev, PCI_DEVICE_ID, &device_id);
	MHITEST_EMERG("Read config space, Device_id:0x%x\n", device_id);

	/*check for probe ID is same as config same read ID ?*/
	if (device_id != temp->pci_dev_id->device) {
		MHITEST_ERR("Device Id does not match with Probe ID..\n");
		return -EIO;
	}

	ret = pci_assign_resource(pci_dev, PCI_BAR_NUM);
	if (ret) {
		MHITEST_ERR("Failed to assign PCI resource  Error:%d\n", ret);
		goto out;
	}
	/*VERIFY_ME(ret,"PCI BAR assigned?");*/

	ret = pci_enable_device(pci_dev);
	if (ret) {
		MHITEST_ERR("Failed to Enable PCI device  Error:%d\n", ret);
		goto out;
	}
	/*VERIFY_ME(ret,"PCI device enabled?");*/

	ret = pci_request_region(pci_dev, PCI_BAR_NUM, "mhitest_region");
	if (ret) {
		MHITEST_ERR("Failed to req. region Error:%d\n", ret);
		goto out2;
	}

	ret = pci_set_dma_mask(pci_dev, DMA_BIT_MASK(pci_dma_mask));
	if (ret) {
		MHITEST_ERR("Failed to set dma mask:(%d) ret:%d\n",
					pci_dma_mask, ret);
		goto out3;
	}

	pci_set_master(pci_dev);

	temp->bar = pci_iomap(pci_dev, PCI_BAR_NUM, 0);
	if (!temp->bar) {
		MHITEST_ERR("Failed to do PCI IO map ..\n");
		ret = -EIO;
		goto out4;
	}
	/* do we need this after bus enable...I think yes lets keep to reminder
	*         pci_save_state(pci_dev);
	*              pci_priv->default_state = pci_store_saved_state(pci_dev);
	*
	*/
	/*Let save this pci config space before suspend */
	pci_save_state(pci_dev);
	temp->pci_dev_default_state = pci_store_saved_state(pci_dev);

	return 0;

out4:
	pci_clear_master(pci_dev);
out3:
	pci_release_region(pci_dev, PCI_BAR_NUM);
out2:
	pci_disable_device(pci_dev);
out:
	return ret;
}
void mhitest_global_soc_reset(struct mhitest_platform *mplat)
{
	u32 current_ee;
	u32 count = 0;

	current_ee = mhi_get_exec_env(mplat->mhi_ctrl);
	MHITEST_EMERG("Soc Globle Reset issued\n");

	do {
		writel_relaxed(PCIE_SOC_GLOBAL_RESET_VALUE,
			       PCIE_SOC_GLOBAL_RESET_ADDRESS +
			       mplat->bar);
		msleep(20);
		current_ee = mhi_get_exec_env(mplat->mhi_ctrl);
		count++;
	} while (current_ee != MHI_EE_PBL &&
		 count < MAX_SOC_GLOBAL_RESET_WAIT_CNT);

	if (current_ee != MHI_EE_PBL && count >= MAX_SOC_GLOBAL_RESET_WAIT_CNT)
		MHITEST_EMERG("SoC global reset failed! Reset count : %d\n",
			      count);
	else
		MHITEST_ERR("SOC Global reset count: %d\n", count);
}

void mhitest_pci_disable_bus(struct mhitest_platform *mplat)
{
	struct pci_dev *pci_dev = mplat->pci_dev;
	u32 in_reset = -1, temp = -1, retries = 3;

	mhitest_global_soc_reset(mplat);

	msleep(2000);

	mhi_set_mhi_state(mplat->mhi_ctrl, MHI_STATE_RESET);

        while (retries--) {
                temp = readl_relaxed(mplat->mhi_ctrl->regs  + 0x38);
                in_reset = (temp & 0x2) >> 0x1;
                if (in_reset){
                        MHITEST_LOG("Number of retry left:%d- trying again\n",
                                                                retries);
                        udelay(10);
                        continue;
                }
                break;
        }

	if (in_reset) {
		MHITEST_ERR("Device failed to exit RESET state\n");
		return;
	}
	MHITEST_EMERG("MHI Reset good!\n");

	if (mplat->bar) {
		pci_iounmap(pci_dev, mplat->bar);
		mplat->bar = NULL;
	}

	pci_clear_master(pci_dev);
	pci_release_region(pci_dev, PCI_BAR_NUM);
	if (pci_is_enabled(pci_dev))
		pci_disable_device(pci_dev);
}

int mhitest_unregister_ramdump(struct mhitest_platform *mplat)
{
	struct mhitest_ramdump_info *mhitest_rdinfo = &mplat->mhitest_rdinfo;

	if (mhitest_rdinfo->ramdump_dev)
		destroy_ramdump_device(mhitest_rdinfo->ramdump_dev);
	kfree(mhitest_rdinfo->dump_data_vaddr);
	mhitest_rdinfo->dump_data_vaddr = NULL;
	mhitest_rdinfo->dump_data_valid = false;

return 0;
}

int mhitest_register_ramdump(struct mhitest_platform *mplat)
{
	struct subsys_desc *mhitest_ss_desc;
	struct mhitest_ramdump_info *mhitest_rdinfo;
	struct mhitest_dump_data *dump_data;
	struct device *dev = &mplat->plat_dev->dev;
	u32 ramdump_size = 0;
	int ret;

	mhitest_ss_desc = &mplat->mhitest_ss_desc;
	mhitest_rdinfo = &mplat->mhitest_rdinfo;
	dump_data = &mhitest_rdinfo->dump_data;
	if (!dev->of_node) {
		MHITEST_ERR("of node is null\n");
		return -ENOMEM;
	}
	if (!dev->of_node->name) {
		MHITEST_ERR("of node->name  is NULL\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32(dev->of_node, "qcom,wlan-ramdump-dynamic",
					 &ramdump_size);
	if (ret == 0)
		mhitest_rdinfo->ramdump_size = ramdump_size;

	mhitest_rdinfo->dump_data_vaddr = kzalloc(0x1000, GFP_KERNEL);
	if (!mhitest_rdinfo->dump_data_vaddr)
		return -ENOMEM;

	dump_data->paddr = virt_to_phys(mhitest_rdinfo->dump_data_vaddr);

	/*TODO: used ramdom version and magic..etc check and correct it.*/
	dump_data->version = 0x00;
	dump_data->magic = 0xAA55AA55;
	dump_data->seg_version = 0x1;
	strlcpy(dump_data->name, "mhitest_mod",
		sizeof(dump_data->name));
	mhitest_rdinfo->ramdump_dev =
		create_ramdump_device(mhitest_ss_desc->name,
				      mhitest_ss_desc->dev);
	if (!mhitest_rdinfo->ramdump_dev) {
		MHITEST_ERR("Failed to create ramdump device!\n");
		ret = -ENOMEM;
		goto free_ramdump;
	}

	MHITEST_LOG("Ramdump registered ramdump_size:0x%x\n", ramdump_size);

	return 0;
free_ramdump:
	kfree(mhitest_rdinfo->dump_data_vaddr);
	mhitest_rdinfo->dump_data_vaddr = NULL;
	return ret;

}

int mhitest_prepare_pci_mhi_msi(struct mhitest_platform *temp)
{
	int ret;

	MHITEST_VERB("Enter\n");
	if (!temp->pci_dev) {
		MHITEST_ERR("pci_dev is NULLL\n");
		return -EINVAL;
	}
	ret = mhitest_register_ramdump(temp);
	if (ret) {
		MHITEST_ERR("Error not able to reg ramdump. ret :%d\n", ret);
		goto unreg_rdump;
	}
#if 1
	/* 1. pci enable bus*/
	ret = mhitest_pci_enable_bus(temp);
	if (ret) {
		MHITEST_ERR("Error mhitest_pci_enable. ret :%d\n", ret);
		goto out;
	}

	/*go with some condition for specific device for msi en*/
	/* 2. pci enable msi*/
	ret = mhitest_pci_en_msi(temp);
	if (ret) {
		MHITEST_ERR("Error mhitest_pci_enable_msi. ret :%d\n", ret);
		goto disable_bus;
	}

	/* 3. pci register mhi -of_controller*/
	ret = mhitest_pci_register_mhi(temp);
	if (ret) {
		MHITEST_ERR("Error pci register mhi. ret :%d\n", ret);
		goto disable_bus;
	}

	ret = mhitest_pci_get_link_status(temp);
	if (ret) {
		MHITEST_ERR("Error not able to get pci link status:%d\n", ret);
		goto out;
	}
	ret = mhitest_suspend_pci_link(temp);
	if (ret) {
		MHITEST_ERR("Error not able to suspend pci:%d\n", ret);
		goto out;
	}

	mhitest_power_off_device(temp);
	MHITEST_VERB("Exit\n");
	return 0;
#endif
disable_bus:
	mhitest_pci_disable_bus(temp);
unreg_rdump:
	mhitest_unregister_ramdump(temp);
out:
	return ret;
}

char *mhitest_get_mhi_state_str(enum mhi_state state)
{
	switch (state) {
	case MHI_INIT:
		return "MHI_INIT";
	case MHI_DEINIT:
		return "MHI_DEINIT";
	case MHI_POWER_ON:
		return "MHI_POWER_ON";
	case MHI_POWER_OFF:
		return "MHI_POWER_OFF";
	case MHI_FORCE_POWER_OFF:
		return "MHI_FORCE_POWER_OFF";
	default:
		return "UNKNOWN";
	}
}

int mhitest_pci_set_mhi_state(struct mhitest_platform *mplat,
						enum mhi_state state)
{
	int ret = 0;

	if (state < 0) {
		MHITEST_ERR("Invalid MHI state : %d\n", state);
		return -EINVAL;
	}
	MHITEST_EMERG("Set MHI_STATE- [%s]-(%d)\n",
				mhitest_get_mhi_state_str(state), state);
	switch (state) {
	case MHI_INIT:
		ret = mhi_prepare_for_power_up(mplat->mhi_ctrl);
		break;
	case MHI_POWER_ON:
		ret = mhi_sync_power_up(mplat->mhi_ctrl);
		break;
	case MHI_DEINIT:
		mhi_unprepare_after_power_down(mplat->mhi_ctrl);
		ret = 0;
		break;
	case MHI_POWER_OFF:
		mhi_power_down(mplat->mhi_ctrl, true);
		ret = 0;
		break;

	default:
		MHITEST_ERR("I dont know the state:%d!!\n", state);
		ret = -EINVAL;
	}
	return ret;
}

int mhitest_pci_start_mhi(struct mhitest_platform *mplat)
{
	int ret;
	MHITEST_VERB("Enter\n");

	if (!mplat->mhi_ctrl) {
		MHITEST_ERR("mhit_ctrl is NULL .. returning..\n");
		return -EINVAL;
	}

	mplat->mhi_ctrl->timeout_ms = MHI_TIMEOUT_DEFAULT;

	ret = mhitest_pci_set_mhi_state(mplat, MHI_INIT);
	if (ret) {
		MHITEST_ERR("Error not able to set mhi init. returning..\n");
		goto out1;
	}
	ret = mhitest_pci_set_mhi_state(mplat, MHI_POWER_ON);
	if (ret) {
		MHITEST_ERR("Error not able to POWER ON\n");
		if (ret == -ETIMEDOUT) {
		/* Thought it is ETIMEOUT we are returning 0 here so that
		 * we should be able to do rcremove and rmmod.
		 * rcremove api are not exported so mhitest driver can not call
		 * them. Printing Error message here to know the user.
		 */
			MHITEST_ERR("###### -ETIMEDOUT ERRRORR, do rcremove and rmmod to clean-up\n");
			ret = 0;
		}
		goto out1;
	}
	return ret;
	MHITEST_VERB("Exit\n");
out1:
	MHITEST_VERB("Exit-Error\n");
	return ret;
}
int mhitest_prepare_start_mhi(struct mhitest_platform *mplat)
{
	int ret;

	/* 1. power on , resume link if?*/
	ret = mhitest_power_on_device(mplat);
	if (ret) {
		MHITEST_ERR("Error ret:%d\n", ret);
		goto out;
	}
	ret = mhitest_resume_pci_link(mplat);
	if (ret) {
		MHITEST_ERR("Error ret: %d\n", ret);
		goto out;
	}

	/* 2. start mhi*/
	ret = mhitest_pci_start_mhi(mplat);
	if (ret) {
		MHITEST_ERR("Error ret: %d\n", ret);
		goto out;
	}

out:
	return ret;
}

int mhitest_pci_probe(struct pci_dev *pci_dev, const struct pci_device_id *id)
{
	struct mhitest_platform *temp = get_mhitest_mplat(id->device);

	MHITEST_VERB("Enter\n");

	if (!temp) {
		MHITEST_ERR("temp is null..\n");
		return -ENOMEM;
	}
	MHITEST_LOG("Vendor:0x%x Device:0x%x probe d id:0x%x d_instance:%d\n",
			pci_dev->vendor, pci_dev->device, id->device,
					temp->d_instance);
	/* store this tho main struct*/
	temp->pci_dev = pci_dev;
	temp->device_id = pci_dev->device;
	temp->pci_dev_id = id;
	MHITEST_VERB("Exit\n");
	return 0;
}
extern int debug_lvl;
extern int domain;
int mhitest_pci_probe2(struct pci_dev *pci_dev, const struct pci_device_id *id)
{
	struct mhitest_platform *mplat;
	struct platform_device *plat_dev = get_plat_device();
	int ret;
	struct device_node *np;

	if ((domain != -1) && pci_domain_nr(pci_dev->bus) != domain) {
		MHITEST_LOG("Skipping MHI Device on: %04x:%02x:%02x\n",
			    pci_domain_nr(pci_dev->bus),
			    pci_dev->bus->number, PCI_SLOT(pci_dev->devfn));
		return 0;
	}

	MHITEST_EMERG("--->\n");
	mplat = devm_kzalloc(&plat_dev->dev, sizeof(*mplat), GFP_KERNEL);
	if (!mplat) {
		MHITEST_ERR("Error: not able to allocate memory\n");
		ret = -ENOMEM;
		goto fail_probe;
	}

	np = of_find_compatible_node(NULL, NULL, "qcom,cnss-qcn9000");
	if (!np) {
		MHITEST_ERR("Couldn't find necessary node\n");
		return -ENODEV;
	}
#ifdef CONFIG_CNSS2
	/* let's reuse the same !!*/
	plat_dev->dev.of_node = np;
#else
	plat_dev->dev.of_node = pci_dev->dev.of_node;
#endif

	mplat->mhitest_klog_lvl = debug_lvl;

	mplat->plat_dev = plat_dev;
	platform_set_drvdata(plat_dev, mplat);
	mplat->pci_dev = pci_dev;
	mplat->device_id = pci_dev->device;
	mplat->pci_dev_id = id;
	MHITEST_LOG("Vendor ID:0x%x Device ID:0x%x Probed Device ID:0x%x\n",
			pci_dev->vendor, pci_dev->device, id->device);
	ret = mhitest_event_work_init(mplat);
	if (ret)
		goto out1;

	ret = mhitest_store_mplat(mplat);
	if (ret) {
		MHITEST_ERR("Error ret:%d\n", ret);
		goto out1;
	}
	ret = mhitest_subsystem_register(mplat);
	if (ret) {
		MHITEST_ERR("Error subsystem register: ret:%d\n", ret);
		goto out1;
	}

	MHITEST_EMERG("<---done\n");
	return 0;
out1:
	kfree(mplat);
fail_probe:
	return ret;
}
void mhitest_pci_remove(struct pci_dev *pci_dev)
{
	struct mhitest_platform *mplat;
	struct platform_device *plat_dev = get_plat_device();
	struct device_node *np;
	static int index = 0;

	if ((domain != -1) && pci_domain_nr(pci_dev->bus) != domain) {
		MHITEST_LOG("Skipping MHI Device on: %04x:%02x:%02x\n",
			    pci_domain_nr(pci_dev->bus),
			    pci_dev->bus->number, PCI_SLOT(pci_dev->devfn));
		return;
	}
	MHITEST_LOG("mhitest PCI removing\n");

	np = of_find_compatible_node(NULL, NULL, "qcom,testmhi");
	if (!np) {
		MHITEST_ERR("Couldn't find necessary node\n");
		return;
	}
	/* Revert back */
	plat_dev->dev.of_node = np;

	mplat = get_mhitest_mplat(index++);
/*
	mhitest_pci_set_mhi_state(mplat, MHI_POWER_OFF);
	msleep(1000);
	mhitest_pci_set_mhi_state(mplat, MHI_DEINIT);
	mhitest_pci_remove_all(mplat);
*/
	mhitest_subsystem_unregister(mplat);
	mhitest_event_work_deinit(mplat);
	pci_load_and_free_saved_state(pci_dev, &mplat->pci_dev_default_state);
	kfree(mplat->mhi_ctrl);
	mhitest_free_mplat(mplat);
}
static const struct pci_device_id mhitest_pci_id_table[] = {
		/*
		* dummy used for testing probe etc
		* {0x17cb, 0x1002, PCI_ANY_ID, PCI_ANY_ID},
		* {0x168c, 0x0056, PCI_ANY_ID, PCI_ANY_ID},
		*/
/*Hasting*/	{QCA6390_VENDOR_ID, QCA6390_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID},
/*Pine*/	{QCN90xx_VENDOR_ID, QCN90xx_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID},
};
struct pci_driver mhitest_pci_driver = {
	.name	= "mhitest_pci",
	/*.probe	= mhitest_pci_probe,*/
	.probe	= mhitest_pci_probe2,
	.remove	= mhitest_pci_remove,
	.id_table	= mhitest_pci_id_table,
};


int mhitest_pci_register(void)
{
	int ret;

	ret = pci_register_driver(&mhitest_pci_driver);
	/*VERIFY_ME(ret,"PCI device registered ?\n");*/
	if (ret) {
		MHITEST_ERR("Error ret:%d\n", ret);
		goto out;
	}
out:
	return ret;
}
void mhitest_pci_unregister(void)
{
	MHITEST_VERB("\n");
	pci_unregister_driver(&mhitest_pci_driver);
}
