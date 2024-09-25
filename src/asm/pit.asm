global pit_delay
global pit_counter

section .text

bits 64

; void pit_delay(u64 ticks)
pit_delay:

    add rdi, [pit_counter] ; timestamp + ticks

.pit_delay_loop:    
    mov rsi, [pit_counter] ; timestamp
    cmp rdi, rsi
    ja .pit_delay_loop

    ret

section .data

pit_counter: dq 0