#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS binaries and examples under unix
#
#  $Log$
#  Revision 1.4  1999/02/22 12:03:38  midas
#  Added dio to unix makefile
#
#  Revision 1.3  1999/01/19 09:11:24  midas
#  Added -DNO_PTY for Ultrix
#
#  Revision 1.2  1998/11/22 20:13:26  midas
#  Added fal.o to standard installation
#
#  Revision 1.1  1998/11/22 20:10:33  midas
#  Changed makefile to Makefile
#
#  Revision 1.4  1998/10/28 10:53:20  midas
#  Added -lutil for linux
#
#  Revision 1.3  1998/10/19 17:54:36  pierre
#  - Add lazylogger task
#
#  Revision 1.2  1998/10/12 12:18:55  midas
#  Added Log tag in header
#
#
#  Previous revision history
#  ------------------------------------------------------------------
#  date         by    modification
#  ---------    ---   -----------------------------------------------
#  11-NOV-96    SR    created
#  29-JAN-97    SR    added mtape utility
#  06-MAY-97    SR    included ybos in libmidas.a
#  05-SEP-97    SR    reorganized examples subtree
#  28-Oct-97   PA+SR  reorganization for multiple OS.
#  08-Jan-98    SR    Added creation of OS_DIR
#
#####################################################################
#
# Usage:
#        gmake             To compile the MIDAS library and binaries
#        gmake install     To install the library and binaries
#        gmake examples    To compile low-level examples (not necessary)
#
#
# This makefile contains conditional code to determine the operating
# system it is running under by evaluating the OSTYPE environment
# variable. In case this does not work or if the GNU make is not
# available, the conditional code has to be evaluated manually.
# Remove all ifeq - endif blocks except the one belonging to the
# current operating system. From this block remove only the first
# and the last line (the one with the ifeq and endif statement).
#
# "gmake install" will copy MIDAS binaries, the midas library and
# the midas include files to specific directories for each version.
#This allows to keep different versions on one machine. Only logical
#links are created in some system direcotries pointing to the proper
#version executables and libraries. You may change these directories
#to match your preferences.

#
# System direcotries for installation, modify if needed
#
SYSBIN_DIR = /usr/local/bin
SYSLIB_DIR = /usr/local/lib
SYSINC_DIR = /usr/local/include

#####################################################################
# Nothing needs to be modified after this line 
#####################################################################

#-----------------------
# Common flags
#
CC = cc
CFLAGS = -g -I$(INC_DIR) -I$(DRV_DIR)

#-----------------------
# OSF/1 (DEC UNIX)
#
ifeq ($(HOSTTYPE),alpha)
OSTYPE = osf1
endif

ifeq ($(OSTYPE),osf1)
OS_DIR = osf1
OSFLAGS = -DOS_OSF1 -DOS_UNIX
FFLAGS = -nofor_main -D 40000000 -T 20000000
LIBS = -lbsd
endif

#-----------------------
# Ultrix
#
ifeq ($(HOSTTYPE),mips)
OSTYPE = ultrix
endif

ifeq ($(OSTYPE),ultrix)
OS_DIR = ultrix
OSFLAGS = -DOS_ULTRIX -DOS_UNIX -DNO_PTY
LIBS =
endif

#-----------------------
# FreeBSD
#
ifeq ($(OSTYPE), FreeBSD)
OS_DIR = freeBSD
OSFLAGS = -DOS_FREEBSD -DOS_UNIX
LIBS = -lbsd -lcompat
endif

#-----------------------
# This is for Linux
#
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

ifeq ($(OSTYPE),linux)
OS_DIR = linux
OSFLAGS = -DOS_LINUX -DOS_UNIX
LIBS = -lbsd -lutil
endif

#-----------------------
# This is for Solaris
#
ifeq ($(OSTYPE),solaris)
CC = gcc
OS_DIR = solaris
OSFLAGS = -DOS_SOLARIS -DOS_UNIX
LIBS = -lsocket -lnsl
endif

