#pragma once

#include <vmm.h>
#include <intr.h>
#include <util.h>
#include <sync.h>

// Thread/Task control block
struct tcb 
{
    u64 tid;

    struct global_context regintr_ctx;
    struct page_table *vmm_ctx;

    mutex_t lock;
    struct klist_node list_node;

} __attribute__((packed));

void tcb_init(struct tcb *tcb);

void tcb_schedule(struct tcb *task);

// Scheduler struct
struct scheduler
{
    mutex_t lock;
    bool initialized;
    struct tcb *curr_task;
    struct klist task_list;
};

void scheduler_init(struct scheduler *sched);
void scheduler_add_task(struct scheduler *sched, struct tcb *tcb);
void scheduler_kickstart(struct scheduler *sched, struct tcb *task);
struct tcb* scheduler_schedule(struct scheduler *sched, struct global_context *saved_ctx);