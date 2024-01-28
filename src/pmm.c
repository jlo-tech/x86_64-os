#include <pmm.h>

// Linker variables
extern char kernel_base;
extern char kernel_limit;

static const u64 kernel_base_addr  = (u64)&kernel_base;
static const u64 kernel_limit_addr = (u64)&kernel_limit;

/**
 * Align address to a specific alignment
 * If addresses are not aligned they are rounded up 
 */
static u64 align(u64 addr, u64 alignment)
{
    u64 masked = addr & (alignment - 1);
    return (addr == masked) ? addr : (masked + alignment);
}

static struct ktree_node* ktree_leftmost(struct ktree_node *node)
{
    while(node->valid[KTREE_LEFT])
    {
        node = node->left;
    }

    return node;
}

static struct ktree_node* ktree_rightmost(struct ktree_node *node)
{
    while(node->valid[KTREE_RIGHT])
    {
        node = node->right;
    }

    return node;
}

void ktree_insert(struct ktree_root *root, void *container, i64 offset, i64 (*compare)(void*, void*))
{
    // If this is the first node just insert it
    if(!root->valid)
    {
        root->root = (struct ktree_node*)(container+offset);
        root->valid = true;
    }
    else
    {
        struct ktree_node *node;
        // If this is not the first traverse tree

        // "Load" first node
        node = root->root;

        // Actual traversing
        while(1) {
            // Check in which direction to go
            i64 c = compare((void*)(((u8*)node)-offset), container);
            // Handle left first
            if(c < 0)
            {
                // If there is already a node
                if(node->valid[KTREE_LEFT])
                {
                    // Then go to next one
                    node = node->left;
                    continue;
                }
                // If there is no one
                else 
                {
                    // If not insert
                    node->left = (struct ktree_node*)(container+offset);
                    node->valid[KTREE_LEFT] = true;
                    // And set the parent node pointer
                    ((struct ktree_node*)(container+offset))->parent = node;
                    // Done
                    return;
                }
            }
            // Handle right right afterwards
            else
            {
                // If there is already a node
                if(node->valid[KTREE_RIGHT])
                {
                    // Then go to next one
                    node = node->right;
                    continue;
                }
                // If there is no one
                else 
                {
                    // If not insert
                    node->right = (struct ktree_node*)(container+offset);
                    node->valid[KTREE_RIGHT] = true;
                    // And set the parent node pointer
                    ((struct ktree_node*)(container+offset))->parent = node;
                    // Done
                    return;
                }
            }
        }
    }
}

void ktree_remove(struct ktree_root *root, void *container, i64 offset, i64 (*compare)(void*, void*))
{
    struct ktree_node *node;
    // "Load" first node
    node = root->root;
    bool last;
    // Actual traversing
    while(1)
    {
        // Check in which direction to go
        i64 c = compare((void*)(((u8*)node)-offset), container);
        // Handle left first
        if(c == 0)
        {
            // Actually delete node
            if(last == KTREE_LEFT)
            {
                // If this node has no childs just delete it
                if(!node->valid[KTREE_LEFT] && !node->valid[KTREE_RIGHT])
                {
                    node->parent->valid[last] = false;
                }
                // If only left child is valid
                else if(node->valid[KTREE_LEFT] && !node->valid[KTREE_RIGHT])
                {
                    node->parent->left = node->left;
                    node->left->parent = node->parent;
                    
                }
                // If only right child is valid
                else if(!node->valid[KTREE_LEFT] && node->valid[KTREE_RIGHT])
                {
                    node->parent->left = node->right;
                    node->right->parent = node->parent;
                }
                // If both are valid
                else
                {
                    // Find smallest (leftmost) value of the right subtree
                    struct ktree_node *leftmost = ktree_leftmost(node->right);
                    // Replace node to be deleted with leftmost node of right subtree
                    node->parent->left = leftmost;
                    leftmost->parent = node->parent;
                    // Delete leftmost from old position
                    if(node->right == leftmost)
                    {
                        // Not really necessary, but for completenes just do it
                        leftmost->parent->valid[KTREE_RIGHT] = false;
                    }
                    else 
                    {
                        leftmost->parent->valid[KTREE_LEFT] = false;
                    }
                    // Add original subtree
                    leftmost->left = node->left;
                    leftmost->right = node->right;
                }
            }
            if(last == KTREE_RIGHT)
            {
                // If this node has no childs just delete it
                if(!node->valid[KTREE_LEFT] && !node->valid[KTREE_RIGHT])
                {
                    node->parent->valid[last] = false;
                }
                // If only left child is valid
                else if(node->valid[KTREE_LEFT] && !node->valid[KTREE_RIGHT])
                {
                    node->parent->right = node->left;
                    node->left->parent = node->parent;
                    
                }
                // If only right child is valid
                else if(!node->valid[KTREE_LEFT] && node->valid[KTREE_RIGHT])
                {
                    node->parent->right = node->right;
                    node->right->parent = node->parent;
                }
                // If both are valid
                else
                {
                    // Find biggest (rightmost) value of the left subtree
                    struct ktree_node *rightmost = ktree_rightmost(node->left);
                    // Replace node to be deleted with rightmost node of left subtree
                    node->parent->right = rightmost;
                    rightmost->parent = node->parent;
                    // Delete rightmost from old position
                    if(node->left == rightmost)
                    {
                        // Not really necessary, but for completenes just do it
                        rightmost->parent->valid[KTREE_LEFT] = false;
                    }
                    else 
                    {
                        rightmost->parent->valid[KTREE_RIGHT] = false;
                    }
                    // Add original subtree
                    rightmost->left = node->left;
                    rightmost->right = node->right;
                }
            }
            return;
        }
        else if(c < 0)
        {
            // If there is already a node
            if(node->valid[KTREE_LEFT])
            {
                // Then go to next one
                node = node->left;
                last = KTREE_LEFT;
                continue;
            }
            // If there is no one
            else 
            {
                return;
            }
        }
        // Handle right right afterwards
        else 
        {
            // If there is already a node
            if(node->valid[KTREE_RIGHT])
            {
                // Then go to next one
                node = node->right;
                last = KTREE_RIGHT;
                continue;
            }
            // If there is no one
            else
            {
                return;
            }
        }
    }
}