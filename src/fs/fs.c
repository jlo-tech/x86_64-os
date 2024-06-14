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
                        // TODO: Currently only cache may wb sb or remove num_blocks field
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
static u64 fs_free_block(struct fs *fs, u64 block_index);

// Allocates one additional block on inode
static u64 fs_expand_inode(struct fs *fs, u64 inode_index);
// Shrinks inode by one block
static u64 fs_shrink_inode(struct fs *fs, u64 inode_index);

// Get inode by path
static u64 fs_query_inode(struct fs *fs, char *path);

// Write/Read inodes from harddrive
static u64 fs_write_inode(u64 index, struct inode *inode);
static u64 fs_read_inode(u64 index, struct inode *inode);

// Create/Delete inodes by creating removing dir entries
static u64 fs_create_inode(struct fs *fs, char *name, char *path);
static u64 fs_delete_inode(struct fs *fs, char *name, char *path);

// Write/Read contents into files
static u64 fs_write_content(struct fs *fs, char *path, u64 off, u64 len, u8 *data);
static u64 fs_read_content(struct fs *fs, char *path, u64 off, u64 len, u8 *data);