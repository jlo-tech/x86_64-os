extern kmain

global _start

global id_ptr
global id_dir
global id_tab

section .multiboot_header

header_start:
    
    dd 0xe85250d6                ; magic number
    dd 0                         ; architecture
    dd header_end - header_start ; header length
    
    ; checksum
    dd 0x100000000 - (0xe85250d6 + (header_end - header_start))

    ; (currently not needed) optional tags

    ; final tag
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size

header_end:

; 32
extern mem_zero
extern fill_initial_pt

; 64
extern fill_page_directory
extern fill_page_table

section .text

bits 32
_start:

    ; check that os was loaded by multiboot2 bootloader
    cmp eax, 0x36d76289
    je .continue_execution

    ; else hlt cpu
    hlt

.continue_execution:

    ; setup stack
    mov esp, kernel_stack

    ; save multiboot structure
    push 0
    push ebx

    ; setup paging (identity mapping)
    
    ; map first level
    mov eax, page_directory
    or eax, 0b11
    mov [page_directory_pointer], eax

    ; map second level
    mov eax, page_table
    or eax, 0b11
    mov [page_directory], eax

    mov eax, page_table
    add eax, 0x1000
    or eax, 0b11
    mov [page_directory+8], eax

    mov eax, page_table
    add eax, 0x2000
    or eax, 0b11
    mov [page_directory+16], eax

    mov eax, page_table
    add eax, 0x3000
    or eax, 0b11
    mov [page_directory+24], eax

    ; map first 4GiB of virtual space
    mov edi, page_table
    mov esi, 0
    call fill_initial_pt

    mov edi, page_table + 0x1000
    mov esi, 0x40000000
    call fill_initial_pt

    mov edi, page_table + 0x2000
    mov esi, 0x80000000
    call fill_initial_pt

    mov edi, page_table + 0x3000
    mov esi, 0xC0000000
    call fill_initial_pt

    ; finally enable paging
    mov eax, page_directory_pointer
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
    lgdt [gdt_long_mode.pointer]

    ; reload code segment
    jmp 0x08:_kernel

bits 64
_kernel:

    ; reload data segment (with null selector)
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; pass multiboot2 information structure to kernel
    pop rdi

    call kmain

    ; stop cpu
    hlt

section .rodata

gdt_long_mode:
.table:
    dq 0
.kernel_code:
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) ; kernel code
.kernel_data:
    dq (0<<43) | (1<<44) | (1<<47) | (1<<53) ; kernel data
.user_code:
    dq (1<<43) | (1<<44) | (1<<45) | (1<<46) | (1<<47) | (1<<53) ; user code
.user_data:
    dq (0<<43) | (1<<44) | (1<<45) | (1<<46) | (1<<47) | (1<<53) ; user data
.pointer:
    dw $ - gdt_long_mode - 1
    dq gdt_long_mode

section .bss

; stack space
align 4096
resb 4096
kernel_stack:

; identity mapping for whole virtual address space
page_directory_pointer:
    resb 4096
page_directory:
    resb 4096
page_table:
    resb 4096 * 4

id_ptr:
    resb 4096
id_dir:
    resb 4096 * 512
id_tab:
    resb 4096 * 512 * 512