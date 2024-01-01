#include <pmm.h>

// Linker variables
extern char kernel_base;
extern char kernel_limit;

static const u64 kernel_base_addr  = (u64)&kernel_base;
static const u64 kernel_limit_addr = (u64)&kernel_limit;

static u64 align(u64 addr, u64 alignment)
{
    if((addr % alignment) == 0)
    {
        return addr;
    }
    else
    {
        return (addr & ~(alignment - 1)) + alignment;
    }
}