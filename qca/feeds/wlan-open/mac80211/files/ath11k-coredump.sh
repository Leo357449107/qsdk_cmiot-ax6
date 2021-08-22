#!/bin/sh
: '
 Copyright (c) 2020, The Linux Foundation. All rights reserved.

 Permission to use, copy, modify, and/or distribute this software for any
 purpose with or without fee is hereby granted, provided that the above
 copyright notice and this permission notice appear in all copies.

 THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
'

SERVER=$(fw_printenv serverip | cut -c10-24);
TIMESTAMP=$(date +%Y%m%d%H%M%S)

if [ ! -n "$SERVER" ]; then
	printf "%s\n" "Wrong configuaration SERVER = $SERVER" > /dev/console
	exit 0
fi

if [ -e /sys/devices/platform/soc/c000000.wifi1/devcoredump/data ] && [ "$ACTION" = add ]; then
	FILENAME="IPQ8074-m3dump-$TIMESTAMP.bin"
	DUMPPATH="/sys/devices/platform/soc/c000000.wifi1/devcoredump/data"
fi
if [ -e /sys/devices/platform/soc/c000000.wifi/devcoredump/data ] && [ "$ACTION" = add ]; then
	FILENAME="IPQ6018-m3dump-$TIMESTAMP.bin"
	DUMPPATH="/sys/devices/platform/soc/c000000.wifi/devcoredump/data"
fi

if [ -e /sys/bus/pci/devices/0000:01:00.0/devcoredump/data ] && [ "$ACTION" = add ]; then
	FILENAME="qcn9000-pci0-q6dump-$TIMESTAMP.bin"
	DUMPPATH="/sys/bus/pci/devices/0000:01:00.0/devcoredump/data"
fi

if [ -e /sys/bus/pci/devices/0001:01:00.0/devcoredump/data ] && [ "$ACTION" = add ]; then
	FILENAME="qcn9000-pci1-q6dump-$TIMESTAMP.bin"
	DUMPPATH="/sys/bus/pci/devices/0001:01:00.0/devcoredump/data"
fi

if [ -e /sys/devices/platform/soc/soc:wifi1@c000000/devcoredump/data ] && [ "$ACTION" = add ]; then
	FILENAME="qcn9100_1-q6dump-$TIMESTAMP.bin"
	DUMPPATH="/sys/devices/platform/soc/soc:wifi1@c000000/devcoredump/data"
fi

if [ -e /sys/devices/platform/soc/soc:wifi2@c000000/devcoredump/data ] && [ "$ACTION" = add ]; then
	FILENAME="qcn9100_2-q6dump-$TIMESTAMP.bin"
	DUMPPATH="/sys/devices/platform/soc/soc:wifi2@c000000/devcoredump/data"
fi

if [ -n "$FILENAME" ]; then
	printf "%s\n" "Collecting dump_data in $SERVER" > /dev/console
	cd /tmp
	cp $DUMPPATH $FILENAME
	$(tftp -l $FILENAME -p $SERVER 2>&1)
	if [ $? -eq 0 ]; then
		printf "%s\n" "dump_data collected in $SERVER" \
								> /dev/console
	else
		printf "%s\n" "dump_data collection failed in $SERVER" \
								> /dev/console
	fi
	echo 1 > $DUMPPATH
	rm $FILENAME
	cd /
fi
