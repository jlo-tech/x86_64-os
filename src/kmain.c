#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <intr.h>

#include <multiboot.h>

static struct framebuffer fb;

struct container
{
    u64 data;
    struct ktree_node tree;
} __attribute__((packed));

int cmp(void *x, void *y)
{
    u64 a = *(u64*)x;
    u64 b = *(u64*)y;

    if(a == b)
        return 0;
    if(a < b)
        return 1;
    if(a > b)
        return -1;

    return 0;
}

struct container c0 = {.data = 0, .tree = {.valid = {0, 0}}};
struct container c1 = {.data = 1, .tree = {.valid = {0, 0}}};
struct container c2 = {.data = 2, .tree = {.valid = {0, 0}}};
struct container c3 = {.data = 3, .tree = {.valid = {0, 0}}};
struct container c4 = {.data = 4, .tree = {.valid = {0, 0}}};
struct container c5 = {.data = 5, .tree = {.valid = {0, 0}}};

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
        struct multiboot_memory_map_entry *entry = (struct multiboot_memory_map_entry*)(((u8*)memmap + 16) + i * memmap->entry_size);
        u64 etype = (u64)entry->type;
        u64 eaddr = (u64)entry->address;
        u64 elen  = (u64)entry->length;
        vga_printf(&fb, "Type: %d, Addr: %d, Len: %d\n", etype, eaddr, elen);
    }

    intr_setup();
    intr_enable();

    vga_printf(&fb, "Still alive!\n");

    struct ktree rt = {.valid = false};

    ktree_insert(&rt, &c4.tree, 8, cmp);
    ktree_insert(&rt, &c1.tree, 8, cmp);
    ktree_insert(&rt, &c0.tree, 8, cmp);
    ktree_insert(&rt, &c2.tree, 8, cmp);
    ktree_insert(&rt, &c3.tree, 8, cmp);
    ktree_insert(&rt, &c5.tree, 8, cmp);

    ktree_remove(&rt, &c1.tree, 8, cmp);

    u64 x = 4;
    vga_printf(&fb, "%d \n", ktree_contains(&rt, &x, 8, cmp));

    vga_printf(&fb, "Still alive\n");

    // Wait for interrupts
    __asm__ volatile("hlt");
}