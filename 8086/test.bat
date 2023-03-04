del *.out
nasm listing38 -o listing38.out
8086.exe listing38.out > 8086
nasm 8086 -o 8086.out
fc listing38.out 8086.out
