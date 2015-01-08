@REM This cmd.exe batch script generates
@REM   a package-based Boost distribution
@REM   for use with bpm.
@REM 
@REM It needs to be run from the Boost root.
@REM 
@REM Copyright 2014, 2015 Peter Dimov
@REM 
@REM Distributed under the Boost Software License, Version 1.0.
@REM See accompanying file LICENSE_1_0.txt or copy at
@REM http://www.boost.org/LICENSE_1_0.txt

FOR /f %%i IN ('git rev-parse HEAD') DO @SET REV=%%i

FOR /f %%i IN ('git rev-parse --short HEAD') DO @SET SHREV=%%i

FOR /f %%i IN ('git rev-parse --abbrev-ref HEAD') DO @SET BRANCH=%%i

SET OUTDIR=..\pkg-%BRANCH%-%SHREV%

mkdir %OUTDIR%

dist\bin\boostdep.exe --track-sources --list-dependencies | xz --format=lzma > %OUTDIR%\dependencies.txt.lzma
dist\bin\boostdep.exe --list-buildable | xz --format=lzma > %OUTDIR%\buildable.txt.lzma

FOR /d %%i IN (libs/*) DO tar cf %OUTDIR%\%%i.tar.lzma --lzma libs/%%i/

tar cf %OUTDIR%\build.tar.lzma --lzma b2.exe boost-build.jam boostcpp.jam Jamroot libs/Jamfile.v2 tools/build/
