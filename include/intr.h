#pragma once

#include <util.h>

#define IDT_INT_GATE    (0b1110 << 8)  // Disables interrupts on handler entry
#define IDT_TRAP_GATE   (0b1111 << 8)  // Allows interrupts during handler running
#define IDT_PRIV(level) ((level & 0b11) << 13) // Priv. level allowed to access this interrupt
#define IDT_PRESENT     (1 << 15) // Sets int handler valid

struct interrupt_descriptor
{
    u16 fn_low;
    u16 selector;
    u16 options;
    u16 fn_mid;
    u32 fn_high;
    u32 reserved;
} __attribute__((packed));

struct interrupt_descriptor_table 
{
    struct interrupt_descriptor desc[256];
} __attribute__((packed));

struct interrupt_descriptor_table_descriptor
{
    u16 limit;
    u64 base;
} __attribute__((packed));

void idt_init(struct interrupt_descriptor_table *idt);
void idt_register(struct interrupt_descriptor_table_descriptor *idtr);
void idt_register_handler(struct interrupt_descriptor_table *idt, u64 id, u64 handler_addr, u16 options);

void intr_enable();
void intr_disable();