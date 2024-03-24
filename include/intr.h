#pragma once

#include <isr.h>
#include <vga.h>
#include <util.h>
#include <keyboard.h>

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

void intr_setup();

struct cpu_context
{
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;

    u64 rbp;

    u64 rdi;
    u64 rsi;

    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
};

// Primary PIC
#define PIC_PRI_CMD  0x20
#define PIC_PRI_DATA 0x21
// Secondary PIC
#define PIC_SEC_CMD  0xA0
#define PIC_SEC_DATA 0xA1

void pic_init();
u16  pic_get_mask();
void pic_set_mask(u16 mask);
void pic_eoi(u8 irq);

struct cpu_context* intr_handler(struct cpu_context* saved_context, u64 code);