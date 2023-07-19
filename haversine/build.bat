@echo off
cl /nologo /W4 /Z7 /WX input.c
cl /nologo /W4 /Z7 /WX test.c
cl /nologo /W4 /Z7 /WX  main.c
del *.obj *.ilk