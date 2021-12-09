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
#ifndef _APPE_TUNNEL_H_
#define _APPE_TUNNEL_H_

#define TPR_UDF_CTRL_0_MAX_ENTRY	16
#define TPR_UDF_PROFILE_BASE_MAX_ENTRY	8
#define TPR_UDF_PROFILE_OFFSET_MAX_ENTRY	8
#define TL_L3_IF_TBL_MAX_ENTRY		128
#define TL_KEY_GEN_MAX_ENTRY		16
#define TL_TBL_MAX_ENTRY		128
#define EG_PROTO_MAPPING0_MAX_ENTRY	2
#define EG_PROTO_MAPPING1_MAX_ENTRY	2
#define TX_BUFF_THRSH_MAX_ENTRY		8
#define EG_HEADER_DATA_MAX_ENTRY	128
#define EG_XLAT_TUN_CTRL_MAX_ENTRY	128
#define EG_EDIT_RULE_MAX_ENTRY		16

sw_error_t
appe_tpr_udf_ctrl_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_ctrl_0_u *value);

sw_error_t
appe_tpr_udf_ctrl_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_ctrl_0_u *value);

sw_error_t
appe_tpr_udf_profile_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_base_u *value);

sw_error_t
appe_tpr_udf_profile_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_base_u *value);

sw_error_t
appe_tpr_udf_profile_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_offset_u *value);

sw_error_t
appe_tpr_udf_profile_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tpr_udf_profile_offset_u *value);

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_udf_profile_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_udf_profile_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_overlay_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_l4_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_program_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_program_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_program_type_incl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_program_type_incl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_ctrl_0_l3_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_base_udf3_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_base_udf3_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_base_udf1_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_base_udf1_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_base_udf0_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_base_udf0_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_base_udf2_base_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_base_udf2_base_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_offset_udf1_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_offset_udf1_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_offset_udf0_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_offset_udf0_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_offset_udf2_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_offset_udf2_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tpr_udf_profile_offset_udf3_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tpr_udf_profile_offset_udf3_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_op_get(
		a_uint32_t dev_id,
		union tl_tbl_op_u *value);

sw_error_t
appe_tl_tbl_op_set(
		a_uint32_t dev_id,
		union tl_tbl_op_u *value);

sw_error_t
appe_tl_tbl_op_data0_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data0_u *value);

sw_error_t
appe_tl_tbl_op_data0_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data0_u *value);

sw_error_t
appe_tl_tbl_op_data1_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data1_u *value);

sw_error_t
appe_tl_tbl_op_data1_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data1_u *value);

sw_error_t
appe_tl_tbl_op_data2_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data2_u *value);

sw_error_t
appe_tl_tbl_op_data2_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data2_u *value);

sw_error_t
appe_tl_tbl_op_data3_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data3_u *value);

sw_error_t
appe_tl_tbl_op_data3_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data3_u *value);

sw_error_t
appe_tl_tbl_op_data4_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data4_u *value);

sw_error_t
appe_tl_tbl_op_data4_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data4_u *value);

sw_error_t
appe_tl_tbl_op_data5_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data5_u *value);

sw_error_t
appe_tl_tbl_op_data5_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data5_u *value);

sw_error_t
appe_tl_tbl_op_data6_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data6_u *value);

sw_error_t
appe_tl_tbl_op_data6_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data6_u *value);

sw_error_t
appe_tl_tbl_op_data7_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data7_u *value);

sw_error_t
appe_tl_tbl_op_data7_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data7_u *value);

sw_error_t
appe_tl_tbl_op_data8_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data8_u *value);

sw_error_t
appe_tl_tbl_op_data8_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data8_u *value);

sw_error_t
appe_tl_tbl_op_data9_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data9_u *value);

sw_error_t
appe_tl_tbl_op_data9_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data9_u *value);

sw_error_t
appe_tl_tbl_op_data10_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data10_u *value);

sw_error_t
appe_tl_tbl_op_data10_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data10_u *value);

sw_error_t
appe_tl_tbl_op_data11_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data11_u *value);

sw_error_t
appe_tl_tbl_op_data11_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data11_u *value);

sw_error_t
appe_tl_tbl_op_data12_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data12_u *value);

sw_error_t
appe_tl_tbl_op_data12_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data12_u *value);

