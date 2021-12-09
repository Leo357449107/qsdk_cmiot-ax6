/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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


/**
 * @defgroup fal_vxlan FAL_VXLAN
 * @{
 */
#ifndef _FAL_VXLAN_H_
#define _FAL_VXLAN_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "sw.h"
#include "fal_type.h"
#include "fal_tunnel.h"

typedef enum {
	FAL_VXLAN =0,
	FAL_VXLAN_GPE =1,
} fal_vxlan_type_t;

typedef struct {
	a_uint8_t ethernet;
	a_uint8_t ipv4;
	a_uint8_t ipv6;
} fal_vxlan_gpe_proto_cfg_t;

enum
{
	/*vxlan*/
	FUNC_VXLAN_ENTRY_ADD = 0,
	FUNC_VXLAN_ENTRY_DEL,
	FUNC_VXLAN_ENTRY_GETFIRST,
	FUNC_VXLAN_ENTRY_GETNEXT,
	FUNC_VXLAN_GPE_PROTO_CFG_SET,
	FUNC_VXLAN_GPE_PROTO_CFG_GET,
};


sw_error_t
fal_vxlan_entry_add(a_uint32_t dev_id, fal_vxlan_type_t type, fal_tunnel_udp_entry_t * entry);

sw_error_t
fal_vxlan_entry_del(a_uint32_t dev_id, fal_vxlan_type_t type, fal_tunnel_udp_entry_t * entry);

sw_error_t
fal_vxlan_entry_getfirst(a_uint32_t dev_id, fal_vxlan_type_t type, fal_tunnel_udp_entry_t * entry);

sw_error_t
fal_vxlan_entry_getnext(a_uint32_t dev_id, fal_vxlan_type_t type, fal_tunnel_udp_entry_t * entry);

sw_error_t
fal_vxlan_gpe_proto_cfg_set(a_uint32_t dev_id, fal_vxlan_gpe_proto_cfg_t * proto_cfg);

sw_error_t
fal_vxlan_gpe_proto_cfg_get(a_uint32_t dev_id, fal_vxlan_gpe_proto_cfg_t * proto_cfg);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _FAL_VXLAN_H_ */
/**
 * @}
 */

