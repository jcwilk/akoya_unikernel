#include "event/runtime.h"

#include "arch/irq.h"
#include "event/device.h"
#include "net/dhcp/dhcp.h"
#include "net/link/link.h"
#include "net/lwip/lwip_pump.h"
#include "time/timer.h"
#include "time/time.h"

#define RUNTIME_SLOT_MAX 8
#define RUNTIME_IDLE_THRESHOLD_MS 2U

typedef struct {
    runtime_step_fn step;
    runtime_active_fn active;
} runtime_slot_t;

static runtime_slot_t slots[RUNTIME_SLOT_MAX];
static int slot_count;
static deferred_device_t *keyboard_dev;
static deferred_device_t *nic_dev;
static int runtime_halt_requested;

void runtime_set_keyboard_device(deferred_device_t *dev)
{
    keyboard_dev = dev;
}

void runtime_set_nic_device(deferred_device_t *dev)
{
    nic_dev = dev;
}

void runtime_init(void)
{
    slot_count = 0;
    runtime_halt_requested = 0;
    arch_irq_init();
    device_init();
    timer_init();
}

void runtime_register_slot(runtime_step_fn step, runtime_active_fn active)
{
    if (slot_count >= RUNTIME_SLOT_MAX || step == 0) {
        return;
    }
    slots[slot_count].step = step;
    slots[slot_count].active = active;
    slot_count++;
}

void runtime_request_halt(void)
{
    runtime_halt_requested = 1;
}

static int runtime_slots_active(void)
{
    for (int i = 0; i < slot_count; i++) {
        if (slots[i].active == 0 || slots[i].active()) {
            return 1;
        }
    }
    return 0;
}

static void runtime_service_devices(void)
{
    if (keyboard_dev != 0 && keyboard_dev->pending) {
        if (device_bottom_half_step(keyboard_dev)) {
            keyboard_dev->pending = 1;
        }
    }
    if (nic_dev != 0 && nic_dev->pending) {
        if (device_bottom_half_step(nic_dev)) {
            nic_dev->pending = 1;
        }
    }
}

static int runtime_quiescent(void)
{
    if (device_any_pending()) {
        return 0;
    }
    if (runtime_slots_active()) {
        return 0;
    }
    if (timer_any_due()) {
        return 0;
    }
    return 1;
}

int runtime_pump_once(void)
{
    device_poll_registered();
    timer_poll();
    runtime_service_devices();

    if (dhcp_link_rx_active()) {
        link_poll();
    } else {
        lwip_stack_pump(8);
    }

    for (int i = 0; i < slot_count; i++) {
        if (slots[i].active == 0 || slots[i].active()) {
            slots[i].step();
        }
    }

    return runtime_halt_requested;
}

void runtime_run_until_halt(void)
{
    arch_sti();

    for (;;) {
        if (runtime_pump_once()) {
            break;
        }

        if (runtime_quiescent()) {
            uint32_t next = timer_next_deadline_ms();
            uint32_t now = time_millis();
            if (next != 0xFFFFFFFFU && next > now && (next - now) > RUNTIME_IDLE_THRESHOLD_MS) {
                device_poll_registered();
                __asm__ volatile ("hlt");
            } else {
                device_poll_registered();
                __asm__ volatile ("pause");
            }
        }
    }
}

void runtime_run_forever(void)
{
    runtime_run_until_halt();
    for (;;) {
        __asm__ volatile ("hlt");
    }
}
