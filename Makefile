#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS binaries and examples under unix
#
#  $Log$
#  Revision 1.43  2003/05/09 22:44:42  pierre
#  add ROOTSYS path to root-config
#
#  Revision 1.42  2003/04/28 11:12:55  midas
#  Adjusted ROOT flags
#
#  Revision 1.41  2003/04/20 02:59:13  olchansk
#  merge ROOT code into mana.c
#  remove MANA_LITE, replaced with HAVE_HBOOK, HAVE_ROOT
#  ROOT histogramming support almost complete,
#  ROOT TTree filling is missing
#  all ROOT code is untested, but compiles with RH-8.0, GCC-3.2, ROOT v3.05.03
#
#  Revision 1.40  2003/04/18 01:49:49  olchansk
#  Add "rmidas"
#
#  Revision 1.39  2003/04/16 23:28:56  olchansk
#  change cflags to -g -O2 -Wall -Wunitialized for maximum warnings (-Wuninitialized does not work with -O2)
#  if ROOTSYS is set, build mlogger with ROOT support
#  if ROOTSYS is set, build rmana.o with ROOT support
#
#  Revision 1.38  2003/04/08 00:05:16  olchansk
#  add rmana.o (the ROOT MIDAS analyzer)
#
#  Revision 1.37  2002/06/04 07:32:00  midas
#  Added 'melog'
#
#  Revision 1.36  2002/05/10 05:20:54  pierre
#  add MANA_LITE option on mana & fal
#
#  Revision 1.35  2002/01/24 22:26:48  pierre
#  rm elog, add cc for webpaw,odbhist
#
#  Revision 1.34  2002/01/23 11:27:32  midas
#  Removed elogd (now separate package)
#
#  Revision 1.33  2001/10/05 22:32:38  pierre
#  - added mvmestd in install include.
#  - change ybos.c to ybos.o in mdump build rule.
#
#  Revision 1.32  2001/08/07 11:04:03  midas
#  Added -lc flag for libmidas.so because of missing stat()
#
#  Revision 1.31  2001/08/07 10:40:46  midas
#  Added -w flag for HBOOK files to supress warnings cause by cfortran.h
#
#  Revision 1.30  2001/08/06 12:02:56  midas
#  Fixed typo
#
#  Revision 1.29  2001/05/23 13:17:19  midas
#  Added elogd
#
#  Revision 1.28  2001/02/19 12:42:25  midas
#  Fixed bug with missing "-Dextname" under Linux
#
#  Revision 1.27  2000/12/01 09:26:44  midas
#  Added fal.o (again?)
#
#  Revision 1.26  2000/11/20 13:42:39  midas
#  Install mcleanup
#
#  Revision 1.25  2000/11/20 13:13:06  midas
#  Fixed little bug
#
#  Revision 1.24  2000/11/20 11:40:36  midas
#  Added pmana.o for parallelized analyzer
#
#  Revision 1.23  2000/09/29 21:00:35  pierre
#  - Add SPECIFIC_OS_PRG in PROGS
#  - Define SPECIFIC_OS_PRG or each OS
#  - Add -lc in LIBS for OSF1
#  - Add $(LIBS) in SHLIB for OSF1
#
#  Revision 1.22  2000/07/21 18:30:11  pierre
#  - Added MIDAS_PREF_FLAGS for custom build
#
#  Revision 1.21  2000/05/11 14:21:34  midas
#  Added webpaw
#
#  Revision 1.20  2000/04/27 14:48:27  midas
#  Added mgd.c in mhttpd
#
#  Revision 1.19  2000/04/20 23:26:12  pierre
#  - Correct stripchart.tcl installtion
#
#  Revision 1.18  2000/04/18 20:35:05  pierre
#  - Added mchart and stripchart.tcl installation
#
#  Revision 1.17  2000/02/29 20:19:53  midas
#  Removed -lbsd for Linux
#
#  Revision 1.16  2000/02/24 19:39:22  midas
#  Add esone.h in installation
#
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
#
#  Midas preference flags
#  -DYBOS_VERSION_3_3  for YBOS up to version 3.3 
MIDAS_PREF_FLAGS  =

#####################################################################
# Nothing needs to be modified after this line 
#####################################################################

