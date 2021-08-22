/*
 * Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _REPEATER_TYPES_H_
#define _REPEATER_TYPES_H_

#define RPTR_LOGD(args ...)                                              \
	QDF_TRACE(QDF_MODULE_ID_RPTR, QDF_TRACE_LEVEL_DEBUG, ## args)

#define RPTR_LOGI(args ...)                                              \
	QDF_TRACE(QDF_MODULE_ID_RPTR, QDF_TRACE_LEVEL_INFO, ## args)

#define RPTR_LOGW(args ...)                                              \
	QDF_TRACE(QDF_MODULE_ID_RPTR, QDF_TRACE_LEVEL_WARN, ## args)

#define RPTR_LOGE(args ...)                                              \
	QDF_TRACE(QDF_MODULE_ID_RPTR, QDF_TRACE_LEVEL_ERROR, ## args)

#define RPTR_LOGF(args ...)                                              \
	QDF_TRACE(QDF_MODULE_ID_RPTR, QDF_TRACE_LEVEL_FATAL, ## args)
#endif
