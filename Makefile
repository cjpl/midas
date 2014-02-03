#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS binaries and examples under unix
#
#  $Id$
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
# Option to set the shared library path on MIDAS executables
#
NEED_RPATH=1

#
# Option to use the static ROOT library libRoot.a
#
# To link midas with the static ROOT library, say "make ... NEED_LIBROOTA=1"
#
NEED_LIBROOTA=

#
# Optional MYSQL library for mlogger and for the history
#
HAVE_MYSQL := $(shell mysql_config --include 2> /dev/null)

#
# Optional ODBC history support
#
HAVE_ODBC := $(shell if [ -e /usr/include/sql.h ]; then echo 1; fi)

#
# Optional SQLITE history support
#
HAVE_SQLITE := $(shell if [ -e /usr/include/sqlite3.h ]; then echo 1; fi)

#
# Option to use our own implementation of strlcat, strlcpy
#
NEED_STRLCPY=1

#
# Directory in which mxml.c/h resides. This library has to be checked
# out separately from the midas CVS since it's used in several projects
#
MXML_DIR=../mxml

#
# Directory in which mscb.c/h resides. These files are necessary for
# the optional MSCB support in mhttpd
#
MSCB_DIR=../mscb

#
# Indicator that MSCB is available
#
HAVE_MSCB := $(shell if [ -e $(MSCB_DIR) ]; then echo 1; fi)

#
# Optional zlib support for data compression in the mlogger and in the analyzer
#
NEED_ZLIB=

#####################################################################
# Nothing needs to be modified after this line 
#####################################################################

#-----------------------
# Common flags
#
CC = gcc $(USERFLAGS)
CXX = g++ $(USERFLAGS)
CFLAGS = -g -O2 -Wall -Wno-strict-aliasing -Wuninitialized -I$(INC_DIR) -I$(DRV_DIR) -I$(MXML_DIR) -I$(MSCB_DIR) -DHAVE_FTPLIB

#-----------------------
# Ovevwrite MAX_EVENT_SIZE with environment variable
#
ifdef MIDAS_MAX_EVENT_SIZE
CFLAGS += -DMAX_EVENT_SIZE=$(MIDAS_MAX_EVENT_SIZE)
endif

#-----------------------
# Cross-compilation, change GCC_PREFIX
#
ifeq ($(OSTYPE),crosscompile)
GCC_PREFIX=$(HOME)/linuxdcc/Cross-Compiler/gcc-4.0.2/build/gcc-4.0.2-glibc-2.3.6/powerpc-405-linux-gnu
GCC_BIN=$(GCC_PREFIX)/bin/powerpc-405-linux-gnu-
LIBS=-L$(HOME)/linuxdcc/userland/lib -pthread -lutil -lrt -ldl
CC  = $(GCC_BIN)gcc
CXX = $(GCC_BIN)g++
OSTYPE = cross-ppc405
OS_DIR = $(OSTYPE)
CFLAGS += -DOS_LINUX
NEED_MYSQL=
HAVE_ODBC=
HAVE_SQLITE=
endif

#-----------------------
# ARM Cross-compilation, change GCC_PREFIX
#
ifeq ($(OSTYPE),crosslinuxarm)

ifndef USE_TI_ARM
USE_YOCTO_ARM=1
endif

ifdef USE_TI_ARM
# settings for the TI Sitara ARM SDK
TI_DIR=/ladd/data0/olchansk/MityARM/TI/ti-sdk-am335x-evm-06.00.00.00/linux-devkit/sysroots/i686-arago-linux/usr
TI_EXT=arm-linux-gnueabihf
GCC_BIN=$(TI_DIR)/bin/$(TI_EXT)-
LIBS=-Wl,-rpath,$(TI_DIR)/$(TI_EXT)/lib -pthread -lutil -lrt -ldl
endif

ifdef USE_YOCTO_ARM
# settings for the Yocto poky cross compilers
POKY_DIR=/ladd/data0/olchansk/MityARM/Yocto/opt/poky/1.5/sysroots
POKY_EXT=arm-poky-linux-gnueabi
POKY_HOST=x86_64-pokysdk-linux
POKY_ARM=armv7a-vfp-neon-poky-linux-gnueabi
GCC_BIN=$(POKY_DIR)/$(POKY_HOST)/usr/bin/$(POKY_EXT)/$(POKY_EXT)-
LIBS=--sysroot $(POKY_DIR)/$(POKY_ARM) -Wl,-rpath,$(POKY_DIR)/$(POKY_ARM)/usr/lib -pthread -lutil -lrt -ldl
CFLAGS += --sysroot $(POKY_DIR)/$(POKY_ARM)
endif

