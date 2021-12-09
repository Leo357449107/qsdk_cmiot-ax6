/*
 * Copyright (c) 2021, The Linux Foundation. All rights reserved.
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


/**
 * @defgroup
 * @{
 */
#include "sw.h"
#include "hsl.h"
#include "hppe_reg_access.h"
#include "appe_tunnel_reg.h"
#include "appe_tunnel.h"

sw_error_t
appe_tpr_udf_ctrl_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_ctrl_0_u *value)
{
	if (index >= TPR_UDF_CTRL_0_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				TUNNEL_PARSER_BASE_ADDR + TPR_UDF_CTRL_0_ADDRESS + \
				index * TPR_UDF_CTRL_0_INC,
				&value->val);
}

sw_error_t
appe_tpr_udf_ctrl_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_ctrl_0_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_PARSER_BASE_ADDR + TPR_UDF_CTRL_0_ADDRESS + \
				index * TPR_UDF_CTRL_0_INC,
				value->val);
}

sw_error_t
appe_tpr_udf_profile_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_base_u *value)
{
	if (index >= TPR_UDF_PROFILE_BASE_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				TUNNEL_PARSER_BASE_ADDR + TPR_UDF_PROFILE_BASE_ADDRESS + \
				index * TPR_UDF_PROFILE_BASE_INC,
				&value->val);
}

sw_error_t
appe_tpr_udf_profile_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_base_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_PARSER_BASE_ADDR + TPR_UDF_PROFILE_BASE_ADDRESS + \
				index * TPR_UDF_PROFILE_BASE_INC,
				value->val);
}

sw_error_t
appe_tpr_udf_profile_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_offset_u *value)
{
	if (index >= TPR_UDF_PROFILE_OFFSET_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				TUNNEL_PARSER_BASE_ADDR + TPR_UDF_PROFILE_OFFSET_ADDRESS + \
				index * TPR_UDF_PROFILE_OFFSET_INC,
				&value->val);
}

sw_error_t
appe_tpr_udf_profile_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_offset_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_PARSER_BASE_ADDR + TPR_UDF_PROFILE_OFFSET_ADDRESS + \
				index * TPR_UDF_PROFILE_OFFSET_INC,
				value->val);
}

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_type;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_type = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.overlay_type_incl;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.overlay_type_incl = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l3_type_incl;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l3_type_incl = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_udf_profile_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf_profile;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_udf_profile_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf_profile = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.overlay_type;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.overlay_type = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.valid;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.valid = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_type_incl;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_type_incl = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_program_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.program_type;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_program_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.program_type = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_program_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.program_type_incl;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_program_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.program_type_incl = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l3_type;
	return ret;
}

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_ctrl_0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_ctrl_0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l3_type = value;
	ret = appe_tpr_udf_ctrl_0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf3_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf3_base;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf3_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf3_base = value;
	ret = appe_tpr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf1_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf1_base;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf1_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf1_base = value;
	ret = appe_tpr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf0_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf0_base;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf0_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf0_base = value;
	ret = appe_tpr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf2_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf2_base;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_base_udf2_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf2_base = value;
	ret = appe_tpr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf1_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf1_offset;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf1_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf1_offset = value;
	ret = appe_tpr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf0_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf0_offset;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf0_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf0_offset = value;
	ret = appe_tpr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf2_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf2_offset;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf2_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf2_offset = value;
	ret = appe_tpr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf3_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf3_offset;
	return ret;
}

