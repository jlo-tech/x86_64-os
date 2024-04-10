#pragma once

#include <types.h>

#define MSR_IA32_LSTAR 0xC0000082
#define MSR_IA32_FMASK 0xC0000084

u64 rmsr(u32 msr);
void wmsr(u32 msr, u64 val);

/* Setup fast syscalls */
void syscalls_setup();