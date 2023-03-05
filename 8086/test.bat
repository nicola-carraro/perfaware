del data\*.out
nasm data\listing%1.asm -o data\listing%1.out
8086.exe data\listing%1.out > data\output.asm
nasm data\output.asm -o data\output.out
fc data\output.out data\listing%1.out