sw_error_t
appe_tl_tbl_op_data13_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data13_u *value);

sw_error_t
appe_tl_tbl_op_data13_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data13_u *value);

sw_error_t
appe_tl_tbl_op_data14_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data14_u *value);

sw_error_t
appe_tl_tbl_op_data14_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data14_u *value);

sw_error_t
appe_tl_tbl_op_data15_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data15_u *value);

sw_error_t
appe_tl_tbl_op_data15_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data15_u *value);

sw_error_t
appe_tl_tbl_op_data16_get(
		a_uint32_t dev_id,
		union tl_tbl_op_data16_u *value);

sw_error_t
appe_tl_tbl_op_data16_set(
		a_uint32_t dev_id,
		union tl_tbl_op_data16_u *value);

sw_error_t
appe_tl_tbl_rd_op_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_u *value);

sw_error_t
appe_tl_tbl_rd_op_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_u *value);

sw_error_t
appe_tl_tbl_rd_op_data0_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data0_u *value);

sw_error_t
appe_tl_tbl_rd_op_data0_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data0_u *value);

sw_error_t
appe_tl_tbl_rd_op_data1_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data1_u *value);

sw_error_t
appe_tl_tbl_rd_op_data1_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data1_u *value);

sw_error_t
appe_tl_tbl_rd_op_data2_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data2_u *value);

sw_error_t
appe_tl_tbl_rd_op_data2_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data2_u *value);

sw_error_t
appe_tl_tbl_rd_op_data3_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data3_u *value);

sw_error_t
appe_tl_tbl_rd_op_data3_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data3_u *value);

sw_error_t
appe_tl_tbl_rd_op_data4_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data4_u *value);

sw_error_t
appe_tl_tbl_rd_op_data4_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data4_u *value);

sw_error_t
appe_tl_tbl_rd_op_data5_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data5_u *value);

sw_error_t
appe_tl_tbl_rd_op_data5_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data5_u *value);

sw_error_t
appe_tl_tbl_rd_op_data6_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data6_u *value);

sw_error_t
appe_tl_tbl_rd_op_data6_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data6_u *value);

sw_error_t
appe_tl_tbl_rd_op_data7_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data7_u *value);

sw_error_t
appe_tl_tbl_rd_op_data7_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data7_u *value);

sw_error_t
appe_tl_tbl_rd_op_data8_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data8_u *value);

sw_error_t
appe_tl_tbl_rd_op_data8_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data8_u *value);

sw_error_t
appe_tl_tbl_rd_op_data9_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data9_u *value);

sw_error_t
appe_tl_tbl_rd_op_data9_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data9_u *value);

sw_error_t
appe_tl_tbl_rd_op_data10_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data10_u *value);

sw_error_t
appe_tl_tbl_rd_op_data10_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data10_u *value);

sw_error_t
appe_tl_tbl_rd_op_data11_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data11_u *value);

sw_error_t
appe_tl_tbl_rd_op_data11_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data11_u *value);

sw_error_t
appe_tl_tbl_rd_op_data12_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data12_u *value);

sw_error_t
appe_tl_tbl_rd_op_data12_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data12_u *value);

sw_error_t
appe_tl_tbl_rd_op_data13_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data13_u *value);

sw_error_t
appe_tl_tbl_rd_op_data13_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data13_u *value);

sw_error_t
appe_tl_tbl_rd_op_data14_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data14_u *value);

sw_error_t
appe_tl_tbl_rd_op_data14_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data14_u *value);

sw_error_t
appe_tl_tbl_rd_op_data15_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data15_u *value);

sw_error_t
appe_tl_tbl_rd_op_data15_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data15_u *value);

sw_error_t
appe_tl_tbl_rd_op_data16_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data16_u *value);

sw_error_t
appe_tl_tbl_rd_op_data16_set(
		a_uint32_t dev_id,
		union tl_tbl_rd_op_data16_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data0_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data0_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data1_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data1_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data2_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data2_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data3_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data3_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data4_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data4_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data5_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data5_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data6_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data6_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data7_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data7_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data8_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data8_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data9_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data9_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data10_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data10_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data11_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data11_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data12_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data12_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data13_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data13_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data14_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data14_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data15_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data15_u *value);

