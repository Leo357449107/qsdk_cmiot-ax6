#
# Copyright (c) 2015-2021 The Linux Foundation. All rights reserved.
#

QCA_LITHIUM:=kmod-qvit-lithium
QCA_EDMA:=kmod-qca-edma
NSS_COMMON:= \
	kmod-qca-nss-dp \
	kmod-qca-nss-drv \
	kmod-qca-nss-gmac

NSS_EIP197_FW:= \
	qca-nss-fw-eip-hk \
	qca-nss-fw-eip-cp

NSS_STANDARD:= \
	qca-nss-fw2-retail \
	qca-nss-fw-hk-retail \
	qca-nss-fw-cp-retail \
	qca-nss-fw-mp-retail

NSS_ENTERPRISE:= \
	qca-nss-fw2-enterprise \
	qca-nss-fw2-enterprise_custA \
	qca-nss-fw2-enterprise_custC \
	qca-nss-fw2-enterprise_custR \
	qca-nss-fw-hk-enterprise \
	qca-nss-fw-hk-enterprise_custA \
	qca-nss-fw-hk-enterprise_custC \
	qca-nss-fw-hk-enterprise_custR \
	qca-nss-fw-cp-enterprise \
	qca-nss-fw-mp-enterprise \
	qca-nss-fw-cp-enterprise_custA \
	qca-nss-fw-cp-enterprise_custC \
	qca-nss-fw-cp-enterprise_custR

NSS_MACSEC:= \
	kmod-qca-nss-macsec \
	qca-wpa-supplicant-macsec \
	qca-hostap-macsec

QCA_ECM_STANDARD:= kmod-qca-nss-ecm-standard
QCA_ECM_PREMIUM:= kmod-qca-nss-ecm-premium
QCA_ECM_ENTERPRISE:= kmod-qca-nss-ecm-noload

NSS_CLIENTS_STANDARD:= kmod-qca-nss-drv-qdisc kmod-qca-nss-drv-igs kmod-qca-nss-drv-tun6rd \
	kmod-qca-nss-drv-tunipip6 kmod-qca-nss-drv-l2tpv2 kmod-qca-nss-drv-pptp \
	kmod-qca-nss-drv-map-t kmod-qca-nss-drv-lag-mgr \
	kmod-qca-nss-drv-bridge-mgr kmod-qca-nss-drv-gre kmod-qca-nss-drv-pppoe \
	kmod-qca-nss-drv-ovpn-mgr kmod-qca-nss-drv-ovpn-link kmod-qca-nss-drv-vxlanmgr kmod-qca-nss-drv-netlink \
	kmod-qca-ovsmgr kmod-qca-nss-drv-match kmod-qca-nss-drv-mirror kmod-qca-nss-drv-mscs

NSS_CLIENTS_256MB:= kmod-qca-nss-drv-bridge-mgr kmod-qca-nss-drv-pppoe

NSS_CLIENTS_ENTERPRISE:= kmod-qca-nss-drv-qdisc kmod-qca-nss-drv-profile \
	kmod- kmod-qca-nss-drv-bridge-mgr kmod-qca-nss-drv-netlink kmod-qca-nss-drv-tlsmgr \
	kmod-qca-nss-drv-match kmod-qca-nss-drv-mirror kmod-qca-nss-drv-mscs

NSS_CRYPTO:= kmod-qca-nss-crypto kmod-qca-nss-cfi-cryptoapi kmod-qca-nss-cfi-ocf kmod-qca-nss-drv-ipsecmgr kmod-crypto-ocf kmod-qca-nss-drv-ipsecmgr-klips

NSS_RMNET:= kmod-rmnet-nss

HW_CRYPTO:= kmod-crypto-qcrypto

SHORTCUT_FE:= kmod-shortcut-fe kmod-shortcut-fe-cm kmod-shortcut-fe-drv
QCA_RFS:= kmod-qca-rfs

SWITCH_SSDK_PKGS:= kmod-qca-ssdk-hnat kmod-qca-ssdk-nohnat qca-ssdk-shell swconfig
SWITCH_SSDK_NOHNAT_PKGS:= kmod-qca-ssdk-nohnat qca-ssdk-shell swconfig
SWITCH_OPEN_PKGS:= kmod-switch-ar8216 swconfig

MACSEC_OPEN_PKGS:= wpa-supplicant-macsec hostapd-macsec

