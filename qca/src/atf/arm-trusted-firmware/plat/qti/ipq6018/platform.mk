#
# Copyright (c) 2017-2018, ARM Limited and Contributors. All rights reserved.
# Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Make for ipq6018 QTI platform.

QTI_PLAT_PATH		:=	plat/qti
CHIPSET			:=	${PLAT}
SOC			:=	$(patsubst ipq%,%,${PLAT})

# Turn On Separate code & data.
SEPARATE_CODE_AND_RODATA	:=	1
USE_COHERENT_MEM		:=	1
WARMBOOT_ENABLE_DCACHE_EARLY	:=	1

# Disable the PSCI platform compatibility layer
ENABLE_PLAT_COMPAT		:=	0

# Enable PSCI v1.0 extended state ID format
PSCI_EXTENDED_STATE_ID	:=	1
ARM_RECOM_STATE_ID_ENC  :=  1

COLD_BOOT_SINGLE_CPU		:=	1
PROGRAMMABLE_RESET_ADDRESS	:=	1

RESET_TO_BL31			:=	0

MULTI_CONSOLE_API		:=	1
GICV2_G0_FOR_EL3               := 1
# Enable stack protector.
ENABLE_STACK_PROTECTOR := strong

#IPQ6018 Boot Params
XBL_BOOT                       := 1
$(eval $(call add_define,XBL_BOOT))

#IPQ6018 Console Uart Prints
#Set to 0 by Default to use Diag Ring Buffer
QTI_UART_PRINT                       := 0
$(eval $(call add_define,QTI_UART_PRINT))

QTI_6018_PLATFORM             := 1
$(eval $(call add_define,QTI_6018_PLATFORM))

QTI_EXTERNAL_INCLUDES	:=	-I${QTI_PLAT_PATH}/${CHIPSET}/inc			\
				-I${QTI_PLAT_PATH}/common/inc				\
				-I${QTI_PLAT_PATH}/common/inc/$(ARCH)			\
				-I${QTI_PLAT_PATH}/qtiseclib/inc			\
				-I${QTI_PLAT_PATH}/qtiseclib/inc/${CHIPSET}			\

QTI_BL31_SOURCES	:=	$(QTI_PLAT_PATH)/${CHIPSET}/src/$(ARCH)/qti_helpers.S	\
				$(QTI_PLAT_PATH)/common/src/$(ARCH)/a53.S	\
				$(QTI_PLAT_PATH)/common/src/$(ARCH)/qti_a53.S \
				$(QTI_PLAT_PATH)/common/src/qti_stack_protector.c	\
				$(QTI_PLAT_PATH)/common/src/qti_common.c		\
				$(QTI_PLAT_PATH)/common/src/qti_bl31_setup.c		\
				$(QTI_PLAT_PATH)/common/src/qti_gic_v2.c		\
				$(QTI_PLAT_PATH)/common/src/qti_interrupt_svc.c		\
				$(QTI_PLAT_PATH)/common/src/qti_syscall.c		\
				$(QTI_PLAT_PATH)/common/src/qti_topology.c		\
				$(QTI_PLAT_PATH)/common/src/qti_pm.c			\
				$(QTI_PLAT_PATH)/qtiseclib/src/qtiseclib_cb_interface.c	\


PLAT_INCLUDES		:=	-Idrivers/arm/gic/common/				\
				-Idrivers/arm/gic/v2/					\
				-Iinclude/plat/common/					\

PLAT_INCLUDES		+=	${QTI_EXTERNAL_INCLUDES}

include lib/xlat_tables_v2/xlat_tables.mk
PLAT_BL_COMMON_SOURCES	+=	${XLAT_TABLES_LIB_SRCS}					\
				plat/common/aarch64/crash_console_helpers.S


#PSCI Sources.
PSCI_SOURCES		:=	plat/common/plat_psci_common.c				\

# GIC sources.
GIC_SOURCES		:=	drivers/arm/gic/common/gic_common.c			\
				drivers/arm/gic/v2/gicv2_main.c				\
				drivers/arm/gic/v2/gicv2_helpers.c			\
				plat/common/plat_gicv2.c				\


CONSOLE_SOURCES		:=	drivers/console/aarch64/console.S			\
# Prohibit using deprecated interfaces. We rely on this for this platform.
#ERROR_DEPRECATED		:=	1


BL31_SOURCES		+=	${QTI_BL31_SOURCES}					\
				${PSCI_SOURCES}						\
				${GIC_SOURCES}						\
				${CONSOLE_SOURCES}					\


LIB_QTI_PATH	:=	${QTI_PLAT_PATH}/qtiseclib/lib/${CHIPSET}

# By default libqtisec_dbg.a used by debug variant. When this library doesn't exist,
# debug variant will use release version (libqtisec.a) of the library.
QTISECLIB = qtisec
ifneq (${DEBUG}, 0)
ifneq ("$(wildcard $(LIB_QTI_PATH)/libqtisec_dbg.a)","")
QTISECLIB = qtisec_dbg
else
$(warning Release version of qtisec library used in Debug build!!..)
endif
endif

LDFLAGS += -z max-page-size=4096 -L ${LIB_QTI_PATH}
LDLIBS += -l$(QTISECLIB)

