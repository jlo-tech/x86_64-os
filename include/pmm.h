#pragma once

#include <types.h>

#define PAGE_SIZE (u64)(1 << 12)
#define PAGE_MASK (u64)(PAGE_SIZE - 1)   // To check for correct alignment

/* We will use a modified version of the buddy allocator 
   combined with trees to make memory allocation as fast as possible */


/* Allocator management structure */
struct kheap_alloc
{
    
};

/* Structure sitting at top of each chunk */
struct kchunk
{
    
};