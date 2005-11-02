#
#  Makefile for msc executable under linux
#
#  27.11.2002
#  Tomasz Brys
#  tomasz.brys@psi.ch
#

# get OS type from shell
OSTYPE = $(shell uname)

ifeq ($(OSTYPE),Darwin)
OSTYPE = darwin
endif

ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

# directory where mxml.c and strlcpy.c resides
MXMLDIR       = ../../mxml/

# directory where musbstd.h resides
MIDASINC      = $(MIDASSYS)/include

# directory where musbstd.c resides
USBDIR        = $(MIDASSYS)/drivers/usb/

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

