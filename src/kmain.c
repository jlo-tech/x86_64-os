#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <intr.h>

#include <multiboot.h>

static struct framebuffer fb;

// Static vars for testing the ktree code
struct ktree_root kt_root;
struct container kt_node0;
struct container kt_node1;
struct container kt_node2;
struct container kt_node3;

struct container
{
    u64 selector;
    struct ktree_node node;
} __attribute__((packed));

i64 cmp(struct container *c0, struct container *c1)
{
    if(c0->selector < c1->selector)
    {
        return -1;
    }
    if(c0->selector > c1->selector)
    {
        return 1;
    }
    return 0;
}

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

    for(int i = 0; i < multiboot_memmap_num_entries(memmap); i++)
    {
        struct multiboot_memory_map_entry *entry = (((u8*)memmap + 16) + i * memmap->entry_size);
        u64 etype = (u64)entry->type;
        u64 eaddr = (u64)entry->address;
        u64 elen  = (u64)entry->length;
    }

    intr_setup();
    intr_enable();

    vga_printf(&fb, "Still alive!\n");

    kt_node0.selector = 1;
    kt_node1.selector = 0;
    kt_node2.selector = 3;
    kt_node3.selector = 2;

    ktree_insert(&kt_root, &kt_node0, 8, cmp);
    ktree_insert(&kt_root, &kt_node1, 8, cmp);
    ktree_insert(&kt_root, &kt_node2, 8, cmp);
    ktree_insert(&kt_root, &kt_node3, 8, cmp);

    vga_printf(&fb, "Still alive\n");

    // Wait for interrupts
    __asm__ volatile("hlt");
}