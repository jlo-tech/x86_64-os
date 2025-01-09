#pragma once

#include <intr.h>
#include <util.h>
#include <sync.h>

extern void dummy_task();

// Thread/Task control block
struct tcb 
{
    u64 tid;

    struct cpu_context cpu_ctx;
    struct interrupt_context int_ctx;

    struct klist_node list_node;

    mutex_t lock;
} __attribute__((packed));

void tcb_init(struct tcb *tcb);
