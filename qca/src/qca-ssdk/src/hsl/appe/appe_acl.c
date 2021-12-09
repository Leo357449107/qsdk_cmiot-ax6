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
#include "appe_acl_reg.h"
#include "appe_acl.h"

sw_error_t
appe_ipr_udf_ctrl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union ipr_udf_ctrl_u *value)
{
	if (index >= IPR_UDF_CTRL_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				IPR_CSR_BASE_ADDR + IPR_UDF_CTRL_ADDRESS + \
				index * IPR_UDF_CTRL_INC,
				&value->val);
}

sw_error_t
appe_ipr_udf_ctrl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union ipr_udf_ctrl_u *value)
{
	return hppe_reg_set(
				dev_id,
				IPR_CSR_BASE_ADDR + IPR_UDF_CTRL_ADDRESS + \
				index * IPR_UDF_CTRL_INC,
				value->val);
}

sw_error_t
appe_ipr_udf_profile_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union ipr_udf_profile_base_u *value)
{
	if (index >= IPR_UDF_PROFILE_BASE_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				IPR_CSR_BASE_ADDR + IPR_UDF_PROFILE_BASE_ADDRESS + \
				index * IPR_UDF_PROFILE_BASE_INC,
				&value->val);
}

sw_error_t
appe_ipr_udf_profile_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union ipr_udf_profile_base_u *value)
{
	return hppe_reg_set(
				dev_id,
				IPR_CSR_BASE_ADDR + IPR_UDF_PROFILE_BASE_ADDRESS + \
				index * IPR_UDF_PROFILE_BASE_INC,
				value->val);
}

sw_error_t
appe_ipr_udf_profile_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union ipr_udf_profile_offset_u *value)
{
	if (index >= IPR_UDF_PROFILE_OFFSET_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				IPR_CSR_BASE_ADDR + IPR_UDF_PROFILE_OFFSET_ADDRESS + \
				index * IPR_UDF_PROFILE_OFFSET_INC,
				&value->val);
}

sw_error_t
appe_ipr_udf_profile_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union ipr_udf_profile_offset_u *value)
{
	return hppe_reg_set(
				dev_id,
				IPR_CSR_BASE_ADDR + IPR_UDF_PROFILE_OFFSET_ADDRESS + \
				index * IPR_UDF_PROFILE_OFFSET_INC,
				value->val);
}

sw_error_t
appe_ipr_udf_ctrl_l3_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l3_type_incl;
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_l3_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l3_type_incl = value;
	ret = appe_ipr_udf_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_l4_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_type;
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_l4_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_type = value;
	ret = appe_ipr_udf_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_udf_profile_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf_profile;
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_udf_profile_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf_profile = value;
	ret = appe_ipr_udf_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.valid;
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.valid = value;
	ret = appe_ipr_udf_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_l3_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l3_type;
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_l3_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l3_type = value;
	ret = appe_ipr_udf_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_l4_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_type_incl;
	return ret;
}

sw_error_t
appe_ipr_udf_ctrl_l4_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_ctrl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_ctrl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_type_incl = value;
	ret = appe_ipr_udf_ctrl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf3_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf3_base;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf3_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf3_base = value;
	ret = appe_ipr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf0_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf0_base;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf0_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf0_base = value;
	ret = appe_ipr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf1_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf1_base;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf1_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf1_base = value;
	ret = appe_ipr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf2_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf2_base;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_base_udf2_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_base_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_base_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf2_base = value;
	ret = appe_ipr_udf_profile_base_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf1_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf1_offset;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf1_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf1_offset = value;
	ret = appe_ipr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf3_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf3_offset;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf3_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf3_offset = value;
	ret = appe_ipr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf0_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf0_offset;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf0_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf0_offset = value;
	ret = appe_ipr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf2_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	*value = reg_val.bf.udf2_offset;
	return ret;
}

sw_error_t
appe_ipr_udf_profile_offset_udf2_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union ipr_udf_profile_offset_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_ipr_udf_profile_offset_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.udf2_offset = value;
	ret = appe_ipr_udf_profile_offset_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_eg_ipo_ext_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_ipo_ext_tbl_u *value)
{
	if (index >= EG_IPO_EXT_TBL_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_IPO_EXT_TBL_ADDRESS + \
				index * EG_IPO_EXT_TBL_INC,
				&value->val);
}

