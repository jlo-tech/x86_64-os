global tss_load

section .text

bits 64

tss_load:
    xor rax, rax
    mov ax, 0x2b
    ltr ax
    ret