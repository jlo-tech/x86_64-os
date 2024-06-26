#include <fs/fs.h>

void fs_init(struct fs *fs, virtio_blk_dev_t *blk_dev)
{
    // Save block device pointer
    fs->blk_dev = blk_dev;
    // Cache superblock
    virtio_block_dev_read(fs->blk_dev, 0, (u8*)&fs->sb_cache, 1);
    // Initialize if empty
    if(fs->sb_cache.disk_size == 0 || fs->sb_cache.num_blocks == 0)
    {
        fs->sb_cache.disk_size = fs->blk_dev->size;
        fs->sb_cache.block_map_size = (((fs->sb_cache.disk_size - 1) % 512) == 0) ?
                                            (fs->sb_cache.disk_size / 512) :
                                            (fs->sb_cache.disk_size / 512) + 1;
        fs->sb_cache.num_blocks = 0;
        // Alloc root dir block
        fs->sb_cache.root_dir_inode_index = fs_alloc_block(fs);
        // Zero out padding, to prevent data leak
        bzero((u8*)&fs->sb_cache.padding, 480);
        // Write back to disk
        virtio_block_dev_write(fs->blk_dev, 0, (u8*)&fs->sb_cache, 1);
    }
}

// Allocate block on disk
u64 fs_alloc_block(struct fs *fs)
{
    u64 bc[64]; // Bitmap cache
    // Skip superblock 
    for(u64 i = 1; i < 1 + fs->sb_cache.block_map_size; i++)
    {
        // Read bitmap block
        virtio_block_dev_read(fs->blk_dev, i, (u8*)&bc, 1);
        // Check if we can find a free block
        for(int j = 0; j < 64; j++)
        {
            if(bc[j] != 0xFFFFFFFFFFFFFFFF)
            {
                for(int k = 0; k < 64; k++)
                {
                    if(((bc[j] >> k) & 1) == BLOCK_FREE)
                    {
                        // Calc index of free block (keep superblock and map itself in mind)
                        u64 block_index = (1 + fs->sb_cache.block_map_size) + (512 * (i-1) + 64 * j + k);
                        // Check that we don't run out of disk space
                        if(block_index > fs->sb_cache.disk_size)
                        {
                            return FS_ERROR;
                        }
                        // Mark block as used
                        bc[j] = bc[j] | (BLOCK_USED << k);
                        // Increase block count 
                        // TODO: Currently only cache may write back sb or remove num_blocks field
                        fs->sb_cache.num_blocks++;
                        // Write bitmap back to disk
                        virtio_block_dev_write(fs->blk_dev, i, (u8*)&bc, 1);
                        // Return allocated block
                        return block_index;
                    }
                }
            }
        }
    }
    // No block found
    return FS_ERROR;
}

// Release block on disk
u64 fs_free_block(struct fs *fs, u64 block_index)
{
    // Bounds check
    if(block_index > fs->sb_cache.disk_size || block_index < (1 + fs->sb_cache.block_map_size))
    {
        return FS_ERROR;
    }
    // Bitmap cache
    u64 bc[64];
    // Calculate position
    u64 index_from_base = block_index - (1 + fs->sb_cache.block_map_size);
    // Read bitmap 
    virtio_block_dev_read(fs->blk_dev, 1 + (index_from_base / 512), (u8*)&bc, 1);
    // Mark as free
    u64 qword_index = (index_from_base % 512) / 64;
    bc[qword_index] = bc[qword_index] & ~(BLOCK_USED << ((index_from_base % 512) % 64));
    // Write back
    virtio_block_dev_write(fs->blk_dev, 1 + (index_from_base / 512), (u8*)&bc, 1);
    // Return on success
    return block_index;
}

static u64 fs_load_inode(struct fs *fs, u64 inode_index, struct inode *local)
{
    if(inode_index > fs->sb_cache.disk_size || inode_index < (1 + fs->sb_cache.block_map_size))
    {
        return FS_ERROR;
    }
    
    virtio_block_dev_read(fs->blk_dev, inode_index, (u8*)local, 1);
    
    return 0;
}

static u64 fs_write_inode(struct fs *fs, u64 inode_index, struct inode *local)
{
    if(inode_index > fs->sb_cache.disk_size || inode_index < (1 + fs->sb_cache.block_map_size))
    {
        return FS_ERROR;
    }
    
    virtio_block_dev_write(fs->blk_dev, inode_index, (u8*)local, 1);

    return 0;
}

static i64 fs_which_layer_inode(i64 offset)
{
    if(offset > FS_NUM_BLOCKS * 64 * 512)
    {
        return FS_ERROR;
    }
    return offset / (64 * 512);
}

static i64 fs_which_block_inode(i64 layer, i64 offset)
{
    if(offset > FS_NUM_BLOCKS * 64 * 512 || layer > (FS_NUM_BLOCKS - 1))
    {
        return FS_ERROR;
    }
    return (offset - (layer * 64 * 512)) / 512;
}

