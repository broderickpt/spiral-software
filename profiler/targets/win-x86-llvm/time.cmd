@echo off

REM  Copyright (c) 2018-2021, Carnegie Mellon University
REM  See LICENSE for details

set SGBETEMPDIR=%cd%

set OLDPATH=%PATH%
set PATH_FOR_PROFILER_COMPILER="C:\LLVM\bin"
set PATH=%PATH%;%PATH_FOR_PROFILER_COMPILER%

REM  When SPIRAL is installed using the windows installer and the LLVM clang compiler
REM  is selected the user must specify the path to clang.exe; this file is then
REM  updated with that path.  If a user builds SPIRAL standalone the
REM  PATH_FOR_PROFILER_COMPILER variable should be customized for the user's
REM  environment. 

make -R -C ../../targets/win-x86-llvm GAP=%SGBETEMPDIR%\testcode.c STUB=%SGBETEMPDIR%\testcode.h CC="clang" CFLAGS="-O2 -march=native -std=c99 -w" OUTDIR=%SGBETEMPDIR% -s > time.txt

set PATH=%OLDPATH%