sw_error_t
appe_tpr_udf_profile_offset_udf3_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tpr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tpr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf3_offset = value;
	ret = appe_tpr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_op_get(
		a_uint32_t dev_id,
		union tl_tbl_op_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_set(
		a_uint32_t dev_id,
		union tl_tbl_op_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data0_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data0_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA0_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data0_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data0_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA0_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data1_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data1_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA1_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data1_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data1_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA1_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data2_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data2_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA2_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data2_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data2_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA2_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data3_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data3_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA3_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data3_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data3_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA3_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data4_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data4_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA4_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data4_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data4_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA4_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data5_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data5_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA5_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data5_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data5_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA5_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data6_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data6_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA6_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data6_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data6_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA6_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data7_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data7_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA7_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data7_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data7_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA7_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data8_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data8_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA8_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data8_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data8_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA8_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data9_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data9_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA9_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data9_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data9_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA9_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data10_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data10_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA10_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data10_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data10_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA10_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data11_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data11_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA11_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data11_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data11_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA11_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data12_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data12_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA12_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data12_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data12_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA12_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data13_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data13_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA13_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data13_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data13_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA13_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data14_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data14_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA14_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data14_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data14_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA14_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data15_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data15_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA15_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data15_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data15_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA15_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_op_data16_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data16_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA16_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_op_data16_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data16_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_OP_DATA16_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data0_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data0_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA0_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data0_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data0_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA0_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data1_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data1_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA1_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data1_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data1_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA1_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data2_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data2_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA2_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data2_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data2_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA2_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data3_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data3_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA3_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data3_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data3_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA3_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data4_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data4_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA4_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data4_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data4_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA4_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data5_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data5_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA5_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data5_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data5_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA5_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data6_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data6_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA6_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data6_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data6_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA6_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data7_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data7_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA7_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data7_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data7_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA7_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data8_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data8_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA8_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data8_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data8_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA8_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data9_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data9_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA9_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data9_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data9_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA9_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data10_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data10_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA10_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data10_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data10_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA10_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data11_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data11_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA11_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data11_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data11_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA11_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data12_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data12_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA12_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data12_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data12_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA12_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data13_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data13_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA13_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data13_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data13_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA13_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data14_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data14_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA14_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data14_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data14_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA14_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data15_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data15_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA15_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data15_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data15_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA15_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data16_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data16_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA16_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_op_data16_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data16_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_OP_DATA16_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data0_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data0_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA0_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data1_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data1_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA1_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data2_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data2_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA2_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data3_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data3_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA3_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data4_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data4_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA4_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data5_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data5_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA5_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data6_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data6_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA6_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data7_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data7_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA7_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data8_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data8_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA8_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data9_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data9_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA9_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data10_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data10_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA10_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data11_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data11_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA11_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data12_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data12_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA12_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data13_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data13_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA13_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data14_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data14_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA14_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data15_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data15_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA15_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_tbl_rd_rslt_data16_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data16_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_RD_RSLT_DATA16_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_ctrl_get(
		a_uint32_t dev_id,
		union tl_ctrl_u *value)
{
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_CTRL_ADDRESS,
				&value->val);
}

sw_error_t
appe_tl_ctrl_set(
		a_uint32_t dev_id,
		union tl_ctrl_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_CTRL_ADDRESS,
				value->val);
}

sw_error_t
appe_tl_l3_if_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_l3_if_tbl_u *value)
{
	if (index >= TL_L3_IF_TBL_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_L3_IF_TBL_ADDRESS + \
				index * TL_L3_IF_TBL_INC,
				&value->val);
}

sw_error_t
appe_tl_l3_if_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_l3_if_tbl_u *value)
{
	return hppe_reg_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_L3_IF_TBL_ADDRESS + \
				index * TL_L3_IF_TBL_INC,
				value->val);
}

sw_error_t
appe_tl_key_gen_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_key_gen_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_KEY_GEN_ADDRESS + \
				index * TL_KEY_GEN_INC,
				value->val,
				3);
}

sw_error_t
appe_tl_key_gen_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_key_gen_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_KEY_GEN_ADDRESS + \
				index * TL_KEY_GEN_INC,
				value->val,
				3);
}

sw_error_t
appe_tl_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_tbl_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_ADDRESS + \
				index * TL_TBL_INC,
				value->val,
				14);
}

sw_error_t
appe_tl_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_tbl_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_TBL_ADDRESS + \
				index * TL_TBL_INC,
				value->val,
				14);
}

sw_error_t
appe_eg_ipv4_hdr_ctrl_get(
		a_uint32_t dev_id,
		union eg_ipv4_hdr_ctrl_u *value)
{
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_IPV4_HDR_CTRL_ADDRESS,
				&value->val);
}

sw_error_t
appe_eg_ipv4_hdr_ctrl_set(
		a_uint32_t dev_id,
		union eg_ipv4_hdr_ctrl_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_IPV4_HDR_CTRL_ADDRESS,
				value->val);
}

sw_error_t
appe_eg_udp_entropy_ctrl_get(
		a_uint32_t dev_id,
		union eg_udp_entropy_ctrl_u *value)
{
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_UDP_ENTROPY_CTRL_ADDRESS,
				&value->val);
}

sw_error_t
appe_eg_udp_entropy_ctrl_set(
		a_uint32_t dev_id,
		union eg_udp_entropy_ctrl_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_UDP_ENTROPY_CTRL_ADDRESS,
				value->val);
}

sw_error_t
appe_ecn_profile_get(
		a_uint32_t dev_id,
		union ecn_profile_u *value)
{
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + ECN_PROFILE_ADDRESS,
				&value->val);
}

sw_error_t
appe_ecn_profile_set(
		a_uint32_t dev_id,
		union ecn_profile_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + ECN_PROFILE_ADDRESS,
				value->val);
}

sw_error_t
appe_eg_proto_mapping0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping0_u *value)
{
	if (index >= EG_PROTO_MAPPING0_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_PROTO_MAPPING0_ADDRESS + \
				index * EG_PROTO_MAPPING0_INC,
				&value->val);
}

sw_error_t
appe_eg_proto_mapping0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping0_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_PROTO_MAPPING0_ADDRESS + \
				index * EG_PROTO_MAPPING0_INC,
				value->val);
}

sw_error_t
appe_eg_proto_mapping1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping1_u *value)
{
	if (index >= EG_PROTO_MAPPING1_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_PROTO_MAPPING1_ADDRESS + \
				index * EG_PROTO_MAPPING1_INC,
				&value->val);
}

