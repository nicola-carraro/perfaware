
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