sw_error_t
appe_tl_tbl_rd_rslt_data16_get(
		a_uint32_t dev_id,
		union tl_tbl_rd_rslt_data16_u *value);

sw_error_t
appe_tl_ctrl_get(
		a_uint32_t dev_id,
		union tl_ctrl_u *value);

sw_error_t
appe_tl_ctrl_set(
		a_uint32_t dev_id,
		union tl_ctrl_u *value);

sw_error_t
appe_tl_l3_if_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_l3_if_tbl_u *value);

sw_error_t
appe_tl_l3_if_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_l3_if_tbl_u *value);

sw_error_t
appe_tl_key_gen_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_key_gen_u *value);

sw_error_t
appe_tl_key_gen_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_key_gen_u *value);

sw_error_t
appe_tl_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_tbl_u *value);

sw_error_t
appe_tl_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_tbl_u *value);

sw_error_t
appe_tl_tbl_op_op_mode_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_op_op_mode_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_op_op_type_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_op_op_type_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_op_op_rslt_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_op_op_rslt_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_op_busy_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_op_busy_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_op_entry_index_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_op_entry_index_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_op_hash_block_bitmap_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_op_hash_block_bitmap_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_rd_op_op_mode_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_rd_op_op_mode_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_rd_op_op_type_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_rd_op_op_type_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_rd_op_op_rslt_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_rd_op_op_rslt_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_rd_op_busy_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_rd_op_busy_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_rd_op_entry_index_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_rd_op_entry_index_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_tbl_rd_op_hash_block_bitmap_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_tbl_rd_op_hash_block_bitmap_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_hash_mode_0_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_hash_mode_0_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_pppoe_multicast_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_pppoe_multicast_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_vlan_check_de_acce_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_vlan_check_de_acce_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_udp_csum_zero_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_udp_csum_zero_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_vlan_check_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_vlan_check_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_src_if_check_de_acce_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_src_if_check_de_acce_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_map_non_tcp_udp_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_map_non_tcp_udp_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_pppoe_multicast_de_acce_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_pppoe_multicast_de_acce_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_src_if_check_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_src_if_check_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_hash_mode_1_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_hash_mode_1_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_map_udp_csum_zero_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_map_udp_csum_zero_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_map_dst_check_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_map_dst_check_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_udp_csum_zero_de_acce_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_udp_csum_zero_de_acce_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_map_src_check_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_map_src_check_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_ipv4_df_set_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_ipv4_df_set_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_ctrl_tl_de_acce_cmd_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tl_ctrl_tl_de_acce_cmd_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_l3_if_tbl_ipv6_decap_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_l3_if_tbl_ipv6_decap_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_cmd_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_l3_if_tbl_ttl_exceed_cmd_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_l3_if_tbl_min_ipv6_mtu_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_l3_if_tbl_min_ipv6_mtu_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_l3_if_tbl_dmac_check_dis_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_l3_if_tbl_dmac_check_dis_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_l3_if_tbl_lpm_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_l3_if_tbl_lpm_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_l3_if_tbl_ipv4_decap_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_l3_if_tbl_ipv4_decap_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_key_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_key_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_ip_prot_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_ip_prot_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_udf1_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_udf1_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_udf0_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_udf0_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_sport_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_sport_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_vni_resv_mask_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_vni_resv_mask_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_udf1_mask_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_udf1_mask_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_sip_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_sip_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_udf0_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_udf0_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_dport_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_dport_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_vni_resv_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_vni_resv_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_udf0_mask_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_udf0_mask_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_udf1_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_udf1_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_key_gen_dip_inc_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_key_gen_dip_inc_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_protocol_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_protocol_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_key_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_key_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_exp_profile_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_exp_profile_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_cvlan_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_cvlan_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_tl_l3_if_check_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_tl_l3_if_check_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_cdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_cdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_svlan_check_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_svlan_check_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_dscp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_dscp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_udf0_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_udf0_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_l4_dport_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_l4_dport_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_l4_sport_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_l4_sport_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_tl_l3_if_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_tl_l3_if_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_decap_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_decap_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_svlan_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_svlan_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_udf0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_udf0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_de_acce_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_de_acce_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_vni_resv_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_vni_resv_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_sdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_sdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_ipv6_dst_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value);