CC  = $(GCC_BIN)gcc
CXX = $(GCC_BIN)g++
OSTYPE = linux-arm
OS_DIR = $(OSTYPE)
CFLAGS += -DOS_LINUX
CFLAGS += -fPIC
NEED_MYSQL=
HAVE_ODBC=
HAVE_SQLITE=
ROOTSYS=
endif

#-----------------------
# OSF/1 (DEC UNIX)
#
ifeq ($(HOSTTYPE),alpha)
OSTYPE = osf1
endif

ifeq ($(OSTYPE),osf1)
OS_DIR = osf1
OSFLAGS = -DOS_OSF1
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
OSFLAGS = -DOS_ULTRIX -DNO_PTY
LIBS =
SPECIFIC_OS_PRG = 
endif

#-----------------------
# FreeBSD
#
ifeq ($(OSTYPE), FreeBSD)
OS_DIR = freeBSD
OSFLAGS = -DOS_FREEBSD
LIBS = -lbsd -lcompat
SPECIFIC_OS_PRG = 
endif

#-----------------------
# MacOSX/Darwin is just a funny Linux
#
ifeq ($(OSTYPE),Darwin)
OSTYPE = darwin
endif

ifeq ($(OSTYPE),darwin)
OS_DIR = darwin
OSFLAGS = -DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY -fPIC -Wno-unused-function
LIBS = -lpthread
SPECIFIC_OS_PRG = $(BIN_DIR)/mlxspeaker
NEED_ZLIB=1
NEED_STRLCPY=
NEED_RANLIB=1
NEED_RPATH=
SHLIB+=$(LIB_DIR)/libmidas-shared.dylib
SHLIB+=$(LIB_DIR)/libmidas-shared.so
endif

#-----------------------
# This is for Cygwin
#
ifeq ($(OSTYPE),CYGWIN_NT-5.1)
OSTYPE = cygwin
endif

ifeq ($(OSTYPE),cygwin)

OS_DIR = cygwin
OSFLAGS = -DOS_LINUX -DOS_CYGWIN -Wno-unused-function
LIBS = -lutil -lpthread
endif

#-----------------------
# This is for Linux
#
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

ifeq ($(OSTYPE),linux)

# >2GB file support
CFLAGS += -D_LARGEFILE64_SOURCE

# include ZLIB support
NEED_ZLIB=1

OS_DIR = linux
OSFLAGS += -DOS_LINUX -fPIC -Wno-unused-function
LIBS = -lutil -lpthread -lrt -ldl
SPECIFIC_OS_PRG = $(BIN_DIR)/mlxspeaker $(BIN_DIR)/dio
endif

#-----------------------
# This is for Solaris
#
ifeq ($(OSTYPE),solaris)
CC = gcc
OS_DIR = solaris
OSFLAGS = -DOS_SOLARIS
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

#
# Midas operating system dependent directories
#
LIB_DIR  = $(OS_DIR)/lib
BIN_DIR  = $(OS_DIR)/bin

#
# targets
#
GIT_REVISION = $(INC_DIR)/git-revision.h
EXAMPLES = $(BIN_DIR)/consume $(BIN_DIR)/produce \
	$(BIN_DIR)/rpc_test $(BIN_DIR)/msgdump $(BIN_DIR)/minife \
	$(BIN_DIR)/minirc $(BIN_DIR)/odb_test

PROGS = $(BIN_DIR)/mserver $(BIN_DIR)/mhttpd \
	$(BIN_DIR)/mlogger $(BIN_DIR)/odbedit \
	$(BIN_DIR)/mtape $(BIN_DIR)/mhist \
	$(BIN_DIR)/mstat $(BIN_DIR)/mdump \
	$(BIN_DIR)/lazylogger \
	$(BIN_DIR)/mtransition \
	$(BIN_DIR)/mhdump \
	$(BIN_DIR)/mchart $(BIN_DIR)/stripchart.tcl \
	$(BIN_DIR)/webpaw $(BIN_DIR)/odbhist \
	$(BIN_DIR)/melog \
	$(BIN_DIR)/mh2sql \
	$(BIN_DIR)/mfe_link_test \
	$(BIN_DIR)/mana_link_test \
	$(BIN_DIR)/fal_link_test \
	$(BIN_DIR)/mjson_test \
	$(SPECIFIC_OS_PRG)

