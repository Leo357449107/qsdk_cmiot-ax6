PWD := $(shell pwd)

.PHONY:drv cli install clean

all: drv cli install

drv:
	export SUBDIR=$(PWD)/drv && make -C drv/

cli:
	export SUBDIR=$(PWD)/cli && make -C cli/

install:
	export SUBDIR=$(PWD)/drv && make install -C drv/
	export SUBDIR=$(PWD)/cli && make install -C cli/

clean:
	export SUBDIR=$(PWD)/drv && make clean -C drv/
	export SUBDIR=$(PWD)/cli && make clean -C cli/
	rm -rf $(PWD)/obj
	rm -f Module.symvers modules.order

