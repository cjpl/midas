#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     UNIX Makefile for MIDAS mini example experiment
#
#  $Log$
#  Revision 1.1  1999/02/22 12:24:55  midas
#  Initial revision
#
#
#####################################################################

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

# This is for Linux ----------------
LIBS = -lbsd -lm -lutil
FF = g77
OSFLAGS = -DOS_LINUX -DOS_UNIX -Dextname

# for f2c instead of egcs g77, uncomment following lines
#FF = cc
#LIBS = -lbsd -lm -lutil /usr/lib/libf2c.a

# This is for OSF1 -----------------
#LIBS = -lbsd
#OSFLAGS = -DOS_OSF1 -DOS_UNIX -Dextname
#FF = f77
#FFLAGS = -nofor_main -D 40000000 -T 20000000

# This is for Ultrix ---------------
#LIBS =
#OSFLAGS = -DOS_ULTRIX -DOS_UNIX -Dextname

# This is for FreeBSD --------------
#LIBS = -lbsd -lcompat
#OSFLAGS = -DOS_FREEBSD -DOS_UNIX

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
#                 
INC_DIR 	= /usr/local/include
LIB_DIR 	= /usr/local/lib
CERNLIB_DIR	= /cern/pro/lib

#-------------------------------------------------------------------
# Define CAMAC driver linked to the frontend program
#                 
CAMAC_DRV	= camacnul.o
#CAMAC_DRV	= hyt1331.o


####################################################################
# Lines below here should not be edited
####################################################################

LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = cc
CFLAGS = -g -I$(INC_DIR)
LDFLAGS =

all: frontend analyzer

frontend:  $(LIB) $(LIB_DIR)/mfe.o frontend.o $(CAMAC_DRV)
	$(CC) $(CFLAGS) $(OSFLAGS) -o frontend $(LIB_DIR)/mfe.o frontend.o $(CAMAC_DRV) $(LIB) $(LDFLAGS) $(LIBS)

analyzer: $(LIB) $(LIB_DIR)/mana.o analyzer.o
	$(FF) $(FFLAGS) -o analyzer $(LIB_DIR)/mana.o analyzer.o \
	$(CERNLIB_DIR)/libpacklib.a $(LIB_DIR)/libz.a \
	$(LIB) $(LDFLAGS) $(LIBS)

fal: $(LIB) $(LIB_DIR)/fal.o frontend.o analyzer.o $(CAMAC_DRV)
	$(FF) $(FFLAGS) -o fal $(LIB_DIR)/fal.o analyzer.o frontend.o \
	$(CAMAC_DRV) $(CERNLIB_DIR)/libpacklib.a $(LIB_DIR)/libz.a \
	$(LIB) $(LDFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

clean:
	rm -f *.o *~ \#*
