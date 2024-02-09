#include <vmm.h>

// ***********************************************************
// * These functions are used to create the identity mapping *
// ***********************************************************

extern struct page_table page_id_ptr;
extern struct page_table page_id_dir[512];
extern struct page_table page_id_tab[512 * 512];

void paging_id_fill_table(struct page_table *table, u64 frame_offset)
{
    u64 curr_offset = frame_offset;
    for(int i = 0; i < 512; i++)
    {
        table->entries[i] = curr_offset | 0b10000011; // 2MiB pages
        curr_offset += 0x200000;
    }
}

void paging_id_fill_directory(struct page_table *directory, u64 frame_offset)
{
    u64 curr_offset = frame_offset;
    for(int i = 0; i < 512; i++)
    {
        directory->entries[i] = curr_offset | 0b11;
        curr_offset += sizeof(struct page_table);
    }
}

// Full mapping of virtual address space
void paging_id_full()
{
    paging_id_fill_directory(&page_id_ptr, (u64)&page_id_dir);

    for(u64 i = 0; i < 512; i++)
    {
        paging_id_fill_directory(&page_id_dir[i], (u64)&page_id_tab[i * 512]);
    }

    for(u64 i = 0; i < 512 * 512; i++)
    {
        paging_id_fill_table(&page_id_tab[i], i * 0x40000000);
    }
}

// *************************************************************
// * These functions are for general purpose paging operations *
// *************************************************************

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

void paging_activate(struct page_table *table)
{
    // Activate page table
    __asm__ volatile("mov %0,%%cr3" : : "r" (table) : "memory");
}