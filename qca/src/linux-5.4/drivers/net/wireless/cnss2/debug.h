/* Copyright (c) 2016-2018, The Linux Foundation. All rights reserved.
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

#ifndef _CNSS_DEBUG_H
#define _CNSS_DEBUG_H

#include <linux/ipc_logging.h>
#include <linux/printk.h>
#include <linux/of.h>

#define CNSS_IPC_LOG_PAGES		32

enum cnss_log_level {
	CNSS_LOG_LEVEL_NONE,
	CNSS_LOG_LEVEL_ERROR,
	CNSS_LOG_LEVEL_WARN,
	CNSS_LOG_LEVEL_INFO,
	CNSS_LOG_LEVEL_DEBUG,
	CNSS_LOG_LEVEL_MAX
};

extern int log_level;

extern void *cnss_ipc_log_context;
extern void *cnss_ipc_log_long_context;

#define cnss_ipc_log_string(_x...) do {					\
		if (cnss_ipc_log_context)				\
			ipc_log_string(cnss_ipc_log_context, _x);	\
	} while (0)

#define cnss_ipc_log_long_string(_x...) do {				\
		if (cnss_ipc_log_long_context)				\
			ipc_log_string(cnss_ipc_log_long_context, _x);	\
	} while (0)

#define cnss_pr_err(_fmt, ...) do {					\
		if (plat_priv) {					\
			pr_err("cnss[%x]: ERR: " _fmt,			\
			       plat_priv->wlfw_service_instance_id,	\
			       ##__VA_ARGS__);				\
			cnss_ipc_log_string("[%x] ERR: " pr_fmt(_fmt),	\
					    plat_priv->			\
					    wlfw_service_instance_id,	\
					    ##__VA_ARGS__);		\
		} else {						\
			pr_err("cnss: ERR: " _fmt, ##__VA_ARGS__);	\
		}							\
	} while (0)

#define cnss_pr_warn(_fmt, ...) do {					\
		if (plat_priv) {					\
			if (log_level >= CNSS_LOG_LEVEL_WARN)		\
				pr_err("cnss[%x]: WARN: " _fmt,		\
				       plat_priv->			\
				       wlfw_service_instance_id,	\
				       ##__VA_ARGS__);			\
			else						\
				pr_warn("cnss[%x]: WARN: " _fmt,	\
					plat_priv->			\
					wlfw_service_instance_id,	\
					##__VA_ARGS__);			\
			cnss_ipc_log_string("[%x] WRN: " pr_fmt(_fmt),	\
					    plat_priv->			\
					    wlfw_service_instance_id,	\
					    ##__VA_ARGS__);		\
		} else {						\
			pr_err("cnss: WARN: " _fmt, ##__VA_ARGS__);	\
		}							\
	} while (0)

#define cnss_pr_info(_fmt, ...) do {					\
		if (plat_priv) {					\
			if (log_level >= CNSS_LOG_LEVEL_INFO)		\
				pr_err("cnss[%x]: INFO: " _fmt,		\
				       plat_priv->			\
				       wlfw_service_instance_id,	\
				       ##__VA_ARGS__);			\
			else						\
				pr_info("cnss[%x]: INFO: " _fmt,	\
					plat_priv->			\
					wlfw_service_instance_id,	\
					##__VA_ARGS__);			\
			cnss_ipc_log_string("[%x] INF: " pr_fmt(_fmt),	\
					    plat_priv->			\
					    wlfw_service_instance_id,	\
					    ##__VA_ARGS__);		\
		} else {						\
			pr_err("cnss: INFO: " _fmt, ##__VA_ARGS__);	\
		}							\
	} while (0)

#define cnss_pr_dbg(_fmt, ...) do {					\
		if (plat_priv) {					\
			if (log_level >= CNSS_LOG_LEVEL_DEBUG)		\
				pr_err("cnss[%x]: DBG: " _fmt,		\
				       plat_priv->			\
				       wlfw_service_instance_id,	\
				       ##__VA_ARGS__);			\
			else						\
				pr_debug("cnss[%x]: DBG: " _fmt,	\
					 plat_priv->			\
					 wlfw_service_instance_id,	\
					 ##__VA_ARGS__);		\
			cnss_ipc_log_string("[%x] DBG: " pr_fmt(_fmt),	\
					    plat_priv->			\
					    wlfw_service_instance_id,	\
					    ##__VA_ARGS__);		\
		} else {						\
			pr_err("cnss: DBG: " _fmt, ##__VA_ARGS__);	\
		}							\
	} while (0)

#define cnss_pr_vdbg(_fmt, ...) do {					\
		pr_err("cnss: " _fmt, ##__VA_ARGS__);	\
		cnss_ipc_log_long_string("%scnss: " _fmt, "",		\
					 ##__VA_ARGS__);		\
	} while (0)

#define CNSS_ASSERT(_condition) do {					\
		if (!(_condition)) {					\
			cnss_dump_qmi_history();			\
			cnss_pr_err("ASSERT at line %d\n",		\
				    __LINE__);				\
			BUG_ON(1);					\
		}							\
	} while (0)

#define cnss_fatal_err(_fmt, ...) do {					\
		if (plat_priv) {					\
			pr_err("cnss[%x]: FATAL: " _fmt,		\
			       plat_priv->wlfw_service_instance_id,	\
			       ##__VA_ARGS__);				\
		} else {						\
			pr_err("cnss: FATAL: " _fmt, ##__VA_ARGS__);	\
		}							\
	} while (0)

int cnss_debug_init(void);
void cnss_debug_deinit(void);
int cnss_debugfs_create(struct cnss_plat_data *plat_priv);
void cnss_debugfs_destroy(struct cnss_plat_data *plat_priv);
void qmi_record(u8 instance_id, u16 msg_id, s8 error_msg, s8 resp_err_msg);

#endif /* _CNSS_DEBUG_H */
