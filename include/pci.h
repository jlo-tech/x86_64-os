#pragma once

#include <io.h>
#include <util.h>
#include <types.h>

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

#define PCI_INVALID 0xFFFF // Vendor id for unplugged device

typedef struct
{
    u16 vendor_id;
    u16 device_id;

    u16 command;
    u16 status;

    u8 revision_id;
    u32 classcode;

    u8 cache_line_size;
    u8 latency_timer;
    u8 header_type;
    u8 bist;

    union
    {
        // Standard (Header Type = 0)
        struct
        {
            u32 bar0;
            u32 bar1;
            u32 bar2;
            u32 bar3;
            u32 bar4;
            u32 bar5;
            
            u32 cardbus_cis_pointer;
            
            u16 subsystem_vendor_id;
            u16 subsystem_id;

            u32 expansion_rom_base_address;

            u8 capabilities_pointer;

            u8 interrupt_line;
            u8 interrupt_pin;
            u8 min_grant;
            u8 max_latency;
        } standard;

        struct 
        {
            u32 bar0;
            u32 bar1;

            u8 primary_bus_no;
            u8 secondary_bus_no;
            u8 subordinate_bus_no;
            u8 secondary_latency_timer;

            u8 io_base;
            u8 io_limit;
            u16 secondary_status;

            u16 memory_base;
            u16 memory_limit;

            u16 prefetchable_memory_base;
            u16 prefetchable_memory_limit;

            u32 prefetchable_memory_base_upper;
            u32 prefetchable_memory_limit_upper;
            
            u16 io_base_upper;
            u16 io_limit_upper;

            u8 capabilities_pointer;

            u32 extension_rom_base_address;
            
            u8 interrupt_line;
            u8 interrupt_pin;

            u16 bridge_control;
        } bridge;
    };
} pci_dev_t;

void pci_device_info(pci_dev_t *pci_dev, u8 bus, u8 dev, u8 fun);
bool pci_device_ready(pci_dev_t *dev);