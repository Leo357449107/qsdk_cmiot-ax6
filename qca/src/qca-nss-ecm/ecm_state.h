/*
 **************************************************************************
 * Copyright (c) 2015, 2021, The Linux Foundation.  All rights reserved.
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

struct ecm_state_file_instance;

int ecm_state_write_reset(struct ecm_state_file_instance *sfi, char *prefix);

int ecm_state_prefix_add(struct ecm_state_file_instance *sfi, char *prefix);
int ecm_state_prefix_index_add(struct ecm_state_file_instance *sfi, uint32_t index);
int ecm_state_prefix_remove(struct ecm_state_file_instance *sfi);

int ecm_state_write(struct ecm_state_file_instance *sfi, char *name, char *fmt, ...);

