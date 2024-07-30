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

    // Zero out block to prevent data leaks and  
    // keep blocks clean when allocated to be pointer blocks
    bzero((u8*)&tmp, FS_BLOCK_SIZE);
    r = fs_write(fs, index, (u8*)&tmp);
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
        off = shift * (3 - i);
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

/**
 * Allocated new blocks to an inode
 *
 * @param block_index A new block is allocated for the "block_index"th block on the inode
 *
 */
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
 * Checks if a block only contains zeros
 *
 * @param block Pointer to block in memory
 */
static bool __fs_block_all_zeros(i64 *block)
{
    for(i64 i = 0; i < (FS_BLOCK_SIZE >> 3); i++)
    {
        if(block[i] != 0)
        {
            return false;
        }
    }

    return true;
}

/**
 * Removes block from inode and marks it as free 
 */
i64 __fs_inode_free(struct fs *fs, i64 inode_data_tree, i64 inode_index, i64 block_index)
{
    const i64 mask  = ((i64)511);
    const i64 shift = ((i64)9);
  
    i64 off, next, trace[4], stubs[4];  

    // Local pointer
    i64 *loc = (i64*)&tmp;  

    // Current block index
    i64 bx = inode_data_tree;

    // Traverse tree
    for(i64 i = 0; i < 4; i++)
    {
        // Load current layer block
        fs_read(fs, bx, (u8*)&tmp);

        // Save traversed blocks in trace array 
        trace[i] = bx;

        // Get pointer to next layer
        off = shift * (3 - i);
        stubs[i] = (block_index & (mask << off)) >> off;
        next = loc[stubs[i]];

        // If block was never allocated return error
        if(next == 0)
        {
            // Block was not allocated
            return FS_ERROR;        
        }

        // Continue with next layer
        bx = next;
    }

    // Remove from inode
    loc[stubs[3]] = 0;
    // And write changes to disk
    fs_write(fs, trace[3], (u8*)&tmp);
    // Mark block as free 
    fs_free(fs, bx); 

    // Iterate through previously created array and free blocks that could be freed
    //  1. Read block
    //  2. Check if it only contains 0s
    //  3. If so free that block by writing 0 to higher level block
    //  4. If not abort cause nothing more to do
    //  Start at 1. for next level
    // Write 0 to inode's data tree or to highest level block which has not be freed
   
    // Reload block polluted by fs_free
    fs_read(fs, trace[3], (u8*)&tmp);

    for(i64 i = 3; i > 0; i--)
    {
        // Last block was alredy read by previous loop / iteration
        if(__fs_block_all_zeros(loc))
        {
            // Mark block as free again
            fs_free(fs, trace[i]);
            // Read higher level block
            fs_read(fs, trace[i-1], (u8*)&tmp);
            // Mark entry for lower level block as zero
            loc[stubs[i-1]] = 0;
            // Write back to disk
            fs_write(fs, trace[i-1], (u8*)&tmp);
        }
        else
        {
            // If this block is not zero we also cannot free higher levels
            goto __fs_inode_free_end;
        }      
    }

    // Check highest level block
    struct inode *inode = (struct inode*)&tmp;
    if(__fs_block_all_zeros(loc))
    {
        // Load inode
        fs_read(fs, inode_index, (u8*)&tmp);
        // Mark block as free
        fs_free(fs, inode->data_tree);
        // Reload (by fs_free) polluted block
        fs_read(fs, inode_index, (u8*)&tmp);
        // Free
        inode->data_tree = 0;
        // Write back to disk
        fs_write(fs, inode_index, (u8*)&tmp);
    }

__fs_inode_free_end:

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
    return __fs_inode_free(fs, inode->data_tree, inode_index, block_index);
}

/**
 * Resize inode 
 *
 * @param inode_index Inode index
 * @param size The size of the inode in bytes
 *
 * @return New size or error
 */
