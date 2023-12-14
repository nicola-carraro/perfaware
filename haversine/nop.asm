
global movAllBytes

global nopAllBytes

global cmpAllBytes

global decAllBytes

global decSlow

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