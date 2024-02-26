#pragma once

#include <types.h>

#define KTREE_LEFT  0
#define KTREE_RIGHT 1

struct ktree
{
    bool valid;
    struct ktree_node *root;
};

struct ktree_node
{
    bool valid[2];
    struct ktree_node *left;
    struct ktree_node *right;
    struct ktree_node *parent;
} __attribute__((packed));

void bzero(u8 *mem, u64 size);

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
};

struct klist_node
{
    bool valid;
    struct klist_node *next;
};

bool klist_empty(struct klist *root);
void klist_push(struct klist *root, struct klist_node *node);
void klist_pop(struct klist *root, struct klist_node *node);