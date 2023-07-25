@echo off

setlocal

set common=/nologo /W4 /Z7 /WX

set build_type=

set profile=/D PROFILE

:argactionstart
if -%1-==-- goto argactionend
if "%1"=="noprofile" set profile=
if "%1"=="release" set build_type=/O2 /Fd /D NODEBUG
shift
goto argactionstart
:argactionend

cl %common% input.c
cl %common% test.c
cl %common% %profile% %build_type% main.c
del *.obj *.ilk

if not exist data mkdir data

endlocal