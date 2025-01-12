global tss_load
global dummy_task

section .text

bits 64

tss_load:
    xor rax, rax
    mov ax, 0x2b
    ltr ax
    ret
