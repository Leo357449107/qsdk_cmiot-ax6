/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#include "nss_macsec_types.h"
#include "nss_macsec.h"
#include "nss_macsec_secy.h"
#include "nss_macsec_utility.h"
#include "nss_macsec_ops.h"

struct mutex api_lock;

u32 nss_macsec_mutex_init(void)
{
	mutex_init(&api_lock);

	return 0;
}

u32 nss_macsec_mutex_destroy(void)
{
	mutex_destroy(&api_lock);
	return 0;
}

u32 nss_macsec_mutex_lock(void)
{
	mutex_lock(&api_lock);
	return 0;
}

u32 nss_macsec_mutex_unlock(void)
{
	mutex_unlock(&api_lock);
	return 0;
}

u32 fal_macsec_secy_num_get(void)
{
	return g_secy_number;
}

u32 fal_secy_sc_sa_mapping_mode_get(u32 secy_id)
{
	return g_secy_cfg[secy_id].sc_sa_map_mode;
}

u32 fal_secy_get_sa_num_per_sc(u32 secy_id)
{
	return (1 << g_secy_cfg[secy_id].sc_sa_map_mode);
}

u32 fal_secy_channel_num(u32 secy_id)
{
	g_error_t rv;
	struct secy_drv_t *secy_drv;
	u32 number = 0;

	secy_drv = nss_macsec_secy_ops_get(secy_id);
	if (secy_drv != NULL) {
		if (secy_drv->secy_channel_number_get != NULL) {
			rv = secy_drv->secy_channel_number_get(secy_id,
				&number);
			if (!rv)
				return number;
		}
	}

	return (32 >> (g_secy_cfg[secy_id].sc_sa_map_mode));
}

u32 fal_tx_channel_2_sc_id(u32 secy_id, u32 channel)
{
	return (channel << FAL_SECY_SC_SA_MODE(secy_id));
}

u32 fal_tx_sc_id_2_channel(u32 secy_id, u32 sc_id)
{
	return (sc_id >> FAL_SECY_SC_SA_MODE(secy_id));
}

u32 fal_rx_channel_2_sc_id(u32 secy_id, u32 channel)
{
	return (channel << FAL_SECY_SC_SA_MODE(secy_id));
}

u32 fal_rx_sc_id_2_channel(u32 secy_id, u32 sc_id)
{
	return (sc_id >> FAL_SECY_SC_SA_MODE(secy_id));
}