sw_error_t
appe_eg_proto_mapping1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping1_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_PROTO_MAPPING1_ADDRESS + \
				index * EG_PROTO_MAPPING1_INC,
				value->val);
}

sw_error_t
appe_dbg_addr_get(
		a_uint32_t dev_id,
		union dbg_addr_u *value)
{
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + DBG_ADDR_ADDRESS,
				&value->val);
}

sw_error_t
appe_dbg_addr_set(
		a_uint32_t dev_id,
		union dbg_addr_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + DBG_ADDR_ADDRESS,
				value->val);
}

sw_error_t
appe_dbg_data_get(
		a_uint32_t dev_id,
		union dbg_data_u *value)
{
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + DBG_DATA_ADDRESS,
				&value->val);
}

sw_error_t
appe_tx_buff_thrsh_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tx_buff_thrsh_u *value)
{
	if (index >= TX_BUFF_THRSH_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + TX_BUFF_THRSH_ADDRESS + \
				index * TX_BUFF_THRSH_INC,
				&value->val);
}

sw_error_t
appe_tx_buff_thrsh_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tx_buff_thrsh_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + TX_BUFF_THRSH_ADDRESS + \
				index * TX_BUFF_THRSH_INC,
				value->val);
}

sw_error_t
appe_eg_header_data_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_header_data_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_HEADER_DATA_ADDRESS + \
				index * EG_HEADER_DATA_INC,
				value->val,
				32);
}

sw_error_t
appe_eg_header_data_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_header_data_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_HEADER_DATA_ADDRESS + \
				index * EG_HEADER_DATA_INC,
				value->val,
				32);
}

sw_error_t
appe_eg_xlat_tun_ctrl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_xlat_tun_ctrl_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_XLAT_TUN_CTRL_ADDRESS + \
				index * EG_XLAT_TUN_CTRL_INC,
				value->val,
				3);
}

sw_error_t
appe_eg_xlat_tun_ctrl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_xlat_tun_ctrl_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_XLAT_TUN_CTRL_ADDRESS + \
				index * EG_XLAT_TUN_CTRL_INC,
				value->val,
				3);
}

sw_error_t
appe_eg_edit_rule_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_edit_rule_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_EDIT_RULE_ADDRESS + \
				index * EG_EDIT_RULE_INC,
				value->val,
				3);
}

sw_error_t
appe_eg_edit_rule_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_edit_rule_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_EDIT_RULE_ADDRESS + \
				index * EG_EDIT_RULE_INC,
				value->val,
				3);
}
sw_error_t
appe_tl_tbl_op_op_mode_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	*value = reg_val.bf.op_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_op_op_mode_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.op_mode = value;
	ret = appe_tl_tbl_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_op_op_type_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	*value = reg_val.bf.op_type;
	return ret;
}

sw_error_t
appe_tl_tbl_op_op_type_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.op_type = value;
	ret = appe_tl_tbl_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_op_op_rslt_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	*value = reg_val.bf.op_rslt;
	return ret;
}

sw_error_t
appe_tl_tbl_op_op_rslt_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.op_rslt = value;
	ret = appe_tl_tbl_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_op_busy_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	*value = reg_val.bf.busy;
	return ret;
}

sw_error_t
appe_tl_tbl_op_busy_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.busy = value;
	ret = appe_tl_tbl_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_op_entry_index_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	*value = reg_val.bf.entry_index;
	return ret;
}

sw_error_t
appe_tl_tbl_op_entry_index_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.entry_index = value;
	ret = appe_tl_tbl_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_op_hash_block_bitmap_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	*value = reg_val.bf.hash_block_bitmap;
	return ret;
}

sw_error_t
appe_tl_tbl_op_hash_block_bitmap_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.hash_block_bitmap = value;
	ret = appe_tl_tbl_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_op_mode_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	*value = reg_val.bf.op_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_op_mode_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.op_mode = value;
	ret = appe_tl_tbl_rd_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_op_type_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	*value = reg_val.bf.op_type;
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_op_type_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.op_type = value;
	ret = appe_tl_tbl_rd_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_op_rslt_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	*value = reg_val.bf.op_rslt;
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_op_rslt_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.op_rslt = value;
	ret = appe_tl_tbl_rd_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_busy_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	*value = reg_val.bf.busy;
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_busy_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.busy = value;
	ret = appe_tl_tbl_rd_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_entry_index_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	*value = reg_val.bf.entry_index;
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_entry_index_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.entry_index = value;
	ret = appe_tl_tbl_rd_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_hash_block_bitmap_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	*value = reg_val.bf.hash_block_bitmap;
	return ret;
}

