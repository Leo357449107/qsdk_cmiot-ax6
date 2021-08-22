#! /bin/sh

#
# Copyright (c) 2019, The Linux Foundation. All rights reserved.
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

retval=0

check_temp()
{
  for i in `seq 0 1 11`; do
    temp=$(cat /sys/devices/virtual/thermal/thermal_zone$i/temp)
    echo "Thermal_zone$i = $temp" > /dev/console 2>&1
    if [ $temp -lt $1 ]
    then
      echo "Thermal_zone$i is below threshold $1C" > /dev/console 2>&1
      retval=0
      return
    fi
  done
  retval=1
}

run_load()
{
  stress -c 10 -m 1 --vm-bytes 50M --timeout 20s
}

echo "Threshold Temperature(celcius)=$1" > /dev/console 2>&1
governor=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)
echo "CPU governor = $governor" > /dev/console 2>&1

while true; do
  check_temp $1

  if [ $retval -eq 0 ]
  then
    run_load
  else
    echo "Booting ...." > /dev/console 2>&1
    return
  fi
done
