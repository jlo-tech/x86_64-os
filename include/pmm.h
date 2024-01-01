#pragma once

#include <types.h>

#define PAGE_SIZE (u64)(1 << 12)
#define PAGE_MASK (u64)(PAGE_SIZE - 1)   // To check for correct alignment

struct node
{
    u64 size;
    struct node *next;
};