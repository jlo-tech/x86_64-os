#pragma once

#include <types.h>

void outb(u32 port, u8 data);
void outw(u32 port, u16 data);
void outd(u32 port, u32 data);

u8 inb(u32 port);
u16 inw(u32 port);
u32 ind(u32 port);