#include <virtio_blk.h>

bool virtio_block_dev_init(virtio_blk_dev_t *blk_dev, virtio_dev_t *virtio_dev)
{
    // Save for later
    blk_dev->virtio_dev = virtio_dev;

    // Get virtio device's io offset
    u32 iobase = pci_bar(virtio_dev->pci_dev, 0);

    // Check error
    if(iobase == 0xFFFFFFFF)
        return false;

    // Get only address from bar
    iobase &= 0xFFFFFFFC;

    // Read size of disk
    blk_dev->size = (((u64)ind(iobase + 0x18)) << 32) | ((u64)ind(iobase + 0x14));
   
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

    return true;
}

bool virtio_block_dev_write(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data, u64 num_sectors)
{
    // Submit write to device
    struct virtio_block_req_hdr *blkhdr = (struct virtio_block_req_hdr*)align(kmalloc(4096), 4096);
    blkhdr->type = VIRTIO_BLK_T_OUT;
    blkhdr->ioprio = 0;
    blkhdr->sector = sector;

    u8 *status = (u8*)align(kmalloc(4096), 4096);
    *status = 0xff;

    struct virtq_desc desc_arr[3];

    desc_arr[0].addr = (u64)blkhdr;
    desc_arr[0].len = 16;
    desc_arr[0].flags = 0;

    desc_arr[1].addr = (u64)data;
    desc_arr[1].len = 512 * num_sectors;
    desc_arr[1].flags = 0;

    desc_arr[2].addr = (u64)status;
    desc_arr[2].len = 1;
    desc_arr[2].flags = VRING_DESC_F_WRITE;

    bool ret = virtio_deploy(blk_dev->virtio_dev, 0, desc_arr, 3);
    if(!ret)
        return false;

    // Wait for completion
    while(*status == 0xff);

    // IOError code is due to conflicting sector sizes of guest and host (see https://bugzilla.redhat.com/show_bug.cgi?id=1738839)

    // Free resources
    kfree((i64)blkhdr);
    kfree((i64)status);

    return true;
}

bool virtio_block_dev_read(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data, u64 num_sectors)
{
    struct virtio_block_req_hdr *blkhdr = (struct virtio_block_req_hdr*)align(kmalloc(4096), 4096);
    blkhdr->type = VIRTIO_BLK_T_IN;
    blkhdr->ioprio = 0;
    blkhdr->sector = sector;

    u8 *status = (u8*)align(kmalloc(4096), 4096);
    *status = 0xff;

    struct virtq_desc desc_arr[3];

    desc_arr[0].addr = (u64)blkhdr;
    desc_arr[0].len = 16;
    desc_arr[0].flags = 0;

    desc_arr[1].addr = (u64)data;
    desc_arr[1].len = 512 * num_sectors;
    desc_arr[1].flags = VRING_DESC_F_WRITE;

    desc_arr[2].addr = (u64)status;
    desc_arr[2].len = 1;
    desc_arr[2].flags = VRING_DESC_F_WRITE;

    bool ret = virtio_deploy(blk_dev->virtio_dev, 0, desc_arr, 3);
    if(!ret)
        return false;

    // Wait for completion
    while(*status == 0xff);

    // Free resources
    kfree((i64)blkhdr);
    kfree((i64)status);

    return true;
}

/* Write one block */
inline bool virtio_block_dev_write_block(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data)
{
    return virtio_block_dev_write(blk_dev, sector, data, 1);
}

/* Read one block */
inline bool virtio_block_dev_read_block(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data)
{
    return virtio_block_dev_read(blk_dev, sector, data, 1);
}

/* Zeros out one block */
bool virtio_block_dev_zero(virtio_blk_dev_t *blk_dev, u64 sector)
{
    unsigned char data[512] = {0};
    return virtio_block_dev_write(blk_dev, sector, data, 1);
}