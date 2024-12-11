#pragma once

#include <types.h>

#define KTREE_LEFT  0
#define KTREE_RIGHT 1

#define OFFSET(type, member) ((i64)&(((type*)0)->member))
#define ENCLAVE(type, member, ptr) (type*)(((void*)ptr) - (void*)(&((type*)0)->member))

struct ktree
{
    bool valid;
    struct ktree_node *root;
} __attribute__((packed));

struct ktree_node
{
    bool valid[2];
    struct ktree_node *left;
    struct ktree_node *right;
    struct ktree_node *parent;
} __attribute__((packed));

/* GP methods */
void bzero(u8 *mem, u64 size);
void memcpy(void *dst, void *src, size_t sz);
bool memcmp(u8 *m0, u8 *m1, size_t n);
size_t strlen(char *str);
size_t min(size_t a, size_t b);
size_t max(size_t a, size_t b);
i64 abs(i64 x);

bool ktree_empty(struct ktree *root);

void ktree_insert(struct ktree *root, struct ktree_node *node, 
                  int off, int (*cmp)(void*, void*));

void ktree_remove(struct ktree *root, struct ktree_node *node, 
                  int off, int (*cmp)(void*, void*));

bool ktree_contains(struct ktree *root, void *val, 
                    int off, int (*cmp)(void*, void*));

bool ktree_find(struct ktree *root, void *val, 
                    int off, int (*cmp)(void*, void*), 
                    struct ktree_node **res);

struct klist
{
    bool valid;
    struct klist_node *root;
} __attribute__((packed));

struct klist_node
{
    bool valid;
    struct klist_node *next;
} __attribute__((packed));

bool klist_empty(struct klist *root);
void klist_push(struct klist *root, struct klist_node *node);
void klist_pop(struct klist *root, struct klist_node *node);

struct kqueue
{
    size_t head;
    size_t tail;
    size_t capacity;
    void **data;
};

void kqueue_init(struct kqueue *kqueue, size_t capacity);
void kqueue_deinit(struct kqueue *kqueue);
bool kqueue_enqueue(struct kqueue *kqueue, void *item);
bool kqueue_dequeue(struct kqueue *kqueue, void **item);
bool kqueue_peek(struct kqueue *kqueue, void **item);

// Statically sized (and non optimized) map
struct kmap
{
    size_t capacity;
    size_t key_size;
    size_t val_size;
    void *keys;
    void *vals;
    u64 *bitmap;
};

void kmap_init(struct kmap *kmap, size_t capacity, size_t key_size, size_t val_size);
void kmap_new(struct kmap *kmap, void *key, void *val);
void kmap_get(struct kmap *kmap, void *key, void *val);
void kmap_del(struct kmap *kmap, void *key);
bool kmap_contains(struct kmap *kmap, void *key);