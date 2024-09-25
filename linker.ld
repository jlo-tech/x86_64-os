ENTRY(_start)

SECTIONS 
{
    . = 0x8000;

    .smp :
    {
        src/asm/smp.o(.text)
    }

    . = 1M;

    kernel_base = .;

    .boot :
    {
        *(.multiboot_header)
    }

    .text : ALIGN(0x1000)
    {
        src/*.o(.text)
        src/fs/*.o(.text)
        src/asm/*.o(.text)
    }

    .rodata : ALIGN(0x1000)
    {
        *(.rodata)
    }

    .data : ALIGN(0x1000)
    {
        *(.data)
    }

    .bss : ALIGN(0x1000)
    {
        *(.bss)
    }

    kernel_limit = .;
}