sw_error_t
appe_eg_ipo_ext_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_ipo_ext_tbl_u *value)
{
	return hppe_reg_set(
				dev_id,
				NSS_PTX_CSR_BASE_ADDR + EG_IPO_EXT_TBL_ADDRESS + \
				index * EG_IPO_EXT_TBL_INC,
				value->val);
}

sw_error_t
appe_eg_ipo_ext_tbl_policy_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union eg_ipo_ext_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_ipo_ext_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.policy_id;
	return ret;
}

sw_error_t
appe_eg_ipo_ext_tbl_policy_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union eg_ipo_ext_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_eg_ipo_ext_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.policy_id = value;
	ret = appe_eg_ipo_ext_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_reg_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_REG_ADDRESS + \
				index * PRE_IPO_RULE_REG_INC,
				value->val,
				3);
}

sw_error_t
appe_pre_ipo_rule_reg_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_reg_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_REG_ADDRESS + \
				index * PRE_IPO_RULE_REG_INC,
				value->val,
				3);
}

sw_error_t
appe_pre_ipo_mask_reg_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_mask_reg_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_MASK_REG_ADDRESS + \
				index * PRE_IPO_MASK_REG_INC,
				value->val,
				2);
}

sw_error_t
appe_pre_ipo_mask_reg_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_mask_reg_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_MASK_REG_ADDRESS + \
				index * PRE_IPO_MASK_REG_INC,
				value->val,
				2);
}

sw_error_t
appe_pre_ipo_rule_ext_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_ext_1_u *value)
{
	if (index >= PRE_IPO_RULE_EXT_1_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_EXT_1_ADDRESS + \
				index * PRE_IPO_RULE_EXT_1_INC,
				&value->val);
}

sw_error_t
appe_pre_ipo_rule_ext_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_ext_1_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_EXT_1_ADDRESS + \
				index * PRE_IPO_RULE_EXT_1_INC,
				value->val);
}

sw_error_t
appe_pre_ipo_rule_ext_2_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_ext_2_u *value)
{
	if (index >= PRE_IPO_RULE_EXT_2_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_EXT_2_ADDRESS + \
				index * PRE_IPO_RULE_EXT_2_INC,
				&value->val);
}

sw_error_t
appe_pre_ipo_rule_ext_2_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_ext_2_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_EXT_2_ADDRESS + \
				index * PRE_IPO_RULE_EXT_2_INC,
				value->val);
}

sw_error_t
appe_pre_ipo_rule_ext_4_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_ext_4_u *value)
{
	if (index >= PRE_IPO_RULE_EXT_4_MAX_ENTRY)
		return SW_OUT_OF_RANGE;
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_EXT_4_ADDRESS + \
				index * PRE_IPO_RULE_EXT_4_INC,
				&value->val);
}

sw_error_t
appe_pre_ipo_rule_ext_4_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_rule_ext_4_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_RULE_EXT_4_ADDRESS + \
				index * PRE_IPO_RULE_EXT_4_INC,
				value->val);
}

sw_error_t
appe_pre_ipo_dbg_addr_get(
		a_uint32_t dev_id,
		union pre_ipo_dbg_addr_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_DBG_ADDR_ADDRESS,
				&value->val);
}

sw_error_t
appe_pre_ipo_dbg_addr_set(
		a_uint32_t dev_id,
		union pre_ipo_dbg_addr_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_DBG_ADDR_ADDRESS,
				value->val);
}

sw_error_t
appe_pre_ipo_dbg_data_get(
		a_uint32_t dev_id,
		union pre_ipo_dbg_data_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_DBG_DATA_ADDRESS,
				&value->val);
}

sw_error_t
appe_pre_ipo_dbg_data_set(
		a_uint32_t dev_id,
		union pre_ipo_dbg_data_u *value)
{
	return SW_NOT_SUPPORTED;
}

sw_error_t
appe_pre_ipo_spare_reg_get(
		a_uint32_t dev_id,
		union pre_ipo_spare_reg_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_SPARE_REG_ADDRESS,
				&value->val);
}

sw_error_t
appe_pre_ipo_spare_reg_set(
		a_uint32_t dev_id,
		union pre_ipo_spare_reg_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_SPARE_REG_ADDRESS,
				value->val);
}

