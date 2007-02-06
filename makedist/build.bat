@echo off
rem Usage: build <version> like "build 161"
rem Build MIDAS version

echo Have you tagged the release? 
echo (svn tag from file:///y:/svn/midas/trunk to 
echo               file:///y:/svn/midas/tags/midas-x-x-x) 
echo If not, enter Ctrl-Break now and do so!

sleep 10

rem go to midas root
cd ..

set version=%1
set dir=\tmp\midas%version%
set archive=\tmp\midas%version%.zip

rem create temporary directory
rmdir /S /Q %dir% > nul
mkdir \tmp > nul

rem export files
echo Getting standard files...
cd ..
svn export file:///y:/svn/midas/trunk %dir%
cd midas

echo on

copy install.bat %dir% > nul

copy doc\readme.txt %dir% > nul
copy doc\install_nt.txt %dir%\install.txt > nul
copy doc\overview.txt %dir% > nul


echo Creating archive...
cd \tmp
zip -r midas.zip midas%version%

cd \midas
rm midas%version%.zip > nul
move \tmp\midas.zip midas%version%.zip > nul
rmdir /S /Q %dir% > nul

echo Sending archive to midas.psi.ch

scp midas%version%.zip ritt@midas:html/midas/download/windows/
