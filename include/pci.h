#pragma once

#include <io.h>
#include <types.h>

#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC

#define PCI_INVALID 0xFFFF // Vendor id for unplugged device

typedef struct
{
    u8 bus;
    u8 dev;
    u8 fun;
} pci_dev_t;

u32 pci_read_dword(pci_dev_t *pci_dev, u8 reg);
u16 pci_read_word(pci_dev_t *pci_dev, u8 reg);
u8 pci_read_byte(pci_dev_t *pci_dev, u8 reg);

void pci_write_dword(pci_dev_t *pci_dev, u8 reg, u32 val);
void pci_write_word(pci_dev_t *pci_dev, u8 reg, u16 val);
void pci_write_byte(pci_dev_t *pci_dev, u8 reg, u8 val);

bool pci_ready(pci_dev_t *pci_dev);
u16 pci_vendor_id(pci_dev_t *pci_dev);
u16 pci_device_id(pci_dev_t *pci_dev);
u8 pci_revision_id(pci_dev_t *pci_dev);
u8 pci_class_code(pci_dev_t *pci_dev);
u8 pci_subclass_code(pci_dev_t *pci_dev);
u8 pci_programming_interface(pci_dev_t *pci_dev);
u8 pci_header_type(pci_dev_t *pci_dev);
u8 pci_multi_function(pci_dev_t *pci_dev);
u32 pci_bar(pci_dev_t *pci_dev, u8 bar_index);
u8 pci_bar_mem_type(pci_dev_t *pci_dev, u8 bar_index);
u8 pci_bar_mem_addr_size(pci_dev_t *pci_dev, u8 bar_index);
u64 pci_bar_addr_space(pci_dev_t *pci_dev, u8 bar_index);

void pci_scan();