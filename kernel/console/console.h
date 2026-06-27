#ifndef AKOYA_CONSOLE_H
#define AKOYA_CONSOLE_H

void console_init(void);
void console_write(const char *message);
void console_write_line(const char *message);
void console_backspace(void);
void console_newline(void);
void console_write_prompt(const char *prompt);
void console_clear(void);

#endif