#####################################################################
# end of conditional code
#####################################################################

#
# Midas directories
#
INC_DIR  = include
SRC_DIR  = src
UTL_DIR  = utils
DRV_DIR  = drivers
EXAM_DIR = examples
ZLIB_DIR = ./zlib-1.0.4

#
# Midas operating system dependent directories
#
LIB_DIR  = $(OS_DIR)/lib
BIN_DIR  = $(OS_DIR)/bin

#
# targets
#
EXAMPLES = $(BIN_DIR)/consume $(BIN_DIR)/produce \
	$(BIN_DIR)/rpc_test $(BIN_DIR)/msgdump $(BIN_DIR)/minife \
	$(BIN_DIR)/minirc $(BIN_DIR)/odb_test

OBJS =  $(LIB_DIR)/midas.o $(LIB_DIR)/system.o $(LIB_DIR)/mrpc.o \
	$(LIB_DIR)/odb.o $(LIB_DIR)/ybos.o $(LIB_DIR)/ftplib.o

LIB =   $(LIB_DIR)/libmidas.a

all:    $(OS_DIR) $(LIB_DIR) $(BIN_DIR) \
	$(LIB_DIR)/mana.o $(LIB_DIR)/mfe.o \
	$(LIB_DIR)/fal.o $(BIN_DIR)/dio.o \
	$(BIN_DIR)/mserver $(BIN_DIR)/mhttpd \
	$(BIN_DIR)/mlogger $(BIN_DIR)/odbedit \
	$(BIN_DIR)/mtape $(BIN_DIR)/mhist \
	$(BIN_DIR)/mstat $(BIN_DIR)/mcnaf \
	$(BIN_DIR)/mdump $(BIN_DIR)/lazylogger

examples: $(EXAMPLES)

#####################################################################

#
# create library and binary directories
#

$(OS_DIR):
	@if [ ! -d  $(OS_DIR) ] ; then \
           echo "Making directory $(OS_DIR)" ; \
           mkdir $(OS_DIR); \
        fi;

$(LIB_DIR):
	@if [ ! -d  $(LIB_DIR) ] ; then \
           echo "Making directory $(LIB_DIR)" ; \
           mkdir $(LIB_DIR); \
        fi;

$(BIN_DIR):
	@if [ ! -d  $(BIN_DIR) ] ; then \
           echo "Making directory $(BIN_DIR)" ; \
           mkdir $(BIN_DIR); \
        fi;

#
# main binaries
#

$(BIN_DIR)/mserver:  $(LIB) $(SRC_DIR)/mserver.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/mserver $(SRC_DIR)/mserver.c $(LIB) $(LIBS)
$(BIN_DIR)/mhttpd:  $(LIB) $(SRC_DIR)/mhttpd.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/mhttpd $(SRC_DIR)/mhttpd.c $(LIB) $(LIBS)
$(BIN_DIR)/mlogger:  $(LIB) $(SRC_DIR)/mlogger.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/mlogger $(SRC_DIR)/mlogger.c $(LIB) $(LIBS)
$(BIN_DIR)/odbedit: $(LIB) $(SRC_DIR)/odbedit.c $(SRC_DIR)/cmdedit.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/cmdedit.o $(SRC_DIR)/cmdedit.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/odbedit.o $(SRC_DIR)/odbedit.c	
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/odbedit $(LIB_DIR)/odbedit.o $(LIB_DIR)/cmdedit.o $(LIB)  $(LIBS)

#
# examples
#

