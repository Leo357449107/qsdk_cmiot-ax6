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

#ifndef __WIL6210_SLAVE_I_H__
#define __WIL6210_SLAVE_I_H__

int wil_register_slave(struct wil6210_priv *wil);
void wil_unregister_slave(struct wil6210_priv *wil);
void wil_slave_evt_internal_fw_event(struct wil6210_vif *vif,
				     struct wmi_internal_fw_event_event *evt,
				     int len);
void wil_slave_evt_internal_set_channel(
	struct wil6210_vif *vif,
	struct wmi_internal_fw_set_channel_event *evt,
	int len);
void wil_slave_tdm_connect(struct wil6210_vif *vif,
			   struct wmi_tdm_connect_event *evt,
			   int len);
void wil_slave_tdm_disconnect(struct wil6210_vif *vif,
			      struct wmi_tdm_disconnect_event *evt,
			      int len);
void wil_slave_evt_connect(struct wil6210_vif *vif,
			   int tx_link_id, int rx_link_id,
			   const u8 *mac, u8 cid);
void wil_slave_evt_disconnect(struct wil6210_vif *vif, u8 cid);
int wil_slave_rx_data(struct wil6210_vif *vif, u8 cid, struct sk_buff *skb);
const char *wil_slave_get_board_file(struct wil6210_priv *wil);

#endif /* __WIL6210_SLAVE_I_H__ */
