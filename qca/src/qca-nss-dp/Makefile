###################################################
# Makefile for the NSS data plane driver
###################################################

obj ?= .

obj-m += qca-nss-dp.o

qca-nss-dp-objs += nss_dp_attach.o \
		   nss_dp_ethtools.o \
		   nss_dp_main.o \
		   hal/soc_ops/$(SoC)/nss_$(SoC).o

ifneq ($(CONFIG_NET_SWITCHDEV),)
qca-nss-dp-objs += nss_dp_switchdev.o
endif

NSS_DP_INCLUDE = -I$(obj)/include -I$(obj)/exports -I$(obj)/hal/include \
		 -I$(obj)/hal/dp_ops/include

ifeq ($(SoC),$(filter $(SoC),ipq807x ipq60xx))
qca-nss-dp-objs += hal/dp_ops/edma_dp/edma_v1/edma_cfg.o \
		   hal/dp_ops/edma_dp/edma_v1/edma_data_plane.o \
		   hal/dp_ops/edma_dp/edma_v1/edma_tx_rx.o \
		   hal/gmac_ops/qcom/qcom_if.o \
		   hal/gmac_ops/syn/xgmac/syn_if.o
NSS_DP_INCLUDE += -I$(obj)/hal/dp_ops/edma_dp/edma_v1/include
ccflags-y += -DNSS_DP_PPE_SUPPORT
endif

ifeq ($(SoC),$(filter $(SoC),ipq807x))
ccflags-y += -DNSS_DP_IPQ807X -DNSS_DP_EDMA_TX_SMALL_PKT_WAR
endif

ifeq ($(SoC),$(filter $(SoC),ipq60xx))
ccflags-y += -DNSS_DP_IPQ60XX
endif

ifeq ($(SoC),$(filter $(SoC),ipq50xx))
qca-nss-dp-objs += hal/dp_ops/syn_gmac_dp/syn_dp_cfg_rx.o \
		   hal/dp_ops/syn_gmac_dp/syn_dp_cfg_tx.o \
		   hal/dp_ops/syn_gmac_dp/syn_dp_rx.o \
		   hal/dp_ops/syn_gmac_dp/syn_dp_tx.o \
		   hal/dp_ops/syn_gmac_dp/syn_dp.o \
		   hal/gmac_ops/syn/gmac/syn_if.o
NSS_DP_INCLUDE += -I$(obj)/hal/dp_ops/syn_gmac_dp/include
ccflags-y += -DNSS_DP_IPQ50XX
endif

ifeq ($(SoC),$(filter $(SoC),ipq95xx))
qca-nss-dp-objs += hal/dp_ops/edma_dp/edma_v2/edma.o \
		   hal/dp_ops/edma_dp/edma_v2/edma_cfg_rx.o \
		   hal/dp_ops/edma_dp/edma_v2/edma_cfg_tx.o \
		   hal/dp_ops/edma_dp/edma_v2/edma_dp.o \
		   hal/dp_ops/edma_dp/edma_v2/edma_misc.o \
		   hal/dp_ops/edma_dp/edma_v2/edma_rx.o \
		   hal/dp_ops/edma_dp/edma_v2/edma_tx.o \
		   hal/gmac_ops/qcom/qcom_if.o \
		   hal/gmac_ops/syn/xgmac/syn_if.o
NSS_DP_INCLUDE += -I$(obj)/hal/dp_ops/edma_dp/edma_v2
NSS_DP_INCLUDE += -I$(obj)/hal/dp_ops/edma_dp/edma_v2/include
ccflags-y += -DNSS_DP_IPQ95XX -DNSS_DP_PPE_SUPPORT
endif

ccflags-y += $(NSS_DP_INCLUDE)
ccflags-y += -Wall -Werror
ccflags-y += -DEDMA_DEBUG_LEVEL=2
