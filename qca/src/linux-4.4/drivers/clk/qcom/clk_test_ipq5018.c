/*
 * Copyright (c) 2019, 2020 The Linux Foundation. All rights reserved.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/reset.h>

static int test = -EINVAL;
char *clk_name = "clk";
unsigned long clk_freq = 24000000;

/* Supported Test Operations
 *  SET_RATE		0
 *  PREPARE_ENABLE	1
 *  DISABLE_UNPREPARE	2
 *  RESETS		3
 */
module_param(test, int, S_IRUGO);
MODULE_PARM_DESC(test, "test no");

module_param(clk_name, charp, S_IRUGO);
MODULE_PARM_DESC(clk_name, "Clock name");

module_param(clk_freq, ulong, S_IRUGO);
MODULE_PARM_DESC(clk_freq, "Clock freq");

static int clk_test_probe(struct platform_device *pdev)
{
	struct clk * core_clk;
	struct reset_control *temp_reset;
	int ret_val = 0;

	if (test == 3) {
		temp_reset = devm_reset_control_get(&pdev->dev, clk_name);
		if (IS_ERR(temp_reset)) {
			printk(KERN_ALERT "Error in getting reset %s\n", clk_name);
			return 0;
		}
		ret_val = reset_control_deassert(temp_reset);

		printk(KERN_ERR "clk-test test-id#%d for %s is %s\n", test, clk_name, ret_val ? "FAILURE" : "SUCCESS");
		return 0;
	}

	core_clk = devm_clk_get(&pdev->dev, clk_name);
	if (IS_ERR(core_clk)) {
		printk(KERN_ALERT "Error in getting clk %s\n", clk_name);
		return 0;
	}

	switch(test) {
		case 0:
			ret_val = clk_set_rate(core_clk, clk_freq);
			break;
		case 1:
			ret_val = clk_prepare_enable(core_clk);
			break;
		case 2:
			clk_disable_unprepare(core_clk);
			break;
		case 4:
			ret_val = clk_prepare_enable(core_clk);
			clk_disable_unprepare(core_clk);
			if (!ret_val)
				ret_val = clk_set_rate(core_clk, clk_freq);
			break;
		default:
			break;
	}

	printk(KERN_ERR "clk-test test-id#%d for %s is %s\n", test, clk_name, ret_val ? "FAILURE" : "SUCCESS");

	return 0;
}

static struct of_device_id clk_test_match[] = {
	{       .compatible = "clk-test",
	},
	{}
};

static struct platform_driver clk_test_driver = {
	.probe  = clk_test_probe,
	.driver = {
		.name           = "clk-test",
		.owner          = THIS_MODULE,
		.of_match_table = clk_test_match,
	},
};

static int __init clk_test_init(void)
{
	int ret;
	ret = platform_driver_register(&clk_test_driver);
	if(ret == 0) {
		printk(KERN_ALERT "Registered Sucessfully \n");
	} else {
		printk(KERN_ALERT "Registered Failed \n");
	}

	return 0;
}
module_init(clk_test_init);

static void __exit clk_test_exit(void)
{
	platform_driver_unregister(&clk_test_driver);
	return;
}
module_exit(clk_test_exit);

MODULE_DESCRIPTION("clk test module");
MODULE_LICENSE("Dual BSD/GPL");
