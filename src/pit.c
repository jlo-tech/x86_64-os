#include <pit.h>

void pit_freq(u16 freq)
{
    outb(PIT_CMD, 0x8); // Set mode 4
    outb(PIT_CHN, (freq >> 0) & 0xff);
    outb(PIT_CHN, (freq >> 8) & 0xff);
}