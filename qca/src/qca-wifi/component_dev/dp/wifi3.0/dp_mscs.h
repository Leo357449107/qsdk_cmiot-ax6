
/*
 * Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DP_MSCS_H_
#define _DP_MSCS_H_

#if WLAN_SUPPORT_MSCS
/**
 * dp_mscs_peer_lookup_status - mscs lookup status
 * @DP_MSCS_PEER_LOOKUP_STATUS_ALLOW_MSCS_QOS_TAG_UPDATE
 * @DP_MSCS_PEER_LOOKUP_STATUS_DENY_QOS_TAG_UPDATE
 * @DP_MSCS_PEER_LOOKUP_STATUS_ALLOW_INVALID_QOS_TAG_UPDATE
 */
enum dp_mscs_peer_lookup_status {
	DP_MSCS_PEER_LOOKUP_STATUS_ALLOW_MSCS_QOS_TAG_UPDATE,
	DP_MSCS_PEER_LOOKUP_STATUS_ALLOW_INVALID_QOS_TAG_UPDATE,
	DP_MSCS_PEER_LOOKUP_STATUS_DENY_QOS_TAG_UPDATE,
};

int dp_mscs_peer_lookup_n_get_priority(struct cdp_soc_t *soc_hdl,
		uint8_t *src_mac_addr, uint8_t *dst_mac_addr, qdf_nbuf_t nbuf);
#endif

#endif /* QCA_MSCS_H*/
