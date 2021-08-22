/* Copyright (c) 2009-2014, 2016 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ARCH_ARM_MACH_MSM_SOCINFO_H_
#define _ARCH_ARM_MACH_MSM_SOCINFO_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/of_fdt.h>
#include <linux/of.h>

#include <asm/cputype.h>
/*
 * SOC version type with major number in the upper 16 bits and minor
 * number in the lower 16 bits.  For example:
 *   1.0 -> 0x00010000
 *   2.3 -> 0x00020003
 */
#define SOCINFO_VERSION_MAJOR(ver) (((ver) & 0xffff0000) >> 16)
#define SOCINFO_VERSION_MINOR(ver) ((ver) & 0x0000ffff)
#define SOCINFO_VERSION(maj, min)  ((((maj) & 0xffff) << 16)|((min) & 0xffff))

#ifdef CONFIG_OF
#define of_board_is_cdp()	of_machine_is_compatible("qti,cdp")
#define of_board_is_sim()	of_machine_is_compatible("qti,sim")
#define of_board_is_rumi()	of_machine_is_compatible("qti,rumi")
#define of_board_is_fluid()	of_machine_is_compatible("qti,fluid")
#define of_board_is_liquid()	of_machine_is_compatible("qti,liquid")
#define of_board_is_dragonboard()	\
	of_machine_is_compatible("qti,dragonboard")
#define of_board_is_cdp()	of_machine_is_compatible("qti,cdp")
#define of_board_is_mtp()	of_machine_is_compatible("qti,mtp")
#define of_board_is_qrd()	of_machine_is_compatible("qti,qrd")
#define of_board_is_xpm()	of_machine_is_compatible("qti,xpm")
#define of_board_is_skuf()	of_machine_is_compatible("qti,skuf")
#define of_board_is_sbc()	of_machine_is_compatible("qti,sbc")

#define machine_is_msm8974()	of_machine_is_compatible("qti,msm8974")
#define machine_is_msm9625()	of_machine_is_compatible("qti,msm9625")
#define machine_is_msm8610()	of_machine_is_compatible("qti,msm8610")
#define machine_is_msm8226()	of_machine_is_compatible("qti,msm8226")
#define machine_is_apq8074()	of_machine_is_compatible("qti,apq8074")
#define machine_is_msm8926()	of_machine_is_compatible("qti,msm8926")

#define early_machine_is_msm8610()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8610")
#define early_machine_is_msm8909()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8909")
#define early_machine_is_msm8916()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8916")
#define early_machine_is_msm8936()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8936")
#define early_machine_is_msm8939()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8939")
#define early_machine_is_apq8084()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,apq8084")
#define early_machine_is_mdm9630()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,mdm9630")
#define early_machine_is_msmzirc()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msmzirc")
#define early_machine_is_fsm9900()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,fsm9900")
#define early_machine_is_msm8994()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8994")
#define early_machine_is_msm8992()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8992")
#define early_machine_is_fsm9010()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,fsm9010")
#define early_machine_is_msm8976()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8976")
#define early_machine_is_msmtellurium()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msmtellurium")
#define early_machine_is_msm8996()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8996")
#define early_machine_is_msm8929()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8929")
#define early_machine_is_msm8998()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msm8998")
#define early_machine_is_apq8098()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,apq8098")
#define early_machine_is_msmhamster()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,msmhamster")
#define early_machine_is_sdm660()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sdm660")
#define early_machine_is_sda660()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sda660")
#define early_machine_is_sdm455()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sdm455")
#define early_machine_is_sdm636()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sdm636")
#define early_machine_is_sda636()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sda636")
#define early_machine_is_sdm658()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sdm658")
#define early_machine_is_sda658()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sda658")
#define early_machine_is_sdm630()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sdm630")
#define early_machine_is_sda630()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qti,sda630")
#define early_machine_is_ipq6018()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,ipq6018")
#define early_machine_is_ipq6028()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,ipq6028")
#define early_machine_is_ipq6000()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,ipq6000")
#define early_machine_is_ipq6010()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,ipq6010")
#define early_machine_is_ipq6005()	\
	of_flat_dt_is_compatible(of_get_flat_dt_root(), "qcom,ipq6005")
