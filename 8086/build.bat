@echo off
cl /nologo /W4 /Z7 /WX 8086.c
cl /nologo /W4 /Z7 /WX test.c
del *.obj *.ilk