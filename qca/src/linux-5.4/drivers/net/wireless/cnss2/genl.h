/* Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#ifndef __CNSS_GENL_H__
#define __CNSS_GENL_H__

#include <net/netlink.h>
#include <net/genetlink.h>

#define CNSS_GENL_STR_LEN_MAX 32
enum cnss_genl_msg_type {
	CNSS_GENL_MSG_TYPE_UNSPEC,
	CNSS_GENL_MSG_TYPE_QDSS,
	CNSS_GENL_MSG_TYPE_DAEMON_SUPPORT,
	CNSS_GENL_MSG_TYPE_COLD_BOOT_SUPPORT,
	CNSS_GENL_MSG_TYPE_HDS_SUPPORT,
	CNSS_GENL_MSG_TYPE_REGDB_SUPPORT,
};

#ifdef CONFIG_CNSS2_GENL
int cnss_genl_init(void);
void cnss_genl_exit(void);
int cnss_genl_send_msg(void *buff, u8 type,
		       char *file_name, u32 total_size);
int cnss_genl_process_msg(struct sk_buff *skb, struct genl_info *info);
#else
static inline int cnss_genl_init(void)
{
	return 0;
}

static inline void cnss_genl_exit(void)
{
}

static inline int cnss_genl_send_msg(void *buff, u8 type,
				     char *file_name, u32 total_size)
{
	return 0;
}

static int cnss_genl_process_msg(struct sk_buff *skb, struct genl_info *info)
{
	return 0;
}
#endif

#endif
