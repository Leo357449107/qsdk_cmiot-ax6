#!/bin/sh
#
# Copyright (C) 2011 OpenWrt.org

find_net_config() {(
	local vif="$1"
	local cfg
	local ifname

	config_get cfg "$vif" network

	[ -z "$cfg" ] && {
		include /lib/network
		scan_interfaces

		config_get ifname "$vif" ifname

		cfg="$(find_config "$ifname")"
	}
	[ -z "$cfg" ] && return 0
	echo "$cfg"
)}


bridge_interface() {(
	local cfg="$1"
	[ -z "$cfg" ] && return 0

	include /lib/network
	scan_interfaces

	for cfg in $cfg; do
		config_get iftype "$cfg" type
		[ "$iftype" = bridge ] && config_get "$cfg" ifname
		prepare_interface_bridge "$cfg"
		return $?
	done
)}

start_net() {(
	local iface="$1"
	local config="$2"
	local vifmac="$3"

	[ -f "/var/run/$iface.pid" ] && kill "$(cat /var/run/${iface}.pid)" 2>/dev/null
	[ -z "$config" ] || {
		include /lib/network
		scan_interfaces
		for config in $config; do
			setup_interface "$iface" "$config" "" "$vifmac"
		done
	}
)}

set_wifi_up() {
	local cfg="$1"
	local ifname="$2"
	uci_set_state wireless "$cfg" up 1
	uci_set_state wireless "$cfg" ifname "$ifname"
}

set_wifi_down() {
	local cfg="$1"
	local vifs vif vifstr

	[ -f "/var/run/wifi-${cfg}.pid" ] &&
		kill "$(cat "/var/run/wifi-${cfg}.pid")" 2>/dev/null
	uci_revert_state wireless "$cfg"
	config_get vifs "$cfg" vifs
	for vif in $vifs; do
		uci_revert_state wireless "$vif"
	done
}

scan_wifi() {
	local cfgfile="$1"
	DEVICES=
	config_cb() {
		local type="$1"
		local section="$2"

		# section start
		case "$type" in
			wifi-device)
				append DEVICES "$section"
				config_set "$section" vifs ""
				config_set "$section" ht_capab ""
			;;
		esac

		# section end
		config_get TYPE "$CONFIG_SECTION" TYPE
		case "$TYPE" in
			wifi-iface)
				config_get device "$CONFIG_SECTION" device
				config_get vifs "$device" vifs
				append vifs "$CONFIG_SECTION"
				config_set "$device" vifs "$vifs"
			;;
		esac
	}
	config_load "${cfgfile:-wireless}"
}

wifi_trap() {
	for driver in ${1:-$DRIVERS}; do (
		if eval "type trap_$driver" 2>/dev/null >/dev/null; then
			eval "trap_$driver" >&2
		fi
	); done
}
