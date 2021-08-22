/*
 **************************************************************************
 * Copyright (c) 2020, The Linux Foundation.  All rights reserved.
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

#ifndef __ECM_CLASSIFIER_EMESH_PUBLIC_H__
#define __ECM_CLASSIFIER_EMESH_PUBLIC_H__

/*
 * Mesh latency config update callback function to which MSCS client will register
 */
typedef int (*ecm_classifier_emesh_callback_t)(uint8_t dest_mac[],
		uint32_t service_interval, uint32_t burst_size,
		uint16_t priority, uint8_t add_or_sub);

struct ecm_classifier_emesh_callbacks {
	ecm_classifier_emesh_callback_t update_peer_mesh_latency_params;
};

int ecm_classifier_emesh_latency_config_callback_register(struct ecm_classifier_emesh_callbacks *mesh_cb);
void ecm_classifier_emesh_latency_config_callback_unregister(void);
#endif /* __ECM_CLASSIFIER_EMESH_PUBLIC_H__ */