WIFI_OPEN_PKGS:= kmod-ath9k kmod-ath10k kmod-ath11k wpad-mesh hostapd-utils \
		 kmod-art2-netlink sigma-dut-open wpa-cli qcmbr-10.4-netlink \
		 athtestcmd-lith-nl ath10k-firmware-qca988x ath10k-firmware-qca9887 \
		 ath10k-firmware-qca9888 ath10k-firmware-qca9984 \
		 ath10k-firmware-qca4019 \
		 -qca-whc-lbd -qca-whc-init -libhyficommon -qca-thermald-10.4

WIFI_OPEN_PKGS_8M:= kmod-ath11k wpad-mesh hostapd-utils \
		wpa-cli -qca-whc-lbd -qca-whc-init -libhyficommon \
		wififw_mount_script

WIFI_10_4_PKGS:=kmod-qca-wifi-10.4-unified-profile \
    qca-hostap-10.4 qca-hostapd-cli-10.4 qca-wpa-supplicant-10.4 \
    qca-wpa-cli-10.4 qca-spectral-10.4 qca-wpc-10.4 sigma-dut-10.4 \
    qcmbr-10.4 qca-wrapd-10.4 qca-wapid qca-acfg-10.4 whc whc-ui \
    qca-lowi qca-iface-mgr-10.4 athdiag

WIFI_PKGS:=kmod-qca-wifi-unified-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-spectral qca-wpc sigma-dut \
	qcmbr-10.4 qca-wrapd qca-wapid qca-acfg whc-mesh whc-ui \
	qca-lowi qca-iface-mgr-10.4 qca-icm qca-cfg80211 athdiag qca-cnss-daemon \
	athtestcmd-lith qca-cfg80211tool

WIFI_PKGS_MINENT:=kmod-qca-wifi-custc-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-spectral qca-wpc sigma-dut \
	qcmbr-10.4 qca-wrapd qca-wapid qca-acfg whc whc-ui \
	qca-lowi qca-iface-mgr-10.4 qca-icm qca-cfg80211 athdiag qca-cnss-daemon \
	athtestcmd-lith qca-cfg80211tool

WIFI_PKGS_256MB:=kmod-qca-wifi-lowmem-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-wpc sigma-dut \
	qcmbr-10.4 qca-wrapd qca-wapid qca-acfg whc-mesh whc-ui \
	qca-iface-mgr-10.4 qca-icm qca-cfg80211 athdiag qca-cnss-daemon \
	athtestcmd-lith qca-cfg80211tool

WIFI_PKGS_16M:=kmod-qca-wifi-flash_16mb-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-cfg80211 qca-cfg80211tool

WIFI_10_4_FW_PKGS:=qca-wifi-fw-hw2-10.4-asic qca-wifi-fw-hw4-10.4-asic \
	qca-wifi-fw-hw3-10.4-asic qca-wifi-fw-hw6-10.4-asic \
	qca-wifi-fw-hw5-10.4-asic qca-wifi-fw-hw11-10.4-asic \
	qca-wifi-hk-fw-hw1-10.4-asic qca-wifi-cyp-fw-hw1-11.0-asic

WIL6210_PKGS:=kmod-wil6210 wigig-firmware iwinfo qca-fst-manager

OPENWRT_STANDARD:= \
	luci openssl-util

OPENWRT_256MB:=luci pm-utils wififw_mount_script qca-thermald-10.4 qca-wlanfw-upgrade -file \
		-kmod-ata-core -kmod-ata-ahci -kmod-ata-ahci-platform \
		-kmod-usb2 -kmod-usb3 -kmod-usb-dwc3-qcom \
		-kmod-usb-phy-qcom-dwc3 -kmod-usb-dwc3-of-simple \
		-kmod-usb-phy-ipq807x -kmod-usb-f-qdss

STORAGE:=kmod-scsi-core kmod-usb-storage kmod-usb-uas kmod-nls-cp437 kmod-nls-iso8859-1  \
	kmod-fs-msdos kmod-fs-vfat kmod-fs-ntfs ntfs-3g e2fsprogs

USB_ETHERNET:= kmod-usb-net-rtl8152 kmod-usb-net

TEST_TOOLS:=ethtool i2c-tools tcpdump

UTILS:=file luci-app-samba rng-tools profilerd

COREBSP_UTILS:=pm-utils wififw_mount_script qca-thermald-10.4 qca-qmi-framework qca-time-services \
	qca-wlanfw-upgrade dashboard

