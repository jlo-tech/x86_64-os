%include "src/asm/macros.asm"

global syscall_handler

extern do_syscall

section .text

bits 64

syscall_handler:
    ; Swap gs with kernel stack
    swapgs
    ; save regs
    mov [gs:0x08], rsp ; stack pointer
    mov [gs:0x10], r11 ; flags
    mov [gs:0x18], rcx ; instruction pointer
    ; set stack pointer
    mov rsp, [gs:0x0]
    ; save cpu context
    save_context
    ; call c syscall handler
    mov rdi, rsp
    call do_syscall
    ; skip old rax value 
    add rsp, 8
    ; restore context but don't overwrite rax
    restore_context_sys
    ; restore regs
    mov rsp, [gs:0x08] ; stack pointer
    mov r11, [gs:0x10] ; flags
    mov rcx, [gs:0x18] ; instruction pointer
    ; Get back gs
    swapgs
    ; Return to user mode
    o64 sysret


; ------------------------------------

; for testing purpose only

global user_func

user_func:
    mov rdi, 0
    mov rsi, ttp
    syscall
.loop:
    jmp .loop


section .data
ttp: db "Hello, Kernel!", 0