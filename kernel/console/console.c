#include "console.h"

#define VGA_MEMORY 0xB8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_COLOR  0x07

#define COM1_PORT 0x3F8

static int vga_row;
static int vga_col;

static inline void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port)
{
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void)
{
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
}

static int serial_is_transmit_empty(void)
{
    return inb(COM1_PORT + 5) & 0x20;
}

static void serial_write_char(char character)
{
    while (!serial_is_transmit_empty()) {
    }

    outb(COM1_PORT, (unsigned char)character);
}

static void vga_put_char(char character)
{
    if (character == '\n') {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT) {
            vga_row = VGA_HEIGHT - 1;
        }
        return;
    }

    const unsigned int index = (unsigned int)(vga_row * VGA_WIDTH + vga_col);
    volatile unsigned short *vga = (volatile unsigned short *)VGA_MEMORY;
    vga[index] = (unsigned short)character | ((unsigned short)VGA_COLOR << 8);

    vga_col++;
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
        if (vga_row >= VGA_HEIGHT) {
            vga_row = VGA_HEIGHT - 1;
        }
    }
}

void console_init(void)
{
    vga_row = 0;
    vga_col = 0;
    serial_init();
}

void console_write(const char *message)
{
    for (const char *cursor = message; *cursor != '\0'; cursor++) {
        serial_write_char(*cursor);
        vga_put_char(*cursor);
    }
}

void console_write_line(const char *message)
{
    console_write(message);
    serial_write_char('\n');
    vga_put_char('\n');
}
