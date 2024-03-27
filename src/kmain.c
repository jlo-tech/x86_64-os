#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <pit.h>
#include <pci.h>
#include <intr.h>

#include <multiboot.h>

struct framebuffer fb;

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

struct klist rl;
struct klist_node ln0;
struct klist_node ln1;
struct klist_node ln2;

struct kheap heap;
extern int cmp_chunks(struct kchunk *c0, struct kchunk* c1);

void kmain(struct multiboot_information *mb_info)
{
    // Setup identity page mapping
    paging_id_full();

    fb.fgc = green;
    fb.bgc = black;
    fb.bright = true;

    vga_clear(&fb);
    vga_printf(&fb, "Kernel at your service!\n");

    // Init heap
    kheap_init(&heap);

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

        // Insert some memory chunk
        if(i == 0)
        {
            // NOTE: When adding chunk it must be correctly aligned to a even multiple of PAGE_SIZE
            struct kchunk *kc = (struct kchunk*)0;
            bzero((void*)kc, sizeof(struct kchunk));
            kc->addr = 0;
            kc->size = 16;
            ktree_insert(&heap.free_buddies[4], &kc->tree_handle, OFFSET(struct kchunk, tree_handle), (int (*)(void*, void*))cmp_chunks);
        }
    }

    // Enable local interrupts
    intr_setup();
    intr_enable();

    // Enable external interrupts
    pic_init();
    
    // Configure timer
    pit_freq(-1); // Max sleep time

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

    // Test klist...
    klist_push(&rl, &ln0);
    klist_push(&rl, &ln1);
    klist_push(&rl, &ln2);
    
    klist_pop(&rl, &ln1);
    klist_pop(&rl, &ln0);
    klist_pop(&rl, &ln2);

    i64 ptr0 = kheap_alloc(&heap, 4096);
    i64 ptr1 = kheap_alloc(&heap, 4096);
    i64 ptr2 = kheap_alloc(&heap, 4096);
    i64 ptr3 = kheap_alloc(&heap, 2048);

    kheap_free(&heap, ptr0);
    kheap_free(&heap, ptr1);
    kheap_free(&heap, ptr2);
    kheap_free(&heap, ptr3);

    vga_printf(&fb, "Still alive!\n");

    pci_scan();

    // Wait for interrupts
    while(1) 
    {
        __asm__ volatile("hlt");
    }
}