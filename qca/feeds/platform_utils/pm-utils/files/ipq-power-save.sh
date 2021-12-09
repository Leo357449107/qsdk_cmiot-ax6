#!/bin/sh

#
# Copyright (c) 2015-2016, 2019, The Linux Foundation. All rights reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

[ -e /lib/ipq806x.sh ] && . /lib/ipq806x.sh

type ipq806x_board_name &>/dev/null  || ipq806x_board_name() {
	echo $(board_name) | sed 's/^\([^-]*-\)\{1\}//g'
}

. /lib/functions.sh

ipq8064_ac_power()
{
	echo "Entering AC-Power Mode"
# Krait Power-UP Sequence
	/etc/init.d/powerctl restart

# Clocks Power-UP Sequence
	echo 400000000 > /sys/kernel/debug/clk/afab_a_clk/rate
	echo 64000000 > /sys/kernel/debug/clk/dfab_a_clk/rate
	echo 64000000 > /sys/kernel/debug/clk/sfpb_a_clk/rate
	echo 64000000 > /sys/kernel/debug/clk/cfpb_a_clk/rate
	echo 133000000 > /sys/kernel/debug/clk/nssfab0_a_clk/rate
	echo 133000000 > /sys/kernel/debug/clk/nssfab1_a_clk/rate
	echo 400000000 > /sys/kernel/debug/clk/ebi1_a_clk/rate

# Enabling Auto scale on NSS cores
	echo 1 > /proc/sys/dev/nss/clock/auto_scale

# PCIe Power-UP Sequence
	sleep 1
	echo 1 > /sys/bus/pci/rcrescan
	sleep 2
	echo 1 > /sys/bus/pci/rescan

	sleep 1

# Wifi Power-up Sequence
	wifi up

# Bringing Up LAN Interface
	ifup lan

# Sata Power-UP Sequence
	[ -f /sys/devices/platform/msm_sata.0/ahci.0/msm_sata_suspend ] && {
		echo 0 > /sys/devices/platform/msm_sata.0/ahci.0/msm_sata_suspend
	}
	[ -f sys/devices/soc.2/29000000.sata/ipq_ahci_suspend ] && {
		echo 0 > sys/devices/soc.2/29000000.sata/ipq_ahci_suspend
	}
	[ -f /sys/devices/platform/soc/29000000.sata/ipq_ahci_suspend ] && {
		echo 0 > /sys/devices/platform/soc/29000000.sata/ipq_ahci_suspend
	}

	sleep 1
	echo "- - -" > /sys/class/scsi_host/host0/scan

# USB Power-UP Sequence
	[ -d /sys/module/dwc3_ipq ] || insmod dwc3-ipq
	[ -d /sys/module/dwc3_qcom ] || insmod dwc3-qcom
	[ -d /sys/module/phy_qcom_hsusb ] || insmod phy-qcom-hsusb
	[ -d /sys/module/phy_qcom_ssusb ] || insmod phy-qcom-ssusb
	[ -d /sys/module/dwc3_of_simple ] || insmod dwc3-of-simple.ko
	[ -d /sys/module/phy_qcom_dwc3 ] || insmod phy-qcom-dwc3.ko
	[ -d /sys/module/dwc3 ] || insmod dwc3

# SD/MMC Power-UP sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		if [[ -f /tmp/sysinfo/sd_drvname  && ! -d /sys/block/mmcblk0 ]]
		then
			sd_drvname=$(cat /tmp/sysinfo/sd_drvname)
			echo $sd_drvname > /sys/bus/amba/drivers/mmci-pl18x/bind
		fi
	fi

	exit 0
}

