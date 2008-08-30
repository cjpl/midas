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
#  Midas preference flags
#  -DYBOS_VERSION_3_3  for YBOS up to version 3.3 
MIDAS_PREF_FLAGS  =

#
# Option to build the midas shared library
#
# To link midas with the static libmidas.a, say "make ... NEED_SHLIB="
# To link midas with the shared libmidas.so, say "make ... NEED_SHLIB=1"
#
NEED_SHLIB=

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
# Optional libmysqlclient library for mlogger
#
# To add mySQL support to the logger, say "make ... NEED_MYSQL=1"
#
# Here we try to figure out automatically if mySQL is installed
MYSQL_CONFIG := $(shell which mysql_config 2> /dev/null)
ifdef MYSQL_CONFIG
  MYSQLINCDIR := $(shell mysql_config --cflags | sed -e 's,^.*-I\([^ ]*\).*$$,\1,' -e s/\'//g)
  NEED_MYSQL := $(shell if [ -e $(MYSQLINCDIR)/mysql.h ]; then echo 1; fi) 
endif

#
# Optional ODBC history support
#
HAVE_ODBC := $(shell if [ -e /usr/include/sql.h ]; then echo 1; fi)

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
# Optional zlib support for data compression in the mlogger and in the analyzer
#
NEED_ZLIB=

#####################################################################
# Nothing needs to be modified after this line 
#####################################################################

#-----------------------
# Common flags
#
CC = cc
CXX = g++
CFLAGS = -g -O3 -Wall -Wuninitialized -I$(INC_DIR) -I$(DRV_DIR) -I$(MXML_DIR) -L$(LIB_DIR) -DINCLUDE_FTPLIB $(MIDAS_PREF_FLAGS) $(USERFLAGS)

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
GCC_PREFIX=/home/fgddaq/linuxdcc/Cross-Compiler/gcc-4.0.2/build/gcc-4.0.2-glibc-2.3.6/powerpc-405-linux-gnu
GCC_BIN=$(GCC_PREFIX)/bin/powerpc-405-linux-gnu-
LIBS=-L/home/fgddaq/linuxdcc/userland/lib -pthread -lutil
CC  = $(GCC_BIN)gcc
CXX = $(GCC_BIN)g++
OSTYPE = cross-ppc405
OS_DIR = $(OSTYPE)
NEED_MYSQL=
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
NEED_STRLCPY=
NEED_RANLIB=1
NEED_SHLIB=
NEED_RPATH=
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
OSFLAGS = -DOS_LINUX -fPIC -Wno-unused-function
# add to compile midas in 32-bit mode
# OSFLAGS += -m32
LIBS = -lutil -lpthread
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
EXAMPLES = $(BIN_DIR)/consume $(BIN_DIR)/produce \
	$(BIN_DIR)/rpc_test $(BIN_DIR)/msgdump $(BIN_DIR)/minife \
	$(BIN_DIR)/minirc $(BIN_DIR)/odb_test

PROGS = $(BIN_DIR)/mserver $(BIN_DIR)/mhttpd \
	$(BIN_DIR)/mlogger $(BIN_DIR)/odbedit \
	$(BIN_DIR)/mtape $(BIN_DIR)/mhist \
	$(BIN_DIR)/mstat $(BIN_DIR)/mcnaf \
	$(BIN_DIR)/mdump $(BIN_DIR)/lazylogger \
	$(BIN_DIR)/mhdump \
	$(BIN_DIR)/mchart $(BIN_DIR)/stripchart.tcl \
	$(BIN_DIR)/webpaw $(BIN_DIR)/odbhist \
	$(BIN_DIR)/melog \
	$(SPECIFIC_OS_PRG)

ANALYZER = $(LIB_DIR)/mana.o

ifdef CERNLIB
ANALYZER += $(LIB_DIR)/hmana.o
endif

ifdef ROOTSYS
ANALYZER += $(LIB_DIR)/rmana.o
endif

ifdef HAVE_ODBC
PROGS += $(BIN_DIR)/mh2sql
endif

OBJS =  $(LIB_DIR)/midas.o $(LIB_DIR)/system.o $(LIB_DIR)/mrpc.o \
	$(LIB_DIR)/odb.o $(LIB_DIR)/ybos.o $(LIB_DIR)/ftplib.o \
	$(LIB_DIR)/mxml.o \
	$(LIB_DIR)/history.o $(LIB_DIR)/alarm.o $(LIB_DIR)/elog.o

ifdef NEED_STRLCPY
OBJS += $(LIB_DIR)/strlcpy.o
endif

LIBNAME=$(LIB_DIR)/libmidas.a
LIB    =$(LIBNAME)
ifdef NEED_SHLIB
SHLIB = $(LIB_DIR)/libmidas.so
LIB   = -lmidas -Wl,-rpath,$(SYSLIB_DIR)
endif
VPATH = $(LIB_DIR):$(INC_DIR)

all: check-mxml \
	$(OS_DIR) $(LIB_DIR) $(BIN_DIR) \
	$(LIBNAME) $(SHLIB) \
	$(ANALYZER) \
	$(LIB_DIR)/cnaf_callback.o \
	$(LIB_DIR)/mfe.o \
	$(LIB_DIR)/fal.o $(PROGS)

linux32:
	$(MAKE) ROOTSYS= OS_DIR=linux-m32 USERFLAGS=-m32

linux64:
	$(MAKE) ROOTSYS= OS_DIR=linux-m64 USERFLAGS=-m64

crosscompile:
	echo OSTYPE=$(OSTYPE)
	$(MAKE) ROOTSYS= OS_DIR=$(OSTYPE)-crosscompile OSTYPE=crosscompile

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

ifdef NEED_MYSQL
CFLAGS      += -DHAVE_MYSQL $(shell mysql_config --include)
MYSQL_LIBS  := -L/usr/lib/mysql $(shell mysql_config --libs)
# only for mlogger LIBS        += $(MYSQL_LIBS)
NEED_ZLIB = 1
endif

ifdef HAVE_ODBC
CFLAGS      += -DHAVE_ODBC
OBJS        += $(LIB_DIR)/history_odbc.o
LIBS        += -lodbc
ifeq ($(OSTYPE),darwin)
LIBS        += /System/Library/Frameworks/CoreFoundation.framework/CoreFoundation
endif
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

$(BIN_DIR)/mlogger: $(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(ROOTLIBS) $(MYSQL_LIBS) $(LIBS)

$(BIN_DIR)/%:$(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)

$(BIN_DIR)/odbedit: $(SRC_DIR)/odbedit.c $(SRC_DIR)/cmdedit.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/odbedit.c $(SRC_DIR)/cmdedit.c $(LIB) $(LIBS)

$(BIN_DIR)/mhttpd: $(SRC_DIR)/mhttpd.c $(SRC_DIR)/mgd.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/mhttpd.c $(SRC_DIR)/mgd.c $(LIB) $(LIBS) -lm

$(PROGS): $(LIBNAME) $(SHLIB)

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

ifdef NEED_SHLIB
$(SHLIB): $(OBJS)
	rm -f $@
	ld -shared -o $@ $^ $(LIBS) -lc
endif

#
# frontend and backend framework
#

$(LIB_DIR)/mfe.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/fal.o: $(SRC_DIR)/fal.c msystem.h midas.h midasinc.h mrpc.h

$(LIB_DIR)/fal.o: $(SRC_DIR)/fal.c msystem.h midas.h midasinc.h mrpc.h
	$(CXX) -Dextname -DMANA_LITE -c $(CFLAGS) $(OSFLAGS) -o $@ $<
$(LIB_DIR)/mana.o: $(SRC_DIR)/mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $<
$(LIB_DIR)/hmana.o: $(SRC_DIR)/mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CC) -Dextname -DHAVE_HBOOK -c $(CFLAGS) $(OSFLAGS) -o $@ $<
ifdef ROOTSYS
$(LIB_DIR)/rmana.o: $(SRC_DIR)/mana.c msystem.h midas.h midasinc.h mrpc.h
	$(CXX) -DUSE_ROOT -c $(CFLAGS) $(OSFLAGS) $(ROOTCFLAGS) -o $@ $<
endif

#
# library objects
#

$(LIB_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $<

$(LIB_DIR)/%.o:$(SRC_DIR)/%.cxx
	$(CXX) -c $(CFLAGS) $(OSFLAGS) -o $@ $<

$(LIB_DIR)/mxml.o:$(MXML_DIR)/mxml.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $(MXML_DIR)/mxml.c

$(LIB_DIR)/strlcpy.o:$(MXML_DIR)/strlcpy.c
	$(CC) -c $(CFLAGS) $(OSFLAGS) -o $@ $(MXML_DIR)/strlcpy.c

$(LIB_DIR)/midas.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/system.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/mrpc.o: msystem.h midas.h mrpc.h
$(LIB_DIR)/odb.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/ybos.o: msystem.h midas.h midasinc.h mrpc.h
$(LIB_DIR)/ftplib.o: msystem.h midas.h midasinc.h
$(LIB_DIR)/mxml.o: msystem.h midas.h midasinc.h $(MXML_DIR)/mxml.h

#
# utilities
#
$(BIN_DIR)/%:$(UTL_DIR)/%.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(LIBS)


$(BIN_DIR)/mcnaf: $(UTL_DIR)/mcnaf.c $(DRV_DIR)/camac/camacrpc.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mcnaf.c $(DRV_DIR)/camac/camacrpc.c $(LIB) $(LIBS)

$(BIN_DIR)/mdump: $(UTL_DIR)/mdump.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mdump.c $(LIB) $(LIBS)

$(BIN_DIR)/mhdump: $(UTL_DIR)/mhdump.cxx
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $<

$(BIN_DIR)/mh2sql: $(UTL_DIR)/mh2sql.cxx $(LIB_DIR)/history_odbc.o
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $^ $(LIBS)

$(BIN_DIR)/lazylogger: $(SRC_DIR)/lazylogger.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/lazylogger.c $(LIB) $(LIBS)

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
ifdef NEED_SHLIB
	install -v -m 644 $(LIB_DIR)/libmidas.so $(SYSLIB_DIR)
else
	rm -fv $(SYSLIB_DIR)/libmidas.so
endif

# drivers
	@echo "... "
	@echo "... Installing drivers to $(PREFIX)/drivers"
	@echo "... "

	@for file in `find $(DRV_DIR) -type f | grep -v .svn` ; \
	  do \
	  install -v -D -m 644 $$file $(PREFIX)/$$file ;\
	  done

#--------------------------------------------------------------
# mininal_install
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
	rm -f $(LIB_DIR)/*.o $(LIB_DIR)/*.a $(LIB_DIR)/*.so *~ \#*

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

