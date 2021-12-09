# ###################################################
# # Makefile for the Service Prioritization
# ###################################################

#
# Target module name
#
TARGET := emesh-sp

obj-m += $(TARGET).o

# Target objects
$(TARGET)-objs := \
	sp.o \
	sp_hook.o \
	sp_mapdb.o

# Module extra compilation flags
ccflags-y += -Werror -Wall -g
ccflags-y += -DSP_DEBUG_LEVEL=0
