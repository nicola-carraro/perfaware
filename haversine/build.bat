@echo off
cl /nologo /W4 /Z7 /WX input.c
cl /nologo /W4 /Z7 /WX test.c
del *.obj *.ilk