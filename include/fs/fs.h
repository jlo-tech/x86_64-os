#pragma once

#include <util.h>
#include <types.h>
#include <virtio_blk.h>

#define BLOCK_FREE 0
#define BLOCK_USED 1

#define TYPE_DIRECTORY 0
#define TYPE_FILE      1

#define FS_ERROR 0xFFFFFFFFFFFFFFFF

/* 
 *  Disk Layout:
 *  
 *  + ---------------------------- |
 *  | Super Block                  |
 *  | ---------------------------- |
 *  | Block Map                    |
 *  | ---------------------------- |
 *  | Inodes & Data Blocks         |
 *  + ---------------------------- |
 */

// Data structures

struct superblock
{
    u64 disk_size;              // Size of the whole disk (in 512 byte blocks)
    u64 block_map_size;         // Size of the map which marks blocks as free/used (in blocks) (equals disk_size / 512)
    u64 num_blocks;             // Number of allocated blocks and inodes
    u64 root_dir_inode_index;   // Block index of inode of root dir
    u8 padding[480];            // Padding to fill one sector
} __attribute__((packed));

struct dir_entry
{
    u64 inode_index;            // Inode index of 0 is invalid and means entry not valid
    char file_name[120];
} __attribute__((packed));

struct inode
{
    u64 type;               // file / dir (when file: data blocks contain file content, 
                            //             when dir: data blocks contain dir_entry descriptors)
    u64 file_size;          // in bytes
    u64 l1_data_blocks[30]; // direct pointers to data blocks
    u64 l2_data_blocks[16]; // pointers to data blocks which contain pointers to data blocks
    u64 l3_data_blocks[16]; // same but with two layers of indirection
} __attribute__((packed));

// Management structures

struct fs
{
    virtio_blk_dev_t *blk_dev;  // Virtio block device
    struct superblock sb_cache; // Cached superblock
};

// Methods

// Init superblock and initialize root dir
void fs_init(struct fs *fs, virtio_blk_dev_t *blk_dev);

u64 fs_alloc_block(struct fs *fs);
u64 fs_free_block(struct fs *fs, u64 block_index);