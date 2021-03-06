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

/*
 * DOC: contains MLO manager containing init/deinit public api's
 */
#ifndef _WLAN_MLO_MGR_MAIN_H_
#define _WLAN_MLO_MGR_MAIN_H_

#include <qdf_atomic.h>
#include <wlan_objmgr_psoc_obj.h>
#include <wlan_objmgr_pdev_obj.h>
#include <wlan_objmgr_vdev_obj.h>
#include <wlan_mlo_mgr_public_structs.h>

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * wlan_mlo_mgr_init() - Initialize the MLO data structures
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_mgr_init(void);

/**
 * wlan_mlo_mgr_deinit() - De-init the MLO data structures
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_mgr_deinit(void);

/**
 * wlan_mlo_mgr_vdev_created_notification() - mlo mgr vdev create handler
 * @vdev: vdev object
 * @arg_list: Argument list
 *
 * This function is called as part of vdev creation. This will initialize
 * the MLO dev context if the interface type is ML.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_mgr_vdev_created_notification(struct wlan_objmgr_vdev *vdev,
						  void *arg_list);

/**
 * wlan_mlo_mgr_vdev_destroyed_notification() - mlo mgr vdev delete handler
 * @vdev: vdev object
 * @arg_list: Argument list
 *
 * This function is called as part of vdev delete. This will de-initialize
 * the MLO dev context if the interface type is ML.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_mlo_mgr_vdev_destroyed_notification(struct wlan_objmgr_vdev *vdev,
						    void *arg_list);

#ifdef WLAN_MLO_USE_SPINLOCK
/**
 * ml_link_lock_create - Create MLO link mutex/spinlock
 * @mlo_ctx:  MLO manager global context
 *
 * Creates mutex/spinlock
 *
 * Return: void
 */
static inline
void ml_link_lock_create(struct mlo_mgr_context *mlo_ctx)
{
	qdf_spinlock_create(&mlo_ctx->ml_dev_list_lock);
}

/**
 * ml_link_lock_destroy - Destroy ml link mutex/spinlock
 * @mlo_ctx:  MLO manager global context
 *
 * Destroy mutex/spinlock
 *
 * Return: void
 */
static inline void
ml_link_lock_destroy(struct mlo_mgr_context *mlo_ctx)
{
	qdf_spinlock_destroy(&mlo_ctx->ml_dev_list_lock);
}

/**
 * ml_link_lock_acquire - acquire ml link mutex/spinlock
 * @mlo_ctx:  MLO manager global context
 *
 * acquire mutex/spinlock
 *
 * return: void
 */
static inline
void ml_link_lock_acquire(struct mlo_mgr_context *mlo_ctx)
{
	qdf_spin_lock_bh(&mlo_ctx->ml_dev_list_lock);
}

/**
 * ml_link_lock_release - release MLO dev mutex/spinlock
 * @mlo_ctx:  MLO manager global context
 *
 * release mutex/spinlock
 *
 * return: void
 */
static inline
void ml_link_lock_release(struct mlo_mgr_context *mlo_ctx)
{
	qdf_spin_unlock_bh(&mlo_ctx->ml_dev_list_lock);
}

/**
 * mlo_dev_lock_create - Create MLO device mutex/spinlock
 * @mldev:  ML device context
 *
 * Creates mutex/spinlock
 *
 * Return: void
 */
static inline void
mlo_dev_lock_create(struct wlan_mlo_dev_context *mldev)
{
	qdf_spinlock_create(&mldev->mlo_dev_lock);
}

/**
 * mlo_dev_lock_destroy - Destroy CM SM mutex/spinlock
 * @mldev:  ML device context
 *
 * Destroy mutex/spinlock
 *
 * Return: void
 */
static inline void
mlo_dev_lock_destroy(struct wlan_mlo_dev_context *mldev)
{
	qdf_spinlock_destroy(&mldev->mlo_dev_lock);
}

/**
 * mlo_dev_lock_acquire - acquire CM SM mutex/spinlock
 * @mldev:  ML device context
 *
 * acquire mutex/spinlock
 *
 * return: void
 */
static inline
void mlo_dev_lock_acquire(struct wlan_mlo_dev_context *mldev)
{
	qdf_spin_lock_bh(&mldev->mlo_dev_lock);
}

/**
 * mlo_dev_lock_release - release MLO dev mutex/spinlock
 * @mldev:  ML device context
 *
 * release mutex/spinlock
 *
 * return: void
 */
static inline
void mlo_dev_lock_release(struct wlan_mlo_dev_context *mldev)
{
	qdf_spin_unlock_bh(&mldev->mlo_dev_lock);
}
#else
static inline
void ml_link_lock_create(struct mlo_mgr_context *mlo_ctx)
{
	qdf_mutex_create(&mlo_ctx->ml_dev_list_lock);
}

static inline void
ml_link_lock_destroy(struct mlo_mgr_context *mlo_ctx)
{
	qdf_mutex_destroy(&mlo_ctx->ml_dev_list_lock);
}

static inline
void ml_link_lock_acquire(struct mlo_mgr_context *mlo_ctx)
{
	qdf_mutex_acquire(&mlo_ctx->ml_dev_list_lock);
}

static inline
void ml_link_lock_release(struct mlo_mgr_context *mlo_ctx)
{
	qdf_mutex_release(&mlo_ctx->ml_dev_list_lock);
}

static inline
void mlo_dev_lock_create(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_create(&mldev->mlo_dev_lock);
}

static inline
void mlo_dev_lock_destroy(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_destroy(&mldev->mlo_dev_lock);
}

static inline void mlo_dev_lock_acquire(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_acquire(&mldev->mlo_dev_lock);
}

static inline void mlo_dev_lock_release(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_release(&mldev->mlo_dev_lock);
}
#endif /* WLAN_MLO_USE_SPINLOCK */

#else
static inline QDF_STATUS wlan_mlo_mgr_init(void)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS wlan_mlo_mgr_deinit(void)
{
	return QDF_STATUS_SUCCESS;
}
#endif
#endif
