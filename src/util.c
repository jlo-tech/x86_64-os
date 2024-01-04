#include <util.h>

void bzero(u8 *mem, u64 size)
{
    for(u64 i = 0; i < size; i++)
    {
        mem[i] = 0;
    }
}