#
#==============================================================================
#                           COPYRIGHT NOTICE
#
#
#------------------------------------------------------------------------------
#   Description: Makefile for the Hytec 1331 CAMAC-PCI driver module (2.6 kernel)
#------------------------------------------------------------------------------
#
#==============================================================================
#

DEVMAJOR:=60 # LOCAL/EXPERIMENTAL use, see devices.txt
DEVMINOR:=
INCLUDES=$(PWD)/../hyt1331

ifneq ($(KERNELRELEASE),)
obj-m := khyt1331.o

EXTRA_CFLAGS += -DMOD_MAJOR=$(DEVMAJOR) -DMOD_NAME=\"$(PROJNAME)\" \
                -DHYT1331_VERSION=\"$(MOD_VERSION)\" -I$(INCLUDES)

else

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)

# Install paths
moduledir:= /lib/modules/$(shell uname -r)
mandir:= /usr/man

all:
	echo $(VMIINCPATH)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules clean

install: all $(DEVDIR)
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install
	install -m0755 -oroot -groot -d $(mandir)/man9
	install -m0644 -oroot -groot -D ../doc/*.9 $(mandir)/man9/
	/sbin/depmod

uninstall: clean
	rm -f $(moduledir)/extra/$(PROJNAME).*
	ls ../doc/*.9 | sed 's|../doc|$(mandir)/man9|' | xargs rm -f
	/sbin/depmod

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

# Major and minor device numbers and device names come from devices.txt
# in kernel source documentation
$(DEVDIR):
	mkdir -p $@
	mknod --mode=666 $(DEVDIR)/ctl c $(DEVMAJOR) $(DEVMINOR)

endif
