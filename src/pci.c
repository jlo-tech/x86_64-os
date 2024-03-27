#include <pci.h>

/* Note:
    In all pci_read_xxx/pci_write_xxx methods
    REG needs to be aligned to a 4 byte boundary
*/

u32 pci_read_dword(pci_dev_t *pci_dev, u8 reg)
{
    u32 pbus = ((u32)pci_dev->bus) << 16;
    u32 pdev = ((u32)pci_dev->dev) << 11;
    u32 pfun = ((u32)pci_dev->fun) << 8;
    u32 preg = ((u32)reg) << 0;

    u32 addr = ((u32)0x80000000) | pbus | pdev | pfun | preg;

    outd(PCI_ADDR, addr);
    
    return ind(PCI_DATA);
}

u16 pci_read_word(pci_dev_t *pci_dev, u8 reg)
{
    u32 pbus = ((u32)pci_dev->bus) << 16;
    u32 pdev = ((u32)pci_dev->dev) << 11;
    u32 pfun = ((u32)pci_dev->fun) << 8;
    u32 preg = ((u32)reg) << 0;

    u32 addr = ((u32)0x80000000) | pbus | pdev | pfun | preg;

    outd(PCI_ADDR, addr);
    
    return inw(PCI_DATA);
}

u8 pci_read_byte(pci_dev_t *pci_dev, u8 reg)
{
    u32 pbus = ((u32)pci_dev->bus) << 16;
    u32 pdev = ((u32)pci_dev->dev) << 11;
    u32 pfun = ((u32)pci_dev->fun) << 8;
    u32 preg = ((u32)reg) << 0;

    u32 addr = ((u32)0x80000000) | pbus | pdev | pfun | preg;

    outd(PCI_ADDR, addr);
    
    return inb(PCI_DATA);
}

void pci_write_dword(pci_dev_t *pci_dev, u8 reg, u32 val)
{
    u32 pbus = ((u32)pci_dev->bus) << 16;
    u32 pdev = ((u32)pci_dev->dev) << 11;
    u32 pfun = ((u32)pci_dev->fun) << 8;
    u32 preg = ((u32)reg) << 0;

    u32 addr = ((u32)0x80000000) | pbus | pdev | pfun | preg;

    outd(PCI_ADDR, addr);
    outd(PCI_DATA, val);
}

void pci_write_word(pci_dev_t *pci_dev, u8 reg, u16 val)
{
    u32 pbus = ((u32)pci_dev->bus) << 16;
    u32 pdev = ((u32)pci_dev->dev) << 11;
    u32 pfun = ((u32)pci_dev->fun) << 8;
    u32 preg = ((u32)reg) << 0;

    u32 addr = ((u32)0x80000000) | pbus | pdev | pfun | preg;

    outd(PCI_ADDR, addr);
    outw(PCI_DATA, val);
}

void pci_write_byte(pci_dev_t *pci_dev, u8 reg, u8 val)
{
    u32 pbus = ((u32)pci_dev->bus) << 16;
    u32 pdev = ((u32)pci_dev->dev) << 11;
    u32 pfun = ((u32)pci_dev->fun) << 8;
    u32 preg = ((u32)reg) << 0;

    u32 addr = ((u32)0x80000000) | pbus | pdev | pfun | preg;

    outd(PCI_ADDR, addr);
    outb(PCI_DATA, val);
}

bool pci_ready(pci_dev_t *pci_dev)
{
    return (pci_read_word(pci_dev, 0x0) != (u16)PCI_INVALID);
}

u16 pci_vendor_id(pci_dev_t *pci_dev)
{
    return pci_read_word(pci_dev, 0x0);
}

u16 pci_device_id(pci_dev_t *pci_dev)
{
    return pci_read_dword(pci_dev, 0x0) >> 16;
}

u8 pci_revision_id(pci_dev_t *pci_dev)
{
    return pci_read_byte(pci_dev, 0x8);
}

u8 pci_class_code(pci_dev_t *pci_dev)
{
    return (pci_read_dword(pci_dev, 0x8) >> 24) & 0xFF;
}

u8 pci_subclass_code(pci_dev_t *pci_dev)
{
    return (pci_read_dword(pci_dev, 0x8) >> 16) & 0xFF;
}

u8 pci_programming_interface(pci_dev_t *pci_dev)
{
    return (pci_read_dword(pci_dev, 0x8) >> 8) & 0xFF;
}

u8 pci_header_type(pci_dev_t *pci_dev)
{
    return (pci_read_dword(pci_dev, 0xC) >> 16) & 0x7F;
}

// Is PCI device multi function?
u8 pci_multi_function(pci_dev_t *pci_dev)
{
    return ((pci_read_dword(pci_dev, 0xC) >> 16) & 0x80) >> 7;
}