sw_error_t
appe_tl_tbl_rd_op_hash_block_bitmap_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_tbl_rd_op_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_rd_op_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.hash_block_bitmap = value;
	ret = appe_tl_tbl_rd_op_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_hash_mode_0_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_hash_mode_0;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_hash_mode_0_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_hash_mode_0 = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_pppoe_multicast_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.pppoe_multicast_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_pppoe_multicast_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pppoe_multicast_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_vlan_check_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_vlan_check_de_acce;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_vlan_check_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_vlan_check_de_acce = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_udp_csum_zero_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.udp_csum_zero_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_udp_csum_zero_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udp_csum_zero_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_vlan_check_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_vlan_check_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_vlan_check_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_vlan_check_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_src_if_check_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_src_if_check_de_acce;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_src_if_check_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_src_if_check_de_acce = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_non_tcp_udp_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_map_non_tcp_udp_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_non_tcp_udp_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_map_non_tcp_udp_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_pppoe_multicast_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.pppoe_multicast_de_acce;
	return ret;
}

sw_error_t
appe_tl_ctrl_pppoe_multicast_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pppoe_multicast_de_acce = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_src_if_check_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_src_if_check_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_src_if_check_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_src_if_check_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_hash_mode_1_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_hash_mode_1;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_hash_mode_1_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_hash_mode_1 = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_udp_csum_zero_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_map_udp_csum_zero_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_udp_csum_zero_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_map_udp_csum_zero_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_dst_check_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_map_dst_check_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_dst_check_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_map_dst_check_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_udp_csum_zero_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.udp_csum_zero_de_acce;
	return ret;
}

sw_error_t
appe_tl_ctrl_udp_csum_zero_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udp_csum_zero_de_acce = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_src_check_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_map_src_check_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_map_src_check_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_map_src_check_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_ipv4_df_set_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.ipv4_df_set;
	return ret;
}

sw_error_t
appe_tl_ctrl_ipv4_df_set_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv4_df_set = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_de_acce_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.tl_de_acce_cmd;
	return ret;
}

sw_error_t
appe_tl_ctrl_tl_de_acce_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union tl_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tl_de_acce_cmd = value;
	ret = appe_tl_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ttl_exceed_de_acce;
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ttl_exceed_de_acce = value;
	ret = appe_tl_l3_if_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ipv6_decap_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ipv6_decap_en;
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ipv6_decap_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv6_decap_en = value;
	ret = appe_tl_l3_if_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ttl_exceed_cmd;
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ttl_exceed_cmd = value;
	ret = appe_tl_l3_if_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_min_ipv6_mtu_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.min_ipv6_mtu;
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_min_ipv6_mtu_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.min_ipv6_mtu = value;
	ret = appe_tl_l3_if_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_dmac_check_dis_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dmac_check_dis;
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_dmac_check_dis_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dmac_check_dis = value;
	ret = appe_tl_l3_if_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_lpm_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.lpm_en;
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_lpm_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.lpm_en = value;
	ret = appe_tl_l3_if_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ipv4_decap_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ipv4_decap_en;
	return ret;
}

sw_error_t
appe_tl_l3_if_tbl_ipv4_decap_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_l3_if_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_l3_if_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv4_decap_en = value;
	ret = appe_tl_l3_if_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_key_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.key_type;
	return ret;
}

sw_error_t
appe_tl_key_gen_key_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.key_type = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_ip_prot_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ip_prot_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_ip_prot_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ip_prot_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_udf1_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf1_id;
	return ret;
}

sw_error_t
appe_tl_key_gen_udf1_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf1_id = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_udf0_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf0_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_udf0_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf0_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_sport_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.sport_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_sport_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.sport_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_vni_resv_mask_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.vni_resv_mask_1 << 20 | \
		reg_val.bf.vni_resv_mask_0;
	return ret;
}

sw_error_t
appe_tl_key_gen_vni_resv_mask_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.vni_resv_mask_1 = value >> 20;
	reg_val.bf.vni_resv_mask_0 = value & (((a_uint64_t)1<<20)-1);
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_udf1_mask_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf1_mask_1 << 4 | \
		reg_val.bf.udf1_mask_0;
	return ret;
}

sw_error_t
appe_tl_key_gen_udf1_mask_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf1_mask_1 = value >> 4;
	reg_val.bf.udf1_mask_0 = value & (((a_uint64_t)1<<4)-1);
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_sip_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.sip_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_sip_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.sip_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_udf0_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf0_id;
	return ret;
}

sw_error_t
appe_tl_key_gen_udf0_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf0_id = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_dport_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dport_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_dport_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dport_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_vni_resv_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.vni_resv_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_vni_resv_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.vni_resv_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_udf0_mask_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf0_mask;
	return ret;
}

sw_error_t
appe_tl_key_gen_udf0_mask_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf0_mask = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_udf1_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf1_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_udf1_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf1_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_key_gen_dip_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dip_inc;
	return ret;
}

sw_error_t
appe_tl_key_gen_dip_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_key_gen_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_key_gen_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dip_inc = value;
	ret = appe_tl_key_gen_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_protocol_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.protocol;
	return ret;
}

sw_error_t
appe_tl_tbl_protocol_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.protocol = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_key_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.key_type;
	return ret;
}

