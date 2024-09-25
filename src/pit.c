#include <pit.h>
#include <vga.h>

extern u64 pit_counter;

void pit_freq(u16 freq)
{      
    outb(PIT_CMD, 2 << 1 | 3 << 4);
    outb(PIT_CHN, freq >> 0);
    outb(PIT_CHN, freq >> 8);
}

void pit_handle_intr()
{
    pit_counter++;
}