$(BIN_DIR)/consume: $(LIB) $(EXAM_DIR)/lowlevel/consume.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/consume $(EXAM_DIR)/lowlevel/consume.c $(LIB) $(LIBS)
$(BIN_DIR)/produce: $(LIB) $(EXAM_DIR)/lowlevel/produce.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/produce $(EXAM_DIR)/lowlevel/produce.c $(LIB) $(LIBS)
$(BIN_DIR)/rpc_test: $(LIB) $(EXAM_DIR)/lowlevel/rpc_test.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/rpc_test $(EXAM_DIR)/lowlevel/rpc_test.c $(LIB) $(LIBS)
$(BIN_DIR)/msgdump: $(LIB) $(EXAM_DIR)/basic/msgdump.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/msgdump $(EXAM_DIR)/basic/msgdump.c $(LIB) $(LIBS)
$(BIN_DIR)/minife: $(LIB) $(EXAM_DIR)/basic/minife.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/minife  $(EXAM_DIR)/basic/minife.c $(LIB) $(LIBS)
$(BIN_DIR)/minirc: $(LIB) $(EXAM_DIR)/basic/minirc.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/minirc  $(EXAM_DIR)/basic/minirc.c $(LIB) $(LIBS)
$(BIN_DIR)/odb_test: $(LIB) $(EXAM_DIR)/basic/odb_test.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/odb_test  $(EXAM_DIR)/basic/odb_test.c $(LIB) $(LIBS)

#
# midas library
#

$(LIB): $(OBJS)
	rm -f $(LIB)
	ld -o $(LIB) -r $(OBJS)

#
# library objects
#

$(LIB_DIR)/midas.o: $(SRC_DIR)/midas.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h $(INC_DIR)/mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/midas.o $(SRC_DIR)/midas.c
$(LIB_DIR)/system.o: $(SRC_DIR)/system.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h $(INC_DIR)/mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/system.o $(SRC_DIR)/system.c
$(LIB_DIR)/mrpc.o: $(SRC_DIR)/mrpc.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/mrpc.o $(SRC_DIR)/mrpc.c
$(LIB_DIR)/odb.o: $(SRC_DIR)/odb.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h $(INC_DIR)/mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/odb.o $(SRC_DIR)/odb.c
$(LIB_DIR)/ybos.o: $(SRC_DIR)/ybos.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h $(INC_DIR)/mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/ybos.o $(SRC_DIR)/ybos.c
$(LIB_DIR)/ftplib.o: $(SRC_DIR)/ftplib.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/ftplib.o $(SRC_DIR)/ftplib.c

#
# frontend and backend framework
#

$(LIB_DIR)/mfe.o: $(SRC_DIR)/mfe.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h $(INC_DIR)/mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/mfe.o $(SRC_DIR)/mfe.c
$(LIB_DIR)/mana.o: $(SRC_DIR)/mana.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h $(INC_DIR)/mrpc.h
	$(CC) -Dextname -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/mana.o $(SRC_DIR)/mana.c
$(LIB_DIR)/fal.o: $(SRC_DIR)/fal.c $(INC_DIR)/msystem.h $(INC_DIR)/midas.h $(INC_DIR)/midasinc.h $(INC_DIR)/mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $(LIB_DIR)/fal.o $(SRC_DIR)/fal.c

$(BIN_DIR)/dio.o: $(DRV_DIR)/dio.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/dio $(DRV_DIR)/dio.c

#
# utilities
#

$(BIN_DIR)/mdump:	$(LIB) $(UTL_DIR)/mdump.c
	$(CC) $(CFLAGS) $(OSFLAGS) $(CC_INCLUDES) -o $(BIN_DIR)/mdump $(UTL_DIR)/mdump.c $(LIB) $(LIBS)
$(BIN_DIR)/mcnaf:	$(LIB) $(UTL_DIR)/mcnaf.c $(DRV_DIR)/camacrpc.c
	$(CC) $(CFLAGS) $(OSFLAGS) $(CC_INCLUDES) -o $(BIN_DIR)/mcnaf $(UTL_DIR)/mcnaf.c $(DRV_DIR)/camacrpc.c $(LIB) $(LIBS)
