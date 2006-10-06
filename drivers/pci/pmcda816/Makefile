#
#==============================================================================
#                           COPYRIGHT NOTICE
#
#   Copyright (C) 2002-2003 GE Fanuc
#   International Copyright Secured.  All Rights Reserved.
#
#------------------------------------------------------------------------------
#   Description: Makefile for the GE Fanuc VMEbus driver module (2.6 kernel)
#------------------------------------------------------------------------------
#
#==============================================================================
#

MODNAME:=pmcda816
DEVMAJOR:=222
DEVMINOR:=0

obj-m := pmcda816_module.o
EXTRA_CFLAGS += -DMOD_MAJOR=$(DEVMAJOR) -DMOD_NAME=\"$(MODNAME)\" 

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

all: pmcda816 pmcda816_module.ko device

pmcda816_module.ko: pmcda816_module.c
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean::
	-rm -fv *.o *.ko *.mod.c pmcda816

device: /dev/pmcda816

install: pmcda816_module.ko
	cp -pf $^ /root/

/dev/pmcda816:
	-rm -f $@
	mknod --mode=666 $@ c $(DEVMAJOR) $(DEVMINOR)

%: %.cxx
	g++ -m32 -o $@ -O2 -g -Wall -Wuninitialized $<

#endif
