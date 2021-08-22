#
# @brief NSS CFI Makefile
#

export BUILD_ID = \"Build Id: $(shell date +'%m/%d/%y, %H:%M:%S')\"

ifneq (,$(filter $(ocf), y))
obj-m += $(CFI_OCF_DIR)/
endif

ifneq (,$(filter $(cryptoapi), y))
obj-m += $(CFI_CRYPTOAPI_DIR)/
endif

ifeq ($(SoC),$(filter $(SoC),ipq806x))
obj-m += $(CFI_IPSEC_DIR)/
endif
