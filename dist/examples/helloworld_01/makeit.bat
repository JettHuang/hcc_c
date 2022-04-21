@REM Please setup visual studio development environment:
@REM      C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat
@REM

@echo off

set CCROOT=F:\GitHub\hcc_c\dist
set PATH=%PATH%;%CCROOT%\bin
set INCLUDE=%CCROOT%\include
set CPP=hcpp_d.exe
set CC=hcc_d.exe

set CFLAGS=-D__NO_ISOCEXT -Dwin32 -D_WIN32 -D_M_IX86


if exist helloworwld.i del helloworld.i
if exist helloworwld.asm del helloworld.asm
if exist helloworwld.obj del helloworld.obj
if exist helloworld.exe del helloworld.exe

%CPP% %CFLAGS% -o helloworld.i helloworld.c
if errorlevel 1 goto errcpp

%CC% -o helloworld.asm helloworld.i
if errorlevel 1 goto errcc

ml.exe /c /coff helloworld.asm
if errorlevel 1 goto errasm

link.exe /subsystem:console /entry:mainCRTStartup "oldnames.lib" "msvcrt.lib" "legacy_stdio_definitions.lib" "kernel32.lib" helloworld.obj 
if errorlevel 1 goto errlink

goto TheEnd

:errlink
echo Link error
goto TheEnd

:errasm
echo Assembly Error
goto TheEnd

:errcpp
echo c preprocesser error
goto TheEnd

:errcc
echo c compile error
goto TheEnd

:noMASM
echo MASM32 is required...
goto TheEnd

:TheEnd

pause

