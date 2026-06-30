#include "time/timer.h"

#include "event/runtime.h"
#include "time/time.h"

typedef struct {
    int in_use;
    uint32_t deadline_ms;
    timer_callback_fn fn;
    void *ctx;
} timer_entry_t;

static timer_entry_t timers[TIMER_MAX];
static volatile int timer_wait_flag;

static void timer_wait_done(void *ctx)
{
    (void)ctx;
    timer_wait_flag = 1;
}

void timer_init(void)
{
    for (int i = 0; i < TIMER_MAX; i++) {
        timers[i].in_use = 0;
    }
    timer_wait_flag = 0;
}

int timer_schedule(uint32_t delay_ms, timer_callback_fn fn, void *ctx)
{
    uint32_t deadline = time_millis() + delay_ms;

    for (int i = 0; i < TIMER_MAX; i++) {
        if (!timers[i].in_use) {
            timers[i].in_use = 1;
            timers[i].deadline_ms = deadline;
            timers[i].fn = fn;
            timers[i].ctx = ctx;
            return i;
        }
    }
    return TIMER_HANDLE_INVALID;
}

void timer_cancel(int handle)
{
    if (handle < 0 || handle >= TIMER_MAX) {
        return;
    }
    timers[handle].in_use = 0;
}

void timer_poll(void)
{
    uint32_t now = time_millis();

    for (int i = 0; i < TIMER_MAX; i++) {
        if (!timers[i].in_use) {
            continue;
        }
        if (now >= timers[i].deadline_ms) {
            timer_callback_fn fn = timers[i].fn;
            void *ctx = timers[i].ctx;
            timers[i].in_use = 0;
            if (fn != 0) {
                fn(ctx);
            }
        }
    }
}

uint32_t timer_next_deadline_ms(void)
{
    uint32_t best = 0xFFFFFFFFU;

    for (int i = 0; i < TIMER_MAX; i++) {
        if (!timers[i].in_use) {
            continue;
        }
        if (timers[i].deadline_ms < best) {
            best = timers[i].deadline_ms;
        }
    }
    return best;
}

int timer_any_due(void)
{
    uint32_t now = time_millis();

    for (int i = 0; i < TIMER_MAX; i++) {
        if (timers[i].in_use && now >= timers[i].deadline_ms) {
            return 1;
        }
    }
    return 0;
}

void timer_wait_ms(uint32_t delay_ms)
{
    timer_wait_flag = 0;
    int handle = timer_schedule(delay_ms, timer_wait_done, 0);
    if (handle == TIMER_HANDLE_INVALID) {
        time_delay_ms(delay_ms);
        return;
    }

    while (!timer_wait_flag) {
        (void)runtime_pump_once();
    }
    timer_cancel(handle);
}
