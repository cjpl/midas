#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     UNIX Makefile for MIDAS slow control frontend
#
#  $Id$
#
#####################################################################
#
#--------------------------------------------------------------------
# The MIDASSYS should be defined prior the use of this Makefile
ifndef MIDASSYS
missmidas::
	@echo "...";
	@echo "Missing definition of environment variable 'MIDASSYS' !";
	@echo "...";
endif

# get OS type from shell
OSTYPE = $(shell uname)

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

# This is for Darwin ---------------
ifeq ($(OSTYPE),Darwin)
OS_DIR = darwin
LIBS = -lpthread
OSFLAGS = -DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY
endif

# This is for Linux ----------------
ifeq ($(OSTYPE),Linux)
OS_DIR = linux
LIBS = -lm -lutil -lpthread -lrt
OSFLAGS = -DOS_LINUX
endif

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
#                 
INC_DIR 	= $(MIDASSYS)/include
LIB_DIR 	= $(MIDASSYS)/$(OS_DIR)/lib
DRV_DIR		= $(MIDASSYS)/drivers
MSCB_DIR        = $(MIDASSYS)/mscb
MXML_DIR        = $(MIDASSYS)/../mxml

#-------------------------------------------------------------------
# Drivers needed by the frontend program
#                 
DRIVERS         = hv.o multi.o null.o nulldev.o mscbdev.o mscb.o

####################################################################
# Lines below here should not be edited
####################################################################

LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = cc
CFLAGS = -g -Wall -I$(INC_DIR) -I$(DRV_DIR) -I$(MSCB_DIR) -I$(MXML_DIR)
LDFLAGS =

all: scfe mscb_fe

scfe:  $(LIB) $(LIB_DIR)/mfe.o frontend.o $(DRIVERS)
	$(CC) -o scfe frontend.o $(LIB_DIR)/mfe.o $(DRIVERS) $(LIB) $(LDFLAGS) $(LIBS)

mscb_fe:  $(LIB) $(LIB_DIR)/mfe.o mscb_fe.o $(DRIVERS)
	$(CC) -o mscb_fe mscb_fe.o $(LIB_DIR)/mfe.o $(DRIVERS) $(LIB) $(LDFLAGS) $(LIBS)

hv.o: $(DRV_DIR)/class/hv.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

multi.o: $(DRV_DIR)/class/multi.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

nulldev.o: $(DRV_DIR)/device/nulldev.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

mscbdev.o: $(DRV_DIR)/device/mscbdev.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

mscb.o: $(MSCB_DIR)/mscb.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

null.o: $(DRV_DIR)/bus/null.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

clean:
	rm -f *.o *~ \#*

