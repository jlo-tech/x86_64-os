#pragma once

#include <types.h>

#define MSR_IA32_EFER  0xC0000080
#define MSR_IA32_STAR  0xC0000081
#define MSR_IA32_LSTAR 0xC0000082
#define MSR_IA32_CSTAR 0xC0000083
#define MSR_IA32_FMASK 0xC0000084

#define MSR_IA32_KERNEL_GS_BASE 0xC0000102

/*
 * Struct used to refer to kernel stack on syscall 
 * from user mode and to save important user data
 */
struct kernel_root
{
    u64 kernel_stack;
    u64 user_stack;
    u64 user_flags;
    u64 user_rip;
} __attribute__((packed));

/* Setup fast syscalls */
void syscalls_setup();
