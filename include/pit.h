#pragma once

#include <types.h>

#define PIT_CMD 0x43
#define PIT_CHN 0x40

void pit_freq(u16 freq);