# Makefile based on https://github.com/nix-community/acpi_call/blob/master/Makefile
# This makefile is for dkms and testing
# NOT recommended for actual installing

obj-m := wujie14.o
wujie14-objs := wujie14-km.o wujie14-perfmode.o wujie14-wmi-event.o wujie14-kb.o 

KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build
VERSION ?= $(shell cat VERSION)

default:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

# This install target actually install the ko.zst to /lib/modules/$(uname -r)/update
# And then a depmod is called 
# Side-effects of this target should can be removed by rm'ing the ko.zst and calling a depmod -a
install:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules_install
