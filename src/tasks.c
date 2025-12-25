#include <tasks.h>

extern void schedule_task(struct tcb *task); // src/asm/interrupts.asm

// Basic init of tcb
void tcb_init(struct tcb *tcb)
{
    // basic zero out init
    bzero((u8*)&tcb->regintr_ctx.cpu_context, sizeof(struct cpu_context));
    bzero((u8*)&tcb->regintr_ctx.intr_context, sizeof(struct interrupt_context));
    // same here
    bzero((u8*)&tcb->list_node, sizeof(struct klist_node));
    // and here
    mutex_init(&tcb->lock);
}

void tcb_schedule(struct tcb *task)
{
    schedule_task(task);
}

void scheduler_init(struct scheduler *sched)
{
    mutex_init(&sched->lock);
    bzero((void*)&sched->task_list, sizeof(struct klist));
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

    if(!sched->initialized) 
    {
        return (void*)-1;
    }
    else
    {
        // Save current context
        memcpy((void*)&sched->curr_task->regintr_ctx, saved_ctx, sizeof(struct global_context));
        // Get next task
        if(sched->curr_task->list_node.valid) {
            sched->curr_task = ENCLAVE(struct tcb, list_node, sched->curr_task->list_node.next);
        }
        else {
            // Start from the beginning
            sched->curr_task = ENCLAVE(struct tcb, list_node, sched->task_list.root);
        }
        return sched->curr_task;
    }
}