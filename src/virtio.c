#include <virtio.h>

#define BARRIER asm("mfence");

static u16 virtq_size(u16 qs)
{
    return align(sizeof(struct virtq_desc) * qs + 
                 sizeof(u16) * 2 + qs * sizeof(u16), 4096) + 
                 sizeof(u16) * 2 + qs * sizeof(struct virtq_used_elem);
}

extern struct framebuffer fb;

/* Initialize one of a device's virtqs */
static bool virtio_create_queue(virtio_dev_t *virtio_dev, u16 queue_num)
{
    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev->pci_dev, 0);
    
    // Check error
    if(iobase == 0xFFFFFFFF)
        return false;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    // Select right queue
    outw(iobase + VIRTIO_HEADER_QUEUE_SELECT, queue_num);
    
    // Get size of queue
    u16 queue_elems = inw(iobase + VIRTIO_HEADER_QUEUE_SIZE);
    
    // Save for later
    virtio_dev->virtqs[queue_num].elems = queue_elems;

    // Check if queue exists
    if(queue_elems == 0)
        return false;

    // Size for total virtq (NOTE: device can have multiple virtqs)
    u16 queue_size = virtq_size(queue_elems);

    // Allocate memory
    virtio_dev->virtqs[queue_num].space = kmalloc(queue_size);

    // Check error
    if(virtio_dev->virtqs[queue_num].space == -1)
        return false;

    // Zero memory
    bzero((u8*)virtio_dev->virtqs[queue_num].space, queue_size);

    // Write aligned address back to queue_address 
    outd(iobase + VIRTIO_HEADER_QUEUE_ADDRESS, align((u64)virtio_dev->virtqs[queue_num].space, 4096) / 4096);

    // Init
    virtio_dev->virtqs[queue_num].dptr = 0;
    // Fill pointers to structs in memory
    virtio_dev->virtqs[queue_num].desc  = (struct virtq_desc*)align(virtio_dev->virtqs[queue_num].space, 4096);
    virtio_dev->virtqs[queue_num].avail = (struct virtq_avail*)(align(virtio_dev->virtqs[queue_num].space, 4096) + sizeof(struct virtq_desc) * queue_elems);
    virtio_dev->virtqs[queue_num].used  = (struct virtq_used*)align(align(virtio_dev->virtqs[queue_num].space, 4096) + sizeof(struct virtq_desc) * queue_elems + sizeof(u16) * 2 + queue_elems * sizeof(u16), 4096);

    return true;
}

bool virtio_dev_init(virtio_dev_t *virtio_dev, pci_dev_t *pci_dev, u16 num_queues)
{
    // Save for later
    virtio_dev->num_queues = num_queues;

    // Save pointer to PCI dev
    virtio_dev->pci_dev = pci_dev;

    // Allocate space for queues
    virtio_dev->virtqs = (struct virtq*)kmalloc(num_queues * sizeof(struct virtq));

    // Check that mem is available
    if(((i64)virtio_dev->virtqs) == -1)
        return false;

    for(u16 i = 0; i < num_queues; i++)
    {
        virtio_create_queue(virtio_dev, i);
    }

    // No error
    return true;
}

bool virtio_dev_deinit(virtio_dev_t *virtio_dev)
{
    // Free all virtqs
    for(u16 i = 0; i < virtio_dev->num_queues; i++)
    {
        kfree(virtio_dev->virtqs[i].space);
    }

    // Free virtq pointer array
    kfree((i64)virtio_dev->virtqs);

    return true;
}

bool virtio_dev_reset(virtio_dev_t *virtio_dev)
{
    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev->pci_dev, 0);
    
    // Check error
    if(iobase == 0xFFFFFFFF)
        return false;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    // Actual reset
    outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 0);

    // No error
    return true;
}

