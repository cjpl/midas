# Makefile
# $Id$
#MIDASSYS=/home/olchansk/daq/midas/midas

OSFLAGS  = -m32 -DOS_LINUX -Dextname
CFLAGS   =  -g -O2 -Wall -Wuninitialized -I$(INC_DIR) -I$(DRV_DIR) -I$(VMICHOME)/include
CXXFLAGS = $(CFLAGS) -DHAVE_ROOT -DUSE_ROOT -I$(ROOTSYS)/include

LIBS = -lm -lz -lutil -lnsl -lpthread

DRV_DIR         = $(MIDASSYS)/drivers
INC_DIR         = $(MIDASSYS)/include
LIB_DIR         = $(MIDASSYS)/linux/lib

# MIDAS library
MIDASLIBS = $(LIB_DIR)/libmidas.a

# ROOT library
ROOTLIBS = $(shell $(ROOTSYS)/bin/root-config --libs) -lThread -Wl,-rpath,$(ROOTSYS)/lib

all: fevme.exe analyzer.exe

vmicvme.o: $(MIDASSYS)/drivers/vme/vmic/vmicvme.c
	gcc -c -o $@ -O2 -g -Wall -Wuninitialized $< -I$(MIDASSYS)/include

%.o: $(MIDASSYS)/drivers/vme/%.c
	gcc -c -o $@ -O2 -g -Wall -Wuninitialized $< -I$(MIDASSYS)/include

fevme.exe: $(MIDASLIBS) $(LIB_DIR)/mfe.o fevme.o vmicvme.o v792.o v1190B.o
	$(CXX) -o $@ $(CFLAGS) $(OSFLAGS) $^ $(MIDASLIBS) $(LIBS) -lvme

analyzer.exe: $(LIB_DIR)/rmana.o analyzer.o modules.o $(MIDASLIBS)
	g++ -o $@ -m32 -O2 -g $^ $(ROOTLIBS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(OSFLAGS) -c $<

%.o: %.cxx
	$(CXX) $(CXXFLAGS) $(OSFLAGS) -c $<

clean::
	-rm -f *.o *.exe

# end
