#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS example frontend and analyzer
#
#  $Log$
#  Revision 1.16  2003/04/21 04:04:03  olchansk
#  add building "ranalyzer" - the ROOT analyzer
#  replace MANA_LITE with HAVE_HBOOK and HAVE_ROOT
#  compile HBOOK modules into h*.o, ROOT modules into r*.o
#  the HBOOK analyzer is still called "analyzer"
#
#  Revision 1.15  2003/04/14 13:46:30  midas
#  Fixed bug that camacnu.o was not compiled after a 'make clean'
#
#  Revision 1.14  2003/04/08 00:13:07  olchansk
#  add ranalyzer, the ROOT MIDAS analyzer
#
#  Revision 1.13  2002/05/10 05:23:18  pierre
#  add MANA_LITE option
#
#  Revision 1.12  2000/12/01 09:35:18  midas
#  Added fal
#
#  Revision 1.11  2000/10/02 14:04:08  midas
#  Added -lzlib for OSF1
#
#  Revision 1.10  2000/05/18 13:18:33  midas
#  Added -lnsl for linux (problems with CERNLIB 2000)
#
#  Revision 1.9  2000/03/15 12:48:36  midas
#  Added -Dextname for linux
#
#  Revision 1.8  1999/12/22 08:57:27  midas
#  Use ../../drivers directory
#
#  Revision 1.7  1999/12/22 08:53:56  midas
#  Changed /midas/drivers to ../../drivers
#
#  Revision 1.6  1999/12/22 00:05:00  pierre
#  - Set default driver to camacnul
#
#  Revision 1.5  1999/12/21 10:15:29  midas
#  Removed kernel flags
#
#  Revision 1.4  1999/12/20 15:33:39  midas
#  Adjustments for different driver directories
#
#  Revision 1.3  1999/12/10 00:45:48  pierre
#  - Add DRIVER selection and UFE option
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
CFLAGS = -g -O2 -Wall -Wuninitialized

# for f2c, uncomment following:
#LIBS = -lbsd -lm -lutil /usr/lib/libf2c.a
#FF = cc
# for egcs g77, use this
FF = g77
LIBS = -lm -lz -lutil -lnsl

#-----------------------------------------
ifdef ROOTSYS
ROOTCFLAGS:= -DHAVE_ROOT -I$(ROOTSYS)/include
ROOTLIBS:= -Wl,-rpath,$(ROOTSYS)/lib -L$(ROOTSYS)/lib -lCore -lCint -lHist -lTree -lMatrix -ldl
endif

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
#                 
DRV_DIR         = ../../drivers
INC_DIR 	= ../../include
LIB_DIR 	= ../../linux/lib
#
CERNLIB_PACK	= /cern/pro/lib/libpacklib.a
#
#-------------------------------------------------------------------
# List of analyzer modules
#
# LITE modules for "analyzer"
MODULES 	= adccalib.o adcsum.o scaler.o

# ROOT modules for the ROOT analyzer "ranalyzer"
RMODULES 	= radccalib.o radcsum.o rscaler.o

# HBOOK modules for the HBOOK analyzer "hanalyzer"
HMODULES 	= hadccalib.o hadcsum.o hscaler.o

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

all: $(UFE) fal analyzer ranalyzer

$(UFE): $(LIB) $(LIB_DIR)/mfe.o $(DRIVER).o $(UFE).c
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(UFE) $(UFE).c $(DRIVER).o \
	$(LIB_DIR)/mfe.o $(LIB) $(LDFEFLAGS) $(LIBS)

$(DRIVER).o: $(DRV_DIR)/bus/$(DRIVER).c
	$(CC) $(CFLAGS) $(OSFLAGS) -c -o $(DRIVER).o $(DRV_DIR)/bus/$(DRIVER).c

analyzer: $(LIB) $(LIB_DIR)/mana.o hanalyzer.o $(HMODULES)
	$(FF) $(FFLAGS) -o $@ $(LIB_DIR)/mana.o hanalyzer.o \
	$(MODULES) $(CERNLIB_PACK) \
	$(LIB) $(LDFLAGS) $(LIBS)

ranalyzer: $(LIB) $(LIB_DIR)/rmana.o ranalyzer.o $(RMODULES)
	$(CXX) $(FFLAGS) -o $@ $(LIB_DIR)/rmana.o ranalyzer.o $(RMODULES) \
	$(LIB) $(LDFLAGS) $(ROOTLIBS) $(LIBS)

fal: $(LIB) $(LIB_DIR)/fal.o $(DRV_DIR)/bus/$(DRIVER).o $(UFE).c $(MODULES) $(UFE).o hanalyzer.o
	$(FF) $(FFLAGS) -o fal $(LIB_DIR)/fal.o $(UFE).o $(DRIVER).o \
	hanalyzer.o $(MODULES) $(CERNLIB_PACK) \
	$(LIB) $(LDFEFLAGS) $(LIBS)

ranalyzer.o $(RMODULES): r%.o: %.c
	$(CXX) $(ROOTCFLAGS) $(USERFLAGS) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

hanalyzer.o $(HMODULES): h%.o: %.c
	$(CC) -DHAVE_HBOOK $(CFLAGS) $(OSFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(USERFLAGS) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

clean::
	rm -f *.o *~ \#*

#end file