u8 pci_bar(pci_dev_t *pci_dev, u8 bar_index)
{
    u8 ht = pci_header_type(pci_dev);
    if((ht == 0x0 && bar_index > 5) || (ht == 1 && bar_index > 1))
    {
        // Error
        return -1;
    }

    u32 bar = pci_read_dword(pci_dev, 0x10 + 0x4 * bar_index);
    return bar;
}

// 0 = Memory mapped, 1 = IO mapped
u8 pci_bar_mem_type(pci_dev_t *pci_dev, u8 bar_index)
{
    u32 bar = pci_bar(pci_dev, bar_index);
    if(bar == (u32)-1)
        return -1;
    return bar & 0x1;
}

// 32 (= 0) vs 64 (= 1) bit memory address
u8 pci_bar_mem_addr_size(pci_dev_t *pci_dev, u8 bar_index)
{
    u32 mt = pci_bar_mem_type(pci_dev, bar_index);
    if(mt == (u32)-1 || mt == (u32)1)
        return -1;
    
    // 32 bit
    if(((mt >> 1) & 0x3) == 0)
        return 0;

    // 64 bit
    if(((mt >> 1) & 0x3) == 2)
        return 1;
    
    // Reserved
    return -1;
}

// Determines address space of bar
u64 pci_bar_addr_space(pci_dev_t *pci_dev, u8 bar_index)
{
    u8 as = pci_bar_mem_addr_size(pci_dev, bar_index);

    if(as == (u8)-1)
        return -1; // Error

    // Disable non intentional writes
    u16 command = pci_read_word(pci_dev, 0x4);
    pci_write_word(pci_dev, 0x4, command & 0xFFFC);

    if(as == 0)
    {
        // 32 bit bar type
        u32 backup = pci_bar(pci_dev, bar_index);
        // Write all ones
        pci_write_dword(pci_dev, 0x10 + 0x4 * bar_index, (u32)0xFFFFFFFF);
        // Calculate address space size
        u32 space = (~pci_bar(pci_dev, bar_index)) + 1;
        // Restore old value
        pci_write_dword(pci_dev, 0x10 + 0x4 * bar_index, backup);
        // Enable device again
        pci_write_word(pci_dev, 0x4, command);
        // Return size
        return (u64)space;
    }
    else
    {
        // 64bit bar type
        u32 backup0 = pci_bar(pci_dev, bar_index);
        u32 backup1 = pci_bar(pci_dev, bar_index + 1);
        // Write all ones
        pci_write_dword(pci_dev, 0x10 + 0x4 * bar_index, (u32)0xFFFFFFFF);
        pci_write_dword(pci_dev, 0x10 + 0x4 * (bar_index+1), (u32)0xFFFFFFFF);
        // Read back
        u64 rb0 = (u64)pci_bar(pci_dev, bar_index);
        u64 rb1 = (u64)pci_bar(pci_dev, bar_index+1);
        u64 space = (~((rb1 << 32) | rb0)) + 1;
        // Restore old values
        pci_write_dword(pci_dev, 0x10 + 0x4 * bar_index, backup0);
        pci_write_dword(pci_dev, 0x10 + 0x4 * (bar_index+1), backup1);
        // Enable device again
        pci_write_word(pci_dev, 0x4, command);
        // Return size
        return space;
    }
}

#include <vga.h>
extern struct framebuffer fb;

void pci_scan()
{
    vga_printf(&fb, "PCI Scan:\n");
    for(u16 bus = 0; bus < 256; bus++)
    {
        for(u16 dev = 0; dev < 32; dev++)
        {
            for(u16 fun = 0; fun < 8; fun++)
            {
                pci_dev_t pdev = {.bus = bus, .fun = fun, .dev = dev};

                if(pci_ready(&pdev))
                {
                    u16 vid = pci_vendor_id(&pdev);
                    u16 did = pci_device_id(&pdev);
                    u8 cc = pci_class_code(&pdev);
                    u8 sc = pci_subclass_code(&pdev);
                    u8 pi = pci_programming_interface(&pdev);
                    u8 ht = pci_header_type(&pdev);
                    u8 mf = pci_multi_function(&pdev);
                    u8 bmt = pci_bar_mem_type(&pdev, 0);
                    u64 as = pci_bar_mem_addr_size(&pdev, 0);
                    u64 bs = pci_bar_addr_space(&pdev, 0);

                    vga_printf(&fb, "VID: %h DID: %h CC: %h SC: %h PI: %h HT: %h BMT: %h MF: %h AS: %h BS: %h\n", vid, did, cc, sc, pi, ht, bmt, mf, as, bs);
                }
            }
        }
    }
}