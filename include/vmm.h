#pragma once

#include <types.h>

// This struct is used as table, directory and pointer.
struct page_table
{
    u64 entries[512];
} __attribute__((packed));