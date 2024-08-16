#pragma once

#include <types.h>

// MP Floating Pointer Structure
struct mp_fps
{
    char signature[4];
    u32  phys_addr_ptr;
    u8   length;
    u8   spec_rev;
    u8   checksum;
    u8   feature1;
    u32  feature2to5;
} __attribute__((packed));

struct mp_ct_hdr
{
    char signature[4];
    u16 base_table_length;
    u8 spec_rev;
    u8 checksum;
    char oem_id[8];
    char prod_id[12];
    u32 oem_table_pointer;
    u16 oem_table_size;
    u16 entry_count;
    u32 local_apic_mm_addr;
    u16 ex_table_length;
    u8 ex_table_checksum;
    u8 reserved;
} __attribute__((packed));

// Search for floating pointer structure 
struct mp_fps* mp_search_fps();

// Check for configuration table 
struct mp_ct_hdr* mp_check_ct(struct mp_fps *fps);