sw_error_t
appe_pre_ipo_glb_hit_counter_get(
		a_uint32_t dev_id,
		union pre_ipo_glb_hit_counter_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_GLB_HIT_COUNTER_ADDRESS,
				&value->val);
}

sw_error_t
appe_pre_ipo_glb_hit_counter_set(
		a_uint32_t dev_id,
		union pre_ipo_glb_hit_counter_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_GLB_HIT_COUNTER_ADDRESS,
				value->val);
}

sw_error_t
appe_pre_ipo_glb_miss_counter_get(
		a_uint32_t dev_id,
		union pre_ipo_glb_miss_counter_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_GLB_MISS_COUNTER_ADDRESS,
				&value->val);
}

sw_error_t
appe_pre_ipo_glb_miss_counter_set(
		a_uint32_t dev_id,
		union pre_ipo_glb_miss_counter_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_GLB_MISS_COUNTER_ADDRESS,
				value->val);
}

sw_error_t
appe_pre_ipo_glb_bypass_counter_get(
		a_uint32_t dev_id,
		union pre_ipo_glb_bypass_counter_u *value)
{
	return hppe_reg_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_GLB_BYPASS_COUNTER_ADDRESS,
				&value->val);
}

sw_error_t
appe_pre_ipo_glb_bypass_counter_set(
		a_uint32_t dev_id,
		union pre_ipo_glb_bypass_counter_u *value)
{
	return hppe_reg_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_GLB_BYPASS_COUNTER_ADDRESS,
				value->val);
}

sw_error_t
appe_pre_ipo_cnt_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_cnt_tbl_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				INGRESS_POLICER_BASE_ADDR + PRE_IPO_CNT_TBL_ADDRESS + \
				index * PRE_IPO_CNT_TBL_INC,
				value->val,
				3);
}

sw_error_t
appe_pre_ipo_cnt_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_cnt_tbl_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				INGRESS_POLICER_BASE_ADDR + PRE_IPO_CNT_TBL_ADDRESS + \
				index * PRE_IPO_CNT_TBL_INC,
				value->val,
				3);
}

sw_error_t
appe_pre_ipo_action_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_action_u *value)
{
	return hppe_reg_tbl_get(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_ACTION_ADDRESS + \
				index * PRE_IPO_ACTION_INC,
				value->val,
				6);
}

sw_error_t
appe_pre_ipo_action_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union pre_ipo_action_u *value)
{
	return hppe_reg_tbl_set(
				dev_id,
				PRE_IPO_CSR_BASE_ADDR + PRE_IPO_ACTION_ADDRESS + \
				index * PRE_IPO_ACTION_INC,
				value->val,
				6);
}

sw_error_t
appe_pre_ipo_rule_reg_src_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.src_1 << 1 | \
		reg_val.bf.src_0;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_src_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.src_1 = value >> 1;
	reg_val.bf.src_0 = value & (((a_uint64_t)1<<1)-1);
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_inverse_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.inverse_en;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_inverse_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.inverse_en = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_rule_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.rule_type;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_rule_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.rule_type = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_src_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.src_type;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_src_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.src_type = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_range_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.range_en;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_range_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.range_en = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_inner_outer_sel_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.inner_outer_sel;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_inner_outer_sel_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.inner_outer_sel = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_fake_mac_header_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.fake_mac_header;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_fake_mac_header_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.fake_mac_header = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_res_chain_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.res_chain;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_res_chain_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.res_chain = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_pri_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = reg_val.bf.pri;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_pri_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.pri = value;
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_rule_field_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	*value = (a_uint64_t)reg_val.bf.rule_field_1 << 32 | \
		reg_val.bf.rule_field_0;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_reg_rule_field_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value)
{
	union pre_ipo_rule_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.rule_field_1 = value >> 32;
	reg_val.bf.rule_field_0 = value & (((a_uint64_t)1<<32)-1);
	ret = appe_pre_ipo_rule_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_mask_reg_maskfield_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value)
{
	union pre_ipo_mask_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_mask_reg_get(dev_id, index, &reg_val);
	*value = (a_uint64_t)reg_val.bf.maskfield_1 << 32 | \
		reg_val.bf.maskfield_0;
	return ret;
}

