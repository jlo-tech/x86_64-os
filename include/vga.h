#pragma once

#include <types.h>

enum vga_color {
    black, 
    blue,
    green,
    cyan, 
    red, 
    magenta, 
    brown,
    gray
};

struct framebuffer 
{
    enum vga_color fgc;
    u8 bright;
    enum vga_color bgc;
    u8 blink;
    u16 pos;
};

void vga_clear();
void vga_print_char(struct framebuffer *fb, char c);
void vga_print_str(struct framebuffer *fb, char *s);
void vga_print_uint(struct framebuffer *fb, u64 n);
void vga_print_hex(struct framebuffer *fb, u64 n);
void vga_print_int(struct framebuffer *fb, i64 n);

void vga_printf(struct framebuffer *fb, char *s, ...);

void kprintf(char *s, ...);
void kclear();