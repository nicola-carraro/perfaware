
global movAllBytes

global nopAllBytes

global cmpAllBytes

global decAllBytes

global decSlow

global nop3x1

global nop1x3

global nop9

global jumps

global align64

global align1

global align15

global align62

global align63

global read1

global read2

global read3

global read4

global write1

global write2

global write3

global write4

global read2x4

global read2x8

global read2x16

global read1x32

global read2x32

global read2x64

global read1x64

global avx512_zmm

global testCache

global testCacheAnd

global testCacheUnaligned

global testCacheUnalignedNonContiguous

global testCacheSet

global cacheSetComparison

global testTemporal

global testNonTemporal

section .text

movAllBytes:
    xor rax, rax
.loop:
    mov  [rdx + rax], al  
    inc  rax  
    cmp  rax,rcx  
    jb   .loop
    ret

nopAllBytes:
    xor rax, rax
.loop:
    db  0x0f, 0x1f, 0x00  
    inc  rax  
    cmp  rax,rcx  
    jb   .loop
    ret

cmpAllBytes:
    xor rax, rax
.loop:
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

decAllBytes:
.loop:
    dec rcx
    jnz .loop
    ret

decSlow:
    xor rax, rax
.loop:
    dec  rcx  
    cmp  rax, rcx  
    jb   .loop
    ret

nop3x1:
    xor rax, rax
.loop:
    db  0x0f, 0x1f, 0x00
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

nop1x3:
    xor rax, rax
.loop:
    nop
    nop
    nop
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

nop9:
    xor rax, rax
.loop:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret


jumps:
    xor rax, rax
.otherLoop:
   db  0x0f, 0x1f, 0x00 
.loop:
    inc       rax
    cmp byte  [rdx + rax], 0
    jnz       .otherLoop
    cmp       rax, rcx
    jb        .loop
    ret

align64:
    xor rax, rax
    align 64
.loop:
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

align1:
    xor rax, rax
    align 64
    nop
.loop:
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

align15:
    xor rax, rax
    align 64
    %rep 15
    nop
    %endrep
.loop:
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

align62:
    xor rax, rax
    align 64
    %rep 62
    nop
    %endrep
.loop:
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

align63:
    xor rax, rax
    align 64
    %rep 63
    nop
    %endrep
.loop:
    inc  rax  
    cmp  rax, rcx  
    jb   .loop
    ret

read1:
    align 64
.loop:
    mov rax, [rdx]
    sub rcx, 1
    jnle .loop
    ret

read2:
    align 64
.loop:
    mov rax, [rdx]
    mov rax, [rdx]
    sub rcx, 2
    jnle .loop
    ret

read3:
    align 64
.loop:
    mov rax, [rdx]
    mov rax, [rdx]
    mov rax, [rdx]
    sub rcx, 3
    jnle .loop
    ret


read4:
    align 64
.loop:
    mov rax, [rdx]
    mov rax, [rdx]
    mov rax, [rdx]
    mov rax, [rdx]
    sub rcx, 4
    jnle .loop
    ret

write1:
    align 64
.loop:
    mov [rdx], rax
    sub rcx, 1
    jnle .loop
    ret

write2:
    align 64
.loop:
    mov [rdx], rax
    mov [rdx], rax
    sub rcx, 2
    jnle .loop
    ret

write3:
    align 64
.loop:
    mov [rdx], rax
    mov [rdx], rax
    mov [rdx], rax
    sub rcx, 3
    jnle .loop
    ret


write4:
    align 64
.loop:
    mov [rdx], rax
    mov [rdx], rax
    mov [rdx], rax
    mov [rdx], rax
    sub rcx, 4
    jnle .loop
    ret


read2x4:
    align 64
.loop:
    mov eax, [rdx]
    mov eax, [rdx]
    sub rcx, 8
    jnle .loop
    ret

read2x8:
    align 64