FAILSAFE:= kmod-bootconfig

NETWORKING:=mcproxy -dnsmasq dnsmasq-dhcpv6 bridge ip-full \
	rp-pppoe-relay iptables-mod-extra iputils-tracepath iputils-tracepath6 \
	kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp \
	luci-app-upnp luci-app-ddns luci-proto-ipv6 \
	luci-app-multiwan kmod-bonding

NETWORKING_256MB:=-dnsmasq dnsmasq-dhcpv6 bridge ip-full \
	rp-pppoe-relay iptables-mod-extra iputils-tracepath iputils-tracepath6 \
	kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp \
	luci-app-upnp luci-app-ddns luci-proto-ipv6 \
	luci-app-multiwan

NETWORKING_8MB:=dnsmasq -dnsmasq-dhcpv6 kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp

NETWORKING_16MB:=-dnsmasq dnsmasq-dhcpv6 kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp ip \
		rp-pppoe-relay

CD_ROUTER:=kmod-ipt-ipopt kmod-bonding kmod-nat-sctp lacpd \
	arptables ds-lite 6rd ddns-scripts xl2tpd \
	quagga quagga-ripd quagga-zebra quagga-watchquagga quagga-vtysh \
	kmod-ipv6 ip6tables iptables-mod-ipsec iptables-mod-filter \
	isc-dhcp-relay-ipv6 rp-pppoe-server ppp-mod-pptp

CD_ROUTER_256MB:=kmod-ipt-ipopt kmod-nat-sctp lacpd \
	arptables ddns-scripts \
	quagga quagga-ripd quagga-zebra quagga-watchquagga quagga-vtysh \
	kmod-ipv6 ip6tables iptables-mod-filter \
	isc-dhcp-relay-ipv6 rp-pppoe-server

BLUETOOTH:=kmod-bluetooth bluez-libs bluez-utils kmod-ath3k

BLUETOPIA:=bluetopia

ZIGBEE:=zigbee_efr32

MINIDUMP:=minidump

QOS:=tc kmod-sched kmod-sched-core kmod-sched-connmark kmod-ifb iptables \
	iptables-mod-filter iptables-mod-ipopt iptables-mod-conntrack-extra

MAP_PKGS:=map-t 464xlat tayga

HYFI:=hyfi-mesh hyfi-ui
PLC:=qca-plc-serv qca-plc-fw

QCA_EZMESH:=qca-ezmesh qca-ezmesh-ctrl qca-ezmesh-agent qca-ezmesh-alg qca-ezmesh-agentalg

QCA_MAD:=qca-mad

AQ_PHY:=kmod-aq_phy kmod-qca_85xx_sw aq-fw-download
#These packages depend on SWITCH_SSDK_PKGS
IGMPSNOOING_RSTP:=rstp qca-mcs-apps

IPSEC:=openswan kmod-ipsec kmod-ipsec4 kmod-ipsec6

AUDIO:=kmod-sound-soc-ipq alsa

VIDEO:=kmod-qpic_panel_ertft

NSS_USERSPACE:=nlcfg

KPI:=iperf-mt sysstat

USB_DIAG:=kmod-diag-char kmod-usb-f-diag qca-diag kmod-usb-gdiag

CHAR_DIAG:=kmod-diag-char qca-diag

CNSS_DIAG:=cnssdiag

FTM:=ftm

QMSCT_CLIENT:=qmsct_client

OPENVPN:= openvpn-easy-rsa openvpn-openssl luci-app-openvpn

QRTR:=qca-qrtr

QMI_SAMPLE_APP:=kmod-qmi_sample_client

MHI_QRTR:=kmod-mhi-qrtr-mproc

EMESH_SP:=kmod-emesh-sp

ifneq ($(LINUX_VERSION),3.18.21)
	EXTRA_NETWORKING:=$(NSS_COMMON) $(QCA_EDMA) $(NSS_STANDARD) $(CD_ROUTER) -lacpd \
	$(HW_CRYPTO) $(QCA_RFS) $(AUDIO) $(VIDEO) \
	$(IGMPSNOOING_RSTP) $(IPSEC) $(QOS) $(QCA_ECM_PREMIUM) kmod-qca-nss-macsec \
	$(NSS_CRYPTO) $(NSS_CLIENTS_STANDARD) $(NSS_EIP197_FW) $(MAP_PKGS) $(AQ_PHY) $(FAILSAFE) \
	$(SWITCH_OPEN_PKGS) rdk-v-wifi-ath10k $(MACSEC_OPEN_PKGS)