#else
#define of_board_is_sim()		0
#define of_board_is_rumi()		0
#define of_board_is_fluid()		0
#define of_board_is_liquid()		0
#define of_board_is_dragonboard()	0
#define of_board_is_cdp()		0
#define of_board_is_mtp()		0
#define of_board_is_qrd()		0
#define of_board_is_xpm()		0
#define of_board_is_skuf()		0
#define of_board_is_sbc()		0

#define machine_is_msm8974()		0
#define machine_is_msm9625()		0
#define machine_is_msm8610()		0
#define machine_is_msm8226()		0
#define machine_is_apq8074()		0
#define machine_is_msm8926()		0

#define early_machine_is_msm8610()	0
#define early_machine_is_msm8909()	0
#define early_machine_is_msm8916()	0
#define early_machine_is_msm8936()	0
#define early_machine_is_msm8939()	0
#define early_machine_is_apq8084()	0
#define early_machine_is_mdm9630()	0
#define early_machine_is_fsm9900()	0
#define early_machine_is_fsm9010()	0
#define early_machine_is_msmtellurium()	0
#define early_machine_is_msm8996()	0
#define early_machine_is_msm8976() 0
#define early_machine_is_msm8929()	0
#define early_machine_is_msm8998()	0
#define early_machine_is_apq8098()	0
#define early_machine_is_msmhamster()	0
#define early_machine_is_sdm660()	0
#define early_machine_is_sda660()	0
#define early_machine_is_sdm455()	0
#define early_machine_is_sdm636()	0
#define early_machine_is_sda636()	0
#define early_machine_is_sdm658()	0
#define early_machine_is_sda658()	0
#define early_machine_is_sdm630()	0
#define early_machine_is_sda630()	0
#define early_machine_is_ipq6018()	0
#define early_machine_is_ipq6028()	0
#define early_machine_is_ipq6000()	0
#define early_machine_is_ipq6010()	0
#define early_machine_is_ipq6005()	0
#endif

#define PLATFORM_SUBTYPE_MDM	1
#define PLATFORM_SUBTYPE_INTERPOSERV3 2
#define PLATFORM_SUBTYPE_SGLTE	6

enum msm_cpu {
	MSM_CPU_UNKNOWN = 0,
	MSM_CPU_7X01,
	MSM_CPU_7X25,
	MSM_CPU_7X27,
	MSM_CPU_8X50,
	MSM_CPU_8X50A,
	MSM_CPU_7X30,
	MSM_CPU_8X55,
	MSM_CPU_8X60,
	MSM_CPU_8960,
	MSM_CPU_8960AB,
	MSM_CPU_7X27A,
	FSM_CPU_9XXX,
	MSM_CPU_7X25A,
	MSM_CPU_7X25AA,
	MSM_CPU_7X25AB,
	MSM_CPU_8064,
	MSM_CPU_8064AB,
	MSM_CPU_8064AA,
	MSM_CPU_8930,
	MSM_CPU_8930AA,
	MSM_CPU_8930AB,
	MSM_CPU_7X27AA,
	MSM_CPU_9615,
	MSM_CPU_8974,
	MSM_CPU_8974PRO_AA,
	MSM_CPU_8974PRO_AB,
	MSM_CPU_8974PRO_AC,
	MSM_CPU_8627,
	MSM_CPU_8625,
	MSM_CPU_9625,
	MSM_CPU_8909,
	MSM_CPU_8916,
	MSM_CPU_8936,
	MSM_CPU_8939,
	MSM_CPU_8226,
	MSM_CPU_8610,
	MSM_CPU_8625Q,
	MSM_CPU_8084,
	MSM_CPU_9630,
	FSM_CPU_9900,
	MSM_CPU_ZIRC,
	MSM_CPU_8994,
	MSM_CPU_8992,
	FSM_CPU_9010,
	MSM_CPU_TELLURIUM,
	MSM_CPU_8996,
	MSM_CPU_8976,
	MSM_CPU_8929,
	MSM_CPU_8998,
	MSM_CPU_HAMSTER,
	MSM_CPU_660,
	MSM_CPU_455,
	MSM_CPU_630,
	MSM_CPU_636,
	IPQ_CPU_6018,
	IPQ_CPU_6028,
	IPQ_CPU_6000,
	IPQ_CPU_6010,
	IPQ_CPU_6005,
};

