#pragma once

#include <io.h>
#include <types.h>

#define PIT_CMD 0x43
#define PIT_CHN 0x40

void pit_freq(u16 freq);

void pit_handle_intr();

void pit_delay(u64 ticks);