NSS_COMMON:= \
	kmod-qca-nss-dp \
	kmod-qca-nss-drv \
	-kmod-qca-nss-gmac

NSS_EIP197_FW:= \
	qca-nss-fw-eip-hk \
	qca-nss-fw-eip-cp

NSS_STANDARD:= \
	-qca-nss-fw2-retail \
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
QCA_ECM_ENTERPRISE:= kmod-qca-nss-ecm-noload
QCA_ECM_PREMIUM:= kmod-qca-nss-ecm-premium

NSS_CLIENTS_STANDARD:= kmod-qca-nss-drv-qdisc kmod-qca-nss-drv-igs kmod-qca-nss-drv-tun6rd \
	kmod-qca-nss-drv-tunipip6 kmod-qca-nss-drv-l2tpv2 kmod-qca-nss-drv-pptp \
	kmod-qca-nss-drv-map-t kmod-qca-nss-drv-lag-mgr \
	kmod-qca-nss-drv-bridge-mgr kmod-qca-nss-drv-gre kmod-qca-nss-drv-pppoe \
	kmod-qca-nss-drv-ovpn-mgr kmod-qca-nss-drv-ovpn-link kmod-qca-nss-drv-vxlanmgr \
	kmod-qca-nss-drv-netlink kmod-qca-ovsmgr kmod-qca-nss-drv-match kmod-qca-nss-drv-mirror \
	kmod-qca-nss-drv-mscs

NSS_CLIENTS_256MB:= kmod-qca-nss-drv-bridge-mgr kmod-qca-nss-drv-pppoe

NSS_CLIENTS_ENTERPRISE:= kmod-qca-nss-drv-qdisc kmod-qca-nss-drv-profile \
	kmod- kmod-qca-nss-drv-bridge-mgr kmod-qca-nss-drv-netlink kmod-qca-nss-drv-tlsmgr \
	kmod-qca-nss-drv-match kmod-qca-nss-drv-mirror kmod-qca-nss-drv-mscs

NSS_CRYPTO:= kmod-qca-nss-crypto kmod-qca-nss-cfi-cryptoapi -kmod-qca-nss-cfi-ocf kmod-qca-nss-drv-ipsecmgr kmod-qca-nss-drv-ipsecmgr-xfrm -kmod-crypto-ocf -kmod-qca-nss-drv-ipsecmgr-klips

NSS_RMNET:= kmod-rmnet-nss

HW_CRYPTO:= kmod-crypto-qcrypto

NSS_SFE:= kmod-qca-nss-sfe

NSS_MESH:= kmod-qca-nss-drv-wifi-meshmgr

QCA_RFS:= kmod-qca-rfs

SWITCH_SSDK_PKGS:= kmod-qca-ssdk-hnat kmod-qca-ssdk-nohnat qca-ssdk-shell swconfig

MACSEC_OPEN_PKGS:= wpa-supplicant-macsec hostapd-macsec

WIFI_OPEN_PKGS:= kmod-ath11k wpad-mesh hostapd-utils \
	sigma-dut-open wpa-cli qcmbr-netlink iwinfo \
	athtestcmd-lith-nl -qca-whc-lbd -qca-whc-init -libhyficommon qca-wifi-scripts

WIFI_OPEN_PKGS_8M:= kmod-ath11k wpad-mesh hostapd-utils \
	wpa-cli qca-whc-lbd qca-whc-init libhyficommon \
	wififw_mount_script

WIFI_PKGS:=kmod-qca-wifi-unified-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-cfg80211tool qca-wifi-scripts \
	qca-acfg qca-wrapd athtestcmd-lith qca-iface-mgr \
	qca-wapid qca-lowi athdiag whc-mesh whc-ui \
	qca-spectral qca-icm qcmbr sigma-dut \
	qca-wpc qca-cfg80211 qca-cnss-daemon

WIFI_PKGS_MINENT:=kmod-qca-wifi-custc-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-spectral qca-wpc sigma-dut \
	qcmbr qca-wrapd qca-wapid qca-acfg \
	qca-lowi qca-icm qca-cfg80211 athdiag qca-cnss-daemon \
	athtestcmd-lith qca-cfg80211tool

WIFI_PKGS_256MB:=kmod-qca-wifi-lowmem-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-cfg80211tool qca-wifi-scripts \
	qca-wpc sigma-dut qca-wrapd qca-wapid qca-acfg \
	qca-iface-mgr qca-icm qca-cfg80211 athdiag qca-cnss-daemon \
	athtestcmd-lith whc-mesh