ipq8064_battery_power()
{
	echo "Entering Battery Mode..."

# Wifi Power-down Sequence
	wifi unload

# Bring Down LAN Interface
	ifdown lan

# PCIe Power-Down Sequence

# Remove devices
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		v=`cat ${d}/vendor`
		[ "xx${v}" != "xx0x17cb" ] && echo 1 > ${d}/remove
	done

# Remove Buses
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		echo 1 > ${d}/remove
	done

# Remove RC
	sleep 2

	[ -f /sys/bus/pci/rcremove ] && {
		echo 1 > /sys/bus/pci/rcremove
	}
	[ -f /sys/devices/pci0000:00/pci_bus/0000:00/rcremove ] && {
		echo 1 > /sys/devices/pci0000:00/pci_bus/0000:00/rcremove
	}
	sleep 1

# Find scsi devices and remove it

	partition=`cat /proc/partitions | awk -F " " '{print $4}'`

	for entry in $partition; do
		sd_entry=$(echo $entry | head -c 2)

		if [ "$sd_entry" = "sd" ]; then
			[ -f /sys/block/$entry/device/delete ] && {
				echo 1 > /sys/block/$entry/device/delete
			}
		fi
	done

# Sata Power-Down Sequence
	[ -f /sys/devices/platform/msm_sata.0/ahci.0/msm_sata_suspend ] && {
		echo 1 > /sys/devices/platform/msm_sata.0/ahci.0/msm_sata_suspend
	}
	[ -f /sys/devices/soc.2/29000000.sata/ipq_ahci_suspend ] && {
		echo 1 > /sys/devices/soc.2/29000000.sata/ipq_ahci_suspend
	}
	[ -f /sys/devices/platform/soc/29000000.sata/ipq_ahci_suspend ] && {
		echo 1 > /sys/devices/platform/soc/29000000.sata/ipq_ahci_suspend
	}

# USB Power-down Sequence
	[ -d /sys/module/dwc3_ipq ] && rmmod dwc3-ipq
	[ -d /sys/module/dwc3 ] && rmmod dwc3
	[ -d /sys/module/dwc3_qcom ] && rmmod dwc3-qcom
	[ -d /sys/module/phy_qcom_hsusb ] && rmmod phy-qcom-hsusb
	[ -d /sys/module/phy_qcom_ssusb ] && rmmod phy-qcom-ssusb
	[ -d /sys/module/dwc3_of_simple ] && rmmod dwc3-of-simple.ko
	[ -d /sys/module/phy_qcom_dwc3 ] && rmmod phy-qcom-dwc3.ko

	sleep 1

#SD/MMC Power-down Sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		if [ -d /sys/block/mmcblk0 ]
		then
			sd_drvname=`readlink /sys/block/mmcblk0 | awk -F "/" '{print $5}'`
			echo "$sd_drvname" > /tmp/sysinfo/sd_drvname
			echo $sd_drvname > /sys/bus/amba/drivers/mmci-pl18x/unbind
		fi
	fi

# Disabling Auto scale on NSS cores
	echo 0 > /proc/sys/dev/nss/clock/auto_scale

# Clocks Power-down Sequence

	echo 400000000 > /sys/kernel/debug/clk/afab_a_clk/rate
	echo 32000000 > /sys/kernel/debug/clk/dfab_a_clk/rate
	echo 32000000 > /sys/kernel/debug/clk/sfpb_a_clk/rate
	echo 32000000 > /sys/kernel/debug/clk/cfpb_a_clk/rate
	echo 133000000 > /sys/kernel/debug/clk/nssfab0_a_clk/rate
	echo 133000000 > /sys/kernel/debug/clk/nssfab1_a_clk/rate
	echo 400000000 > /sys/kernel/debug/clk/ebi1_a_clk/rate

# Scaling Down UBI Cores
	echo 110000000 > /proc/sys/dev/nss/clock/current_freq

# Krait Power-down Sequence
	echo 384000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
	echo 384000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_min_freq
	echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
	echo "powersave" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
}

ipq4019_ap_dk01_1_ac_power()
{
	echo "Entering AC-Power Mode"
# Cortex Power-UP Sequence
	/etc/init.d/powerctl restart

# Power on Malibu PHY of LAN ports
	ssdk_sh port poweron set 1
	ssdk_sh port poweron set 2
	ssdk_sh port poweron set 3
	ssdk_sh port poweron set 4
# Wifi Power-up Sequence
	wifi up

# USB Power-UP Sequence
	if ! [ -d /sys/module/dwc3_ipq40xx -o -d /sys/module/dwc3_of_simple ]
	then
		insmod phy-qca-baldur.ko
		insmod phy-qca-uniphy.ko
		if [ -e /lib/modules/$(uname -r)/dwc3-of-simple.ko ]
		then
			insmod dwc3-of-simple.ko
		else
			insmod dwc3-ipq40xx.ko
		fi
		insmod dwc3.ko
	fi

# LAN interface up
	ifup lan

	exit 0
}

ipq4019_ap_dk01_1_battery_power()
{
	echo "Entering Battery Mode..."

# Wifi Power-down Sequence
	wifi unload

# Find scsi devices and remove it

	partition=`cat /proc/partitions | awk -F " " '{print $4}'`

	for entry in $partition; do
		sd_entry=$(echo $entry | head -c 2)

		if [ "$sd_entry" = "sd" ]; then
			[ -f /sys/block/$entry/device/delete ] && {
				echo 1 > /sys/block/$entry/device/delete
			}
		fi
	done

# Power off Malibu PHY of LAN ports
	ssdk_sh port poweroff set 1
	ssdk_sh port poweroff set 2
	ssdk_sh port poweroff set 3
	ssdk_sh port poweroff set 4

# USB Power-down Sequence
	if [ -d /sys/module/dwc3_ipq40xx -o -d /sys/module/dwc3_of_simple ]
	then
		rmmod dwc3
		if [ -d /sys/module/dwc3_ipq40xx ]
		then
			rmmod dwc3-ipq40xx
		else
			rmmod dwc3-of-simple
		fi
		rmmod phy-qca-uniphy
		rmmod phy-qca-baldur
	fi
	sleep 2

# LAN interface down
	ifdown lan

# Cortex Power-down Sequence
	echo 48000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
	echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
}

ipq4019_ap_dk04_1_ac_power()
{
	echo "Entering AC-Power Mode"
# Cortex Power-UP Sequence
	/etc/init.d/powerctl restart

# Power on Malibu PHY of LAN ports
	ssdk_sh port poweron set 1
	ssdk_sh port poweron set 2
	ssdk_sh port poweron set 3
	ssdk_sh port poweron set 4

# PCIe Power-UP Sequence
	sleep 1
	echo 1 > /sys/bus/pci/rcrescan
	sleep 2
	echo 1 > /sys/bus/pci/rescan

	sleep 1

# Wifi Power-up Sequence
	wifi up

# USB Power-UP Sequence
	if ! [ -d /sys/module/dwc3_ipq40xx -o -d /sys/module/dwc3_of_simple ]
	then
		insmod phy-qca-baldur.ko
		insmod phy-qca-uniphy.ko
		if [ -e /lib/modules/$(uname -r)/dwc3-of-simple.ko ]
		then
			insmod dwc3-of-simple.ko
		else
			insmod dwc3-ipq40xx.ko
		fi
		insmod dwc3.ko
	fi

# LAN interface up
	ifup lan

# SD/MMC Power-UP sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		if [[ -f /tmp/sysinfo/sd_drvname  && ! -d /sys/block/mmcblk0 ]]
		then
			sd_drvname=$(cat /tmp/sysinfo/sd_drvname)
			echo $sd_drvname > /sys/bus/platform/drivers/sdhci_msm/bind
		fi
	fi

	sleep 1

	exit 0
}