#-----------------------
# Common flags
#
CC = cc
CFLAGS = -g -O2 -Wall -I$(INC_DIR) -I$(DRV_DIR) -L$(LIB_DIR) -DINCLUDE_FTPLIB

#-----------------------
# OSF/1 (DEC UNIX)
#
ifeq ($(HOSTTYPE),alpha)
OSTYPE = osf1
endif

ifeq ($(OSTYPE),osf1)
OS_DIR = osf1
OSFLAGS = -DOS_OSF1 $(MIDAS_PREF_FLAGS) $(USERFLAGS)
FFLAGS = -nofor_main -D 40000000 -T 20000000
LIBS = -lc -lbsd
SPECIFIC_OS_PRG = 
endif

#-----------------------
# Ultrix
#
ifeq ($(HOSTTYPE),mips)
OSTYPE = ultrix
endif

ifeq ($(OSTYPE),ultrix)
OS_DIR = ultrix
OSFLAGS = -DOS_ULTRIX -DNO_PTY $(MIDAS_PREF_FLAGS) $(USERFLAGS)
LIBS =
SPECIFIC_OS_PRG = 
endif

#-----------------------
# FreeBSD
#
ifeq ($(OSTYPE), FreeBSD)
OS_DIR = freeBSD
OSFLAGS = -DOS_FREEBSD $(MIDAS_PREF_FLAGS) $(USERFLAGS)
LIBS = -lbsd -lcompat
SPECIFIC_OS_PRG = 
endif

#-----------------------
# This is for Linux
#
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

ifeq ($(OSTYPE),linux)
OS_DIR = linux
OSFLAGS = -DOS_LINUX -fPIC $(MIDAS_PREF_FLAGS) $(USERFLAGS)
LIBS = -lutil
SPECIFIC_OS_PRG = $(BIN_DIR)/mlxspeaker $(BIN_DIR)/dio
endif

#-----------------------
# This is for Solaris
#
ifeq ($(OSTYPE),solaris)
CC = gcc
OS_DIR = solaris
OSFLAGS = -DOS_SOLARIS $(MIDAS_PREF_FLAGS) $(USERFLAGS)
LIBS = -lsocket -lnsl
SPECIFIC_OS_PRG = 
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
	$(BIN_DIR)/mchart $(BIN_DIR)/stripchart.tcl \
	$(BIN_DIR)/webpaw $(BIN_DIR)/odbhist \
	$(BIN_DIR)/melog \
	$(SPECIFIC_OS_PRG)

ifdef ROOTSYS
PROGS += $(BIN_DIR)/rmidas
endif

OBJS =  $(LIB_DIR)/midas.o $(LIB_DIR)/system.o $(LIB_DIR)/mrpc.o \
	$(LIB_DIR)/odb.o $(LIB_DIR)/ybos.o $(LIB_DIR)/ftplib.o

LIBNAME=$(LIB_DIR)/libmidas.a
SHLIB = $(LIB_DIR)/libmidas.so
LIB =   -lmidas
# Uncomment this for static linking of midas executables
#LIB =   $(LIBNAME)
VPATH = $(LIB_DIR):$(INC_DIR)

all:    $(OS_DIR) $(LIB_DIR) $(BIN_DIR) \
	$(LIBNAME) $(SHLIB) \
	$(LIB_DIR)/mana.o $(LIB_DIR)/hmana.o $(LIB_DIR)/rmana.o \
	$(LIB_DIR)/mfe.o \
	$(LIB_DIR)/fal.o $(PROGS)

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


ifdef ROOTSYS
ROOTLIBS    := $(shell $(ROOTSYS)/bin/root-config --libs)
ROOTGLIBS   := $(shell $(ROOTSYS)/bin/root-config --glibs)
ROOTCFLAGS  := $(shell $(ROOTSYS)/bin/root-config --cflags)

$(BIN_DIR)/mlogger: $(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CXX) $(CFLAGS) $(OSFLAGS) -DHAVE_ROOT $(ROOTCFLAGS) -o $@ $< $(LIB) $(ROOTLIBS) $(LIBS)

$(BIN_DIR)/rmidas: $(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CXX) $(CFLAGS) $(OSFLAGS) -DHAVE_ROOT $(ROOTCFLAGS) -o $@ $< $(LIB) $(ROOTGLIBS) $(LIBS)
else

