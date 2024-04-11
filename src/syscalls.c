#include <syscalls.h>

extern void syscall_handler();

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

void syscalls_setup()
{
    // Disable interrupts but leave rest as it is
    wmsr(MSR_IA32_FMASK, 1 << 9);

    // Set handler address
    wmsr(MSR_IA32_LSTAR, (u64)syscall_handler);

    // Set segment selectors in STAR register
    // NOTE: The manual specifies that certains offsets are added to selector values
    // therefore they do not exactly correspond to gdt entries
    u64 cal_sel = (u64)((1 << 3) | 0) << 32;
    u64 ret_sel = (u64)((2 << 3) | 3) << 48;
    wmsr(MSR_IA32_STAR, ret_sel | cal_sel | (rmsr(MSR_IA32_STAR) & 0xFFFFFFFF));

    // Enable syscalls in IA32_EFER MSR
    wmsr(MSR_IA32_EFER, rmsr(MSR_IA32_EFER) | 1);
}