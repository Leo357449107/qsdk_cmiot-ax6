/*
 **************************************************************************
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

#ifndef __NSS_VXLAN_MGR_H__
#define __NSS_VXLAN_MGR_H__

typedef int32_t (*nss_vxlanmgr_get_ipsec_if_num_by_ip_callback_t)(uint8_t ip_version, uint32_t *src_ip, uint32_t *dest_ip);

/**
 * nss_vxlanmgr_get_ipsec_if_num
 *	Callback to get the inner IPSec interface number that was used to register with NSS.
 *
 * get_if_num_by_ip: passes IP version and IP addresses to get associated IPsec if_num.
 */
struct nss_vxlanmgr_get_ipsec_if_num {
	nss_vxlanmgr_get_ipsec_if_num_by_ip_callback_t get_ifnum_by_ip;
};

/**
 * nss_vxlanmgr_unregister_ipsecmgr_callback_by_ip
 *	Unregister IPSecmgr callback function with vxlanmgr.
 *
 * @return
 * none
 */
void nss_vxlanmgr_unregister_ipsecmgr_callback_by_ip(void);

/**
 * nss_vxlanmgr_register_ipsecmgr_callback_by_ip
 *	Register IPSecmgr callback function with vxlanmgr.
 *
 * @datatypes
 * nss_vxlanmgr_get_ipsec_if_num \n
 *
 * @param[in] cb    IPSecmgr callback function to be registered with vxlanmgr.
 *
 * @return
 * none
 */
void nss_vxlanmgr_register_ipsecmgr_callback_by_ip(struct nss_vxlanmgr_get_ipsec_if_num *cb);
#endif