endif

define Profile/QSDK_Open
	NAME:=Qualcomm-Atheros SDK Open Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(SWITCH_SSDK_NOHNAT_PKGS) $(QCA_EDMA) \
	$(WIFI_OPEN_PKGS) $(STORAGE) $(USB_ETHERNET) $(UTILS) $(NETWORKING) \
	$(TEST_TOOLS) $(COREBSP_UTILS) $(KPI) $(SHORTCUT_FE) $(EXTRA_NETWORKING) \
	$(USB_DIAG) $(FTM) $(CNSS_DIAG) qca-cnss-daemon qca-wifi-hk-fw-hw1-10.4-asic \
	qrtr $(QMI_SAMPLE_APP) $(FAILSAFE) ath11k-fwtest ath11k-qdss $(MHI_QRTR) \
	-lk-ipq807x -lk-ipq806x -lk-ipq6018 -lk-ipq40xx
endef

define Profile/QSDK_Open/Description
	QSDK Open package set configuration.
	Enables wifi open source packages
endef

$(eval $(call Profile,QSDK_Open))

define Profile/QSDK_Premium
	NAME:=Qualcomm-Atheros SDK Premium Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(QCA_EDMA) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS) $(WIFI_10_4_FW_PKGS) $(STORAGE) $(CD_ROUTER) \
		$(NETWORKING) $(OPENVPN) $(UTILS) $(SHORTCUT_FE) $(HW_CRYPTO) $(QCA_RFS) \
		$(AUDIO) $(VIDEO) $(IGMPSNOOING_RSTP) $(IPSEC) $(QOS) $(QCA_ECM_PREMIUM) \
		$(NSS_MACSEC) $(TEST_TOOLS) $(NSS_CRYPTO) $(NSS_CLIENTS_STANDARD) $(COREBSP_UTILS) \
		$(MAP_PKGS) $(HYFI) $(QCA_MAD) $(QCA_EZMESH) $(AQ_PHY) $(FAILSAFE) kmod-art2 -lacpd $(USB_DIAG) \
		$(QCA_LITHIUM) $(NSS_EIP197_FW) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) \
		$(MHI_QRTR) $(KPI) $(QRTR) $(NSS_USERSPACE) \
		$(NSS_RMNET) $(MINIDUMP) $(EMESH_SP) kmod-macvlan
endef

define Profile/QSDK_Premium/Description
	QSDK Premium package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_Premium))

define Profile/QSDK_Standard
	NAME:=Qualcomm-Atheros SDK Standard Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(QCA_EDMA) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS) $(STORAGE) $(SHORTCUT_FE) $(HW_CRYPTO) $(QCA_RFS) \
		$(IGMPSNOOING_RSTP) $(NETWORKING) $(QOS) $(UTILS) ethtool $(COREBSP_UTILS) \
		qca-wifi-fw-hw5-10.4-asic $(KPI)
endef

define Profile/QSDK_Standard/Description
	QSDK Standard package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_Standard))