sw_error_t
appe_tl_tbl_key_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.key_type = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_exp_profile_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.exp_profile;
	return ret;
}

sw_error_t
appe_tl_tbl_exp_profile_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.exp_profile = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_cvlan_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.cvlan_fmt;
	return ret;
}

sw_error_t
appe_tl_tbl_cvlan_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.cvlan_fmt = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_tl_l3_if_check_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.tl_l3_if_check_en;
	return ret;
}

sw_error_t
appe_tl_tbl_tl_l3_if_check_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.tl_l3_if_check_en = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_cdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.cdei_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_cdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.cdei_mode = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_svlan_check_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.svlan_check_en;
	return ret;
}

sw_error_t
appe_tl_tbl_svlan_check_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.svlan_check_en = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_dscp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.dscp_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_dscp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.dscp_mode = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_udf0_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.udf0_valid;
	return ret;
}

sw_error_t
appe_tl_tbl_udf0_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.udf0_valid = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_l4_dport_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.l4_dport_1 << 2 | \
		reg_val.bf0.l4_dport_0;
	return ret;
}

sw_error_t
appe_tl_tbl_l4_dport_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.l4_dport_1 = value >> 2;
	reg_val.bf0.l4_dport_0 = value & (((a_uint64_t)1<<2)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.valid;
	return ret;
}

sw_error_t
appe_tl_tbl_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.valid = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_l4_sport_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.l4_sport;
	return ret;
}

sw_error_t
appe_tl_tbl_l4_sport_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.l4_sport = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_tl_l3_if_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.tl_l3_if;
	return ret;
}

sw_error_t
appe_tl_tbl_tl_l3_if_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.tl_l3_if = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_decap_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.decap_en;
	return ret;
}

sw_error_t
appe_tl_tbl_decap_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.decap_en = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_svlan_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.svlan_fmt;
	return ret;
}

sw_error_t
appe_tl_tbl_svlan_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.svlan_fmt = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_udf0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.udf0;
	return ret;
}

sw_error_t
appe_tl_tbl_udf0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.udf0 = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.de_acce;
	return ret;
}

sw_error_t
appe_tl_tbl_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.de_acce = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_vni_resv_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.vni_resv_valid;
	return ret;
}

sw_error_t
appe_tl_tbl_vni_resv_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.vni_resv_valid = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_sdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.sdei_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_sdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.sdei_mode = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_ipv6_dst_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = (a_uint64_t)reg_val.bf0.ipv6_dst_addr_1 << 18 | \
		reg_val.bf0.ipv6_dst_addr_0;
	return ret;
}

sw_error_t
appe_tl_tbl_ipv6_dst_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.ipv6_dst_addr_1 = value >> 18;
	reg_val.bf0.ipv6_dst_addr_0 = value & (((a_uint64_t)1<<18)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_ecn_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.ecn_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_ecn_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.ecn_mode = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_spcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.spcp_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_spcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.spcp_mode = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_vni_resv_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.vni_resv_1 << 18 | \
		reg_val.bf0.vni_resv_0;
	return ret;
}

sw_error_t
appe_tl_tbl_vni_resv_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.vni_resv_1 = value >> 18;
	reg_val.bf0.vni_resv_0 = value & (((a_uint64_t)1<<18)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_udp_csum_zero_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.udp_csum_zero;
	return ret;
}

sw_error_t
appe_tl_tbl_udp_csum_zero_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.udp_csum_zero = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_src_info_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.src_info_type;
	return ret;
}

sw_error_t
appe_tl_tbl_src_info_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.src_info_type = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_src_info_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.src_info;
	return ret;
}

sw_error_t
appe_tl_tbl_src_info_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.src_info = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_udf1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.udf1_1 << 2 | \
		reg_val.bf0.udf1_0;
	return ret;
}

sw_error_t
appe_tl_tbl_udf1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.udf1_1 = value >> 2;
	reg_val.bf0.udf1_0 = value & (((a_uint64_t)1<<2)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_cpcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.cpcp_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_cpcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.cpcp_mode = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_ipv4_src_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf1.ipv4_src_addr_1 << 18 | \
		reg_val.bf1.ipv4_src_addr_0;
	return ret;
}

sw_error_t
appe_tl_tbl_ipv4_src_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf1.ipv4_src_addr_1 = value >> 18;
	reg_val.bf1.ipv4_src_addr_0 = value & (((a_uint64_t)1<<18)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_ipv4_dst_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf1.ipv4_dst_addr_1 << 18 | \
		reg_val.bf1.ipv4_dst_addr_0;
	return ret;
}

sw_error_t
appe_tl_tbl_ipv4_dst_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf1.ipv4_dst_addr_1 = value >> 18;
	reg_val.bf1.ipv4_dst_addr_0 = value & (((a_uint64_t)1<<18)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_udf1_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.udf1_valid;
	return ret;
}

sw_error_t
appe_tl_tbl_udf1_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.udf1_valid = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_fwd_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.fwd_type;
	return ret;
}

