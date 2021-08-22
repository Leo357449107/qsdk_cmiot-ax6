/* Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include <linux/msi.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#define to_qgic2_msi(n) container_of(n, struct qgic2_msi, dev)

/*
 * Maximum number of MSI IRQs per qgicm address.
 */
#define MAX_MSI_IRQS		32
#define APCS_QGIC2M_0		0
#define APCS_QGIC2M_1		1

struct qgic2_msi_controller;

struct qgic2_msi {
	struct device			*dev;
	int				msi[MAX_MSI_IRQS];
	unsigned int			base_irq;
	int				num_irqs;
	int				nvec_used;
	u32				msi_gicm_addr;
	u32				msi_gicm_base;
	DECLARE_BITMAP(msi_irq_in_use, MAX_MSI_IRQS);
	struct msi_desc			*desc;
	struct qgic2_msi_controller	*chip;
	__u8				msi_enabled:1;
	__u8				no_64bit_msi:1; /* device may only use 32-bit MSIs */
};

struct qgic2_msi_controller {
	struct module *owner;
	struct device *dev;
	struct device_node *of_node;
	struct list_head list;

	int (*setup_irq)(struct qgic2_msi_controller *chip, struct qgic2_msi *qgic);
	int (*setup_irqs)(struct qgic2_msi_controller *chip, struct qgic2_msi *qgic, int nvec);
	void (*teardown_irq)(struct qgic2_msi *qgic, unsigned int irq);
	void (*teardown_irqs)(struct qgic2_msi *qgic);
};

struct qgic2_msi *get_qgic2_struct(int qgicm_id);

#ifdef CONFIG_QGIC2_MSI
/* qgic2_enable_msi() function to enable QGIC MSI irqs
 * qgicm_id	- INDEX of the QGIC2M MSI address space
 * num_vectors	- Number of IRQ vectors to enable
 *
 * It returns qgic2_msi* pointer on success and
 * ERR_PTR on failure.
 */
struct qgic2_msi *qgic2_enable_msi(int qgicm_id, int num_vectors);
void qgic2_disable_msi(int qgicm_id);
#else
static inline struct qgic2_msi *qgic2_enable_msi(int qgicm_id, int num_vectors)
{
	return NULL;
}

static inline void qgic2_disable_msi(int qgicm_id)
{
}
#endif
