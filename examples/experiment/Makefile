#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS example frontend and analyzer
#
#  Revision history
#  ------------------------------------------------------------------
#  date         by    modification
#  ---------    ---   -----------------------------------------------
#  05-SEP-97    SR    created
#
#  $Log$
#  Revision 1.3  1999/12/10 00:45:48  pierre
#  - Add DRIVER selection and UFE option
#
#####################################################################

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

# This is for OSF1 -----------------------
#LIBS = -lbsd
#OSFLAGS = -DOS_OSF1 -Dextname
#FF = f77
#FFLAGS = -nofor_main -D 40000000 -T 20000000

# This is for Ultrix ---------------------
#LIBS =
#OSFLAGS = -DOS_ULTRIX -Dextname

# This is for FreeBSD --------------------
#LIBS = -lbsd -lcompat
#OSFLAGS = -DOS_FREEBSD 
 
# This is for Linux ----------------------
OSFLAGS   = -DOS_LINUX 
LDFEFLAGS = -DMODULE -D__KERNEL__
CFEFLAGS  = -O3
# for f2c, uncomment following:
#LIBS = -lbsd -lm -lutil /usr/lib/libf2c.a
#FF = cc
# for egcs g77, use this
FF = g77
LIBS = -lm -lz -lutil

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
#                 
DRV_DIR         = /midas/drivers
INC_DIR 	= /usr/local/include
LIB_DIR 	= /usr/local/lib
CERNLIB_DIR	= /cern/pro/lib

#-------------------------------------------------------------------
# List of analyzer modules
#                 
MODULES 	= adccalib.o adcsum.o scaler.o
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

LIB = $(LIB_DIR)/libmidas.a
INCLUDES = -I$(INC_DIR)

# compiler
CC = cc
CFLAGS = -g 
LDFLAGS =


all: $(DRIVER).o $(UFE) analyzer

$(UFE): $(LIB) $(LIB_DIR)/mfe.o $(DRIVER).o $(UFE).o
	$(CC) $(CFEFLAGS) $(OSFLAGS) $(INCLUDES) -c -o $(UFE).o $(UFE).c 
	$(CC) $(CFEFLAGS) -o $(UFE) $(UFE).o $(DRIVER).o $(LIB_DIR)/mfe.o \
	$(LIB) $(LDFEFLAGS) $(LIBS)

$(DRIVER).o: $(DRV_DIR)/$(DRIVER).c
	$(CC) $(CFEFLAGS) $(INCLUDES) $(OSFLAGS) -c -o $(DRIVER).o $(DRV_DIR)/$(DRIVER).c

analyzer: $(LIB) $(LIB_DIR)/mana.o analyzer.o $(MODULES)
	$(FF) $(FFLAGS) -o analyzer $(LIB_DIR)/mana.o analyzer.o \
	$(MODULES) $(CERNLIB_DIR)/libpacklib.a \
	$(LIB) $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFEFLAGS) $(OSFLAGS) $(INCLUDES) -c $<

clean:
	rm -f *.o *~ \#*
