/*
 * Copyright (c) 2016-2017, 2020, The Linux Foundation. All rights reserved.
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
 * @defgroup
 * @{
 */
#ifndef _HPPE_SEC_REG_H_
#define _HPPE_SEC_REG_H_


/*[register] L3_EXCEPTION_CMD*/
#define L3_EXCEPTION_CMD
#if defined(APPE)
#define L3_EXCEPTION_CMD_ADDRESS 0x81c
#else
#define L3_EXCEPTION_CMD_ADDRESS 0x544
#endif
#define L3_EXCEPTION_CMD_NUM     72
#define L3_EXCEPTION_CMD_INC     0x4
#define L3_EXCEPTION_CMD_TYPE    REG_TYPE_RW
#define L3_EXCEPTION_CMD_DEFAULT 0x0
	/*[field] L3_EXCEP_CMD*/
	#define L3_EXCEPTION_CMD_L3_EXCEP_CMD
	#define L3_EXCEPTION_CMD_L3_EXCEP_CMD_OFFSET  0
	#define L3_EXCEPTION_CMD_L3_EXCEP_CMD_LEN     2
	#define L3_EXCEPTION_CMD_L3_EXCEP_CMD_DEFAULT 0x0
	/*[field] DE_ACCE*/
	#define L3_EXCEPTION_CMD_DE_ACCE
	#define L3_EXCEPTION_CMD_DE_ACCE_OFFSET  2
	#define L3_EXCEPTION_CMD_DE_ACCE_LEN     1
	#define L3_EXCEPTION_CMD_DE_ACCE_DEFAULT 0x0

struct l3_exception_cmd {
	a_uint32_t  l3_excep_cmd:2;
	a_uint32_t  de_acce:1;
	a_uint32_t  _reserved0:29;
};

union l3_exception_cmd_u {
	a_uint32_t val;
	struct l3_exception_cmd bf;
};

/*[register] L3_EXP_L3_ONLY_CTRL*/
#define L3_EXP_L3_ONLY_CTRL
#if defined(APPE)
#define L3_EXP_L3_ONLY_CTRL_ADDRESS 0x94c
#else
#define L3_EXP_L3_ONLY_CTRL_ADDRESS 0x664
#endif
#define L3_EXP_L3_ONLY_CTRL_NUM     72
#define L3_EXP_L3_ONLY_CTRL_INC     0x4
#define L3_EXP_L3_ONLY_CTRL_TYPE    REG_TYPE_RW
#define L3_EXP_L3_ONLY_CTRL_DEFAULT 0x0
	/*[field] EXCEP_EN*/
	#define L3_EXP_L3_ONLY_CTRL_EXCEP_EN
	#define L3_EXP_L3_ONLY_CTRL_EXCEP_EN_OFFSET  0
	#define L3_EXP_L3_ONLY_CTRL_EXCEP_EN_LEN     1
	#define L3_EXP_L3_ONLY_CTRL_EXCEP_EN_DEFAULT 0x0

struct l3_exp_l3_only_ctrl {
	a_uint32_t  excep_en:1;
	a_uint32_t  _reserved0:31;
};

union l3_exp_l3_only_ctrl_u {
	a_uint32_t val;
	struct l3_exp_l3_only_ctrl bf;
};

/*[register] L3_EXP_L2_ONLY_CTRL*/
#define L3_EXP_L2_ONLY_CTRL
#if defined(APPE)
#define L3_EXP_L2_ONLY_CTRL_ADDRESS 0xa7c
#else
#define L3_EXP_L2_ONLY_CTRL_ADDRESS 0x784
#endif
#define L3_EXP_L2_ONLY_CTRL_NUM     72
#define L3_EXP_L2_ONLY_CTRL_INC     0x4
#define L3_EXP_L2_ONLY_CTRL_TYPE    REG_TYPE_RW
#define L3_EXP_L2_ONLY_CTRL_DEFAULT 0x0
	/*[field] EXCEP_EN*/
	#define L3_EXP_L2_ONLY_CTRL_EXCEP_EN
	#define L3_EXP_L2_ONLY_CTRL_EXCEP_EN_OFFSET  0
	#define L3_EXP_L2_ONLY_CTRL_EXCEP_EN_LEN     1
	#define L3_EXP_L2_ONLY_CTRL_EXCEP_EN_DEFAULT 0x0

