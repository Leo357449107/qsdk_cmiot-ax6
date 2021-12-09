/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
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
#include "status_api.h"

LIST_HEAD(status_systems);

/*
 * APIs provided for upper layer, start
 */
/*
 * lacpd_status_sys_use: find a status system
 */
struct lacpd_status_system * lacpd_status_sys_use(const char *name, struct lacpd_status_user *user)
{
	struct lacpd_status_system *status_sys;

	if (!name || !user || !user->os_service) {
		return NULL;
	}

	list_for_each_entry(status_sys, &status_systems, node) {
		if (!strncmp(name, status_sys->name, sizeof(status_sys->name))) {
			status_sys->user = user;
			return status_sys;
		}
	}

	return NULL;
}

/*
 * APIs provided for upper layer, end
 */
/*
 * APIs provided for lower layer, start
 */
/*
 * lacpd_status_sys_register: register a status system
 */
bool lacpd_status_sys_register(struct lacpd_status_system *status_sys)
{
	list_add_tail(&status_sys->node, &status_systems);
	return true;
}
/*
 * APIs provided for lower layer, end
 */
