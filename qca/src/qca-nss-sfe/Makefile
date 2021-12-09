#
# Makefile for Shortcut FE.
#

obj-m += qca-nss-sfe.o

qca-nss-sfe-objs := sfe_ipv4.o sfe.o sfe_init.o

ifdef SFE_SUPPORT_IPV6
qca-nss-sfe-objs += \
	sfe_ipv6.o
ccflags-y += -DSFE_SUPPORT_IPV6
endif

ccflags-y += -Werror -Wall -Iexports/
