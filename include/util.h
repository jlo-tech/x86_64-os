#pragma once

#include <types.h>

#define KTREE_LEFT  0
#define KTREE_RIGHT 1

struct ktree
{
    bool valid[2];
    struct ktree *left;
    struct ktree *right;
    struct ktree *parent;
} __attribute__((packed));

void bzero(u8 *mem, u64 size);

void ktree_insert(struct ktree *root, struct ktree *node, 
                  int off, int (*cmp)(void*, void*));

void ktree_remove(struct ktree *root, struct ktree *node, 
                  int off, int (*cmp)(void*, void*));