.loop:
    mov rax, [rdx]
    mov rax, [rdx]
    sub rcx, 16
    jnle .loop
    ret

read2x16:
    align 64
.loop:
    vmovdqu xmm0, [rdx]
    vmovdqu xmm0, [rdx]
    sub rcx, 32
    jnle .loop
    ret

read1x32:
    align 64
.loop:
    vmovdqu ymm0, [rdx]
    sub rcx, 32
    jnle .loop
    ret

read2x32:
    align 64
.loop:
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx]
    sub rcx, 64
    jnle .loop
    ret

read2x64:
    align 64
.loop:
    vmovdqu64 zmm0, [rdx]
    vmovdqu64 zmm0, [rdx]
    sub rcx, 128
    jnle .loop
    ret

read1x64:
    align 64
.loop:
    vmovdqu64 zmm0, [rdx]
    sub rcx, 64
    jnle .loop
    ret

avx512_zmm:
   align 64
   vmovdqu64 zmm0, [rcx]

avx512_ymm:
   vmovdqu64 ymm0, [rcx]

avx512_xmm:
   vmovdqu64 xmm0, [rcx]

testCache:    
    align 64
    xor rax, rax
.loop:  
    vmovdqu ymm0, [rdx + rax]
    vmovdqu ymm0, [rdx + rax + 32]
    vmovdqu ymm0, [rdx + rax + 64]
    vmovdqu ymm0, [rdx + rax + 96]
    vmovdqu ymm0, [rdx + rax + 128]
    vmovdqu ymm0, [rdx + rax + 160]
    vmovdqu ymm0, [rdx + rax + 192]
    vmovdqu ymm0, [rdx + rax + 224]
    sub rcx, 256
    add rax, 256
    cmp rax, r8
    jb .loop
    xor rax, rax
    cmp rcx, rax
    jg .loop
    ret

testCacheAnd:    
    align 64
    xor rax, rax
.loop:  
    vmovdqu ymm0, [rdx + rax]
    vmovdqu ymm0, [rdx + rax + 32]
    vmovdqu ymm0, [rdx + rax + 64]
    vmovdqu ymm0, [rdx + rax + 96]
    vmovdqu ymm0, [rdx + rax + 128]
    vmovdqu ymm0, [rdx + rax + 160]
    vmovdqu ymm0, [rdx + rax + 192]
    vmovdqu ymm0, [rdx + rax + 224]
    sub rcx, 256
    add rax, 256
    and rax, r8
    cmp rcx, 0
    jg .loop
    ret



testCacheUnaligned:    
    align 64
    mov rax, r9
.loop:  
    vmovdqu ymm0, [rdx + rax]
    vmovdqu ymm0, [rdx + rax + 32]
    vmovdqu ymm0, [rdx + rax + 64]
    vmovdqu ymm0, [rdx + rax + 96]
    vmovdqu ymm0, [rdx + rax + 128]
    vmovdqu ymm0, [rdx + rax + 160]
    vmovdqu ymm0, [rdx + rax + 192]
    vmovdqu ymm0, [rdx + rax + 224]
    sub rcx, 256
    add rax, 256
    cmp rax, r8
    jb .loop
    mov rax, r9
    cmp rcx, rax
    jg .loop
    ret


testCacheUnalignedNonContiguous:    
    align 64
    mov rax, r9
.loop:  

    vmovdqu ymm0, [rdx + rax]
    vmovdqu ymm0, [rdx + rax + 64]
    vmovdqu ymm0, [rdx + rax + 128]
    vmovdqu ymm0, [rdx + rax + 192]
    vmovdqu ymm0, [rdx + rax + 256]
    vmovdqu ymm0, [rdx + rax + 320]
    vmovdqu ymm0, [rdx + rax + 384]
    vmovdqu ymm0, [rdx + rax + 448]
    sub rcx, 256
    add rax, 512
    cmp rax, r8
    jb .loop
    mov rax, r9
    cmp rcx, rax
    jg .loop
    ret


