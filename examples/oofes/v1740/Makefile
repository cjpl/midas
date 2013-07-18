####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for the v1740 frontend
#
#  $Id: Makefile 3655 2007-03-21 20:51:28Z amaudruz $
#
#####################################################################
#

# If not connected to hardware, use this to simulate it and generate
# random data
SIMULATION=0

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
#
# get OS type from shell
OSTYPE = $(shell uname)

#-----------------------------------------
# This is for Linux
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

ifeq ($(OSTYPE),linux)
#OS_DIR = linux-m64
OS_DIR = linux
OSFLAGS = -DOS_LINUX -DLINUX
CFLAGS = -g -Wall -DSIMULATION=$(SIMULATION) #-fno-omit-frame-pointer 
LDFLAGS = -g -lm -lz -lutil -lnsl -lpthread -lrt -lc 
endif

#-----------------------------------------
# optimize?

# CFLAGS += -O2

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
CONET2_DIR   = $(HOME)/packages/CONET2
CAENCOMM_DIR = $(CONET2_DIR)/CAENComm_1_0_2_linux
CAENCOMM_LIB = $(CAENCOMM_DIR)/lib/x64
CAENVME_DIR  = $(CONET2_DIR)/CAENVMELib-2.23-1
CAENVME_LIB  = $(CAENVME_DIR)/lib/x64
MIDAS_INC    = $(MIDASSYS)/include
MIDAS_LIB    = $(MIDASSYS)/$(OS_DIR)/lib
MIDAS_SRC    = $(MIDASSYS)/src
MIDAS_DRV    = $(MIDASSYS)/drivers/vme
#ROOTANA      = $(HOME)/packages/rootana

####################################################################
# Lines below here should not be edited
####################################################################
#
# compiler
CC   = gcc
CXX  = g++
#
# MIDAS library
LIBMIDAS=-L$(MIDAS_LIB) -lmidas
#
# CAENComm
LIBCAENCOMM=-L$(CAENCOMM_LIB) -lCAENComm
#
# CAENVME
LIBCAENVME=-L$(CAENVME_LIB) -lCAENVME
#
# All includes
INCS = -I. -I./include -I$(MIDAS_INC) -I$(MIDAS_DRV) -I$(CAENVME_DIR)/include -I$(CAENCOMM_DIR)/include

####################################################################
# General commands
####################################################################

all: fe
	@echo "***** Finished"
	@echo "***** Use 'make doc' to build documentation"

fe : feoV1740.exe

doc ::
	doxygen
	@echo "***** Use firefox --no-remote doc/html/index.html to view if outside gateway"

####################################################################
# Libraries/shared stuff
####################################################################

ov1740.o : $(MIDAS_DRV)/ov1740.c
	$(CC) -c $(CFLAGS) $(INCS) $< -o $@ 

####################################################################
# Single-thread frontend
####################################################################

v1740CONET2.o : v1740CONET2.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -Ife -c $< -o $@

feoV1740.exe: $(LIB) $(MIDAS_LIB)/mfe.o feoV1740.o ov1740.o v1740CONET2.o
	$(CXX) $(OSFLAGS) feoV1740.o v1740CONET2.o ov1740.o $(MIDAS_LIB)/mfe.o $(LIB) $(LIBMIDAS) $(LIBCAENCOMM) $(LIBCAENVME) -o $@ $(LDFLAGS)

feoV1740.o : feoV1740.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) $(INCS) -Ife -c $< $(LIBCAENVME) -o $@

####################################################################
# Clean
####################################################################

clean::
	rm -f *.o *.exe
	rm -f *~
	rm -rf html

#end file
