#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <pit.h>
#include <pci.h>
#include <intr.h>
#include <syscalls.h>
#include <user_mode.h>

#include <virtio.h>
#include <virtio_blk.h>

#include <multiboot.h>

struct framebuffer fb;

extern struct kheap kernel_heap;
extern int cmp_chunks(struct kchunk *c0, struct kchunk* c1);


extern void switch_context(struct interrupt_context *ctx);
char __attribute__((aligned(4096))) user_stack[4096];
void user_func()
{
    while(1)
    {
        asm volatile(
            "mov $777, %rdi\n" 
            "syscall \n"
        );
    }
}

void kmain(struct multiboot_information *mb_info)
{
    // Setup identity page mapping
    paging_id_full();

    tss_init();

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

    pci_dev_t pci_dev = {.bus = 0x0, .dev = 0x4, .fun = 0x0};
    virtio_dev_t virtio_dev;
    virtio_blk_dev_t blk_dev;
    
    virtio_dev_init(&virtio_dev, &pci_dev, 1);
    virtio_block_dev_init(&blk_dev, &virtio_dev);

    // Write
    u8 *data_out = (u8*)align(kmalloc(4096), 4096);
    bzero(data_out, 512);

    data_out[0] = 'O';
    data_out[1] = 'k';

    virtio_block_dev_write(&blk_dev, 0, data_out, 1);

    // Read
    u8 *data_in = (u8*)align(kmalloc(4096), 4096);
    bzero(data_in, 512);

    virtio_block_dev_read(&blk_dev, 0, data_in, 1);

    vga_printf(&fb, "%s\n", data_in);

    // Enable syscalls
    syscalls_setup();

    // Switch to user mode
    struct interrupt_context ctx;
    ctx.rip = (u64)user_func;
    ctx.cs = (4 << 3) | 3;
    ctx.rflags = 0x202;
    ctx.rsp = (u64)user_stack;
    ctx.ds = (3 << 3) | 3;

    switch_context(&ctx);

    // TODO: Currently all pages are also user accessabile (see vmm.c) remove that and implement proper paging
    // TODO: Syscall handler

    // Wait for interrupts
    while(1) 
    {
        __asm__ volatile("hlt");
    }
}