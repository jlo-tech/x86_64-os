#include <io.h>

void outb(u32 port, u8 data)
{
    __asm__ volatile("outb %b0, %w1" :: "a" (data), "Nd" (port));
}

void outw(u32 port, u16 data)
{
    __asm__ volatile("outw %w0, %w1" :: "a" (data), "Nd" (port));
}

void outd(u32 port, u32 data)
{
    __asm__ volatile("outl %0, %w1" :: "a" (data), "Nd" (port));
}

u8 inb(u32 port)
{
    u8 data;
    __asm__ volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}

u16 inw(u32 port)
{
    u16 data;
    __asm__ volatile("inw %w1, %w0" : "=a" (data) : "Nd" (port));
    return data;
}

u32 ind(u32 port)
{
    u32 data;
    __asm__ volatile("inl %w1, %0" : "=a" (data) : "Nd" (port));
    return data;
}