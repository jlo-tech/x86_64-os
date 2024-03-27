#pragma once

#include <io.h>
#include <vga.h>
#include <util.h>
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