#
#  Makefile for msc executable under linux
#
#  27.11.2002
#  Tomasz Brys
#  tomasz.brys@psi.ch
#

OUTNAME       = msc 
CC            = gcc -g
FLAGS         = -Wall
LIBS	      = -lusb

all: $(OUTNAME)

$(OUTNAME): mscb.o msc.o rpc.o
	$(CC) $(FLAGS) mscb.o msc.o rpc.o -o $(OUTNAME) $(LIBS)

rpc.o: rpc.c rpc.h
	$(CC) $(FLAGS) -c rpc.c

mscb.o: mscb.c mscb.h
	$(CC) $(FLAGS) -c mscb.c 

msc.o: msc.c
	$(CC) $(FLAGS) -c msc.c 

clean:
	rm *.o $(OUTNAME)

