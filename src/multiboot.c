#include <multiboot.h>

static struct multiboot_information* multiboot_next_tag(struct multiboot_information *mb_info)
{
    u32 mask = ~0b111;
    
    u32 p = (u32)mb_info;
    p = p + mb_info->size;

    if((p & mask) != 0)
    {
        p = (p & mask) + 8;
    }

    return (struct multiboot_information*)(u64)p;
}

struct multiboot_memory_map* multiboot_memmap(struct multiboot_information* mb_info)
{
    while(mb_info->type != 6)
    {
        mb_info = multiboot_next_tag(mb_info);
    }

    return (struct multiboot_memory_map*)mb_info;
}

int multiboot_memmap_num_entries(struct multiboot_memory_map* memmap)
{
    return (int)((memmap->size - 16) / memmap->entry_size);
}

struct multiboot_memory_map_entry* multiboot_memmap_entry(struct multiboot_memory_map *memmap, u64 index)
{
    u64 num_entries = multiboot_memmap_num_entries(memmap);
    index = index % num_entries;
    return (struct multiboot_memory_map_entry*)(memmap + 16 + index * memmap->entry_size);
}