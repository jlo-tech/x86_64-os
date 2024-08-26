#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <pit.h>
#include <pci.h>
#include <apic.h>
#include <intr.h>
#include <kernel.h>
#include <syscalls.h>
#include <user_mode.h>

#include <virtio.h>
#include <virtio_blk.h>

#include <multiboot.h>

#include <fs/fs.h>

extern struct kheap kernel_heap;
extern int cmp_chunks(struct kchunk *c0, struct kchunk* c1);

// Linker variables
extern void kernel_base;
extern void kernel_limit;
const u64 kernel_base_addr  = (u64)&kernel_base;
const u64 kernel_limit_addr = (u64)&kernel_limit;

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

    kclear();
    kprintf("Kernel at your service!\n");

    // Init heap
    kheap_init(&kernel_heap);

    kprintf("Multiboot info struct: %u\n", (u64)mb_info);
    kprintf("Multiboot memory map: %u\n", (u64)multiboot_memmap(mb_info));

    struct multiboot_memory_map* memmap = multiboot_memmap(mb_info);

    u64 num_entries = multiboot_memmap_num_entries(memmap);
    kprintf("Memory map num entries: %u\n", num_entries);

    for(int i = 0; i < multiboot_memmap_num_entries(memmap); i++)
    {
        struct multiboot_memory_map_entry *entry = (struct multiboot_memory_map_entry*)(((u8*)memmap + 16) + i * memmap->entry_size);
        u64 etype = (u64)entry->type;
        u64 eaddr = (u64)entry->address;
        u64 elen  = (u64)entry->length;
        kprintf("Index: %d, Type: %d, Addr: %d, Len: %d\n", i, etype, eaddr, elen);

        // Insert some memory chunk
        if(etype == 1 && i != 3) // Dont load chunk were kernel lays TODO: find more dynamic approach to this
        {
            // NOTE: When adding chunk it must be correctly aligned to a even multiple of PAGE_SIZE
            struct kchunk *kc = (struct kchunk*)eaddr;
            bzero((void*)kc, sizeof(struct kchunk));
            kc->addr = eaddr;
            kc->size = elen / PAGE_SIZE;
            ktree_insert(&kernel_heap.free_buddies[63 - __builtin_clzll(kc->size)], &kc->tree_handle, OFFSET(struct kchunk, tree_handle), (int (*)(void*, void*))cmp_chunks);
        }
    }

    kprintf("Kernel start %d\n", kernel_base_addr);
    kprintf("Kernel limit %d\n", kernel_limit_addr);

    // Setup local interrupts
    //intr_setup();

    // Enable external interrupts
    // pic_init();

    // Configure timer
    // pit_freq(-1); // Max sleep time

    kclear();

    pci_scan();

    // TODO: Find PCI device by vendor id
    pci_dev_t pci_dev = {.bus = 0x0, .dev = 0x4, .fun = 0x0};
    virtio_dev_t virtio_dev;
    virtio_blk_dev_t blk_dev;

    virtio_dev_init(&virtio_dev, &pci_dev, 1);
    virtio_block_dev_init(&blk_dev, &virtio_dev);

   
    // Test fs...
    struct fs fs;
    fs_init(&fs, &blk_dev, true); 
    
    fs_mk(&fs, "/", "File", FS_TYPE_FILE);
    i64 handle = fs_handle(&fs, "/File");
 
    
    fs_wrfl(&fs, handle, (u8*)"Content data...", sizeof("Content data..."));  
    
    fs_seek(&fs, handle, 2);

    char data[64] = {0};
    fs_refl(&fs, handle, (u8*)data, 16);
    
    kprintf("%s\n", data);

    // Parse MP Tables
    kclear();

    void **page = (void**)kmalloc(4096);
    struct mp_ct_hdr *hdr = mp_check_ct(mp_search_fps());
    mp_ct_entries(hdr, page);
    mp_ct_extended_entries(hdr, page);

    kprintf("%h %h %d\n", hdr->local_apic_mm_addr, lapic_base_addr(), lapic_enabled());


    // Enable syscalls
    syscalls_setup();

    // Setup paging
    struct page_table *pt = (struct page_table*)align(kmalloc(4096), 4096);
    bzero((u8*)pt, sizeof(struct page_table));

#if 0
    // Map kernel but this time make it user accessible
    paging_map_range(pt, 0, 0, 
        PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE, 
        align(kernel_limit_addr, 4096),
        4096);

    // Load new pt
    //paging_activate(pt);
#endif

    // Switch to user mode
    struct interrupt_context ctx;
    ctx.rip = (u64)user_func;
    ctx.cs = (4 << 3) | 3;
    ctx.rflags = 0x202;
    ctx.rsp = (u64)user_stack;
    ctx.ds = (3 << 3) | 3;

    intr_setup();
    pic_disable();
    intr_enable();
 
    lapic_init(0xF1);
    lapic_timer_init(0xF2, true, 1000000, 6);


    //switch_context(&ctx);

    // Wait for interrupts
    while(1) 
    {
        __asm__ volatile("hlt");
    }
}
