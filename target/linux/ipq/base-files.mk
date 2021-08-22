
EXTRA_FILES:= \
	/etc/config/network \
	/sbin/firstboot \
	/sbin/sysupgrade \
	/cron \
	/sbin/led.sh \
	/sbin/sysdebug \
	/etc/rc.button/power \
	/powerctl \
	/lib/functions/leds.sh \
	/lib/debug/system \
	/lib/upgrade \
	/etc/init.d/led \
	/etc/init.d/powerctl \
	/etc/rc.button/rfkill \
	/etc/rc.button/power \
	/lib/functions/uci-defaults-new.sh \
	/lib/functions/leds.sh \
	/bin/board_detect

define Package/base-files/install-target
	rm -f $(1)/etc/config/network
	$(if $(CONFIG_LOWMEM_FLASH),$(foreach file, $(EXTRA_FILES), rm -rf $(1)/$(file);))
endef
