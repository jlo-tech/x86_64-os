#include <tasks.h>

extern void schedule_task(struct tcb *task); // src/asm/interrupts.asm

// Basic init of tcb
void tcb_init(struct tcb *tcb)
{
    // basic zero out init
    bzero((u8*)&tcb->cpu_ctx, sizeof(struct cpu_context));
    bzero((u8*)&tcb->int_ctx, sizeof(struct interrupt_context));
    // same here
    bzero((u8*)&tcb->list_node, sizeof(struct klist_node));
    // and here
    mutex_init(&tcb->lock);
}

void tcb_schedule(struct tcb *task)
{
    schedule_task(task);
}