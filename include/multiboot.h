#pragma once

// Multiboot2 information struct
struct multiboot_information
{
    u32 type;
    u32 size;
} __attribute__((packed));

struct multiboot_memory_map
{
    u32 type;
    u32 size;
    u32 entry_size;
    u32 entry_version;
} __attribute__((packed));

struct multiboot_memory_map_entry
{
    u64 address;
    u64 length;
    u32 type;
    u32 reserved;
} __attribute__((packed));

struct multiboot_information* multiboot_next_tag(struct multiboot_information *mb_info)
{
    u32 mask = ~0b111;
    
    u32 p = (u32)mb_info;
    p = p + mb_info->size;

    if((p & mask) != 0)
    {
        p = (p & mask) + 8;
    }

    return (struct multiboot_information*)p;
}

struct multiboot_memory_map* multiboot_memmap(struct multiboot_information* mb_info)
{
    while(mb_info->type != 6)
    {
        mb_info = multiboot_next_tag(mb_info);
    }

    return (struct multiboot_memory_map*)mb_info;
}

u64 multiboot_memmap_num_entries(struct multiboot_memory_map* memmap)
{
    return (memmap->size - 16) / memmap->entry_size;
}

struct multiboot_memory_map_entry* multiboot_memmap_entry(struct multiboot_memory_map *memmap, u64 index)
{
    u64 num_entries = multiboot_memmap_num_entries(memmap);
    index = index % num_entries;
    return memmap + 16 + index * memmap->entry_size;
}