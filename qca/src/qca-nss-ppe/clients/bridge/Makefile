ccflags-y := -I$(obj)/../../exports -I$(obj)/..
ccflags-y += -DNSS_PPE_BUILD_ID="$(BUILD_ID)"
ccflags-$(NSS_BRIDGE_MGR_OVS_ENABLE) += -DNSS_BRIDGE_MGR_OVS_ENABLE

obj-m += qca-nss-ppe-bridge-mgr.o
qca-nss-ppe-bridge-mgr-objs := nss_ppe_bridge_mgr.o
ifeq ($(NSS_BRIDGE_MGR_OVS_ENABLE), y)
qca-nss-bridge-mgr-objs += nss_bridge_mgr_ovs.o
endif

ccflags-y += -DNSS_PPE_BRIDGE_MGR_DEBUG_LEVEL=0
ccflags-y += -Wall -Werror

ifneq (,$(filter $(CONFIG_BONDING),y m))
ccflags-y += -DBONDING_SUPPORT
endif
