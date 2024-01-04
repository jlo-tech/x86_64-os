#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <intr.h>

#include <multiboot.h>

static struct framebuffer fb;

void kmain(struct multiboot_information *mb_info)
{
    // Setup identity page mapping
    paging_id_full();

    fb.fgc = green;
    fb.bgc = black;
    fb.bright = true;

    vga_clear(&fb);
    vga_printf(&fb, "Kernel at your service!\n");

    vga_printf(&fb, "Multiboot info struct: %u\n", (u64)mb_info);
    vga_printf(&fb, "Multiboot memory map: %u\n", (u64)multiboot_memmap(mb_info));

    struct multiboot_memory_map* memmap = multiboot_memmap(mb_info);

    u64 num_entries = multiboot_memmap_num_entries(memmap);
    vga_printf(&fb, "Memory map num entries: %u\n", num_entries);

    struct node *root = 0;

    for(int i = 0; i < multiboot_memmap_num_entries(memmap); i++)
    {
        struct multiboot_memory_map_entry *entry = (((u8*)memmap + 16) + i * memmap->entry_size);
        u64 etype = (u64)entry->type;
        u64 eaddr = (u64)entry->address;
        u64 elen  = (u64)entry->length;

        if(true)
        {
            vga_printf(&fb, "%u %u %h\n", etype, eaddr, elen);

            if(i == 0)
            {
                root->size = elen;
            }

            if(i == 3)
            {
                root->next = eaddr;
                root->next->size = elen;
            }

            if(i == 6)
            {
                root->next->next = eaddr;
                root->next->next->size = elen;
            }
        }
    }

    vga_printf(&fb, "%h\n", root->size);
    vga_printf(&fb, "%h\n", root->next->size);
    vga_printf(&fb, "%h\n", root->next->next->size);

    intr_setup();
    intr_enable();

    vga_printf(&fb, "Still alive!\n");

    // Wait for interrupts
    __asm__ volatile("hlt");
}