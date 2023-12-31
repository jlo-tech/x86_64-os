extern kmain

global _start

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
    ; using page size extension allows to map 2MiB pages

    ; map first level
    mov eax, page_directories
    xor ebx, ebx
.page_first_level_loop:
    add eax, 0b11
    mov [page_directory_pointer + ebx * 8], eax
    sub eax, 0b11
    inc ebx
    add eax, 4096
    ; check if all page directories were mapped
    cmp ebx, 512
    jne .page_first_level_loop

    ; map second level
    mov eax, page_tables
    xor ebx, ebx
    xor ecx, ecx
    mov edx, page_directories
.page_second_level_loop:
    add eax, 0b11
    mov [edx + ebx * 8], eax
    sub eax, 0b11
    inc ebx ; go to next entry
    add eax, 4096 ; go to next page table
    ; check if all page tables were mapped for that directory
    cmp ebx, 512
    jne .page_second_level_loop
    ; move on to new page directory
    mov ebx, 0
    inc ecx
    add edx, 4096 ; go to next page directory
    ; and check if we are finished
    cmp ecx, 512
    jne .page_second_level_loop

    ; map third level directly to pages cause of PSE (2MiB pages)
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    mov edx, page_tables
.page_third_level_loop:
    add eax, 0b10000011
    mov [edx + ebx * 8], eax
    sub eax, 0b10000011
    add eax, 0x200000
    inc ebx
    ; check if all page entries were mapped for that table
    cmp ebx, 512
    jne .page_third_level_loop
    ; move on to new page directory
    mov ebx, 0
    inc ecx
    add edx, 4096 ; go to next page directory
    ; and check if we are finished
    cmp ecx, 512 * 512
    jne .page_third_level_loop

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

    ; pass multiboot2 information structure to rust
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
    resb 4096 * 1
page_directories:
    resb 4096 * 512
page_tables:
    resb 4096 * 512 * 512