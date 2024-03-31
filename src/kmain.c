#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <pit.h>
#include <pci.h>
#include <intr.h>

#include <virtio.h>
#include <multiboot.h>

struct framebuffer fb;

extern struct kheap kernel_heap;
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
    kheap_init(&kernel_heap);

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
        if(etype == 1)
        {
            // NOTE: When adding chunk it must be correctly aligned to a even multiple of PAGE_SIZE
            struct kchunk *kc = (struct kchunk*)eaddr;
            bzero((void*)kc, sizeof(struct kchunk));
            kc->addr = eaddr;
            kc->size = elen / PAGE_SIZE;
            ktree_insert(&kernel_heap.free_buddies[63 - __builtin_clzll(kc->size)], &kc->tree_handle, OFFSET(struct kchunk, tree_handle), (int (*)(void*, void*))cmp_chunks);
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

    vga_clear();

    pci_scan();

    pci_dev_t virtio_dev = {.bus = 0x0, .dev = 0x4, .fun = 0x0};
    virtio_block_dev_init(&virtio_dev);

    // Wait for interrupts
    while(1) 
    {
        __asm__ volatile("hlt");
    }
}