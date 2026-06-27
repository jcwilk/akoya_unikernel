#include "time/time.h"

static uint32_t tsc_start;
static uint32_t tsc_per_ms;

static inline uint32_t rdtsc32(void)
{
    uint32_t lo;
    __asm__ volatile ("rdtsc" : "=a"(lo) : : "edx");
    return lo;
}

void time_init(void)
{
    tsc_start = rdtsc32();
    tsc_per_ms = 100000U;
}

void time_poll(void)
{
}

uint32_t time_millis(void)
{
    return (rdtsc32() - tsc_start) / tsc_per_ms;
}

void time_delay_ms(uint32_t milliseconds)
{
    uint32_t deadline = time_millis() + milliseconds;
    while (time_millis() < deadline) {
        __asm__ volatile ("pause");
    }
}
