#Build rmnet core
DATA_DLKM_BOARD_PLATFORMS_LIST := lahaina
DATA_DLKM_BOARD_PLATFORMS_LIST += holi
ifneq ($(TARGET_BOARD_AUTO),true)
ifeq ($(call is-board-platform-in-list,$(DATA_DLKM_BOARD_PLATFORMS_LIST)),true)
BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/rmnet_core.ko
BOARD_VENDOR_KERNEL_MODULES += $(KERNEL_MODULES_OUT)/rmnet_ctl.ko
endif
endif

