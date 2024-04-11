#pragma once

#include <util.h>
#include <types.h>

struct task_state_segment
{
    u32 reserved_0;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved_1;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved_2;
    u16 reserved_3;
    u16 iopb;
} __attribute__((packed));

struct tss_descriptor
{
    u16 limit;
    u16 base_low;
    u8  base_mid;
    u16 flags;
    u8  base_high;
    u32 base_upper;
    u32 reserved;
} __attribute__((packed));

void tss_init();