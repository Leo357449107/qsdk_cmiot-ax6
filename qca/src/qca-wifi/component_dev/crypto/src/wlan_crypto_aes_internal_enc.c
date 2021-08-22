/*
 * Copyright (c) 2017 The Linux Foundation. All rights reserved.
 */
/*
 * AES (Rijndael) cipher - encrypt
 *
 * Modifications to public domain implementation:
 * - cleanup
 * - use C pre-processor to make it easier to change S table access
 * - added option (AES_SMALL_TABLES) for reducing code size by about 8 kB at
 *   cost of reduced throughput (quite small difference on Pentium 4,
 *   10-25% when using -O1 or -O2 optimization)
 *
 * Copyright (c) 2003-2012, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include <qdf_types.h>
#include <qdf_mem.h>
#include <qdf_util.h>
#include "wlan_crypto_aes_i.h"
#include "wlan_crypto_def_i.h"

static void rijndaelEncrypt(const uint32_t rk[], int Nr, const uint8_t pt[16],
				uint8_t ct[16]){
	uint32_t s0, s1, s2, s3, t0, t1, t2, t3;
#ifndef FULL_UNROLL
	int r;
#endif /* ?FULL_UNROLL */

	/*
	 * map byte array block to cipher state
	 * and add initial round key:
	 */
	s0 = GETU32(pt) ^ rk[0];
	s1 = GETU32(pt +  4) ^ rk[1];
	s2 = GETU32(pt +  8) ^ rk[2];
	s3 = GETU32(pt + 12) ^ rk[3];

#define ROUND(i, d, s) {\
d##0 = TE0(s##0) ^ TE1(s##1) ^ TE2(s##2) ^ TE3(s##3) ^ rk[4 * i]; \
d##1 = TE0(s##1) ^ TE1(s##2) ^ TE2(s##3) ^ TE3(s##0) ^ rk[4 * i + 1]; \
d##2 = TE0(s##2) ^ TE1(s##3) ^ TE2(s##0) ^ TE3(s##1) ^ rk[4 * i + 2]; \
d##3 = TE0(s##3) ^ TE1(s##0) ^ TE2(s##1) ^ TE3(s##2) ^ rk[4 * i + 3]; }

#ifdef FULL_UNROLL

	ROUND(1, t, s);
	ROUND(2, s, t);
	ROUND(3, t, s);
	ROUND(4, s, t);
	ROUND(5, t, s);
	ROUND(6, s, t);
	ROUND(7, t, s);
	ROUND(8, s, t);
	ROUND(9, t, s);
	if (Nr > 10) {
		ROUND(10, s, t);
		ROUND(11, t, s);
		if (Nr > 12) {
			ROUND(12, s, t);
			ROUND(13, t, s);
		}
	}

	rk += Nr << 2;

#else  /* !FULL_UNROLL */

	/* Nr - 1 full rounds: */
	r = Nr >> 1;
	for (;;) {
		ROUND(1, t, s);
		rk += 8;
		if (--r == 0)
			break;
		ROUND(0, s, t);
	}

#endif /* ?FULL_UNROLL */

#undef ROUND

	/*
	 * apply last round and
	 * map cipher state to byte array block:
	 */
	s0 = TE41(t0) ^ TE42(t1) ^ TE43(t2) ^ TE44(t3) ^ rk[0];
	PUTU32(ct, s0);
	s1 = TE41(t1) ^ TE42(t2) ^ TE43(t3) ^ TE44(t0) ^ rk[1];
	PUTU32(ct +  4, s1);
	s2 = TE41(t2) ^ TE42(t3) ^ TE43(t0) ^ TE44(t1) ^ rk[2];
	PUTU32(ct +  8, s2);
	s3 = TE41(t3) ^ TE42(t0) ^ TE43(t1) ^ TE44(t2) ^ rk[3];
	PUTU32(ct + 12, s3);
}


void *wlan_crypto_aes_encrypt_init(const uint8_t *key, size_t len)
{
	uint32_t *rk;
	int res;
	rk = qdf_mem_malloc(AES_PRIV_SIZE);
	if (rk == NULL)
		return NULL;
	res = wlan_crypto_rijndaelKeySetupEnc(rk, key, len * 8);
	if (res < 0) {
		qdf_mem_free(rk);
		return NULL;
	}
	rk[AES_PRIV_NR_POS] = res;
	return rk;
}


void wlan_crypto_aes_encrypt(void *ctx, const uint8_t *plain, uint8_t *crypt)
{
	uint32_t *rk = ctx;
	rijndaelEncrypt(ctx, rk[AES_PRIV_NR_POS], plain, crypt);
}


void wlan_crypto_aes_encrypt_deinit(void *ctx)
{
	qdf_mem_set(ctx, AES_PRIV_SIZE, 0);
	qdf_mem_free(ctx);
}
