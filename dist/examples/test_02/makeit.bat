@REM Please setup visual studio development environment:
@REM      C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat
@REM

@REM call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"

@echo off

set CCROOT=F:\GitHub\hcc_c\dist
set INCLUDE=%CCROOT%\include
set CPP=%CCROOT%\bin\hcpp_d.exe
set CC=%CCROOT%\bin\hcc_d.exe

set CFLAGS=-D__NO_ISOCEXT -Dwin32 -D_WIN32 -D_M_IX86


if exist test.i del test.i
if exist test.asm del test.asm
if exist test.obj del test.obj
if exist test.exe del test.exe

%CPP% %CFLAGS% -o test.i test.c
if errorlevel 1 goto errcpp

%CC% -o test.asm test.i
if errorlevel 1 goto errcc

ml.exe /c /coff test.asm
if errorlevel 1 goto errasm

link.exe /subsystem:console /entry:mainCRTStartup "oldnames.lib" "msvcrt.lib" "legacy_stdio_definitions.lib" "kernel32.lib" test.obj 
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

