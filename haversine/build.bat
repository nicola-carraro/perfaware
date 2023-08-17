@echo off

setlocal

set common=/nologo /W4  /WX

set build_type=/Z7

set profile=/D PROFILE

:argactionstart
if -%1-==-- goto argactionend
if "%1"=="noprofile" set profile=
if "%1"=="release" set build_type=/O2 /Fd /D NODEBUG /D NDEBUG 
shift
goto argactionstart
:argactionend

cl %common% input.c
cl %common% test.c
cl %common% %profile% %build_type% main.c
del *.obj *.ilk

if not exist data mkdir data

endlocal