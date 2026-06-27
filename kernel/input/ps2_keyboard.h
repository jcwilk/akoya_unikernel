#ifndef AKOYA_INPUT_PS2_KEYBOARD_H
#define AKOYA_INPUT_PS2_KEYBOARD_H

#define PS2_LINE_MAX 128

void ps2_keyboard_init(void);
int ps2_read_line(char *buf, int cap);

#endif
