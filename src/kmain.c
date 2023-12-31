#include <vga.h>

void kmain()
{
    struct framebuffer fb;
    fb.fgc = green;
    fb.bgc = black;
    fb.bright = true;

    vga_clear(&fb);
    vga_printf(&fb, "Kernel at your service!\n");
}