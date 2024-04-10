#include <user_mode.h>

struct task_state_segment tss;

extern void kernel_stack;
extern void global_descriptor_table_pointer;
extern void global_descriptor_table;
extern void tss_load();

void tss_init()
{
    // Clear
    bzero((u8*)&tss, sizeof(struct task_state_segment));

    // Set values of tss
    tss.rsp0 = (u64)&kernel_stack;
    tss.iopb = sizeof(struct task_state_segment);       // No IOPB (since tss limit == IOPB)

    // Modify gdt's tss entry
    struct tss_descriptor *tssd = (struct tss_descriptor*)(((u8*)&global_descriptor_table)+(8*5));
    tssd->limit     = sizeof(struct task_state_segment);
    tssd->base_low  = (((u64)&tss) >> 0)  & 0xFFFF;
    tssd->base_mid  = (((u64)&tss) >> 16) & 0xFF;
    tssd->base_high = (((u64)&tss) >> 24) & 0xFF;
    tssd->flags     = (1 << 7) | (3 << 5) | 9;         // Present | DPL | Type

    // Load new tss
    tss_load((5 << 3) | 3);
}