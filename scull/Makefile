# Comment/uncomment the following line to disable/enable debugging
#DEBUG = y

ifeq ($(DEBUG),y)
	DEBFLAGS = -O -g -DSCULL_DEBUG
else
	DEBFLAGS = -O2
endif

LDDINC=$(PWD)/../include

EXTRA_CFLAGS += $(DEBFLAGS)
EXTRA_CFLAGS += -I$(LDDINC)

ifneq ($(KERNELRELEASE),)
# Called from kernel build system

obj-m += scull.o
obj-m += scullpipe.o

# scull-objs := scull.o
scullpipe-objs := pipe.o



else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

install:
	install -m 644 scull.ko $(DESTDIR)/lib/modules/$(KERNEL_VERSION)/extra/
	install -m 644 scullpipe.ko $(DESTDIR)/lib/modules/$(KERNEL_VERSION)/extra/

endif

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod \
		modules.order *.symvers

depend .depend dep:
	$(CC) $(EXTRA_CFLAGS) -M *.c > .depend

ifeq (.depend,$(wildcard .depend))
include .depend
endif
