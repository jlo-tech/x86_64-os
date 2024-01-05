
bits 64

extern intr_handler

section .text

; TODO: Function for switching contexts and privilege levels (based on iretq)

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

; ====================== ;
; General purpose macros ;
; ====================== ;
%macro save_context 0

    ; NOTE: We currently don't save vector or floating point registers

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8

    push rsi
    push rdi

    push rbp

    push rdx
    push rcx
    push rbx
    push rax

%endmacro

%macro restore_context 0

    pop rax
    pop rbx
    pop rcx
    pop rdx

    pop rbp

    pop rdi
    pop rsi
    
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

%endmacro

; General handler which saves and restores the interrupt context
isr_stub:

    ; save context
    save_context
    
    ; call c handler
    mov rdi, rsp    ; pointer to saved vars
    call intr_handler
    mov rsp, rax    ; restore saved context

    ; restore context
    restore_context

    ; remove interrupt code (pushed by the ISRs) from stack
    add rsp, 8

    ; return from interrupt
    iretq