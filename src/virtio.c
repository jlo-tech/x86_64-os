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
i64 virtio_create_queue(struct virtq *virtq, pci_dev_t *virtio_dev, u16 queue_num)
{
    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev, 0);
    
    // Check error
    if(iobase == 0xFFFFFFFF)
        return -1;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    // Select right queue
    outw(iobase + VIRTIO_HEADER_QUEUE_SELECT, queue_num);
    
    // Get size of queue
    u16 queue_size = inw(iobase + VIRTIO_HEADER_QUEUE_SIZE);
    
    // Check if queue exists
    if(queue_size == 0)
        return -1;

    // Save num for later
    virtq->num = queue_num;

    // Save size for later
    virtq->size = queue_size;

    // Size for total virtq (NOTE: device can have multiple virtqs)
    queue_size = virtq_size(queue_size);

    // Allocate memory
    i64 buf = kmalloc(queue_size);

    // Check error
    if(buf == -1)
        return -1;

    // Zero memory
    bzero((u8*)buf, queue_size);

    // Write aligned address back to queue_address 
    outd(iobase + VIRTIO_HEADER_QUEUE_ADDRESS, align((u64)buf, 4096) / 4096);

    // Fill pointers to structs in memory
    virtq->desc  = (struct virtq_desc*)align(buf, 4096);
    virtq->avail = (struct virtq_avail*)(align(buf, 4096) + sizeof(struct virtq_desc) * virtq->size);
    virtq->used  = (struct virtq_used*)align(align(buf, 4096) + sizeof(struct virtq_desc) * virtq->size + sizeof(u16) * 2 + virtq->size * sizeof(u16), 4096);

    return buf;
}

#if 0
bool virtio_push_buffer(struct virtq *virtq, pci_dev_t *virtio_dev, void *buf, u32 len)
{
    // Next free descriptor
    u16 new_idx = virtq->avail->idx % virtq->size;

    // Fill buffer
    struct virtq_desc *desc = &virtq->desc[new_idx];
    desc->addr = (u64)buf;
    desc->len = len;

    // Mem sync
    BARRIER

    // Chaining of buffers is currenly omitted
    // TODO : Implement it

    // Make buffer visible
    virtq->avail->ring[virtq->avail->idx % virtq->size] = new_idx;

    // Mem sync
    BARRIER

    // Update index and make buffers visible
    virtq->avail->idx++;

    // Mem sync
    BARRIER

    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev, 0);
    
    // Check error
    if(iobase == 0xFFFFFFFF)
        return false;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    // Notify device
    outw(iobase + VIRTIO_HEADER_QUEUE_NOTIFY, virtq->num);

    // No error
    return true;
}

bool virtio_pull_buffer(struct virtq **virtq_arr, i32 num_queues, pci_dev_t *virtio_dev, u32 *desc_chain_idx, u32 *desc_chain_len)
{
    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev, 0);
    
    // Check error
    if(iobase == 0xFFFFFFFF)
        return false;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    // Read IRS register
    u8 isr = inb(iobase + VIRTIO_HEADER_ISR_STATUS);   

    // Check if interrupt was for this device
    if((isr & 1) == 0)
        return false;

    // Traverse queues
    for(i32 i = 0; i < num_queues; i++)
    {
        // Check on which queue the progress happens
        if(virtq_arr[i]->last_idx != virtq_arr[i]->used->idx)
        {
            *desc_chain_idx = virtq_arr[i]->used->ring[virtq_arr[i]->last_idx].id;
            *desc_chain_len = virtq_arr[i]->used->ring[virtq_arr[i]->last_idx].len;
            // Update last idx
            virtq_arr[i]->last_idx = virtq_arr[i]->used->idx;
            // Done
            break;
        }
    }

    // No error
    return true;
}
#endif

bool virtio_device_reset(pci_dev_t *virtio_dev)
{
    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev, 0);
    
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

bool virtio_block_dev_init(pci_dev_t *virtio_dev)
{
    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev, 0);

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
    virtio_device_reset(virtio_dev);

    // Unlock device
    outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 3);

    // Create virtqueue
    struct virtq vq;
    virtio_create_queue(&vq, virtio_dev, 0);

    vga_printf(&fb, "Queue Size: %h\n", virtq_size(vq.size));

    // Currently no advanced features
    outd(iobase + VIRTIO_HEADER_GUEST_FEATURES, 0);

    // Device ready
    outb(iobase + VIRTIO_HEADER_DEVICE_STATUS, 7);

    // Submit write to device
    struct virtio_block_req *blkreq = align(kmalloc(4096), 4096);
    blkreq->type = VIRTIO_BLK_T_OUT;
    blkreq->ioprio = 1;
    blkreq->sector = 0;

    char *data = align(kmalloc(4096), 4096);
    data[0] = 1;

    u8 *status = align(kmalloc(4096), 4096);
    

    //virtio_push_buffer(&vq, virtio_dev, (void*)&blkreq, sizeof(struct virtio_block_req));

    // Hardcoded virtq push
    vq.desc[0].addr = (u64)blkreq;
    vq.desc[0].len = 16;
    vq.desc[0].flags = VRING_DESC_F_NEXT;
    vq.desc[0].next = 1;

    vq.desc[1].addr = (u64)data;
    vq.desc[1].len = 512;
    vq.desc[1].flags = VRING_DESC_F_NEXT;
    vq.desc[1].next = 2;

    vq.desc[2].addr = (u64)status;
    vq.desc[2].len = 1;
    vq.desc[2].flags = VRING_DESC_F_WRITE;
    vq.desc[2].next = 0;

    BARRIER

    // Make buffer visible
    vq.avail->ring[0] = 0;
    vq.avail->ring[1] = 1;
    vq.avail->ring[2] = 2;

    // Mem sync
    BARRIER

    // Update index and make buffers visible
    vq.avail->idx = 3;

    // Mem sync
    BARRIER

    // Notify device
    outw(iobase + VIRTIO_HEADER_QUEUE_NOTIFY, 0);

    //while(blkreq->status == 0xff);
    //vga_printf(&fb, "Status %h\n", blkreq.status);

    return true;
}