i64 fs_inode_resize(struct fs *fs, i64 inode_index, i64 size)
{
    // Check that inode_index is valid
    if(inode_index == 0)
        return FS_ERROR;
    
    // Read inode from disk
    bool b = fs_read(fs, inode_index, (u8*)&tmp);
    // Err check
    if(!b)
        return FS_ERROR;

    // Pointer to inode
    struct inode *inode = (struct inode*)&tmp;

    // Allready allocated blocks
    i64 aab = inode->file_size / FS_BLOCK_SIZE;
    
    if((inode->file_size % FS_BLOCK_SIZE) != 0)
        aab++;

    // Blocks needed for the inode when set to new size
    i64 new_blocks = size / FS_BLOCK_SIZE;

    if((size % FS_BLOCK_SIZE) != 0)
        new_blocks++;

    // Diff between current blocks and needed blocks
    i64 block_diff = new_blocks - aab;

    // Do allocations
    if(block_diff > 0)
    {
        // Allocate additional blocks
        for(i64 i = 0; i < block_diff; i++)
        {
            i64 r = fs_inode_alloc(fs, inode_index, aab + i); 
            if(r == FS_ERROR)
                return FS_ERROR;
        }
    }

    // Do frees
    if(block_diff < 0)
    {
        // Free not needed blocks
        for(i64 i = 0; i < abs(block_diff); i++)
        {
            i64 r = fs_inode_free(fs, inode_index, (aab - 1) - i);
            if(r == FS_ERROR)
                return FS_ERROR;
        }
    }
    
    // Write back new size
    fs_read(fs, inode_index, (u8*)&tmp);
    inode->file_size = size;
    fs_write(fs, inode_index, (u8*)&tmp);

    // If no errors then return new size
    return size;
}

/**
 * Returns the index of the nth block in an inode
 *
 * @param inode_index Index of inode
 * @param n Nth block of the inode to be returned
 *
 * @return Returns position of the nth block in the inode or error
 */
i64 fs_inode_nth_block(struct fs *fs, i64 inode_index, i64 n)
{
    const i64 mask  = ((i64)511);
    const i64 shift = ((i64)9);
    
    i64 off, stub, next;

    // Local pointer
    i64 *loc = (i64*)&tmp;  
    struct inode *inode = (struct inode*)&tmp;

    // Load inode
    fs_read(fs, inode_index, (u8*)tmp);

    // Current block index
    i64 bx = inode->data_tree;

    // Traverse tree
    for(i64 i = 0; i < 4; i++)
    {
        // Load current layer block
        fs_read(fs, bx, (u8*)&tmp);

        // Get pointer to next layer
        off = shift * (3 - i);
        stub = (n & (mask << off)) >> off;
        next = loc[stub];

        // Block not allocated
        if(next == 0)
            return FS_ERROR;        
        
        // Continue with next layer
        bx = next;
    }
 
    // Return nth block index
    return bx;
}

// String helper methods
static i64 __fs_strlen(char *s)
{
    for(int i = 0; i < FS_NAME_LEN; i++)
    {
        if(*s == '\0')
            return i;
        s++;
    }

    return FS_NAME_LEN;
}

static bool __fs_strcmp(char *s0, char *s1)
{
    i64 l0 = __fs_strlen(s0);
    i64 l1 = __fs_strlen(s1);
    
    // Different lengths mean different strings
    if(l0 != l1)
       return false;

    for(int i = 0; i < l0; i++)
    {
        // Are strings still equal
        if(s0[i] != s1[i])
            return false;
    }
    // Strings are equal
    return true;
}

static void __fs_strcpy(char *dst, char *src)
{
    i64 len = __fs_strlen(src);

    for(i64 i = 0; i < len; i++)
    {
        dst[i] = src[i];
    }
}

/**
 * Make entry function
 *
 * @param inode_index Index of inode to add entry to
 * @param name Name of the new entry
 */