struct l3_exp_l2_only_ctrl {
	a_uint32_t  excep_en:1;
	a_uint32_t  _reserved0:31;
};

union l3_exp_l2_only_ctrl_u {
	a_uint32_t val;
	struct l3_exp_l2_only_ctrl bf;
};

/*[register] L3_EXP_L2_FLOW_CTRL*/
#define L3_EXP_L2_FLOW_CTRL
#if defined(APPE)
#define L3_EXP_L2_FLOW_CTRL_ADDRESS 0xbac
#else
#define L3_EXP_L2_FLOW_CTRL_ADDRESS 0x8a4
#endif
#define L3_EXP_L2_FLOW_CTRL_NUM     72
#define L3_EXP_L2_FLOW_CTRL_INC     0x4
#define L3_EXP_L2_FLOW_CTRL_TYPE    REG_TYPE_RW
#define L3_EXP_L2_FLOW_CTRL_DEFAULT 0x0
	/*[field] EXCEP_EN*/
	#define L3_EXP_L2_FLOW_CTRL_EXCEP_EN
	#define L3_EXP_L2_FLOW_CTRL_EXCEP_EN_OFFSET  0
	#define L3_EXP_L2_FLOW_CTRL_EXCEP_EN_LEN     1
	#define L3_EXP_L2_FLOW_CTRL_EXCEP_EN_DEFAULT 0x0

struct l3_exp_l2_flow_ctrl {
	a_uint32_t  excep_en:1;
	a_uint32_t  _reserved0:31;
};

union l3_exp_l2_flow_ctrl_u {
	a_uint32_t val;
	struct l3_exp_l2_flow_ctrl bf;
};

/*[register] L3_EXP_L3_FLOW_CTRL*/
#define L3_EXP_L3_FLOW_CTRL
#if defined(APPE)
#define L3_EXP_L3_FLOW_CTRL_ADDRESS 0xcdc
#else
#define L3_EXP_L3_FLOW_CTRL_ADDRESS 0x9c4
#endif
#define L3_EXP_L3_FLOW_CTRL_NUM     72
#define L3_EXP_L3_FLOW_CTRL_INC     0x4
#define L3_EXP_L3_FLOW_CTRL_TYPE    REG_TYPE_RW
#define L3_EXP_L3_FLOW_CTRL_DEFAULT 0x0
	/*[field] EXCEP_EN*/
	#define L3_EXP_L3_FLOW_CTRL_EXCEP_EN
	#define L3_EXP_L3_FLOW_CTRL_EXCEP_EN_OFFSET  0
	#define L3_EXP_L3_FLOW_CTRL_EXCEP_EN_LEN     1
	#define L3_EXP_L3_FLOW_CTRL_EXCEP_EN_DEFAULT 0x0

struct l3_exp_l3_flow_ctrl {
	a_uint32_t  excep_en:1;
	a_uint32_t  _reserved0:31;
};

union l3_exp_l3_flow_ctrl_u {
	a_uint32_t val;
	struct l3_exp_l3_flow_ctrl bf;
};

/*[register] L3_EXP_MULTICAST_CTRL*/
#define L3_EXP_MULTICAST_CTRL
#if defined(APPE)
#define L3_EXP_MULTICAST_CTRL_ADDRESS 0xe0c
#else
#define L3_EXP_MULTICAST_CTRL_ADDRESS 0xae4
#endif
#define L3_EXP_MULTICAST_CTRL_NUM     72
#define L3_EXP_MULTICAST_CTRL_INC     0x4
#define L3_EXP_MULTICAST_CTRL_TYPE    REG_TYPE_RW
#define L3_EXP_MULTICAST_CTRL_DEFAULT 0x0
	/*[field] EXCEP_EN*/
	#define L3_EXP_MULTICAST_CTRL_EXCEP_EN
	#define L3_EXP_MULTICAST_CTRL_EXCEP_EN_OFFSET  0
	#define L3_EXP_MULTICAST_CTRL_EXCEP_EN_LEN     1
	#define L3_EXP_MULTICAST_CTRL_EXCEP_EN_DEFAULT 0x0

struct l3_exp_multicast_ctrl {
	a_uint32_t  excep_en:1;
	a_uint32_t  _reserved0:31;
};

union l3_exp_multicast_ctrl_u {
	a_uint32_t val;
	struct l3_exp_multicast_ctrl bf;
};

