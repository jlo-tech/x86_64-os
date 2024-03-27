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
                u16 vid = pci_vendor_id(&pdev);
                u16 did = pci_device_id(&pdev);
                u8 cc = pci_class_code(&pdev);
                u8 sc = pci_subclass_code(&pdev);
                u8 pi = pci_programming_interface(&pdev);
                u8 ht = pci_header_type(&pdev);
                u8 mf = pci_multi_function(&pdev);

                if((vid & 0xFFFF) != PCI_INVALID)
                {
                    u32 m = pci_read_dword(&pdev, 8);
                    vga_printf(&fb, "VID: %h DID: %h CC: %h SC: %h PI: %h HT: %h MF %h\n", vid, did, cc, sc, pi, ht, mf);
                }
            }
        }
    }
}