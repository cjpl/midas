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

OUTNAME       = msc 
CC            = gcc -g -O2
FLAGS         = -Wall -Wuninitialized

ifeq ($(OSTYPE),linux)
CC   += -DOS_UNIX -DOS_LINUX -DHAVE_LIBUSB
LIBS  = -lusb
endif

ifeq ($(OSTYPE),darwin)
CC   += -DOS_UNIX -DOS_DARWIN -DHAVE_STRLCPY
LIBS  = -lIOKit /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation
endif

all: $(OUTNAME)

$(OUTNAME): mscb.o msc.o mscbrpc.o
	$(CC) $(FLAGS) mscb.o msc.o mscbrpc.o -o $(OUTNAME) $(LIBS)

mscbrpc.o: mscbrpc.c mscbrpc.h
	$(CC) $(FLAGS) -c mscbrpc.c

mscb.o: mscb.c mscb.h
	$(CC) $(FLAGS) -c mscb.c 

msc.o: msc.c
	$(CC) $(FLAGS) -c msc.c 

clean:
	rm *.o $(OUTNAME)

