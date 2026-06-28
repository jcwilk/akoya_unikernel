#include "input/ps2_readline.h"

#include "input/ps2_keyboard.h"

int ps2_read_line(char *buf, int cap)
{
    if (cap <= 0) {
        return -1;
    }

    int len = 0;
    buf[0] = '\0';

    for (;;) {
        unsigned char scan = 0;
        if (ps2_poll_scancode(&scan) != 0) {
            continue;
        }

        if (ps2_process_scancode(scan, buf, cap, &len) != 0) {
            return len;
        }
    }
}
