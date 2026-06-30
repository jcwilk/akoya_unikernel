#ifndef AKOYA_TIME_TIMER_H
#define AKOYA_TIME_TIMER_H

#include <stdint.h>

typedef void (*timer_callback_fn)(void *ctx);

#define TIMER_MAX 16
#define TIMER_HANDLE_INVALID (-1)

void timer_init(void);
int timer_schedule(uint32_t delay_ms, timer_callback_fn fn, void *ctx);
void timer_cancel(int handle);
void timer_poll(void);
uint32_t timer_next_deadline_ms(void);
int timer_any_due(void);
void timer_wait_ms(uint32_t delay_ms);

#endif
