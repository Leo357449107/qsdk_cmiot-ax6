/*
 **************************************************************************
 * Copyright (c) 2015, 2021, The Linux Foundation.  All rights reserved.
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

/*
 * Structure used to synchronise a classifier instance with the state as presented by the accel engine
 */
enum ecm_classifier_pcc_results {
	ECM_CLASSIFIER_PCC_RESULT_NOT_YET,		/* Accel is neither permitted nor denied just yet - try again later */
	ECM_CLASSIFIER_PCC_RESULT_DENIED,		/* Accel is denied for this connection */
	ECM_CLASSIFIER_PCC_RESULT_PERMITTED,		/* Accel is permitted for this connection */
};
typedef enum ecm_classifier_pcc_results ecm_classifier_pcc_result_t;

/*
 * Feature flags.
 *
 * The registrant(customer) can request multiple features to be enabled on the
 * connection, using a bitmap of features with the 'ecm_classifier_pcc_info'
 * object. Note that currently only 'mirror' feature is supported to begin with.
 */
enum ecm_classifier_pcc_feature_flags {
	ECM_CLASSIFIER_PCC_FEATURE_NONE,
	ECM_CLASSIFIER_PCC_FEATURE_MIRROR = 0x1,	/* Set by customer if mirror is needed on the connection */
};

/*
 * Mirror netdevices to be used for mirroring an offloaded flow
 */
struct ecm_classifier_pcc_mirror_info {
	struct net_device *tuple_mirror_dev;	/* Tuple direction mirror netdevice. */
	struct net_device *tuple_ret_mirror_dev;	/* Tuple return direction mirror netdevice. */
};

/*
 * Feature information related to the connection, returned by registrant's callback
 */
struct ecm_classifier_pcc_info {
	uint32_t feature_flags;			/* Bitmap of requested features (ecm_classifier_pcc_feature_ids) */
	struct ecm_classifier_pcc_mirror_info mirror;
};

struct ecm_classifier_pcc_registrant;

typedef void (*ecm_classifier_pcc_ref_method_t)(struct ecm_classifier_pcc_registrant *r);
typedef void (*ecm_classifier_pcc_deref_method_t)(struct ecm_classifier_pcc_registrant *r);
typedef ecm_classifier_pcc_result_t (*ecm_classifier_pcc_okay_to_accel_v4_method_t)(struct ecm_classifier_pcc_registrant *r, uint8_t *src_mac, __be32 src_ip, int src_port, uint8_t *dest_mac, __be32 dest_ip, int dest_port, int protocol);
typedef ecm_classifier_pcc_result_t (*ecm_classifier_pcc_okay_to_accel_v6_method_t)(struct ecm_classifier_pcc_registrant *r, uint8_t *src_mac, struct in6_addr *src_ip, int src_port, uint8_t *dest_mac, struct in6_addr *dest_ip, int dest_port, int protocol);

/*
 * The below two APIs can be used to query the customer's PCC registrant when ECM
 * is processing the packet. The return value is used to decide whether acceleration
 * is needed for the conntection or not. Customer can (optionally) also share additional
 * connection properties to enable for the connection using the object 'cinfo' that is passed
 * to them. Currently support is present only for one connection property named 'mirror'.
 * Note: Object pointed to by 'cinfo' is allocated by ECM
 */
typedef ecm_classifier_pcc_result_t
 (*ecm_classifier_pcc_get_accel_info_v4_method_t)(struct ecm_classifier_pcc_registrant *r,
		  uint8_t *src_mac, __be32 src_ip, int src_port,
		  uint8_t *dest_mac, __be32 dest_ip, int dest_port,
		  int protocol, struct ecm_classifier_pcc_info *cinfo);

typedef ecm_classifier_pcc_result_t
 (*ecm_classifier_pcc_get_accel_info_v6_method_t)(struct ecm_classifier_pcc_registrant *r,
		  uint8_t *src_mac, struct in6_addr *src_ip, int src_port,
		  uint8_t *dest_mac, struct in6_addr *dest_ip, int dest_port,
		  int protocol, struct ecm_classifier_pcc_info *cinfo);

/*
 * struct ecm_classifier_pcc_registrant
 *	Used by customer parental control code to register their existance with the ECM PCC classifier
 */
struct ecm_classifier_pcc_registrant {
	uint16_t version;					/* Customer Parental Controls (CPC) supplies 1 for this field. */

	struct ecm_classifier_pcc_registrant *pcc_next;		/* ECM PCC use */
	struct ecm_classifier_pcc_registrant *pcc_prev;		/* ECM PCC use */
	uint32_t pcc_flags;					/* ECM PCC use */

	atomic_t ref_count;					/* CPC sets this to 1 initially when registering with ECM.
								 * PCC takes its own private 'ref' for the registrant so after registering the CPC should 'deref' the initial '1'.
								 * CPC MUST NOT deallocate this structure until the ref_count is dropped to zero by deref() calls
								 */
	struct module *this_module;				/* Pointer to the registrants module */

	ecm_classifier_pcc_ref_method_t ref;			/* When called the ref_count is incremented by 1 */
	ecm_classifier_pcc_deref_method_t deref;		/* When called the ref_count is decremented by 1.
								 * When ref_count becomes 0 no further calls will be made upon this registrant
								 */
	ecm_classifier_pcc_okay_to_accel_v4_method_t okay_to_accel_v4;
								/* ECM PCC asks the CPC if the given connection is okay to accelerate */
	ecm_classifier_pcc_okay_to_accel_v6_method_t okay_to_accel_v6;
								/* ECM PCC asks the CPC if the given connection is okay to accelerate */
	ecm_classifier_pcc_get_accel_info_v4_method_t get_accel_info_v4;
								/* ECM PCC asks the CPC for the acceleration information */
	ecm_classifier_pcc_get_accel_info_v6_method_t get_accel_info_v6;
								/* ECM PCC asks the CPC for the acceleration information */
};

extern int ecm_classifier_pcc_register(struct ecm_classifier_pcc_registrant *r);
extern void ecm_classifier_pcc_unregister_begin(struct ecm_classifier_pcc_registrant *r);

extern void ecm_classifier_pcc_permit_accel_v4(uint8_t *src_mac, __be32 src_ip, int src_port, uint8_t *dest_mac, __be32 dest_ip, int dest_port, int protocol);
extern void ecm_classifier_pcc_deny_accel_v4(uint8_t *src_mac, __be32 src_ip, int src_port, uint8_t *dest_mac, __be32 dest_ip, int dest_port, int protocol);

/*
 * This API can be used to decelerate an existing IPv4 ECM connection.
 */
extern bool ecm_classifier_pcc_decel_v4(uint8_t *src_mac, __be32 src_ip, int src_port,
		 uint8_t *dest_mac, __be32 dest_ip, int dest_port, int protocol);

extern void ecm_classifier_pcc_permit_accel_v6(uint8_t *src_mac, struct in6_addr *src_ip, int src_port, uint8_t *dest_mac, struct in6_addr *dest_ip, int dest_port, int protocol);
extern void ecm_classifier_pcc_deny_accel_v6(uint8_t *src_mac, struct in6_addr *src_ip, int src_port, uint8_t *dest_mac, struct in6_addr *dest_ip, int dest_port, int protocol);

/*
 * This API can be used to decelerate an existing IPv6 ECM connection.
 */
extern bool ecm_classifier_pcc_decel_v6(uint8_t *src_mac, struct in6_addr *src_ip,
		 int src_port, uint8_t *dest_mac, struct in6_addr *dest_ip,
		 int dest_port, int protocol);