#	whc-ui \

WIFI_PKGS_16M:=kmod-qca-wifi-flash_16mb-profile \
	qca-hostap qca-hostapd-cli qca-wpa-supplicant \
	qca-wpa-cli qca-cfg80211 qca-cfg80211tool qca-wifi-scripts

WIFI_FW_PKGS:=qca-wifi-hk-fw-hw1-10.4-asic qca-wifi-cyp-fw-hw1-11.0-asic qca-wifi-wkk-fw-hw1-asic \
	qca-wifi-fw-hw3-10.4-asic qca-wifi-fw-hw6-10.4-asic qca-wifi-fw-hw4-10.4-asic

OPENWRT_STANDARD:= luci openssl-util

OPENWRT_256MB:=luci pm-utils wififw_mount_script qca-thermald qca-wlanfw-upgrade -file \
	-kmod-ata-core -kmod-ata-ahci -kmod-ata-ahci-platform \
	-kmod-usb2 -kmod-usb3 -kmod-usb-dwc3-qcom \
	-kmod-usb-phy-qcom-dwc3 -kmod-usb-dwc3-of-simple \
	-kmod-usb-phy-ipq807x -kmod-usb-f-qdss

STORAGE:=kmod-scsi-core kmod-usb-storage kmod-usb-uas kmod-nls-cp437 kmod-nls-iso8859-1 \
	kmod-fs-msdos kmod-fs-vfat kmod-fs-ntfs ntfs-3g e2fsprogs losetup

USB_ETHERNET:= kmod-usb-net-rtl8152 kmod-usb-net

TEST_TOOLS:=ethtool i2c-tools tcpdump

UTILS:=file luci-app-samba4 rng-tools profilerd

COREBSP_UTILS:=pm-utils wififw_mount_script qca-thermald qca-qmi-framework qca-time-services \
	qca-wlanfw-upgrade dashboard

FAILSAFE:= kmod-bootconfig

NETWORKING:=mcproxy -dnsmasq dnsmasq-dhcpv6 bridge ip-bridge ip-full mwan3 \
	rp-pppoe-relay iptables-mod-extra iputils-tracepath iputils-tracepath6 \
	luci-app-upnp luci-app-ddns luci-proto-ipv6 \
	kmod-nf-nathelper-extra kmod-nf-nathelper \
	kmod-ipt-nathelper-rtsp nftables kmod-nft-netdev \
	kmod-nft-offload kmod-bonding

NETWORKING_256MB:=-dnsmasq dnsmasq-dhcpv6 bridge ip-full \
	rp-pppoe-relay iptables-mod-extra iputils-tracepath iputils-tracepath6 \
	kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp \
	luci-app-upnp luci-app-ddns luci-proto-ipv6 \
	luci-app-multiwan

NETWORKING_8MB:=dnsmasq -dnsmasq-dhcpv6 kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp

NETWORKING_16MB:=-dnsmasq dnsmasq-dhcpv6 kmod-nf-nathelper-extra kmod-ipt-nathelper-rtsp ip \
	rp-pppoe-relay

CD_ROUTER:=kmod-ipt-ipopt kmod-bonding kmod-ipt-sctp lacpd \
	arptables ds-lite 6rd ddns-scripts xl2tpd \
	quagga quagga-ripd quagga-zebra quagga-watchquagga quagga-vtysh \
	kmod-ipv6 ip6tables iptables-mod-ipsec iptables-mod-filter \
	isc-dhcp-relay-ipv6 rp-pppoe-server ppp-mod-pptp iptables-mod-physdev

CD_ROUTER_256MB:=kmod-ipt-ipopt kmod-ipt-sctp lacpd \
	arptables ddns-scripts \
	quagga quagga-ripd quagga-zebra quagga-watchquagga quagga-vtysh \
	kmod-ipv6 ip6tables iptables-mod-filter \
	isc-dhcp-relay-ipv6 rp-pppoe-server iptables-mod-physdev

BLUETOOTH:=kmod-bluetooth bluez-libs bluez-utils kmod-ath3k

BLUETOPIA:=bluetopia

ZIGBEE:=zigbee_efr32

QOS:=tc kmod-sched kmod-sched-core kmod-sched-connmark kmod-ifb iptables \
	iptables-mod-filter iptables-mod-ipopt iptables-mod-conntrack-extra

MAP_PKGS:=map 464xlat tayga