i64 fs_inode_add_entry(struct fs *fs, i64 inode_index, char *name)
{
     // Load inode
    struct inode *inode = (struct inode*)&tmp;    
    fs_read(fs, inode_index, (u8*)&tmp);
        
    // Check if inode is a directory
    if(inode->type != FS_TYPE_DIRECTORY)
        return FS_ERROR;

    // Calc number of blocks that need to be traversed
    i64 trav = inode->file_size / FS_BLOCK_SIZE;

    if((inode->file_size % FS_BLOCK_SIZE) != 0)
        trav++;

    // Backup some vars
    i64 total_size = inode->file_size;
    i64 num_entries = inode->num_entries;
    i64 free_space = inode->file_size - (num_entries * sizeof(struct dir_entry));
    i64 free_entries = free_space / sizeof(struct dir_entry);

    // Pre alloc block
    i64 nb = fs_alloc(fs);

    // Pointer to entries
    struct dir_entry *entries = (struct dir_entry*)&tmp;

    // Read block by block 
    for(i64 i = 0; i < trav; i++)
    {
        i64 r = fs_inode_nth_block(fs, inode_index, i); 
        if(r == FS_ERROR)
            return FS_ERROR;
        
        // Read nth block
        fs_read(fs, r, (u8*)&tmp);
        
        // Check that name does not already exists
        for(i64 j = 0; j < (FS_BLOCK_SIZE / (i64)sizeof(struct dir_entry)); j++)
        {
            if(__fs_strcmp(name, entries[j].file_name))
            {
                // Free pre allocted block
                fs_free(fs, nb);
                // Return error
                return FS_ERROR;  
            }
        }

        if(free_entries > 0)
        {
            // Insert new entry
            for(i64 j = 0; j < (FS_BLOCK_SIZE / (i64)sizeof(struct dir_entry)); j++)
            {
                // Search for free entry
                if(entries[j].inode_index == 0)
                {
                    // Insert entry
                    entries[j].inode_index = nb;
                    __fs_strcpy(entries[j].file_name, name);
                    // Write back to disk
                    fs_write(fs, r, (u8*)&tmp);
                    
                    // Increase inode num_entries counter
                    fs_read(fs, inode_index, (u8*)&tmp);
                    inode->num_entries++;
                    fs_write(fs, inode_index, (u8*)&tmp);

                    return nb;
                }  
            }
        }
    }

  
    // Allocate extra space and insert entry there
    fs_inode_resize(fs, inode_index, total_size + FS_BLOCK_SIZE); 

    // How many blocks will the inode have
    i64 next_block = total_size / FS_BLOCK_SIZE;
    if((total_size % FS_BLOCK_SIZE) != 0)
       next_block++;

    // Get index of next (still free) block
    i64 next_block_index = fs_inode_nth_block(fs, inode_index, next_block);
    // Error check
    if(next_block_index == -1)
        return FS_ERROR;

    // Clear tmp
    bzero((u8*)&tmp, FS_BLOCK_SIZE);

    // Insert entry
    entries[0].inode_index = nb;
    __fs_strcpy((char*)&entries[0].file_name, name);
    
    // Write to disk
    fs_write(fs, next_block_index, (u8*)&tmp);

    // Increase inode num_entries counter
    fs_read(fs, inode_index, (u8*)&tmp);
    inode->num_entries++;
    fs_write(fs, inode_index, (u8*)&tmp);

    // Entry not found
    return FS_ERROR;
}

/**
 * Delete entry from directory
 *
 * @param inode_index The inode that is to be modified
 * @param name Name of the entry / file / folder to be deleted
 *
 */
i64 fs_inode_del_entry(struct fs *fs, i64 inode_index, char *name)
{
     // Load inode
    struct inode *inode = (struct inode*)&tmp;    
    fs_read(fs, inode_index, (u8*)&tmp);
        
    // Check if inode is a directory
    if(inode->type != FS_TYPE_DIRECTORY)
        return FS_ERROR;

    // Calc number of blocks that need to be traversed
    i64 num_entries = inode->num_entries;
    i64 trav = (num_entries * sizeof(struct dir_entry)) / FS_BLOCK_SIZE;

    if(((num_entries * sizeof(struct dir_entry)) % FS_BLOCK_SIZE) != 0)
        trav++;

    // Read block by block 
    for(i64 i = 0; i < trav; i++)
    {
        i64 r = fs_inode_nth_block(fs, inode_index, i); 
        if(r == FS_ERROR)
            return FS_ERROR;
        
        // Read nth block
        fs_read(fs, r, (u8*)&tmp);

        // Search in block for name
        struct dir_entry *entries = (struct dir_entry*)&tmp;
        for(i64 j = 0; j < (FS_BLOCK_SIZE / (i64)sizeof(struct dir_entry)); j++)
        {
            // Stop traversing when all entries are read
            if((i64)((i * (FS_BLOCK_SIZE / sizeof(struct dir_entry))) + j) >= num_entries)
                return 0;

            // Check entry
            if(__fs_strcmp(name, entries[j].file_name))
            {
                // Free blocks of inode that should be deleted
                i64 iitbd = entries[j].inode_index;
                fs_inode_resize(fs, iitbd, 0);
                fs_free(fs, iitbd);                 
                // Read back polluted block
                fs_read(fs, r, (u8*)&tmp);
                // Remove entry
                entries[j].inode_index = 0;
                bzero((u8*)&entries[j], FS_NAME_LEN);
                // Write back
                fs_write(fs, r, (u8*)&tmp);
                // Check if resize is needed
                // Get last block and check for entries
                bool is_empty = true;
                fs_read(fs, trav-1, (u8*)&tmp);
                for(i64 k = 0; k < (FS_BLOCK_SIZE / (i64)sizeof(struct dir_entry)); k++)
                {
                    if(entries[k].inode_index != 0)
                    {
                        return 0;
                    }
                }   
                // Free last block
                fs_read(fs, inode_index, (u8*)&tmp);
                fs_inode_resize(fs, inode_index, inode->file_size - FS_BLOCK_SIZE);
                
                return 0;
            }
        }
    }

    return FS_ERROR;
}

