#include <io.h>
#include <vga.h>
#include <intr.h>
#include <tasks.h>
#include <syscalls.h>

extern void kernel_stack;
extern struct scheduler rrsched;

extern void syscall_handler();
extern void return_to_task_from_syscall(struct tcb*);

struct kernel_root kernel_root_struct;

void syscalls_setup()
{
    // Disable interrupts but leave rest as it is
    wmsr(MSR_IA32_FMASK, 1 << 9);

    // Set handler address
    wmsr(MSR_IA32_LSTAR, (u64)syscall_handler);

    // Init kernel root struct
    kernel_root_struct.kernel_stack = (u64)&kernel_stack;

    // Set root struct for syscall handler
    wmsr(MSR_IA32_KERNEL_GS_BASE, (u64)&kernel_root_struct);

    // Set segment selectors in STAR register
    // NOTE: The manual specifies that certains offsets are added to selector values
    // therefore they do not exactly correspond to gdt entries
    u64 cal_sel = (u64)((1 << 3) | 0) << 32;
    u64 ret_sel = (u64)((2 << 3) | 3) << 48;
    wmsr(MSR_IA32_STAR, ret_sel | cal_sel | (rmsr(MSR_IA32_STAR) & 0xFFFFFFFF));

    // Enable syscalls in IA32_EFER MSR
    wmsr(MSR_IA32_EFER, rmsr(MSR_IA32_EFER) | 1);
}

/* Syscalls */

void do_print(u8* str)
{
    kprintf("%s", str);
}

static void scan_completion_callback(struct tcb *task)
{
    task->regintr_ctx.cpu_context.rax = keyboard_data(
        (u8*)task->regintr_ctx.cpu_context.rsi,
        (i64)task->regintr_ctx.cpu_context.rdx);
}

i64 do_scan(u8* buf, i64 size, struct cpu_context *ctx)
{
    // Only return when enter was pressed
    if(keyboard_enter())
        return keyboard_data(buf, size);
    else {
        // Set completion callback
        rrsched.curr_task->completion_callback = scan_completion_callback;
        // And make current process sleep
        rrsched.curr_task->state = WAITING_FOR_KEYPRESS;
        // Then save current cpu context
        scheduler_update_current_cpu_context(&rrsched, &kernel_root_struct, ctx);
        // And switch to another task
        return_to_task_from_syscall(scheduler_schedule_no_safe(&rrsched));
    }
    return 0;
}

/*
 * Syscall handler
 */
u64 do_syscall(struct cpu_context *ctx)
{
    kprintf("Syscall no %d\n", ctx->rdi);

    switch(ctx->rdi)
    {
        case 0:
            do_print((u8*)ctx->rsi);
            break;
    
        case 1:
            ctx->rax = do_scan((u8*)ctx->rsi, (i64)ctx->rdx, ctx);
            break;

        default:
            break;
    }

    return 0;
}
