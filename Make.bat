@echo off

set CYGWIN=C:\cygwin\bin
set CHERE_INVOKING=1
%CYGWIN%\bash --login -i -c 'make; exec bash'

pause