ipq4019_ap_dk04_1_battery_power()
{
	echo "Entering Battery Mode..."

# Wifi Power-down Sequence
	wifi unload

# PCIe Power-Down Sequence

# Remove devices
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		v=`cat ${d}/vendor`
		[ "xx${v}" != "xx0x17cb" ] && echo 1 > ${d}/remove
	done

# Remove Buses
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		echo 1 > ${d}/remove
	done

# Remove RC
	sleep 2

	[ -f /sys/bus/pci/rcremove ] && {
		echo 1 > /sys/bus/pci/rcremove
	}
	[ -f /sys/devices/pci0000:00/pci_bus/0000:00/rcremove ] && {
		echo 1 > /sys/devices/pci0000:00/pci_bus/0000:00/rcremove
	}
	sleep 1

# Find scsi devices and remove it

	partition=`cat /proc/partitions | awk -F " " '{print $4}'`

	for entry in $partition; do
		sd_entry=$(echo $entry | head -c 2)

		if [ "$sd_entry" = "sd" ]; then
			[ -f /sys/block/$entry/device/delete ] && {
				echo 1 > /sys/block/$entry/device/delete
			}
		fi
	done

# Power off Malibu PHY of LAN ports
	ssdk_sh port poweroff set 1
	ssdk_sh port poweroff set 2
	ssdk_sh port poweroff set 3
	ssdk_sh port poweroff set 4

# USB Power-down Sequence
	if [ -d /sys/module/dwc3_ipq40xx -o -d /sys/module/dwc3_of_simple ]
	then
		rmmod dwc3
		if [ -d /sys/module/dwc3_ipq40xx ]
		then
			rmmod dwc3-ipq40xx
		else
			rmmod dwc3-of-simple
		fi
		rmmod phy-qca-uniphy
		rmmod phy-qca-baldur
	fi
	sleep 2
#SD/MMC Power-down Sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		if [ -d /sys/block/mmcblk0 ]
		then
			sd_drvname=`readlink /sys/block/mmcblk0 | grep -o "[0-9]*.sdhci"`
			echo "$sd_drvname" > /tmp/sysinfo/sd_drvname
			echo $sd_drvname > /sys/bus/platform/drivers/sdhci_msm/unbind
		fi
	fi

# LAN interface down
	ifdown lan

# Cortex Power-down Sequence
	echo 48000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq
	echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
}

ipq5018_phy_power_on()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-mp03.1 | ap-mp03.1-c2 | db-mp03.1)
		echo 1 > /sys/ssdk/dev_id
		ssdk_sh port poweron set 1
		ssdk_sh port poweron set 2
		ssdk_sh port poweron set 3
		ssdk_sh port poweron set 4
		echo 0 > /sys/ssdk/dev_id
		;;
		ap-mp02.1 | ap-mp03.3 | ap-mp03.3-c2 | ap-mp03.3-c3 | ap-mp03.3-c4 | ap-mp03.3-c5 | ap-mp03.5-c1 | ap-mp03.5-c2 | ap-mp03.6-c1 | ap-mp03.6-c2 | db-mp02.1 | db-mp03.3 | db-mp03.3-c2)
		echo 0 > /sys/ssdk/dev_id
		ssdk_sh port poweron set 2
		;;
	esac
}

ipq5018_phy_power_off()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-mp03.1 | ap-mp03.1-c2 | db-mp03.1)
		echo 1 > /sys/ssdk/dev_id
		ssdk_sh port poweroff set 1
		ssdk_sh port poweroff set 2
		ssdk_sh port poweroff set 3
		ssdk_sh port poweroff set 4
		echo 0 > /sys/ssdk/dev_id
		;;
		ap-mp02.1 | ap-mp03.3 | ap-mp03.3-c2 | ap-mp03.3-c3 | ap-mp03.3-c4 | ap-mp03.3-c5 | ap-mp03.5-c1 | ap-mp03.5-c2 | ap-mp03.6-c1 | ap-mp03.6-c2 | db-mp02.1 | db-mp03.3 | db-mp03.3-c2)
		echo 0 > /sys/ssdk/dev_id
		ssdk_sh port poweroff set 2
		;;
	esac
}

ipq6018_phy_power_on()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-cp01-c1 | ap-cp01-c2 | ap-cp01-c3 | ap-cp01-c4 | ap-cp01-c5 | db-cp01)
		ssdk_sh port poweron set 2
		ssdk_sh port poweron set 3
		ssdk_sh port poweron set 4
		ssdk_sh port poweron set 5
		;;
		ap-cp02-c1 | db-cp02)
		ssdk_sh port poweron set 5
		;;
		ap-cp03-c1)
		ssdk_sh port poweron set 4
		;;
	esac
}

ipq6018_phy_power_off()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-cp01-c1 | ap-cp01-c2 | ap-cp01-c3 | ap-cp01-c4 | ap-cp01-c5 | db-cp01)
		ssdk_sh port poweroff set 2
		ssdk_sh port poweroff set 3
		ssdk_sh port poweroff set 4
		ssdk_sh port poweroff set 5
		;;
		ap-cp02-c1 | db-cp02)
		ssdk_sh port poweroff set 5
		;;
		ap-cp03-c1)
		ssdk_sh port poweroff set 4
		;;
	esac
}

