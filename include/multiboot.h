#pragma once

#include <types.h>

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

struct multiboot_memory_map* multiboot_memmap(struct multiboot_information* mb_info);
u64 multiboot_memmap_num_entries(struct multiboot_memory_map* memmap);
struct multiboot_memory_map_entry* multiboot_memmap_entry(struct multiboot_memory_map *memmap, u64 index);
