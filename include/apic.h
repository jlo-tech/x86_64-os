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

/* Base mp conf table entries */

struct mp_ct_processor_entry
{
    u8  entry_type;
    u8  local_apic_id;
    u8  local_apic_version;
    u8  cpu_flags;
    u32 cpu_signature;
    u32 feature_flags;
    u64 reserved;
};

struct mp_ct_bus_entry
{
    u8 entry_type;
    u8 bus_id;
    char bus_type_string[6];
};

struct mp_ct_io_apic_entry
{
    u8  entry_type;
    u8  io_apic_id;
    u8  io_apic_version;
    u8  io_apic_flags;
    u32 io_apic_mm_addr;
};

struct mp_ct_io_interrupt_entry
{
    u8  entry_type;
    u8  interrupt_type;
    u16 io_interrupt_flags;
    u8  src_bus_id;
    u8  src_bus_irq;
    u8  dst_io_apic_id;
    u8  dst_io_apic_intin;
};

struct mp_ct_local_interrupt_entry
{
    u8  entry_type;
    u8  interrupt_type;
    u16 local_interrupt_flag;
    u8  src_bus_id;
    u8  src_bus_irq;
    u8  dst_local_apic_id;
    u8  dst_local_apic_lintin; 
};

/* Extended mp conf table entries */

struct mp_ct_ext_sys_addr_space_entry
{
    u8 entry_type;
    u8 entry_length;
    u8 bus_id;
    u8 addr_type;
    u64 addr_base;
    u64 addr_length;    
};

struct mp_ct_ext_bus_hierarchy_desc_entry
{
    u8  entry_type;
    u8  entry_length;
    u8  bus_id;
    u8  bus_info;
    u8  parent_bus;
    u8  reserved0;
    u16 reserved1;
};

struct mp_ct_ext_comp_bus_addr_space_mod_entry
{
    u8  entry_type;
    u8  entry_length;
    u8  bus_id;
    u8  addr_mod;
    u32 range_list;
};

// Search for floating pointer structure 
struct mp_fps* mp_search_fps();

// Check for configuration table 
struct mp_ct_hdr* mp_check_ct(struct mp_fps *fps);

// Parse ct entries and return pointers to array in res array
void mp_ct_entries(struct mp_ct_hdr *hdr, void **res);

// Parse extended ct entries 
bool mp_ct_extended_entries(struct mp_ct_hdr *hdr, void **res);

// Find PIT mapping
struct mp_ct_io_interrupt_entry* mp_ct_find_pit(struct mp_ct_hdr *hdr);

// Find IOAPIC
struct mp_ct_io_apic_entry* mp_ct_find_ioapic(struct mp_ct_hdr *hdr);

//-------//
// LAPIC //
//-------//

#define IA32_APIC_BASE_MSR 0x1B

// Local APIC offsets
#define LAPIC_ID            0x20
#define LAPIC_VERSION       0x30
#define LAPIC_TPR           0x80
#define LAPIC_EOI           0xB0
#define LAPIC_LDR           0xD0    // logical Destination Register
#define LAPIC_DFR           0xE0    // Destination Format Register
#define LAPIC_SIVR          0xF0    // Spurious Interrupt Vector Register
#define LAPIC_ERR           0x280   // Error status register
#define LAPIC_ICR_LOW       0x300   // Interrupt Command Register
#define LAPIC_ICR_HIGH      0x310   // Interrupt Command Register
#define LAPIC_LVT_TIMER     0x320
#define LAPIC_LVT_THERMAL   0x330
#define LAPIC_LVT_PERF_CTR  0x340
#define LAPIC_LVT_LINT0     0x350
#define LAPIC_LVT_LINT1     0x360
#define LAPIC_LVT_ERROR     0x370
#define LAPIC_INIT_COUNT    0x380   // Initial Count Register (for Timer)
#define LAPIC_CURR_COUNT    0x390   // Current Count Register (for Timer)
#define LAPIC_DIVIDE_CONF   0x3E0   // Divide Configuration Register (for Timer)

typedef size_t lapic_t;

lapic_t lapic_init(u8 spurious_interrupt_vector, 
                   u8 lint0_interrupt_vector, 
                   u8 lint1_interrupt_vector,
                   u8 error_interrupt_vector);
lapic_t lapic_fetch();
u8      lapic_id();

bool   lapic_enabled();
void   lapic_end_of_int();

void lapic_timer_init(lapic_t lapic, u8 interrupt_vector, bool periodic, u32 count, u32 divider);
void lapic_timer_deinit(lapic_t lapic);

//------//
// IPIs //
//------//

#define IPI_SHORTHAND_NO            0x0
#define IPI_SHORTHAND_SELF          0x1
#define IPI_SHORTHAND_ALL_IN_SELF   0x2
#define IPI_SHORTHAND_ALL_EX_SELF   0x3

#define IPI_TRIGGER_EDGE    0x0
#define IPI_TRIGGER_LEVEL   0x1

#define IPI_LEVEL_DEASSERT  0x0
#define IPI_LEVEL_ASSERT    0x1

#define IPI_DELIVERY_FIXED  0x0
#define IPI_DELIVERY_LPRIO  0x1
#define IPI_DELIVERY_SMI    0x2
#define IPI_DELIVERY_NMI    0x4
#define IPI_DELIVERY_INIT   0x5
#define IPI_DELIVERY_START  0x6

#define IPI_TRAMPOLINE_VECTOR 0x8
#define IPI_TRAMPOLINE_ORIGIN (IPI_TRAMPOLINE_VECTOR << 12)

// Boot an AP
bool lapic_boot_ap(lapic_t lapic, u8 lapic_dst_id);

//--------//
// IOAPIC //
//--------//

typedef size_t ioapic_t;

#define IOAPIC_SEL  0x00
#define IOAPIC_WIN  0x10

#define IOAPIC_ID   0x00
#define IOAPIC_VER  0x01
#define IOAPIC_ARB  0x02
#define IOAPIC_RED  0x10

void ioapic_write(ioapic_t apic, u32 reg, u32 data);
u32  ioapic_read(ioapic_t apic, u32 reg);

u32 ioapic_id(ioapic_t apic);
void ioapic_redirect(ioapic_t apic, u8 index, u64 redirect_entry);
bool ioapic_delivery_status(ioapic_t apic, u8 index);
void ioapic_mask(ioapic_t apic, u8 index, u32 mask);

// Generic SMP stuff

#define SMP_STACK_SIZE 16364

struct smp_boot_params
{
    u8 *stack_pointer;      // Reinitialized for each core
    bool launch_success;
} __attribute__((packed));