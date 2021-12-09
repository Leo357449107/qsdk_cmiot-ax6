/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#ifndef _APPE_GLOBAL_REG_H_
#define _APPE_GLOBAL_REG_H_

/*[register] PORT_MUX_CTRL*/
#define SWITCH_PORT_MUX_CTRL
#define SWITCH_PORT_MUX_CTRL_ADDRESS 0x10
#define SWITCH_PORT_MUX_CTRL_NUM     1
#define SWITCH_PORT_MUX_CTRL_INC     0x4
#define SWITCH_PORT_MUX_CTRL_TYPE    REG_TYPE_RW
#define SWITCH_PORT_MUX_CTRL_DEFAULT 0x0

struct appe_port_mux_ctrl {
	a_uint32_t  port1_pcs_sel:1;
	a_uint32_t  port2_pcs_sel:1;
	a_uint32_t  port3_pcs_sel:1;
	a_uint32_t  port4_pcs_sel:1;
	a_uint32_t  port5_pcs_sel:1;
	a_uint32_t  port6_pcs_sel:1;
	a_uint32_t  _reserved0:2;
	a_uint32_t  port1_mac_sel:1;
	a_uint32_t  port2_mac_sel:1;
	a_uint32_t  port3_mac_sel:1;
	a_uint32_t  port4_mac_sel:1;
	a_uint32_t  port5_mac_sel:1;
	a_uint32_t  port6_mac_sel:1;
	a_uint32_t  _reserved1:18;
};

union appe_port_mux_ctrl_u {
	a_uint32_t val;
	struct appe_port_mux_ctrl bf;
};

#endif