$(BIN_DIR)/mtape:  $(LIB) $(UTL_DIR)/mtape.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/mtape  $(UTL_DIR)/mtape.c $(LIB)  $(LIBS)
$(BIN_DIR)/mhist:  $(LIB) $(UTL_DIR)/mhist.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(BIN_DIR)/mhist  $(UTL_DIR)/mhist.c $(LIB)  $(LIBS)
$(BIN_DIR)/mstat:  $(LIB) $(UTL_DIR)/mstat.c 
	$(CC) $(CFLAGS) $(OSFLAGS) $(CC_INCLUDES) -o $(BIN_DIR)/mstat $(UTL_DIR)/mstat.c $(LIB) $(LIBS)
$(BIN_DIR)/lazylogger:  $(LIB) $(UTL_DIR)/lazylogger.c 
	$(CC) $(CFLAGS) $(OSFLAGS) $(CC_INCLUDES) -o $(BIN_DIR)/lazylogger $(UTL_DIR)/lazylogger.c $(LIB) $(LIBS)

#####################################################################

install:
# system programs
	@echo "... "
	@echo "... Installing programs to $(SYSBIN_DIR)"
	@echo "... "

	@if [ ! -d  $(SYSBIN_DIR) ] ; then \
          echo "Making directory $(SYSBIN_DIR)" ; \
          mkdir -p $(SYSBIN_DIR); \
        fi;

	@for i in mserver mhttpd odbedit mlogger ; \
	  do \
	  echo $$i ; \
	  cp $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
	  chmod 755 $(SYSBIN_DIR)/$$i ; \
	  done

# utilities
	@echo "... "
	@echo "... Installing utilities to $(SYSBIN_DIR)"
	@echo "... "

	@for i in mhist mtape mstat lazylogger mdump mcnaf dio ; \
	  do \
	  echo $$i ; \
	  cp $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
	  chmod 755 $(SYSBIN_DIR)/$$i ; \
	  done ; \
        chmod +s $(SYSBIN_DIR)/dio

# include
	@echo "... "
	@echo "... Installing include files to $(SYSINC_DIR)"
	@echo "... "

	@if [ ! -d  $(SYSINC_DIR) ] ; then \
          echo "Making directory $(SYSINC_DIR)" ; \
          mkdir -p $(SYSINC_DIR); \
        fi;

	@for i in midas msystem midasinc mrpc ybos cfortran hbook hardware mcstd mfbstd ; \
	  do \
	  echo $$i.h ; \
	  cp $(INC_DIR)/$$i.h $(SYSINC_DIR) ; \
	  chmod 644 $(SYSINC_DIR)/$$i.h ; \
	  done

# library + objects
	@echo "... "
	@echo "... Installing library and objects to $(SYSLIB_DIR)"
	@echo "... "

	@if [ ! -d  $(SYSLIB_DIR) ] ; then \
          echo "Making directory $(SYSLIB_DIR)" ; \
          mkdir -p $(SYSLIB_DIR); \
        fi;

	@for i in libmidas.a mana.o mfe.o fal.o ; \
	  do \
	  echo $$i ; \
	  cp $(LIB_DIR)/$$i $(SYSLIB_DIR) ; \
	  chmod 644 $(SYSLIB_DIR)/$$i ; \
	  done

	@if [ -d  $(ZLIB_DIR) ] ; then \
	  cp $(ZLIB_DIR)/zlib.h $(SYSINC_DIR) ;\
	  chmod 644 $(SYSINC_DIR)/zlib.h ;\
	  cp $(ZLIB_DIR)/zconf.h $(SYSINC_DIR) ;\
	  chmod 644 $(SYSINC_DIR)/zconf.h ;\
	  cp $(ZLIB_DIR)/libz.a $(SYSLIB_DIR) ;\
	  chmod 644 $(SYSLIB_DIR)/libz.a ;\
        fi;


clean:
	rm -f $(LIB_DIR)/*.o *~ \#*
