#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS example frontend and analyzer
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

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

#-----------------------------------------
# This is for Linux
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

ifeq ($(OSTYPE),linux)

OS_DIR = linux
OSFLAGS = -DOS_LINUX -Dextname
CFLAGS = -m32 -g -O2 -Wall
LIBS = -lm -lz -lutil -lnsl -lpthread
endif

#-----------------------
# MacOSX/Darwin is just a funny Linux
#
ifeq ($(OSTYPE),Darwin)
OSTYPE = darwin
endif

ifeq ($(OSTYPE),darwin)
OS_DIR = darwin
FF = cc
OSFLAGS = -DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY -DAbsoftUNIXFortran -fPIC -Wno-unused-function
LIBS = -lpthread
SPECIFIC_OS_PRG = $(BIN_DIR)/mlxspeaker
NEED_STRLCPY=
NEED_RANLIB=1
NEED_SHLIB=
NEED_RPATH=

endif

#-----------------------------------------
# ROOT flags and libs
#
ifdef ROOTSYS
ROOTCFLAGS := $(shell  $(ROOTSYS)/bin/root-config --cflags)
ROOTCFLAGS += -DHAVE_ROOT -DUSE_ROOT
ROOTLIBS   := $(shell  $(ROOTSYS)/bin/root-config --libs) -Wl,-rpath,$(ROOTSYS)/lib
ROOTLIBS   += -lThread
else
missroot:
	@echo "...";
	@echo "Missing definition of environment variable 'ROOTSYS' !";
	@echo "...";
endif
#-------------------------------------------------------------------
# The following lines define directories. Adjust if necessary
#                 
DRV_DIR   = $(MIDASSYS)/drivers/vme
INC_DIR   = $(MIDASSYS)/include
LIB_DIR   = $(MIDASSYS)/$(OS_DIR)/lib
SRC_DIR   = $(MIDASSYS)/src
VME_DIR   = /home/olchansk/daq/vmisft-7433-3.4-KO1/vme_universe
#-------------------------------------------------------------------
# List of analyzer modules
#
MODULES   = adccalib.o adcsum.o scaler.o

#-------------------------------------------------------------------
# Hardware driver can be (camacnul, kcs2926, kcs2927, hyt1331)
#
DRIVER = vmeio v1190B

#-------------------------------------------------------------------
# Frontend code name defaulted to frontend in this example.
# comment out the line and run your own frontend as follow:
# gmake UFE=my_frontend
#
UFE = fevmemodules

####################################################################
# Lines below here should not be edited
####################################################################

# MIDAS library
LIB = $(LIB_DIR)/libmidas.a -lvme

# compiler
CC = gcc
CXX = g++
CFLAGS += -g -I. -I$(INC_DIR) -I$(DRV_DIR) -I$(DRV_DIR)/vmic -I$(VME_DIR)/include -L$(VME_DIR)/lib
LDFLAGS +=

all: $(UFE) 

$(UFE): $(LIB) $(LIB_DIR)/mfe.o vmicvme.o vmeio.o v1190B.o v792.o v1729.o vf48.o $(UFE).c $(SRC_DIR)/cnaf_callback.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(UFE) $(UFE).c \
	vmicvme.o vmeio.o v1190B.o v792.o vf48.o v1729.o $(LIB_DIR)/mfe.o $(LIB) \
	$(LDFEFLAGS) $(LIBS)

vmicvme.o: $(DRV_DIR)/vmic/vmicvme.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

vmeio.o: $(DRV_DIR)/vmeio.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

v1190B.o: $(DRV_DIR)/v1190B.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

v792.o: $(DRV_DIR)/v792.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

vf48.o: $(DRV_DIR)/vf48.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

v1729.o: $(DRV_DIR)/v1729.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

# $(DRIVER).o: $(DRV_DIR)/$(DRIVER).c
# 	$(CC) $(CFLAGS) $(OSFLAGS) -c -o $(DRIVER).o $(DRV_DIR)/$(DRIVER).c

analyzer: $(LIB) $(LIB_DIR)/rmana.o analyzer.o $(MODULES)
	$(CXX) $(CFLAGS) -o $@ $(LIB_DIR)/rmana.o analyzer.o $(MODULES) \
	$(LIB) $(LDFLAGS) $(ROOTLIBS) $(LIBS)

%.o: %.c experim.h
	$(CXX) $(USERFLAGS) $(ROOTCFLAGS) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

clean::
	rm -f *.o *~ \#*

#end file
