/*
 **************************************************************************
 * Copyright (c) 2015-2016, 2019, The Linux Foundation. All rights reserved.
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

/*
 * @file nss_nl_if.h
 *	NSS Netlink interface
 */
#ifndef __NSS_NL_IF_H
#define __NSS_NL_IF_H

#define NSS_NL_VER_MINOR 1
#define NSS_NL_VER_MAJOR 1
#define NSS_NL_VER_SHIFT 8

#define NSS_NL_VER ((NSS_NL_VER_MAJOR << NSS_NL_VER_SHIFT) | NSS_NL_VER_MINOR)

/**
 * @brief interface types
 */
enum nss_nl_iftype {
	NSS_NL_IFTYPE_PHYSICAL = 0,
	NSS_NL_IFTYPE_VLAN,
	NSS_NL_IFTYPE_PPPOE,
	NSS_NL_IFTYPE_TUNNEL_IPSEC,
	NSS_NL_IFTYPE_TUNNEL_GRE,
	NSS_NL_IFTYPE_TUNNEL_PPTP,
	NSS_NL_IFTYPE_TUNNEL_TUN6RD,
	NSS_NL_IFTYPE_MAX
};

#endif /* __NSS_NL_IF_H */


