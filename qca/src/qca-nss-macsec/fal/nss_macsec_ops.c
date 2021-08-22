/*
 * Copyright (c) 2018, The Linux Foundation. All rights reserved.
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


#include "nss_macsec_ops.h"

static struct secy_driver_instance_t secy_driver[] = {
	{NAPA_PHY_CHIP, NULL, qca808x_secy_driver_init,
		qca808x_secy_driver_exit},
	{MAX_PHY_CHIP, NULL, NULL, NULL}
};

static u32 secy_max_num;
static struct secy_info_t secy_info[MAX_SECY_ID] = {
		{0, MAX_PHY_CHIP},
		{0, MAX_PHY_CHIP},
		{0, MAX_PHY_CHIP},
	};

g_error_t secy_id_validate_check(u32 secy_id)
{
	if (secy_id < secy_max_num)
		return ERROR_OK;
	else
		return ERROR_NOT_SUPPORT;
}

g_error_t secy_ops_init(enum secy_phy_type_t phy_type)
{

	if (phy_type >= MAX_PHY_CHIP)
		return ERROR_PARAM;

	if (secy_driver[phy_type].secy_ops != NULL)
		secy_driver[phy_type].secy_ops = NULL;

	return ERROR_OK;
}
g_error_t secy_ops_register(enum secy_phy_type_t phy_type,
	struct secy_drv_t *secy_ops)
{

	secy_driver[phy_type].secy_ops = secy_ops;

	return ERROR_OK;

}
struct secy_drv_t *nss_macsec_secy_ops_get(u32 secy_id)
{
	enum secy_phy_type_t phy_type = 0;

	if (secy_id >= MAX_SECY_ID)
		return NULL;

	phy_type = secy_info[secy_id].secy_phy_type;

	return secy_driver[phy_type].secy_ops;

}
void nss_macsec_secy_info_set(u32 secy_id, u32 phy_addr)
{
	secy_info[secy_id].phy_addr = phy_addr;

}

u32 secy_id_to_phy_addr(u32 secy_id)
{
	return secy_info[secy_id].phy_addr;
}

int nss_macsec_secy_driver_init(u32 secy_id)
{
	u16 org_id = 0, rev_id = 0;
	u32 phy_id = 0;
	enum secy_phy_type_t phy_type = MAX_PHY_CHIP;

	phy_mii_reg_read(secy_id, 0x2, &org_id);
	phy_mii_reg_read(secy_id, 0x3, &rev_id);

	phy_id = (org_id<<16) | rev_id;

	switch (phy_id) {
	case PHY_NAPA:
	case PHY_NAPA_1_1:
		phy_type = NAPA_PHY_CHIP;
		secy_info[secy_id].secy_phy_type = phy_type;
		break;
	default:
		break;
	}
	if (secy_driver[phy_type].init != NULL)
		secy_driver[phy_type].init(secy_id);

	secy_max_num++;

	return 0;
}
g_error_t secy_ops_unregister(enum secy_phy_type_t phy_type)
{

	if (secy_driver[phy_type].secy_ops != NULL)
		secy_driver[phy_type].secy_ops = NULL;

	return ERROR_OK;
}
g_error_t nss_macsec_secy_driver_cleanup(u32 secy_id)
{
	enum secy_phy_type_t phy_type = MAX_PHY_CHIP;

	phy_type = secy_info[secy_id].secy_phy_type;

	if (secy_driver[phy_type].exit != NULL)
		secy_driver[phy_type].exit(secy_id);

	return ERROR_OK;
}

