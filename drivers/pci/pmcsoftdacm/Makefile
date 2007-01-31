#
# Makefile for the pmc-softdac-m driver
#
MODNAME:=pmcsoftdacm
DEVMAJOR:=223
DEVMINOR:=0

obj-m := $(MODNAME)_module.o
EXTRA_CFLAGS += -DMOD_MAJOR=$(DEVMAJOR) -DMOD_NAME=\"$(MODNAME)\" 

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

all: softdac pmcsoftdacm_module.ko device

%.ko: %.c
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean::
	-rm -fv *.o *.ko *.mod.c softdac

device: /dev/$(MODNAME)

install: pmcsoftdacm_module.ko
	cp -pf $^ /root/
	@echo
	@echo "Put the following commands into /etc/rc.local"
	@echo
	@echo "insmod /root/$(MODNAME)_module.ko"
	@echo "mknod --mode=666 /dev/$(MODNAME) c $(DEVMAJOR) $(DEVMINOR)"
	@echo

/dev/$(MODNAME):
	-rm -f $@
	mknod --mode=666 $@ c $(DEVMAJOR) $(DEVMINOR)

%: %.cxx
	g++ -m32 -DMAIN_ENABLE -o $@ -O2 -g -Wall -Wuninitialized $<

#endif
