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
    ; call c syscall handler
    call do_syscall
    ; restore regs
    mov rsp, [gs:0x08] ; stack pointer
    mov r11, [gs:0x10] ; flags
    mov rcx, [gs:0x18] ; instruction pointer
    ; Get back gs
    swapgs
    ; Return to user mode
    o64 sysret