#pragma once

#include <util.h>
#include <types.h>
#include <virtio_blk.h>

#define FS_BLOCK_FREE 0
#define FS_BLOCK_USED 1

#define FS_TYPE_DIRECTORY 0
#define FS_TYPE_FILE      1

#define FS_ERROR (-1)

// Physical sector size of disk
#define FS_SECTOR_SIZE 512

// Number of sectors that form a fs block
#define FS_FACTOR 8

// One fs block (4kb) contains eight disk sectors (512b)
#define FS_BLOCK_SIZE (FS_FACTOR * FS_SECTOR_SIZE)

// Length of names
#define FS_NAME_LEN 120

// Pad structures to given size
#define FS_PADDING(total, consumed) u8 __pad[total - consumed]

// How many elements of size "size" can fit into space "total" when "consumed" is already occupied
#define FS_FILL(total, consumed, size) ((total - consumed) / size)

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

struct superblock
{
    i64 disk_size;              // Size of the whole disk (in bytes)
    i64 bitmap_size;            // Size of the map which marks blocks as free/used (in bytes)
    i64 root_dir_inode_index;   // Block index of inode of root dir

    FS_PADDING(FS_BLOCK_SIZE, 24);

} __attribute__((packed));

struct inode
{
    i64 type;                       // File / dir (when file: data blocks contain file content, 
                                    //             when dir: data blocks contain dir_entry descriptors)
    i64 file_size;                  // In bytes
    i64 data_tree;                  // Pointer to root block of 4-level tree 

    i64 num_entries;                // Number of entries in dir, ignored in file

    i64 pos;                        // Position in file (can be changed by seek)

    FS_PADDING(FS_BLOCK_SIZE, 40);

} __attribute__((packed));

struct dir_entry
{
    i64 inode_index;                // Inode index of 0 is invalid and means entry not valid
    char file_name[FS_NAME_LEN];
} __attribute__((packed));

struct fs
{
    virtio_blk_dev_t *blk_dev;      // Virtio block device
    struct superblock sb_cache;     // Cached superblock
};

// Init superblock and initialize root dir
bool fs_init(struct fs *fs, virtio_blk_dev_t *blk_dev, bool fresh);

i64 fs_alloc(struct fs *fs);
i64 fs_free(struct fs *fs, i64 index);

i64 fs_inode_alloc(struct fs *fs, i64 inode_index, i64 block_index);
i64 fs_inode_free(struct fs *fs, i64 inode_index, i64 block_index);
i64 fs_inode_resize(struct fs *fs, i64 inode_index, i64 size);
i64 fs_inode_nth_block(struct fs *fs, i64 inode_index, i64 n);
i64 fs_inode_add_entry(struct fs *fs, i64 inode_index, char *name);
i64 fs_inode_del_entry(struct fs *fs, i64 inode_index, char *name);
i64 fs_inode_query_name(struct fs *fs, i64 inode_index, char *name);
i64 fs_inode_query(struct fs *fs, char *path);

bool fs_mk(struct fs *fs, char *path, char *name, i64 type);
bool fs_rm(struct fs *fs, char *path, char *name);
i64 fs_handle(struct fs *fs, char *path);
bool fs_size(struct fs *fs, i64 handle, i64 *size);
bool fs_seek(struct fs *fs, i64 handle, i64 off);
bool fs_wrfl(struct fs *fs, i64 handle, u8 *data, i64 len);
bool fs_refl(struct fs *fs, i64 handle, u8 *data, i64 len);

