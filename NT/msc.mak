# Microsoft Developer Studio Generated NMAKE File, Based on msc.dsp
!IF "$(CFG)" == ""
CFG=msc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to msc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "msc - Win32 Release" && "$(CFG)" != "msc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "msc.mak" CFG="msc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "msc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "msc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "msc - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "\midas\mscb\msc.exe"


CLEAN :
	-@erase "$(INTDIR)\msc.obj"
	-@erase "$(INTDIR)\mscb.obj"
	-@erase "$(INTDIR)\mscbrpc.obj"
	-@erase "$(INTDIR)\musbstd.obj"
	-@erase "$(INTDIR)\strlcpy.obj"
	-@erase "$(INTDIR)\mxml.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "\midas\mscb\msc.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE" /D "HAVE_LIBUSB" /Fp"$(INTDIR)\msc.pch" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c /I "\mxml" /I "\midas\include" /I "\midas\mscb\drivers\windows\libusb\include"

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=wsock32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\msc.pdb" /machine:I386 /out:"c:\midas\mscb\msc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\msc.obj" \
	"$(INTDIR)\mscb.obj" \
	"$(INTDIR)\strlcpy.obj" \
	"$(INTDIR)\musbstd.obj" \
	"$(INTDIR)\mxml.obj" \
	"$(INTDIR)\mscbrpc.obj" \
	"\midas\mscb\drivers\windows\libusb\lib\libusb.lib"       


"\midas\mscb\msc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "msc - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "\midas\mscb\msc.exe" "$(OUTDIR)\msc.bsc"


CLEAN :
	-@erase "$(INTDIR)\msc.obj"
	-@erase "$(INTDIR)\msc.sbr"
	-@erase "$(INTDIR)\mscb.obj"
	-@erase "$(INTDIR)\mscb.sbr"
	-@erase "$(INTDIR)\mscbrpc.obj"
	-@erase "$(INTDIR)\mscbrpc.sbr"
	-@erase "$(INTDIR)\strlcpy.obj"
	-@erase "$(INTDIR)\strlcpy.sbr"
	-@erase "$(INTDIR)\musbstd.obj"
	-@erase "$(INTDIR)\musbstd.sbr"
	-@erase "$(INTDIR)\mxml.obj"
	-@erase "$(INTDIR)\mxml.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\msc.bsc"
	-@erase "$(OUTDIR)\msc.pdb"
	-@erase "\midas\mscb\msc.exe"
	-@erase "\midas\mscb\msc.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "_CRT_SECURE_NO_DEPRECATE" /D "_CRT_NONSTDC_NO_DEPRECATE" /D "HAVE_LIBUSB" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\msc.pch" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c /I "\mxml" /I "\midas\include" /I "\midas\mscb\drivers\windows\libusb\include"

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\msc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\msc.sbr" \
	"$(INTDIR)\mscb.sbr" \
	"$(INTDIR)\strlcpy.sbr" \
	"$(INTDIR)\musbstd.sbr" \
	"$(INTDIR)\mxml.sbr" \
	"$(INTDIR)\mscbrpc.sbr"


"$(OUTDIR)\msc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=wsock32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\msc.pdb" /debug /machine:I386 /out:"c:\midas\mscb\msc.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\msc.obj" \
	"$(INTDIR)\mscb.obj" \
	"$(INTDIR)\strlcpy.obj" \
	"$(INTDIR)\musbstd.obj" \
	"$(INTDIR)\mxml.obj" \
	"$(INTDIR)\mscbrpc.obj" \
	"\midas\mscb\drivers\windows\libusb\lib\libusb.lib"       


"\midas\mscb\msc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "msc - Win32 Release" || "$(CFG)" == "msc - Win32 Debug"
SOURCE=..\mscb\msc.c

!IF  "$(CFG)" == "msc - Win32 Release"


"$(INTDIR)\msc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "msc - Win32 Debug"


"$(INTDIR)\msc.obj"	"$(INTDIR)\msc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\mscb\mscb.c

!IF  "$(CFG)" == "msc - Win32 Release"


"$(INTDIR)\mscb.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "msc - Win32 Debug"


"$(INTDIR)\mscb.obj"	"$(INTDIR)\mscb.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\mscb\mscbrpc.c

!IF  "$(CFG)" == "msc - Win32 Release"


"$(INTDIR)\mscbrpc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "msc - Win32 Debug"


"$(INTDIR)\mscbrpc.obj"	"$(INTDIR)\mscbrpc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=\mxml\strlcpy.c

!IF  "$(CFG)" == "msc - Win32 Release"


"$(INTDIR)\strlcpy.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "msc - Win32 Debug"


"$(INTDIR)\strlcpy.obj"	"$(INTDIR)\strlcpy.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=\mxml\mxml.c

!IF  "$(CFG)" == "msc - Win32 Release"


"$(INTDIR)\mxml.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "msc - Win32 Debug"


"$(INTDIR)\mxml.obj"	"$(INTDIR)\mxml.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=\midas\drivers\usb\musbstd.c

!IF  "$(CFG)" == "msc - Win32 Release"


"$(INTDIR)\musbstd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "msc - Win32 Debug"


"$(INTDIR)\musbstd.obj"	"$(INTDIR)\musbstd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

!ENDIF 

