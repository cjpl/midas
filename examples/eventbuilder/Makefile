#####################################################################
#
#  Name:         Makefile
#  Created by:   Pierre-Andre Amaudruz
#
#  Contents:     Makefile for mevb MIDAS example 
#
#  $Log$
#  Revision 1.1.1.2  2002/01/17 21:37:44  pierre
#  Initial Version
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
CFLAGS = 

# for f2c, uncomment following:
#LIBS = -lbsd -lm -lutil /usr/lib/libf2c.a
#FF = cc
# for egcs g77, use this
FF = g77
LIBS = -lm -lz -lutil -lnsl

#-------------------------------------------------------------------
# The following lines define direcories. Adjust it if necessary.
#                 
DRV_DIR         = ../../drivers
INC_DIR         = /usr/local/include
LIB_DIR         = /usr/local/lib
CERNLIB_DIR     = /cern/pro/lib

#-------------------------------------------------------------------
# Hardware driver can be (camacnul, kcs2926, kcs2927, hyt1331)
#
DRIVER = camacnul

####################################################################
# Lines below here should not be edited
####################################################################

# MIDAS library
LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = cc
CFLAGS += -g -I$(INC_DIR) -I$(DRV_DIR)
LDFLAGS +=

all: mevb fe1 fe2

fe1: $(LIB) $(LIB_DIR)/mfe.o $(DRV_DIR)/bus/$(DRIVER).o fe1.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o fe1 fe1.c $(DRIVER).o \
	$(LIB_DIR)/mfe.o $(LIB) $(LDFEFLAGS) $(LIBS)

fe2: $(LIB) $(LIB_DIR)/mfe.o $(DRV_DIR)/bus/$(DRIVER).o fe2.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o fe2 fe2.c $(DRIVER).o \
	$(LIB_DIR)/mfe.o $(LIB) $(LDFEFLAGS) $(LIBS)

yfe1: $(LIB) $(LIB_DIR)/mfe.o $(DRV_DIR)/bus/$(DRIVER).o yfe1.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o yfe1 yfe1.c $(DRIVER).o \
	$(LIB_DIR)/mfe.o $(LIB) $(LDFEFLAGS) $(LIBS)

yfe2: $(LIB) $(LIB_DIR)/mfe.o $(DRV_DIR)/bus/$(DRIVER).o yfe2.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o yfe2 yfe2.c $(DRIVER).o \
	$(LIB_DIR)/mfe.o $(LIB) $(LDFEFLAGS) $(LIBS)

$(DRIVER).o: $(DRV_DIR)/bus/$(DRIVER).c
	$(CC) $(CFLAGS) $(OSFLAGS) -c -o $(DRIVER).o $(DRV_DIR)/bus/$(DRIVER).c

mevb:   $(LIB) mevb.c ebuser.o
	$(CC) $(CFLAGS) $(OSFLAGS) -o mevb mevb.c ebuser.o $(LIB) $(LDFEFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

clean:
	rm -f *.o *~ mevb \#*


