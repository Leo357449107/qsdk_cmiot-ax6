/*
 * Copyright (C) 2013, Intel Corporation
 * Copyright (C) 2014, Bin Meng <bmeng.cn@gmail.com>
 *
 * SPDX-License-Identifier:	Intel
 */

#ifndef _FSP_HEADER_H_
#define _FSP_HEADER_H_

#define FSP_HEADER_OFF	0x94	/* Fixed FSP header offset in the FSP image */

struct __packed fsp_header {
	u32	sign;			/* 'FSPH' */
	u32	hdr_len;		/* header length */
	u8	reserved1[3];
	u8	hdr_rev;		/* header rev */
	u32	img_rev;		/* image rev */
	char	img_id[8];		/* signature string */
	u32	img_size;		/* image size */
	u32	img_base;		/* image base */
	u32	img_attr;		/* image attribute */
	u32	cfg_region_off;		/* configuration region offset */
	u32	cfg_region_size;	/* configuration region size */
	u32	api_num;		/* number of API entries */
	u32	fsp_tempram_init;	/* tempram_init offset */
	u32	fsp_init;		/* fsp_init offset */
	u32	fsp_notify;		/* fsp_notify offset */
	u32	reserved2;
};

#endif
