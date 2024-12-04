#include <vga.h>
#include <pmm.h>
#include <vmm.h>
#include <pit.h>
#include <pci.h>
#include <apic.h>
#include <intr.h>
#include <sync.h>
#include <kernel.h>
#include <net/net.h>
#include <syscalls.h>
#include <user_mode.h>

#include <virtio.h>
#include <virtio_blk.h>
#include <virtio_net.h>

#include <multiboot.h>

#include <fs/fs.h>
#include <net/net.h>

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

#if 0
    kprintf("Kernel start %d\n", kernel_base_addr);
    kprintf("Kernel limit %d\n", kernel_limit_addr);
#endif

    pci_scan();

    // TODO: Find PCI device by device id
    pci_dev_t pci_dev_blk = {.bus = 0x0, .dev = 0x3, .fun = 0x0};
    virtio_dev_t virtio_dev_blk;
    virtio_blk_dev_t blk_dev;

    virtio_dev_init(&virtio_dev_blk, &pci_dev_blk, 1);
    virtio_block_dev_init(&blk_dev, &virtio_dev_blk);

    // Test virtio net
    pci_dev_t pci_dev_net = {.bus = 0x0, .dev = 0x4, .fun = 0x0};
    virtio_dev_t virtio_dev_net;
    virtio_net_dev_t *net_dev = (virtio_net_dev_t*)kmalloc(sizeof(virtio_net_dev_t));

    virtio_dev_init(&virtio_dev_net, &pci_dev_net, 2);
    virtio_net_dev_init(net_dev, &virtio_dev_net);
    
    virtio_net_init(net_dev);

    //kprintf("Int pin of virtio blk dev: %d\n", pci_intr_pin(&pci_dev_blk));
    //kprintf("Int pin of virtio net dev: %d\n", pci_intr_pin(&pci_dev_net));

    void **page = (void**)kmalloc(4096);
    struct mp_ct_hdr *hdr = mp_check_ct(mp_search_fps());
    mp_ct_entries(hdr, page);
    mp_ct_extended_entries(hdr, page);

#if 0
    kprintf("Entry count: %d\n", hdr->entry_count);

    for(int i = 0; i < hdr->entry_count; i++)
    {
        u8 *ep = page[i];
        if(*ep == 0)
            kprintf("APIC ID: %d\n", *(ep+1));
    } 
#endif

    struct mp_ct_io_interrupt_entry *pit_entry = mp_ct_find_pit(hdr);

    struct mp_ct_io_apic_entry *ioapic_entry = mp_ct_find_ioapic(hdr);
    //kprintf("IOAPIC base: %h\n", ioapic_entry->io_apic_mm_addr);

    // Redirect PIT interrupt
    u64 redirection_entry = INTR_NUM_PIT;
    redirection_entry |= (u64)ioapic_entry->io_apic_id << 56;
    ioapic_redirect(ioapic_entry->io_apic_mm_addr, pit_entry->dst_io_apic_intin, redirection_entry);


    // Print info needed for redirection entry to react to virtio interrupt
    struct mp_ct_io_interrupt_entry *ectio = mp_ct_find_virtio(hdr, 4);
    kprintf("Virtio redirection entry: %h\n", ectio->dst_io_apic_intin);

    // NOTE: We map device 4 here (net) but device 3 (block) maps to the same IOAPIC entry
    //       therefore we map the whole virtio device space for now

    // Redirect virtio int net handler
    u64 virtio_redirection_entry = INTR_NUM_VIRT_NET;
    virtio_redirection_entry |= (u64)ioapic_entry->io_apic_id << 56;
    ioapic_redirect(ioapic_entry->io_apic_mm_addr, ectio->dst_io_apic_intin, virtio_redirection_entry);

    // Enable syscalls
    syscalls_setup();