ipq5018_ac_power()
{
	echo "Entering AC-Power Mode"
# Cortex Power-UP Sequence
	/etc/init.d/powerctl restart

# Enabling Auto scale on NSS cores
	echo 1 > /proc/sys/dev/nss/clock/auto_scale

# Power on PHYs of LAN ports
	ipq5018_phy_power_on
# PCIe Power-UP Sequence
	sleep 1
	echo 1 > /sys/bus/pci/rcrescan
	sleep 2
	echo 1 > /sys/bus/pci/rescan

	sleep 1

# USB Power-UP Sequence
	if ! [ -d /sys/module/dwc3_qcom ]
	then
		insmod phy-qca-uniphy.ko
		insmod phy-qca-m31.ko
		insmod dwc3.ko
		insmod dwc3-qcom.ko
		insmod u_qdss.ko
		insmod usb_f_qdss.ko
	fi

	if [ -d config/usb_gadget/g1 ]
	then
		echo "8a00000.dwc3" > /config/usb_gadget/g1/UDC
	fi
# LAN interface up
	ifup lan

# Wifi Power-up Sequence
	if [ -f /lib/modules/$(uname -r)/ath11k.ko ]; then
		insmod ath11k
		insmod ath11k_ahb
		insmod ath11k_pci
		sleep 2
		wifi up
	else
		wifi load
	fi

# SD/MMC Power-UP sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		for sd_drvname in $(cat /tmp/sysinfo/sd_drvname)
		do
			echo $sd_drvname > /sys/bus/platform/drivers/sdhci_msm/bind
		done
	fi

	sleep 1

	exit 0
}

ipq5018_battery_power()
{
	echo "Entering Battery Mode..."

# Wifi Power-down Sequence
	lsmod | grep ath11k > /dev/null
	if [ $? -eq 0 ]; then
		wifi down
		sleep 2
		rmmod ath11k_pci
		rmmod ath11k_ahb
		rmmod ath11k
	else
		wifi unload
	fi

# PCIe Power-Down Sequence

# Remove devices
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		v=`cat ${d}/vendor`
		[ "xx${v}" != "xx0x17cb" ] && echo 1 > ${d}/remove
	done

# Remove Buses
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		echo 1 > ${d}/remove
	done

# Remove RC
	sleep 2

	[ -f /sys/bus/pci/rcremove ] && {
		echo 1 > /sys/bus/pci/rcremove
	}
	[ -f /sys/devices/pci0000:00/pci_bus/0000:00/rcremove ] && {
		echo 1 > /sys/devices/pci0000:00/pci_bus/0000:00/rcremove
	}
	sleep 1

# Find scsi devices and remove it
	partition=`cat /proc/partitions | awk -F " " '{print $4}'`

	for entry in $partition; do
		sd_entry=$(echo $entry | head -c 2)

		if [ "$sd_entry" = "sd" ]; then
			[ -f /sys/block/$entry/device/delete ] && {
				echo 1 > /sys/block/$entry/device/delete
			}
		fi
	done

# Power off PHYs of LAN ports
	ipq5018_phy_power_off

# USB Power-down Sequence
	if [ -d config/usb_gadget/g1 ]
	then
		echo "" > /config/usb_gadget/g1/UDC
	fi

	if [ -d /sys/module/dwc3_qcom ]
	then
		rmmod usb_f_qdss
		rmmod u_qdss
		rmmod dwc3-qcom
		rmmod dwc3
		rmmod phy_qca_m31
		rmmod phy_qca_uniphy
	fi
	sleep 2

#SD/MMC Power-down Sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		rm /tmp/sysinfo/sd_drvname
		if [ -d /sys/block/mmcblk0 ]; then
			sd_drvname=`readlink /sys/block/mmcblk0 | grep -o "[0-9]*.sdhci[^/]*"`
			echo "$sd_drvname" >> /tmp/sysinfo/sd_drvname
			echo $sd_drvname >> /sys/bus/platform/drivers/sdhci_msm/unbind
		fi
	fi
# LAN interface down
	ifdown lan

# Disabling Auto scale on NSS cores
	echo 0 > /proc/sys/dev/nss/clock/auto_scale

# Cortex Power-down Sequence
	echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
}

ipq6018_ac_power()
{
	echo "Entering AC-Power Mode"
# Cortex Power-UP Sequence
	/etc/init.d/powerctl restart

# Enabling Auto scale on NSS cores
	echo 1 > /proc/sys/dev/nss/clock/auto_scale

# Power on PHYs of LAN ports
	ipq6018_phy_power_on
# PCIe Power-UP Sequence
	sleep 1
	echo 1 > /sys/bus/pci/rcrescan
	sleep 2
	echo 1 > /sys/bus/pci/rescan

	sleep 1

# USB Power-UP Sequence
	if ! [ -d /sys/module/dwc3_qcom ]
	then
		insmod phy-msm-ssusb-qmp.ko
		insmod phy-msm-qusb.ko
		insmod dwc3.ko
		insmod dwc3-qcom.ko
		insmod u_qdss.ko
		insmod usb_f_qdss.ko
	fi

	if [ -d config/usb_gadget/g1 ]
	then
		echo "8a00000.dwc3" > /config/usb_gadget/g1/UDC
	fi
# LAN interface up
	ifup lan

# Wifi Power-up Sequence
	if [ -f /lib/modules/$(uname -r)/ath11k.ko ]; then
		insmod ath11k
		insmod ath11k_ahb
		insmod ath11k_pci
		sleep 2
		wifi up
	else
		wifi load
	fi

# SD/MMC Power-UP sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		for sd_drvname in $(cat /tmp/sysinfo/sd_drvname)
		do
			echo $sd_drvname > /sys/bus/platform/drivers/sdhci_msm/bind
		done
	fi

	sleep 1

	exit 0
}