HYFI:=hyfi-mesh hyfi-ui

QCA_MAD:=qca-mad

QCA_EZMESH:=qca-ezmesh qca-ezmesh-ctrl qca-ezmesh-agent qca-ezmesh-alg qca-ezmesh-agentalg

AQ_PHY:=kmod-aq_phy kmod-qca_85xx_sw aq-fw-download

#These packages depend on SWITCH_SSDK_PKGS
IGMPSNOOPING_RSTP:=rstp qca-mcs-apps

IPSEC:=openswan kmod-ipsec kmod-ipsec4 kmod-ipsec6

AUDIO:=kmod-sound-soc-ipq alsa

VIDEO:=kmod-qpic_panel_ertft

NSS_USERSPACE:=nlcfg

KPI:=iperf sysstat

CHAR_DIAG:=kmod-diag-char qca-diag

USB_DIAG:=kmod-diag-char kmod-usb-f-diag qca-diag kmod-usb-gdiag

CNSS_DIAG:=cnssdiag

FTM:=ftm

QMSCT_CLIENT:=qmsct_client

OPENVPN:= openvpn-easy-rsa openvpn-openssl luci-app-openvpn

MINIDUMP:= minidump

SWITCH_SSDK_NOHNAT_PKGS:= kmod-qca-ssdk-nohnat qca-ssdk-shell swconfig

QMI_SAMPLE_APP:=kmod-qmi_sample_client

MHI_QRTR:=kmod-mhi-qrtr-mproc

EMESH_SP:=kmod-emesh-sp

EXTRA_NETWORKING:= $(CD_ROUTER) $(NSS_EIP197_FW) -rdk-v-wifi-ath10k kmod-qca-nss-macsec \
	$(MACSEC_OPEN_PKGS) $(NSS_CRYPTO) $(NSS_CLIENTS_STANDARD)

define Profile/QSDK_Premium
	NAME:=Qualcomm Technologies, Inc SDK Premium Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(STORAGE) $(AUDIO) \
		$(VIDEO) $(TEST_TOOLS) $(COREBSP_UTILS) \
		$(AQ_PHY) $(FAILSAFE) -lacpd $(USB_DIAG) $(SWITCH_SSDK_PKGS) $(CNSS_DIAG) \
		$(FTM) $(QMSCT_CLIENT) $(KPI) $(NSS_COMMON) \
		$(NSS_STANDARD) $(UTILS) $(NETWORKING) $(CD_ROUTER) $(NSS_CLIENTS_STANDARD) \
		$(QCA_ECM_PREMIUM) $(NSS_CRYPTO) $(NSS_EIP197_FW) $(IGMPSNOOPING_RSTP) \
		$(WIFI_PKGS) $(WIFI_FW_PKGS) $(HW_CRYPTO) $(IPSEC) $(MAP_PKGS) $(MINIDUMP) \
		$(OPENVPN) $(QOS) $(HYFI) $(NSS_MACSEC) $(NSS_USERSPACE) $(NSS_RMNET) $(NSS_SFE) \
		$(QCA_MAD) $(EMESH_SP) $(QCA_EZMESH) kmod-macvlan
endef

#		$(QCA_RFS)

define Profile/QSDK_Premium/Description
	QSDK Premium package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_Premium))

define Profile/QSDK_Open
	NAME:=Qualcomm Technologies, Inc SDK Open Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(STORAGE) $(TEST_TOOLS) $(AUDIO) $(VIDEO) \
		$(COREBSP_UTILS) $(FAILSAFE) $(USB_DIAG) $(SWITCH_SSDK_NOHNAT_PKGS) \
		$(FTM) $(KPI) $(UTILS) $(NETWORKING) $(EXTRA_NETWORKING) \
		$(WIFI_OPEN_PKGS) $(USB_ETHERNET) $(NSS_COMMON) $(NSS_STANDARD) $(NSS_MESH) \
		$(QCA_ECM_PREMIUM) $(MAP_PKGS) $(IGMPSNOOPING_RSTP) $(IPSEC) $(QOS) -lacpd  \
		qca-cnss-daemon qca-wifi-hk-fw-hw1-10.4-asic $(CNSS_DIAG) athdiag $(EMESH_SP) \
		qrtr $(QMI_SAMPLE_APP) $(NSS_SFE) ath11k-fwtest ath11k-qdss libtirpc cfr_tools kmod-qca-ovsmgr
endef

