@echo off

setlocal

set common=/nologo /W4  /WX /std:c11

set build_type=/Z7

set profile=/D PROFILE

:argactionstart
if -%1-==-- goto argactionend
if "%1"=="noprofile" set profile=
if "%1"=="release" set build_type=/O2 /Z7
shift
goto argactionstart
:argactionend

del *.pdb

nasm -f win64 asm.asm
lib /nologo  asm.obj

REM cl %common% input.c
REM cl %common% test.c
REM cl %common% %profile% %build_type% main.c
REM cl %common% %profile% %build_type% Advapi32.lib asm.lib repetition.c
REM cl %common% %profile% %build_type%  asm.lib asm.c
REM cl %common% %profile% %build_type%  asm.lib cache.c
REM cl %common% %profile% %build_type%  asm.lib sets.c
cl %common% %profile% %build_type%  asm.lib nontemporal.c
REM cl %common% %profile% %build_type% faults.c

del *.obj *.ilk *.lib


if not exist data mkdir data

endlocal