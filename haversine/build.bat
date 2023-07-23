@echo off

setlocal

set common=/nologo /W4 /Z7 /WX

set main=

set defines=/D PROFILE

:argactionstart
if -%1-==-- goto argactionend
if "%1"=="noprofile" set defines=
if "%1"=="release" set common=%common% /O2
if "%1"=="release" set defines=%defines% /Fd /D NODEBUG
shift
goto argactionstart
:argactionend

cl %common% input.c
cl %common% test.c
cl %common% %defines% main.c
del *.obj *.ilk

if not exist data mkdir data

endlocal