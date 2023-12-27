
global movAllBytes

global nopAllBytes

global cmpAllBytes

global decAllBytes

global decSlow

global nop3x1

global nop1x3

global nop9

global jumps

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