ipq6018_battery_power()
{
	echo "Entering Battery Mode..."

# Wifi Power-down Sequence
	lsmod | grep ath11k > /dev/null
	if [ $? -eq 0 ]; then
		wifi down
		sleep 2
		rmmod ath11k_pci
		rmmod ath11k_ahb
		rmmod ath11k
	else
		wifi unload
	fi

# PCIe Power-Down Sequence

# Remove devices
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		v=`cat ${d}/vendor`
		[ "xx${v}" != "xx0x17cb" ] && echo 1 > ${d}/remove
	done

# Remove Buses
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		echo 1 > ${d}/remove
	done

# Remove RC
	sleep 2

	[ -f /sys/bus/pci/rcremove ] && {
		echo 1 > /sys/bus/pci/rcremove
	}
	[ -f /sys/devices/pci0000:00/pci_bus/0000:00/rcremove ] && {
		echo 1 > /sys/devices/pci0000:00/pci_bus/0000:00/rcremove
	}
	sleep 1

# Find scsi devices and remove it
	partition=`cat /proc/partitions | awk -F " " '{print $4}'`

	for entry in $partition; do
		sd_entry=$(echo $entry | head -c 2)

		if [ "$sd_entry" = "sd" ]; then
			[ -f /sys/block/$entry/device/delete ] && {
				echo 1 > /sys/block/$entry/device/delete
			}
		fi
	done

# Power off PHYs of LAN ports
	ipq6018_phy_power_off

# USB Power-down Sequence
	if [ -d config/usb_gadget/g1 ]
	then
		echo "" > /config/usb_gadget/g1/UDC
	fi

	if [ -d /sys/module/dwc3_qcom ]
	then
		rmmod usb_f_qdss
		rmmod u_qdss
		rmmod dwc3-qcom
		rmmod dwc3
		rmmod phy_msm_qusb
		rmmod phy_msm_ssusb_qmp
	fi
	sleep 2

#SD/MMC Power-down Sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		rm /tmp/sysinfo/sd_drvname
		if [ -d /sys/block/mmcblk0 ]; then
			sd_drvname=`readlink /sys/block/mmcblk0 | grep -o "[0-9]*.sdhci[^/]*"`
			echo "$sd_drvname" >> /tmp/sysinfo/sd_drvname
			echo $sd_drvname >> /sys/bus/platform/drivers/sdhci_msm/unbind
		fi
	fi
# LAN interface down
	ifdown lan

# Disabling Auto scale on NSS cores
	echo 0 > /proc/sys/dev/nss/clock/auto_scale

# Scaling Down UBI Cores
	echo 187200000 > /proc/sys/dev/nss/clock/current_freq

# Cortex Power-down Sequence
	echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
}

ipq9574_phy_power_on()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-al01-c1 | ap-al02-c1 | ap-al02-c2 | db-al01-c1 | db-al01-c2 | db-al01-c3 |\
			db-al02-c1 | db-al02-c2)
			ssdk_sh port poweron set 2
			ssdk_sh port poweron set 3
			ssdk_sh port poweron set 4
			ssdk_sh port poweron set 5
			ssdk_sh port poweron set 6
		;;
		db-al02-c3)
			ssdk_sh port poweron set 2
			ssdk_sh port poweron set 3
			ssdk_sh port poweron set 4
			ssdk_sh port poweron set 6
		;;
	esac
}

ipq9574_phy_power_off()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-al01-c1 | ap-al02-c1 | ap-al02-c2 | db-al01-c1 | db-al01-c2 | db-al01-c3 |\
			db-al02-c1 | db-al02-c2)
			ssdk_sh port poweroff set 2
			ssdk_sh port poweroff set 3
			ssdk_sh port poweroff set 4
			ssdk_sh port poweroff set 5
			ssdk_sh port poweroff set 6
		;;
		db-al02-c3)
			ssdk_sh port poweroff set 2
			ssdk_sh port poweroff set 3
			ssdk_sh port poweroff set 4
			ssdk_sh port poweroff set 6
		;;
	esac
}

ipq9574_ac_power()
{
	echo "Entering AC-Power Mode"
# Cortex Power-UP Sequence
	/etc/init.d/powerctl restart

# Enabling Auto scale on NSS cores
	echo 1 > /proc/sys/dev/nss/clock/auto_scale

# Power on PHYs of LAN ports
	ipq9574_phy_power_on
# PCIe Power-UP Sequence
	sleep 1
	echo 1 > /sys/bus/pci/rcrescan
	sleep 2

# USB Power-UP Sequence
	if [ -e /lib/modules/$(uname -r)/dwc3-qcom.ko ]
	then
		insmod phy-qcom-qusb2.ko
		insmod dwc3-qcom.ko
		insmod dwc3.ko
		insmod usb_f_qdss.ko
	fi

	if [ -d config/usb_gadget/g1 ]
	then
		echo "8a00000.dwc3" > /config/usb_gadget/g1/UDC
	fi

# LAN interface up
	ifup lan

# Wifi Power-up Sequence
	if [ -f /lib/modules/$(uname -r)/ath11k.ko ]; then
		insmod ath11k
		insmod ath11k_ahb
		insmod ath11k_pci
		sleep 2
		wifi up
	else
		wifi load
	fi

# SD/MMC Power-UP sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		for sd_drvname in $(cat /tmp/sysinfo/sd_drvname)
		do
			echo $sd_drvname > /sys/bus/platform/drivers/sdhci_msm/bind
		done
	fi

	sleep 1

	exit 0
}

