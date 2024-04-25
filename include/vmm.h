#pragma once

#include <pmm.h>
#include <types.h>

#define PAGE_PRESENT    (1 << 0)     // Page is in memory/valid
#define PAGE_WRITABLE   (1 << 1)     // Page is writable
#define PAGE_USER       (1 << 2)     // Page accessible by user
#define PAGE_WTCACHE    (1 << 3)     // Write through cache
#define PAGE_DCACHING   (1 << 4)     // Disable caching
#define PAGE_ACCESSED   (1 << 5)     // Set by cpu when page was accessed
#define PAGE_DIRTY      (1 << 6)     // Set by cpu on write to this page
#define PAGE_HUGE       (1 << 7)     // Creates 1GiB page in level 3 and a 2MiB Page in level 2
#define PAGE_GLOBAL     (1 << 8)     // Page won't be flushed from caches on addr space switch, but PGE bit in CR4 must be set
#define PAGE_NO_EXEC    (1 << 63)    // Page isn't executable, NXE bit in EFER must be set

// This struct is used as table, directory and pointer.
struct page_table
{
    u64 entries[512];
} __attribute__((packed));

// Initial identity mapping
void paging_id_full();

// Load page table
void paging_activate(struct page_table *table);

// General Purpose functions
void paging_map(struct page_table *p4, u64 virt_addr, u64 phys_addr, u16 flags);
void paging_map_range(struct page_table *p4, u64 virt_addr, u64 phys_addr, u16 flags, u64 size, u64 page_size);
u64 paging_walk(struct page_table *p4, u64 virt_addr);