/*
 * Generic network code. Moved from net.c
 *
 * Copyright 1994 - 2000 Neil Russell.
 * Copyright 2000 Roland Borde
 * Copyright 2000 Paolo Scaffardi
 * Copyright 2000-2002 Wolfgang Denk, wd@denx.de
 * Copyright 2009 Dirk Behme, dirk.behme@googlemail.com
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>

struct in_addr string_to_ip(const char *s)
{
	struct in_addr addr;
	char *e;
	int i;

	addr.s_addr = 0;
	if (s == NULL)
		return addr;

	for (addr.s_addr = 0, i = 0; i < 4; ++i) {
		ulong val = s ? simple_strtoul(s, &e, 10) : 0;
		addr.s_addr <<= 8;
		addr.s_addr |= (val & 0xFF);
		if (s) {
			s = (*e) ? e+1 : e;
		}
	}

	addr.s_addr = htonl(addr.s_addr);
	return addr;
}
