#include <fs/fs.h>

// Block cache to save stack space
static u8 tmp[FS_BLOCK_SIZE];

/**
 * Writes sectors to disk
 *
 * @param index Disk sector index on disk
 * @param data Data
 * @param len Length of data in sectors
 *
 * @return Success of operation
 */
bool fs_write_sectors(struct fs *fs, i64 index, u8 *data, i64 len)
{
    // Bounds check
    if((index * FS_SECTOR_SIZE) >= fs->sb_cache.disk_size || index < 0)
    {
        return false;
    }
    // Work
    return virtio_block_dev_write(fs->blk_dev, index, data, len); 
}

/**
 * Writes multiple fs blocks to disk
 *
 * @param index Position on disk where index marks the (index)th fs block
 * @param len Length of data in fs blocks (aka number of 4k chunks)
 */
bool fs_write_many(struct fs *fs, i64 index, u8 *data, i64 len)
{
    return fs_write_sectors(fs, index * FS_FACTOR, data, len * FS_FACTOR);
}

/**
 * Writes single fs block to disk
 */
bool fs_write(struct fs *fs, i64 index, u8 *data)
{
    return fs_write_many(fs, index, data, 1);
}

/**
 * Reads sectors from disk
 *
 * @param index Disk sector index on disk
 * @param data Data
 * @param len Length of data in sectors
 *
 * @return Success of operation
 */
bool fs_read_sectors(struct fs *fs, i64 index, u8* data, i64 len)
{
    // Bounds check
    if((index * FS_SECTOR_SIZE) >= fs->sb_cache.disk_size || index < 0)
    {
        return false;
    }
    // Work
    return virtio_block_dev_read(fs->blk_dev, index, data, len);
}

/**
 * Reads multiple fs blocks from disk
 *
 * @param index Position on disk where index marks the (index)th fs block
 * @param len Length of data in fs blocks (aka number of 4k chunks)
 */
bool fs_read_many(struct fs *fs, i64 index, u8 *data, i64 len)
{
    return fs_read_sectors(fs, index * FS_FACTOR, data, len * FS_FACTOR);
}

/**
 * Reads single fs block from disk
 */
bool fs_read(struct fs *fs, i64 index, u8 *data)
{
    return fs_read_many(fs, index, data, 1);
}

/**
 * Allocate fs block
 */
i64 fs_alloc(struct fs *fs)
{
    // Err
    bool r;

    // Local pointer to tmp
    i64 *loc = (i64*)&tmp;

    // Iterate over bitmap
    for(i64 i = 0; i < (fs->sb_cache.bitmap_size / FS_BLOCK_SIZE); i++)
    {
        // Read bitmap block
        r = fs_read(fs, 1 + i, tmp);    // Skip superblock
        if(!r)
            return false;

        // Find free block
        for(i64 j = 0; j < (FS_BLOCK_SIZE >> 3); j++)
        {
            // Check if there is a free entry
            if(loc[j] == (i64)0xFFFFFFFFFFFFFFFF)
            {
                continue;
            }

            // Get next free entry
            i64 pos = 0;
            if(loc[j] != 0)
            {
                pos = __builtin_ffsll(~loc[j]) - 1;
            }

            // Mark entry as used
            loc[j] |= (((i64)1) << pos);

            // Write back
            r = fs_write(fs, 1 + i, tmp);
            if(!r)
                return false;

            // Return fs block index
            return 1 + 
                   (fs->sb_cache.bitmap_size / FS_BLOCK_SIZE) + 
                   (i * FS_BLOCK_SIZE << 3) + 
                   (j * 64) + pos;
        }
    } 
    // Out of disk space
    return FS_ERROR;
}

/**
 * Free fs block 
 */
i64 fs_free(struct fs *fs, i64 index)
{
    bool r; // Err

    i64 ind = (index - 1) - (fs->sb_cache.bitmap_size / FS_BLOCK_SIZE);
    
    i64 block = ind / (FS_BLOCK_SIZE << 3);
    i64 word  = ind / 64;
    i64 pos   = ind % 64;

    // Load block
    r = fs_read(fs, block + 1, (u8*)&tmp);    
    // Error check
    if(!r)
        return FS_ERROR;

    // Mark as free again
    i64 *loc = (i64*)&tmp; 
    loc[word] = loc[word] & ~(((i64)1) << pos);
    
    // Write back
    r = fs_write(fs, block + 1, (u8*)&tmp);
    // Error check
    if(!r)
        return FS_ERROR;

    return index;
}


/**
 * Calculates the size of the needed bitmap in bytes
 */
i64 __bitmap_size(i64 disk_size, i64 block_size)
{    
    i64 bits_per_block = block_size << 3;
    i64 total_blocks = disk_size / block_size;

    if((disk_size % block_size) != 0)
        total_blocks++;

    i64 bitmap_size = total_blocks / bits_per_block;
    
    if(total_blocks % bits_per_block)
        bitmap_size++;

    return bitmap_size * FS_BLOCK_SIZE;
}

/**
 * Init filesystem 
 */
bool fs_init(struct fs *fs, virtio_blk_dev_t *blk_dev)
{
    fs->blk_dev = blk_dev;

    fs->sb_cache.disk_size = (blk_dev->size * FS_SECTOR_SIZE); 
    fs->sb_cache.bitmap_size = __bitmap_size(fs->sb_cache.disk_size, FS_BLOCK_SIZE);

    // Alloc root inode 
    i64 r = fs_alloc(fs);
    if(r == FS_ERROR)
        return false; 
    // Zero out root inode
    bzero((u8*)&tmp, FS_BLOCK_SIZE);
    if(!fs_write(fs, r, (u8*)&tmp))
        return false;

    // Write super block to start of the disk
    return fs_write(fs, 0, (u8*)&fs->sb_cache);
}

