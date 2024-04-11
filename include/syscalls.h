#pragma once

#include <types.h>

#define MSR_IA32_EFER  0xC0000080
#define MSR_IA32_STAR  0xC0000081
#define MSR_IA32_LSTAR 0xC0000082
#define MSR_IA32_CSTAR 0xC0000083
#define MSR_IA32_FMASK 0xC0000084

u64 rmsr(u32 msr);
void wmsr(u32 msr, u64 val);

/* Setup fast syscalls */
void syscalls_setup();