ipq9574_battery_power()
{
	echo "Entering Battery Mode..."

# Wifi Power-down Sequence
	lsmod | grep ath11k > /dev/null
	if [ $? -eq 0 ]; then
		wifi down
		sleep 2
		rmmod ath11k_pci
		rmmod ath11k_ahb
		rmmod ath11k
	else
		wifi unload
	fi

# PCIe Power-Down Sequence

	[ -f /sys/bus/pci/rcremove ] && {
		echo 1 > /sys/bus/pci/rcremove
	}
	sleep 1

# Find scsi devices and remove it
	partition=`cat /proc/partitions | awk -F " " '{print $4}'`

	for entry in $partition; do
		sd_entry=$(echo $entry | head -c 2)

		if [ "$sd_entry" = "sd" ]; then
			[ -f /sys/block/$entry/device/delete ] && {
				echo 1 > /sys/block/$entry/device/delete
			}
		fi
	done


# Power off PHYs of LAN ports
        ipq9574_phy_power_off
# USB Power-down Sequence
	if [ -d config/usb_gadget/g1 ]
	then
		echo "" > /config/usb_gadget/g1/UDC
	fi

	if [ -d /sys/module/dwc3_qcom ]
	then
		rmmod usb_f_qdss
		rmmod dwc3
		rmmod dwc3_qcom
		rmmod phy_qcom_qusb2
	fi
	sleep 2

#SD/MMC Power-down Sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		rm /tmp/sysinfo/sd_drvname
		if [ -d /sys/block/mmcblk0 ]; then
			sd_drvname=`readlink /sys/block/mmcblk0 | grep -o "[0-9]*.sdhci[^/]*"`
			echo "$sd_drvname" >> /tmp/sysinfo/sd_drvname
			echo $sd_drvname >> /sys/bus/platform/drivers/sdhci_msm/unbind
		fi
	fi

# LAN interface down
	ifdown lan

# Disabling Auto scale on NSS cores
	echo 0 > /proc/sys/dev/nss/clock/auto_scale

# Scaling Down UBI Cores
	echo 1500000000 > /proc/sys/dev/nss/clock/current_freq;

# Cortex Power-down Sequence
	echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

}

ipq8074_phy_power_on()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-hk01-c1 | ap-hk01-c3 | ap-hk01-c4 | ap-hk01-c5 | ap-hk01-c6 | ap-hk07 |\
		ap-hk09 | ap-hk10-c1 | ap-hk10-c2 | ap-hk11-c1 | ap-hk12 | ap-ac01 | ap-ac02 | ap-oak03 | db-hk01 | db-hk02)
		ssdk_sh port poweron set 2
		ssdk_sh port poweron set 3
		ssdk_sh port poweron set 4
		ssdk_sh port poweron set 5
		ssdk_sh port poweron set 6
		;;
		ap-hk01-c2 | ap-oak02 | ap-hk14)
		ssdk_sh port poweron set 2
		ssdk_sh port poweron set 3
		ssdk_sh port poweron set 4
		ssdk_sh port poweron set 6
		;;
		ap-hk02 | ap-hk08)
		ssdk_sh port poweron set 5
		ssdk_sh port poweron set 6
		;;
		ap-ac03 | ap-ac04)
		ssdk_sh port poweron set 2
		ssdk_sh port poweron set 3
		ssdk_sh port poweron set 4
		ssdk_sh port poweron set 5
		;;
	esac
}

ipq8074_phy_power_off()
{
	local board=$(ipq806x_board_name)
	case "$board" in
		ap-hk01-c1 | ap-hk01-c3 | ap-hk01-c4 | ap-hk01-c5 | ap-hk01-c6 | ap-hk07 |\
		ap-hk09 | ap-hk10-c1 | ap-hk10-c2 | ap-hk11-c1 | ap-hk12 | ap-ac01 | ap-ac02 | ap-oak03 | db-hk01 | db-hk02)
		ssdk_sh port poweroff set 2
		ssdk_sh port poweroff set 3
		ssdk_sh port poweroff set 4
		ssdk_sh port poweroff set 5
		ssdk_sh port poweroff set 6
		;;
		ap-hk01-c2 | ap-oak02 | ap-hk14)
		ssdk_sh port poweroff set 2
		ssdk_sh port poweroff set 3
		ssdk_sh port poweroff set 4
		ssdk_sh port poweroff set 6
		;;
		ap-hk02 | ap-hk08)
		ssdk_sh port poweroff set 5
		ssdk_sh port poweroff set 6
		;;
		ap-ac03 | ap-ac04)
		ssdk_sh port poweroff set 2
		ssdk_sh port poweroff set 3
		ssdk_sh port poweroff set 4
		ssdk_sh port poweroff set 5
		;;
	esac
}

