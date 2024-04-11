global syscall_handler

section .text

bits 64

syscall_handler:
    ; TODO: Real handler
    nop 
    o64 sysret