#pragma once

#include <types.h>
#include <util.h>

#define PAGE_SIZE (u64)(1 << 12)
#define PAGE_MASK (u64)(PAGE_SIZE - 1)   // To check for correct alignment

/* Helpers */
u64 align(u64 addr, u64 alignment);

/* We will use a modified version of the buddy allocator 
   combined with trees to make memory allocation as fast as possible */

/* Allocator management structure */
struct kheap
{
   struct ktree used_buddies;     // Sortex by addr
   struct ktree free_buddies[48]; // Sorted by size
};

/* Structure sitting at top of each chunk */
struct kchunk
{
   i64 addr;
   i64 size; // In number of pages
   struct ktree_node tree_handle;
} __attribute__((packed));

/* Note: It is assumed that the effective part of an addresses is < 64bit */

void kheap_init(struct kheap *heap);
i64 kheap_alloc(struct kheap *heap, i64 size);
i64 kheap_free(struct kheap *heap, i64 addr);

i64 kmalloc(i64 size);
i64 kfree(i64 addr);