#include <util.h>
#include <tasks.h>
#include <syscalls.h>

extern void tss_load(u64);
extern void schedule_task(struct tcb *task); // src/asm/interrupts.asm

// Basic init of tcb
void tcb_init(struct tcb *tcb)
{
    // basic zero out init
    bzero((u8*)&tcb->regintr_ctx.cpu_context, sizeof(struct cpu_context));
    bzero((u8*)&tcb->regintr_ctx.intr_context, sizeof(struct interrupt_context));
    // same here
    tcb->list_node.valid = false;
    tcb->list_node.next = (struct klist_node*)-1;
    // and here
    mutex_init(&tcb->lock);
    // and here
    tcb->state = RUNNABLE;
}

void tcb_schedule(struct tcb *task)
{
    schedule_task(task);
}

void scheduler_init(struct scheduler *sched)
{
    mutex_init(&sched->lock);
    sched->task_list.valid = false;
    sched->task_list.root = (struct klist_node*)-1;
}

void scheduler_add_task(struct scheduler *sched, struct tcb *tcb)
{
    klist_push(&sched->task_list, &tcb->list_node);
}

void scheduler_kickstart(struct scheduler *sched, struct tcb *task)
{
    scheduler_add_task(sched, task);
    sched->curr_task = task;
    sched->initialized = true;
    tcb_schedule(task);
}

struct tcb* scheduler_schedule(struct scheduler *sched, struct global_context *saved_ctx)
{
    // Simple single core round robin scheduler
    // Make sure scheduler is initialized...
    if(!sched->initialized) 
    {
        return (void*)-1;
    }
    else
    {
        // Save current context
        memcpy((void*)&sched->curr_task->regintr_ctx, saved_ctx, sizeof(struct global_context));
        // Search for next runnable task
        return scheduler_schedule_no_safe(sched);
    }
}

// Simple round robin scheduling
struct tcb* scheduler_schedule_no_safe(struct scheduler *sched)
{
    // Search for next runnable task
    do {
        if(sched->curr_task->list_node.valid) {
            sched->curr_task = ENCLAVE(struct tcb, list_node, sched->curr_task->list_node.next);
            if(sched->curr_task->state == RUNNABLE)
                break;
        }
        else {
            // Start from the beginning
            sched->curr_task = ENCLAVE(struct tcb, list_node, sched->task_list.root);
            if(sched->curr_task->state == RUNNABLE)
                break;
        }
    } while(1);

    // TODO: Remove
    kprintf("Scheudling task: [%d]\n", sched->curr_task->tid);

    // Return actual task
    return sched->curr_task;
}

// Wakes up tasks (flags them runnable again and executes completion callback) that are waiting for a certain event (i.e. have a certain task state)
void scheduler_wakeup(struct scheduler *sched, enum task_state state)
{
    if(!sched->initialized)
        return;

    // Start at the first task
    struct tcb *ct = ENCLAVE(struct tcb, list_node, sched->task_list.root);
    // Iterate through task list
    while(ct->list_node.valid)
    {
        // Check for right state
        if(ct->state == state) {
            // Run completion callback
            ct->completion_callback(ct);
            // Mark as runnable again
            ct->state = RUNNABLE;
        }
        // Next task
        ct = ENCLAVE(struct tcb, list_node, ct->list_node.next);
    }
}

// Saves the new cpu_context into the curren task
// This method is intended to be used in the syscall handler
void scheduler_update_current_cpu_context(
    struct scheduler *sched, 
    struct kernel_root *kernel_root_struct, 
    struct cpu_context *cpu_ctx)
{
    // First copy new context
    memcpy((void*)&sched->curr_task->regintr_ctx.cpu_context, 
           (void*)cpu_ctx, 
           sizeof(struct cpu_context));

    // Copy values saved by the syscall handler
    sched->curr_task->regintr_ctx.intr_context.rip = kernel_root_struct->user_rip;
    sched->curr_task->regintr_ctx.intr_context.rsp = kernel_root_struct->user_stack;
    sched->curr_task->regintr_ctx.intr_context.rflags = kernel_root_struct->user_flags;
}