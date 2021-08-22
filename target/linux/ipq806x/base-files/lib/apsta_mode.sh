#!/bin/sh
#
# Copyright (c) 2018 The Linux Foundation. All rights reserved.
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

sta_intf="$1"
ap_intf="$2"
hostapd_conf="$3"

ap_ht_capab=$(cat $hostapd_conf 2> /dev/null | grep ht_capab | grep -v vht | cut -d'=' -f 2)

# Hostapd VHT calculations
hostapd_vht_oper_chwidth() {
	local sta_width="$1"
	case $sta_width in
		"80")
			ap_vht_oper_chwidth=1;;
		"160")
			ap_vht_oper_chwidth=2;;
		"80+80")
			ap_vht_oper_chwidth=3;;
		"20"|"40"|*)
			ap_vht_oper_chwidth=0;;
	esac
}

hostapd_vht_oper_centr_freq_seg0_idx() {
	local sta_width="$1"
	local sta_channel="$2"

	case $ap_vht_oper_chwidth in
		"0")
			case $sta_width in
			"20")
				ap_vht_oper_centr_freq_seg0_idx=$sta_channel;;
			"40")
				case "$(( ($sta_channel / 4) % 2 ))" in
					1) ap_vht_oper_centr_freq_seg0_idx=$(($sta_channel + 2));;
					0) ap_vht_oper_centr_freq_seg0_idx=$(($sta_channel - 2));;
				esac
			;;
			esac
		;;
		"1")
			case "$(( ($sta_channel / 4) % 4 ))" in
				1) ap_vht_oper_centr_freq_seg0_idx=$(($sta_channel + 6));;
				2) ap_vht_oper_centr_freq_seg0_idx=$(($sta_channel + 2));;
				3) ap_vht_oper_centr_freq_seg0_idx=$(($sta_channel - 2));;
				0) ap_vht_oper_centr_freq_seg0_idx=$(($sta_channel - 6));;
			esac
		;;
		"2")
			case "$sta_channel" in
				36|40|44|48|52|56|60|64) ap_vht_oper_centr_freq_seg0_idx=50;;
				100|104|108|112|116|120|124|128) ap_vht_oper_centr_freq_seg0_idx=114;;
			esac
	esac
}

# Hostapd HT40 secondary channel offset calculations
hostapd_ht40_mode() {
	local sta_channel="$1"

	# 5Ghz channels
	if [ $sta_channel -ge 36 ]; then
		case "$(( ($sta_channel / 4) % 2 ))" in
			1) if [ ! -z $ap_ht_mode ]; then
				ap_ht_mode="$(echo $ap_ht_mode | sed -e "s/HT40-/HT40+/g")"
			   else
				ap_ht_mode="[HT40+]"
			   fi
			;;
			0) if [ ! -z $ap_ht_mode ]; then
				ap_ht_mode="$(echo $ap_ht_mode | sed -e "s/HT40+/HT40-/g")"
			   else
				ap_ht_mode="[HT40-]"
			   fi
			;;
		esac
	else
		# 2.4Ghz channels
		if [ "$sta_channel" -lt 7 ]; then
			if [ ! -z $ap_ht_mode ]; then
				ap_ht_mode="$(echo $ap_ht_mode | sed -e "s/HT40-/HT40+/g")"
                        else
                                ap_ht_mode="[HT40+]"
                        fi
		else
			if [ ! -z $ap_ht_mode ]; then
				ap_ht_mode="$(echo $ap_ht_mode | sed -e "s/HT40+/HT40-/g")"
                        else
                                ap_ht_mode="[HT40-]"
                        fi
		fi
	fi
	#echo "Secondary offset is $ap_ht_mode" > /dev/ttyMSM0
	hostapd_cli -i $ap_intf set ht_capab $ap_ht_mode 2> /dev/null
}

# Hostapd HT20 mode
hostapd_ht20_mode() {
        local ht_capab_20=$(echo $ap_ht_capab | sed -e 's/\(\[HT40*+*-*]\)//g')
        #echo "Setting HT capab $ht_capab_20" > /dev/ttyMSM0
        hostapd_cli -i $ap_intf set ht_capab $ht_capab_20 2> /dev/null
}

