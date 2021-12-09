/*
 * Copyright (C) 2015, Bin Meng <bmeng.cn@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/irq.h>
#include <asm/pci.h>
#include <asm/post.h>
#include <asm/processor.h>
#include <asm/arch/device.h>
#include <asm/arch/qemu.h>

static bool i440fx;

static void qemu_chipset_init(void)
{
	u16 device, xbcs;
	int pam, i;

	/*
	 * i440FX and Q35 chipset have different PAM register offset, but with
	 * the same bitfield layout. Here we determine the offset based on its
	 * PCI device ID.
	 */
	device = x86_pci_read_config16(PCI_BDF(0, 0, 0), PCI_DEVICE_ID);
	i440fx = (device == PCI_DEVICE_ID_INTEL_82441);
	pam = i440fx ? I440FX_PAM : Q35_PAM;

	/*
	 * Initialize Programmable Attribute Map (PAM) Registers
	 *
	 * Configure legacy segments C/D/E/F to system RAM
	 */
	for (i = 0; i < PAM_NUM; i++)
		x86_pci_write_config8(PCI_BDF(0, 0, 0), pam + i, PAM_RW);

	if (i440fx) {
		/*
		 * Enable legacy IDE I/O ports decode
		 *
		 * Note: QEMU always decode legacy IDE I/O port on PIIX chipset.
		 * However Linux ata_piix driver does sanity check on these two
		 * registers to see whether legacy ports decode is turned on.
		 * This is to make Linux ata_piix driver happy.
		 */
		x86_pci_write_config16(PIIX_IDE, IDE0_TIM, IDE_DECODE_EN);
		x86_pci_write_config16(PIIX_IDE, IDE1_TIM, IDE_DECODE_EN);

		/* Enable I/O APIC */
		xbcs = x86_pci_read_config16(PIIX_ISA, XBCS);
		xbcs |= APIC_EN;
		x86_pci_write_config16(PIIX_ISA, XBCS, xbcs);
	} else {
		/* Configure PCIe ECAM base address */
		x86_pci_write_config32(PCI_BDF(0, 0, 0), PCIEX_BAR,
				       CONFIG_PCIE_ECAM_BASE | BAR_EN);
	}
}

int arch_cpu_init(void)
{
	int ret;

	post_code(POST_CPU_INIT);

	ret = x86_cpu_init_f();
	if (ret)
		return ret;

	return 0;
}

#ifndef CONFIG_EFI_STUB
int print_cpuinfo(void)
{
	post_code(POST_CPU_INFO);
	return default_print_cpuinfo();
}
#endif

void reset_cpu(ulong addr)
{
	/* cold reset */
	x86_full_reset();
}

int arch_early_init_r(void)
{
	qemu_chipset_init();

	return 0;
}

int arch_misc_init(void)
{
	return pirq_init();
}

#ifdef CONFIG_GENERATE_MP_TABLE
int mp_determine_pci_dstirq(int bus, int dev, int func, int pirq)
{
	u8 irq;

	if (i440fx) {
		/*
		 * Not like most x86 platforms, the PIRQ[A-D] on PIIX3 are not
		 * connected to I/O APIC INTPIN#16-19. Instead they are routed
		 * to an irq number controled by the PIRQ routing register.
		 */
		irq = x86_pci_read_config8(PCI_BDF(bus, dev, func),
					   PCI_INTERRUPT_LINE);
	} else {
		/*
		 * ICH9's PIRQ[A-H] are not consecutive numbers from 0 to 7.
		 * PIRQ[A-D] still maps to [0-3] but PIRQ[E-H] maps to [8-11].
		 */
		irq = pirq < 8 ? pirq + 16 : pirq + 12;
	}

	return irq;
}
#endif
