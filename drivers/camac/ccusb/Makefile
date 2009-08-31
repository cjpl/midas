#####################################################################
#
#  Name:         Makefile
#
#####################################################################

CFLAGS = -g -O2 -Wall -Wuninitialized
LIBS = -lm -lz -lutil -lnsl -lpthread

#-------------------------------------------------------------------

MIDASSYS := $(HOME)/daq/midas/midas
MIDASLIB := $(MIDASSYS)/linux/lib

CFLAGS += -g -O2 -DOS_LINUX -DHAVE_LIBUSB -I$(MIDASSYS)/include
CXXFLAGS = $(CFLAGS)

ALL= test_ccusb ccusb_flash feccusb

all: $(ALL)

clean::
	-rm -f $(ALL)

feccusb: %: %.o ccusb.o musbstd.o $(MIDASLIB)/mfe.o $(MIDASLIB)/libmidas.a
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LDFEFLAGS) $(LIBS) -lusb

musbstd.o: %.o: $(MIDASSYS)/drivers/usb/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

test_ccusb: %: %.cxx ccusb.o musbstd.o
	$(CXX) $(CFLAGS) -DMAIN -o $@ $< ccusb.o musbstd.o -lusb

ccusb_flash: %: %.o
	$(CXX) -o $@ $(CXXFLAGS) $^ -lusb

ccusb.o: %.o: $(MIDASSYS)/drivers/camac/ccusb/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

libxxusb.o: %.o: ./%.c
	$(CC) $(CFLAGS) -I. -c -o $@ $<

libxxusb: %: %.o
	$(CC) $(CFLAGS) -I. -o $@ $< -lusb

%.o: %.c
	$(CXX) $(USERFLAGS) $(CFLAGS) -o $@ -c $<

%.o: %.cxx
	$(CXX) -c $(CFLAGS) -I$(MIDASSYS)/include $<

clean::
	rm -f *.o *~ \#*

#end file
