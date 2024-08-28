#include <io.h>
#include <apic.h>
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
    // Load the idt
    __asm__ volatile("lidt %0" : : "m"(*idtr));
}

void idt_register_handler(struct interrupt_descriptor_table *idt, u64 id, u64 handler_addr, u16 options)
{
    // Craft idt entry
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

static struct interrupt_descriptor_table idt __attribute__((aligned(64)));                 // Alignment for better performance
static struct interrupt_descriptor_table_descriptor idtr __attribute__((aligned(16)));
/*
 * Setup idt and register interrupts
*/
void intr_setup()
{
    idt_init(&idt);

    idt_register_handler(&idt, 0, (u64)isr_0, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 1, (u64)isr_1, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 2, (u64)isr_2, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 3, (u64)isr_3, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 4, (u64)isr_4, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 5, (u64)isr_5, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 6, (u64)isr_6, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 7, (u64)isr_7, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 8, (u64)isr_8, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 9, (u64)isr_9, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 10, (u64)isr_10, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 11, (u64)isr_11, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 12, (u64)isr_12, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 13, (u64)isr_13, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 14, (u64)isr_14, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 15, (u64)isr_15, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 16, (u64)isr_16, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 17, (u64)isr_17, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 18, (u64)isr_18, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 19, (u64)isr_19, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 20, (u64)isr_20, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 21, (u64)isr_21, IDT_PRESENT | IDT_INT_GATE);
    // The missing interrupts here are reserved
    idt_register_handler(&idt, 32, (u64)isr_32, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 33, (u64)isr_33, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 34, (u64)isr_34, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 35, (u64)isr_35, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 36, (u64)isr_36, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 37, (u64)isr_37, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 38, (u64)isr_38, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 39, (u64)isr_39, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 40, (u64)isr_40, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 41, (u64)isr_41, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 42, (u64)isr_42, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 43, (u64)isr_43, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 44, (u64)isr_44, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 45, (u64)isr_45, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 46, (u64)isr_46, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 47, (u64)isr_47, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 48, (u64)isr_48, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 49, (u64)isr_49, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 50, (u64)isr_50, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 51, (u64)isr_51, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 52, (u64)isr_52, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 53, (u64)isr_53, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 54, (u64)isr_54, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 55, (u64)isr_55, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 56, (u64)isr_56, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 57, (u64)isr_57, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 58, (u64)isr_58, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 59, (u64)isr_59, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 60, (u64)isr_60, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 61, (u64)isr_61, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 62, (u64)isr_62, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 63, (u64)isr_63, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 64, (u64)isr_64, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 65, (u64)isr_65, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 66, (u64)isr_66, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 67, (u64)isr_67, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 68, (u64)isr_68, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 69, (u64)isr_69, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 70, (u64)isr_70, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 71, (u64)isr_71, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 72, (u64)isr_72, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 73, (u64)isr_73, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 74, (u64)isr_74, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 75, (u64)isr_75, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 76, (u64)isr_76, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 77, (u64)isr_77, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 78, (u64)isr_78, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 79, (u64)isr_79, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 80, (u64)isr_80, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 81, (u64)isr_81, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 82, (u64)isr_82, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 83, (u64)isr_83, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 84, (u64)isr_84, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 85, (u64)isr_85, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 86, (u64)isr_86, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 87, (u64)isr_87, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 88, (u64)isr_88, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 89, (u64)isr_89, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 90, (u64)isr_90, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 91, (u64)isr_91, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 92, (u64)isr_92, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 93, (u64)isr_93, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 94, (u64)isr_94, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 95, (u64)isr_95, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 96, (u64)isr_96, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 97, (u64)isr_97, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 98, (u64)isr_98, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 99, (u64)isr_99, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 100, (u64)isr_100, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 101, (u64)isr_101, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 102, (u64)isr_102, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 103, (u64)isr_103, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 104, (u64)isr_104, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 105, (u64)isr_105, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 106, (u64)isr_106, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 107, (u64)isr_107, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 108, (u64)isr_108, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 109, (u64)isr_109, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 110, (u64)isr_110, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 111, (u64)isr_111, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 112, (u64)isr_112, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 113, (u64)isr_113, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 114, (u64)isr_114, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 115, (u64)isr_115, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 116, (u64)isr_116, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 117, (u64)isr_117, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 118, (u64)isr_118, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 119, (u64)isr_119, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 120, (u64)isr_120, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 121, (u64)isr_121, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 122, (u64)isr_122, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 123, (u64)isr_123, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 124, (u64)isr_124, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 125, (u64)isr_125, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 126, (u64)isr_126, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 127, (u64)isr_127, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 128, (u64)isr_128, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 129, (u64)isr_129, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 130, (u64)isr_130, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 131, (u64)isr_131, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 132, (u64)isr_132, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 133, (u64)isr_133, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 134, (u64)isr_134, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 135, (u64)isr_135, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 136, (u64)isr_136, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 137, (u64)isr_137, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 138, (u64)isr_138, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 139, (u64)isr_139, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 140, (u64)isr_140, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 141, (u64)isr_141, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 142, (u64)isr_142, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 143, (u64)isr_143, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 144, (u64)isr_144, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 145, (u64)isr_145, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 146, (u64)isr_146, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 147, (u64)isr_147, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 148, (u64)isr_148, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 149, (u64)isr_149, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 150, (u64)isr_150, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 151, (u64)isr_151, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 152, (u64)isr_152, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 153, (u64)isr_153, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 154, (u64)isr_154, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 155, (u64)isr_155, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 156, (u64)isr_156, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 157, (u64)isr_157, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 158, (u64)isr_158, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 159, (u64)isr_159, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 160, (u64)isr_160, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 161, (u64)isr_161, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 162, (u64)isr_162, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 163, (u64)isr_163, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 164, (u64)isr_164, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 165, (u64)isr_165, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 166, (u64)isr_166, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 167, (u64)isr_167, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 168, (u64)isr_168, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 169, (u64)isr_169, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 170, (u64)isr_170, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 171, (u64)isr_171, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 172, (u64)isr_172, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 173, (u64)isr_173, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 174, (u64)isr_174, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 175, (u64)isr_175, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 176, (u64)isr_176, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 177, (u64)isr_177, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 178, (u64)isr_178, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 179, (u64)isr_179, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 180, (u64)isr_180, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 181, (u64)isr_181, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 182, (u64)isr_182, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 183, (u64)isr_183, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 184, (u64)isr_184, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 185, (u64)isr_185, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 186, (u64)isr_186, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 187, (u64)isr_187, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 188, (u64)isr_188, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 189, (u64)isr_189, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 190, (u64)isr_190, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 191, (u64)isr_191, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 192, (u64)isr_192, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 193, (u64)isr_193, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 194, (u64)isr_194, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 195, (u64)isr_195, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 196, (u64)isr_196, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 197, (u64)isr_197, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 198, (u64)isr_198, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 199, (u64)isr_199, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 200, (u64)isr_200, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 201, (u64)isr_201, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 202, (u64)isr_202, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 203, (u64)isr_203, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 204, (u64)isr_204, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 205, (u64)isr_205, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 206, (u64)isr_206, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 207, (u64)isr_207, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 208, (u64)isr_208, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 209, (u64)isr_209, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 210, (u64)isr_210, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 211, (u64)isr_211, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 212, (u64)isr_212, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 213, (u64)isr_213, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 214, (u64)isr_214, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 215, (u64)isr_215, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 216, (u64)isr_216, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 217, (u64)isr_217, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 218, (u64)isr_218, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 219, (u64)isr_219, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 220, (u64)isr_220, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 221, (u64)isr_221, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 222, (u64)isr_222, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 223, (u64)isr_223, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 224, (u64)isr_224, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 225, (u64)isr_225, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 226, (u64)isr_226, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 227, (u64)isr_227, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 228, (u64)isr_228, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 229, (u64)isr_229, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 230, (u64)isr_230, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 231, (u64)isr_231, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 232, (u64)isr_232, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 233, (u64)isr_233, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 234, (u64)isr_234, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 235, (u64)isr_235, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 236, (u64)isr_236, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 237, (u64)isr_237, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 238, (u64)isr_238, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 239, (u64)isr_239, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 240, (u64)isr_240, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 241, (u64)isr_241, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 242, (u64)isr_242, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 243, (u64)isr_243, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 244, (u64)isr_244, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 245, (u64)isr_245, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 246, (u64)isr_246, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 247, (u64)isr_247, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 248, (u64)isr_248, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 249, (u64)isr_249, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 250, (u64)isr_250, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 251, (u64)isr_251, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 252, (u64)isr_252, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 253, (u64)isr_253, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 254, (u64)isr_254, IDT_PRESENT | IDT_INT_GATE);
    idt_register_handler(&idt, 255, (u64)isr_255, IDT_PRESENT | IDT_INT_GATE);

    // Craft idt pointer
    idtr.limit = (256 * 16) - 1;
    idtr.base = (u64)&idt;

    idt_register(&idtr);
}

void pic_init()
{
    u8 icw1  = (1 << 4) | 1; // Initialize & IC4
    u8 icw2p = 0x20;         // Offset for interruipts
    u8 icw2s = 0x28;         // Offset for interruipts
    u8 icw3p = 0x4;
    u8 icw3s = 0x2;
    u8 icw4  = 0x1;

    // Start init
    outb(PIC_PRI_CMD, icw1);
    outb(PIC_SEC_CMD, icw1);

    // Send ICWs...
    outb(PIC_PRI_DATA, icw2p);
    outb(PIC_SEC_DATA, icw2s);

    outb(PIC_PRI_DATA, icw3p);
    outb(PIC_SEC_DATA, icw3s);

    outb(PIC_PRI_DATA, icw4);
    outb(PIC_SEC_DATA, icw4);

    // Null data regs
    outb(PIC_PRI_DATA, 0);
    outb(PIC_SEC_DATA, 0);
}

u16 pic_get_mask()
{
    u16 u = (u16)inb(PIC_SEC_DATA);
    u16 l = (u16)inb(PIC_PRI_DATA);
    return  (u << 8) | l;
}

void pic_set_mask(u16 mask)
{
    u8 u = (mask >> 8) & 0xff;
    u8 l = (mask >> 0) & 0xff;

    outb(PIC_PRI_DATA, l);
    outb(PIC_SEC_DATA, u);
}

void pic_eoi(u8 irq)
{
    if(irq < 8)
        outb(PIC_PRI_CMD, 0x20);

    outb(PIC_SEC_CMD, 0x20);
}

void pic_disable()
{
    // Mask all interrupts
    pic_set_mask(0xFFFF);
}

/*
 * context: saved cpu context
 * code: number of interrupt/exception
*/
struct cpu_context* intr_handler(struct cpu_context* saved_context, u64 code)
{
    kprintf("Interrupt [%d]\n", code);

    if(code == 0x20)
    {
        pic_eoi(0);
    }

    if(code == 0x21)
    {
        keyboard_handle_keypress();

        u8 buf[128] = {0};
        keyboard_data(buf, 4);
        kprintf("Buf: %s\n", buf);
        
        pic_eoi(1);
    }

    if(code == 242)
    {
        lapic_end_of_int(lapic_fetch());
    }

    return saved_context;
}
