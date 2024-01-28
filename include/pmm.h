#pragma once

#include <types.h>

#define PAGE_SIZE (u64)(1 << 12)
#define PAGE_MASK (u64)(PAGE_SIZE - 1)   // To check for correct alignment

/* Some more or less generic tree implementation */

#define KTREE_LEFT    0
#define KTREE_RIGHT   1

struct ktree_root
{
    bool valid;
    struct ktree_node *root;
};

struct ktree_node
{
    bool valid[2];              // Tells if pointers left, right are valid (allows us to use even the null pointer as a valid address)
    struct ktree_node *left;
    struct ktree_node *right;
    struct ktree_node *parent;
};

/** 
 * Insert a new node into the ktree
 * @param container is a pointer to the container structure
 * @param offset is the offset from the container to the ktree_node
 * @param compare Comparison function (takes two container structures and returns -1 on <, 0 on ==, 1 on >)
*/
void ktree_insert(struct ktree_root *root, void *container, i64 offset, i64 (*compare)(void*, void*));

/** 
 * Remove node from the ktree
*/
void ktree_remove(struct ktree_root *root, void *container, i64 offset, i64 (*compare)(void*, void*));


/* We will use a modified version of the buddy allocator 
   combined with trees to make memory allocation as fast as possible */


/* Allocator management structure */
struct kheap_alloc
{
    
};

/* Structure sitting at top of each chunk */
struct kchunk
{
    
};