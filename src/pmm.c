#include <pmm.h>

// Linker variables
extern char kernel_base;
extern char kernel_limit;

static const u64 kernel_base_addr  = (u64)&kernel_base;
static const u64 kernel_limit_addr = (u64)&kernel_limit;

#define ABS(x) ((x < 0) ? (-x) : x)

/**
 * Align address to a specific alignment
 * If addresses are not aligned they are rounded up 
 */
static u64 align(u64 addr, u64 alignment)
{
    u64 masked = addr & (alignment - 1);
    return (addr == masked) ? addr : (masked + alignment);
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

static int cmp_chunks(struct kchunk *c0, struct kchunk* c1)
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
    if(klist_empty(&heap->free_buddies[index]))
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
                if(klist_empty(&heap->free_buddies[i]))
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
            struct kchunk rc = *ENCLAVE(struct kchunk, list_handle, heap->free_buddies[j].root);

            // Remove old "big" chunk
            klist_pop(&heap->free_buddies[j], heap->free_buddies[j].root);

            // Split bigger chunk
            struct kchunk *nc0;
            struct kchunk *nc1;
            split_chunk(rc, &nc0, &nc1);

            // Add new chunks to smaller list
            klist_push(&heap->free_buddies[j-1], &nc0->list_handle);
            klist_push(&heap->free_buddies[j-1], &nc1->list_handle);
        }
    }

    // Get actual chunk
    struct kchunk *chunk = ENCLAVE(struct kchunk, list_handle, heap->free_buddies[index].root);

    // Remove chunk from free list
    klist_pop(&heap->free_buddies[index], heap->free_buddies[index].root);
    
    // Insert in used (but now by address)
    ktree_insert(&heap->used_buddies, &chunk->tree_handle, (i64)&(((struct kchunk*)0)->tree_handle), (int (*)(void*, void*))cmp_chunks);

    // Return address
    return (chunk->addr + sizeof(struct kchunk));
}

i64 kheap_free(i64 addr)
{
    // TODO
    return -1;
}