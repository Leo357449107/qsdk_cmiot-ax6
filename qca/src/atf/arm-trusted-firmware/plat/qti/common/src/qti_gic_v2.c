/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include <drivers/arm/gicv2.h>
#include <drivers/arm/gic_common.h>
#include <bl31/interrupt_mgmt.h>
#include <plat_qti.h>
#include <platform.h>
#include <platform_def.h>
#include <qtiseclib_defs.h>

/* Group0 interrupts list (separated by comma) */


#define PLAT_QTI_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(QTISECLIB_INT_ID_CPU_WAKEUP_SGI, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_LEVEL),           \
	INTR_PROP_DESC(QTISECLIB_INT_ID_SEC_WDOG_BARK, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_EDGE),            \
	INTR_PROP_DESC(QTISECLIB_INT_ID_NON_SEC_WDOG_BITE, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_LEVEL),			\
	INTR_PROP_DESC(QTISECLIB_INT_ID_RESET_SGI, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(QTISECLIB_INT_ID_SYSTEM_NOC_ERROR, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(QTISECLIB_INT_ID_PC_NOC_ERROR, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(QTISECLIB_INT_ID_MEM_NOC_ERROR, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(QTISECLIB_INT_ID_AHB_TIMEOUT, GIC_HIGHEST_SEC_PRIORITY, \
			grp, GIC_INTR_CFG_EDGE), \
        INTR_PROP_DESC(QTISECLIB_INT_ID_VMIDMT_ERR_CLT_SEC, GIC_HIGHEST_SEC_PRIORITY, grp, \
                        GIC_INTR_CFG_EDGE), \
        INTR_PROP_DESC(QTISECLIB_INT_ID_VMIDMT_ERR_CLT_NONSEC, GIC_HIGHEST_SEC_PRIORITY, grp, \
                        GIC_INTR_CFG_EDGE), \
        INTR_PROP_DESC(QTISECLIB_INT_ID_VMIDMT_ERR_CFG_SEC, GIC_HIGHEST_SEC_PRIORITY, grp, \
                        GIC_INTR_CFG_EDGE), \
        INTR_PROP_DESC(QTISECLIB_INT_ID_VMIDMT_ERR_CFG_NONSEC, GIC_HIGHEST_SEC_PRIORITY, grp, \
                        GIC_INTR_CFG_EDGE), \
        INTR_PROP_DESC(QTISECLIB_INT_ID_XPU_SEC , GIC_HIGHEST_SEC_PRIORITY, grp, \
                        GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(QTISECLIB_INT_ID_XPU_NON_SEC, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE)

/* Array of o be configured by the gic driver */
static const interrupt_prop_t qti_interrupt_props[] = {
	PLAT_QTI_G1S_IRQ_PROPS(GICV2_INTR_GROUP0)
};

static unsigned int target_mask_array[PLATFORM_CORE_COUNT];
const gicv2_driver_data_t qti_gic_data = {
	.gicd_base = QTI_GICD_BASE,
	.gicc_base = QTI_GICC_BASE,
	.interrupt_props = qti_interrupt_props,
	.interrupt_props_num = ARRAY_SIZE(qti_interrupt_props),
	.target_masks = target_mask_array,
	.target_masks_num = ARRAY_SIZE(target_mask_array),
};

void gic_set_spi_routing(unsigned int id, unsigned int irm, u_register_t target)
{
	return;
}

void plat_qti_gic_driver_init(void)
{
	/*
	 * The GICv3 driver is initialized in EL3 and does not need
	 * to be initialized again in SEL1. This is because the S-EL1
	 * can use GIC system registers to manage interrupts and does
	 * not need GIC interface base addresses to be configured.
	 */
	gicv2_driver_init(&qti_gic_data);
}

/******************************************************************************
 * ARM common helper to initialize the GIC. Only invoked by BL31
 *****************************************************************************/
void plat_qti_gic_init(void)
{

	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_set_pe_target_mask(plat_my_core_pos());
	gicv2_cpuif_enable();
}

/******************************************************************************
 * ARM common helper to enable the GIC CPU interface
 *****************************************************************************/
void plat_qti_gic_cpuif_enable(void)
{
    gicv2_cpuif_enable();
    gicv2_set_pe_target_mask(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helper to disable the GIC CPU interface
 *****************************************************************************/
void plat_qti_gic_cpuif_disable(void)
{
	gicv2_cpuif_disable();
}

/******************************************************************************
 * ARM common helper to initialize the per-cpu redistributor interface in GICv3
 *****************************************************************************/
void plat_qti_gic_pcpu_init(void)
{
	gicv2_pcpu_distif_init();
	gicv2_set_pe_target_mask(plat_my_core_pos());
}

/******************************************************************************
 * ARM common helpers to power GIC redistributor interface
 *****************************************************************************/
void plat_qti_gic_redistif_on(void)
{
}

void plat_qti_gic_redistif_off(void)
{
}