define Profile/QSDK_QBuilder
	NAME:=Qualcomm-Atheros SDK QBuilder Profile
	PACKAGES:=luci openssl-util kmod-qca-nss-dp kmod-qca-nss-drv kmod-qca-nss-gmac qca-nss-fw2-retail \
		qca-nss-fw-hk-retail qca-nss-fw-cp-retail kmod-qca-ssdk-nohnat qca-ssdk-shell swconfig \
		kmod-scsi-core kmod-usb-storage kmod-usb-uas kmod-nls-cp437 kmod-nls-iso8859-1 kmod-fs-msdos \
		kmod-fs-vfat kmod-fs-ntfs ntfs-3g e2fsprogs kmod-shortcut-fe kmod-shortcut-fe-cm kmod-shortcut-fe-drv \
		rstp qca-mcs-apps kmod-qca-wifi-unified-profile qca-hostap qca-hostapd-cli qca-wpa-supplicant qca-wpa-cli \
		qca-spectral qca-wpc sigma-dut qcmbr-10.4 qca-wrapd qca-wapid qca-acfg whc-mesh whc-ui qca-lowi qca-iface-mgr-10.4 \
		qca-icm qca-cfg80211 athdiag qca-cnss-daemon athtestcmd-lith qca-wifi-fw-hw2-10.4-asic qca-wifi-fw-hw4-10.4-asic \
		qca-wifi-fw-hw3-10.4-asic qca-wifi-fw-hw6-10.4-asic qca-wifi-fw-hw5-10.4-asic qca-wifi-fw-hw11-10.4-asic \
		qca-wifi-hk-fw-hw1-10.4-asic qca-wifi-cyp-fw-hw1-11.0-asic kmod-aq_phy kmod-qca_85xx_sw aq-fw-download mcproxy \
		-dnsmasq dnsmasq-dhcpv6 bridge ip-full rp-pppoe-relay iptables-mod-extra iputils-tracepath iputils-tracepath6 \
		kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp luci-app-upnp luci-app-ddns luci-proto-ipv6 luci-app-multiwan tc kmod-sched \
		kmod-sched-core kmod-sched-connmark kmod-ifb iptables iptables-mod-filter iptables-mod-ipopt iptables-mod-conntrack-extra \
		qca-nss-fw-eip-hk qca-nss-fw-eip-cp file luci-app-samba rng-tools profilerd ethtool i2c-tools tcpdump pm-utils \
		wififw_mount_script qca-thermald-10.4 qca-qmi-framework qca-time-services qca-wlanfw-upgrade dashboard qca-wifi-fw-hw5-10.4-asic \
		iperf-mt sysstat nlcfg kmod-bootconfig qca-cfg80211tool
endef

define Profile/QSDK_QBuilder/Description
	QSDK QBuilder package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_QBuilder))

define Profile/QSDK_Enterprise
	NAME:=Qualcomm-Atheros SDK Enterprise Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(QCA_EDMA) $(NSS_ENTERPRISE) $(SWITCH_SSDK_NOHNAT_PKGS) \
		$(WIFI_PKGS) $(WIFI_10_4_FW_PKGS) $(STORAGE) $(HW_CRYPTO) $(QCA_RFS) \
		$(IGMPSNOOING_RSTP) $(NETWORKING) $(QOS) $(UTILS) $(TEST_TOOLS) $(COREBSP_UTILS) \
		$(QCA_ECM_ENTERPRISE) $(NSS_CLIENTS_ENTERPRISE) $(NSS_MACSEC) $(NSS_CRYPTO) \
		$(IPSEC) $(NSS_EIP197_FW) $(CD_ROUTER) $(AQ_PHY) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) -lacpd \
		$(USB_DIAG) $(MHI_QRTR) $(KPI) $(QRTR) $(FAILSAFE) $(NSS_USERSPACE)
endef

define Profile/QSDK_Enterprise/Description
	QSDK Enterprise package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_Enterprise))

define Profile/QSDK_MinEnt
	NAME:=Qualcomm-Atheros SDK MinEnt Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(QCA_EDMA) $(NSS_ENTERPRISE) $(SWITCH_SSDK_NOHNAT_PKGS) \
		$(WIFI_PKGS_MINENT) $(WIFI_10_4_FW_PKGS) $(STORAGE) $(HW_CRYPTO) $(QCA_RFS) \
		$(IGMPSNOOING_RSTP) $(NETWORKING) $(QOS) $(UTILS) $(TEST_TOOLS) $(COREBSP_UTILS) \
		$(QCA_ECM_ENTERPRISE) $(NSS_CLIENTS_ENTERPRISE) $(NSS_MACSEC) $(NSS_CRYPTO) \
		$(IPSEC) $(NSS_EIP197_FW) $(CD_ROUTER) $(AQ_PHY) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) -lacpd \
		$(USB_DIAG) $(MHI_QRTR) $(KPI) $(QRTR) $(FAILSAFE) $(NSS_USERSPACE)
endef

define Profile/QSDK_MinEnt/Description
	QSDK MinEnt package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_MinEnt))

define Profile/QSDK_Deluxe
	NAME:=Qualcomm-Atheros SDK Deluxe Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(QCA_EDMA) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS) $(WIFI_10_4_FW_PKGS) $(STORAGE) $(CD_ROUTER) \
		$(NETWORKING) $(UTILS) $(SHORTCUT_FE) $(HW_CRYPTO) $(QCA_RFS) \
		$(AUDIO) $(VIDEO) $(IGMPSNOOING_RSTP) $(IPSEC) $(QOS) $(QCA_ECM_PREMIUM) \
		$(NSS_MACSEC) $(TEST_TOOLS) $(NSS_CRYPTO) $(NSS_CLIENTS_STANDARD) $(COREBSP_UTILS) \
		$(MAP_PKGS) $(HYFI) $(QCA_EZMESH) $(AQ_PHY) $(FAILSAFE) kmod-art2 -lacpd $(USB_DIAG) \
		$(QCA_LITHIUM) $(NSS_EIP197_FW) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) \
		qca-wifi-npr-fw-hw1-10.4-asic $(KPI)
