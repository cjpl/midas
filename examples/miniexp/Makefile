#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS example frontend and analyzer
#
#  $Id:$
#
#####################################################################

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

#-----------------------------------------
# This is for OSF1
#LIBS = -lbsd -lzlib
#OSFLAGS = -DOS_OSF1 -Dextname
#FF = f77
#FFLAGS = -nofor_main -D 40000000 -T 20000000

#-----------------------------------------
# This is for Linux
OSFLAGS = -DOS_LINUX -Dextname
CFLAGS = -O3

# for f2c, uncomment following:
#LIBS = -lbsd -lm -lutil /usr/lib/libf2c.a
#FF = cc
# for egcs g77, use this
FF = g77
LIBS = -lm -lz -lutil -lnsl -lpthread

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
#                 
DRV_DIR         = ../../drivers
INC_DIR 	= /usr/local/include
LIB_DIR 	= /usr/local/lib
CERNLIB_DIR	= /cern/pro/lib

#-------------------------------------------------------------------
# List of analyzer modules
#                 
MODULES 	= 

#-------------------------------------------------------------------
# Hardware driver can be (camacnul, kcs2926, kcs2927, hyt1331)
#
DRIVER = camacnul

#-------------------------------------------------------------------
# Frontend code name defaulted to frontend in this example.
# comment out the line and run your own frontend as follow:
# gmake UFE=my_frontend
#
UFE = frontend

####################################################################
# Lines below here should not be edited
####################################################################

# MIDAS library
LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = cc
CFLAGS += -g -I$(INC_DIR) -I$(DRV_DIR)
LDFLAGS +=

all: $(UFE) 

$(UFE): $(LIB) $(LIB_DIR)/mfe.o $(DRV_DIR)/bus/$(DRIVER).o $(UFE).c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(UFE) $(UFE).c $(DRIVER).o \
	$(LIB_DIR)/mfe.o $(LIB) $(LDFEFLAGS) $(LIBS)

$(DRIVER).o: $(DRV_DIR)/bus/$(DRIVER).c
	$(CC) $(CFLAGS) $(OSFLAGS) -c -o $(DRIVER).o $(DRV_DIR)/bus/$(DRIVER).c

analyzer: $(LIB) $(LIB_DIR)/mana.o analyzer.o $(MODULES)
	$(FF) $(FFLAGS) -o analyzer $(LIB_DIR)/mana.o analyzer.o \
	$(MODULES) $(CERNLIB_DIR)/libpacklib.a \
	$(LIB) $(LDFLAGS) $(LIBS)

fal: $(LIB) $(LIB_DIR)/fal.o $(DRV_DIR)/bus/$(DRIVER).o $(UFE).c $(MODULES) $(UFE).o analyzer.o
	$(FF) $(FFLAGS) -o fal $(LIB_DIR)/fal.o $(UFE).o $(DRIVER).o \
	analyzer.o $(MODULES) $(CERNLIB_DIR)/libpacklib.a \
	$(LIB) $(LDFEFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

clean:
	rm -f *.o *~ \#*
