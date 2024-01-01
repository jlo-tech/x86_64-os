#include <vmm.h>

// These functions are used to create the identity mapping

extern struct page_table id_ptr;
extern struct page_table id_dir[512];
extern struct page_table id_tab[512 * 512];

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
    paging_id_fill_directory(&id_ptr, &id_dir);

    for(u64 i = 0; i < 512; i++)
    {
        paging_id_fill_directory(&id_dir[i], &id_tab[i * 512]);
    }

    for(u64 i = 0; i < 512 * 512; i++)
    {
        paging_id_fill_table(&id_tab[i], i * 0x40000000);
    }

    __asm__ volatile("mov %0,%%cr3" : : "r" (&id_ptr) : "memory");
}