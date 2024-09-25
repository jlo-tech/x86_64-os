#include <io.h>
#include <pit.h>
#include <pmm.h>
#include <vga.h>
#include <apic.h>

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

/**
 *
 * Searches for the entry that describes to which IOAPIC and 
 * to which pin/irq the PIT is mapped 
 *
 */
struct mp_ct_io_interrupt_entry* mp_ct_find_pit(struct mp_ct_hdr *hdr)
{
    void **entries = (void**)kmalloc(hdr->entry_count * sizeof(void*)); 

    // Get all entries in the MP base table
    mp_ct_entries(hdr, entries);

    u8 bus_id = 0;
    size_t entry_id = 0;
    struct mp_ct_io_interrupt_entry *addr;

    // Search right bus entry
    for(int i = 0; i < hdr->entry_count; i++)
    {
        u8 *e_type = (u8*)entries[i];
        if(*e_type ==  1)
        {
            u8 *e_bus_id = (e_type+1);
            u8 *e_bus_type_string = e_type + 2;
            // Search for  bus
            if(memcmp(e_bus_type_string, (u8*)"ISA", 3) == true)
            {
                bus_id = *e_bus_id;
                break;  
            }
        }
    }

    // Search for right interrupt entry
    for(int i = 0; i < hdr->entry_count; i++)
    {
        u8 *e_type = (u8*)entries[i];
        if(*e_type == 3)
        {
            // Check for right intr type, bus and irq
            if(*(e_type+1) == 0 && *(e_type+4) == bus_id && *(e_type+5) == 0)
            {
                // kprintf("IOAPIC: %d, PIN: %d\n", *(e_type+6), *(e_type+7));
                entry_id = i;
                addr = (struct mp_ct_io_interrupt_entry*)entries[entry_id];
                break;
            }
        }
    }

    kfree((i64)entries);

    return addr;
}

struct mp_ct_io_apic_entry* mp_ct_find_ioapic(struct mp_ct_hdr *hdr)
{
    void **entries = (void**)kmalloc(hdr->entry_count * sizeof(void*)); 

    // Get all entries in the MP base table
    mp_ct_entries(hdr, entries);

    struct mp_ct_io_apic_entry *addr;

    // Search right bus entry
    for(int i = 0; i < hdr->entry_count; i++)
    {
        u8 *e_type = (u8*)entries[i];
        if(*e_type ==  2)
        {
            addr = (struct mp_ct_io_apic_entry*)entries[i];
            break;
        }
    }

    kfree((i64)entries);

    return addr;
}

/* Local APIC */

lapic_t lapic_init(u8 spurious_interrupt_vector, 
                   u8 lint0_interrupt_vector, 
                   u8 lint1_interrupt_vector,
                   u8 error_interrupt_vector)
{
    // APIC base address
    size_t addr = rmsr(IA32_APIC_BASE_MSR) & (0x7FFFFFL << 12);

    // Fill LVT
    mmio_writed(addr + LAPIC_LVT_LINT0, lint0_interrupt_vector);
    mmio_writed(addr + LAPIC_LVT_LINT1, lint1_interrupt_vector);
    mmio_writed(addr + LAPIC_LVT_ERROR, error_interrupt_vector); 

    // Enable all external interrupts 
    mmio_writed(addr + LAPIC_TPR, 0);

    // Set spurious inter vector and enable APIC
    u32 sivr_val = (1 << 8) | spurious_interrupt_vector;
    mmio_writed(addr + LAPIC_SIVR, sivr_val); 

    return addr;
}

lapic_t lapic_fetch()
{
    // Return base address
    return rmsr(IA32_APIC_BASE_MSR) & (0x7FFFFFL << 12); 
}

u8 lapic_id(lapic_t lapic)
{
    // Return local apic id
    return (u8)(mmio_readd(lapic + LAPIC_ID) >> 24);
}

void lapic_end_of_int(lapic_t lapic)
{
    mmio_writed(lapic + LAPIC_EOI, 0);
}

bool lapic_enabled()
{
    return (rmsr(IA32_APIC_BASE_MSR) >> 11) & 1;
}

void lapic_timer_init(lapic_t lapic, u8 interrupt_vector, bool periodic, u32 count, u32 divider)
{
    // Divide config register determines division factor for clock
    divider = (divider ^ ((divider & 0x4) << 1)) & 0xB; // Set bits according to spec
    mmio_writed(lapic + LAPIC_DIVIDE_CONF, divider); 

    // Write LVT Timer Register
    u32 time_reg_val = (((u32)periodic) << 17) | ((u32)interrupt_vector);
    mmio_writed(lapic + LAPIC_LVT_TIMER, time_reg_val);

    // Write count value and thereby start the counter
    mmio_writed(lapic + LAPIC_INIT_COUNT, count);
}

