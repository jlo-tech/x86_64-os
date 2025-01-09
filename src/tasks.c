#include <tasks.h>

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

