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

#define pr_fmt(fmt) "cnss_genl: " fmt

#include <linux/err.h>
#include <linux/module.h>

#include "genl.h"
#include "main.h"
#include "debug.h"

#define CNSS_GENL_FAMILY_NAME "cnss-genl"
#define CNSS_GENL_MCAST_GROUP_NAME "cnss-genl-grp"
#define CNSS_GENL_VERSION 1
#define CNSS_GENL_DATA_LEN_MAX 4000

enum {
	CNSS_GENL_ATTR_MSG_UNSPEC,
	CNSS_GENL_ATTR_MSG_TYPE,
	CNSS_GENL_ATTR_MSG_FILE_NAME,
	CNSS_GENL_ATTR_MSG_TOTAL_SIZE,
	CNSS_GENL_ATTR_MSG_SEG_ID,
	CNSS_GENL_ATTR_MSG_END,
	CNSS_GENL_ATTR_MSG_DATA_LEN,
	CNSS_GENL_ATTR_MSG_DATA,
	CNSS_GENL_ATTR_MSG_INSTANCE_ID,
	CNSS_GENL_ATTR_MSG_VALUE,
	__CNSS_GENL_ATTR_MAX,
};

#define CNSS_GENL_ATTR_MAX (__CNSS_GENL_ATTR_MAX - 1)

enum {
	CNSS_GENL_CMD_UNSPEC,
	CNSS_GENL_CMD_MSG,
	__CNSS_GENL_CMD_MAX,
};

#define CNSS_GENL_CMD_MAX (__CNSS_GENL_CMD_MAX - 1)

static struct genl_ops cnss_genl_ops[] = {
	{
		.cmd = CNSS_GENL_CMD_MSG,
		.doit = cnss_genl_process_msg,
	},
};

static struct genl_multicast_group cnss_genl_mcast_grp[] = {
	{
		.name = CNSS_GENL_MCAST_GROUP_NAME,
	},
};

static struct genl_family cnss_genl_family = {
	.id = 0,
	.hdrsize = 0,
	.name = CNSS_GENL_FAMILY_NAME,
	.version = CNSS_GENL_VERSION,
	.maxattr = CNSS_GENL_ATTR_MAX,
	.module = THIS_MODULE,
	.ops = cnss_genl_ops,
	.n_ops = ARRAY_SIZE(cnss_genl_ops),
	.mcgrps = cnss_genl_mcast_grp,
	.n_mcgrps = ARRAY_SIZE(cnss_genl_mcast_grp),
};

int cnss_genl_process_msg(struct sk_buff *skb, struct genl_info *info)
{
	struct nlmsghdr *nl_header = nlmsg_hdr(skb);
	struct genlmsghdr *genl_header = nlmsg_data(nl_header);
	struct nlattr *attrs[CNSS_GENL_ATTR_MAX + 1];
	int ret = 0;
	u8 type;
	u32 instance_id;
	u32 value;

	if (genl_header->cmd != CNSS_GENL_CMD_MSG) {
		pr_err("%s: Invalid cmd %d on NL", __func__, genl_header->cmd);
		return -EINVAL;
	}

	ret = genlmsg_parse(nl_header, &cnss_genl_family, attrs,
			    CNSS_GENL_ATTR_MAX, NULL, NULL);
	if (ret < 0) {
		pr_err("%s: RX NLMSG: Parse fail %d", __func__, ret);
		return -EINVAL;
	}

	type = nla_get_u8(attrs[CNSS_GENL_ATTR_MSG_TYPE]);
	instance_id = nla_get_u32(attrs[CNSS_GENL_ATTR_MSG_INSTANCE_ID]);
	value = nla_get_u32(attrs[CNSS_GENL_ATTR_MSG_VALUE]);

	cnss_update_platform_feature_support(type, instance_id, value);

	return 0;
}