/*[register] L3_EXCEPTION_PARSING_CTRL_REG*/
#define L3_EXCEPTION_PARSING_CTRL_REG
#define L3_EXCEPTION_PARSING_CTRL_REG_ADDRESS 0x24
#define L3_EXCEPTION_PARSING_CTRL_REG_NUM     1
#define L3_EXCEPTION_PARSING_CTRL_REG_INC     0x4
#define L3_EXCEPTION_PARSING_CTRL_REG_TYPE    REG_TYPE_RW
#define L3_EXCEPTION_PARSING_CTRL_REG_DEFAULT 0x0
	/*[field] SMALL_TTL*/
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_TTL
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_TTL_OFFSET  0
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_TTL_LEN     8
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_TTL_DEFAULT 0x0
	/*[field] SMALL_HOP_LIMIT*/
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_HOP_LIMIT
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_HOP_LIMIT_OFFSET  8
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_HOP_LIMIT_LEN     8
	#define L3_EXCEPTION_PARSING_CTRL_REG_SMALL_HOP_LIMIT_DEFAULT 0x0

struct l3_exception_parsing_ctrl_reg {
	a_uint32_t  small_ttl:8;
	a_uint32_t  small_hop_limit:8;
	a_uint32_t  _reserved0:16;
};

union l3_exception_parsing_ctrl_reg_u {
	a_uint32_t val;
	struct l3_exception_parsing_ctrl_reg bf;
};

