#pragma once

#include <pmm.h>
#include <types.h>
#include <virtio.h>

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

struct virtio_block_req_hdr
{
    u32 type;
    u32 ioprio;
    u64 sector;
}__attribute__((packed));

typedef struct virtio_blk_dev
{
    u64 size; // Size of disk in sectors
    virtio_dev_t *virtio_dev;
} virtio_blk_dev_t;

// Init block device
bool virtio_block_dev_init(virtio_blk_dev_t *blk_dev, virtio_dev_t *virtio_dev);
// Read/Write multiple sectors
bool virtio_block_dev_write(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data, u64 num_sectors);
bool virtio_block_dev_read(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data, u64 num_sectors);
// Read/Write single sector
bool virtio_block_dev_write_block(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data);
bool virtio_block_dev_read_block(virtio_blk_dev_t *blk_dev, u64 sector, u8 *data);
// Clear single sector
bool virtio_block_dev_zero(virtio_blk_dev_t *blk_dev, u64 sector);