void lapic_timer_deinit(lapic_t lapic)
{
    // Mask interrupts 
    mmio_writed(lapic + LAPIC_LVT_TIMER, mmio_readd(lapic + LAPIC_LVT_TIMER) & (~(1L<<16)));

    // Stop timer by writing 0 to count reg
    mmio_writed(lapic + LAPIC_INIT_COUNT, 0);
}

// IOAPIC related methods

void ioapic_write(ioapic_t apic, u32 reg, u32 data)
{
    mmio_writed(apic + IOAPIC_SEL, reg);
    mmio_writed(apic + IOAPIC_WIN, data);
}

u32 ioapic_read(ioapic_t apic, u32 reg)
{
    mmio_writed(apic + IOAPIC_SEL, reg);
    return mmio_readd(apic + IOAPIC_WIN);
}

u32 ioapic_id(ioapic_t apic)
{
    return ioapic_read(apic, IOAPIC_ID);
}

void ioapic_redirect(ioapic_t apic, u8 index, u64 redirect_entry)
{
    u32 high = (u32)(redirect_entry >> 32);
    u32 low  = (u32)(redirect_entry);

    ioapic_write(apic, IOAPIC_RED + (index << 1), low);
    ioapic_write(apic, IOAPIC_RED + (index << 1) + 1, high);
}

void ioapic_mask(ioapic_t apic, u8 index, bool mask)
{
    // TODO: Mask/unmask interrupt in ioapic
    // TODO: To be able to disable timer
}

bool ioapic_delivery_status(ioapic_t apic, u8 index)
{
    u32 re = ioapic_read(apic, IOAPIC_RED + (index << 1));
    return (re >> 12) & 1;
}

// IPIs

/**
 * Sends an IPI
 */
static void lapic_ipi(lapic_t lapic, 
               u8 dest, u8 shorthand, u8 trigger, 
               u8 level, u8 type, u8 vector)
{
    // Craft ICR value
    u32 high = dest << 24;
    u32 low  = ((shorthand & 0x3) << 18) | 
               ((trigger & 0x1) << 15) |
               ((level & 0x1) << 14) |
               ((type & 0xF) << 8) | 
               vector;
    
    // Actually send IPI (triggered by writting low dword)
    mmio_writed(lapic + LAPIC_ICR_HIGH, high);
    mmio_writed(lapic + LAPIC_ICR_LOW, low); 
}

/**
 * Sends init ipi
 */
static void lapic_ipi_init(lapic_t lapic, u8 lapic_dst_id)
{
    lapic_ipi(lapic, 
              lapic_dst_id, 
              IPI_SHORTHAND_NO, 
              IPI_TRIGGER_EDGE,
              IPI_LEVEL_ASSERT,
              IPI_DELIVERY_INIT, 
              0);
}

/**
 * Sends startup ipi
 */
static void lapic_ipi_startup(lapic_t lapic, u8 lapic_dst_id, u8 vector)
{
    lapic_ipi(lapic, 
              lapic_dst_id,
              IPI_SHORTHAND_NO,
              IPI_TRIGGER_EDGE,
              IPI_LEVEL_ASSERT,
              IPI_DELIVERY_START,
              vector);
}

/**
 * Checks if last ipi was successfully delivered
 */
static bool lapic_ipi_delivered(lapic_t lapic)
{
    // Wait 100ms in total
    i64 timeout = 100;

    while(timeout > 0)
    {
        // Check for delivery
        bool status = (mmio_readd(lapic + LAPIC_ICR_LOW) >> 12) & 1;
        
        if(status == 0)
        {
            // IPI was dispatched
            return true;
        }

        // Wait 1ms (assuming 1 tick = 200 usec)
        pit_delay(5);
    }

    // Could not dispatch IPI
    return false;
}

// Location of SMP trampoline code
extern void smp_trampoline_begin;
extern void smp_trampoline_end;

/**
 * Starts an ap
 */
bool lapic_boot_ap(lapic_t lapic, u8 lapic_dst_id)
{
    bool dispatched;

    // Copy startup code to right location
    memcpy((void*)IPI_TRAMPOLINE_ORIGIN, 
           (void*)&smp_trampoline_begin, 
           (size_t)&smp_trampoline_end - (size_t)&smp_trampoline_begin);

    // Configure timer (1 tick = 200 usec)
    pit_freq(5000);

    // Send init IPI
    lapic_ipi_init(lapic, lapic_dst_id);

    // Make sure IPI was dispatched
    dispatched = lapic_ipi_delivered(lapic);
    if(!dispatched)
        goto fail;

    // Wait 10 ms
    pit_delay(50);

    // Send startup IPI
    lapic_ipi_startup(lapic, lapic_dst_id, IPI_TRAMPOLINE_VECTOR);
    // Make sure IPI was dispatched
    dispatched = lapic_ipi_delivered(lapic);
    if(!dispatched)
    {
        goto fail;
    }
    // Delay 200 usec
    pit_delay(1);

    // TODO: Verify that AP booted successfully (by atomically counting up var)
    
    // Successfully booted AP
    return true;

fail:
    // Couldn't boot AP
    return false;
}