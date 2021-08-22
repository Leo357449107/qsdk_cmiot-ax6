/*
 **************************************************************************
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#include <linux/version.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/skbuff.h>

#include <qca_mscs_if.h>
#include <qca_mesh_latency_if.h>
#include <nss_api_if.h>
#include <ecm_classifier_mscs_public.h>
#include <ecm_classifier_emesh_public.h>

#if defined(CONFIG_DYNAMIC_DEBUG)

/*
 * Compile messakes for dynamic enable/disable
 */
#define nss_mscs_warning(s, ...) pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#define nss_mscs_info(s, ...) pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#define nss_mscs_trace(s, ...) pr_debug("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#else

/*
 * Statically compile messages at different levels
 */
#if (NSS_MSCS_DEBUG_LEVEL < 2)
#define nss_mscs_warning(s, ...)
#else
#define nss_mscs_warning(s, ...) pr_warn("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_MSCS_DEBUG_LEVEL < 3)
#define nss_mscs_info(s, ...)
#else
#define nss_mscs_info(s, ...)   pr_notice("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#endif

#if (NSS_MSCS_DEBUG_LEVEL < 4)
#define nss_mscs_trace(s, ...)
#else
#define nss_mscs_trace(s, ...)  pr_info("%s[%d]:" s, __func__, __LINE__, ##__VA_ARGS__)
#endif
#endif

/*
 * nss_mscs_ecm
 * 	Register MSCS client callback with ECM MSCS classifier to support MSCS wifi peer lookup.
 */
static struct ecm_classifier_mscs_callbacks nss_mscs_ecm = {
	    .get_peer_priority = qca_mscs_peer_lookup_n_get_priority,
};

/*
 * nss_emesh_ecm
 * 	Register EMESH client callback with ECM EMSH classifier to update peer mesh latency parameters.
 */
static struct ecm_classifier_emesh_callbacks nss_emesh_ecm = {
	    .update_peer_mesh_latency_params = qca_mesh_latency_update_peer_parameter,
};

/*
 * nss_mscs_init_module()
 *	MSCS clinet module init function
 */
int __init nss_mscs_init_module(void)
{
#ifdef CONFIG_OF
	/*
	 * If the node is not compatible, don't do anything.
	 */
	if (!of_find_node_by_name(NULL, "nss-common")) {
		return 0;
	}
#endif

	/*
	 * MSCS is enabled only on supported platform
	 */
	if (!nss_cmn_get_nss_enabled()) {
		nss_mscs_warning("MSCS client is not compatible with this Platform\n");
		return -1;
	}

	if (ecm_classifier_mscs_callback_register(&nss_mscs_ecm)) {
		nss_mscs_warning("ecm mscs classifier callback registration failed.\n");
		return -1;
	}

	if (ecm_classifier_emesh_latency_config_callback_register(&nss_emesh_ecm)) {
		ecm_classifier_mscs_callback_unregister();
		nss_mscs_warning("ecm mesh classifier callback registration failed.\n");
		return -1;
	}

	nss_mscs_info("NSS MSCS Client loaded: %s\n", NSS_CLIENT_BUILD_ID);
	return 0;

}

/*
 * nss_mscs_exit_module()
 *	MSCS module exit function
 */
void __exit nss_mscs_exit_module(void)
{
#ifdef CONFIG_OF

	/*
	 * If the node is not compatible, don't do anything.
	 */
	if (!of_find_node_by_name(NULL, "nss-common")) {
		return;
	}
#endif

	ecm_classifier_mscs_callback_unregister();
	ecm_classifier_emesh_latency_config_callback_unregister();
	nss_mscs_info("MSCS Client unloaded\n");

}

module_init(nss_mscs_init_module);
module_exit(nss_mscs_exit_module);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("NSS MSCS client module");
