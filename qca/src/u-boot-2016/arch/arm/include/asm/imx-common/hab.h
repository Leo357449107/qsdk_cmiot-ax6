/*
 * Copyright (C) 2012-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier:    GPL-2.0+
 *
*/

#ifndef __SECURE_MX6Q_H__
#define __SECURE_MX6Q_H__

#include <linux/types.h>

/* -------- start of HAB API updates ------------*/
/* The following are taken from HAB4 SIS */

/* Status definitions */
enum hab_status {
	HAB_STS_ANY = 0x00,
	HAB_FAILURE = 0x33,
	HAB_WARNING = 0x69,
	HAB_SUCCESS = 0xf0
};

/* Security Configuration definitions */
enum hab_config {
	HAB_CFG_RETURN = 0x33,	/* < Field Return IC */
	HAB_CFG_OPEN = 0xf0,	/* < Non-secure IC */
	HAB_CFG_CLOSED = 0xcc	/* < Secure IC */
};

/* State definitions */
enum hab_state {
	HAB_STATE_INITIAL = 0x33,	/* Initialising state (transitory) */
	HAB_STATE_CHECK = 0x55,		/* Check state (non-secure) */
	HAB_STATE_NONSECURE = 0x66,	/* Non-secure state */
	HAB_STATE_TRUSTED = 0x99,	/* Trusted state */
	HAB_STATE_SECURE = 0xaa,	/* Secure state */
	HAB_STATE_FAIL_SOFT = 0xcc, /* Soft fail state */
	HAB_STATE_FAIL_HARD = 0xff, /* Hard fail state (terminal) */
	HAB_STATE_NONE = 0xf0,		/* No security state machine */
	HAB_STATE_MAX
};

enum hab_reason {
	HAB_RSN_ANY = 0x00,			/* Match any reason */
	HAB_ENG_FAIL = 0x30,		/* Engine failure */
	HAB_INV_ADDRESS = 0x22,		/* Invalid address: access denied */
	HAB_INV_ASSERTION = 0x0c,   /* Invalid assertion */
	HAB_INV_CALL = 0x28,		/* Function called out of sequence */
	HAB_INV_CERTIFICATE = 0x21, /* Invalid certificate */
	HAB_INV_COMMAND = 0x06,     /* Invalid command: command malformed */
	HAB_INV_CSF = 0x11,			/* Invalid csf */
	HAB_INV_DCD = 0x27,			/* Invalid dcd */
	HAB_INV_INDEX = 0x0f,		/* Invalid index: access denied */
	HAB_INV_IVT = 0x05,			/* Invalid ivt */
	HAB_INV_KEY = 0x1d,			/* Invalid key */
	HAB_INV_RETURN = 0x1e,		/* Failed callback function */
	HAB_INV_SIGNATURE = 0x18,   /* Invalid signature */
	HAB_INV_SIZE = 0x17,		/* Invalid data size */
	HAB_MEM_FAIL = 0x2e,		/* Memory failure */
	HAB_OVR_COUNT = 0x2b,		/* Expired poll count */
	HAB_OVR_STORAGE = 0x2d,		/* Exhausted storage region */
	HAB_UNS_ALGORITHM = 0x12,   /* Unsupported algorithm */
	HAB_UNS_COMMAND = 0x03,		/* Unsupported command */
	HAB_UNS_ENGINE = 0x0a,		/* Unsupported engine */
	HAB_UNS_ITEM = 0x24,		/* Unsupported configuration item */
	HAB_UNS_KEY = 0x1b,	        /* Unsupported key type/parameters */
	HAB_UNS_PROTOCOL = 0x14,	/* Unsupported protocol */
	HAB_UNS_STATE = 0x09,		/* Unsuitable state */
	HAB_RSN_MAX
};

