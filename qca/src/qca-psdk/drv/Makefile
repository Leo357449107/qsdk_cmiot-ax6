# Target module name
TARGET := qca808x

include $(SUBDIR)/../config.mk

QCA808X_INCLUDES = \
		   -I$(SUBDIR)/include \
		   -I$(SUBDIR)/include/fal \
		   -I$(SUBDIR)/include/hsl \
		   -I$(SUBDIR)/include/osal

QCA808X_FLAGS = \
		-DKVER32		\
		-DUK_IF			\
		-DKERNEL_MODULE		\
		-DAPI_LOCK		\
		-DUK_MINOR_DEV=253	\
		-DSSDK_STR="\"psdk\""	\
		-DSHELL_DEV="\"qca-psdk\""\
		-DQCA808X_PORTS_INFO	\
		-DIN_PHY_I2C_MODE
ccflags-y += $(QCA808X_INCLUDES) $(QCA808X_FLAGS)

# Kernel module compilation - part 2
ifneq ($(KERNELRELEASE),)

# Kernel module part of makefile
include $(SUBDIR)/target.mk

else
# Kernel Makefile targets - part 1
KERNEL_MAKEOPTS := -C $(LINUX_DIR) \
			M=$(SUBDIR) \
			CC="$(TOOL_PATH)/$(CROSS_COMPILE)gcc" \
			LD="$(TOOL_PATH)/$(CROSS_COMPILE)ld" \
			ARCH="$(ARCH)"

all: module install

module:
	make $(KERNEL_MAKEOPTS)  modules

install:
	$(TOOL_PATH)/$(CROSS_COMPILE)strip --strip-debug $(TARGET).ko
	mkdir -p $(SUBDIR)/../obj
	cp -f $(TARGET).ko $(SUBDIR)/../obj/

clean:
	make -C $(LINUX_DIR) M=$(SUBDIR) clean

endif

