PWD=$(shell pwd)
VER=$(shell uname -r)
KERNEL_BUILD=/lib/modules/$(VER)/build

obj-m += usercounter.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	$(MAKE) -C $(KERNEL_BUILD) M=$(PWD) modules \
	INSTALL_MOD_PATH=$(INSTALL_ROOT) modules_install
