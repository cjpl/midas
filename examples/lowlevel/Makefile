#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     UNIX Makefile for MIDAS slow control frontend
#
#  $Id$
#
#####################################################################

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

# This is for Linux ----------------
LIBS = -lutil -lpthread
OSFLAGS = -DOS_LINUX -fPIC

# This is for OSF1 -----------------
#LIBS = -lbsd
#OSFLAGS = -DOS_OSF1

# This is for Ultrix ---------------
#LIBS =
#OSFLAGS = -DOS_ULTRIX -Dextname

# This is for FreeBSD --------------
#LIBS = -lbsd -lcompat
#OSFLAGS = -DOS_FREEBSD

#-------------------------------------------------------------------
# The following lines define direcories. Adjust if necessary
#                 
INC_DIR 	= ../../include
LIB_DIR 	= ../../linux/lib

####################################################################
# Lines below here should not be edited
####################################################################

LIB = $(LIB_DIR)/libmidas.a

# compiler
CC = cc
CFLAGS = -O2 -g -Wall -Wuninitialized -I$(INC_DIR) -L$(LIB_DIR)

PROGS = produce consume rpc_test rpc_clnt rpc_srvr
all: $(PROGS)

$(PROGS): %: %.c $(LIB)
	$(CC) $(CFLAGS) $(OSFLAGS) -o $@ $< -lmidas $(LIBS)

clean:
	rm -f $(PROGS) *~ \#*