ipq8074_ac_power()
{
	echo "Entering AC-Power Mode"
# Cortex Power-UP Sequence
	/etc/init.d/powerctl restart

# Enabling Auto scale on NSS cores
	echo 1 > /proc/sys/dev/nss/clock/auto_scale

# Power on Malibu PHY of LAN ports
	ipq8074_phy_power_on
# PCIe Power-UP Sequence
	sleep 1
	echo 1 > /sys/bus/pci/rcrescan
	sleep 2
	echo 1 > /sys/bus/pci/rescan

	sleep 1

# USB Power-UP Sequence
	if [ -e /lib/modules/$(uname -r)/dwc3-of-simple.ko ]
	then
		insmod phy-msm-ssusb-qmp.ko
		insmod phy-msm-qusb.ko
		insmod dbm
		insmod dwc3-of-simple.ko
		insmod dwc3.ko
	        insmod u_qdss
	        insmod usb_f_qdss
	elif [ -e /lib/modules/$(uname -r)/dwc3-qcom.ko ]
	then
		insmod phy-qcom-qusb2.ko
		insmod dwc3-qcom.ko
		insmod dwc3.ko
		insmod usb_f_qdss.ko
	fi

	if [ -d config/usb_gadget/g1 ]
	then
		echo "8a00000.dwc3" > /config/usb_gadget/g1/UDC
	fi

# LAN interface up
	ifup lan

# Wifi Power-up Sequence
	if [ -f /lib/modules/$(uname -r)/ath11k.ko ]; then
		insmod ath11k
		insmod ath11k_ahb
		insmod ath11k_pci
		sleep 2
		wifi up
	else
		wifi load
	fi

# SD/MMC Power-UP sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		for sd_drvname in $(cat /tmp/sysinfo/sd_drvname)
		do
			echo $sd_drvname > /sys/bus/platform/drivers/sdhci_msm/bind
		done
	fi

	if [ -f /tmp/sysinfo/sd1_drvname ]
	then
		sd1_drvname=$(cat /tmp/sysinfo/sd1_drvname)
		echo $sd1_drvname > /sys/bus/platform/drivers/sdhci_msm/bind
	fi

	sleep 1

	exit 0
}

ipq8074_battery_power()
{
	echo "Entering Battery Mode..."

# Wifi Power-down Sequence
	lsmod | grep ath11k > /dev/null
	if [ $? -eq 0 ]; then
		wifi down
		sleep 2
		rmmod ath11k_pci
		rmmod ath11k_ahb
		rmmod ath11k
	else
		wifi unload
	fi

# PCIe Power-Down Sequence

# Remove devices
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		v=`cat ${d}/vendor`
		[ "xx${v}" != "xx0x17cb" ] && echo 1 > ${d}/remove
	done

# Remove Buses
	sleep 2
	for i in `ls /sys/bus/pci/devices/`; do
		d=/sys/bus/pci/devices/${i}
		echo 1 > ${d}/remove
	done

# Remove RC
	sleep 2

	[ -f /sys/bus/pci/rcremove ] && {
		echo 1 > /sys/bus/pci/rcremove
	}
	[ -f /sys/devices/pci0000:00/pci_bus/0000:00/rcremove ] && {
		echo 1 > /sys/devices/pci0000:00/pci_bus/0000:00/rcremove
	}
	sleep 1

# Find scsi devices and remove it
	partition=`cat /proc/partitions | awk -F " " '{print $4}'`

	for entry in $partition; do
		sd_entry=$(echo $entry | head -c 2)

		if [ "$sd_entry" = "sd" ]; then
			[ -f /sys/block/$entry/device/delete ] && {
				echo 1 > /sys/block/$entry/device/delete
			}
		fi
	done


# Power off Malibu PHY of LAN ports
	ipq8074_phy_power_off
# USB Power-down Sequence
	if [ -d config/usb_gadget/g1 ]
	then
		echo "" > /config/usb_gadget/g1/UDC
	fi

	if [ -d /sys/module/dwc3_of_simple ]
	then
		rmmod usb_f_qdss
		rmmod u_qdss
		rmmod dwc3
		rmmod dwc3-of-simple
		rmmod dbm
		rmmod phy_msm_qusb
		rmmod phy_msm_ssusb_qmp
	elif [ -d /sys/module/dwc3_qcom ]
	then
		rmmod usb_f_qdss
		rmmod dwc3
		rmmod dwc3_qcom
		rmmod phy_qcom_qusb2
	fi
	sleep 2

#SD/MMC Power-down Sequence
	local emmcblock="$(find_mmc_part "rootfs")"

	if [ -z "$emmcblock" ]; then
		rm /tmp/sysinfo/sd_drvname
		for device in /sys/block/mmcblk0 /sys/block/mmcblk1
		do
		if [ -d $device ]; then
			sd_drvname=`readlink $device | grep -o "[0-9]*.sdhci"`
			echo "$sd_drvname" >> /tmp/sysinfo/sd_drvname
			echo $sd_drvname >> /sys/bus/platform/drivers/sdhci_msm/unbind
		fi
		done
	else
		rm /tmp/sysinfo/sd1_drvname
		if [ -z "${emmcblock##*mmcblk1*}" ] ;then
			sd1_drvname=`readlink /sys/block/mmcblk0 | grep -o "[0-9]*.sdhci"`
			echo "$sd1_drvname" > /tmp/sysinfo/sd1_drvname
			echo $sd1_drvname > /sys/bus/platform/drivers/sdhci_msm/unbind
		else
			sd1_drvname=`readlink /sys/block/mmcblk1 | grep -o "[0-9]*.sdhci"`
			echo "$sd1_drvname" > /tmp/sysinfo/sd1_drvname
			echo $sd1_drvname > /sys/bus/platform/drivers/sdhci_msm/unbind
		fi
	fi

# LAN interface down
	ifdown lan

# Disabling Auto scale on NSS cores
	echo 0 > /proc/sys/dev/nss/clock/auto_scale

# Scaling Down UBI Cores
	local board=$(ipq806x_board_name)
	case "$board" in

		ap-ac*)
			echo 187200000 > /proc/sys/dev/nss/clock/current_freq;
			;;
		*)
			echo 748800000 > /proc/sys/dev/nss/clock/current_freq;
			;;
	esac