/*[register] L4_EXCEPTION_PARSING_CTRL_0_REG*/
#define L4_EXCEPTION_PARSING_CTRL_0_REG
#define L4_EXCEPTION_PARSING_CTRL_0_REG_ADDRESS 0x28
#define L4_EXCEPTION_PARSING_CTRL_0_REG_NUM     1
#define L4_EXCEPTION_PARSING_CTRL_0_REG_INC     0x4
#define L4_EXCEPTION_PARSING_CTRL_0_REG_TYPE    REG_TYPE_RW
#define L4_EXCEPTION_PARSING_CTRL_0_REG_DEFAULT 0x0
	/*[field] TCP_FLAGS0*/
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0_OFFSET  0
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0_DEFAULT 0x0
	/*[field] TCP_FLAGS0_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0_MASK
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0_MASK_OFFSET  8
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS0_MASK_DEFAULT 0x0
	/*[field] TCP_FLAGS1*/
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1_OFFSET  16
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1_DEFAULT 0x0
	/*[field] TCP_FLAGS1_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1_MASK
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1_MASK_OFFSET  24
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_0_REG_TCP_FLAGS1_MASK_DEFAULT 0x0

struct l4_exception_parsing_ctrl_0_reg {
	a_uint32_t  tcp_flags0:6;
	a_uint32_t  _reserved0:2;
	a_uint32_t  tcp_flags0_mask:6;
	a_uint32_t  _reserved1:2;
	a_uint32_t  tcp_flags1:6;
	a_uint32_t  _reserved2:2;
	a_uint32_t  tcp_flags1_mask:6;
	a_uint32_t  _reserved3:2;
};

union l4_exception_parsing_ctrl_0_reg_u {
	a_uint32_t val;
	struct l4_exception_parsing_ctrl_0_reg bf;
};

/*[register] L4_EXCEPTION_PARSING_CTRL_1_REG*/
#define L4_EXCEPTION_PARSING_CTRL_1_REG
#define L4_EXCEPTION_PARSING_CTRL_1_REG_ADDRESS 0x2c
#define L4_EXCEPTION_PARSING_CTRL_1_REG_NUM     1
#define L4_EXCEPTION_PARSING_CTRL_1_REG_INC     0x4
#define L4_EXCEPTION_PARSING_CTRL_1_REG_TYPE    REG_TYPE_RW
#define L4_EXCEPTION_PARSING_CTRL_1_REG_DEFAULT 0x0
	/*[field] TCP_FLAGS2*/
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2_OFFSET  0
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2_DEFAULT 0x0
	/*[field] TCP_FLAGS2_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2_MASK
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2_MASK_OFFSET  8
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS2_MASK_DEFAULT 0x0
	/*[field] TCP_FLAGS3*/
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3_OFFSET  16
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3_DEFAULT 0x0
	/*[field] TCP_FLAGS3_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3_MASK
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3_MASK_OFFSET  24
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_1_REG_TCP_FLAGS3_MASK_DEFAULT 0x0

struct l4_exception_parsing_ctrl_1_reg {
	a_uint32_t  tcp_flags2:6;
	a_uint32_t  _reserved0:2;
	a_uint32_t  tcp_flags2_mask:6;
	a_uint32_t  _reserved1:2;
	a_uint32_t  tcp_flags3:6;
	a_uint32_t  _reserved2:2;
	a_uint32_t  tcp_flags3_mask:6;
	a_uint32_t  _reserved3:2;
};

union l4_exception_parsing_ctrl_1_reg_u {
	a_uint32_t val;
	struct l4_exception_parsing_ctrl_1_reg bf;
};

/*[register] L4_EXCEPTION_PARSING_CTRL_2_REG*/
#define L4_EXCEPTION_PARSING_CTRL_2_REG
#define L4_EXCEPTION_PARSING_CTRL_2_REG_ADDRESS 0x30
#define L4_EXCEPTION_PARSING_CTRL_2_REG_NUM     1
#define L4_EXCEPTION_PARSING_CTRL_2_REG_INC     0x4
#define L4_EXCEPTION_PARSING_CTRL_2_REG_TYPE    REG_TYPE_RW
#define L4_EXCEPTION_PARSING_CTRL_2_REG_DEFAULT 0x0
	/*[field] TCP_FLAGS4*/
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4_OFFSET  0
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4_DEFAULT 0x0
	/*[field] TCP_FLAGS4_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4_MASK
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4_MASK_OFFSET  8
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS4_MASK_DEFAULT 0x0
	/*[field] TCP_FLAGS5*/
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5_OFFSET  16
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5_DEFAULT 0x0
	/*[field] TCP_FLAGS5_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5_MASK
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5_MASK_OFFSET  24
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_2_REG_TCP_FLAGS5_MASK_DEFAULT 0x0

struct l4_exception_parsing_ctrl_2_reg {
	a_uint32_t  tcp_flags4:6;
	a_uint32_t  _reserved0:2;
	a_uint32_t  tcp_flags4_mask:6;
	a_uint32_t  _reserved1:2;
	a_uint32_t  tcp_flags5:6;
	a_uint32_t  _reserved2:2;
	a_uint32_t  tcp_flags5_mask:6;
	a_uint32_t  _reserved3:2;
};

union l4_exception_parsing_ctrl_2_reg_u {
	a_uint32_t val;
	struct l4_exception_parsing_ctrl_2_reg bf;
};

/*[register] L4_EXCEPTION_PARSING_CTRL_3_REG*/
#define L4_EXCEPTION_PARSING_CTRL_3_REG
#define L4_EXCEPTION_PARSING_CTRL_3_REG_ADDRESS 0x34
#define L4_EXCEPTION_PARSING_CTRL_3_REG_NUM     1
#define L4_EXCEPTION_PARSING_CTRL_3_REG_INC     0x4
#define L4_EXCEPTION_PARSING_CTRL_3_REG_TYPE    REG_TYPE_RW
#define L4_EXCEPTION_PARSING_CTRL_3_REG_DEFAULT 0x0
	/*[field] TCP_FLAGS6*/
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6_OFFSET  0
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6_DEFAULT 0x0
	/*[field] TCP_FLAGS6_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6_MASK
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6_MASK_OFFSET  8
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS6_MASK_DEFAULT 0x0
	/*[field] TCP_FLAGS7*/
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7_OFFSET  16
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7_DEFAULT 0x0
	/*[field] TCP_FLAGS7_MASK*/
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7_MASK
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7_MASK_OFFSET  24
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7_MASK_LEN     6
	#define L4_EXCEPTION_PARSING_CTRL_3_REG_TCP_FLAGS7_MASK_DEFAULT 0x0

struct l4_exception_parsing_ctrl_3_reg {
	a_uint32_t  tcp_flags6:6;
	a_uint32_t  _reserved0:2;
	a_uint32_t  tcp_flags6_mask:6;
	a_uint32_t  _reserved1:2;
	a_uint32_t  tcp_flags7:6;
	a_uint32_t  _reserved2:2;
	a_uint32_t  tcp_flags7_mask:6;
	a_uint32_t  _reserved3:2;
};

union l4_exception_parsing_ctrl_3_reg_u {
	a_uint32_t val;
	struct l4_exception_parsing_ctrl_3_reg bf;
};

#endif