ANALYZER = $(LIB_DIR)/mana.o

ifdef CERNLIB
ANALYZER += $(LIB_DIR)/hmana.o
endif

ifdef ROOTSYS
ANALYZER += $(LIB_DIR)/rmana.o
endif

OBJS =  $(LIB_DIR)/midas.o $(LIB_DIR)/system.o $(LIB_DIR)/mrpc.o \
	$(LIB_DIR)/odb.o $(LIB_DIR)/ftplib.o \
	$(LIB_DIR)/mxml.o \
	$(LIB_DIR)/mjson.o \
	$(LIB_DIR)/json_paste.o \
	$(LIB_DIR)/dm_eb.o \
	$(LIB_DIR)/history_common.o \
	$(LIB_DIR)/history_midas.o \
	$(LIB_DIR)/history_odbc.o \
	$(LIB_DIR)/history_schema.o \
	$(LIB_DIR)/history.o $(LIB_DIR)/alarm.o $(LIB_DIR)/elog.o

ifdef NEED_STRLCPY
OBJS += $(LIB_DIR)/strlcpy.o
endif

LIBNAME = $(LIB_DIR)/libmidas.a
LIB     = $(LIBNAME)
SHLIB   = $(LIB_DIR)/libmidas-shared.so

VPATH = $(LIB_DIR):$(INC_DIR)

all: check-mxml \
	$(GIT_REVISION) \
	$(OS_DIR) $(LIB_DIR) $(BIN_DIR) \
	$(LIBNAME) $(SHLIB) \
	$(ANALYZER) \
	$(LIB_DIR)/cnaf_callback.o \
	$(LIB_DIR)/mfe.o \
	$(LIB_DIR)/fal.o $(PROGS)

dox:
	doxygen

linux32:
	$(MAKE) ROOTSYS= HAVE_MYSQL= HAVE_ODBC= HAVE_SQLITE= OS_DIR=linux-m32 USERFLAGS=-m32

linux64:
	$(MAKE) ROOTSYS= OS_DIR=linux-m64 USERFLAGS=-m64

clean32:
	$(MAKE) ROOTSYS= OS_DIR=linux-m32 USERFLAGS=-m32 clean

clean64:
	$(MAKE) ROOTSYS= OS_DIR=linux-m64 USERFLAGS=-m64 clean

crosscompile:
	echo OSTYPE=$(OSTYPE)
	$(MAKE) ROOTSYS= OS_DIR=$(OSTYPE)-crosscompile OSTYPE=crosscompile

linuxarm:
	echo OSTYPE=$(OSTYPE)
	$(MAKE) ROOTSYS= HAVE_MYSQL= HAVE_ODBC= HAVE_SQLITE= OS_DIR=$(OSTYPE)-arm OSTYPE=crosslinuxarm

cleanarm:
	$(MAKE) ROOTSYS= OS_DIR=linux-arm clean

cleandox:
	-rm -rf html


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
# put current GIT revision into header file to be included by programs
#
$(GIT_REVISION): $(SRC_DIR)/midas.c $(SRC_DIR)/odb.c $(SRC_DIR)/system.c
	echo \#define GIT_REVISION \"`git log -n 1 --pretty=format:"%ad - %h"`\" > $(GIT_REVISION)

#
# main binaries
#

ifdef HAVE_MYSQL
CFLAGS      += -DHAVE_MYSQL $(shell mysql_config --include)
MYSQL_LIBS  += -L/usr/lib/mysql $(shell mysql_config --libs)
NEED_ZLIB = 1
endif

ifdef HAVE_ODBC
CFLAGS      += -DHAVE_ODBC
ODBC_LIBS   += -lodbc
ifeq ($(OSTYPE),darwin)
ODBC_LIBS   += /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation
endif
endif

ifdef HAVE_SQLITE
CFLAGS      += -DHAVE_SQLITE
SQLITE_LIBS += -lsqlite3
endif

ifdef ROOTSYS
ROOTLIBS    := $(shell $(ROOTSYS)/bin/root-config --libs)
ROOTGLIBS   := $(shell $(ROOTSYS)/bin/root-config --glibs)
ROOTCFLAGS  := $(shell $(ROOTSYS)/bin/root-config --cflags)

