#
# Simple makefile
#


CC = c51.exe
LL = bL51.exe

CFLAGS = "OPTIMIZE(8,SPEED)" BROWSE DEBUG OBJECTEXTEND "TABS(3)" "INCDIR(../)" "DEFINE(SCS_2000)"
LFLAGS = "RAMSIZE (256)" "DISABLEWARNING (16)" "CODE (?PR?UPGRADE?___MSCBMAIN (0XF000))"
IFLAGS = -kr -nut -i3 -l90

all: scs_2000.hex


scs_2000.OBJ: scs_2000.c ../mscbemb.h
	$(CC) scs_2000.c $(CFLAGS)

scs_2000_lib.OBJ: scs_2000_lib.c ../mscbemb.h
	$(CC) scs_2000_lib.c $(CFLAGS)

../mscbmain.OBJ: ../mscbmain.c ../mscbemb.h
	$(CC) ../mscbmain.c $(CFLAGS) 

../mscbutil.OBJ: ../mscbutil.c ../mscbemb.h
	$(CC) ../mscbutil.c $(CFLAGS) 

../lcd_menu.OBJ: ../lcd_menu.c ../mscbemb.h
	$(CC) ../lcd_menu.c $(CFLAGS) 

scs_2000.hex: scs_2000.OBJ scs_2000_lib.OBJ ../mscbmain.OBJ ../mscbutil.OBJ ../lcd_menu.OBJ
	$(LL) scs_2000.obj, ../mscbmain.obj, ../mscbutil.obj, ../lcd_menu.obj, scs_2000_lib.obj TO scs_2000 $(LFLAGS)
	oh51 scs_2000

indent:
	indent $(IFLAGS) *.c

clean:
	rm -f *~ *.OBJ ../mscbmain.obj ../mscbutil.obj ../lcd_menu.obj scs_2000 *.hex
