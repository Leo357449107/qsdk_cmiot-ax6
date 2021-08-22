/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
#ifndef __WIL6210_SLAVE_H__
#define __WIL6210_SLAVE_H__

#include <linux/netdevice.h>

/* increment whenever changing wil_slave_ops or wil_slave_rops */
#define WIL_SLAVE_API_VERSION	2

/* these constants must be in sync with definition in wmi.h */
#define WIL_MAX_IOCTL_REPLY_PAYLOAD_SIZE	1024
#define WIL_MAX_INTERNAL_EVENT_PAYLOAD_SIZE	1024

/* should be in sync with wil6210 driver */
#define WIL_SLAVE_MAX_CID			16
/* max possible link ID */
#define WIL_SLAVE_MAX_LINKS			32

/* for link statistics */
struct wil_slave_link_stats {
	u64 rx_packets;
	u64 tx_packets;
	u64 rx_bytes;
	u64 tx_bytes;
	u64 rx_errors;
	u64 tx_errors;
	u64 tx_pend_bytes;
	u64 tx_pend_packets;
};

struct wil_slave_rops {
	int api_version;
	void (*rx_event)(void *ctx, u16 id, u8 *evt, u32 len);
	void (*connected)(void *ctx, int tx_link_id, int rx_link_id,
			  const u8 *mac, u8 cid);
	void (*disconnected)(void *ctx, u8 cid);
	int (*rx_data)(void *ctx, u8 cid, struct sk_buff *skb);
	void (*set_channel)(void *ctx, u8 channel);
	void (*slave_going_down)(void *ctx);
};

struct wil_slave_ops {
	int api_version;
	int (*register_master)(void *dev, void *ctx,
			       const struct wil_slave_rops *rops);
	void (*unregister_master)(void *dev);
	int (*ioctl)(void *dev, u16 code, const u8 *req_buf, u16 req_len,
		     u8 *resp_buf, u16 *resp_len);
	int (*set_key)(void *dev, const u8 *mac, const u8 *key, u16 len);
	netdev_tx_t (*tx_data)(void *dev, u8 cid, struct sk_buff *skb);
	int (*link_stats)(void *dev, u8 cid,
			  struct wil_slave_link_stats *stats);
	int (*fw_reload)(void *dev, const char *board_file);
	void (*get_mac)(void *dev, u8 *mac);
	struct napi_struct *(*get_napi_rx)(void *dev);
	void (*sync_rx)(void *dev);
};

/* platform device data for interaction with master driver */
struct wil_slave_platdata {
	struct wil_slave_ops *ops;
	void *dev_ctx;
};

#endif /* __WIL6210_SLAVE_H__ */
