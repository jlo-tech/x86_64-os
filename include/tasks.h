#pragma once

#include <vmm.h>
#include <intr.h>
#include <util.h>
#include <sync.h>

// Thread/Task control block
struct tcb 
{
    u64 tid;

    struct cpu_context cpu_ctx;
    struct interrupt_context int_ctx;
    struct page_table *vmm_ctx;

    mutex_t lock;
    struct klist_node list_node;

} __attribute__((packed));

void tcb_init(struct tcb *tcb);

void tcb_schedule(struct tcb *task);

// Scheduler struct
struct sched 
{
    mutex_t lock;
    struct klist_node *task_list;
};

// TODO: Implement scheduler