/**
 * Returns inode index of the object with name "name".
 * NOTE: This method only searches in the given inode
 *       and makes no path traversal.
 *
 * @param inode_index Index of the inode to search in
 * @param name The name of the directoy/file we want the inode for
 *
 * @return Inode index of the requested object 
 */
i64 fs_inode_query_name(struct fs *fs, i64 inode_index, char* name)
{
    // Load inode
    struct inode *inode = (struct inode*)&tmp;    
    fs_read(fs, inode_index, (u8*)&tmp);
        
    // Check if inode is a directory
    if(inode->type != FS_TYPE_DIRECTORY)
        return FS_ERROR;

    // Calc number of blocks that need to be traversed
    i64 num_entries = inode->num_entries;
    i64 trav = (num_entries * sizeof(struct dir_entry)) / FS_BLOCK_SIZE;

    if(((num_entries * sizeof(struct dir_entry)) % FS_BLOCK_SIZE) != 0)
        trav++;

    // Read block by block 
    for(i64 i = 0; i < trav; i++)
    {
        i64 r = fs_inode_nth_block(fs, inode_index, i); 
        if(r == FS_ERROR)
            return FS_ERROR;
        
        // Read nth block
        fs_read(fs, r, (u8*)&tmp);

        // Search in block for name
        struct dir_entry *entries = (struct dir_entry*)&tmp;
        for(i64 j = 0; j < (FS_BLOCK_SIZE / (i64)sizeof(struct dir_entry)); j++)
        {
            // Stop traversing when all entries are read
            if((i64)((i * (FS_BLOCK_SIZE / sizeof(struct dir_entry))) + j) >= num_entries)
                return FS_ERROR;

            // Check entry
            if(__fs_strcmp(name, entries[j].file_name))
                return entries[j].inode_index;  
        }
    }

    // Entry not found
    return FS_ERROR;
}

/**
 * Checks if the given path is valid
 * I.e. does it start and end with /
 * and does not contain //  
 *
 * @return Valid path or not
 */
static bool __valid_path(char *path)
{
    if(path[0] != '/')
        return false;
    
    i64 slen = __fs_strlen(path);

    if(path[slen-1] != '/')
        return false;

    for(i64 i = 1; i < slen; i++)
    {
        if(path[i-1] == '/' && path[i] == '/')
            return false;
    }

    return true;
}

/**
 * Get inode (index) by path
 *
 * @param path Path
 *
 * @return Inode index of the dir/file at the given path
 */
i64 fs_inode_query(struct fs *fs, char *path)
{
    // Check if path is valid (see helper method)

    // Split path into names (with helper method)
    
    // Use fs_inode_query_name to travel through dir structure

    // TODO
}

/**
 * Calculates the size of the needed bitmap in bytes
 */
static i64 __bitmap_size(i64 disk_size, i64 block_size)
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
    fs->sb_cache.root_dir_inode_index = 0;

    // Write super block to start of the disk
    return fs_write(fs, 0, (u8*)&fs->sb_cache);
}

