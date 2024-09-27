global atomic_add
global atomic_sub
global atomic_tas
global atomic_clr

section .text

bits 64

; atomic_add(i64 *atomic_var, i64 x)
atomic_add:
    lock add [rdi], rsi
    ret

; atomic_sub(i64 *atomic_var, i64 x)
atomic_sub:
    lock sub [rdi], rsi
    ret 

; atomic test and set 
; returns value of atomic_flag before it was set
; atomic_tas(bool *atomic_flag)
atomic_tas:
    xor rax, rax
    mov rsi, 1
    xchg [rdi], si
    mov ax, si 
    ret

; atomic clear
; atomic_clr(bool *atomic_flag)
atomic_clr:
    mov byte [rdi], 0
    ret
