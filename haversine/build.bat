@echo off

setlocal

set common=/nologo /W4 /Z7 /WX

set defines=/D PROFILE

if "%1"=="noprofile" set defines=

cl %common% input.c
cl %common% test.c
cl %common% %defines% main.c
del *.obj *.ilk

endlocal