sw_error_t
appe_pre_ipo_mask_reg_maskfield_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value)
{
	union pre_ipo_mask_reg_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_mask_reg_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.maskfield_1 = value >> 32;
	reg_val.bf.maskfield_0 = value & (((a_uint64_t)1<<32)-1);
	ret = appe_pre_ipo_mask_reg_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_2_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ext2_2;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_2_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ext2_2 = value;
	ret = appe_pre_ipo_rule_ext_1_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ext2_0;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ext2_0 = value;
	ret = appe_pre_ipo_rule_ext_1_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_3_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ext2_3;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_3_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ext2_3 = value;
	ret = appe_pre_ipo_rule_ext_1_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ext2_1;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_1_ext2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_ext_1_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_1_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ext2_1 = value;
	ret = appe_pre_ipo_rule_ext_1_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_2_ext4_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_ext_2_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_2_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ext4_0;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_2_ext4_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_ext_2_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_2_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ext4_0 = value;
	ret = appe_pre_ipo_rule_ext_2_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_2_ext4_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_ext_2_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_2_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ext4_1;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_2_ext4_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_ext_2_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_2_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ext4_1 = value;
	ret = appe_pre_ipo_rule_ext_2_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_4_ext8_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_rule_ext_4_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_4_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ext8;
	return ret;
}

sw_error_t
appe_pre_ipo_rule_ext_4_ext8_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_rule_ext_4_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_rule_ext_4_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ext8 = value;
	ret = appe_pre_ipo_rule_ext_4_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_cnt_tbl_hit_byte_cnt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value)
{
	union pre_ipo_cnt_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_cnt_tbl_get(dev_id, index, &reg_val);
	*value = (a_uint64_t)reg_val.bf.hit_byte_cnt_1 << 32 | \
		reg_val.bf.hit_byte_cnt_0;
	return ret;
}

sw_error_t
appe_pre_ipo_cnt_tbl_hit_byte_cnt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value)
{
	union pre_ipo_cnt_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_cnt_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.hit_byte_cnt_1 = value >> 32;
	reg_val.bf.hit_byte_cnt_0 = value & (((a_uint64_t)1<<32)-1);
	ret = appe_pre_ipo_cnt_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_cnt_tbl_hit_pkt_cnt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_cnt_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_cnt_tbl_get(dev_id, index, &reg_val);
	*value = reg_val.bf.hit_pkt_cnt;
	return ret;
}

sw_error_t
appe_pre_ipo_cnt_tbl_hit_pkt_cnt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_cnt_tbl_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_cnt_tbl_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.hit_pkt_cnt = value;
	ret = appe_pre_ipo_cnt_tbl_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_l4_port_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.l4_port;
	return ret;
}

sw_error_t
appe_pre_ipo_action_l4_port_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.l4_port = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_vpn_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.vpn_valid;
	return ret;
}

sw_error_t
appe_pre_ipo_action_vpn_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.vpn_valid = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_mirror_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.mirror_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_mirror_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.mirror_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_pcp_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ctag_pcp_1 << 2 | \
		reg_val.bf.ctag_pcp_0;
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_pcp_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ctag_pcp_1 = value >> 2;
	reg_val.bf.ctag_pcp_0 = value & (((a_uint64_t)1<<2)-1);
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_int_dp_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.int_dp_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_int_dp_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.int_dp_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_enqueue_pri_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.enqueue_pri;
	return ret;
}

sw_error_t
appe_pre_ipo_action_enqueue_pri_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.enqueue_pri = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_pcp_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.stag_pcp_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_pcp_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.stag_pcp_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_nat_action_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.nat_action;
	return ret;
}

sw_error_t
appe_pre_ipo_action_nat_action_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.nat_action = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_dscp_tc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dscp_tc;
	return ret;
}

sw_error_t
appe_pre_ipo_action_dscp_tc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dscp_tc = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_cpu_code_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cpu_code_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_cpu_code_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cpu_code_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_learn_dis_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.learn_dis;
	return ret;
}

sw_error_t
appe_pre_ipo_action_learn_dis_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.learn_dis = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_vpn_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.vpn_id_1 << 5 | \
		reg_val.bf.vpn_id_0;
	return ret;
}

