# Recipe extension for cryptodev-linux

KERNEL_MAKE_FLAGS+= \
	CRYPTODEV_CFLAGS=-Wno-vla