ifdef NEED_RPATH
ROOTLIBS   += -Wl,-rpath,$(ROOTSYS)/lib
ROOTGLIBS  += -Wl,-rpath,$(ROOTSYS)/lib
endif

ifdef NEED_LIBROOTA
ROOTLIBS    := $(ROOTSYS)/lib/libRoot.a -lssl -ldl -lcrypt
ROOTGLIBS   := $(ROOTLIBS) -lfreetype
endif

CFLAGS     += -DHAVE_ROOT $(ROOTCFLAGS)

endif # ROOTSYS

ifdef NEED_ZLIB
CFLAGS     += -DHAVE_ZLIB
LIBS       += -lz
endif

ifdef HAVE_MSCB
CFLAGS     += -DHAVE_MSCB
endif

$(BIN_DIR)/mlogger: $(BIN_DIR)/%: $(SRC_DIR)/%.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(ROOTLIBS) $(ODBC_LIBS) $(SQLITE_LIBS) $(MYSQL_LIBS) $(LIBS)

$(BIN_DIR)/%:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/odbedit: $(SRC_DIR)/odbedit.cxx $(SRC_DIR)/cmdedit.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/odbedit.cxx $(SRC_DIR)/cmdedit.cxx $(LIB) $(LIBS)


ifdef HAVE_MSCB
$(BIN_DIR)/mhttpd: $(LIB_DIR)/mhttpd.o $(LIB_DIR)/mongoose.o $(LIB_DIR)/mgd.o $(LIB_DIR)/mscb.o $(LIB_DIR)/sequencer.o
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $^ $(LIB) $(MYSQL_LIBS) $(ODBC_LIBS) $(SQLITE_LIBS) $(LIBS) -lm
else
$(BIN_DIR)/mhttpd: $(LIB_DIR)/mhttpd.o $(LIB_DIR)/mongoose.o $(LIB_DIR)/mgd.o $(LIB_DIR)/sequencer.o
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $^ $(LIB) $(MYSQL_LIBS) $(ODBC_LIBS) $(SQLITE_LIBS) $(LIBS) -lm
endif

$(BIN_DIR)/mh2sql: $(BIN_DIR)/%: $(UTL_DIR)/mh2sql.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(ODBC_LIBS) $(SQLITE_LIBS) $(MYSQL_LIBS) $(LIBS)

$(BIN_DIR)/mhist: $(BIN_DIR)/%: $(UTL_DIR)/%.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(ODBC_LIBS) $(SQLITE_LIBS) $(MYSQL_LIBS) $(LIBS)

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
ifdef NEED_RANLIB
	ranlib $@
endif

ifeq ($(OSTYPE),crosscompile)
%.so: $(OBJS)
	rm -f $@
	$(CXX) -shared -o $@ $^ $(LIBS) -lc
endif

ifeq ($(OSTYPE),crosslinuxarm)
%.so: $(OBJS)
	rm -f $@
	$(CXX) -shared -o $@ $^ $(LIBS) -lc
endif

ifeq ($(OSTYPE),linux)
%.so: $(OBJS)
	rm -f $@
	$(CXX) -shared -o $@ $^ $(LIBS) -lc
endif

ifeq ($(OSTYPE),darwin)
%.dylib: $(OBJS)
	rm -f $@
	g++ -dynamiclib -dylib -o $@ $^ $(LIBS) -lc

%.so: $(OBJS)
	rm -f $@
	g++ -bundle -flat_namespace -undefined suppress -o $@ $^ $(LIBS) -lc
endif

#
# frontend and backend framework
#

$(LIB_DIR)/history_sql.o $(LIB_DIR)/history_schema.o $(LIB_DIR)/history_midas.o $(LIB_DIR)/mhttpd.o $(LIB_DIR)/mlogger.o: history.h

$(LIB_DIR)/mfe.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/fal.o: $(SRC_DIR)/fal.cxx msystem.h midas.h midasinc.h mrpc.h

$(LIB_DIR)/fal.o: $(SRC_DIR)/fal.cxx msystem.h midas.h midasinc.h mrpc.h
	$(CXX) -Dextname -DMANA_LITE -c $(CFLAGS) $(OSFLAGS) -o $@ $<
$(LIB_DIR)/mana.o: $(SRC_DIR)/mana.cxx msystem.h midas.h midasinc.h mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $<
$(LIB_DIR)/hmana.o: $(SRC_DIR)/mana.cxx msystem.h midas.h midasinc.h mrpc.h
	$(CC) -Dextname -DHAVE_HBOOK -c $(CFLAGS) $(OSFLAGS) -o $@ $<
