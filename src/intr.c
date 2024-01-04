#include <intr.h>

#define BIT16_MASK 0xffff
#define BIT32_MASK 0xffffffff

void idt_init(struct interrupt_descriptor_table *idt)
{
    // Clear idt
    bzero((u8*)idt, sizeof(struct interrupt_descriptor_table));
}

void idt_register(struct interrupt_descriptor_table_descriptor *idtr)
{
    __asm__ volatile("lidt %0" : : "m"(*idtr));
}

void idt_register_handler(struct interrupt_descriptor_table *idt, u64 id, u64 handler_addr, u16 options)
{
    struct interrupt_descriptor desc;
    desc.fn_low   = handler_addr;
    desc.selector = 0x8;
    desc.options  = options;
    desc.fn_mid   = handler_addr >> 16;
    desc.fn_high  = handler_addr >> 32;
    desc.reserved = 0;
    // Eventually register handler in idt
    idt->desc[id] = desc;
}

void intr_enable()
{
    __asm__ volatile("sti");
}

void intr_disable()
{
    __asm__ volatile("cli");
}