#if 0
    // Setup paging
    struct page_table *pt = (struct page_table*)align(kmalloc(4096), 4096);
    bzero((u8*)pt, sizeof(struct page_table));

    // Map kernel but this time make it user accessible
    paging_map_range(pt, 0, 0, 
        PAGE_PRESENT | PAGE_USER | PAGE_WRITABLE, 
        align(kernel_limit_addr, 4096),
        4096);

    // Load new pt
    //paging_activate(pt);

    // Switch to user mode
    struct interrupt_context ctx;
    ctx.rip = (u64)user_func;
    ctx.cs = (4 << 3) | 3;
    ctx.rflags = 0x202;
    ctx.rsp = (u64)user_stack;
    ctx.ds = (3 << 3) | 3;
#endif

    intr_setup();
    pic_disable();
    intr_enable();

    kclear();

    u8 mac[6];
    virtio_net_dev_mac(net_dev, (u8*)&mac);
    kprintf("MAC: %h:%h:%h:%h:%h:%h \n", mac[0], mac[1], mac[2], 
                            mac[3], mac[4], mac[5]);

    u8 sip[] = {10, 0, 2, 15};
    u8 dip[] = {10, 0, 2, 2};

#if 0
    u8 *frame = arp_ipv4_craft_packet(mac, sip, dip);

    virtio_net_dev_send(net_dev, (u8*)frame, sizeof(struct eth_head) + sizeof(struct arp_head));
    frame = arp_ipv4_craft_packet(mac, sip, dip);
    virtio_net_dev_send(net_dev, (u8*)frame, sizeof(struct eth_head) + sizeof(struct arp_head));

    // Recv eth-arp packet
    u8* read_frame = NULL;
    virtio_net_dev_recv(net_dev, (u8**)&read_frame);
    virtio_net_dev_recv(net_dev, (u8**)&read_frame);
    // Print
    kprintf("ETH-ARP: ");
    for(size_t i = 0; i < sizeof(struct eth_head) + sizeof(struct arp_head); i++)
    {
        kprintf("%h ", read_frame[i]);
    }
    kprintf("\n");
#endif

#if 1
    u8 router_mac[] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

    u8 *frame = udp_ipv4_craft_packet(mac, router_mac, 
                          sip, 
                          dip, 
                          4444, 
                          4444, 
                          (u8*)"Hello, World!", 
                          sizeof("Hello, World!"));
    
    virtio_net_dev_send(net_dev, (u8*)frame, 
                        sizeof(struct eth_head) + 
                        sizeof(struct ipv4_head) + 
                        sizeof(struct udp_head) + 
                        sizeof("Hello, World!"));
    
#endif

#if 0
    // Test fs...
    // and trigger virtio interrupts
    struct fs fs;
    fs_init(&fs, &blk_dev, true); 
    
    fs_mk(&fs, "/", "File", FS_TYPE_FILE);
    i64 handle = fs_handle(&fs, "/File");
 
    
    fs_wrfl(&fs, handle, (u8*)"Content data...", sizeof("Content data..."));  
    
    fs_seek(&fs, handle, 2);

    char data[64] = {0};
    fs_refl(&fs, handle, (u8*)data, 16);
    
    kprintf("%s\n", data);
#endif

#if 0    
    lapic_t la = lapic_init(0xF1, 0xF2, 0xF3, 0xF4);

    lapic_timer_init(la, 0xF2, true, 1000000, 6);

    // Artificial delay
    for(i64 i = 0; i < 500000000; i++)
    {
        volatile int j = 0;
        j++;
    }

    lapic_timer_deinit(la);
    
    bool res = lapic_boot_ap(lapic_fetch(), 1);
    kprintf("RES: %d\n", res);

    // Disable PIT after were done
    ioapic_mask(ioapic_entry->io_apic_mm_addr, pit_entry->dst_io_apic_intin, 1);
#endif

    //switch_context(&ctx);

    // Wait for interrupts
    while(1) 
    {
        __asm__ volatile("hlt");
    }
}
