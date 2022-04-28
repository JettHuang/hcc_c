@REM Please setup visual studio development environment:
@REM      C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat
@REM

@REM call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\Common7\Tools\VsDevCmd.bat"

@echo off

set CCROOT=F:\GitHub\hcc_c\dist
set CPP=%CCROOT%\bin\hcpp_d.exe
set CC=%CCROOT%\bin\hcc_d.exe

set CFLAGS=-D__NO_ISOCEXT -Dwin32 -D_WIN32 -D_M_IX86


if exist hellowin.i del hellowin.i
if exist hellowin.asm del hellowin.asm
if exist hellowin.obj del hellowin.obj
if exist hellowin.exe del hellowin.exe

%CPP% %CFLAGS% -o hellowin.i hellowin.c
if errorlevel 1 goto errcpp

%CC% -o hellowin.asm --checkret=0 --inline=0 hellowin.i
if errorlevel 1 goto errcc

ml.exe /c /coff hellowin.asm
if errorlevel 1 goto errasm

set LIBS="oldnames.lib" "msvcrt.lib" "legacy_stdio_definitions.lib" "kernel32.lib" "user32.lib" "Gdi32.lib"
link.exe /subsystem:windows /entry:WinMainCRTStartup %LIBS% hellowin.obj 
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

