@echo off
rem Usage: build <version> like "build 100"
rem Build MSCB version

set version=%1
set archive=\tmp\mscb%version%.zip

rem Compiling new DLL
cd \midas\nt
nmake -f mscb.mak CFG="mscb - Win32 Release" clean
nmake -f mscb.mak CFG="mscb - Win32 Release"
nmake -f msc.mak CFG="msc - Win32 Release" Clean
nmake -f msc.mak CFG="msc - Win32 Release"
nmake -f calib_700.mak CFG="calib_700 - Win32 Release" Clean
nmake -f calib_700.mak CFG="calib_700 - Win32 Release"
nmake -f calib_900.mak CFG="calib_900 - Win32 Release" Clean
nmake -f calib_900.mak CFG="calib_900 - Win32 Release"
nmake -f calib_1000.mak CFG="calib_1000 - Win32 Release" Clean
nmake -f calib_1000.mak CFG="calib_1000 - Win32 Release"
nmake -f calib_hvr.mak CFG="calib_hvr - Win32 Release" Clean
nmake -f calib_hvr.mak CFG="calib_hvr - Win32 Release"

cd \midas\mscb\embedded
make

cd \midas\mscb

copy \midas\nt\bin\msc.exe \midas\mscb\
copy \midas\nt\bin\calib_700.exe \midas\mscb\
copy \midas\nt\bin\calib_900.exe \midas\mscb\
copy \midas\nt\bin\calib_1000.exe \midas\mscb\
copy \midas\nt\bin\calib_hvr.exe \midas\mscb\
copy \midas\nt\lib\mscb.dll \midas\mscb\labview\

echo Creating archive...

zip mscb%version%.zip Makefile
zip mscb%version%.zip *.c
zip mscb%version%.zip *.h
zip mscb%version%.zip msc.exe
zip mscb%version%.zip calib_700.exe
zip mscb%version%.zip calib_900.exe
zip mscb%version%.zip calib_1000.exe
zip mscb%version%.zip calib_hvr.exe

zip -rp mscb%version%.zip embedded -i \*.h
zip -rp mscb%version%.zip embedded -i \*.c
zip -rp mscb%version%.zip embedded -i \*.Uv2
zip -rp mscb%version%.zip embedded -i \*.hex

zip -p mscb%version%.zip labview/*.dll
zip -p mscb%version%.zip labview/*.llb
zip -p mscb%version%.zip labview/*.vi

zip -p mscb%version%.zip drivers/windows/mscbusb.inf
zip -p mscb%version%.zip drivers/windows/mscbusb.sys


zip -p mscb%version%.zip \mxml\*.*
zip -p mscb%version%.zip \midas\include\musbstd.h
zip -p mscb%version%.zip \midas\drivers\usb\musbstd.c

echo Sending archive to midas.psi.ch

scp mscb%version%.zip ritt@midas:html/midas/mscb/software/download

rm mscb%version%.zip > nul