# Cortex Power-down Sequence
	echo "powersave" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor

}

board=$(ipq806x_board_name)
case "$1" in
	false)
		case "$board" in
		db149 | ap148 | ap145 | ap148_1xx | db149_1xx | db149_2xx | ap145_1xx | ap160 | ap160_2xx | ap161 | ak01_1xx)
			ipq8064_ac_power ;;
		ap-dk01.1-c1 | ap-dk01.1-c2 | ap-dk05.1-c1)
			ipq4019_ap_dk01_1_ac_power ;;
		ap-dk04.1-c1 | ap-dk04.1-c2 | ap-dk04.1-c3 | ap-dk04.1-c4 | ap-dk04.1-c5 | ap-dk04.1-c6 | ap-dk06.1-c1 | ap-dk07.1-c1 | ap-dk07.1-c2 | ap-dk07.1-c3 | ap-dk07.1-c4)
			ipq4019_ap_dk04_1_ac_power ;;
		ap-hk01-c1 | ap-hk01-c2 | ap-hk01-c3 | ap-hk01-c4 | ap-hk01-c5 | ap-hk01-c6 | ap-hk02 | ap-hk06 | ap-hk07 | ap-hk08 | ap-hk09 | ap-hk10-c1 | ap-hk10-c2 | ap-hk11-c1 | ap-hk12 | ap-hk14 | ap-ac01 | ap-ac02 | ap-ac03 | ap-ac04 | ap-oak02 | ap-oak03 | db-hk01 | db-hk02)
			ipq8074_ac_power ;;
		ap-cp01-c1 | ap-cp01-c2 | ap-cp01-c3 | ap-cp01-c4 | ap-cp01-c5 | ap-cp02-c1 | ap-cp03-c1 | db-cp01 | db-cp02)
			ipq6018_ac_power ;;
		ap-mp02.1 | ap-mp03.1 | ap-mp03.1-c2 | ap-mp03.3 | ap-mp03.3-c2 | ap-mp03.3-c3 | ap-mp03.3-c4 | ap-mp03.3-c5 | ap-mp03.5-c1 | ap-mp03.5-c2 | ap-mp03.6-c1 | ap-mp03.6-c2 | db-mp02.1 | db-mp03.1 | db-mp03.1-c2 | db-mp03.3 | db-mp03.3-c2)
			ipq5018_ac_power ;;
		ap-al* | db-al*)
			ipq9574_ac_power ;;
		esac ;;
	true)
		case "$board" in
		db149 | ap148 | ap145 | ap148_1xx | db149_1xx | db149_2xx | ap145_1xx | ap160 | ap160_2xx | ap161 | ak01_1xx)
			ipq8064_battery_power ;;
		ap-dk01.1-c1 | ap-dk01.1-c2 | ap-dk05.1-c1)
			ipq4019_ap_dk01_1_battery_power ;;
		ap-dk04.1-c1 | ap-dk04.1-c2 | ap-dk04.1-c3 | ap-dk04.1-c4 | ap-dk04.1-c5 | ap-dk04.1-c6 | ap-dk06.1-c1 | ap-dk07.1-c1 | ap-dk07.1-c2 | ap-dk07.1-c3 | ap-dk07.1-c4)
			ipq4019_ap_dk04_1_battery_power ;;
		ap-hk01-c1 | ap-hk01-c2 | ap-hk01-c3 | ap-hk01-c4 | ap-hk01-c5 | ap-hk01-c6 | ap-hk02 | ap-hk06 | ap-hk07 | ap-hk08 | ap-hk09 | ap-hk10-c1 | ap-hk10-c2 | ap-hk11-c1 | ap-hk12 | ap-hk14 | ap-ac01 | ap-ac02 | ap-ac03 | ap-ac04 | ap-oak02 | ap-oak03 | db-hk01 | db-hk02)
			ipq8074_battery_power ;;
		ap-cp01-c1 | ap-cp01-c2 | ap-cp01-c3 | ap-cp01-c4 | ap-cp01-c5 | ap-cp02-c1 | ap-cp03-c1 | db-cp01 | db-cp02)
			ipq6018_battery_power ;;
		ap-mp02.1 | ap-mp03.1 | ap-mp03.1-c2 | ap-mp03.3 | ap-mp03.3-c2 | ap-mp03.3-c3 | ap-mp03.3-c4 | ap-mp03.3-c5 | ap-mp03.5-c1 | ap-mp03.5-c2 | ap-mp03.6-c1 | ap-mp03.6-c2 | db-mp02.1 | db-mp03.1 | db-mp03.1-c2 | db-mp03.3 | db-mp03.3-c2)
			ipq5018_battery_power ;;
		ap-al* | db-al*)
			ipq9574_battery_power ;;
		esac ;;
esac
