#include "input/ps2_keyboard.h"

#include "console/console.h"
#include "io/io.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_STATUS_OUT  0x01

static char scan_to_ascii(unsigned char scan_code)
{
    switch (scan_code) {
    case 0x02: return '1';
    case 0x03: return '2';
    case 0x04: return '3';
    case 0x05: return '4';
    case 0x06: return '5';
    case 0x07: return '6';
    case 0x08: return '7';
    case 0x09: return '8';
    case 0x0A: return '9';
    case 0x0B: return '0';
    case 0x0C: return '-';
    case 0x0D: return '=';
    case 0x10: return 'q';
    case 0x11: return 'w';
    case 0x12: return 'e';
    case 0x13: return 'r';
    case 0x14: return 't';
    case 0x15: return 'y';
    case 0x16: return 'u';
    case 0x17: return 'i';
    case 0x18: return 'o';
    case 0x19: return 'p';
    case 0x1A: return '[';
    case 0x1B: return ']';
    case 0x1E: return 'a';
    case 0x1F: return 's';
    case 0x20: return 'd';
    case 0x21: return 'f';
    case 0x22: return 'g';
    case 0x23: return 'h';
    case 0x24: return 'j';
    case 0x25: return 'k';
    case 0x26: return 'l';
    case 0x27: return ';';
    case 0x28: return '\'';
    case 0x29: return '`';
    case 0x2B: return '\\';
    case 0x2C: return 'z';
    case 0x2D: return 'x';
    case 0x2E: return 'c';
    case 0x2F: return 'v';
    case 0x30: return 'b';
    case 0x31: return 'n';
    case 0x32: return 'm';
    case 0x33: return ',';
    case 0x34: return '.';
    case 0x35: return '/';
    case 0x39: return ' ';
    default:   return '\0';
    }
}

static void ps2_wait_input(void)
{
    for (int i = 0; i < 100000; i++) {
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT) {
            return;
        }
    }
}

static void ps2_drain_output(void)
{
    while (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT) {
        (void)inb(PS2_DATA_PORT);
    }
}

void ps2_keyboard_init(void)
{
    ps2_drain_output();
    outb(PS2_STATUS_PORT, 0xAE);
    ps2_wait_input();
    ps2_drain_output();
}

static int ps2_read_scancode(unsigned char *scan_out)
{
    for (;;) {
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT) {
            unsigned char scan = inb(PS2_DATA_PORT);
            if (scan == 0xE0) {
                ps2_wait_input();
                if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT) {
                    (void)inb(PS2_DATA_PORT);
                }
                continue;
            }
            if (scan & 0x80) {
                continue;
            }
            *scan_out = scan;
            return 0;
        }
    }
}

int ps2_read_line(char *buf, int cap)
{
    if (cap <= 0) {
        return -1;
    }

    int len = 0;
    buf[0] = '\0';

    for (;;) {
        unsigned char scan = 0;
        if (ps2_read_scancode(&scan) != 0) {
            return -1;
        }

        if (scan == 0x1C) {
            console_write_line("");
            return len;
        }

        if (scan == 0x0E) {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                console_backspace();
            }
            continue;
        }

        char ch = scan_to_ascii(scan);
        if (ch == '\0') {
            continue;
        }

        if (len + 1 >= cap) {
            continue;
        }

        buf[len++] = ch;
        buf[len] = '\0';

        char echo[2] = { ch, '\0' };
        console_write(echo);
    }
}
