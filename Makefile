#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS binaries and examples under unix
#
#  $Log$
#  Revision 1.15  1999/12/20 14:24:17  midas
#  Adjusted for different driver direcotries
#
#  Revision 1.14  1999/09/17 11:55:08  midas
#  Remove OS_UNIX (now defined in midas.h)
#
#  Revision 1.13  1999/09/14 15:32:09  midas
#  Added elog
#
#  Revision 1.12  1999/08/24 13:44:25  midas
#  Added odbhist
#
#  Revision 1.11  1999/08/08 19:20:21  midas
#  Added chmod +s to mhttpd
#
#  Revision 1.10  1999/08/03 13:14:33  midas
#  Removed -DPIC, added dio to normal utils compilation
#
#  Revision 1.9  1999/07/22 19:42:58  pierre
#  - Added USERFLAGS to allow other options (-DUSERFLAGS=-static)
#
#  Revision 1.8  1999/06/23 10:01:20  midas
#  Additions (shared library etc.) added by Glenn
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
#        gmake static      To make static executables for debugging
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
# You may change these directories to match your preferences.
#####################################################################

# get OS type from shell
OSTYPE = $(shell uname)

#
# System direcotries for installation, modify if needed
#

ifndef PREFIX
PREFIX     = /usr/local
endif

SYSBIN_DIR = $(PREFIX)/bin
SYSLIB_DIR = $(PREFIX)/lib
SYSINC_DIR = $(PREFIX)/include

#####################################################################
# Nothing needs to be modified after this line 
#####################################################################

#-----------------------
# Common flags
#
CC = cc
CFLAGS = -g -I$(INC_DIR) -I$(DRV_DIR) -L$(LIB_DIR)

#-----------------------
# OSF/1 (DEC UNIX)
#
ifeq ($(HOSTTYPE),alpha)
OSTYPE = osf1
endif

ifeq ($(OSTYPE),osf1)
OS_DIR = osf1
OSFLAGS = -DOS_OSF1 $(USERFLAGS)
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
OSFLAGS = -DOS_ULTRIX -DNO_PTY $(USERFLAGS)
LIBS =
endif

#-----------------------
# FreeBSD
#
ifeq ($(OSTYPE), FreeBSD)
OS_DIR = freeBSD
OSFLAGS = -DOS_FREEBSD $(USERFLAGS)
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
OSFLAGS = -DOS_LINUX -fPIC $(USERFLAGS)
LIBS = -lbsd -lutil
endif

#-----------------------
# This is for Solaris
#
ifeq ($(OSTYPE),solaris)
CC = gcc
OS_DIR = solaris
OSFLAGS = -DOS_SOLARIS $(USERFLAGS)
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

PROGS = $(BIN_DIR)/mserver $(BIN_DIR)/mhttpd \
	$(BIN_DIR)/mlogger $(BIN_DIR)/odbedit \
	$(BIN_DIR)/mtape $(BIN_DIR)/mhist \
	$(BIN_DIR)/mstat $(BIN_DIR)/mcnaf \
	$(BIN_DIR)/mdump $(BIN_DIR)/lazylogger \
	$(BIN_DIR)/mlxspeaker $(BIN_DIR)/dio \
	$(BIN_DIR)/odbhist $(BIN_DIR)/elog

OBJS =  $(LIB_DIR)/midas.o $(LIB_DIR)/system.o $(LIB_DIR)/mrpc.o \
	$(LIB_DIR)/odb.o $(LIB_DIR)/ybos.o $(LIB_DIR)/ftplib.o

LIB =   -lmidas
LIBNAME=$(LIB_DIR)/libmidas.a
SHLIB = $(LIB_DIR)/libmidas.so
VPATH = $(LIB_DIR):$(INC_DIR)

all:    $(OS_DIR) $(LIB_DIR) $(BIN_DIR) \
	$(LIBNAME) $(SHLIB) \
	$(LIB_DIR)/mana.o $(LIB_DIR)/mfe.o \
	$(PROGS)

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

$(BIN_DIR)/%:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/odbedit: $(SRC_DIR)/odbedit.c $(SRC_DIR)/cmdedit.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/odbedit.c $(SRC_DIR)/cmdedit.c $(LIB) $(LIBS)

$(PROGS): $(LIBNAME)

#
# examples
#

$(BIN_DIR)/%:$(EXAM_DIR)/lowlevel/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/%:$(EXAM_DIR)/basic/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(EXAMPLES): $(LIBNAME)

#
# midas library
#

$(LIBNAME): $(OBJS)
	rm -f $@
	ar -crv $@ $^

$(SHLIB): $(OBJS)
	rm -f $@
	ld -shared -o $@ $^

#
# library objects
#

$(LIB_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $<

$(LIB_DIR)/midas.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/system.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/mrpc.o: msystem.h midas.h mrpc.h
$(LIB_DIR)/odb.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/ybos.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/ftplib.o: msystem.h midas.h midasinc.h

#
# frontend and backend framework
#

$(LIB_DIR)/mfe.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/mana.o: $(SRC_DIR)/mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CC) -Dextname -c $(CFLAGS) $(OSFLAGS) -o $@ $<

#
# utilities
#
$(BIN_DIR)/%:$(UTL_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)


$(BIN_DIR)/mcnaf: $(UTL_DIR)/mcnaf.c $(DRV_DIR)/bus/camacrpc.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mcnaf.c $(DRV_DIR)/bus/camacrpc.c $(LIB) $(LIBS)

$(BIN_DIR)/mdump: $(UTL_DIR)/mdump.c $(SRC_DIR)/ybos.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mdump.c $(SRC_DIR)/ybos.c $(LIB) -lz $(LIBS)

$(BIN_DIR)/dio: $(UTL_DIR)/dio.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/dio.c $(LIB) $(LIBS)

#####################################################################

static:
	rm -f $(PROGS)
	make USERFLAGS=-static

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

	@for i in mserver mhttpd odbedit mlogger dio ; \
	  do \
	  echo $$i ; \
	  rm -f $(SYSBIN_DIR)/$$i ; \
	  cp $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
	  chmod 755 $(SYSBIN_DIR)/$$i ; \
	  done
	chmod +s $(SYSBIN_DIR)/dio
	chmod +s $(SYSBIN_DIR)/mhttpd

# utilities
	@echo "... "
	@echo "... Installing utilities to $(SYSBIN_DIR)"
	@echo "... "

	@for i in mhist odbhist elog mtape mstat lazylogger mdump mcnaf mlxspeaker ; \
	  do \
	  echo $$i ; \
	  rm -f $(SYSBIN_DIR)/$$i ; \
	  cp $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
	  chmod 755 $(SYSBIN_DIR)/$$i ; \
	  done

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

	@for i in libmidas.so libmidas.a mana.o mfe.o ; \
	  do \
	  echo $$i ; \
	  rm -f $(SYSLIB_DIR)/$$i ;\
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
