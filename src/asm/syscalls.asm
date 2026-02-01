%include "src/asm/macros.asm"

global syscall_handler
global return_to_task_from_syscall

extern do_syscall
extern schedule_task

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

; This function returns control back to another task then the one who made the syscall
; NOTE: Requires that scheduler_update_current_cpu_context() was called before
; void return_to_task_from_syscall(struct tcb*)
return_to_task_from_syscall:
    ; reset gs register
    swapgs
    ; run actual task
    jmp schedule_task

; ------------------------------------

; for testing purpose only

global user_func0
global user_func1

user_func0:
    mov rdi, 0
    mov rsi, ttp0
    syscall
.loop:
    jmp .loop

user_func1:
    mov rdi, 1
    mov rsi, ttp1
    mov rdx, 2
    syscall
    mov rdi, 0
    mov rsi, ttp1
    syscall
.loop:
    jmp .loop


section .data
ttp0: db "Hello, from Task 0!", 0
ttp1: db "Hello, from Task 1!", 0