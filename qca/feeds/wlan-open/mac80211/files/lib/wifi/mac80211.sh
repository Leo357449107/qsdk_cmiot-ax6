#!/bin/sh
append DRIVERS "mac80211"

lookup_phy() {
	[ -n "$phy" ] && {
		[ -d /sys/class/ieee80211/$phy ] && return
	}

	# Incase of multiple radios belonging to the same soc, the device path
	# of these radio's would be same. To find the correct phy, we can
	# get the phy index of the device in soc and use it during searching
	# the global phy list
	local radio_idx=${device:5:1}
	local first_phy_idx=0
	local delta=0
	local devpath
	config_get devpath "$device" path
	while :; do
	config_get devicepath "radio$first_phy_idx" path
	[ -n "$devicepath" -a -n "$devpath" ] || break
	[ "$devpath" == "$devicepath" ] && break
	first_phy_idx=$(($first_phy_idx + 1))
	done

	delta=$(($radio_idx - $first_phy_idx))

	[ -n "$devpath" ] && {
		for phy in $(ls /sys/class/ieee80211 2>/dev/null); do
			case "$(readlink -f /sys/class/ieee80211/$phy/device)" in
			*$devpath)
				if [ $delta -gt 0 ]; then
					delta=$(($delta - 1))
					continue;
				fi
				return;;
			esac
		done
	}

	local macaddr="$(config_get "$device" macaddr | tr 'A-Z' 'a-z')"
	[ -n "$macaddr" ] && {
		for _phy in /sys/class/ieee80211/*; do
			[ -e "$_phy" ] || continue
			[ "$macaddr" = "$(cat ${_phy}/macaddress)" ] || continue
			phy="${_phy##*/}"
			return
		done
	}
	phy=
	return
}

find_mac80211_phy() {
	local device="$1"

	config_get phy "$device" phy
	lookup_phy
	[ -n "$phy" -a -d "/sys/class/ieee80211/$phy" ] || {
		echo "PHY for wifi device $1 not found"
		return 1
	}
	config_set "$device" phy "$phy"

	config_get macaddr "$device" macaddr
	[ -z "$macaddr" ] && {
		config_set "$device" macaddr "$(cat /sys/class/ieee80211/${phy}/macaddress)"
	}

	[ -z "$macaddr" ] && {
		config_set "$device" macaddr "$(cat /sys/class/ieee80211/${phy}/device/net/wlan${phy#phy}/address)"
	}
	return 0
}

check_mac80211_device() {
	config_get phy "$1" phy
	[ -z "$phy" ] && {
		find_mac80211_phy "$1" >/dev/null || return 0
		config_get phy "$1" phy
	}
	[ "$phy" = "$dev" ] && found=1
}

detect_mac80211() {
	devidx=0

	config_load wireless

	if [ ! -f "/etc/config/wireless" ] || ! grep -q "enable_smp_affinity" "/etc/config/wireless"; then
		cat <<EOF
config smp_affinity  mac80211
	option enable_smp_affinity	1
	option enable_color		1

EOF
	fi

	while :; do
		config_get type "radio$devidx" type
		[ -n "$type" ] || break
		devidx=$(($devidx + 1))
	done

	#add this delay for empty wifi script issue
	count=0
	while [ $count -le 10 ]
	do
		sleep  1
		if ([ $(ls /sys/class/ieee80211 | wc -l  | grep -w "0") ])
		then
			count=$(( count+1 ))
		else
			sleep 1
			break
		fi
	done

	for _dev in /sys/class/ieee80211/*; do
		[ -e "$_dev" ] || continue
		dev="${_dev##*/}"
		found=0
		config_foreach check_mac80211_device wifi-device
		[ "$found" -gt 0 ] && continue

		mode_11n=""
		mode_band="a"
		channel="36"
		htmode=""
		ht_capab=""
		encryption="none"
		security=""

		iw phy "$dev" info | grep -q '5180 MHz' || iw phy "$dev" info | grep -q '5955 MHz' || { mode_band="g"; channel="11"; }
		(iw phy "$dev" info | grep -q '5745 MHz' && (iw phy "$dev" info | grep -q -F '5180 MHz [36] (disabled)')) && { mode_band="a"; channel="149"; }
		iw phy "$dev" info | grep -q '60480 MHz' && { mode_11n="a"; mode_band="d"; channel="2"; }

		iw phy "$dev" info | grep -q 'Capabilities:' && htmode=HT20
		vht_cap=$(iw phy "$dev" info | grep -c 'VHT Capabilities')

		[ "$mode_band" = a ] && htmode="VHT80"

		iw phy "$dev" info | grep -q '5180 MHz' || iw phy "$dev" info | grep -q '5745 MHz' || {
			iw phy "$dev" info | grep -q '5955 MHz' && {
				channel="49"; htmode="HE80"; encryption="sae";
				append ht_capab "	option band	3" "$N"
			}
		}

		[ -n $htmode ] && append ht_capab "	option htmode	$htmode" "$N"

		append security "	option encryption  $encryption" "$N"
		if [ $encryption == "sae" ]; then
			append security "	option sae_pwe	1" "$N"
			append security "	option key	0123456789" "$N"
		fi

		if [ -x /usr/bin/readlink -a -h /sys/class/ieee80211/${dev} ]; then
			path="$(readlink -f /sys/class/ieee80211/${dev}/device)"
		else
			path=""
		fi
		if [ -n "$path" ]; then
			path="${path##/sys/devices/}"
			case "$path" in
				platform*/pci*) path="${path##platform/}";;
			esac
			dev_id="	option path	'$path'"
		else
			dev_id="	option macaddr	$(cat /sys/class/ieee80211/${dev}/macaddress)"
		fi

		cat <<EOF
