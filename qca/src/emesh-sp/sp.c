/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include "sp_mapdb.h"
#include "sp_types.h"

static int sp_state;

/* SPM pre routing hook enabled as default */
int sp_sysctl_data = 1;

/*
 * sp_enable_handler()
 * 	Handler callback function for enabling/disabling SPM on runtime.
 */
static int sp_enable_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;
	int data;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	data = !!sp_sysctl_data;

	if (ret || !write) {
		return ret;
	}

	if (data >= sp_state) {
		DEBUG_WARN("Invalid action(already in this state).\n");
		return 0;
	}

	if (data) {
		sp_mapdb_ruletable_flush();
		if (!sp_hook_init()) {
			sp_state = 1;
		}
	} else {
		sp_hook_fini();
		sp_state = 0;
	}

	return 0;
}

/*
 * sp_ruledb_handler()
 * 	The handler function for printing the rule table for SPM.
 */
static int sp_ruledb_handler(struct ctl_table *ctl, int write, void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec(ctl, write, buffer, lenp, ppos);
	if (write) {
		return ret;
	}

	sp_mapdb_ruletable_print();

	*lenp = 0;
	return ret;
}

static struct ctl_table sp_sysctl_emesh_table[] = {
	{
		.procname	= "enable",
		.maxlen		= sizeof(int),
		.data 		= &sp_sysctl_data,
		.mode		= 0644,
		.proc_handler	= sp_enable_handler,
	},
	{
		.procname	= "ruledb",
		.mode		= 0444,
		.data 		= NULL,
		.proc_handler	= sp_ruledb_handler,
	},

	{ }
};

static struct ctl_table sp_sysctl_root_dir[] = {
	{
		.procname	= "emesh-sp",
		.mode		= 0555,
		.child		= sp_sysctl_emesh_table,
	},
	{ }
};

static struct ctl_table sp_sysctl_root[] = {
	{
		.procname	= "net",
		.mode		= 0555,
		.child		= sp_sysctl_root_dir,
	},
	{ }
};

static struct ctl_table_header *sp_sysctl_header;

/*
 * sp_init()
 * 	Initialize the SPM module.
 *
 * This includes initialize sp_mapdb and sysctl
 * which enables sysctl entries for enabling/disabling SPM in runtime and
 * print rule database.
 */
static int __init sp_init(void)
{
	sp_mapdb_init();
	sp_sysctl_header = register_sysctl_table(sp_sysctl_root);

	DEBUG_TRACE("Service Prioritization Module loaded successfully.\n");
	return 0;
}

/*
 * sp_exit()
 * 	Unload SPM module.
 */
static void __exit sp_exit(void)
{
	if (sp_sysctl_header) {
		unregister_sysctl_table(sp_sysctl_header);
	}

	if (sp_state == 1) {
		sp_hook_fini();
	}

	sp_mapdb_fini();

	DEBUG_TRACE("Service Prioritization Module unloaded!\n");
}

module_init(sp_init);
module_exit(sp_exit);

MODULE_LICENSE("Dual BSD/GPL");
