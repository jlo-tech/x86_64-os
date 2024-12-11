#include <util.h>

#include <pmm.h>

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

bool memcmp(u8 *m0, u8 *m1, size_t n)
{
    for(size_t i = 0; i < n; i++)
    {
        if(m0[i] != m1[i])
            return false;
    }

    return true; 
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

i64 abs(i64 x)
{
    if(x < 0)
        return -x;
    return x;
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

void kqueue_init(struct kqueue *kqueue, size_t capacity)
{
    kqueue->head = 0;
    kqueue->tail = 0;
    kqueue->capacity = capacity;
    kqueue->data = (void**)kmalloc(capacity * sizeof(void*));
}

void kqueue_deinit(struct kqueue *kqueue)
{
    kqueue->head = 0;
    kqueue->tail = 0;
    kqueue->capacity = 0;
    kfree((i64)kqueue->data);
}

bool kqueue_enqueue(struct kqueue *kqueue, void *item)
{
    // Buffer full
    if((kqueue->tail + 1) == kqueue->head)
    {
        return false;
    }

    // Free space available
    kqueue->data[kqueue->tail] = item;
    kqueue->tail = (kqueue->tail + 1) % kqueue->capacity;

    return true;
}

bool kqueue_dequeue(struct kqueue *kqueue, void **item)
{
    // Buffer empty
    if(kqueue->head == kqueue->tail)
    {
        return false;
    }

    // Item avialable
    *item = kqueue->data[kqueue->head];
    kqueue->head = (kqueue->head + 1) % kqueue->capacity;

    return true;
}

bool kqueue_peek(struct kqueue *kqueue, void **item)
{
    // Buffer empty
    if(kqueue->head == kqueue->tail)
    {
        return false;
    }

    // Item avialable
    *item = kqueue->data[kqueue->head];

    return true;
}

#define kmap_index_key(kmap, index) \
            ((u8*)kmap->keys + (kmap->key_size * index))

#define kmap_index_val(kmap, index) \
            ((u8*)kmap->vals + (kmap->val_size * index))
    
void kmap_init(struct kmap *kmap, size_t capacity, size_t key_size, size_t val_size)
{
    kmap->capacity = capacity;
    kmap->key_size = key_size;
    kmap->val_size = val_size;
    kmap->keys  = (void*)kmalloc(capacity * key_size);
    kmap->vals  = (void*)kmalloc(capacity * val_size);
    kmap->bitmap = (u64*)kmalloc((capacity / 64) + 1);
    // Clear bitmap
    bzero((u8*)kmap->bitmap, (capacity / 64) + 1);
}

// Check if an entry is used
static bool kmap_is_free_or_used(struct kmap *kmap, size_t index)
{
    if(index >= kmap->capacity)
        return false;

    // Returns 1 when used and 0 when free
    return (kmap->bitmap[index / 64] >> (index % 64)) & 1;
}

// Toggle entry mark from free to used and vice versa
static void kmap_toggle(struct kmap *kmap, size_t index)
{
    if(index >= kmap->capacity)
        return;

    kmap->bitmap[index / 64] ^= (1 << (index % 64));
}

// Returns next free index (index is out var)
static bool kmap_next(struct kmap *kmap, size_t *index)
{
    for(size_t i = 0; i < (kmap->capacity / 64) + 1; i++)
    {
        if(kmap->bitmap[i] != 0xFFFFFFFFFFFFFFFF)
        {
            for(size_t j = 0; j < 64; j++)
            {
                if(!((kmap->bitmap[i] >> j) & 1))
                {
                    *index = (i * 64 + j);
                    return true;
                }
            }
        }
    }
}

void kmap_new(struct kmap *kmap, void *key, void *val)
{
    size_t pos;
    
    // Check if there is still free space, else return
    if(!kmap_next(kmap, &pos))
    {
        return;
    }

    // Mark as used
    kmap_toggle(kmap, pos);

    // Copy key and val
    memcpy(kmap_index_key(kmap, pos), key, kmap->key_size);
    memcpy(kmap_index_val(kmap, pos), val, kmap->val_size);
}

void kmap_get(struct kmap *kmap, void *key, void *val)
{
    u8 *ckey = (u8*)key;

    for(size_t i = 0; i < kmap->capacity; i++)
    {
        if(memcmp((u8*)kmap->keys + (kmap->key_size * i), ckey, kmap->key_size))
        {
            // Copy value
            memcpy(val, kmap_index_val(kmap, i), kmap->val_size);
            return;
        }
    }
}

bool kmap_contains(struct kmap *kmap, void *key)
{
    u8 *ckey = (u8*)key;

    for(size_t i = 0; i < kmap->capacity; i++)
    {
        if(memcmp(kmap_index_key(kmap, i), ckey, kmap->key_size))
        {
            return true;
        }
    }

    return false;
}

void kmap_del(struct kmap *kmap, void *key)
{
     u8 *ckey = (u8*)key;

    for(size_t i = 0; i < kmap->capacity; i++)
    {
        if(memcmp(kmap_index_key(kmap, i), ckey, kmap->key_size))
        {
            // Mark entry in bitmap as free
            kmap->bitmap[i / 64] ^= (1 << (i % 64));
            return;
        }
    }
}