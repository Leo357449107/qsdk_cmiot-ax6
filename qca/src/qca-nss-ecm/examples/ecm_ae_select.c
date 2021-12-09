/*
 **************************************************************************
 * Copyright (c) 2021, The Linux Foundation.  All rights reserved.
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
 **************************************************************************
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/ctype.h>
#include <linux/etherdevice.h>

#include "ecm_ae_classifier_public.h"

/*
 * This is the module which selects the underlying acceleration engine
 * based ont eh 5-tuple and type of the flow. The flow can be multicast, routed,
 * bridged, ported/non-ported.
 */

/*
 * ecm_ae_select()
 *	Selects the acceleration engine based on the given flow information.
 */
ecm_ae_classifier_result_t ecm_ae_select(struct ecm_ae_classifier_info *info)
{
	pr_debug("%px: Acceleration engine selection\n", info);

	/*
	 * Multicast flows can be accelerated by NSS only.
	 */
	if (info->flag & ECM_AE_CLASSIFIER_FLOW_MULTICAST) {
		return ECM_AE_CLASSIFIER_RESULT_NSS;
	}

	/*
	 * Non-ported flows can be accelerated by NSS only.
	 */
	if (info->protocol != IPPROTO_TCP && info->protocol != IPPROTO_UDP) {
		return ECM_AE_CLASSIFIER_RESULT_NSS;
	}

	/*
	 * Let's accelerate all other routed flows by SFE.
	 */
	if (info->flag & ECM_AE_CLASSIFIER_FLOW_ROUTED) {
		return ECM_AE_CLASSIFIER_RESULT_SFE;
	}

	/*
	 * Let's accelerate all bridge flows by NSS.
	 */
	return ECM_AE_CLASSIFIER_RESULT_NSS;
}

static struct ecm_ae_classifier_ops ae_ops = {
	.ae_get = ecm_ae_select
};

/*
 * ecm_ae_select_init()
 */
static int __init ecm_ae_select_init(void)
{
	pr_info("ECM AE Select INIT\n");

	/*
	 * Register the callbacks.
	 */
	ecm_ae_classifier_ops_register(&ae_ops);

	return 0;
}

/*
 * ecm_ae_select_exit()
 */
static void __exit ecm_ae_select_exit(void)
{
	pr_info("ECM AE Select EXIT\n");

	/*
	 * Unregister the callbacks.
	 */
	ecm_ae_classifier_ops_unregister();
}

module_init(ecm_ae_select_init)
module_exit(ecm_ae_select_exit)

MODULE_DESCRIPTION("ECM Acceleration Engine Selection Module");
#ifdef MODULE_LICENSE
MODULE_LICENSE("Dual BSD/GPL");
#endif
