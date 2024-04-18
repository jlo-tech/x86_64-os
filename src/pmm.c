#include <pmm.h>

#define ABS(x) ((x < 0) ? (-x) : x)

/**
 * Align address to a specific alignment
 * If addresses are not aligned they are rounded up 
 */
u64 align(u64 addr, u64 alignment)
{
    if((addr % alignment) == 0)
    {
        return addr;
    }
    return (addr - (addr % alignment)) + alignment;
}

/*
 * Rounds up to next biggest power of two
*/
static i64 __rp2(i64 x)
{
    if(x == 0)
        return 1;

    i64 r = 1;
    do {
        r <<= 1;
    } while(x >>= 1);
    return r;
}

/*
 * Checks if a number is a power of two
*/
static bool __p2(i64 x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

int cmp_chunks(struct kchunk *c0, struct kchunk* c1)
{
    if(c0->addr < c1->addr)
    {
        return -1;
    }
    if(c0->addr == c1->addr)
    {
        return 0;
    }
    return 1;
}

void kheap_init(struct kheap *heap)
{
    bzero((void*)heap, sizeof(struct kheap));
}

static void split_chunk(struct kchunk chunk_to_split, struct kchunk **new_chunk0, struct kchunk **new_chunk1)
{
    i64 new_size = chunk_to_split.size / 2;

    *new_chunk0 = (struct kchunk*)(chunk_to_split.addr);
    *new_chunk1 = (struct kchunk*)(chunk_to_split.addr + new_size * PAGE_SIZE);

    bzero((void*)*new_chunk0, sizeof(struct kchunk));
    bzero((void*)*new_chunk1, sizeof(struct kchunk));

    (*new_chunk0)->addr = chunk_to_split.addr;
    (*new_chunk1)->addr = chunk_to_split.addr + new_size * PAGE_SIZE;

    (*new_chunk0)->size = new_size;
    (*new_chunk1)->size = new_size;
}

i64 kheap_alloc(struct kheap *heap, i64 size)
{
    size = ABS(size);

    // Reserve extra space for meta information
    size = size + sizeof(struct kchunk);

    // How many pages do we need?
    i64 pages = ((size % 4096) == 0) ? (size / 4096) : (size / 4096) + 1;

    // Next closest power of 2 num of pages
    i64 chunk_size_in_pages = __p2(pages) ? pages : __rp2(pages);

    // Get index in free buddy array
    i64 index = 63 - __builtin_clzll((u64)chunk_size_in_pages);
    if(ktree_empty(&heap->free_buddies[index]))
    {
        int i = index;
        while(i < 48) 
        {
            if(i >= 48)
            {
                // Out of memory
                return -1;
            }
            else
            {
                if(ktree_empty(&heap->free_buddies[i]))
                {
                    i++;
                }
                else
                {
                    break;
                }
            }
        }

        // Need (i - index) splits to create chunk of appropriate size
        for(i64 j = i; j > index; j--)
        {
            // Save old "big" chunk
            struct kchunk rc = *ENCLAVE(struct kchunk, tree_handle, heap->free_buddies[j].root);

            // Remove old "big" chunk
            ktree_remove(&heap->free_buddies[j], 
                        heap->free_buddies[j].root, 
                        OFFSET(struct kchunk, tree_handle), 
                        (int (*)(void*, void*))cmp_chunks);

            // Split bigger chunk
            struct kchunk *nc0;
            struct kchunk *nc1;
            split_chunk(rc, &nc0, &nc1);

            // Add new chunks to smaller list
            ktree_insert(&heap->free_buddies[j-1], &nc0->tree_handle, 
                        OFFSET(struct kchunk, tree_handle), (int (*)(void*, void*))cmp_chunks);

            ktree_insert(&heap->free_buddies[j-1], &nc1->tree_handle, 
                        OFFSET(struct kchunk, tree_handle), (int (*)(void*, void*))cmp_chunks);
        }
    }

    // Get actual chunk
    struct kchunk *chunk = ENCLAVE(struct kchunk, tree_handle, heap->free_buddies[index].root);

    // Remove chunk from free list
    ktree_remove(&heap->free_buddies[index], 
                &chunk->tree_handle,
                OFFSET(struct kchunk, tree_handle), 
                (int (*)(void*, void*))cmp_chunks);

    // Remove old and invalid tree information
    bzero((void*)&chunk->tree_handle, sizeof(struct ktree_node));

    // Insert in used (but now by address)
    ktree_insert(&heap->used_buddies, 
                &chunk->tree_handle, 
                OFFSET(struct kchunk, tree_handle), 
                (int (*)(void*, void*))cmp_chunks);

    // Return address
    return (chunk->addr + sizeof(struct kchunk));
}

i64 kheap_free(struct kheap *heap, i64 addr)
{   
    // Query struct
    struct kchunk query;
    bzero((void*)&query, sizeof(struct kchunk));
    query.addr = addr - sizeof(struct kchunk); // Get original chunk address

    // Load chunk from tree
    struct kchunk *freed;
    struct ktree_node *sr = NULL;

    if(!ktree_find(&heap->used_buddies, &query, 
        OFFSET(struct kchunk, tree_handle),
        (int (*)(void*, void*))cmp_chunks,
        &sr))
    {
        return -1; // Error chunk not found
    }

    // Get kchunk for search result
    freed = ENCLAVE(struct kchunk, tree_handle, sr);

    // Index in free_buddies array
    i64 index = 63 - __builtin_clzll((u64)freed->size);

    // Remove chunk from used
    ktree_remove(&heap->used_buddies, 
                &freed->tree_handle, 
                OFFSET(struct kchunk, tree_handle),
                (int (*)(void*, void*))cmp_chunks);
    
    bzero((void*)&freed->tree_handle, sizeof(struct ktree_node));

    // Insert into free
    ktree_insert(&heap->free_buddies[index], 
                &freed->tree_handle, 
                OFFSET(struct kchunk, tree_handle),
                (int (*)(void*, void*))cmp_chunks);

    while(1) 
    {
        // Determine left or right buddy
        bool odd = (freed->addr / (freed->size * PAGE_SIZE)) & 1;

        // Check for buddy
        i64 buddy_addr = odd ? 
            (freed->addr - freed->size * PAGE_SIZE) : 
            (freed->addr + freed->size * PAGE_SIZE);

        // Query buddy chunk
        struct kchunk *buddy;
        query.addr = buddy_addr;
        if(!ktree_find(&heap->free_buddies[index], &query, 
                OFFSET(struct kchunk, tree_handle), 
                (int (*)(void*, void*))cmp_chunks, 
                &sr))
        {
            // No merging, just return
            return 0;
        }
        else
        {
            // Else merge buddies

            // Load buddy for search result
            buddy = ENCLAVE(struct kchunk, tree_handle, sr);
         
            // Remove both from free
            ktree_remove(&heap->free_buddies[index], 
                &buddy->tree_handle, 
                OFFSET(struct kchunk, tree_handle),
                (int (*)(void*, void*))cmp_chunks);

            bzero((void*)&buddy->tree_handle, sizeof(struct ktree_node));

            ktree_remove(&heap->free_buddies[index], 
                &freed->tree_handle, 
                OFFSET(struct kchunk, tree_handle),
                (int (*)(void*, void*))cmp_chunks);

            bzero((void*)&freed->tree_handle, sizeof(struct ktree_node));

            // Construct new chunk from both buddies to insert
            if(odd) {

                freed = (struct kchunk*)buddy_addr;
                freed->addr = buddy_addr;
            }
            freed->size <<= 1;

            // Go to next bigger slot
            index++;

            // Insert new chunk into (bigger) free slot
            ktree_insert(&heap->free_buddies[index],
                &freed->tree_handle,
                OFFSET(struct kchunk, tree_handle),
                (int (*)(void*, void*))cmp_chunks);

            // Start off another iteration to merge (potenitally) more chunks
        }
    }

    return 0;
}

struct kheap kernel_heap;

i64 kmalloc(i64 size)
{
    return kheap_alloc(&kernel_heap, size);
}

i64 kfree(i64 addr)
{
    return kheap_free(&kernel_heap, addr);
}