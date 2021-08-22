/*
 * Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

#include <linux/cpu.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <soc/qcom/socinfo.h>

#define IPQ60XX_SUPPORTED_SOC 5

#define IPQ6018_BIT BIT(0)
#define IPQ6028_BIT BIT(1)
#define IPQ6010_BIT BIT(2)
#define IPQ6000_BIT BIT(3)
#define IPQ6005_BIT BIT(4)

struct ipq_socinfo {
	int soc_id;
	int soc_bit;
};

struct ipq_socinfo ipq60xx_info[IPQ60XX_SUPPORTED_SOC] = {
	{CPU_IPQ6018, IPQ6018_BIT},
	{CPU_IPQ6028, IPQ6028_BIT},
	{CPU_IPQ6010, IPQ6010_BIT},
	{CPU_IPQ6000, IPQ6000_BIT},
	{CPU_IPQ6005, IPQ6005_BIT},
};

/* cpufreq-dt device registered by ipq-cpufreq-dt */
static struct platform_device *cpufreq_dt_pdev;
static struct device *cpu_dev;

static int __init ipq60xx_cpufreq_init(void)
{
	u32 supported_hw[1] = {0};
	int ret, cpu_id, i;

	if (!of_machine_is_compatible("qcom,ipq6018")) {
		return -ENODEV;
	}

	cpu_dev = get_cpu_device(0);

	cpu_id = read_ipq_cpu_type();

	for(i=0; i<IPQ60XX_SUPPORTED_SOC; i++) {
		if(cpu_id == ipq60xx_info[i].soc_id) {
			supported_hw[0] = ipq60xx_info[i].soc_bit;
			break;
		}
	}

	/* Fallback if SoC ID is invalid */
	if (!supported_hw[0])
		supported_hw[0] = IPQ6018_BIT;

	ret = dev_pm_opp_set_supported_hw(cpu_dev, supported_hw, 1);
	if (ret) {
		pr_err("Failed to set supported opp: %d\n", ret);
		return ret;
	}

	cpufreq_dt_pdev = platform_device_register_simple("cpufreq-dt", -1, NULL, 0);

	if (IS_ERR(cpufreq_dt_pdev)) {
		dev_pm_opp_put_supported_hw(cpu_dev);
		ret = PTR_ERR(cpufreq_dt_pdev);
		pr_err("Failed to register cpufreq-dt: %d\n", ret);
		return ret;
	}

	return 0;
}

module_init(ipq60xx_cpufreq_init);

MODULE_DESCRIPTION("Qualcomm technologies Inc IPQ60xx cpufreq-dt init driver");
MODULE_LICENSE("GPL V2");