sw_error_t
appe_tl_tbl_fwd_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.fwd_type = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_service_code_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.service_code_en;
	return ret;
}

sw_error_t
appe_tl_tbl_service_code_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.service_code_en = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_svlan_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.svlan_id_1 << 7 | \
		reg_val.bf0.svlan_id_0;
	return ret;
}

sw_error_t
appe_tl_tbl_svlan_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.svlan_id_1 = value >> 7;
	reg_val.bf0.svlan_id_0 = value & (((a_uint64_t)1<<7)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_cvlan_check_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.cvlan_check_en;
	return ret;
}

sw_error_t
appe_tl_tbl_cvlan_check_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.cvlan_check_en = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_service_code_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.service_code;
	return ret;
}

sw_error_t
appe_tl_tbl_service_code_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.service_code = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_src_info_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.src_info_valid;
	return ret;
}

sw_error_t
appe_tl_tbl_src_info_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.src_info_valid = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_entry_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.entry_type;
	return ret;
}

sw_error_t
appe_tl_tbl_entry_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.entry_type = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_cvlan_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.cvlan_id;
	return ret;
}

sw_error_t
appe_tl_tbl_cvlan_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.cvlan_id = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_ipv6_src_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = (a_uint64_t)reg_val.bf0.ipv6_src_addr_1 << 18 | \
		reg_val.bf0.ipv6_src_addr_0;
	return ret;
}

sw_error_t
appe_tl_tbl_ipv6_src_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.ipv6_src_addr_1 = value >> 18;
	reg_val.bf0.ipv6_src_addr_0 = value & (((a_uint64_t)1<<18)-1);
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_tbl_ttl_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf0.ttl_mode;
	return ret;
}

sw_error_t
appe_tl_tbl_ttl_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf0.ttl_mode = value;
	ret = appe_tl_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_df_set_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union eg_ipv4_hdr_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_ipv4_hdr_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.ipv4_df_set;
	return ret;
}

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_df_set_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union eg_ipv4_hdr_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_ipv4_hdr_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv4_df_set = value;
	ret = appe_eg_ipv4_hdr_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_id_seed_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union eg_ipv4_hdr_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_ipv4_hdr_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.ipv4_id_seed;
	return ret;
}

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_id_seed_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union eg_ipv4_hdr_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_ipv4_hdr_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv4_id_seed = value;
	ret = appe_eg_ipv4_hdr_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_eg_udp_entropy_ctrl_port_mask_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union eg_udp_entropy_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_udp_entropy_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.port_mask;
	return ret;
}

sw_error_t
appe_eg_udp_entropy_ctrl_port_mask_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union eg_udp_entropy_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_udp_entropy_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.port_mask = value;
	ret = appe_eg_udp_entropy_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_eg_udp_entropy_ctrl_port_base_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union eg_udp_entropy_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_udp_entropy_ctrl_get(dev_id, &reg_val);
	*value = reg_val.bf.port_base;
	return ret;
}

sw_error_t
appe_eg_udp_entropy_ctrl_port_base_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union eg_udp_entropy_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_udp_entropy_ctrl_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.port_base = value;
	ret = appe_eg_udp_entropy_ctrl_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_ecn_profile_profile2_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union ecn_profile_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ecn_profile_get(dev_id, &reg_val);
	*value = reg_val.bf.profile2;
	return ret;
}

sw_error_t
appe_ecn_profile_profile2_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union ecn_profile_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ecn_profile_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.profile2 = value;
	ret = appe_ecn_profile_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_ecn_profile_profile1_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union ecn_profile_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ecn_profile_get(dev_id, &reg_val);
	*value = reg_val.bf.profile1;
	return ret;
}

sw_error_t
appe_ecn_profile_profile1_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union ecn_profile_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ecn_profile_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.profile1 = value;
	ret = appe_ecn_profile_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_ecn_profile_profile0_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union ecn_profile_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ecn_profile_get(dev_id, &reg_val);
	*value = reg_val.bf.profile0;
	return ret;
}

sw_error_t
appe_ecn_profile_profile0_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union ecn_profile_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ecn_profile_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.profile0 = value;
	ret = appe_ecn_profile_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_eg_proto_mapping0_protocol0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_proto_mapping0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_proto_mapping0_get(dev_id, index, &reg_val);
	*value = reg_val.bf.protocol0;
	return ret;
}

sw_error_t
appe_eg_proto_mapping0_protocol0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_proto_mapping0_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_proto_mapping0_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.protocol0 = value;
	ret = appe_eg_proto_mapping0_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_proto_mapping1_protocol1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_proto_mapping1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_proto_mapping1_get(dev_id, index, &reg_val);
	*value = reg_val.bf.protocol1;
	return ret;
}

sw_error_t
appe_eg_proto_mapping1_protocol1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_proto_mapping1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_proto_mapping1_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.protocol1 = value;
	ret = appe_eg_proto_mapping1_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_dbg_addr_dbg_addr_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union dbg_addr_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_dbg_addr_get(dev_id, &reg_val);
	*value = reg_val.bf.dbg_addr;
	return ret;
}

