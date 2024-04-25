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
        directory->entries[i] = curr_offset | 0b011;
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

// ******************************************************************
// * These functions are for general purpose (4k) paging operations *
// ******************************************************************

// Get level indeces
#define PAGE_L1(x) ((x >> 12) & 0x1FF)
#define PAGE_L2(x) ((x >> 21) & 0x1FF)
#define PAGE_L3(x) ((x >> 30) & 0x1FF)
#define PAGE_L4(x) ((x >> 39) & 0x1FF)

void paging_activate(struct page_table *table)
{
    // Activate page table
    __asm__ volatile("mov %0,%%cr3" : : "r" (table) : "memory");
}

// Allocate one page table and insert into parent
static void alloc_entry(struct page_table *table, u64 index, u16 flags)
{
    u64 addr = align(kmalloc(4096), 4096);
    bzero((u8*)addr, 4096);
    table->entries[index] = addr | flags;
}

void paging_map(struct page_table *p4, u64 virt_addr, u64 phys_addr, u16 flags)
{
    // Entry of p4
    u64 pe4 = p4->entries[PAGE_L4(virt_addr)];

    // Allocate new page directory
    if(!(pe4 & PAGE_PRESENT))
    {
        alloc_entry(p4, PAGE_L4(virt_addr), flags);
    }

    // P3 Table
    struct page_table *p3 = (struct page_table*)(p4->entries[PAGE_L4(virt_addr)] & (~4095));

    // P3 Entry
    u64 pe3 = p3->entries[PAGE_L3(virt_addr)];
    if(!(pe3 & PAGE_PRESENT))
    {
        alloc_entry(p3, PAGE_L3(virt_addr), flags);
    }

    // P2 Table
    struct page_table *p2 = (struct page_table*)(p3->entries[PAGE_L3(virt_addr)] & (~4095));

    // P2 Entry
    u64 pe2 = p2->entries[PAGE_L2(virt_addr)];
    if(!(pe2 & PAGE_PRESENT))
    {
        alloc_entry(p2, PAGE_L2(virt_addr), flags);
    }

    // P1 Table
    struct page_table *p1 = (struct page_table*)(p2->entries[PAGE_L2(virt_addr)] & (~4095));

    // Write phys addr into P1 Table
    p1->entries[PAGE_L1(virt_addr)] = phys_addr | flags;
}

void paging_map_range(struct page_table *p4, u64 virt_addr, u64 phys_addr, u16 flags, u64 size, u64 page_size)
{
    for(u64 i = 0; i < size / page_size; i++)
    {
        paging_map(p4, virt_addr + i * page_size, phys_addr + i * page_size, flags);
    }
}

// Virtual to physical address
u64 paging_walk(struct page_table *p4, u64 virt_addr)
{
    // Entry of p4
    u64 pe4 = p4->entries[PAGE_L4(virt_addr)];

    // Allocate new page directory
    if(!(pe4 & PAGE_PRESENT))
    {
        // Error (not mapped)
        return -1;
    }

    // P3 Table
    struct page_table *p3 = (struct page_table*)(pe4 & (~4095));

    // P3 Entry
    u64 pe3 = p3->entries[PAGE_L3(virt_addr)];
    if(!(pe3 & PAGE_PRESENT))
    {
        // Error (not mapped)
        return -1;
    }

    // P2 Table
    struct page_table *p2 = (struct page_table*)(pe3 & (~4095));

    // P2 Entry
    u64 pe2 = p2->entries[PAGE_L2(virt_addr)];
    if(!(pe2 & PAGE_PRESENT))
    {
        // Error (not mapped)
        return -1;
    }

    // P1 Table
    struct page_table *p1 = (struct page_table*)(pe2 & (~4095));

    // P1 Entry
    u64 pe1 = p1->entries[PAGE_L1(virt_addr)];

    if(!(pe1 & PAGE_PRESENT))
    {
        // Error (not mapped)
        return -1;
    }

    // Get physical address (without flags)
    return (pe1 & (~4095)) | (virt_addr & 4095);
}