#
# Makefile for the drm device driver.  This driver provides support for the
# Direct Rendering Infrastructure (DRI) in XFree86 4.1.0 and higher.

ccflags-y := -Iinclude/drm
via_chrome9-objs    :=  via_chrome9_drv.o via_chrome9_drm.o via_chrome9_mm.o via_chrome9_dma.o via_chrome9_verifier.o
obj-m	+= via_chrome9.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	@rm -fr *.ko *.o
install:
	@if [ ! -f ./via_chrome9.ko ]; then \
		$(MAKE) -C $(KERNELDIR) M=$(PWD) modules;\
	fi
	@if [ ! -d /lib/modules/$(shell uname -r)/kernel/drivers/gpu/drm/via_chrome9 ]; then \
		mkdir -p /lib/modules/$(shell uname -r)/kernel/drivers/gpu/drm/via_chrome9; \
	fi
		@cp ./via_chrome9.ko /lib/modules/$(shell uname -r)/kernel/drivers/gpu/drm/via_chrome9/
