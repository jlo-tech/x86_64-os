#include <vga.h>
#include <stdarg.h>

static u16 *framebuffer_address = (u16*)0xb8000;

static const u16 num2char[10] = {'0', 
                                 '1', 
                                 '2', 
                                 '3', 
                                 '4', 
                                 '5', 
                                 '6', 
                                 '7', 
                                 '8', 
                                 '9'};

void vga_clear(struct framebuffer *fb)
{
    // Clear screen
    for(u16 i = 0; i < 80*25; i++)
    {
        *(framebuffer_address+i) = 0;
    }
    // Reset cursor position
    fb->pos = 0;
}

u16 vga_craft_char(struct framebuffer *fb, u16 c)
{

    u16 r = ((u16)fb->fgc) << 8 | c;

    if(fb->bright)
    {
        r = (1 << 11) | r;
    }

    r = ((u16)fb->bgc) << 12 | r;

    if(fb->blink)
    {
        r = (1 << 15) | r;
    }

    return r;
}

void vga_next_line(struct framebuffer* fb)
{
    fb->pos = fb->pos - (fb->pos % 80) + 80;
}

void vga_print_char(struct framebuffer *fb, char c)
{
    if(c == '\n')
    {
        vga_next_line(fb);
    }
    else 
    {
        *(framebuffer_address + (fb->pos % (80*25))) = vga_craft_char(fb, (u16)c);
        fb->pos++;
    }
}

void vga_print_str(struct framebuffer *fb, char *s)
{
    while(*s != '\0')
    {
        vga_print_char(fb, (u16)*s);
        s++;
    }
}

void vga_print_uint(struct framebuffer *fb, u64 n)
{
    int i = 0;    // Position
    u16 buf[32];  // Buffer to hold string representation

    if(n == 0)
    {
        vga_print_char(fb, '0');
        return;
    }
    else 
    {
        // Preparation
        while(n > 0)
        {
            buf[i] = vga_craft_char(fb, (u16)num2char[n % 10]);
            n = n / 10;
            i++;
        }
        // Go back one
        i--;
        // Actual printing
        while(buf[i] != '\0')
        {
            vga_print_char(fb, buf[i]);
            i--;
        }        
    }
}

void vga_print_int(struct framebuffer *fb, i64 n)
{
    if(n < 0)
    {
        vga_print_char(fb, '-');
    }

    // abs()
    n = (n < 0) ? -n : n;
    
    vga_print_uint(fb, n);
}

void vga_printf(struct framebuffer *fb, char *s, ...)
{
    va_list arg;
    va_start(arg, s);

    int i = 0;
    int len = 0;
    while(*(s+len) != '\0')
    {
        len++;
    }

    while(*s != '\0')
    {
        if(*s == '%' && i < (len - 1))
        {
            switch(*(s+1))
            {
                case 'c':
                    vga_print_char(fb, va_arg(arg, int));
                    break;
                case 's':
                    vga_print_str(fb, va_arg(arg, char*));
                    break;
                case 'u':
                    vga_print_uint(fb, va_arg(arg, u64));
                    break;
                case 'd':
                    vga_print_int(fb, va_arg(arg, i64));
                    break;
            }
            // Skip formating character
            s++;
        }
        else
        {
            // Just print plain string
            vga_print_char(fb, (u16)*s);
        }
        // Go to next character
        s++;
        i++;
    }
}