# STA association is completed, hence adjusting hostapd running config
hostapd_adjust_config() {
	sta_channel=$(iw $sta_intf info 2> /dev/null | grep channel | cut -d' ' -f 2)
	sta_width=$(wpa_cli -i $sta_intf signal_poll 2> /dev/null | grep WIDTH | cut -d'=' -f 2 | grep MHz | cut -d' ' -f 1)
	sta_80211ac=$(wpa_cli -i $sta_intf status 2> /dev/null | grep ieee80211ac | cut -d'=' -f 2)

	#echo "STA associated in Channel $sta_channel, Width $sta_width MHz" > /dev/ttyMSM0
	hostapd_cli -i $ap_intf set channel $sta_channel 2> /dev/null
	if [ $sta_channel -ge 36 ]; then
		hostapd_cli -i $ap_intf set hw_mode a 2> /dev/null
	else
		hostapd_cli -i $ap_intf set hw_mode g 2> /dev/null
	fi

	ap_ht_mode=$(echo $ap_ht_capab | sed -n 's/.*\(\[HT40*+*-*]\).*/\1/p')
	#echo "Current AP HT capab $ap_ht_capab" > /dev/ttyMSM0

	if [ "$(wpa_cli -i $sta_intf signal_poll 2> /dev/null | grep WIDTH | cut -d'(' -f 2 | cut -d')' -f 1)" = "no HT" ]; then
		#echo "STA associated in No HT mode, downgrading AP as well" > /dev/ttyMSM0
		hostapd_cli -i $ap_intf set ieee80211ac 0 2> /dev/null
		hostapd_cli -i $ap_intf set ieee80211n 0 2> /dev/null

	elif [ ! -z $sta_80211ac ]; then
		#echo "STA associated in VHT$sta_width mode, applying same config to AP" > /dev/ttyMSM0
		local ap_vht_oper_chwidth
		local ap_vht_oper_centr_freq_seg0_idx

		hostapd_vht_oper_chwidth "$sta_width"

		hostapd_cli -i $ap_intf set ieee80211ac 1 2> /dev/null
		hostapd_cli -i $ap_intf set ieee80211n 1 2> /dev/null

		#echo "vht_oper_chwidth is $ap_vht_oper_chwidth for VHT$sta_width mode" > /dev/ttyMSM0

		hostapd_vht_oper_centr_freq_seg0_idx "$sta_width" "$sta_channel"

		#echo "New vht_oper_chwidth is $ap_vht_oper_chwidth, vht_oper_centr_freq_seg0_idx is $ap_vht_oper_centr_freq_seg0_idx" > /dev/ttyMSM0
		hostapd_cli -i $ap_intf set vht_oper_chwidth $ap_vht_oper_chwidth
		hostapd_cli -i $ap_intf set vht_oper_centr_freq_seg0_idx $ap_vht_oper_centr_freq_seg0_idx

		if [ $sta_width = "20" ]; then
                        #echo "Setting VHT20 mode to AP" > /dev/ttyMSM0
                        hostapd_ht20_mode
                else
                        hostapd_ht40_mode "$sta_channel"
                fi
	else
		#echo "STA associated in HT$sta_width mode, applying same config to AP" > /dev/ttyMSM0
		hostapd_cli -i $ap_intf set ieee80211n 1
		hostapd_cli -i $ap_intf set ieee80211ac 0

		if [ $sta_width = "20" ]; then
                        #echo "Setting HT20 mode to AP" > /dev/ttyMSM0
                        hostapd_ht20_mode
                else
                        hostapd_ht40_mode "$sta_channel"
                fi
	fi
}

#echo "Checking wpa_state $(wpa_cli -i $sta_intf status 2> /dev/null | grep wpa_state | cut -d'=' -f 2)" > /dev/ttyMSM0

while true;
do
	if [[ ! -e "/var/run/wpa_supplicant/$sta_intf" && ! -e "/var/run/hostapd/$ap_intf" ]]; then
		echo "AP+STA mode not running, exiting script" >> /tmp/apsta_debug.log
		exit
	fi

	if [ $(wpa_cli -i $sta_intf status 2> /dev/null | grep wpa_state | cut -d'=' -f 2) = "DISCONNECTED"  -o \
			 $(wpa_cli -i $sta_intf status 2> /dev/null | grep wpa_state | cut -d'=' -f 2) = "SCANNING" ] &&
				[ $(hostapd_cli -i $ap_intf status 2> /dev/null | grep state | cut -d'=' -f 2) = "ENABLED" ]; then
		#echo "wpa_s state: $(wpa_cli -i $sta_intf status 2> /dev/null | grep wpa_state | cut -d'=' -f 2), stopping AP" > /dev/ttyMSM0
		hostapd_cli -i $ap_intf disable
	fi

	if [ $(wpa_cli -i $sta_intf status 2> /dev/null | grep wpa_state | cut -d'=' -f 2) = "COMPLETED" ] &&
		[ $(hostapd_cli -i $ap_intf status 2> /dev/null | grep state | cut -d'=' -f 2) = "DISABLED" ]; then
		#echo "wpa_s state: $(wpa_cli -i $sta_intf status 2> /dev/null | grep wpa_state | cut -d'=' -f 2), starting AP" > /dev/ttyMSM0
		wpa_cli -i $sta_intf signal_poll
		hostapd_adjust_config
		#echo "Enabling below hostapd config:" > /dev/ttyMSM0
		hostapd_cli -i $ap_intf status 2> /dev/null
		[ $(hostapd_cli -i $ap_intf status 2> /dev/null | grep state | cut -d'=' -f 2) = "DISABLED" ] && hostapd_cli -i $ap_intf enable
		sleep 4
		#echo "Hostapd state: $(hostapd_cli status 2> /dev/null | grep state | cut -d'=' -f 2)" > /dev/ttyMSM0

		if [ $(hostapd_cli -i $ap_intf status 2> /dev/null | grep state | cut -d'=' -f 2) = "DISABLED" ]; then
			echo "Hostapd enable failed, exiting" >> /tmp/apsta_debug.log
			date >> /tmp/apsta_debug.log
			hostapd_cli -i $ap_intf status >> /tmp/apsta_debug.log
			wpa_cli -i $sta_intf signal_poll >> /tmp/apsta_debug.log
			wpa_cli -i $sta_intf status >> /tmp/apsta_debug.log
			date >> /tmp/apsta_debug.log
			wifi down
			exit
		fi
	fi
done &
