/*
 **************************************************************************
 * Copyright (c) 2014 - 2015, 2019 The Linux Foundation. All rights reserved.
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
 * nss_nlipsec.h
 *	NSS Netlink IPsec API definitions
 */
#ifndef __NSS_NLIPSEC_H
#define __NSS_NLIPSEC_H



#if (CONFIG_NSS_NLIPSEC == 1)
#define NSS_NLIPSEC_INIT nss_nlipsec_init
#define NSS_NLIPSEC_EXIT nss_nlipsec_exit

bool nss_nlipsec_init(void);
bool nss_nlipsec_exit(void);

/*
 * Get dynamic interface number for inner/outer
 */
int nss_nlipsec_get_ifnum(struct net_device *dev, uint8_t proto, uint16_t dest_port, uint16_t src_port);

/*
 * Get mtu for IPsec tunnel
 */
int nss_nlipsec_get_mtu(struct net_device *dev, uint8_t ip_ver, uint8_t proto, uint16_t dest_port, uint16_t src_port);

#else
#define NSS_NLIPSEC_INIT 0
#define NSS_NLIPSEC_EXIT 0

/*
 * Stub functions
 */
static inline int nss_nlipsec_get_ifnum(struct net_device *dev, uint8_t proto, uint16_t dest_port, uint16_t src_port)
{
	nss_nl_error("%p: Unsupported protocol interface number (dev:%s)\n", dev, dev->name);
	return -1;
}

/*
 * Stub functions
 */
static inline int nss_nlipsec_get_mtu(struct net_device *dev, uint8_t ip_ver, uint8_t proto, uint16_t dest_port,
					uint16_t src_port)
{
	return dev->mtu;
}

#endif /* !CONFIG_NSS_NLIPSEC */
#endif /* __NSS_NLIPSEC_H */