static int cnss_genl_send_data(u8 type, char *file_name, u32 total_size,
			       u32 seg_id, u8 end, u32 data_len, u8 *msg_buff)
{
	struct sk_buff *skb = NULL;
	void *msg_header = NULL;
	int ret = 0;
	char filename[CNSS_GENL_STR_LEN_MAX + 1];

	pr_debug("type:%u, file:%s, size:%x, segid:%u, end:%u, datalen:%u\n",
		 type, file_name, total_size, seg_id, end, data_len);

	if (!file_name)
		strlcpy(filename, "default", sizeof(filename));
	else
		strlcpy(filename, file_name, sizeof(filename));

	skb = genlmsg_new(NLMSG_HDRLEN +
			  nla_total_size(sizeof(type)) +
			  nla_total_size(strlen(filename) + 1) +
			  nla_total_size(sizeof(total_size)) +
			  nla_total_size(sizeof(seg_id)) +
			  nla_total_size(sizeof(end)) +
			  nla_total_size(sizeof(data_len)) +
			  nla_total_size(data_len), GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

	msg_header = genlmsg_put(skb, 0, 0,
				 &cnss_genl_family, 0,
				 CNSS_GENL_CMD_MSG);
	if (!msg_header) {
		ret = -ENOMEM;
		goto fail;
	}

	ret = nla_put_u8(skb, CNSS_GENL_ATTR_MSG_TYPE, type);
	if (ret < 0)
		goto fail;
	ret = nla_put_string(skb, CNSS_GENL_ATTR_MSG_FILE_NAME, filename);
	if (ret < 0)
		goto fail;
	ret = nla_put_u32(skb, CNSS_GENL_ATTR_MSG_TOTAL_SIZE, total_size);
	if (ret < 0)
		goto fail;
	ret = nla_put_u32(skb, CNSS_GENL_ATTR_MSG_SEG_ID, seg_id);
	if (ret < 0)
		goto fail;
	ret = nla_put_u8(skb, CNSS_GENL_ATTR_MSG_END, end);
	if (ret < 0)
		goto fail;
	ret = nla_put_u32(skb, CNSS_GENL_ATTR_MSG_DATA_LEN, data_len);
	if (ret < 0)
		goto fail;
	ret = nla_put(skb, CNSS_GENL_ATTR_MSG_DATA, data_len, msg_buff);
	if (ret < 0)
		goto fail;

	genlmsg_end(skb, msg_header);
	ret = genlmsg_multicast(&cnss_genl_family, skb, 0, 0, GFP_KERNEL);
	return ret;
fail:
	pr_err("genl msg send fail: %d\n", ret);
	if (skb)
		nlmsg_free(skb);
	return ret;
}

#define MAX_RETRY 20
int cnss_genl_send_msg(void *buff, u8 type, char *file_name, u32 total_size)
{
	int ret = 0;
	u8 *msg_buff = buff;
	u32 remaining = total_size;
	u32 seg_id = 0;
	u32 data_len = 0;
	u8 end = 0;
	int retry_count;

	pr_debug("type: %u, total_size: %x\n", type, total_size);

	while (remaining) {
		retry_count = 0;
		if (remaining > CNSS_GENL_DATA_LEN_MAX) {
			data_len = CNSS_GENL_DATA_LEN_MAX;
		} else {
			data_len = remaining;
			end = 1;
		}
retry:
		ret = cnss_genl_send_data(type, file_name, total_size,
					  seg_id, end, data_len, msg_buff);
		if (ret < 0 && retry_count < MAX_RETRY) {
			msleep(50);
			retry_count++;
			goto retry;
		}
		if (retry_count >= MAX_RETRY) {
			pr_err("Failed to send genl data, seg_id  %d\n",
			       seg_id);
			pr_err("Fatal Err: Giving up after %d retries\n",
			       MAX_RETRY);
			pr_err("QDSS trace data is corrupted\n");
		}

		remaining -= data_len;
		msg_buff += data_len;
		seg_id++;
	}

	return ret;
}

static bool genl_registered;
int cnss_genl_init(void)
{
	int ret = 0;

	if (!genl_registered) {
		ret = genl_register_family(&cnss_genl_family);

		if (ret != 0)
			pr_err("genl_register_family fail: %d\n", ret);
		else
			genl_registered = true;
	}
	return ret;
}

void cnss_genl_exit(void)
{
	if (genl_registered) {
		genl_unregister_family(&cnss_genl_family);
		genl_registered = false;
	}
}
