@echo off

set CYGWIN=C:\cygwin\bin
set CHERE_INVOKING=1

if not exist %CYGWIN%\bash.exe set CYGWIN=C:\msys\1.0\bin

%CYGWIN%\bash --login -i -c 'make clean'