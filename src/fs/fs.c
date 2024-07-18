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
 *
 * @return Returns index of the newly allocated block
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
 *
 * @param index Index of the block that shall be marked as free
 *
 * @return Index of the freed block or error
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
 * Adds a freshly allocated block to an inode
 *
 * @param inode Inode that is to be modified
 * @param block_index Address / Index of the block that should be allocted
 *
 * @return Index of the newly allocated block or error
 */
i64 __fs_inode_alloc(struct fs *fs, struct inode *inode, i64 block_index)
{
    const i64 mask  = ((i64)511);
    const i64 shift = ((i64)9);
    
    i64 off, stub, next;

    // Local pointer
    i64 *loc = (i64*)&tmp;  

    // Current block index
    i64 bx = inode->data_tree;

    // Traverse tree
    for(i64 i = 0; i < 4; i++)
    {
        // Load current layer block
        fs_read(fs, bx, (u8*)&tmp);

        // Get pointer to next layer
        off = shift * i;
        stub = (block_index & (mask << off)) >> off;
        next = loc[stub];

        // Allocate new block if not already done
        if(next == 0)
        {
            next = fs_alloc(fs);
            if(next == FS_ERROR)
                return FS_ERROR;
            // Reload block since fs_alloc modifies tmp
            fs_read(fs, bx, (u8*)&tmp);
            // Write newly allocated block into tree
            loc[stub] = next;
            // Write back layer block to disk
            fs_write(fs, bx, (u8*)&tmp);
            // Zero out new block
            bzero((u8*)&tmp, FS_BLOCK_SIZE);
            fs_write(fs, next, (u8*)&tmp);         
        }

        // Continue with next layer
        bx = next;
    }

    return bx;
}

i64 fs_inode_alloc(struct fs *fs, i64 inode_index, i64 block_index)
{
    // Pointer to inode
    struct inode *inode = (struct inode*)&tmp;
    // Read inode from disk
    fs_read(fs, inode_index, (u8*)&tmp);
    // Check if there are already allocated blocks
    if(inode->data_tree == 0)
    {
        // Allocate new block
        i64 n = fs_alloc(fs);
        // Clear new block
        bzero((u8*)&tmp, FS_BLOCK_SIZE);
        fs_write(fs, n, (u8*)&tmp);
        // Reload inode
        fs_read(fs, inode_index, (u8*)&tmp);
        // Set new tree root
        inode->data_tree = n;
        // Write back inode
        fs_write(fs, inode_index, (u8*)&tmp);
    }
    // Do allocation
    return __fs_inode_alloc(fs, inode, block_index);
}

/**
 * Removes block from inode and marks it as free 
 */
i64 __fs_inode_free(struct fs *fs, struct inode *inode, i64 block_index)
{
    const i64 mask  = ((i64)511);
    const i64 shift = ((i64)9);
  
    i64 off, stub, next;  

    // Local pointer
    i64 *loc = (i64*)&tmp;  

    // Current block index
    i64 bx = inode->data_tree;

    // Traverse tree
    for(i64 i = 0; i < 4; i++)
    {
        // Load current layer block
        fs_read(fs, bx, (u8*)&tmp);

        // Get pointer to next layer
        off = shift * i;
        stub = (block_index & (mask << off)) >> off;
        next = loc[stub];
        
        // Allocate new block if not already done
        if(next == 0)
        {
            // Block was not allocated
            return FS_ERROR;        
        }

        // Continue with next layer
        bx = next;
    }

    // Remove from inode
    loc[stub] = 0;
    // Mark block as free 
    fs_free(fs, bx);

    return bx;  
}

i64 fs_inode_free(struct fs *fs, i64 inode_index, i64 block_index)
{
    // Pointer to inode
    struct inode *inode = (struct inode*)&tmp;
    // Read inode from disk
    fs_read(fs, inode_index, (u8*)&tmp);
    // Check if data was already allocated
    if(inode->data_tree == 0)
        return FS_ERROR;
    // Do allocation
    return __fs_inode_free(fs, inode, block_index);
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

    // Set root dir inode
    fs->sb_cache.root_dir_inode_index = r;

    // Write super block to start of the disk
    return fs_write(fs, 0, (u8*)&fs->sb_cache);
}

