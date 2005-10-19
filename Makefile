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
NEED_MYSQL=
MYSQL_LIBS=/usr/lib/mysql/libmysqlclient.a

#
# Option to use our own implementation of strlcat, strlcpy
#
NEED_STRLCPY=1

#
# Directory in which mxml.c/h resides. This library has to be checked
# out separately from the midas CVS since it's used in several projects
#
MXML_DIR=../mxml

#####################################################################
# Nothing needs to be modified after this line 
#####################################################################

#-----------------------
# Common flags
#
CC = cc
CXX = g++
CFLAGS = -g -O2 -Wall -Wuninitialized -I$(INC_DIR) -I$(DRV_DIR) -I$(MXML_DIR) -L$(LIB_DIR) -DINCLUDE_FTPLIB $(MIDAS_PREF_FLAGS) $(USERFLAGS)

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
# This is for Linux
#
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif

ifeq ($(OSTYPE),linux)

# >2GB file support
CFLAGS += -D_LARGEFILE64_SOURCE

OS_DIR = linux
OSFLAGS = -m32 -DOS_LINUX -fPIC -Wno-unused-function
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

ANALYZER = $(LIB_DIR)/mana.o

ifdef CERNLIB
ANALYZER += $(LIB_DIR)/hmana.o
endif

ifdef ROOTSYS
PROGS += $(BIN_DIR)/rmidas
ANALYZER += $(LIB_DIR)/rmana.o
endif

OBJS =  $(LIB_DIR)/midas.o $(LIB_DIR)/system.o $(LIB_DIR)/mrpc.o \
	$(LIB_DIR)/odb.o $(LIB_DIR)/ybos.o $(LIB_DIR)/ftplib.o \
	$(LIB_DIR)/mxml.o $(LIB_DIR)/cnaf_callback.o $(LIB_DIR)/musbstd.o

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

all:    $(OS_DIR) $(LIB_DIR) $(BIN_DIR) \
	$(LIBNAME) $(SHLIB) \
	$(ANALYZER) \
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

ifdef NEED_MYSQL
CFLAGS     += -DHAVE_MYSQL
LIBS       += $(MYSQL_LIBS) -lz
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

$(BIN_DIR)/rmidas: $(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CXX) $(CFLAGS) $(OSFLAGS) -DHAVE_ROOT $(ROOTCFLAGS) -o $@ $< $(LIB) $(ROOTGLIBS) $(LIBS)

endif # ROOTSYS


$(BIN_DIR)/mlogger: $(BIN_DIR)/%: $(SRC_DIR)/%.c
	$(CXX) $(CFLAGS) $(OSFLAGS) -o $@ $< $(LIB) $(ROOTLIBS) $(LIBS)

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
	$(CC) -Dextname -c $(CFLAGS) $(OSFLAGS) -o $@ $<
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


$(BIN_DIR)/mcnaf: $(UTL_DIR)/mcnaf.c $(DRV_DIR)/bus/camacrpc.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mcnaf.c $(DRV_DIR)/bus/camacrpc.c $(LIB) $(LIBS)

$(BIN_DIR)/mdump: $(UTL_DIR)/mdump.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(UTL_DIR)/mdump.c $(LIB) -lz $(LIBS)

$(BIN_DIR)/lazylogger: $(SRC_DIR)/lazylogger.c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $(SRC_DIR)/lazylogger.c $(LIB) -lz $(LIBS)

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
	  install -v -m 755 $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
	  done

	install -v -m 755 $(UTL_DIR)/mcleanup $(SYSBIN_DIR)
	chmod +s $(SYSBIN_DIR)/dio
	chmod +s $(SYSBIN_DIR)/mhttpd

# utilities
	@echo "... "
	@echo "... Installing utilities to $(SYSBIN_DIR)"
	@echo "... "
	@for i in mhist melog odbhist mtape mstat lazylogger mdump mcnaf mlxspeaker mchart stripchart.tcl webpaw; \
	  do \
	  install -v -m 755 $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
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
	  install -v -m 644 $(INC_DIR)/$$i.h $(SYSINC_DIR) ; \
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
	  install -v -m 644 $(LIB_DIR)/$$i $(SYSLIB_DIR) ; \
	  done

ifdef CERNLIB
	install -v -m 644 $(LIB_DIR)/hmana.o $(SYSLIB_DIR)/hmana.o
else
	rm -fv $(SYSLIB_DIR)/hmana.o
      chmod +s $(SYSBIN_DIR)/mhttpd
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

	@if [ -d  $(ZLIB_DIR) ] ; then \
	  install -v -m 644 $(ZLIB_DIR)/zlib.h $(SYSINC_DIR) ;\
	  install -v -m 644 $(ZLIB_DIR)/zconf.h $(SYSINC_DIR) ;\
	  install -v -m 644 $(ZLIB_DIR)/libz.a $(SYSLIB_DIR) ;\
	fi;

#--------------------------------------------------------------
# mininal_install
minimal_install:
# system programs
        @echo "... "
        @echo "... Minimal Install for programs to $(SYSBIN_DIR)"
        @echo "... "

        @if [ ! -d  $(SYSBIN_DIR) ] ; then \
          echo "Making directory $(SYSBIN_DIR)" ; \
          mkdir -p $(SYSBIN_DIR); \
        fi;

        @for i in mserver mhttpd dio ; \
          do \
          install -v -m 755 $(BIN_DIR)/$$i $(SYSBIN_DIR) ; \
          done

        install -v -m 755 $(UTL_DIR)/mcleanup $(SYSBIN_DIR)
        chmod +s $(SYSBIN_DIR)/dio
        chmod +s $(SYSBIN_DIR)/mhttpd

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


