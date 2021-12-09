/*
 **************************************************************************
 * Copyright (c) 2020-2021, The Linux Foundation.  All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

#ifndef __ECM_CLASSIFIER_MSCS_PUBLIC_H__
#define __ECM_CLASSIFIER_MSCS_PUBLIC_H__

#define ECM_CLASSIFIER_MSCS_INVALID_QOS_TAG 0xF

/*
 * MSCS Priority update result vased on WiFi MSCS peer lookup
 */
enum ecm_classifier_mscs_results {
	ECM_CLASSIFIER_MSCS_RESULT_UPDATE_PRIORITY,		/* MSCS priority update allowed */
	ECM_CLASSIFIER_MSCS_RESULT_UPDATE_INVALID_TAG,
	ECM_CLASSIFIER_MSCS_RESULT_DENY_PRIORITY,		/* MSCS priority not allowed for flow and return direction */
};
typedef enum ecm_classifier_mscs_results ecm_classifier_mscs_result_t;

/*
 * Callback function to which MSCS client will register
 */
typedef int (*ecm_classifier_mscs_process_callback_t)(uint8_t src_mac[], uint8_t dst_mac[], struct sk_buff* skb);

struct ecm_classifier_mscs_callbacks {
	ecm_classifier_mscs_process_callback_t get_peer_priority;
};

int ecm_classifier_mscs_callback_register(struct ecm_classifier_mscs_callbacks *mscs_cb);
void ecm_classifier_mscs_callback_unregister(void);
#endif /* __ECM_CLASSIFIER_MSCS_PUBLIC_H__ */
