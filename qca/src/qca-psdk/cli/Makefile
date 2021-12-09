# Target module name
TARGET := psdk_cli

# Include directories
include $(SUBDIR)/../config.mk

CC=$(TOOL_PATH)/$(CROSS_COMPILE)gcc
LD=$(TOOL_PATH)/$(CROSS_COMPILE)ld

QCA808X_INCLUDES = \
		   -I$(SUBDIR)/include \
		   -I$(SUBDIR)/include/fal \
		   -I$(SUBDIR)/include/osal \
		   -I$(SUBDIR)/include/shell

BUILD_DATE :=$(shell date -u  +%F-%T)
VER :="2.0.0"

QCA808X_CLI_FLAGS = \
		-DUSER_MODE		\
		-DKVER32		\
		-DUK_IF			\
		-DUK_MINOR_DEV=253	\
		-DSHELL_DEV="\"/dev/switch_qca808x\""	\
		-DPROMPT_STR="\"psdk>\""		\
		-DSSDK_STR="\"PSDK\""			\
		-DBUILD_DATE="\"$(BUILD_DATE)\""	\
		-DVERSION="\"$(VER)\""

QCA808X_CLI_FLAGS += $(QCA808X_INCLUDES)

# cli part of makefile
include $(SUBDIR)/target.mk

all: $(TARGET)

$(cli-objs):%.o : %.c
	$(CC) $(QCA808X_CLI_FLAGS) -c -o $@ $<

$(TARGET) : $(cli-objs)
	$(CC) -o $(TARGET) -fPIC $^

install:
	$(TOOL_PATH)/$(CROSS_COMPILE)strip $(TARGET)
	mkdir -p $(SUBDIR)/../obj
	cp -f $(TARGET) $(SUBDIR)/../obj/

clean:
	rm -f $(cli-objs)
	rm -f $(TARGET)

