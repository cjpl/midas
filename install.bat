echo off
rem Install MIDAS system under Windows NT

echo Installing MIDAS libraries
start net stop midas
copy nt\lib\midas.dll %SystemRoot%\system32
if not exist \midas\nt\lib mkdir \midas\nt\lib
copy nt\lib\*.* \midas\nt\lib

echo Installing MIDAS executables
if not exist \midas\nt\bin mkdir \midas\nt\bin
copy nt\bin\*.* \midas\nt\bin 

echo Installing MIDAS source and include files
if not exist \midas\include mkdir \midas\include
copy include\*.* \midas\include

echo Installing MIDAS drivers
if not exist \midas\drivers mkdir \midas\drivers
xcopy /s drivers\*.* \midas\drivers