sw_error_t
appe_pre_ipo_action_vpn_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.vpn_id_1 = value >> 5;
	reg_val.bf.vpn_id_0 = value & (((a_uint64_t)1<<5)-1);
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_dei_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.stag_dei_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_dei_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.stag_dei_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ctag_fmt;
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ctag_fmt = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_dest_info_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dest_info;
	return ret;
}

sw_error_t
appe_pre_ipo_action_dest_info_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dest_info = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_svid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.svid;
	return ret;
}

sw_error_t
appe_pre_ipo_action_svid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.svid = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_dest_info_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dest_info_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_dest_info_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dest_info_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_policer_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.policer_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_policer_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.policer_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_cascade_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cascade_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_cascade_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cascade_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_int_dp_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.int_dp;
	return ret;
}

sw_error_t
appe_pre_ipo_action_int_dp_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.int_dp = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_pcp_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ctag_pcp_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_pcp_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ctag_pcp_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_metadata_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.metadata_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_metadata_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.metadata_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_enqueue_pri_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.enqueue_pri_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_enqueue_pri_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.enqueue_pri_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_vpn_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.vpn_type;
	return ret;
}

sw_error_t
appe_pre_ipo_action_vpn_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.vpn_type = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_dei_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.stag_dei;
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_dei_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.stag_dei = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_fwd_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.fwd_cmd;
	return ret;
}

sw_error_t
appe_pre_ipo_action_fwd_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.fwd_cmd = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_dei_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ctag_dei_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_dei_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ctag_dei_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_policer_index_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.policer_index;
	return ret;
}

sw_error_t
appe_pre_ipo_action_policer_index_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.policer_index = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_dei_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.ctag_dei;
	return ret;
}

sw_error_t
appe_pre_ipo_action_ctag_dei_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.ctag_dei = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_pcp_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.stag_pcp;
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_pcp_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.stag_pcp = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_syn_toggle_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.syn_toggle;
	return ret;
}

sw_error_t
appe_pre_ipo_action_syn_toggle_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.syn_toggle = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_service_code_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.service_code_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_service_code_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.service_code_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_qid_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.qid_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_qid_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.qid_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_cascade_data_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cascade_data_1 << 3 | \
		reg_val.bf.cascade_data_0;
	return ret;
}

sw_error_t
appe_pre_ipo_action_cascade_data_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cascade_data_1 = value >> 3;
	reg_val.bf.cascade_data_0 = value & (((a_uint64_t)1<<3)-1);
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_service_code_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.service_code_1 << 1 | \
		reg_val.bf.service_code_0;
	return ret;
}

sw_error_t
appe_pre_ipo_action_service_code_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.service_code_1 = value >> 1;
	reg_val.bf.service_code_0 = value & (((a_uint64_t)1<<1)-1);
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_cvid_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cvid_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_cvid_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cvid_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_dscp_tc_mask_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dscp_tc_mask;
	return ret;
}

sw_error_t
appe_pre_ipo_action_dscp_tc_mask_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dscp_tc_mask = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_cvid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cvid;
	return ret;
}

sw_error_t
appe_pre_ipo_action_cvid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cvid = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_svid_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.svid_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_svid_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.svid_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_cpu_code_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.cpu_code;
	return ret;
}

sw_error_t
appe_pre_ipo_action_cpu_code_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.cpu_code = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_dscp_tc_change_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.dscp_tc_change_en;
	return ret;
}

sw_error_t
appe_pre_ipo_action_dscp_tc_change_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.dscp_tc_change_en = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_qid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.qid;
	return ret;
}

sw_error_t
appe_pre_ipo_action_qid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.qid = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_qos_res_prec_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.qos_res_prec;
	return ret;
}

sw_error_t
appe_pre_ipo_action_qos_res_prec_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.qos_res_prec = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	*value = reg_val.bf.stag_fmt;
	return ret;
}

sw_error_t
appe_pre_ipo_action_stag_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value)
{
	union pre_ipo_action_u reg_val;
	sw_error_t ret = SW_OK;

	ret = appe_pre_ipo_action_get(dev_id, index, &reg_val);
	if (SW_OK != ret)
		return ret;
	reg_val.bf.stag_fmt = value;
	ret = appe_pre_ipo_action_set(dev_id, index, &reg_val);
	return ret;
}