#ifndef AKOYA_CONSOLE_H
#define AKOYA_CONSOLE_H

void console_init(void);
void console_write(const char *message);
void console_write_line(const char *message);
void console_backspace(void);

#endif