endef

define Profile/QSDK_Deluxe/Description
	QSDK Deluxe package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_Deluxe))

define Profile/QSDK_256
	NAME:=Qualcomm-Atheros SDK 256MB Profile
	PACKAGES:=$(OPENWRT_256MB) $(NSS_COMMON) $(QCA_EDMA) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS_256MB) qca-wifi-hk-fw-hw1-10.4-asic $(CD_ROUTER_256MB) $(NETWORKING_256MB) \
		iperf-mt rng-tools $(QCA_RFS) $(IGMPSNOOING_RSTP) $(CHAR_DIAG) \
		$(QCA_ECM_STANDARD) $(NSS_MACSEC) \
		$(NSS_CLIENTS_256MB) $(HYFI) $(QCA_EZMESH) $(FAILSAFE) -lacpd \
		$(QCA_LITHIUM) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) qca-wifi-cyp-fw-hw1-11.0-asic \
		$(QRTR) $(MHI_QRTR) kmod-macvlan $(EMESH_SP) e2fsprogs
endef

define Profile/QSDK_256/Description
	QSDK Premium package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_256))

define Profile/QSDK_512
	NAME:=Qualcomm-Atheros SDK 512MB Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(QCA_EDMA) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS) $(WIFI_10_4_FW_PKGS) $(STORAGE) $(CD_ROUTER) \
		$(NETWORKING) $(OPENVPN) $(UTILS) $(SHORTCUT_FE) $(HW_CRYPTO) $(QCA_RFS) \
		$(AUDIO) $(VIDEO) $(IGMPSNOOING_RSTP) $(IPSEC) $(QOS) $(QCA_ECM_PREMIUM) \
		$(NSS_MACSEC) $(TEST_TOOLS) $(NSS_CRYPTO) $(NSS_CLIENTS_STANDARD) $(COREBSP_UTILS) \
		$(MAP_PKGS) $(HYFI) $(QCA_EZMESH) $(AQ_PHY) $(FAILSAFE) -lacpd $(USB_DIAG) \
		$(QCA_LITHIUM) $(NSS_EIP197_FW) $(EMESH_SP) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) $(KPI) $(NSS_USERSPACE) \
		$(QRTR) $(MHI_QRTR) $(NSS_RMNET) kmod-macvlan
endef

define Profile/QSDK_512/Description
	QSDK Premium package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_512))

define Profile/QSDK_8M
	NAME:=Qualcomm-Atheros SDK 8MB Flash Profile
	PACKAGES:=$(NSS_COMMON) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_OPEN_PKGS_8M) $(NETWORKING_8MB) \
		$(IGMPSNOOING_RSTP) $(QCA_ECM_STANDARD) \
		$(NSS_CLIENTS_256MB) qrtr
endef

define Profile/QSDK_8M/Description
	QSDK 8M package set configuration.
	Enables wifi open source packages
endef

$(eval $(call Profile,QSDK_8M))

define Profile/QSDK_16M
	NAME:=Qualcomm-Atheros SDK 16MB Flash Profile
	PACKAGES:=wififw_mount_script $(NSS_COMMON) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS_16M) qca-wifi-hk-fw-hw1-10.4-asic $(NETWORKING_16MB) \
		$(IGMPSNOOING_RSTP) $(QCA_ECM_STANDARD) $(NSS_CLIENTS_256MB) \
		$(QRTR) $(MHI_QRTR) xz xz-utils \
		-kmod-usb-f-qdss -kmod-bt_tty -kmod-clk-test \
		-kmod-testssr -kmod-ata-core -kmod-ata-ahci -kmod-ata-ahci-platform \
		-kmod-usb2 -kmod-usb3 -kmod-usb-phy-ipq5018 -kmod-usb-dwc3-qcom \
		-kmod-bt_tty -kmod-clk-test -sysupgrade-helper \
		-fwupgrade-tools
endef

define Profile/QSDK_16M/Description
	QSDK 16M package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_16M))
