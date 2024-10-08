extern kmain

global _start

global global_descriptor_table
global global_descriptor_table_pointer
global page_id_ptr
global page_id_dir
global page_id_tab
global kernel_stack

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
    mov eax, page_id_dir
    or eax, 0b11
    mov [page_id_ptr], eax

    ; map second level
    mov eax, page_id_tab
    or eax, 0b11
    mov [page_id_dir], eax

    mov eax, page_id_tab
    add eax, 0x1000
    or eax, 0b11
    mov [page_id_dir+8], eax

    mov eax, page_id_tab
    add eax, 0x2000
    or eax, 0b11
    mov [page_id_dir+16], eax

    mov eax, page_id_tab
    add eax, 0x3000
    or eax, 0b11
    mov [page_id_dir+24], eax

    ; map first 4GiB of virtual space
    mov edi, page_id_tab
    mov esi, 0
    call fill_initial_pt

    mov edi, page_id_tab + 0x1000
    mov esi, 0x40000000
    call fill_initial_pt

    mov edi, page_id_tab + 0x2000
    mov esi, 0x80000000
    call fill_initial_pt

    mov edi, page_id_tab + 0x3000
    mov esi, 0xC0000000
    call fill_initial_pt

    ; finally enable paging
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
    jmp 0x08:_kernel

bits 64
_kernel:

    ; reload data segment
    mov ax, 0x10
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

section .data

; Code/Data-Entries: type | system vs. code/data segment | dpl | present | segment contains 64bit code

global_descriptor_table:
.table:
    dq 0
.kernel_code:
    dq (8<<40) | (1<<44) | (0<<45) | (1<<47) | (1<<53) ; kernel code
.kernel_data:
    dq (2<<40) | (1<<44) | (0<<45) | (1<<47) | (0<<53) ; kernel data
.user_data:
    dq (2<<40) | (1<<44) | (3<<45) | (1<<47) | (0<<53) ; user data
.user_code:
    dq (8<<40) | (1<<44) | (3<<45) | (1<<47) | (1<<53) ; user code
.tss:
    dq 0 ; initialized by software when in long mode
    dq 0 ; TSS descriptor is 16 bytes in long mode

global_descriptor_table_pointer:
    dw $ - global_descriptor_table - 1
    dq global_descriptor_table

section .bss

; stack space
align 4096
resb (16384 * 2)
kernel_stack:

; identity mapping for whole virtual address space
page_id_ptr:
    resb 4096
page_id_dir:
    resb 4096 * 512
page_id_tab:
    resb 4096 * 512 * 512
