# SPDX-License-Identifier: GPL-2.0
# Out-of-tree build of the Linux 7.3 AMD Halo Box LED driver.
#   make                      build for the running kernel
#   make KVER=<release>       build for another installed kernel
ifneq ($(KERNELRELEASE),)

obj-m := amd_halo_led.o
ccflags-y += -include $(src)/compat.h

# Count the commas in the kernel's wmidev_invoke_method() prototype:
# 4 = the five-argument form (before 7.3), which needs the compat wrapper.
halo_wmi_commas := $(shell awk '/^int wmidev_invoke_method/,/;/' \
	$(srctree)/include/linux/wmi.h | tr -cd ',' | wc -c)
ifeq ($(halo_wmi_commas),4)
ccflags-y += -DHALO_WMI_INVOKE_NO_MIN_SIZE
endif

else

KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean
endif
