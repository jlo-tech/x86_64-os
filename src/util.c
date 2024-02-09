#include <util.h>

void bzero(u8 *mem, u64 size)
{
    for(u64 i = 0; i < size; i++)
    {
        mem[i] = 0;
    }
}

struct ktree* ktree_leftmost(struct ktree *root)
{
    if(root->valid[KTREE_LEFT])
    {
        return ktree_leftmost(root->left);
    }
    else
    {
        return root;
    }
}

struct ktree* ktree_rightmost(struct ktree *root)
{
    if(root->valid[KTREE_RIGHT])
    {
        return ktree_rightmost(root->right);
    }
    else
    {
        return root;
    }
}

void ktree_insert(struct ktree *root, struct ktree *node, 
                  int off, int (*cmp)(void*, void*))
{
    void *rv = ((u8*)root) - off;
    void *nv = ((u8*)node) - off;

    int r = cmp(rv, nv);

    if(r < 0)
    {
        if(root->valid[KTREE_LEFT])
        {
            return ktree_insert(root->left, node, off, cmp);
        }
        else 
        {
            root->valid[KTREE_LEFT] = true;
            root->left = node;
            node->parent = root;
            return;
        }
    }
    else
    {
        if(root->valid[KTREE_RIGHT])
        {
            return ktree_insert(root->right, node, off, cmp);
        }
        else
        {
            root->valid[KTREE_RIGHT] = true;
            root->right = node;
            node->parent = root;
            return;
        }
    }
}

void ktree_remove(struct ktree *root, struct ktree *node, 
                  int off, int (*cmp)(void*, void*))
{
    void *rv = ((u8*)root) - off;
    void *nv = ((u8*)node) - off;

    int r = cmp(rv, nv);

    if(r == 0)
    {
        // Delete node

        // No children
        if(!root->valid[KTREE_LEFT] && !root->valid[KTREE_RIGHT])
        {
            if(root->parent->left == root)
            {
                root->parent->valid[KTREE_LEFT] = false;
            }
            else 
            {
                root->parent->valid[KTREE_RIGHT] = false;
            }
        }

        // Only one child
        if(!root->valid[KTREE_LEFT] && root->valid[KTREE_RIGHT])
        {
            if(root->parent->left == root)
            {
                root->parent->left = root->right;
            }
            else 
            {
                root->parent->right = root->right;
            }
        }
        if(root->valid[KTREE_LEFT] && !root->valid[KTREE_RIGHT])
        {
            if(root->parent->left == root)
            {
                root->parent->left = root->left;
            }
            else 
            {
                root->parent->right = root->left;
            }
        }

        // Two children
        if(root->valid[KTREE_LEFT] && root->valid[KTREE_RIGHT])
        {
            struct ktree *tr = root->right;
            if(root->parent->left == root)
            {
                root->parent->left = root->left;
            }
            else
            {
                root->parent->right = root->left;
            }
            struct ktree *rm = ktree_rightmost(root->left);
            rm->right = tr;
            rm->valid[KTREE_RIGHT] = true;
            rm->parent = root->parent;
            tr->parent = rm;
        }
    }
    // Continue traversing
    else if(r < 0)
    {
        if(root->valid[KTREE_LEFT])
        {
            ktree_remove(root->left, node, off, cmp);
        }
        else 
        {
            return;
        }
    }
    else 
    {
        if(root->valid[KTREE_RIGHT])
        {
            ktree_remove(root->right, node, off, cmp);
        }
        else 
        {
            return;
        }
    }
}