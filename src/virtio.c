#include <virtio.h>

#define BARRIER asm("mfence");

static u16 virtq_size(u16 qs)
{
    return align(sizeof(struct virtq_desc) * qs + 
                 sizeof(u16) * 2 + qs * sizeof(u16), 4096) + 
                 sizeof(u16) * 2 + qs * sizeof(struct virtq_used_elem);
}

/* Initialize one of a device's virtqs */
bool virtio_create_queue(virtio_dev_t *virtio_dev, u16 queue_num)
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

    // Copy first descriptor
    vq->desc[vq->dptr % vq->elems].addr  = descriptors[0].addr;
    vq->desc[vq->dptr % vq->elems].len   = descriptors[0].len;
    vq->desc[vq->dptr % vq->elems].flags = descriptors[0].flags;
    
    // Are other descriptors incoming
    if(num_descriptors > 1)
    {
        vq->desc[vq->dptr % vq->elems].flags |= VRING_DESC_F_NEXT;
        vq->desc[vq->dptr % vq->elems].next = (vq->dptr + 1) % vq->elems;
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
    vq->avail->ring[vq->avail->idx % vq->elems] = vq->dptr % vq->elems;
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
