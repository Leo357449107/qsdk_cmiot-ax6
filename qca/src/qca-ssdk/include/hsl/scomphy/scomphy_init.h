/*
 * Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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
 */


/**
 * @defgroup scomphy_init SCOMPHY_INIT
 * @{
 */
#ifndef _SCOMPHY_INIT_H_
#define _SCOMPHY_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ssdk_init.h"

sw_error_t
scomphy_init(a_uint32_t dev_id, ssdk_init_cfg *cfg);

sw_error_t
scomphy_cleanup(a_uint32_t dev_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _SCOMPHY_INIT_H_ */
/**
 * @}
 */
