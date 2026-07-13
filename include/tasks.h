#pragma once

#include <vmm.h>
#include <intr.h>
#include <util.h>
#include <sync.h>
#include <syscalls.h>

enum task_state {
    RUNNABLE,
    WAITING_FOR_TIME,
    WAITING_FOR_KEYPRESS,
};

enum task_type {
    KERNEL_TASK,
    USER_TASK
};

// Thread/Task control block
struct tcb 
{
    // Task id
    u64 tid;

    // Register and Paging data
    struct global_context regintr_ctx;
    struct page_table *vmm_ctx;

    // Task list management
    mutex_t lock;
    struct klist_node list_node;

    // Task state
    enum task_state state;
    
    // This callback function is run on a sleeping task 
    // before it is woken up again
    void (*completion_callback)(struct tcb*);

    // Task type
    enum task_type type;

} __attribute__((packed));

void tcb_init(struct tcb *tcb);

void tcb_schedule(struct tcb *task);

// Scheduler struct // TODO: Make use of multicore
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
struct tcb* scheduler_get_current_task(struct scheduler *sched);
struct tcb* scheduler_schedule(
    struct scheduler *sched, 
    struct global_context *saved_ctx);
struct tcb* scheduler_schedule_no_safe(struct scheduler *sched);
void scheduler_wakeup(struct scheduler *sched, enum task_state state);
void scheduler_update_current_cpu_context(
    struct scheduler *sched, 
    struct kernel_root *kernel_root_struct, 
    struct cpu_context *cpu_ctx);
