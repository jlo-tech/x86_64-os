section .text

global mem_zero
global fill_initial_pt

; This code work with 2MiB pages!!!
; Using the page size extension allows us to map 2MiB pages

bits 32

; writes 0 to a certain area
; mem_zero(int base, int size)
mem_zero:
    add esi, edi        ; end address
.loop:
    mov byte [edi], 0
    inc edi
    cmp edi, esi
    jne .loop
    ret

; fills a whole page table with frame numbers starting at page_frame_offset
; fill_page_table(int pt_base, int offset)
fill_initial_pt:
    xor eax, eax
    or esi, 0b10000011         ; 2MiB pages
.loop:
    mov [edi + eax * 8], esi
    add esi, 0x200000           ; 2MiB
    inc eax
    cmp eax, 512
    jne .loop
    ret