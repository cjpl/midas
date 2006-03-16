########################################################
#  Makefile for msc executable under linux
#
#  Enter just 
#    "make" 
#  if midas and mxml are already installed on 
#  your system, or enter
#    "make NOMIDAS=1" 
#  if you have no midas installed and got mscb 
#  from the mscbxxx.zip file. In that case, the 
#  files the mxml and midas subdirectories below 
#  the mscb directory are used.
#
#
#  $Id:$

# get OS type from shell
OSTYPE = $(shell uname)

ifeq ($(OSTYPE),Darwin)
OSTYPE = darwin
endif

ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

# directory where mxml.c and strlcpy.c resides
ifdef NOMIDAS
MXMLDIR       = mxml/
else
MXMLDIR       = ../../mxml/
endif

# directory where musbstd.h resides
ifdef NOMIDAS
MIDASINC      = midas/include
else
MIDASINC      = $(MIDASSYS)/include
endif

# directory where musbstd.c resides
ifdef NOMIDAS
USBDIR        = midas/drivers/usb/
else
USBDIR        = $(MIDASSYS)/drivers/usb/
endif

OUTNAME       = msc 
CC            = gcc -g -O2
FLAGS         = -Wall -Wuninitialized -I$(MXMLDIR) -I$(MIDASINC)

ifeq ($(OSTYPE),linux)
CC   += -DOS_LINUX -DHAVE_LIBUSB
LIBS  = -lusb
endif

ifeq ($(OSTYPE),darwin)
CC   += -DOS_DARWIN
LIBS  = -lIOKit /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation
endif

all: $(OUTNAME)

$(OUTNAME): mscb.o msc.o mscbrpc.o strlcpy.o mxml.o musbstd.o
	$(CC) $(FLAGS) mscb.o msc.o mscbrpc.o mxml.o musbstd.o strlcpy.o -o $(OUTNAME) $(LIBS)

mscbrpc.o: mscbrpc.c mscbrpc.h
	$(CC) $(FLAGS) -c mscbrpc.c

mscb.o: mscb.c mscb.h
	$(CC) $(FLAGS) -c mscb.c 

msc.o: msc.c
	$(CC) $(FLAGS) -c msc.c 

strlcpy.o: $(MXMLDIR)strlcpy.c
	$(CC) $(FLAGS) -o strlcpy.o -c $(MXMLDIR)strlcpy.c

mxml.o: $(MXMLDIR)mxml.c
	$(CC) $(FLAGS) -o mxml.o -c $(MXMLDIR)mxml.c

musbstd.o: $(USBDIR)musbstd.c
	$(CC) $(FLAGS) -o musbstd.o -c $(USBDIR)musbstd.c

clean:
	rm *.o $(OUTNAME)

