/* Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

#include <linux/types.h>
#include "pci.h"
#include "bus.h"
#include "debug.h"
#include <linux/of_address.h>

static enum cnss_dev_bus_type cnss_get_dev_bus_type(struct device *dev)
{
	if (!dev)
		return CNSS_BUS_NONE;

	if (!dev->bus)
		return CNSS_BUS_NONE;

	if (memcmp(dev->bus->name, "pci", 3) == 0)
		return CNSS_BUS_PCI;
	else if (memcmp(dev->bus->name, "platform", 8) == 0)
		return CNSS_BUS_AHB;
	else
		return CNSS_BUS_NONE;
}

enum cnss_dev_bus_type cnss_get_bus_type(unsigned long device_id)
{
	switch (device_id) {
	case QCA6174_DEVICE_ID:
	case QCN9000_DEVICE_ID:
	case QCN9224_DEVICE_ID:
	case QCA6390_DEVICE_ID:
	case QCA6490_DEVICE_ID:
		return CNSS_BUS_PCI;
	case QCA8074_DEVICE_ID:
	case QCA8074V2_DEVICE_ID:
	case QCA6018_DEVICE_ID:
	case QCA5018_DEVICE_ID:
	case QCN6122_DEVICE_ID:
	case QCA9574_DEVICE_ID:
		return CNSS_BUS_AHB;
	default:
		pr_err("Unknown device_id: 0x%lx\n", device_id);
		return CNSS_BUS_NONE;
	}
}

void *cnss_bus_dev_to_bus_priv(struct device *dev)
{
	if (!dev)
		return NULL;

	switch (cnss_get_dev_bus_type(dev)) {
	case CNSS_BUS_PCI:
		return cnss_get_pci_priv(to_pci_dev(dev));
	case CNSS_BUS_AHB:
		return NULL;
	default:
		return NULL;
	}
}

struct cnss_plat_data *cnss_bus_dev_to_plat_priv(struct device *dev)
{
	void *bus_priv;
	struct pci_dev *pdev;

	if (!dev)
		return NULL;

	switch (cnss_get_dev_bus_type(dev)) {
	case CNSS_BUS_PCI:
		pdev = to_pci_dev(dev);
		if (pdev->device != QCN9000_DEVICE_ID &&
		    pdev->device != QCN9224_DEVICE_ID)
			return NULL;

		bus_priv = cnss_bus_dev_to_bus_priv(dev);
		if (bus_priv)
			return cnss_pci_priv_to_plat_priv(bus_priv);
		else
			return NULL;
	case CNSS_BUS_AHB:
		return cnss_get_plat_priv(to_platform_device(dev));
	default:
		return NULL;
	}
}

int cnss_bus_init_by_type(int type)
{
	switch (type) {
	case CNSS_BUS_PCI:
		return cnss_pci_init(NULL);
	case CNSS_BUS_AHB:
		return 0;
	default:
		pr_err("Unsupported bus type: %d\n",
		       type);
		return -EINVAL;
	}
}

int cnss_bus_init(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_init(plat_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

void cnss_bus_deinit(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		cnss_pci_deinit(plat_priv);
	case CNSS_BUS_AHB:
		break;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return;
	}
}

int cnss_bus_load_m3(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_load_m3(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_alloc_fw_mem(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_alloc_fw_mem(plat_priv);
	case CNSS_BUS_AHB:
		return cnss_ahb_alloc_fw_mem(plat_priv);
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

void cnss_bus_free_fw_mem(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
	case CNSS_BUS_AHB:
		cnss_pci_free_fw_mem(plat_priv);
		break;
	default:
		cnss_pr_err("Unsupported bus type %d\n",
			    plat_priv->bus_type);
	}
}

static
struct device_node *cnss_get_etr_dev_node(struct cnss_plat_data *plat_priv)
{
	struct device_node *dev_node = NULL;

	if (plat_priv->device_id == QCN6122_DEVICE_ID) {
		if (plat_priv->userpd_id == QCN6122_0)
			dev_node = of_find_node_by_name(NULL,
							"q6_qcn6122_etr_1");
		else if (plat_priv->userpd_id == QCN6122_1)
			dev_node = of_find_node_by_name(NULL,
							"q6_qcn6122_etr_2");
	} else {
		dev_node = of_find_node_by_name(NULL, "q6_etr_dump");
	}

	return dev_node;
}

int cnss_bus_alloc_qdss_mem(struct cnss_plat_data *plat_priv)
{
	int i;
	int bus_type;
	struct device_node *dev_node = NULL;
	struct resource q6_etr;
	int ret;

	if (!plat_priv)
		return -ENODEV;

	bus_type = cnss_get_bus_type(plat_priv->device_id);

	switch (bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_alloc_qdss_mem(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		dev_node = cnss_get_etr_dev_node(plat_priv);
		if (!dev_node) {
			cnss_pr_err("No q6_etr_dump available in dts");
			return -ENOMEM;
		}

		ret = of_address_to_resource(dev_node, 0, &q6_etr);
		if (ret) {
			cnss_pr_err("Failed to get resource for q6_etr_dump");
			return -EINVAL;
		}

		for (i = 0; i < plat_priv->qdss_mem_seg_len; i++) {
			plat_priv->qdss_mem[i].va = NULL;
			plat_priv->qdss_mem[i].pa = q6_etr.start;
			plat_priv->qdss_mem[i].size = resource_size(&q6_etr);
			plat_priv->qdss_mem[i].type = QMI_WLFW_MEM_QDSS_V01;

			if (plat_priv->device_id == QCN6122_DEVICE_ID) {
				plat_priv->qdss_mem[i].va =
					ioremap(plat_priv->qdss_mem[i].pa,
						plat_priv->qdss_mem[i].size);
				if (!plat_priv->qdss_mem[i].va) {
					cnss_pr_err("WARNING etr-addr remap failed\n");
					return -ENOMEM;
				}
			}

			cnss_pr_dbg("QDSS mem addr pa 0x%x va 0x%p, size 0x%x",
				    (unsigned int)plat_priv->qdss_mem[i].pa,
				    plat_priv->qdss_mem[i].va,
				    (unsigned int)plat_priv->qdss_mem[i].size);
		}

		return 0;
	default:
		cnss_pr_err("Unsupported bus type:%d\n", bus_type);
		return -EINVAL;
	}
}

void cnss_bus_free_qdss_mem(struct cnss_plat_data *plat_priv)
{
	int bus_type;

	if (!plat_priv)
		return;

	bus_type = cnss_get_bus_type(plat_priv->device_id);

	switch (bus_type) {
	case CNSS_BUS_PCI:
	case CNSS_BUS_AHB:
		cnss_pci_free_qdss_mem(plat_priv);
		break;
	default:
		cnss_pr_err("Unsupported bus type %d\n", bus_type);
		break;
	}
}

u32 cnss_bus_get_wake_irq(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_get_wake_msi(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_force_fw_assert_hdlr(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_force_fw_assert_hdlr(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

void cnss_bus_fw_boot_timeout_hdlr(struct timer_list *timer)
{
	struct cnss_plat_data *plat_priv =
			from_timer(plat_priv, timer, fw_boot_timer);
	if (!plat_priv)
		return;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		cnss_pci_fw_boot_timeout_hdlr(plat_priv->bus_priv);
		break;
	case CNSS_BUS_AHB:
		break;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
	}
}

void cnss_bus_collect_dump_info(struct cnss_plat_data *plat_priv, bool in_panic)
{
	if (!plat_priv)
		return;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_collect_dump_info(plat_priv->bus_priv,
						  in_panic);
	case CNSS_BUS_AHB:
		return;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return;
	}
}

int cnss_bus_call_driver_probe(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_call_driver_probe(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_call_driver_remove(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_call_driver_remove(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_dev_powerup(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_dev_powerup(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_dev_shutdown(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_dev_shutdown(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_dev_crash_shutdown(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_dev_crash_shutdown(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_dev_ramdump(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_dev_ramdump(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_register_driver_hdlr(struct cnss_plat_data *plat_priv, void *data)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_register_driver_hdlr(plat_priv->bus_priv, data);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_unregister_driver_hdlr(struct cnss_plat_data *plat_priv)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_unregister_driver_hdlr(plat_priv->bus_priv);
	case CNSS_BUS_AHB:
		return 0;
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_call_driver_modem_status(struct cnss_plat_data *plat_priv,
				      int modem_current_status)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_call_driver_modem_status(plat_priv->bus_priv,
							 modem_current_status);
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}

int cnss_bus_update_status(struct cnss_plat_data *plat_priv,
			   enum cnss_driver_status status)
{
	if (!plat_priv)
		return -ENODEV;

	switch (plat_priv->bus_type) {
	case CNSS_BUS_PCI:
		return cnss_pci_update_status(plat_priv->bus_priv, status);
	default:
		cnss_pr_err("Unsupported bus type: %d\n",
			    plat_priv->bus_type);
		return -EINVAL;
	}
}
