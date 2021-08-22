/*
 * Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <assert.h>
#include <platform.h>
#include <platform_def.h>
#include <qtiseclib_interface.h>

extern u_register_t __stack_chk_guard;

u_register_t plat_get_stack_protector_canary(void)
{
	u_register_t random = 0x0;

	/* get random data , the below API doesn't return random = 0 in success
	 * case */
	qtiseclib_prng_get_data((uint8_t*)&random, sizeof(random));
	assert(0x0 != random);

	return random;
}

int qti_test_stack_protection(void)
{
  int ret = 0;
  char a[16];
  char *p;
  int i;
  p = (char *)&ret;
  strlcpy(a, (p+4), 16);
    for (i = 0; i < 50; i ++) {
      if (!memcmp(p + i, (char *)&__stack_chk_guard, 8)) {
        ret = 0;
        return ret;
      }
    }
    ret = -1;
    return ret;
}
