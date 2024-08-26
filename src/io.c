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

u64 rmsr(u32 msr)
{
    u32 lo, hi;
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((u64)hi << 32) | lo;
}
 
void wmsr(u32 msr, u64 val)
{
    u32 lo = val & 0xFFFFFFFF;
    u32 hi = (val >> 32) & 0xFFFFFFFF;
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void mmio_writeb(size_t addr, u8  val)
{
    volatile u8 *ptr = (u8*)addr;
    *ptr = val;
}

void mmio_writew(size_t addr, u16 val)
{
    volatile u16 *ptr = (u16*)addr;
    *ptr = val;
}

void mmio_writed(size_t addr, u32 val)
{
    volatile u32 *ptr = (u32*)addr;
    *ptr = val;
}

void mmio_writeq(size_t addr, u64 val)
{
    volatile u64 *ptr = (u64*)addr;
    *ptr = val;
}

u8  mmio_readb(size_t addr)
{
    volatile u8 *ptr = (u8*)addr;
    return *ptr; 
}

u16 mmio_readw(size_t addr)
{
    volatile u16 *ptr = (u16*)addr;
    return *ptr;
}

u32 mmio_readd(size_t addr)
{
    volatile u32 *ptr = (u32*)addr;
    return *ptr;
}

u64 mmio_readq(size_t addr)
{
    volatile u64 *ptr = (u64*)addr;
    return *ptr;
}