#	$(HW_CRYPTO) $(QCA_RFS) \
#	$(AQ_PHY)

define Profile/QSDK_Open/Description
	QSDK Open package set configuration.
	Enables wifi open source packages
endef

$(eval $(call Profile,QSDK_Open))

define Profile/QSDK_QBuilder
	NAME:=Qualcomm Technologies, Inc SDK QBuilder Profile
	PACKAGES:=luci openssl-util kmod-qca-nss-dp kmod-qca-nss-drv -kmod-qca-nss-gmac \
		-qca-nss-fw2-retail qca-nss-fw-hk-retail qca-nss-fw-cp-retail qca-nss-fw-mp-retail \
		kmod-qca-ssdk-nohnat qca-ssdk-shell swconfig \
		kmod-scsi-core kmod-usb-storage kmod-usb-uas kmod-nls-cp437 kmod-nls-iso8859-1 kmod-fs-msdos \
		kmod-fs-vfat kmod-fs-ntfs ntfs-3g e2fsprogs losetup \
		kmod-qca-nss-sfe \
		rstp qca-mcs-apps qca-hostap qca-hostapd-cli qca-wpa-supplicant qca-wpa-cli \
		qca-spectral qca-wpc sigma-dut qcmbr qca-wrapd qca-wapid qca-acfg whc-mesh whc-ui \
		qca-lowi qca-iface-mgr qca-icm qca-cfg80211 athdiag qca-cnss-daemon athtestcmd-lith \
		qca-wifi-hk-fw-hw1-10.4-asic kmod-aq_phy kmod-qca_85xx_sw aq-fw-download mcproxy mwan3 \
		-dnsmasq dnsmasq-dhcpv6 bridge ip-full trace-cmd rp-pppoe-relay iptables-mod-extra \
		iputils-tracepath iputils-tracepath6 \
		kmod-nf-nathelper-extra kmod-nf-nathelper kmod-ipt-nathelper-rtsp luci-app-upnp \
		luci-app-ddns luci-proto-ipv6 luci-app-multiwan tc kmod-sched \
		kmod-sched-core kmod-sched-connmark kmod-ifb iptables kmod-pptp \
		iptables-mod-filter iptables-mod-ipopt iptables-mod-conntrack-extra \
		qca-nss-fw-eip-hk qca-nss-fw-eip-cp kmod-qca-ovsmgr \
		file luci-app-samba rng-tools profilerd ethtool i2c-tools tcpdump \
		pm-utils wififw_mount_script qca-thermald qca-qmi-framework qca-time-services \
		qca-wlanfw-upgrade dashboard iperf sysstat nlcfg kmod-bootconfig qca-cfg80211tool
endef

define Profile/QSDK_QBuilder/Description
	QSDK QBuilder package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_QBuilder))

define Profile/QSDK_Enterprise
	NAME:=Qualcomm Technologies, Inc SDK Enterprise Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(NSS_ENTERPRISE) $(SWITCH_SSDK_NOHNAT_PKGS) \
		$(WIFI_PKGS) $(WIFI_FW_PKGS) $(STORAGE) $(HW_CRYPTO) $(QCA_RFS) \
		$(IGMPSNOOPING_RSTP) $(NETWORKING) $(QOS) $(UTILS) $(TEST_TOOLS) $(COREBSP_UTILS) \
		$(QCA_ECM_ENTERPRISE) $(NSS_CLIENTS_ENTERPRISE) $(NSS_MACSEC) $(NSS_CRYPTO) \
		$(IPSEC) $(NSS_EIP197_FW) $(CD_ROUTER) $(AQ_PHY) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) -lacpd \
		$(USB_DIAG) $(MHI_QRTR) $(KPI) $(FAILSAFE) $(NSS_USERSPACE)
endef

define Profile/QSDK_Enterprise/Description
	QSDK Enterprise package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_Enterprise))

define Profile/QSDK_MinEnt
	NAME:=Qualcomm Technologies, Inc SDK MinEnt Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(NSS_COMMON) $(NSS_ENTERPRISE) $(SWITCH_SSDK_NOHNAT_PKGS) \
		$(WIFI_PKGS_MINENT) $(WIFI_FW_PKGS) $(STORAGE) $(HW_CRYPTO) $(QCA_RFS) \
		$(NETWORKING) $(QOS) $(UTILS) $(TEST_TOOLS) $(COREBSP_UTILS) \
		$(QCA_ECM_ENTERPRISE) $(NSS_CLIENTS_ENTERPRISE) $(NSS_MACSEC) $(NSS_CRYPTO) \
		$(IPSEC) $(NSS_EIP197_FW) $(CD_ROUTER) $(AQ_PHY) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) -lacpd \
		$(USB_DIAG) $(MHI_QRTR) $(KPI) $(FAILSAFE) $(NSS_USERSPACE)
