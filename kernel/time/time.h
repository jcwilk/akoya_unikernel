#ifndef AKOYA_TIME_H
#define AKOYA_TIME_H

#include <stdint.h>

void time_init(void);
void time_poll(void);
uint32_t time_millis(void);
void time_delay_ms(uint32_t milliseconds);

#endif
