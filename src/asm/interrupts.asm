%include "src/asm/macros.asm"

bits 64

extern intr_handler

extern idle_task_func

global switch_context
global schedule_task
global schedule_kernel_task

section .text

; ============= ;
; Generate ISRs ;
; ============= ;

; Single ISR 
%macro isr_macro 1

global isr_%1

isr_%1:
    push qword %1
    jmp isr_stub

%endmacro

; All ISRs
%assign c 0
%rep 256

isr_macro c

%assign c c+1
%endrep

; General handler which saves and restores the interrupt context
isr_stub:
    ; save context
    save_context
    ; call c handler
    mov rdi, rsp          ; pointer to saved vars
    call intr_handler   
    hlt

; Does context/mode switch
; switch_context(struct interrupt_context *ctx)
switch_context:
    ; Load interrupt context
    mov rsp, rdi
    ; Do actual switch
    iretq

; Run task on cpu
; schedule_task(struct tcb *task)
schedule_task:
    ; load page table
    mov rax, [rdi+176] ; add sizeof(tid) + sizeof(global_context) to load vmm_ctx
    mov cr3, rax

    ; load registers from cpu_ctx
    lea rsp, [rdi+8]
    restore_context

    ; remove info code
    add rsp, 8

    ; poping off registers leaves us int_ctx on stack

    ; do actual context switch
    iretq

; schedule_kernel_task(struct tcb *task)
schedule_kernel_task:
    
    ; --------------------------------------------------------
    ; IDEA: Copy everything needed to task's stack and then 
    ;       return (more or less) as before/above
    ; --------------------------------------------------------

    ; load page table
    mov rax, [rdi+176] ; add sizeof(tid) + sizeof(global_context) to load vmm_ctx
    mov cr3, rax

    add rdi, 8          ; skip tid of struct tcb
    mov rsi, [rdi+152]  ; load tasks rsp into rsi
    
    xchg rdi, rsi       ; rdi = tasks rsp, rsi = tcb->regintr_ctx
    sub rdi, 128       
    mov rcx, 15         ; copy 15 regs
    cld
    rep movsq           ; copy task state
    mov rcx, [rsi+8]    ; put rip on tasks stack
    mov [rdi], rcx      ; belongs to line above
    mov rsp, rdi        ; set stack
    sub rsp, 120        ; set tos correctly
    restore_context     ; pop registers

    ; Jumps to RIP of tcb
    sti                 ; enable interrupts
    retq
