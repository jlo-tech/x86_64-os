global tss_load
global idle_task_func

section .text

bits 64

tss_load:
    xor rax, rax
    mov ax, di
    ltr ax
    ret

; This is the idle task (needed by the scheduler)
idle_task_func:
    ;hlt
    jmp idle_task_func