config wifi-device  radio$devidx
	option type     mac80211
	option channel  ${channel}
	option hwmode	11${mode_11n}${mode_band}
$dev_id
$ht_capab
	# REMOVE THIS LINE TO ENABLE WIFI:
	option disabled 1

config wifi-iface
	option device   radio$devidx
	option network  lan
	option mode     ap
	option ssid     OpenWrt
$security

EOF
	devidx=$(($devidx + 1))
	done
}
# This start_lbd is to check the dual band availability and
# make sure that dual bands (2.4G and 5G) available before
# starting lbd init script.

start_lbd() {
	local band_24g
	local band_5g
	local i=0

	driver=$(lsmod | cut -d' ' -f 1 | grep ath10k_core)

	if [ "$driver" == "ath10k_core" ]; then
		while [ $i -lt 10 ]
		do
			BANDS=$(/usr/sbin/iw dev 2> /dev/null | grep channel | cut -d' ' -f 2 | cut -d'.' -f 1)
			for channel in $BANDS
			do
				if [ "$channel" -le "14" ]; then
					band_24g=1
				elif [ "$channel" -ge "36" ]; then
					band_5g=1
				fi
			done

			if [ "$band_24g" == "1" ] && [ "$band_5g" == "1" ]; then
				/etc/init.d/lbd start
				return 0
			fi
			sleep 1
			i=$(($i + 1))
		done
	fi
	return 0
}

post_mac80211() {
	local action=${1}

	config_get enable_smp_affinity mac80211 enable_smp_affinity 0

	if [ "$enable_smp_affinity" -eq 1 ]; then
		[ -f "/lib/smp_affinity_settings.sh" ] && {
                        . /lib/smp_affinity_settings.sh
                        enable_smp_affinity_wifi
                }
		[ -f "/lib/update_smp_affinity.sh" ] && {
			. /lib/update_smp_affinity.sh
			enable_smp_affinity_wigig
		}
	fi

	case "${action}" in
		enable)
			[ -f "/usr/sbin/fst.sh" ] && {
				/usr/sbin/fst.sh start
			}
			if [ -f "/etc/init.d/lbd" ]; then
				start_lbd &
			fi
		;;
	esac

	if [ -e "/sys/module/ath11k/parameters/enable_qdss_trace" ]; then
		qdss_tracing=`cat /sys/module/ath11k/parameters/enable_qdss_trace`
		if [ $qdss_tracing = 1 ]; then
			echo "q6mem" > /sys/bus/coresight/devices/coresight-tmc-etr/out_mode
			echo 1 > /sys/bus/coresight/devices/coresight-tmc-etr/curr_sink
			echo "0x06021FB0 0xc5acce55" > /sys/bus/coresight/devices/coresight-hwevent/setreg
			echo "0x06130FB0 0xc5acce55" > /sys/bus/coresight/devices/coresight-hwevent/setreg
			echo "0x06021000 0x00000320" > /sys/bus/coresight/devices/coresight-hwevent/setreg
			echo "0x06130000 0x00000340" > /sys/bus/coresight/devices/coresight-hwevent/setreg
			echo 1 > /sys/bus/coresight/devices/coresight-stm/enable
		fi
	fi
	return 0
}

pre_mac80211() {
	local action=${1}

	case "${action}" in
		disable)
			[ -f "/usr/sbin/fst.sh" ] && {
				/usr/sbin/fst.sh set_mac_addr
				/usr/sbin/fst.sh stop
			}
			[ ! -f /etc/init.d/lbd ] || /etc/init.d/lbd stop

			extsta_path=/sys/module/mac80211/parameters/extsta
			[ -e $extsta_path ] && echo 0 > $extsta_path
		;;
	esac
	return 0
}
