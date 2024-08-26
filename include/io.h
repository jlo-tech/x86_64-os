#pragma once

#include <types.h>

void outb(u32 port, u8 data);
void outw(u32 port, u16 data);
void outd(u32 port, u32 data);

u8 inb(u32 port);
u16 inw(u32 port);
u32 ind(u32 port);

u64 rmsr(u32 msr);
void wmsr(u32 msr, u64 val);

void mmio_writeb(size_t addr, u8  val);
void mmio_writew(size_t addr, u16 val);
void mmio_writed(size_t addr, u32 val);
void mmio_writeq(size_t addr, u64 val);

u8  mmio_readb(size_t addr);
u16 mmio_readw(size_t addr);
u32 mmio_readd(size_t addr);
u64 mmio_readq(size_t addr);