endef

define Profile/QSDK_MinEnt/Description
	QSDK MinEnt package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_MinEnt))

define Profile/QSDK_256
	NAME:=Qualcomm Technologies, Inc SDK 256MB Profile
	PACKAGES:=$(OPENWRT_256MB) $(NSS_COMMON) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS_256MB) $(WIFI_FW_PKGS) $(CD_ROUTER_256MB) \
		$(NETWORKING_256MB) iperf rng-tools $(QCA_RFS) $(CHAR_DIAG) \
		$(QCA_ECM_STANDARD) $(NSS_MACSEC) $(NSS_CLIENTS_256MB) $(FAILSAFE) \
		-lacpd $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) $(HYFI) $(QCA_EZMESH) kmod-macvlan \
		$(IGMPSNOOPING_RSTP) $(EMESH_SP) e2fsprogs losetup
endef

#       $(MHI_QRTR)

define Profile/QSDK_256/Description
	QSDK Premium package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_256))

define Profile/QSDK_512
	NAME:=Qualcomm Technologies, Inc SDK 512MB Profile
	PACKAGES:=$(OPENWRT_STANDARD) $(AUDIO) $(NSS_COMMON) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS) $(WIFI_FW_PKGS) $(STORAGE) $(CD_ROUTER) \
		$(NETWORKING) $(OPENVPN) $(UTILS) $(NSS_SFE) $(HW_CRYPTO) $(QCA_RFS) \
		$(VIDEO) $(IGMPSNOOPING_RSTP) $(IPSEC) $(QOS) $(QCA_ECM_PREMIUM) \
		$(NSS_MACSEC) $(TEST_TOOLS) $(NSS_CRYPTO) $(NSS_CLIENTS_STANDARD) $(COREBSP_UTILS) \
		$(MAP_PKGS) $(AQ_PHY) $(FAILSAFE) -lacpd $(USB_DIAG) \
		$(NSS_EIP197_FW) $(CNSS_DIAG) $(FTM) $(QMSCT_CLIENT) $(KPI) $(NSS_USERSPACE) \
		$(NSS_RMNET) $(HYFI) $(EMESH_SP) $(QCA_EZMESH) kmod-macvlan
endef

#       $(MHI_QRTR)

define Profile/QSDK_512/Description
	QSDK Premium package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_512))

define Profile/QSDK_8M
	NAME:=Qualcomm Technologies, Inc SDK 8MB Flash Profile
	PACKAGES:=$(NSS_COMMON) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_OPEN_PKGS_8M) $(NETWORKING_8MB) \
		$(IGMPSNOOPING_RSTP) $(QCA_ECM_STANDARD) \
		$(NSS_CLIENTS_256MB) qrtr
endef

define Profile/QSDK_8M/Description
	QSDK 8M package set configuration.
	Enables wifi open source packages
endef

$(eval $(call Profile,QSDK_8M))

define Profile/QSDK_16M
	NAME:=Qualcomm Technologies, Inc SDK 16MB Flash Profile
	PACKAGES:=wififw_mount_script $(NSS_COMMON) $(NSS_STANDARD) $(SWITCH_SSDK_PKGS) \
		$(WIFI_PKGS_16M) qca-wifi-hk-fw-hw1-10.4-asic $(NETWORKING_16MB) \
		$(IGMPSNOOPING_RSTP) $(QCA_ECM_STANDARD) $(NSS_CLIENTS_256MB) \
		$(MHI_QRTR) xz xz-utils -kmod-usb-f-qdss \
		-kmod-testssr -kmod-ata-core -kmod-ata-ahci -kmod-ata-ahci-platform \
		-kmod-usb2 -kmod-usb3 -kmod-usb-phy-ipq5018 -kmod-usb-dwc3-qcom \
		-kmod-bt_tty -kmod-clk-test -sysupgrade-helper \
		-fwupgrade-tools -urandom-seed -urngd
endef

define Profile/QSDK_16M/Description
	QSDK 16M package set configuration.
	Enables qca-wifi 11.0 packages
endef

$(eval $(call Profile,QSDK_16M))
