/*
 * Copyright (c) 2014, 2017-2018, The Linux Foundation. All rights reserved.
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

/*qca808x_start*/
/**
 * @defgroup fal_reg_access FAL_REG_ACCESS
 * @{
 */
#ifndef _FAL_REG_ACCESS_H_
#define _FAL_REG_ACCESS_H_

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

#include "sw.h"
#include "fal_type.h"

    sw_error_t
    fal_phy_get(a_uint32_t dev_id, a_uint32_t phy_addr,
                a_uint32_t reg, a_uint16_t * value);

    sw_error_t
    fal_phy_set(a_uint32_t dev_id, a_uint32_t phy_addr,
                a_uint32_t reg, a_uint16_t value);
/*qca808x_end*/

    sw_error_t
    fal_reg_get(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t value[],
                a_uint32_t value_len);

    sw_error_t
    fal_reg_set(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t value[],
                a_uint32_t value_len);

    sw_error_t
    fal_psgmii_reg_get(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t value[],
                a_uint32_t value_len);

    sw_error_t
    fal_psgmii_reg_set(a_uint32_t dev_id, a_uint32_t reg_addr, a_uint8_t value[],
                a_uint32_t value_len);

    sw_error_t
    fal_reg_field_get(a_uint32_t dev_id, a_uint32_t reg_addr,
                      a_uint32_t bit_offset, a_uint32_t field_len,
                      a_uint8_t value[], a_uint32_t value_len);

    sw_error_t
    fal_reg_field_set(a_uint32_t dev_id, a_uint32_t reg_addr,
                      a_uint32_t bit_offset, a_uint32_t field_len,
                      const a_uint8_t value[], a_uint32_t value_len);

	sw_error_t 
	fal_reg_dump(a_uint32_t dev_id, a_uint32_t reg_idx,fal_reg_dump_t *reg_dump);

	sw_error_t
	fal_dbg_reg_dump(a_uint32_t dev_id, fal_reg_dump_t *reg_dump);

	sw_error_t
	fal_debug_psgmii_self_test(a_uint32_t dev_id, a_bool_t enable,
            a_uint32_t times, a_uint32_t *result);

	sw_error_t
	fal_phy_dump(a_uint32_t dev_id, a_uint32_t phy_addr,
		a_uint32_t idx, fal_phy_dump_t * phy_dump);

	sw_error_t
	fal_uniphy_reg_get(a_uint32_t dev_id, a_uint32_t index, a_uint32_t reg_addr,
		a_uint8_t value[], a_uint32_t value_len);

	sw_error_t
	fal_uniphy_reg_set(a_uint32_t dev_id, a_uint32_t index, a_uint32_t reg_addr,
		a_uint8_t value[], a_uint32_t value_len);
/*qca808x_start*/
#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif                          /* _FAL_REG_ACCESS_H_ */

/**
 * @}
 */
/*qca808x_end*/
