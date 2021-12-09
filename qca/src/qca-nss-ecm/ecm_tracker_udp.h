/*
 **************************************************************************
 * Copyright (c) 2014-2015, The Linux Foundation.  All rights reserved.
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

struct ecm_tracker_udp_instance;

#ifdef ECM_TRACKER_DPI_SUPPORT_ENABLE
typedef int32_t (*ecm_tracker_udp_data_size_get_method_t)(struct ecm_tracker_udp_instance *uti, ecm_tracker_sender_type_t sender, int32_t i);
										/* Return size in bytes of datagram data at index i that was sent by the sender */
typedef int (*ecm_tracker_udp_data_read_method_t)(struct ecm_tracker_udp_instance *uti, ecm_tracker_sender_type_t sender, int32_t i, int32_t offset, int32_t size, void *buffer);
										/* Read size bytes of data at index i into the buffer */
typedef bool (*ecm_tracker_udp_datagram_add_method_t)(struct ecm_tracker_udp_instance *uti, ecm_tracker_sender_type_t sender, struct ecm_tracker_ip_header *ip_hdr, struct ecm_tracker_ip_protocol_header *ecm_udp_header, struct udphdr *udp_header, struct sk_buff *skb);
										/* Add a pre-checked UDP datagram */
#endif

struct ecm_tracker_udp_instance {
	struct ecm_tracker_instance base;					/* MUST BE FIRST FIELD */

#ifdef ECM_TRACKER_DPI_SUPPORT_ENABLE
	ecm_tracker_udp_data_read_method_t data_read;				/* Read data from a UDP datagram (headers not considered) */
	ecm_tracker_udp_data_size_get_method_t data_size_get;			/* Get data size of datagram (headers not considered) */
	ecm_tracker_udp_datagram_add_method_t datagram_add;			/* Add a pre-checked UDP datagram */
#endif
};


struct udphdr *ecm_tracker_udp_check_header_and_read(struct sk_buff *skb, struct ecm_tracker_ip_header *ip_hdr, struct udphdr *port_buffer);

void ecm_tracker_udp_init(struct ecm_tracker_udp_instance *uti, int32_t data_limit, int src_port, int dest_port);
struct ecm_tracker_udp_instance *ecm_tracker_udp_alloc(void);

