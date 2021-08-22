# Makefile for OpenWRT system
INCLUDE_DIR += ./inc
CFLAGS += -I$(INCLUDE_DIR)

RM := rm -rf

RMNETCTL_DIR := ./src
RMNETCLI_DIR := ./cli

LIBRMNETCTL := librmnetctl.so
RMNETCLI := rmnetcli

all: $(LIBRMNETCTL) $(RMNETCLI)

# Steps to compile librmnetctl
LIBRMNETCTL_SRCS := $(RMNETCTL_DIR)/librmnetctl.c
LIBRMNETCTL_OBJS := $(LIBRMNETCTL_SRCS:.c=.o)

$(LIBRMNETCTL): $(LIBRMNETCTL_OBJS)
	$(CC) $(LIBRMNETCTL_OBJS) -shared -o $@
	mkdir -p lib
	mv librmnetctl.so lib/

# Steps to compile rmnetcli
LIBPATH := ./lib
LDFLAGS += -L$(LIBPATH)

RMNETCLI_SRCS := $(RMNETCLI_DIR)/rmnetcli.c
RMNETCLI_OBJS := $(RMNETCLI_SRCS:.c=.o)

$(RMNETCLI): $(RMNETCLI_OBJS)
	$(CC) $(LDFLAGS) $(EXTRA_CFLAGS) $(RMNETCLI_OBJS) -o $@ -lrmnetctl
	mkdir -p bin
	mv rmnetcli bin/

clean:
	$(RM) lib/
	$(RM) bin/
	$(RM) *.o
