/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#include "sw.h"
#include "sw_ioctl.h"
#include "ref_vlan.h"
#include "fal_uk_if.h"

sw_error_t
qca_lan_wan_cfg_set(a_uint32_t dev_id, qca_lan_wan_cfg_t *lan_wan_cfg)
{
	sw_error_t rv;

	rv = sw_uk_exec(SW_API_LAN_WAN_CFG_SET, dev_id, lan_wan_cfg);
	return rv;
}

sw_error_t
qca_lan_wan_cfg_get(a_uint32_t dev_id, qca_lan_wan_cfg_t *lan_wan_cfg)
{
	sw_error_t rv;

	rv = sw_uk_exec(SW_API_LAN_WAN_CFG_GET, dev_id, lan_wan_cfg);
	return rv;
}