sw_error_t
appe_dbg_addr_dbg_addr_set(
		a_uint32_t dev_id,
		a_uint32_t value)
{
	union dbg_addr_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_dbg_addr_get(dev_id, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dbg_addr = value;
	ret = appe_dbg_addr_set(dev_id, &reg_val);
	return ret;
}

sw_error_t
appe_dbg_data_dbg_data_get(
		a_uint32_t dev_id,
		a_uint32_t *value)
{
	union dbg_data_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_dbg_data_get(dev_id, &reg_val);
	*value = reg_val.bf.dbg_data;
	return ret;
}

sw_error_t
appe_tx_buff_thrsh_xon_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tx_buff_thrsh_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tx_buff_thrsh_get(dev_id, index, &reg_val);
	*value = reg_val.bf.xon;
	return ret;
}

sw_error_t
appe_tx_buff_thrsh_xon_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tx_buff_thrsh_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tx_buff_thrsh_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.xon = value;
	ret = appe_tx_buff_thrsh_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tx_buff_thrsh_xoff_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tx_buff_thrsh_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tx_buff_thrsh_get(dev_id, index, &reg_val);
	*value = reg_val.bf.xoff;
	return ret;
}

sw_error_t
appe_tx_buff_thrsh_xoff_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tx_buff_thrsh_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tx_buff_thrsh_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.xoff = value;
	ret = appe_tx_buff_thrsh_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_header_data_data_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value)
{
	union eg_header_data_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_header_data_get(dev_id, index, &reg_val);
	*value = (a_uint64_t)reg_val.bf.data_1 << 32 | \
		reg_val.bf.data_0;
	return ret;
}

sw_error_t
appe_eg_header_data_data_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value)
{
	union eg_header_data_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_header_data_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.data_1 = value >> 32;
	reg_val.bf.data_0 = value & (((a_uint64_t)1<<32)-1);
	ret = appe_eg_header_data_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ip_proto_update_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ip_proto_update;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ip_proto_update_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ip_proto_update = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_cpcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cpcp_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_cpcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cpcp_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_data_length_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.data_length;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_data_length_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.data_length = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_resv_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.resv;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_resv_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.resv = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_cdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cdei_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_cdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cdei_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_dscp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dscp_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_dscp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dscp_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ip_ver_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ip_ver;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ip_ver_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ip_ver = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_id_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ipv4_id_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_id_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv4_id_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l3_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l3_offset;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l3_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l3_offset = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l4_checksum_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_checksum_en;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l4_checksum_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_checksum_en = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ipv6_fl_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ipv6_fl_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ipv6_fl_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv6_fl_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_sport_entropy_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.sport_entropy_en;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_sport_entropy_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.sport_entropy_en = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ctag_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ctag_fmt;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ctag_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ctag_fmt = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_pppoe_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.pppoe_en;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_pppoe_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pppoe_en = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_vni_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.vni_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_vni_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.vni_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l4_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_type;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l4_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_type = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_target_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.edit_rule_target;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_target_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.edit_rule_target = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_sdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.sdei_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_sdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.sdei_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.output_vp;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.output_vp = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ecn_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ecn_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ecn_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ecn_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_spcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.spcp_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_spcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.spcp_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_vlan_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.vlan_offset;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_vlan_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.vlan_offset = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.edit_rule_id;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.edit_rule_id = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_payload_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.payload_type;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_payload_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.payload_type = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l4_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_offset;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_l4_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_offset = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_tunnel_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.tunnel_offset;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_tunnel_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.tunnel_offset = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.output_vp_valid;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.output_vp_valid = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ttl_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ttl_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ttl_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ttl_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.type;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.type = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_stag_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.stag_fmt;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_stag_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.stag_fmt = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_df_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ipv4_df_mode;
	return ret;
}

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_df_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_xlat_tun_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_xlat_tun_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ipv4_df_mode = value;
	ret = appe_eg_xlat_tun_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_src1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.src1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_src1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.src1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.valid3_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.valid3_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.pos2_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pos2_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.valid2_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.valid2_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_src2_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.src2;
	return ret;
}

sw_error_t
appe_eg_edit_rule_src2_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.src2 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.pos2_1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pos2_1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_start3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.start3_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_start3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.start3_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.pos3_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pos3_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_width2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.width2_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_width2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.width2_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.valid2_1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.valid2_1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_width2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.width2_1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_width2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.width2_1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_start3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.start3_1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_start3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.start3_1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_start2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.start2_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_start2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.start2_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_src3_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.src3;
	return ret;
}

sw_error_t
appe_eg_edit_rule_src3_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.src3 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.pos3_1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_pos3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pos3_1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_width3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.width3_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_width3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.width3_0 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.valid3_1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_valid3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.valid3_1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_width3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.width3_1;
	return ret;
}

