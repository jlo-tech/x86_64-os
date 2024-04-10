#include <syscalls.h>

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

static void syscall_handler();

void syscalls_setup()
{
  
    // TODO: Enable syscalls in IA32_EFER MSR


    // Disable interrupts but leave rest as it is
    //wmsr(MSR_IA32_FMASK, 1 << 9);

    // Set handler address
    //wmsr(MSR_IA32_LSTAR, (u64)syscall_handler);
}

static void syscall_handler()
{
    // TODO: Create and load stack and preserve rcx

    // TODO: Implement syscalls

    //__asm__ volatile("sysret");
}