# logger without ROOT support
$(BIN_DIR)/mlogger: $(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(ROOTLIBS) $(LIBS)

endif

$(BIN_DIR)/%:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/odbedit: $(SRC_DIR)/odbedit.c $(SRC_DIR)/cmdedit.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/odbedit.c $(SRC_DIR)/cmdedit.c $(LIB) $(LIBS)

$(BIN_DIR)/mhttpd: $(SRC_DIR)/mhttpd.c $(SRC_DIR)/mgd.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/mhttpd.c $(SRC_DIR)/mgd.c $(LIB) $(LIBS) -lm

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
	ld -shared -o $@ $^ $(LIBS) -lc

#
# frontend and backend framework
#

$(LIB_DIR)/mfe.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/fal.o: $(SRC_DIR)/fal.c msystem.h midas.h midasinc.h mrpc.h

$(LIB_DIR)/fal.o: $(SRC_DIR)/fal.c msystem.h midas.h midasinc.h mrpc.h
	$(CC) -Dextname -c $(CFLAGS) $(OSFLAGS) $(MANA_OPTION) -o $@ $<
$(LIB_DIR)/mana.o: $(SRC_DIR)/mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) $(MANA_OPTION) -o $@ $<
$(LIB_DIR)/hmana.o: $(SRC_DIR)/mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CC) -Dextname -DHAVE_HBOOK -c $(CFLAGS) $(OSFLAGS) $(MANA_OPTION) -o $@ $<
ifdef ROOTSYS
$(LIB_DIR)/rmana.o: $(SRC_DIR)/mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CXX) -DHAVE_ROOT -c $(CFLAGS) $(OSFLAGS) $(ROOTCFLAGS) -o $@ $<
endif

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
# utilities
#
$(BIN_DIR)/%:$(UTL_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)


$(BIN_DIR)/mcnaf: $(UTL_DIR)/mcnaf.c $(DRV_DIR)/bus/camacrpc.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mcnaf.c $(DRV_DIR)/bus/camacrpc.c $(LIB) $(LIBS)

$(BIN_DIR)/mdump: $(UTL_DIR)/mdump.c $(LIB_DIR)/ybos.o
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mdump.c $(LIB) -lz $(LIBS)

$(BIN_DIR)/lazylogger: $(SRC_DIR)/lazylogger.c $(LIB_DIR)/ybos.o
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/lazylogger.c $(LIB) -lz $(LIBS)

$(BIN_DIR)/dio: $(UTL_DIR)/dio.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/dio.c $(LIB) $(LIBS)

$(BIN_DIR)/stripchart.tcl: $(UTL_DIR)/stripchart.tcl
	cp -f $(UTL_DIR)/stripchart.tcl $(BIN_DIR)/. 


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
	cp mcleanup $(SYSBIN_DIR)
	chmod +s $(SYSBIN_DIR)/dio
	chmod +s $(SYSBIN_DIR)/mhttpd

# utilities
	@echo "... "
	@echo "... Installing utilities to $(SYSBIN_DIR)"
	@echo "... "
	@for i in mhist melog odbhist mtape mstat lazylogger mdump mcnaf mlxspeaker mchart stripchart.tcl webpaw; \
	  do \
	  echo $$i ; \
	  rm -f $(SYSBIN_DIR)/$$i ; \
	  cp $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
	  chmod 755 $(SYSBIN_DIR)/$$i ; \
	done
	ln -fs $(SYSBIN_DIR)/stripchart.tcl $(SYSBIN_DIR)/stripchart
	chmod +s $(SYSBIN_DIR)/webpaw

# include
	@echo "... "
	@echo "... Installing include files to $(SYSINC_DIR)"
	@echo "... "

	@if [ ! -d  $(SYSINC_DIR) ] ; then \
          echo "Making directory $(SYSINC_DIR)" ; \
          mkdir -p $(SYSINC_DIR); \
        fi;

	@for i in midas msystem midasinc mrpc ybos cfortran hbook hardware mcstd mvmestd esone mfbstd ; \
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

	@for i in libmidas.so libmidas.a mana.o hmana.o rmana.o mfe.o fal.o ; \
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

mrproper : clean
	rm -rf $(OS_DIR)
	rm -rf vxworks/68kobj vxworks/ppcobj


