ifneq ($(TARGET_PRODUCT),qssi)
RMNET_CORE_DLKM_PLATFORMS_LIST := lahaina
RMNET_CORE_DLKM_PLATFORMS_LIST += holi

ifeq ($(call is-board-platform-in-list, $(RMNET_CORE_DLKM_PLATFORMS_LIST)),true)
#Make file to create RMNET_CORE DLKM
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wno-macro-redefined -Wno-unused-function -Wall -Werror
LOCAL_CLANG :=true

LOCAL_MODULE_PATH := $(KERNEL_MODULES_OUT)
LOCAL_MODULE := rmnet_core.ko

LOCAL_SRC_FILES := \
	rmnet_config.c \
	rmnet_descriptor.c \
	rmnet_genl.c \
	rmnet_handlers.c \
	rmnet_map_command.c \
	rmnet_map_data.c \
	rmnet_vnd.c \
	dfc_qmap.c \
	dfc_qmi.c \
	qmi_rmnet.c \
	wda_qmi.c

RMNET_BLD_DIR := ../../vendor/qcom/opensource/datarmnet/core
DLKM_DIR := $(TOP)/device/qcom/common/dlkm

KBUILD_OPTIONS := $(RMNET_BLD_DIR)

$(warning $(DLKM_DIR))
include $(DLKM_DIR)/AndroidKernelModule.mk

######## Create RMNET_CTL DLKM ########
include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wno-macro-redefined -Wno-unused-function -Wall -Werror
LOCAL_CLANG :=true

LOCAL_MODULE_PATH := $(KERNEL_MODULES_OUT)
LOCAL_MODULE := rmnet_ctl.ko

LOCAL_SRC_FILES := \
	rmnet_ctl_client.c \
	rmnet_ctl_ipa.c

RMNET_BLD_DIR := ../../vendor/qcom/opensource/datarmnet/core
DLKM_DIR := $(TOP)/device/qcom/common/dlkm

KBUILD_OPTIONS := $(RMNET_BLD_DIR)

$(warning $(DLKM_DIR))
include $(DLKM_DIR)/AndroidKernelModule.mk

endif #End of Check for target
endif #End of Check for qssi target
