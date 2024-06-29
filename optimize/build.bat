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

cl %common% %profile% %build_type% hav0.c
cl %common% %profile% %build_type% hav1.c
cl %common% %profile% %build_type% file.c
cl %common% %profile% %build_type% Advapi32.lib hav2.c


del *.obj *.ilk *.lib


if not exist data mkdir data

endlocal