sw_error_t
appe_tl_tbl_ipv6_dst_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value);

sw_error_t
appe_tl_tbl_ecn_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_ecn_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_spcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_spcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_vni_resv_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_vni_resv_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_udp_csum_zero_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_udp_csum_zero_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_src_info_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_src_info_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_src_info_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_src_info_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_udf1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_udf1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_cpcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_cpcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_ipv4_src_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_ipv4_src_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_ipv4_dst_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_ipv4_dst_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_udf1_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_udf1_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_fwd_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_fwd_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_service_code_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_service_code_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_svlan_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_svlan_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_cvlan_check_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_cvlan_check_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_service_code_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_service_code_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_src_info_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_src_info_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_entry_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_entry_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_cvlan_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_cvlan_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_tbl_ipv6_src_addr_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value);

sw_error_t
appe_tl_tbl_ipv6_src_addr_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value);

sw_error_t
appe_tl_tbl_ttl_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_tbl_ttl_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_ipv4_hdr_ctrl_get(
		a_uint32_t dev_id,
		union eg_ipv4_hdr_ctrl_u *value);

sw_error_t
appe_eg_ipv4_hdr_ctrl_set(
		a_uint32_t dev_id,
		union eg_ipv4_hdr_ctrl_u *value);

sw_error_t
appe_eg_udp_entropy_ctrl_get(
		a_uint32_t dev_id,
		union eg_udp_entropy_ctrl_u *value);

sw_error_t
appe_eg_udp_entropy_ctrl_set(
		a_uint32_t dev_id,
		union eg_udp_entropy_ctrl_u *value);

sw_error_t
appe_ecn_profile_get(
		a_uint32_t dev_id,
		union ecn_profile_u *value);

sw_error_t
appe_ecn_profile_set(
		a_uint32_t dev_id,
		union ecn_profile_u *value);

sw_error_t
appe_eg_proto_mapping0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping0_u *value);

sw_error_t
appe_eg_proto_mapping0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping0_u *value);

sw_error_t
appe_eg_proto_mapping1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping1_u *value);

sw_error_t
appe_eg_proto_mapping1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_proto_mapping1_u *value);

sw_error_t
appe_dbg_addr_get(
		a_uint32_t dev_id,
		union dbg_addr_u *value);

sw_error_t
appe_dbg_addr_set(
		a_uint32_t dev_id,
		union dbg_addr_u *value);

sw_error_t
appe_dbg_data_get(
		a_uint32_t dev_id,
		union dbg_data_u *value);

sw_error_t
appe_tx_buff_thrsh_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tx_buff_thrsh_u *value);

sw_error_t
appe_tx_buff_thrsh_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tx_buff_thrsh_u *value);

sw_error_t
appe_eg_header_data_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_header_data_u *value);

sw_error_t
appe_eg_header_data_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_header_data_u *value);

sw_error_t
appe_eg_xlat_tun_ctrl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_xlat_tun_ctrl_u *value);

sw_error_t
appe_eg_xlat_tun_ctrl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_xlat_tun_ctrl_u *value);

sw_error_t
appe_eg_edit_rule_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_edit_rule_u *value);

sw_error_t
appe_eg_edit_rule_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union eg_edit_rule_u *value);

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_df_set_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_df_set_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_id_seed_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_eg_ipv4_hdr_ctrl_ipv4_id_seed_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_eg_udp_entropy_ctrl_port_mask_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_eg_udp_entropy_ctrl_port_mask_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_eg_udp_entropy_ctrl_port_base_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_eg_udp_entropy_ctrl_port_base_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_ecn_profile_profile2_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_ecn_profile_profile2_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_ecn_profile_profile1_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_ecn_profile_profile1_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_ecn_profile_profile0_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_ecn_profile_profile0_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_eg_proto_mapping0_protocol0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_proto_mapping0_protocol0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_proto_mapping1_protocol1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_proto_mapping1_protocol1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_dbg_addr_dbg_addr_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_dbg_addr_dbg_addr_set(
		a_uint32_t dev_id,
		unsigned int value);

sw_error_t
appe_dbg_data_dbg_data_get(
		a_uint32_t dev_id,
		unsigned int *value);

