@REM Please setup visual studio development environment:
@REM      C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat
@REM

@REM call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"

@echo off

set CCROOT=F:\GitHub\hcc_c\dist
set CPP=%CCROOT%\bin\hcpp_d.exe
set CC=%CCROOT%\bin\hcc_d.exe

set CFLAGS=-D__NO_ISOCEXT -Dwin32 -D_WIN32 -D_M_IX86


if exist calc24.i del calc24.i
if exist calc24.asm del calc24.asm
if exist calc24.obj del calc24.obj
if exist calc24.exe del calc24.exe

%CPP% %CFLAGS% -o calc24.i calc24.c
if errorlevel 1 goto errcpp

%CC% -o calc24.asm calc24.i
if errorlevel 1 goto errcc

ml.exe /c /coff calc24.asm
if errorlevel 1 goto errasm

link.exe /subsystem:console /entry:mainCRTStartup "oldnames.lib" "msvcrt.lib" "legacy_stdio_definitions.lib" "kernel32.lib" calc24.obj 
if errorlevel 1 goto errlink

goto TheEnd

:errlink
echo Link error
goto TheEnd

:errasm
echo Assembly Error
goto TheEnd

:errcpp
echo C preprocesser error
goto TheEnd

:errcc
echo C compile error
goto TheEnd

:TheEnd