sw_error_t
appe_eg_edit_rule_width3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.width3_1 = value;
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_edit_rule_start2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	*value = reg_val.bf.start2_1_1 << 4 | \
		reg_val.bf.start2_1_0;
	return ret;
}

sw_error_t
appe_eg_edit_rule_start2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_edit_rule_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_edit_rule_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.start2_1_1 = value >> 4;
	reg_val.bf.start2_1_0 = value & (((a_uint64_t)1<<4)-1);
	ret = appe_eg_edit_rule_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_cnt_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_cnt_tbl_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				INGRESS_POLICER_BASE_ADDR + TL_CNT_TBL_ADDRESS + \
				index * TL_CNT_TBL_INC,
				value->val,
				sizeof(union tl_cnt_tbl_u) / sizeof(a_uint32_t));
}

sw_error_t
appe_tl_cnt_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_cnt_tbl_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				INGRESS_POLICER_BASE_ADDR + TL_CNT_TBL_ADDRESS + \
				index * TL_CNT_TBL_INC,
				value->val,
				sizeof(union tl_cnt_tbl_u) / sizeof(a_uint32_t));
}

sw_error_t
appe_tl_port_vp_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_port_vp_tbl_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_PORT_VP_TBL_ADDRESS + \
				index * TL_PORT_VP_TBL_INC,
				value->val,
				sizeof(union tl_port_vp_tbl_u) / sizeof(a_uint32_t));
}

sw_error_t
appe_tl_port_vp_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_port_vp_tbl_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_PORT_VP_TBL_ADDRESS + \
				index * TL_PORT_VP_TBL_INC,
				value->val,
				sizeof(union tl_port_vp_tbl_u) / sizeof(a_uint32_t));
}

sw_error_t
appe_tl_port_vp_tbl_pre_ipo_profile_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union tl_port_vp_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_port_vp_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.pre_ipo_profile;
	return ret;
}

sw_error_t
appe_tl_port_vp_tbl_pre_ipo_profile_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union tl_port_vp_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_tl_port_vp_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pre_ipo_profile = value;
	ret = appe_tl_port_vp_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_tl_vlan_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_vlan_tbl_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_VLAN_TBL_ADDRESS + \
				index * TL_VLAN_TBL_INC,
				value->val,
				sizeof(union tl_vlan_tbl_u) / sizeof(a_uint32_t));
}

sw_error_t
appe_tl_vlan_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_vlan_tbl_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				TUNNEL_LOOKUP_BASE_ADDR + TL_VLAN_TBL_ADDRESS + \
				index * TL_VLAN_TBL_INC,
				value->val,
				sizeof(union tl_vlan_tbl_u) / sizeof(a_uint32_t));
}

sw_error_t
appe_ecn_map_mode0_0_get(
		a_uint32_t dev_id,
		union ecn_map_mode0_0_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE0_0_ADDRESS,
				&value->val);
}

sw_error_t
appe_ecn_map_mode0_0_set(
		a_uint32_t dev_id,
		union ecn_map_mode0_0_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE0_0_ADDRESS,
				value->val);
}

sw_error_t
appe_ecn_map_mode0_1_get(
		a_uint32_t dev_id,
		union ecn_map_mode0_1_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE0_1_ADDRESS,
				&value->val);
}

sw_error_t
appe_ecn_map_mode0_1_set(
		a_uint32_t dev_id,
		union ecn_map_mode0_1_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE0_1_ADDRESS,
				value->val);
}

sw_error_t
appe_ecn_map_mode1_0_get(
		a_uint32_t dev_id,
		union ecn_map_mode1_0_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE1_0_ADDRESS,
				&value->val);
}

sw_error_t
appe_ecn_map_mode1_0_set(
		a_uint32_t dev_id,
		union ecn_map_mode1_0_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE1_0_ADDRESS,
				value->val);
}

sw_error_t
appe_ecn_map_mode1_1_get(
		a_uint32_t dev_id,
		union ecn_map_mode1_1_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE1_1_ADDRESS,
				&value->val);
}

sw_error_t
appe_ecn_map_mode1_1_set(
		a_uint32_t dev_id,
		union ecn_map_mode1_1_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE1_1_ADDRESS,
				value->val);
}

sw_error_t
appe_ecn_map_mode2_0_get(
		a_uint32_t dev_id,
		union ecn_map_mode2_0_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE2_0_ADDRESS,
				&value->val);
}

sw_error_t
appe_ecn_map_mode2_0_set(
		a_uint32_t dev_id,
		union ecn_map_mode2_0_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE2_0_ADDRESS,
				value->val);
}

sw_error_t
appe_ecn_map_mode2_1_get(
		a_uint32_t dev_id,
		union ecn_map_mode2_1_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE2_1_ADDRESS,
				&value->val);
}

sw_error_t
appe_ecn_map_mode2_1_set(
		a_uint32_t dev_id,
		union ecn_map_mode2_1_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + ECN_MAP_MODE2_1_ADDRESS,
				value->val);
}
