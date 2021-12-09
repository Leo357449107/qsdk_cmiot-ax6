/*-
 * Copyright (c) 2007-2008, Juniper Networks, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0
 */

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <pci.h>
#include <usb.h>

#include "ehci.h"

/* Information about a USB port */
struct ehci_pci_priv {
	struct ehci_ctrl ehci;
};

static void ehci_pci_common_init(pci_dev_t pdev, struct ehci_hccr **ret_hccr,
				 struct ehci_hcor **ret_hcor)
{
	struct ehci_hccr *hccr;
	struct ehci_hcor *hcor;
	uint32_t cmd;

	hccr = (struct ehci_hccr *)pci_map_bar(pdev,
			PCI_BASE_ADDRESS_0, PCI_REGION_MEM);
	hcor = (struct ehci_hcor *)((uint32_t) hccr +
			HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	debug("EHCI-PCI init hccr 0x%x and hcor 0x%x hc_length %d\n",
	      (uint32_t)hccr, (uint32_t)hcor,
	      (uint32_t)HC_LENGTH(ehci_readl(&hccr->cr_capbase)));

	*ret_hccr = hccr;
	*ret_hcor = hcor;

	/* enable busmaster */
	pci_read_config_dword(pdev, PCI_COMMAND, &cmd);
	cmd |= PCI_COMMAND_MASTER;
	pci_write_config_dword(pdev, PCI_COMMAND, cmd);
}

#ifndef CONFIG_DM_USB

#ifdef CONFIG_PCI_EHCI_DEVICE
static struct pci_device_id ehci_pci_ids[] = {
	/* Please add supported PCI EHCI controller ids here */
	{0x1033, 0x00E0},	/* NEC */
	{0x10B9, 0x5239},	/* ULI1575 PCI EHCI module ids */
	{0x12D8, 0x400F},	/* Pericom */
	{0, 0}
};
#endif

/*
 * Create the appropriate control structures to manage
 * a new EHCI host controller.
 */
int ehci_hcd_init(int index, enum usb_init_type init,
		struct ehci_hccr **ret_hccr, struct ehci_hcor **ret_hcor)
{
	pci_dev_t pdev;

#ifdef CONFIG_PCI_EHCI_DEVICE
	pdev = pci_find_devices(ehci_pci_ids, CONFIG_PCI_EHCI_DEVICE);
#else
	pdev = pci_find_class(PCI_CLASS_SERIAL_USB_EHCI, index);
#endif
	if (pdev < 0) {
		printf("EHCI host controller not found\n");
		return -1;
	}
	ehci_pci_common_init(pdev, ret_hccr, ret_hcor);

	return 0;
}

/*
 * Destroy the appropriate control structures corresponding
 * the the EHCI host controller.
 */
int ehci_hcd_stop(int index)
{
	return 0;
}
#endif /* nCONFIG_DM_USB */

#ifdef CONFIG_DM_USB
static int ehci_pci_probe(struct udevice *dev)
{
	struct ehci_hccr *hccr;
	struct ehci_hcor *hcor;

	ehci_pci_common_init(pci_get_bdf(dev), &hccr, &hcor);

	return ehci_register(dev, hccr, hcor, NULL, 0, USB_INIT_HOST);
}

static int ehci_pci_remove(struct udevice *dev)
{
	int ret;

	ret = ehci_deregister(dev);
	if (ret)
		return ret;

	return 0;
}

U_BOOT_DRIVER(ehci_pci) = {
	.name	= "ehci_pci",
	.id	= UCLASS_USB,
	.probe = ehci_pci_probe,
	.remove = ehci_pci_remove,
	.ops	= &ehci_usb_ops,
	.platdata_auto_alloc_size = sizeof(struct usb_platdata),
	.priv_auto_alloc_size = sizeof(struct ehci_pci_priv),
	.flags	= DM_FLAG_ALLOC_PRIV_DMA,
};

static struct pci_device_id ehci_pci_supported[] = {
	{ PCI_DEVICE_CLASS(PCI_CLASS_SERIAL_USB_EHCI, ~0) },
	{},
};

U_BOOT_PCI_DEVICE(ehci_pci, ehci_pci_supported);

#endif /* CONFIG_DM_USB */
