#include <pci.h>

static u16 pci_read(u32 addr)
{
    outd(PCI_ADDR, addr);
    return ind(PCI_DATA);
}

static void pci_write(u32 addr, u16 val)
{
    outd(PCI_ADDR, addr);
    outd(PCI_DATA, val);
}

static u32 pci_addr(u8 bus, u8 dev, u8 fun, u8 reg)
{
    u32 pbus = ((((u32)bus) & 0xFF) << 16);
    u32 pdev = ((((u32)dev) & 0x1F) << 11);
    u32 pfun = ((((u32)fun) & 0x07) << 8);
    u32 preg = ((((u32)reg) & 0xFC) << 0);

    u32 addr = ((u32)0x80000000) | pbus | pdev | pfun | preg;

    return addr;
}

// Loads fields of configuration space, header type specific fields are omitted
void pci_device_info(pci_dev_t *pci_dev, u8 bus, u8 dev, u8 fun)
{
    u32 t;

    // Cleanup
    bzero((u8*)pci_dev, sizeof(pci_dev_t));

    // Load vendor id
    t = pci_read(pci_addr(bus, dev, fun, 0x0));

    pci_dev->vendor_id = (t >> 0) & 0xFFFF;
    pci_dev->device_id = (t >> 16) & 0xFFFF;

    // Check if device is plugged in
    if(pci_dev->vendor_id == PCI_INVALID)
        return;

    t = pci_read(pci_addr(bus, dev, fun, 0x4));

    pci_dev->command = (t >> 0) & 0xFFFF;
    pci_dev->status = (t >> 16) & 0xFFFF;

    t = pci_read(pci_addr(bus, dev, fun, 0x8));

    pci_dev->revision_id = (t >> 0) & 0xFF;
    pci_dev->classcode = (t >> 8) & 0xFFFFFF;
    
    t = pci_read(pci_addr(bus, dev, fun, 0xC));

    pci_dev->cache_line_size = (t >> 0) & 0xFF;
    pci_dev->latency_timer = (t >> 8) & 0xFF;
    pci_dev->header_type = (t >> 16) & 0xFF;
    pci_dev->bist = (t >> 24) & 0xFF;

    // Standard device or PCI/PCI-bridge
    if(pci_dev->header_type == 0x00)
    {
        t = pci_read(pci_addr(bus, dev, fun, 0x10));
        pci_dev->standard.bar0 = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x14));
        pci_dev->standard.bar1 = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x18));
        pci_dev->standard.bar2 = t;
        
        t = pci_read(pci_addr(bus, dev, fun, 0x1C));
        pci_dev->standard.bar3 = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x20));
        pci_dev->standard.bar4 = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x24));
        pci_dev->standard.bar5 = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x28));
        pci_dev->standard.cardbus_cis_pointer = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x2c));
        pci_dev->standard.subsystem_vendor_id = (t >> 0) & 0xFFFF;
        pci_dev->standard.subsystem_id = (t >> 16) & 0xFFFF;

        t = pci_read(pci_addr(bus, dev, fun, 0x30));
        pci_dev->standard.expansion_rom_base_address = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x34));
        pci_dev->standard.capabilities_pointer = t & 0xFF;

        t = pci_read(pci_addr(bus, dev, fun, 0x3c));
        pci_dev->standard.interrupt_line = (t >> 0) & 0xFF;
        pci_dev->standard.interrupt_pin = (t >> 8) & 0xFF;
        pci_dev->standard.min_grant = (t >> 16) & 0xFF;
        pci_dev->standard.max_latency = (t >> 24) & 0xFF;
    }
    else if(pci_dev->header_type == 0x01)
    {
        t = pci_read(pci_addr(bus, dev, fun, 0x10));
        pci_dev->bridge.bar0 = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x14));
        pci_dev->bridge.bar1 = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x18));
        pci_dev->bridge.primary_bus_no = (t >> 0) & 0xFF;
        pci_dev->bridge.secondary_bus_no = (t >> 8) & 0xFF;
        pci_dev->bridge.subordinate_bus_no = (t >> 16) & 0xFF;
        pci_dev->bridge.secondary_latency_timer = (t >> 24) & 0xFF;

        t = pci_read(pci_addr(bus, dev, fun, 0x1C));
        pci_dev->bridge.io_base = (t >> 0) & 0xFF;
        pci_dev->bridge.io_limit = (t >> 8) & 0xFF;
        pci_dev->bridge.secondary_status = (t >> 16) & 0xFFFF;
        
        t = pci_read(pci_addr(bus, dev, fun, 0x20));
        pci_dev->bridge.memory_base = (t >> 0) & 0xFFFF;
        pci_dev->bridge.memory_limit = (t >> 16) & 0xFFFF;

        t = pci_read(pci_addr(bus, dev, fun, 0x24));
        pci_dev->bridge.prefetchable_memory_base = (t >> 0) & 0xFFFF;
        pci_dev->bridge.prefetchable_memory_limit = (t >> 16) & 0xFFFF;

        t = pci_read(pci_addr(bus, dev, fun, 0x28));
        pci_dev->bridge.prefetchable_memory_base_upper = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x2C));
        pci_dev->bridge.prefetchable_memory_limit_upper = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x30));
        pci_dev->bridge.io_base_upper = (t >> 0) & 0xFFFF;
        pci_dev->bridge.io_limit_upper = (t >> 16) & 0xFFFF;

        t = pci_read(pci_addr(bus, dev, fun, 0x34));
        pci_dev->bridge.capabilities_pointer = t & 0xFF;

        t = pci_read(pci_addr(bus, dev, fun, 0x38));
        pci_dev->bridge.extension_rom_base_address = t;

        t = pci_read(pci_addr(bus, dev, fun, 0x3c));
        pci_dev->bridge.interrupt_line = (t >> 0) & 0xFF;
        pci_dev->bridge.interrupt_pin = (t >> 8) & 0xFF;
        pci_dev->bridge.bridge_control = (t >> 16) & 0xFFFF;
    }
}

bool pci_device_ready(pci_dev_t *dev)
{
    return dev->vendor_id != PCI_INVALID;
}