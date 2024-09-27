#pragma once

#include <types.h>

// Basic Atomic Operations

void atomic_add(i64 *atomic_var, i64 x);
void atomic_sub(i64 *atomic_var, i64 x);

bool atomic_tas(bool *atomic_flag);
void atomic_clr(bool *atomic_flag);

// Mutex

typedef bool mutex_t;

void mutex_init(mutex_t *mtx);
void mutex_lock(mutex_t *mtx);
void mutex_unlock(mutex_t *mtx);

// Semaphore

typedef struct
{
    i64 val;
    mutex_t lock;
} semaphore_t;

void semaphore_init(semaphore_t *sem, i64 val);
void semaphore_inc(semaphore_t *sem);
void semaphore_dec(semaphore_t *sem);
