# Makefile for the PPE driver

ccflags-y := -I$(obj) -I$(obj)/..

export BUILD_ID = \"Build Id: $(shell date +'%m/%d/%y, %H:%M:%S')\"
ccflags-y += -DNSS_PPE_BUILD_ID="$(BUILD_ID)"

KERNELVERSION := $(word 1, $(subst ., ,$(KERNELVERSION))).$(word 2, $(subst ., ,$(KERNELVERSION)))

obj-y += clients/

obj ?= .
