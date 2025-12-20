%include "src/asm/macros.asm"

bits 64

extern intr_handler

global switch_context
global schedule_task

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
    mov rsp, rax          ; restore saved context

    ; restore context
    restore_context

    ; remove interrupt code (pushed by the ISRs) from stack
    add rsp, 8

    ; return from interrupt
    iretq

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
    mov rax, [rdi+168] ; add sizeof(tid) + sizeof(cpu_ctx) + sizeof(int_ctx) to load vmm_ctx
    mov cr3, rax

    ; load registers from cpu_ctx
    lea rsp, [rdi+8]
    restore_context
    
    ; poping off registers leaves us int_ctx on stack

    ; do actual context switch
    iretq