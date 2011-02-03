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
nmake -f calib_hvr.mak CFG="calib_hvr - Win32 Release" Clean
nmake -f calib_hvr.mak CFG="calib_hvr - Win32 Release"

cd \midas\mscb

copy \midas\nt\bin\msc.exe \midas\mscb\
copy \midas\nt\bin\calib_hvr.exe \midas\mscb\
copy \midas\nt\lib\mscb.dll \midas\mscb\labview\

echo Creating archive...

zip mscb%version%.zip Makefile
zip mscb%version%.zip *.c
zip mscb%version%.zip *.h
zip mscb%version%.zip msc.exe
zip mscb%version%.zip libusb0.dll
zip mscb%version%.zip calib_hvr.exe

zip -rp mscb%version%.zip embedded -i \*.h
zip -rp mscb%version%.zip embedded -i \*.c
zip -rp mscb%version%.zip embedded -i \*.A51
zip -rp mscb%version%.zip embedded -i \*.uvproj
zip -rp mscb%version%.zip embedded -i \*.uvopt
zip -rp mscb%version%.zip embedded -i \*.hex

zip -p mscb%version%.zip labview/*.dll
zip -p mscb%version%.zip labview/*.llb
zip -p mscb%version%.zip labview/*.vi

zip -p mscb%version%.zip drivers/windows/libusb/libusb0.dll
zip -p mscb%version%.zip drivers/windows/libusb/libusb0.sys
zip -p mscb%version%.zip drivers/windows/libusb/mscblibusb.inf


zip -p mscb%version%.zip \mxml\*.*
zip -p mscb%version%.zip \midas\include\musbstd.h
zip -p mscb%version%.zip \midas\drivers\usb\musbstd.c

echo Sending archive to midas.psi.ch

copy mscb%version%.zip x:\html\midas\mscb\software\download

rm mscb%version%.zip > nul