ifdef ROOTSYS
$(LIB_DIR)/rmana.o: $(SRC_DIR)/mana.cxx msystem.h midas.h midasinc.h mrpc.h
	$(CXX) -DUSE_ROOT -c $(CFLAGS) $(OSFLAGS) $(ROOTCFLAGS) -o $@ $<
endif

#
# library objects
#

$(LIB_DIR)/mhttpd.o: $(LIB_DIR)/%.o: $(SRC_DIR)/%.cxx
	$(CXX) -c $(CFLAGS) $(OSFLAGS) -o $@ $<

$(LIB_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $<

$(LIB_DIR)/%.o:$(SRC_DIR)/%.cxx
	$(CXX) -c $(CFLAGS) $(OSFLAGS) -o $@ $<

$(LIB_DIR)/mxml.o:$(MXML_DIR)/mxml.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $(MXML_DIR)/mxml.c

$(LIB_DIR)/strlcpy.o:$(MXML_DIR)/strlcpy.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $(MXML_DIR)/strlcpy.c

ifdef HAVE_MSCB
$(LIB_DIR)/mscb.o:$(MSCB_DIR)/mscb.c $(MSCB_DIR)/mscb.h
	$(CXX) -x c++ -c $(CFLAGS) $(OSFLAGS) -o $@ $(MSCB_DIR)/mscb.c
endif

$(LIB_DIR)/mhttpd.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/midas.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/system.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/mrpc.o: msystem.h midas.h mrpc.h
$(LIB_DIR)/odb.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/mdsupport.o: msystem.h midas.h midasinc.h
$(LIB_DIR)/ftplib.o: msystem.h midas.h midasinc.h
$(LIB_DIR)/mxml.o: msystem.h midas.h midasinc.h $(MXML_DIR)/mxml.h
$(LIB_DIR)/alarm.o: msystem.h midas.h midasinc.h

#
# utilities
#
$(BIN_DIR)/%: $(UTL_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/%: $(UTL_DIR)/%.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/mcnaf: $(UTL_DIR)/mcnaf.c $(DRV_DIR)/camac/camacrpc.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mcnaf.c $(DRV_DIR)/camac/camacrpc.c $(LIB) $(LIBS)

$(BIN_DIR)/mdump: $(UTL_DIR)/mdump.cxx $(SRC_DIR)/mdsupport.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mdump.cxx $(SRC_DIR)/mdsupport.cxx $(LIB) $(LIBS)

$(BIN_DIR)/mfe_link_test: $(SRC_DIR)/mfe.c
	$(CC) $(CFLAGS) $(OSFLAGS) -DLINK_TEST -o $@ $(SRC_DIR)/mfe.c $(LIB) $(LIBS)

$(BIN_DIR)/fal_link_test: $(SRC_DIR)/fal.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -DMANA_LITE -DLINK_TEST -o $@ $(SRC_DIR)/fal.cxx $(LIB)  $(MYSQL_LIBS) $(LIBS)

$(BIN_DIR)/mana_link_test: $(SRC_DIR)/mana.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -DLINK_TEST -o $@ $(SRC_DIR)/mana.cxx $(LIB) $(LIBS)

$(BIN_DIR)/mhdump: $(UTL_DIR)/mhdump.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $<

$(BIN_DIR)/mtransition: $(SRC_DIR)/mtransition.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/lazylogger: $(SRC_DIR)/lazylogger.cxx $(SRC_DIR)/mdsupport.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $<  $(SRC_DIR)/mdsupport.cxx $(LIB) $(LIBS)

$(BIN_DIR)/dio: $(UTL_DIR)/dio.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/dio.c

$(BIN_DIR)/stripchart.tcl: $(UTL_DIR)/stripchart.tcl
	cp -f $(UTL_DIR)/stripchart.tcl $(BIN_DIR)/. 


#####################################################################

static:
	rm -f $(PROGS)
	make USERFLAGS=-static

#####################################################################

install:
# system programs and utilities
	@echo "... "
	@echo "... Installing programs and utilities to $(SYSBIN_DIR)"
	@echo "... "

	@for file in `find $(BIN_DIR) -type f | grep -v .svn` ; \
	  do \
	  install -v -D -m 755 $$file $(SYSBIN_DIR)/`basename $$file` ; \
	  done

	install -v -m 755 $(UTL_DIR)/mcleanup $(SYSBIN_DIR)
	if [ -f $(SYSBIN_DIR)/dio ]; then chmod +s $(SYSBIN_DIR)/dio ; fi
	if [ -f $(SYSBIN_DIR)/mhttpd ]; then chmod +s $(SYSBIN_DIR)/mhttpd; fi
	if [ -f $(SYSBIN_DIR)/webpaw ]; then chmod +s $(SYSBIN_DIR)/webpaw; fi
	ln -fs $(SYSBIN_DIR)/stripchart.tcl $(SYSBIN_DIR)/stripchart

# include
	@echo "... "
	@echo "... Installing include files to $(SYSINC_DIR)"
	@echo "... "

	@for file in `find $(INC_DIR) -type f | grep -v .svn` ; \
	  do \
	  install -v -D -m 644 $$file $(SYSINC_DIR)/`basename $$file` ; \
	  done

# library + objects
	@echo "... "
	@echo "... Installing library and objects to $(SYSLIB_DIR)"
	@echo "... "

	@for i in libmidas.a mana.o mfe.o fal.o ; \
	  do \
	  install -v -D -m 644 $(LIB_DIR)/$$i $(SYSLIB_DIR)/$$i ; \
	  done

ifdef CERNLIB
	install -v -m 644 $(LIB_DIR)/hmana.o $(SYSLIB_DIR)/hmana.o
else
	rm -fv $(SYSLIB_DIR)/hmana.o
	chmod +s $(SYSBIN_DIR)/mhttpd
endif
ifdef ROOTSYS
	install -v -m 644 $(LIB_DIR)/rmana.o $(SYSLIB_DIR)/rmana.o
else
	rm -fv $(SYSLIB_DIR)/rmana.o
endif
	-rm -fv $(SYSLIB_DIR)/libmidas.so
	-install -v -m 644 $(LIB_DIR)/libmidas-shared.so $(SYSLIB_DIR)

# drivers
	@echo "... "
	@echo "... Installing drivers to $(PREFIX)/drivers"
	@echo "... "

	@for file in `find $(DRV_DIR) -type f | grep -v .svn` ; \
	  do \
	  install -v -D -m 644 $$file $(PREFIX)/$$file ;\
	  done

#--------------------------------------------------------------
# minimal_install
minimal_install:
# system programs
	@echo "... "
	@echo "... Minimal Install for programs to $(SYSBIN_DIR)"
	@echo "... "

	@for i in mserver mhttpd; \
	  do \
	  install -v -m 755 $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
	  done

ifeq ($(OSTYPE),linux)
	install -v -m 755 $(BIN_DIR)/dio $(SYSBIN_DIR)
endif
	install -v -m 755 $(UTL_DIR)/mcleanup $(SYSBIN_DIR)
	if [ -f $(SYSBIN_DIR)/dio ]; then chmod +s $(SYSBIN_DIR)/dio; fi
	if [ -f $(SYSBIN_DIR)/mhttpd ]; then chmod +s $(SYSBIN_DIR)/mhttpd; fi

# utilities
	@echo "... "
	@echo "... No utilities install to $(SYSBIN_DIR)"
	@echo "... "

# include
	@echo "... "
	@echo "... No include install to $(SYSINC_DIR)"
	@echo "... "

# library + objects
	@echo "... "
	@echo "... No library Install to $(SYSLIB_DIR)"
	@echo "... "

indent:
	find . -name "*.[hc]" -exec indent -kr -nut -i3 -l90 {} \;

clean:
	-rm -vf $(LIB_DIR)/*.o $(LIB_DIR)/*.a $(LIB_DIR)/*.so $(LIB_DIR)/*.dylib
	-rm -rvf $(BIN_DIR)/*.dSYM
	-rm -vf $(BIN_DIR)/*

mrproper : clean
	rm -rf $(OS_DIR)
	rm -rf vxworks/68kobj vxworks/ppcobj

check-mxml :
ifeq ($(NEED_STRLCPY), 1)
	@if [ ! -e $(MXML_DIR)/strlcpy.h ]; then \
	  echo "please download mxml."; \
	  echo "http://midas.psi.ch/htmldoc/quickstart.html"; \
	  exit 1; \
	fi
endif
	@if [ ! -e $(MXML_DIR)/mxml.h ]; then \
	  echo "please download mxml."; \
	  echo "http://midas.psi.ch/htmldoc/quickstart.html"; \
	  exit 1; \
	fi

