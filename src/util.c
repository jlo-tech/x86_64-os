#include <util.h>

void bzero(u8 *mem, u64 size)
{
    for(u64 i = 0; i < size; i++)
    {
        mem[i] = 0;
    }
}

void memcpy(void *dst, void *src, size_t sz)
{
    u8* bdst = (u8*)dst;
    u8* bsrc = (u8*)src;

    for(size_t i = 0; i < sz; i++)
    {
        bdst[i] = bsrc[i];
    }
}

size_t strlen(char *str)
{
    size_t c = 0;

    while(*str != 0) {    
        c++;
        str++;
    };
    
    return c;
}

size_t min(size_t a, size_t b)
{
    if(a < b)
        return a;
    return b;
}

size_t max(size_t a, size_t b)
{
    if(a > b)
        return a;
    return b;
}

struct ktree_node* ktree_leftmost(struct ktree_node *root)
{
    struct ktree_node *curr = root;

    while(1)
    {
        if(curr->valid[KTREE_LEFT])
        {
            curr = curr->left;
        }
        else
        {
            return curr;
        }
    }
}

struct ktree_node* ktree_rightmost(struct ktree_node *root)
{
    struct ktree_node *curr = root;

    while(1)
    {
        if(curr->valid[KTREE_RIGHT])
        {
            curr = curr->right;
        }
        else
        {
            return curr;
        }
    }
}

bool ktree_empty(struct ktree *root)
{
    return !root->valid;
}

void __ktree_insert(struct ktree_node *root, struct ktree_node *node, 
                  int off, int (*cmp)(void*, void*))
{
    while(1)
    {
        void *rv = ((u8*)root) - off;
        void *nv = ((u8*)node) - off;

        int r = cmp(rv, nv);

        if(r < 0)
        {
            if(root->valid[KTREE_LEFT])
            {
                root = root->left;
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
                root = root->right;
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
}

void __ktree_remove(struct ktree_node *root, struct ktree_node *node, 
                  int off, int (*cmp)(void*, void*))
{
    while(1) 
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

            return;
        }
        // Continue traversing
        else if(r < 0)
        {
            if(root->valid[KTREE_LEFT])
            {
                root = root->left;
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
                root = root->right;
            }
            else 
            {
                return;
            }
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

bool ktree_find(struct ktree *root, void *val, int off, int (*cmp)(void*, void*), struct ktree_node **res)
{
    struct ktree_node *curr = root->root;

    while(true)
    {
        void *cv = ((u8*)curr) - off;

        int r = cmp(cv, val);

        if(r == 0)
        {
            *res = curr;
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

bool klist_empty(struct klist *root)
{
    return !root->valid;
}

void klist_push(struct klist *root, struct klist_node *node)
{
    if(root->valid)
    {
        node->next = root->root;
        node->valid = true;
        root->root = node;
    }
    else
    {
        root->valid = true;
        root->root = node;
    }
}

void klist_pop(struct klist *root, struct klist_node *node)
{
    if(!root->valid)
    {
        // List is empty
        return;
    }
    else
    {
        if(root->root == node)
        {
            // Delete root node
            root->valid = root->root->valid;
            root->root = root->root->next;
        }
        else
        {
            //Traverse list
            struct klist_node *curr = root->root;
            
            do {
                if(curr->valid)
                {
                    if(curr->next == node)
                    {
                        // Delete node
                        curr->valid = curr->next->valid;
                        curr->next = curr->next->next;
                        return;
                    }
                    else
                    {
                        curr = curr->next;
                    }
                }
                else
                {
                    // That was the last node
                    return;
                }
            } while(1);
        }
    }
}