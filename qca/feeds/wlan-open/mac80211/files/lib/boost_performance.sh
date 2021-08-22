#!/bin/sh
#
# Copyright (c) 2021, The Linux Foundation. All rights reserved.
#
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
[ -e /lib/ipq806x.sh ] && . /lib/ipq806x.sh

type ipq806x_board_name &>/dev/null  || ipq806x_board_name() {
        echo $(board_name) | sed 's/^\([^-]*-\)\{1\}//g'
}

boost_performance() {
	if [ -d "/sys/kernel/debug/ath11k" ]; then
		local board=$(ipq806x_board_name)

		case "$board" in
			ap-hk10-c2)
				#case for rdp413
				echo 1 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0000\:01\:00.0/stats_disable
				echo 1 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0001\:01\:00.0/stats_disable

				echo 0 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0000\:01\:00.0/ce_latency_stats
				echo 0 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0001\:01\:00.0/ce_latency_stats

				echo 0 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0000\:01\:00.0/trace_qdss
				echo 0 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0001\:01\:00.0/trace_qdss

				tc qdisc replace dev wlan0 root noqueue
				tc qdisc replace dev wlan1 root noqueue

				echo 0 > /proc/sys/dev/nss/clock/auto_scale
				echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
				;;
			ap-hk14)
				#case for rdp419
				echo 1 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0000\:01\:00.0/stats_disable
				echo 1 > /sys/kernel/debug/ath11k/ipq8074\ hw2.0/stats_disable

				echo 0 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0000\:01\:00.0/ce_latency_stats
				echo 0 > /sys/kernel/debug/ath11k/ipq8074\ hw2.0/ce_latency_stats

				echo 0 > /sys/kernel/debug/ath11k/qcn9074\ hw1.0_0000\:01\:00.0/trace_qdss
				echo 0 > /sys/kernel/debug/ath11k/ipq8074\ hw2.0/trace_qdss

				tc qdisc replace dev wlan0 root noqueue
				tc qdisc replace dev wlan1 root noqueue
				tc qdisc replace dev wlan2 root noqueue

				echo 0 > /proc/sys/dev/nss/clock/auto_scale
				echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
				;;
			ap-oak03)
				#case for rdp393
				echo 1 > /sys/kernel/debug/ath11k/ipq8074\ hw2.0/stats_disable
				echo 0 > /sys/kernel/debug/ath11k/ipq8074\ hw2.0/ce_latency_stats
				echo 0 > /sys/kernel/debug/ath11k/ipq8074\ hw2.0/trace_qdss

				tc qdisc replace dev wlan0 root noqueue
				tc qdisc replace dev wlan1 root noqueue
				tc qdisc replace dev wlan2 root noqueue

				echo 0 > /proc/sys/dev/nss/clock/auto_scale
				echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
				;;
			ap-cp01-c1)
				#case for rdp352
				echo 1 > /sys/kernel/debug/ath11k/ipq6018\ hw1.0/stats_disable
				echo 0 > /sys/kernel/debug/ath11k/ipq6018\ hw1.0/ce_latency_stats
				echo 0 > /sys/kernel/debug/ath11k/ipq6018\ hw1.0/trace_qdss

				tc qdisc replace dev wlan0 root noqueue
				tc qdisc replace dev wlan1 root noqueue

				echo 0 > /proc/sys/dev/nss/clock/auto_scale
				echo "performance" > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
				echo "performance" > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor
				;;
			*)
				#no settings
				;;
		esac
	fi
}

boost_performance
