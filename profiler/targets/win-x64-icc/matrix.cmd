@echo off
set SGBETEMPDIR=%cd%
"C:\Program Files (x86)\IntelSWTools\compilers_and_libraries_2018.3.210\windows\bin\iclvars.bat" intel64 > nul && make matrix -R -C ../../targets/win-x64-icc GAP=%SGBETEMPDIR%\testcode.c STUB=%SGBETEMPDIR%\testcode.h CC="icl" CFLAGS="/O3 /G7 /QxSSSE3" OUTDIR=%SGBETEMPDIR% -s  > matrix.txt