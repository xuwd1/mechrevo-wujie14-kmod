# Makefile based on https://github.com/nix-community/acpi_call/blob/master/Makefile

obj-m := wujie14.o
wujie14-objs := wujie14-km.o wujie14-perfmode.o wujie14-wmi-event.o wujie14-kb.o 

KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build
VERSION ?= $(shell cat VERSION)

default:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

install:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules_install