/*
 * queue_num: Number of the queue to insert descriptors into (queue_num != num_queue !!!)
*/
bool virtio_deploy(virtio_dev_t *virtio_dev, u16 queue_num, struct virtq_desc *descriptors, u16 num_descriptors)
{
    struct virtq *vq = &virtio_dev->virtqs[queue_num];

    u16 base_idx = vq->dptr;

    // Copy first descriptor
    vq->desc[vq->dptr].addr  = descriptors[0].addr;
    vq->desc[vq->dptr].len   = descriptors[0].len;
    vq->desc[vq->dptr].flags = descriptors[0].flags;
    
    // Are other descriptors incoming
    if(num_descriptors > 1)
    {
        vq->desc[vq->dptr].flags |= VRING_DESC_F_NEXT;
        vq->desc[vq->dptr].next = vq->dptr + 1;
    }

    // Copy other descriptors and chain them automatically
    for(u16 i = 1; i < num_descriptors; i++)
    {
        vq->desc[(vq->dptr + i) % vq->elems].addr  = descriptors[i].addr;
        vq->desc[(vq->dptr + i) % vq->elems].len   = descriptors[i].len;
        vq->desc[(vq->dptr + i) % vq->elems].flags = descriptors[i].flags;
        if(i < num_descriptors - 1)
        {
            vq->desc[(vq->dptr + i) % vq->elems].flags |= VRING_DESC_F_NEXT;
            vq->desc[(vq->dptr + i) % vq->elems].next = (vq->dptr + i + 1) % vq->elems;
        }
    }

    BARRIER

    // Make descriptors available
    vq->avail->ring[vq->avail->idx % vq->elems] = vq->dptr;
    // Sync mem
    BARRIER
    // Make available descriptors visible to device
    vq->avail->idx++;
    // Sync mem
    BARRIER

    // Update dptr
    vq->dptr = (vq->dptr + num_descriptors) % vq->elems;

    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev->pci_dev, 0);
    
    // Check error
    if(iobase == 0xFFFFFFFF)
        return false;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    // Notify device
    outw(iobase + VIRTIO_HEADER_QUEUE_NOTIFY, 0);

    return true;
}

#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_SCSI_CMD 2
#define VIRTIO_BLK_T_SCSI_CMD_OUT 3
#define VIRTIO_BLK_T_FLUSH 4
#define VIRTIO_BLK_T_FLUSH_OUT 5
#define VIRTIO_BLK_T_BARRIER 0x80000000

#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

struct virtio_block_req
{
    u32 type;
    u32 ioprio;
    u64 sector;
    char data[512];
    u8 status;
}__attribute__((packed));

bool virtio_block_dev_init(virtio_dev_t *virtio_dev)
{
    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev->pci_dev, 0);

    // Check error
    if(iobase == 0xFFFFFFFF)
        return false;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    vga_printf(&fb, "IOBASE: %h\n", iobase);

    // Read size of disk
    u64 dev_size = (((u64)ind(iobase + 0x18)) << 32) | ((u64)ind(iobase + 0x14));
    vga_printf(&fb, "BLK DEV SIZE: %h\n", dev_size);

    // Reset device
    virtio_dev_reset(virtio_dev);

    // Unlock device
    outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 3);

    // Create virtqueue
    virtio_create_queue(virtio_dev, 0);

    // Currently no advanced features
    outd(iobase + VIRTIO_HEADER_GUEST_FEATURES, 0);

    // Device ready
    outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 7);

    // Submit write to device
    struct virtio_block_req *blkreq = align(kmalloc(4096), 4096);
    blkreq->type = VIRTIO_BLK_T_OUT;
    blkreq->ioprio = 0;
    blkreq->sector = 0;

    char *data = align(kmalloc(4096), 4096);
    bzero(data, 512);
    data[0] = 1;
    data[1] = 2;

    u8 *status = align(kmalloc(4096), 4096);
    *status = 0xff;

    // Virtq deploy

    struct virtq_desc desc_arr[3];

    desc_arr[0].addr = (u64)blkreq;
    desc_arr[0].len = 16;
    desc_arr[0].flags = 0;

    desc_arr[1].addr = (u64)data;
    desc_arr[1].len = 512;
    desc_arr[1].flags = 0;

    desc_arr[2].addr = (u64)status;
    desc_arr[2].len = 1;
    desc_arr[2].flags = VRING_DESC_F_WRITE;

    virtio_deploy(virtio_dev, 0, desc_arr, 3);
    while(*status == 0xff);

    // IOError code is due to conflicting sector sizes of guest and host (see https://bugzilla.redhat.com/show_bug.cgi?id=1738839)

    // Read back //

    struct virtio_block_req *blkin = align(kmalloc(4096), 4096);
    blkin->type = VIRTIO_BLK_T_IN;
    blkin->ioprio = 0;
    blkin->sector = 0;

    char *din = align(kmalloc(4096), 4096);
    bzero(din, 512);

    u8 *sin = align(kmalloc(4096), 4096);
    *sin = 0xff;

    struct virtq_desc desc_arr_in[3];

    desc_arr_in[0].addr = (u64)blkin;
    desc_arr_in[0].len = 16;
    desc_arr_in[0].flags = 0;

    desc_arr_in[1].addr = (u64)din;
    desc_arr_in[1].len = 512;
    desc_arr_in[1].flags = VRING_DESC_F_WRITE;

    desc_arr_in[2].addr = (u64)sin;
    desc_arr_in[2].len = 1;
    desc_arr_in[2].flags = VRING_DESC_F_WRITE;

    virtio_deploy(virtio_dev, 0, desc_arr_in, 3);
    while(*sin == 0xff);

    vga_printf(&fb, "VIO Data: %h %h\n", din[0], din[1]);

    return true;
}