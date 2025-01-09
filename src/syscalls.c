#include <io.h>
#include <vga.h>
#include <intr.h>
#include <syscalls.h>

extern void kernel_stack;
extern void syscall_handler();

static struct kernel_root kernel_root_struct;

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

        default:
            break;
    }

    return 0;
}
