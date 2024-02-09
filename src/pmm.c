#include <pmm.h>

// Linker variables
extern char kernel_base;
extern char kernel_limit;

static const u64 kernel_base_addr  = (u64)&kernel_base;
static const u64 kernel_limit_addr = (u64)&kernel_limit;

/**
 * Align address to a specific alignment
 * If addresses are not aligned they are rounded up 
 */
static u64 align(u64 addr, u64 alignment)
{
    u64 masked = addr & (alignment - 1);
    return (addr == masked) ? addr : (masked + alignment);
}