cacheSetComparison:    
    align 64
    mov rax, 0
.loop:  
    vmovdqu ymm0, [rdx + rax ]
    vmovdqu ymm0, [rdx + rax + 0b0000_000001_000000]
    vmovdqu ymm0, [rdx + rax + 0b0000_000010_000000]
    vmovdqu ymm0, [rdx + rax + 0b0000_000011_000000]
    vmovdqu ymm0, [rdx + rax + 0b0000_000100_000000]
    vmovdqu ymm0, [rdx + rax + 0b0000_000101_000000]
    vmovdqu ymm0, [rdx + rax + 0b0000_000110_000000]
    vmovdqu ymm0, [rdx + rax + 0b0000_000111_000000]
    sub rcx, 256
    add rax, 0b0000_001000_000000
    and rax, 0b0000_001111_111111
    cmp rcx, 0
    jg .loop
    ret

testCacheSet:    
    align 64
    mov rax, 0
.loop:  
    vmovdqu ymm0, [rdx + rax]
    vmovdqu ymm0, [rdx + rax + 0b0001_000000_000000]
    vmovdqu ymm0, [rdx + rax + 0b0010_000000_000000]
    vmovdqu ymm0, [rdx + rax + 0b0011_000000_000000]
    vmovdqu ymm0, [rdx + rax + 0b0100_000000_000000]
    vmovdqu ymm0, [rdx + rax + 0b0101_000000_000000]
    vmovdqu ymm0, [rdx + rax + 0b0110_000000_000000]
    vmovdqu ymm0, [rdx + rax + 0b0111_000000_000000]
    sub rcx, 256
    add rax, 0b1000_000000_000000
    and rax, 0b1111_111111_111111
    cmp rcx, 0
    jg .loop
    ret

testTemporal:    
    align 64
    xor rax, rax
.loop:  
    vmovdqu ymm0, [rdx]
    vmovdqu [r8 + rax], ymm0
    
    vmovdqu ymm0, [rdx  + 32]
    vmovdqu [r8 + rax + 32], ymm0
    
    vmovdqu ymm0, [rdx  + 64]
    vmovdqu [r8 + rax + 64], ymm0
  
    vmovdqu ymm0, [rdx  + 96]
    vmovdqu [r8 + rax + 96], ymm0
 
    vmovdqu ymm0, [rdx  + 128]
    vmovdqu [r8 + rax + 128], ymm0
    
    vmovdqu ymm0, [rdx  + 160]
    vmovdqu [r8 + rax + 160], ymm0

    vmovdqu ymm0, [rdx  + 192]
    vmovdqu [r8 + rax + 192], ymm0

    vmovdqu ymm0, [rdx  + 224]
    vmovdqu [r8 + rax + 224], ymm0

    add rax, 256
    cmp rax, rcx
    jb .loop
    ret

testNonTemporal:    
    align 64
    xor rax, rax
.loop:  
    vmovdqu ymm0, [rdx]
    vmovntdq [r8 + rax], ymm0
    
    vmovdqu ymm0, [rdx  + 32]
    vmovntdq [r8 + rax + 32], ymm0
    
    vmovdqu ymm0, [rdx  + 64]
    vmovntdq [r8 + rax + 64], ymm0
  
    vmovdqu ymm0, [rdx  + 96]
    vmovntdq [r8 + rax + 96], ymm0
 
    vmovdqu ymm0, [rdx  + 128]
    vmovntdq [r8 + rax + 128], ymm0
    
    vmovdqu ymm0, [rdx  + 160]
    vmovntdq [r8 + rax + 160], ymm0

    vmovdqu ymm0, [rdx  + 192]
    vmovntdq [r8 + rax + 192], ymm0

    vmovdqu ymm0, [rdx  + 224]
    vmovntdq [r8 + rax + 224], ymm0

    add rax, 256
    cmp rax, rcx
    jb .loop
    ret