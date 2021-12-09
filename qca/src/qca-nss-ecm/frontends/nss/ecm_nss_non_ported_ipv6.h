/*
 **************************************************************************
 * Copyright (c) 2015, 2018, 2021 The Linux Foundation. All rights reserved.
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

extern bool ecm_nss_non_ported_ipv6_debugfs_init(struct dentry *dentry);

extern struct ecm_nss_non_ported_ipv6_connection_instance *ecm_nss_non_ported_ipv6_connection_instance_alloc(
								bool can_accel,
								int protocol,
								struct ecm_db_connection_instance **nci);
extern bool ecm_nss_non_ported_ipv6_connection_defunct_callback(void *arg, int *accel_mode);