sw_error_t
appe_tx_buff_thrsh_xon_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tx_buff_thrsh_xon_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tx_buff_thrsh_xoff_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tx_buff_thrsh_xoff_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_header_data_data_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t *value);

sw_error_t
appe_eg_header_data_data_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint64_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ip_proto_update_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ip_proto_update_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_cpcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_cpcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_data_length_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_data_length_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_resv_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_resv_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_cdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_cdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_dscp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_dscp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ip_ver_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ip_ver_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_id_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_id_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_l3_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_l3_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_l4_checksum_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_l4_checksum_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ipv6_fl_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ipv6_fl_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_sport_entropy_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_sport_entropy_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ctag_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ctag_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_pppoe_en_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_pppoe_en_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_vni_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_vni_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_l4_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_l4_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_target_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_target_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_sdei_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_sdei_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ecn_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ecn_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_spcp_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_spcp_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_vlan_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_vlan_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_id_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_edit_rule_id_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_payload_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_payload_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_l4_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_l4_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_tunnel_offset_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_tunnel_offset_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_valid_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_output_vp_valid_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ttl_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ttl_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_type_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_type_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_stag_fmt_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_stag_fmt_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_df_mode_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_xlat_tun_ctrl_ipv4_df_mode_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_src1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_src1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_valid3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_valid3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_pos2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_pos2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_valid2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_valid2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_src2_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_src2_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_pos2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_pos2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_start3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_start3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_pos3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_pos3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_width2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_width2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_valid2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_valid2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_width2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_width2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_start3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_start3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_start2_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_start2_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_src3_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_src3_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_pos3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_pos3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_width3_0_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_width3_0_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_valid3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_valid3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_width3_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_width3_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_eg_edit_rule_start2_1_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_eg_edit_rule_start2_1_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_cnt_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_cnt_tbl_u *value);

sw_error_t
appe_tl_cnt_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_cnt_tbl_u *value);

sw_error_t
appe_tl_port_vp_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_port_vp_tbl_u *value);

sw_error_t
appe_tl_port_vp_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_port_vp_tbl_u *value);

sw_error_t
appe_tl_port_vp_tbl_pre_ipo_profile_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t *value);

sw_error_t
appe_tl_port_vp_tbl_pre_ipo_profile_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		a_uint32_t value);

sw_error_t
appe_tl_vlan_tbl_get(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_vlan_tbl_u *value);

sw_error_t
appe_tl_vlan_tbl_set(
		a_uint32_t dev_id,
		a_uint32_t index,
		union tl_vlan_tbl_u *value);

sw_error_t
appe_ecn_map_mode0_0_get(
		a_uint32_t dev_id,
		union ecn_map_mode0_0_u *value);

sw_error_t
appe_ecn_map_mode0_0_set(
		a_uint32_t dev_id,
		union ecn_map_mode0_0_u *value);

sw_error_t
appe_ecn_map_mode0_1_get(
		a_uint32_t dev_id,
		union ecn_map_mode0_1_u *value);

sw_error_t
appe_ecn_map_mode0_1_set(
		a_uint32_t dev_id,
		union ecn_map_mode0_1_u *value);

sw_error_t
appe_ecn_map_mode1_0_get(
		a_uint32_t dev_id,
		union ecn_map_mode1_0_u *value);

sw_error_t
appe_ecn_map_mode1_0_set(
		a_uint32_t dev_id,
		union ecn_map_mode1_0_u *value);

sw_error_t
appe_ecn_map_mode1_1_get(
		a_uint32_t dev_id,
		union ecn_map_mode1_1_u *value);

sw_error_t
appe_ecn_map_mode1_1_set(
		a_uint32_t dev_id,
		union ecn_map_mode1_1_u *value);

sw_error_t
appe_ecn_map_mode2_0_get(
		a_uint32_t dev_id,
		union ecn_map_mode2_0_u *value);

sw_error_t
appe_ecn_map_mode2_0_set(
		a_uint32_t dev_id,
		union ecn_map_mode2_0_u *value);

sw_error_t
appe_ecn_map_mode2_1_get(
		a_uint32_t dev_id,
		union ecn_map_mode2_1_u *value);

sw_error_t
appe_ecn_map_mode2_1_set(
		a_uint32_t dev_id,
		union ecn_map_mode2_1_u *value);
#endif
