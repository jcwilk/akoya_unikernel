#ifndef AKOYA_INPUT_PS2_KEYBOARD_H
#define AKOYA_INPUT_PS2_KEYBOARD_H

#include "event/device.h"

#define PS2_LINE_MAX 128
#define PS2_SCAN_QUEUE_SIZE 64

void ps2_keyboard_init(void);
void ps2_keyboard_register_device(void);
deferred_device_t *ps2_keyboard_device(void);

int ps2_keyboard_has_scancode(void);
int ps2_keyboard_pop_scancode(unsigned char *scan_out);

int ps2_poll_scancode(unsigned char *scan_out);
char ps2_scancode_to_ascii(unsigned char scan);
int ps2_process_scancode(unsigned char scan, char *buf, int cap, int *len_io);

#endif
