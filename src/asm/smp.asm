global smp_trampoline_begin
global smp_trampoline_end

extern page_id_ptr
extern global_descriptor_table_pointer

extern smp_boot_params
extern smp_ap_boot

section .text

bits 16

smp_trampoline_begin:

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
    ; load segment registers
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; switch to long mode

    ; enable paging (identity mapping)
    mov eax, page_id_ptr
    mov cr3, eax 

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax 

    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; load gdt
    lgdt [global_descriptor_table_pointer]

    ; reload code segment
    jmp 0x08:.lm


bits 64

.lm:

    ; reload data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; setup stack
    mov rsp, [smp_boot_params+0]

    ; let BSP know that we came this far
    mov qword [smp_boot_params+8], 1

    cld
    call smp_ap_boot

    hlt

smp_trampoline_end:
