#ifndef AKOYA_INPUT_PS2_KEYBOARD_H
#define AKOYA_INPUT_PS2_KEYBOARD_H

#define PS2_LINE_MAX 128

void ps2_keyboard_init(void);
int ps2_poll_scancode(unsigned char *scan_out);
char ps2_scancode_to_ascii(unsigned char scan);
int ps2_process_scancode(unsigned char scan, char *buf, int cap, int *len_io);

#endif
