@echo off
rem Usage: build <version> like "build 100"
rem Build MSCB version

set version=%1
set archive=\tmp\mscb%version%.zip

rem Compiling new DLL
rem cd \midas\nt
rem nmake -f mscb.mak CFG="mscb - Win32 Debug"
rem nmake -f msc.mak CFG="msc - Win32 Debug"
rem cd \midas\mscb

copy \midas\nt\bin\msc.exe \midas\mscb\
copy \midas\nt\lib\mscb.dll \midas\mscb\labview\

echo Creating archive...

zip mscb%version%.zip *.c
zip mscb%version%.zip *.h
zip mscb%version%.zip msc.exe

zip -p mscb%version%.zip embedded/*.h
zip -p mscb%version%.zip embedded/*.c
zip -p mscb%version%.zip embedded/*.Uv2
zip -p mscb%version%.zip embedded/*.hex

zip -p mscb%version%.zip labview/*.dll
zip -p mscb%version%.zip labview/*.vi

rem map network drive
net use m: /d > nul
net use m: \\pc2075\midas mi_das /user:midas

echo Sending archive to midas.psi.ch

cp mscb%version%.zip m:\html\mscb\software\download

rm mscb%version%.zip > nul
