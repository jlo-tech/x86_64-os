#include <apic.h>

#include <vga.h>

static bool mp_fps_valid(struct mp_fps *fps)
{
    // Check signature
    if(!(fps->signature[0] == '_' && 
         fps->signature[1] == 'M' && 
         fps->signature[2] == 'P' &&
         fps->signature[3] == '_'))
    {
        return false; 
    }

    // Check length
    if(fps->length != 1)
        return false;

    // Check checksum
    u8 sum = 0;
   
    for(i64 i = 0; i < 16; i++)
    {
        sum += ((u8*)fps)[i];
    } 
    
    if(sum != 0)
        return false;

    return true;
}

static struct mp_fps* mp_search_sig(u32 *space, i32 len)
{
   for(i32 i = 0; i < len; i++)
   {
       struct mp_fps *fps_ptr = (struct mp_fps*)(space + i);
       if(mp_fps_valid(fps_ptr))
           return fps_ptr;
   }
   // Not found
   return (void*)(-1);
}

struct mp_fps* mp_search_fps()
{
    struct mp_fps *ptr;
    
    // Search in first kilobyte of EBDA
    u16 *ebda_ptr  = (u16*)0x40E;
    u32 *ebda_addr = (u32*)(*ebda_ptr);
    ptr = mp_search_sig(ebda_addr, 1024 >> 2);
    if(ptr != (struct mp_fps*)(-1))
        return ptr;

    // Search in last kilobyte of system base memory
    u32 *sys_mem_base_addr = (u32*)(639 * 1024);
    ptr = mp_search_sig(sys_mem_base_addr, 1024 >> 2);
    if(ptr != (struct mp_fps*)(-1))
        return ptr;

    // Search in the bios rom
    u32 *bios_rom_addr = (u32*)(0xF0000);
    ptr = mp_search_sig(bios_rom_addr, (0xFFFFF - 0xF0000) >> 2);
    if(ptr != (struct mp_fps*)(-1))
        return ptr;

    // Not found
    return (void*)(-1);
}

struct mp_ct_hdr* mp_check_ct(struct mp_fps *fps)
{
    if(fps->phys_addr_ptr == 0)
        return (struct mp_ct_hdr*)(-1);

    struct mp_ct_hdr *ptr = (struct mp_ct_hdr*)(u64)fps->phys_addr_ptr; 
    
    // Check signature
    if(!(ptr->signature[0] == 'P' &&
         ptr->signature[1] == 'C' && 
         ptr->signature[2] == 'M' &&
         ptr->signature[3] == 'P')) 
    {
        return (struct mp_ct_hdr*)(-1);
    }

    // Check checksum
    u8 sum = 0;

    for(i64 i = 0; i < (i64)ptr->base_table_length; i++)
    {
        sum += ((u8*)ptr)[i];
    } 
    
    if(sum != 0)
        return (struct mp_ct_hdr*)(-1);

    // Return address of configuration table
    return ptr;
}

// Returns size for a certain ct base entry
static u8 type_to_len(u8 type)
{
    switch(type)
    {
        case 0:
            return 20;
        case 1:
            return 8;
        case 2:
            return 8;
        case 3:
            return 8;
        case 4:
            return 8;

        default:
            return 0;
    }
}

/**
 * Returns pointers to all entries in the base ct 
 *
 * @param hdr Header of the configuration table
 * @param res Output array where pointers pointing to start of entries are placed
 * 
 */
void mp_ct_entries(struct mp_ct_hdr *hdr, void **res)
{
    u64 off = 0;
    u8 *base = (u8*)(((u64)hdr) + sizeof(struct mp_ct_hdr));

    for(u16 i = 0; i < hdr->entry_count; i++)
    {
        res[i] = (void*)(base + off);
        off += type_to_len(*(base + off));
    }
}

/**
 * Returns pointer to all entries in the extended ct
 *
 * @param hdr, Header of the ct
 * @param res Output of pointers to the entries
 *
 * @return False if no ex table else true 
 */
bool mp_ct_extended_entries(struct mp_ct_hdr *hdr, void **res)
{
    if(hdr->ex_table_length == 0)
        return false;

    u64 off = 0;
    u8 *base = (u8*)(((u64)hdr) + hdr->base_table_length);

    i64 i = 0;
    while(off < hdr->ex_table_length)
    {
        res[i] = (void*)(base + off);
        off += *(base + off + 1);
        i++;
    }

    return true;
}