enum hab_context {
	HAB_CTX_ANY = 0x00,			/* Match any context */
	HAB_CTX_FAB = 0xff,		    /* Event logged in hab_fab_test() */
	HAB_CTX_ENTRY = 0xe1,		/* Event logged in hab_rvt.entry() */
	HAB_CTX_TARGET = 0x33,	    /* Event logged in hab_rvt.check_target() */
	HAB_CTX_AUTHENTICATE = 0x0a,/* Logged in hab_rvt.authenticate_image() */
	HAB_CTX_DCD = 0xdd,         /* Event logged in hab_rvt.run_dcd() */
	HAB_CTX_CSF = 0xcf,         /* Event logged in hab_rvt.run_csf() */
	HAB_CTX_COMMAND = 0xc0,     /* Event logged executing csf/dcd command */
	HAB_CTX_AUT_DAT = 0xdb,		/* Authenticated data block */
	HAB_CTX_ASSERT = 0xa0,		/* Event logged in hab_rvt.assert() */
	HAB_CTX_EXIT = 0xee,		/* Event logged in hab_rvt.exit() */
	HAB_CTX_MAX
};

struct imx_sec_config_fuse_t {
	int bank;
	int word;
};

#if defined(CONFIG_SECURE_BOOT)
extern struct imx_sec_config_fuse_t const imx_sec_config_fuse;
#endif

/*Function prototype description*/
typedef enum hab_status hab_rvt_report_event_t(enum hab_status, uint32_t,
		uint8_t* , size_t*);
typedef enum hab_status hab_rvt_report_status_t(enum hab_config *,
		enum hab_state *);
typedef enum hab_status hab_loader_callback_f_t(void**, size_t*, const void*);
typedef enum hab_status hab_rvt_entry_t(void);
typedef enum hab_status hab_rvt_exit_t(void);
typedef void *hab_rvt_authenticate_image_t(uint8_t, ptrdiff_t,
		void **, size_t *, hab_loader_callback_f_t);
typedef void hapi_clock_init_t(void);

#define HAB_ENG_ANY		0x00   /* Select first compatible engine */
#define HAB_ENG_SCC		0x03   /* Security controller */
#define HAB_ENG_RTIC	0x05   /* Run-time integrity checker */
#define HAB_ENG_SAHARA  0x06   /* Crypto accelerator */
#define HAB_ENG_CSU		0x0a   /* Central Security Unit */
#define HAB_ENG_SRTC	0x0c   /* Secure clock */
#define HAB_ENG_DCP		0x1b   /* Data Co-Processor */
#define HAB_ENG_CAAM	0x1d   /* CAAM */
#define HAB_ENG_SNVS	0x1e   /* Secure Non-Volatile Storage */
#define HAB_ENG_OCOTP	0x21   /* Fuse controller */
#define HAB_ENG_DTCP	0x22   /* DTCP co-processor */
#define HAB_ENG_ROM		0x36   /* Protected ROM area */
#define HAB_ENG_HDCP	0x24   /* HDCP co-processor */
#define HAB_ENG_RTL		0x77   /* RTL simulation engine */
#define HAB_ENG_SW		0xff   /* Software engine */

#ifdef CONFIG_ROM_UNIFIED_SECTIONS
#define HAB_RVT_BASE			0x00000100
#else
#define HAB_RVT_BASE			0x00000094
#endif

#define HAB_RVT_ENTRY			(*(uint32_t *)(HAB_RVT_BASE + 0x04))
#define HAB_RVT_EXIT			(*(uint32_t *)(HAB_RVT_BASE + 0x08))
#define HAB_RVT_AUTHENTICATE_IMAGE	(*(uint32_t *)(HAB_RVT_BASE + 0x10))
#define HAB_RVT_REPORT_EVENT		(*(uint32_t *)(HAB_RVT_BASE + 0x20))
#define HAB_RVT_REPORT_STATUS		(*(uint32_t *)(HAB_RVT_BASE + 0x24))

#define HAB_RVT_REPORT_EVENT_NEW               (*(uint32_t *)0x000000B8)
#define HAB_RVT_REPORT_STATUS_NEW              (*(uint32_t *)0x000000BC)
#define HAB_RVT_AUTHENTICATE_IMAGE_NEW         (*(uint32_t *)0x000000A8)
#define HAB_RVT_ENTRY_NEW                      (*(uint32_t *)0x0000009C)
#define HAB_RVT_EXIT_NEW                       (*(uint32_t *)0x000000A0)

#define HAB_CID_ROM 0 /**< ROM Caller ID */
#define HAB_CID_UBOOT 1 /**< UBOOT Caller ID*/

/* ----------- end of HAB API updates ------------*/

#endif
