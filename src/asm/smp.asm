global smp_trampoline_begin
global smp_trampoline_end

section .text

bits 16

smp_trampoline_begin:

    ; TODO Switch to protected mode and 
    ; print some message or change some var

    ; disable interrupts
    cli

    ; load gdt 
    lgdt [.gdtp]

    ; switch to protected mode
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; jump to long mode entry point
    jmp 0x8:.pm

; -------------
; GDT
; -------------

align 16

.gdt:

.null_descriptor: dq 0

.code_segment: 
    dw 0xFFFF ; limit 
    dw 0x0    ; base
    db 0x0    ; base
    db 0x9A   ; present + execute/read
    db 0x4F   ; lots of stuff
    db 0x0    ; base

.data_segment:
    dw 0xFFFF ; limit
    dw 0x0    ; base
    db 0x0    ; base
    db 0x92   ; present + erite/read
    db 0x4F   ; lots of stuff
    db 0x0    ; base

.limit:

.gdtp:
    dw 0x17 ; size of gdt - 1
    dd .gdt  ; pointer to gdt


bits 32

.pm:
    ; load 
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; print something 

    mov edi, 0xb8000
    mov byte [edi], '!'
    mov byte [edi+1], 0xC
    mov edi, 0xb8000
    mov byte [edi+2], '!'
    mov byte [edi+3], 0xC
    mov edi, 0xb8000
    mov byte [edi+4], '!'
    mov byte [edi+5], 0xC

    ; TODO: long mode

    hlt

smp_trampoline_end:
