#include "time/time.h"

#include "io/io.h"

#define PIT_BASE_HZ        1193182UL
#define PIT_CH0_DATA       0x40U
#define PIT_CH2_DATA       0x42U
#define PIT_CMD            0x43U
#define PIT_CTRL_PORT      0x61U
#define PIT_CALIB_MS       50U
#define TSC_PER_MS_MIN     50000U
#define TSC_PER_MS_MAX     5000000U

static uint64_t tsc_start;
static uint32_t tsc_per_ms;

static inline uint64_t rdtsc64(void)
{
    uint32_t lo;
    uint32_t hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

static uint32_t div_u64_u32(uint64_t dividend, uint32_t divisor)
{
    if (divisor == 0U) {
        return 0U;
    }
    if (dividend <= 0xFFFFFFFFULL) {
        return (uint32_t)dividend / divisor;
    }

    uint32_t quotient = 0U;
    uint64_t remainder = 0U;
    for (int bit = 63; bit >= 0; bit--) {
        remainder = (remainder << 1) | ((dividend >> bit) & 1ULL);
        if (remainder >= divisor) {
            remainder -= divisor;
            if (bit < 32) {
                quotient |= (1UL << bit);
            }
        }
    }
    return quotient;
}

static uint32_t calibrate_tsc_per_ms(void)
{
    const uint16_t divisor = (uint16_t)((PIT_BASE_HZ * (uint32_t)PIT_CALIB_MS) / 1000UL);

    /* Channel 2 one-shot; gate on port 0x61 bit 5 (OSDev PIT frequency measure). */
    outb(PIT_CMD, 0xB2U);
    outb(PIT_CH2_DATA, (uint8_t)(divisor & 0xFFU));
    outb(PIT_CH2_DATA, (uint8_t)((divisor >> 8) & 0xFFU));

    uint8_t val = inb(PIT_CTRL_PORT);
    outb(PIT_CTRL_PORT, (uint8_t)((val & 0xFDU) | 0x01U));

    uint32_t guard = 0U;
    while ((inb(PIT_CTRL_PORT) & 0x20U) == 0U) {
        if (++guard > 10000000U) {
            return 100000U;
        }
    }

    uint64_t tsc_begin = rdtsc64();
    guard = 0U;
    while ((inb(PIT_CTRL_PORT) & 0x20U) != 0U) {
        if (++guard > 10000000U) {
            return 100000U;
        }
    }

    uint64_t tsc_delta = rdtsc64() - tsc_begin;
    uint32_t per_ms = div_u64_u32(tsc_delta, (uint32_t)PIT_CALIB_MS);
    if (per_ms < TSC_PER_MS_MIN) {
        return TSC_PER_MS_MIN;
    }
    if (per_ms > TSC_PER_MS_MAX) {
        return TSC_PER_MS_MAX;
    }
    return per_ms;
}

void time_init(void)
{
    tsc_per_ms = calibrate_tsc_per_ms();
    tsc_start = rdtsc64();
}

void time_poll(void)
{
}

uint32_t time_millis(void)
{
    return div_u64_u32(rdtsc64() - tsc_start, tsc_per_ms);
}

void time_delay_ms(uint32_t milliseconds)
{
    uint32_t deadline = time_millis() + milliseconds;
    while (time_millis() < deadline) {
        __asm__ volatile ("pause");
    }
}
