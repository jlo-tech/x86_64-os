#include <keyboard.h>

#define KBD_CMD  0x64
#define KBD_DATA 0x60 

static bool keyboard_data_available()
{
    return inb(KBD_CMD) & 0x1;
}

static u8 keyboard_poll()
{
    return inb(KBD_DATA);
}

unsigned char kbmap[128] =
{
    0,  27, 
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 
    '-', '=', 
    '\b', /* Backspace */
    '\t', /* Tab */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 
    '[', ']', 
    '\n', /* Enter key */
    0,	  /* Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    '\'', '`', 0,	/* Left shift */
    '\\', 
    'z', 'x', 'c', 'v', 'b', 'n',
  ' m', ',', '.', '/',   
    0,				/* Right shift */
    '*',
    0,	 /* Alt */
    ' ', /* Space bar */
    0,	 /* Caps lock */
    0,	 /* F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
    '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
    '+',
    0,	/* End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

static u8 keyboard_translate_scancode(u8 scancode)
{
    return kbmap[scancode];
}

/* Implement ring buffer */
#define RBUF_SIZE 256
static i64 read_ptr = 0;
static i64 write_ptr = 0;
static u8 ring_buffer[RBUF_SIZE] = {0};

static bool alt_pressed = false;
static bool ctrl_pressed = false;
static bool shift_pressed = false;

static bool keyboard_empty()
{
    return read_ptr == write_ptr;
}

// Number of available elements
static i64 keyboard_count()
{
    i64 r = read_ptr - write_ptr;
    return (r < 0) ? -r : r;
}

void keyboard_handle_keypress()
{
    if(!keyboard_data_available())
        return;

    u8 scancode = keyboard_poll();

    if(scancode & 0x80)
    {
        // Key was released

        // Remove release notification
        scancode ^= 0x80;

        // Relase keys 
        switch(scancode)
        {
            case 0x2A: // Shift left
                shift_pressed = false;
                break;

            case 0x36: // Shift right
                shift_pressed = false;
                break;

            case 0x1D: // Ctrl
                ctrl_pressed = false;
                break;

            case 0x38: // Alt
                alt_pressed = false;
                break;

            default:
                break;
        }
    }
    else
    {
        switch(scancode)
        {
            // Handle special codes
           
            case 0xe: // Backspace
                ring_buffer[write_ptr-- % RBUF_SIZE] = 0;
                break;

            case 0x2A: // Shift left
                shift_pressed = true;
                break;

            case 0x36: // Shift right
                shift_pressed = true;
                break;

            case 0x1D: // Ctrl
                ctrl_pressed = true;
                break;

            case 0x38: // Alt
                alt_pressed = true;
                break;

            // Handle normal chars
            default:
                u8 ch = keyboard_translate_scancode(scancode);
                ring_buffer[write_ptr++ % RBUF_SIZE] = ch;
                break;
        }
    }
}

bool keyboard_alt()
{
    return alt_pressed;
}

bool keyboard_shift()
{
    return shift_pressed;
}

bool keyboard_ctrl()
{
    return ctrl_pressed;
}

void keyboard_data(u8 *buf, u64 max_size)
{
    u64 read_size = (max_size < keyboard_count()) ? max_size : keyboard_count();

    for(i64 i = read_size; i > 0; i--)
    {
        *buf++ = ring_buffer[read_ptr++ % RBUF_SIZE];
    }
}