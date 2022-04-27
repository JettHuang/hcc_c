@REM Please setup visual studio development environment:
@REM      C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat
@REM

@REM call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"

@echo off

set CCROOT=F:\GitHub\hcc_c\dist
set CPP=%CCROOT%\bin\hcpp_d.exe
set CC=%CCROOT%\bin\hcc_d.exe

set CFLAGS=-D__NO_ISOCEXT -Dwin32 -D_WIN32 -D_M_IX86


if exist fib.i del fib.i
if exist fib.asm del fib.asm
if exist fib.obj del fib.obj
if exist fib.exe del fib.exe

%CPP% %CFLAGS% -o fib.i fib.c
if errorlevel 1 goto errcpp

%CC% -o fib.asm fib.i
if errorlevel 1 goto errcc

ml.exe /c /coff fib.asm
if errorlevel 1 goto errasm

link.exe /subsystem:console /entry:mainCRTStartup "oldnames.lib" "msvcrt.lib" "legacy_stdio_definitions.lib" "kernel32.lib" fib.obj 
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

