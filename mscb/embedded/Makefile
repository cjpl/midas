#
# Simple makefile
#


CC = c51.exe
LL = bL51.exe

CFLAGS = "OPTIMIZE(9,SIZE)" BROWSE DEBUG OBJECTEXTEND "TABS(3)"
LFLAGS = "RAMSIZE (256)" "DISABLEWARNING (16)" "CODE (?PR?UPGRADE?MSCBMAIN (0X7000))"
IFLAGS = -kr -nut -i3 -l90
EXECS = scs_210 scs_220 scs_300 scs_310 scs_320 scs_400 scs_520 scs_600 scs_700 scs_800 scs_900 scs_910 scs_920 hvr_test
SRCS = scs_210.c scs_220.c scs_300.c scs_310.c scs_320.c scs_400.c scs_520.c scs_600.c scs_700.c scs_800.c scs_900.c scs_910.c scs_920.c

all: $(EXECS) hvr_200.hex hvr_400.hex hvr_500.hex scs_1000.hex scs_1001.hex subm_250.hex subm_260.hex subm_300.hex

$(EXECS): $(SRCS) mscbmain.c mscbutil.c mscbemb.h
	$(CC) $@.c "DEFINE($@)" $(CFLAGS) 
	$(CC) mscbmain.c "DEFINE($@)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE($@)" $(CFLAGS) 
	$(LL) mscbmain.obj, mscbutil.obj, $@.obj TO $@ $(LFLAGS)
	oh51 $@

hvr_200.hex: hvr_200.c mscbmain.c mscbutil.c mscbemb.h
	$(CC) hvr_200.c "DEFINE(HVR_200)" $(CFLAGS) 
	$(CC) mscbmain.c "DEFINE(HVR_200)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(HVR_200)" $(CFLAGS) 
	$(LL) mscbmain.obj, mscbutil.obj, hvr_200.obj TO hvr_200 "RAMSIZE (256)" "DISABLEWARNING (16)" "CODE (?PR?UPGRADE?MSCBMAIN (0XF000))"
	oh51 hvr_200

hvr_400.hex: hvr_400.c mscbmain.c mscbutil.c mscbemb.h
	$(CC) hvr_400.c "DEFINE(HVR_400)" $(CFLAGS) 
	$(CC) mscbmain.c "DEFINE(HVR_400)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(HVR_400)" $(CFLAGS) 
	$(LL) mscbmain.obj, mscbutil.obj, hvr_400.obj TO hvr_400 "RAMSIZE (256)" "DISABLEWARNING (16)" "CODE (?PR?UPGRADE?MSCBMAIN (0X3200))"
	oh51 hvr_400

hvr_500.hex: hvr_500.c mscbmain.c mscbutil.c mscbemb.h
	$(CC) hvr_500.c "DEFINE(HVR_500)" $(CFLAGS) 
	$(CC) mscbmain.c "DEFINE(HVR_500)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(HVR_500)" $(CFLAGS) 
	$(LL) mscbmain.obj, mscbutil.obj, hvr_500.obj TO hvr_500 "RAMSIZE (256)" "DISABLEWARNING (16)" "CODE (?PR?UPGRADE?MSCBMAIN (0X3200))"
	oh51 hvr_500

scs_1000.hex: scs_1000.c mscbmain.c mscbutil.c lcd_menu.c mscbemb.h
	$(CC) scs_1000.c "DEFINE(SCS_1000)" $(CFLAGS) 
	$(CC) mscbmain.c "DEFINE(SCS_1000)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(SCS_1000)" $(CFLAGS) 
	$(CC) lcd_menu.c "DEFINE(SCS_1000)" $(CFLAGS) 
	$(LL) mscbmain.obj, mscbutil.obj, lcd_menu.obj, scs_1000.obj TO scs_1000 "RAMSIZE (256)" "DISABLEWARNING (16)" "CODE (?PR?UPGRADE?MSCBMAIN (0X7000))"
	oh51 scs_1000

scs_1001.hex: scs_1001.c mscbmain.c mscbutil.c lcd_menu.c mscbemb.h
	$(CC) scs_1001.c "DEFINE(SCS_1001)" $(CFLAGS) 
	$(CC) mscbmain.c "DEFINE(SCS_1001)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(SCS_1001)" $(CFLAGS) 
	$(CC) lcd_menu.c "DEFINE(SCS_1001)" $(CFLAGS) 
	$(LL) mscbmain.obj, mscbutil.obj, lcd_menu.obj, scs_1001.obj TO scs_1001 "RAMSIZE (256)" "DISABLEWARNING (16)" "CODE (?PR?UPGRADE?MSCBMAIN (0X7000))"
	oh51 scs_1001

subm_250.hex: usb.c mscbutil.c subm_250.c usb.h mscbemb.h
	$(CC) subm_250.c "DEFINE(SUBM_250)" $(CFLAGS) 
	$(CC) usb.c "DEFINE(SUBM_250)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(SUBM_250)" $(CFLAGS) 
	$(LL) usb.obj, mscbutil.obj, subm_250.obj TO subm_250 "RAMSIZE (256)" "DISABLEWARNING (16)" 
	oh51 subm_250

subm_260.hex: netlib.lib mscbutil.c subm_250.c mscbemb.h
	$(CC) subm_260.c "DEFINE(SUBM_260)" LARGE $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(SUBM_260)" LARGE $(CFLAGS) 
	$(LL) netlib.lib, mscbutil.obj, subm_260.obj TO subm_260 "RAMSIZE (256)" "DISABLEWARNING (16)" 
	oh51 subm_260

subm_300.hex: subm_300.c mscbutil.c mscbemb.h
	$(CC) subm_300.c "DEFINE(SUBM_300)" $(CFLAGS) 
	$(CC) mscbutil.c "DEFINE(SUBM_300)" $(CFLAGS) 
	$(LL) subm_300.obj, mscbutil.obj TO subm_300 "RAMSIZE (256)" "DISABLEWARNING (16)" 
	oh51 subm_300

indent:
	indent $(IFLAGS) *.c

clean:
	rm -f *~ *.OBJ $(EXECS) hvr_400 hvr_500 scs_1000 subm_250 subm_300 *.hex
