#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS example multi-thread frontend
#
#  $Id: Makefile 4647 2009-12-08 07:52:56Z ritt $
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

#-----------------------------------------
# This is for Linux
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

ifeq ($(OSTYPE),linux)

OS_DIR = linux
OSFLAGS = -DOS_LINUX -Dextname
CFLAGS = -g -Wall
#CFLAGS = -g -O2 -Wall
# add to compile in 32-bit mode
# OSFLAGS += -m32
LIBS = -lm -lz -lutil -lnsl -lpthread -lrt
endif

#-----------------------
# MacOSX/Darwin is just a funny Linux
#
ifeq ($(OSTYPE),Darwin)
OS_DIR = darwin
OSFLAGS = -DOS_LINUX -DOS_DARWIN -fPIC -Wno-unused-function
LIBS = -lpthread
endif

#-------------------------------------------------------------------
# The following lines define directories. Adjust if necessary
#                 
INC_DIR   = $(MIDASSYS)/include
LIB_DIR   = $(MIDASSYS)/$(OS_DIR)/lib
SRC_DIR   = $(MIDASSYS)/src

#-------------------------------------------------------------------
# Frontend code name defaulted to mtfe in this example.
# comment out the line and run your own frontend as follow:
# gmake UFE=my_frontend
#
UFE = mtfe

####################################################################
# Lines below here should not be edited
####################################################################

# MIDAS library
LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = gcc
CXX = g++
CFLAGS += -g -I$(INC_DIR)
LDFLAGS +=

all: $(UFE)

$(UFE): $(LIB) $(LIB_DIR)/mfe.o $(UFE).o
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $(UFE) $(UFE).o $(LIB_DIR)/mfe.o $(LIB) $(LIBS)

%.o: %.c
	$(CXX) $(USERFLAGS) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

clean::
	rm -f *.o *~ \#* $(UFE)

#end file