struct msm_soc_info {
	enum msm_cpu generic_soc_type;
	char *soc_id_string;
};

enum pmic_model {
	PMIC_MODEL_PM8058	= 13,
	PMIC_MODEL_PM8028	= 14,
	PMIC_MODEL_PM8901	= 15,
	PMIC_MODEL_PM8027	= 16,
	PMIC_MODEL_ISL_9519	= 17,
	PMIC_MODEL_PM8921	= 18,
	PMIC_MODEL_PM8018	= 19,
	PMIC_MODEL_PM8015	= 20,
	PMIC_MODEL_PM8014	= 21,
	PMIC_MODEL_PM8821	= 22,
	PMIC_MODEL_PM8038	= 23,
	PMIC_MODEL_PM8922	= 24,
	PMIC_MODEL_PM8917	= 25,
	PMIC_MODEL_UNKNOWN	= 0xFFFFFFFF
};

enum msm_cpu socinfo_get_msm_cpu(void);
uint32_t socinfo_get_id(void);
uint32_t socinfo_get_version(void);
uint32_t socinfo_get_raw_id(void);
char *socinfo_get_build_id(void);
uint32_t socinfo_get_platform_type(void);
uint32_t socinfo_get_platform_subtype(void);
uint32_t socinfo_get_platform_version(void);
uint32_t socinfo_get_serial_number(void);
enum pmic_model socinfo_get_pmic_model(void);
uint32_t socinfo_get_pmic_die_revision(void);
int __init socinfo_init(void) __must_check;

#define CPU_IPQ4018 272
#define CPU_IPQ4019 273
#define CPU_IPQ4028 287
#define CPU_IPQ4029 288

#define CPU_IPQ8062 201
#define CPU_IPQ8064 202
#define CPU_IPQ8066 203
#define CPU_IPQ8065 280
#define CPU_IPQ8068 204
#define CPU_IPQ8069 281

#define CPU_IPQ8074 323
#define CPU_IPQ8072 342
#define CPU_IPQ8076 343
#define CPU_IPQ8078 344
#define CPU_IPQ8070 375
#define CPU_IPQ8071 376

#define CPU_IPQ8072A 389
#define CPU_IPQ8074A 390
#define CPU_IPQ8076A 391
#define CPU_IPQ8078A 392
#define CPU_IPQ8070A 395
#define CPU_IPQ8071A 396

#define CPU_IPQ8172  397
#define CPU_IPQ8173  398
#define CPU_IPQ8174  399

#define CPU_IPQ6018 402
#define CPU_IPQ6028 403
#define CPU_IPQ6000 421
#define CPU_IPQ6010 422
#define CPU_IPQ6005 453

#define CPU_IPQ5010 446
#define CPU_IPQ5018 447
#define CPU_IPQ5028 448
#define CPU_IPQ5000 503
#define CPU_IPQ0509 504
#define CPU_IPQ0518 505

static inline int read_ipq_soc_version_major(void)
{
	const int *prop;
	u32 soc_version_major;

	prop = of_get_property(of_find_node_by_path("/"), "soc_version_major",
				NULL);
	if(!prop)
		return -EINVAL;

	soc_version_major = *prop;
	soc_version_major = le32_to_cpu(soc_version_major);

	return soc_version_major;
}