// Changes number of allocated blocks on inode
u64 fs_resize_inode(struct fs *fs, u64 inode_index, u64 new_size)
{
    // Load inode and calculate currently allocated blocks
    struct inode local;
    fs_load_inode(fs, inode_index, &local);
    // Current file size
    i64 cur_size = (i64)local.file_size;
    // Layers
    i64 cur_layer = fs_which_layer_inode(cur_size);
    i64 new_layer = fs_which_layer_inode(new_size);
    // Blocks
    i64 cur_block = fs_which_block_inode(cur_layer, cur_size);
    i64 new_block = fs_which_block_inode(new_layer, new_size);

    // Set new size
    local.file_size = new_size;
    
    // Block that contains pointers
    u64 b[64];

    // Handle simple case first
    if(new_layer == cur_layer)
    {
        // Nothing to do
        if(new_block == cur_block)
            return 0;

        // Allocate blocks
        if(new_block > cur_block)
        {
            // Block that contains pointers
            u64 bidx = local.data_blocks[cur_layer];
            // Allocate block if not already done
            if(bidx == 0)
                bidx = fs_alloc_block(fs);
                // Save newly allocated block
                local.data_blocks[cur_layer] = bidx;

            // Read current block from disk
            virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
            // Alloc new blocks
            for(i64 i = cur_block; i < new_block; i++)
            {
                b[i] = fs_alloc_block(fs);
            }
            // Write back block that contains new allocations
            virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);
        }

        // Free blocks
        if(new_block < cur_block)
        {
            // Block that contains pointers
            u64 bidx = local.data_blocks[cur_layer];
            // Read current block from disk
            virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
            // Free blocks
            for(i64 i = new_block; i < cur_block; i++)
            {
                fs_free_block(fs, b[i]);
                b[i] = 0; // Mark as free
            }
            // Write back block that contains new frees
            virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);
        }
    }

    // Allocate 
    if(new_layer > cur_layer)
    {
        // Loop over layers
        for(i64 i = cur_layer; i <= new_layer; i++)
        {
            // Block id
            u64 bidx = local.data_blocks[i];

            // Allocate block if not already done
            if(bidx == 0)
            {
                bidx = fs_alloc_block(fs);
                // Save newly allocated block
                local.data_blocks[i] = bidx;
            }

            // Fill first block
            if(i == cur_layer)
            {
                // Read current block from disk
                virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
                // Alloc blocks
                for(i64 j = cur_block; j < 64; j++)
                {
                    b[j] = fs_alloc_block(fs);
                }
                // Write back block that contains new allocations
                virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);
            }

            // Fill following "full" blocks
            if(i > cur_layer && i < new_layer)
            {
                // Read current block from disk
                virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
                // Alloc blocks
                for(i64 j = 0; j < 64; j++)
                {
                    b[j] = fs_alloc_block(fs);
                }
                // Write back block that contains new allocations
                virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);
            }

            // Fill last layer block
            if(i == new_layer)
            {
                // Read current block from disk
                virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
                // Alloc blocks
                for(i64 j = 0; j < new_block; j++)
                {
                    b[j] = fs_alloc_block(fs);
                }
                // Write back block that contains new allocations
                virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);
            }
        }
    }
    
    // Free
    if(new_layer < cur_layer)
    {
        // Loop over layers
        for(i64 i = cur_layer; i >= new_layer; i--)
        {
            // Block id
            u64 bidx = local.data_blocks[i];

            // Empty last block
            if(i == cur_layer)
            {
                // Read current block from disk
                virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
                // Free blocks
                for(i64 j = 0; j < cur_block; j++)
                {
                    fs_free_block(fs, b[j]);
                    b[j] = 0; // Mark as free
                }
                // Write back block that contains new frees
                virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);

                // Free block that contains pointers to other blocks
                fs_free_block(fs, local.data_blocks[i]);
                local.data_blocks[i] = 0; // Mark as free
            }

            if(i < cur_layer && i > new_layer)
            {
                // Read current block from disk
                virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
                // Free all blocks
                for(i64 j = 0; j < 64; j++)
                {
                    fs_free_block(fs, b[j]);
                    b[j] = 0; // Mark as free
                }
                // Write back block that contains new frees
                virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);

                // Free block that contains pointers to other blocks
                fs_free_block(fs, local.data_blocks[i]);
                local.data_blocks[i] = 0; // Mark as free
            }

            // Empty first block
            if(i == new_layer)
            {
                // Read current block from disk
                virtio_block_dev_read(fs->blk_dev, bidx, (u8*)&b, 1);
                // Free blocks
                for(i64 j = 63; j >= new_block; j--)
                {
                    fs_free_block(fs, b[j]);
                    b[j] = 0; // Mark as free
                }
                // Write back block that contains new frees
                virtio_block_dev_write(fs->blk_dev, bidx, (u8*)&b, 1);
            }
        }
    }
    // Write back changed inode
    fs_write_inode(fs, inode_index, &local);
    // No error
    return 0;
}

// Get inode by path
static u64 fs_query_inode(struct fs *fs, char *path);

// Create/Delete inodes by creating removing dir entries
static u64 fs_create_inode(struct fs *fs, char *name, char *path);
static u64 fs_delete_inode(struct fs *fs, char *name, char *path);

// Write/Read contents into files
static u64 fs_write_content(struct fs *fs, char *path, u64 off, u64 len, u8 *data);
static u64 fs_read_content(struct fs *fs, char *path, u64 off, u64 len, u8 *data);