#include <util.h>

void bzero(u8 *mem, u64 size)
{
    for(u64 i = 0; i < size; i++)
    {
        mem[i] = 0;
    }
}

struct ktree_node* ktree_leftmost(struct ktree_node *root)
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

struct ktree_node* ktree_rightmost(struct ktree_node *root)
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

void __ktree_insert(struct ktree_node *root, struct ktree_node *node, 
                  int off, int (*cmp)(void*, void*))
{
    void *rv = ((u8*)root) - off;
    void *nv = ((u8*)node) - off;

    int r = cmp(rv, nv);

    if(r < 0)
    {
        if(root->valid[KTREE_LEFT])
        {
            return __ktree_insert(root->left, node, off, cmp);
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
            return __ktree_insert(root->right, node, off, cmp);
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

void __ktree_remove(struct ktree_node *root, struct ktree_node *node, 
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
            struct ktree_node *tr = root->right;
            if(root->parent->left == root)
            {
                root->parent->left = root->left;
            }
            else
            {
                root->parent->right = root->left;
            }
            struct ktree_node *rm = ktree_rightmost(root->left);
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
            __ktree_remove(root->left, node, off, cmp);
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
            __ktree_remove(root->right, node, off, cmp);
        }
        else 
        {
            return;
        }
    }
}

void ktree_insert(struct ktree *root, struct ktree_node *node, 
                  int off, int (*cmp)(void*, void*))
{
    if(root->valid)
    {
        __ktree_insert(root->root, node, off, cmp);
    }
    else
    {
        root->valid = true;
        root->root = node;
    }
}

void ktree_remove(struct ktree *root, struct ktree_node *node, 
                  int off, int (*cmp)(void*, void*))
{
    if(root->valid)
    {
        void *rv = ((u8*)root->root) - off;
        void *nv = ((u8*)node) - off;

        int r = cmp(rv, nv);

        if(r == 0)
        {
            // Delete node

            // No children
            if(!root->root->valid[KTREE_LEFT] && !root->root->valid[KTREE_RIGHT])
            {
                root->valid = false;
            }

            // Only one child
            if(!root->root->valid[KTREE_LEFT] && root->root->valid[KTREE_RIGHT])
            {
                root->root = root->root->right;
            }
            if(root->root->valid[KTREE_LEFT] && !root->root->valid[KTREE_RIGHT])
            {
                root->root = root->root->left;
            }

            // Two children
            if(root->root->valid[KTREE_LEFT] && root->root->valid[KTREE_RIGHT])
            {
                struct ktree_node *tr = root->root->right;
                root->root = root->root->left;
                struct ktree_node *rm = ktree_rightmost(root->root);
                rm->right = tr;
                rm->valid[KTREE_RIGHT] = true;
                tr->parent = rm;
            }
        }
        else if(r < 0)
        {
            __ktree_remove(root->root->left, node, off, cmp);
        }
        else
        {
            __ktree_remove(root->root->right, node, off, cmp);
        }
    }
}

bool ktree_contains(struct ktree *root, void *val, int off, int (*cmp)(void*, void*))
{
    struct ktree_node *curr = root->root;

    while(true)
    {
        void *cv = ((u8*)curr) - off;

        int r = cmp(cv, val);

        if(r == 0)
        {
            return true;
        }
        else if(r < 0)
        {
            if(curr->valid[KTREE_LEFT])
            {
                curr = curr->left;
            }
            else
            {
                return false;
            }
        }
        else
        {
            if(curr->valid[KTREE_RIGHT])
            {
                curr = curr->right;
            }
            else
            {
                return false;
            }
        }
    }
}