static inline int read_ipq_soc_version_minor(void)
{
	const int *prop;
	u32 soc_version_minor;

	prop = of_get_property(of_find_node_by_path("/"), "soc_version_minor",
				NULL);
	if(!prop)
		return -EINVAL;

	soc_version_minor = *prop;
	soc_version_minor = le32_to_cpu(soc_version_minor);

	return soc_version_minor;
}

static inline int read_ipq_cpu_type(void)
{
	const int *prop;
	u32 cpu_type;

	prop = of_get_property(of_find_node_by_path("/"), "cpu_type", NULL);
	/*
	 * Return Default CPU type if "cpu_type" property is not found in DTSI
	 */
	if (!prop)
		return CPU_IPQ8064;

	cpu_type = *prop;
	cpu_type = le32_to_cpu(cpu_type);

	return cpu_type;
}

static inline int cpu_is_ipq4018(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ4018;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq4019(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ4019;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq4028(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ4028;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq4029(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ4029;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq40xx(void)
{
#ifdef CONFIG_ARCH_QCOM
	return  cpu_is_ipq4018() || cpu_is_ipq4019() ||
		cpu_is_ipq4028() || cpu_is_ipq4029();
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8062(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8062;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8064(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8064;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8066(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8066;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8068(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8068;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8065(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8065;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8069(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8069;
#else
	return 0;
#endif
}
static inline int cpu_is_ipq806x(void)
{
#ifdef CONFIG_ARCH_QCOM
	return  cpu_is_ipq8062() || cpu_is_ipq8064() ||
		cpu_is_ipq8066() || cpu_is_ipq8068() ||
		cpu_is_ipq8065() || cpu_is_ipq8069();
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8070(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8070;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8071(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8071;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8072(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8072;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8074(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8074;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8076(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8076;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8078(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8078;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8072a(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8072A;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8074a(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8074A;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8076a(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8076A;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8078a(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8078A;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8070a(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8070A;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8071a(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8071A;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8172(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8172;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8173(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8173;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq8174(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ8174;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq6018(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ6018;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq6028(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ6028;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq6000(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ6000;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq6010(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ6010;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq6005(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ6005;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq5010(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ5010;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq5018(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ5018;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq5028(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ5028;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq5000(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ5000;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq0509(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ0509;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq0518(void)
{
#ifdef CONFIG_ARCH_QCOM
	return read_ipq_cpu_type() == CPU_IPQ0518;
#else
	return 0;
#endif
}

static inline int cpu_is_ipq807x(void)
{
#ifdef CONFIG_ARCH_QCOM
	return  cpu_is_ipq8072() || cpu_is_ipq8074() ||
		cpu_is_ipq8076() || cpu_is_ipq8078() ||
		cpu_is_ipq8070() || cpu_is_ipq8071() ||
		cpu_is_ipq8072a() || cpu_is_ipq8074a() ||
		cpu_is_ipq8076a() || cpu_is_ipq8078a() ||
		cpu_is_ipq8070a() || cpu_is_ipq8071a() ||
		cpu_is_ipq8172() || cpu_is_ipq8173() ||
		cpu_is_ipq8174();
#else
	return 0;
#endif
}

static inline int cpu_is_ipq60xx(void)
{
#ifdef CONFIG_ARCH_QCOM
	return  cpu_is_ipq6018() || cpu_is_ipq6028() ||
		cpu_is_ipq6000() || cpu_is_ipq6010() ||
		cpu_is_ipq6005();
#else
	return 0;
#endif
}

static inline int cpu_is_ipq50xx(void)
{
#ifdef CONFIG_ARCH_QCOM
	return  cpu_is_ipq5010() || cpu_is_ipq5018() ||
		cpu_is_ipq5028() || cpu_is_ipq5000() ||
		cpu_is_ipq0509() || cpu_is_ipq0518();
#else
	return 0;
#endif
}

#endif /* _ARCH_ARM_MACH_MSM_SOCINFO_H_ */
