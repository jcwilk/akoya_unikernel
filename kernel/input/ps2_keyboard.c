#include "input/ps2_keyboard.h"

#include "console/console.h"
#include "io/io.h"

#define PS2_DATA_PORT   0x60
#define PS2_STATUS_PORT 0x64
#define PS2_STATUS_OUT  0x01

static int shift_active;

static char scan_to_ascii(unsigned char scan_code, int shifted)
{
    if (shifted) {
        switch (scan_code) {
        case 0x02: return '!';
        case 0x03: return '@';
        case 0x04: return '#';
        case 0x05: return '$';
        case 0x06: return '%';
        case 0x07: return '^';
        case 0x08: return '&';
        case 0x09: return '*';
        case 0x0A: return '(';
        case 0x0B: return ')';
        case 0x0C: return '_';
        case 0x0D: return '+';
        case 0x1A: return '{';
        case 0x1B: return '}';
        case 0x27: return ':';
        case 0x28: return '"';
        case 0x29: return '~';
        case 0x2B: return '|';
        case 0x33: return '<';
        case 0x34: return '>';
        case 0x35: return '?';
        default:   break;
        }
    }

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
    case 0x10: return shifted ? 'Q' : 'q';
    case 0x11: return shifted ? 'W' : 'w';
    case 0x12: return shifted ? 'E' : 'e';
    case 0x13: return shifted ? 'R' : 'r';
    case 0x14: return shifted ? 'T' : 't';
    case 0x15: return shifted ? 'Y' : 'y';
    case 0x16: return shifted ? 'U' : 'u';
    case 0x17: return shifted ? 'I' : 'i';
    case 0x18: return shifted ? 'O' : 'o';
    case 0x19: return shifted ? 'P' : 'p';
    case 0x1A: return '[';
    case 0x1B: return ']';
    case 0x1E: return shifted ? 'A' : 'a';
    case 0x1F: return shifted ? 'S' : 's';
    case 0x20: return shifted ? 'D' : 'd';
    case 0x21: return shifted ? 'F' : 'f';
    case 0x22: return shifted ? 'G' : 'g';
    case 0x23: return shifted ? 'H' : 'h';
    case 0x24: return shifted ? 'J' : 'j';
    case 0x25: return shifted ? 'K' : 'k';
    case 0x26: return shifted ? 'L' : 'l';
    case 0x27: return ';';
    case 0x28: return '\'';
    case 0x29: return '`';
    case 0x2B: return '\\';
    case 0x2C: return shifted ? 'Z' : 'z';
    case 0x2D: return shifted ? 'X' : 'x';
    case 0x2E: return shifted ? 'C' : 'c';
    case 0x2F: return shifted ? 'V' : 'v';
    case 0x30: return shifted ? 'B' : 'b';
    case 0x31: return shifted ? 'N' : 'n';
    case 0x32: return shifted ? 'M' : 'm';
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
    shift_active = 0;
    ps2_drain_output();
    outb(PS2_STATUS_PORT, 0xAE);
    ps2_wait_input();
    ps2_drain_output();
}

static int ps2_handle_modifier(unsigned char scan, int is_break)
{
    if (scan == 0x2A || scan == 0x36) {
        shift_active = is_break ? 0 : 1;
        return 1;
    }
    return 0;
}

int ps2_poll_scancode(unsigned char *scan_out)
{
    if (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUT)) {
        return 1;
    }

    unsigned char scan = inb(PS2_DATA_PORT);
    if (scan == 0xE0) {
        ps2_wait_input();
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT) {
            (void)inb(PS2_DATA_PORT);
        }
        return 1;
    }

    int is_break = (scan & 0x80) != 0;
    unsigned char make = (unsigned char)(scan & 0x7FU);

    if (ps2_handle_modifier(make, is_break)) {
        return 1;
    }

    if (is_break) {
        return 1;
    }

    *scan_out = make;
    return 0;
}

char ps2_scancode_to_ascii(unsigned char scan)
{
    return scan_to_ascii(scan, shift_active);
}

int ps2_process_scancode(unsigned char scan, char *buf, int cap, int *len_io)
{
    if (scan == 0x1C) {
        console_newline();
        return 1;
    }

    if (scan == 0x0E) {
        if (*len_io > 0) {
            (*len_io)--;
            buf[*len_io] = '\0';
            console_backspace();
        }
        return 0;
    }

    char ch = scan_to_ascii(scan, shift_active);
    if (ch == '\0') {
        return 0;
    }

    if (*len_io + 1 >= cap) {
        return 0;
    }

    buf[(*len_io)++] = ch;
    buf[*len_io] = '\0';

    char echo[2] = { ch, '\0' };
    console_write(echo);
    return 0;
}
