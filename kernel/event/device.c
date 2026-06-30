#include "event/device.h"

#include "arch/irq.h"

#define DEVICE_MAX 4

static deferred_device_t *devices[DEVICE_MAX];
static int device_count;

void device_init(void)
{
    device_count = 0;
}

void device_register(deferred_device_t *dev)
{
    if (dev == 0 || device_count >= DEVICE_MAX) {
        return;
    }
    dev->pending = 0;
    devices[device_count++] = dev;
}

static deferred_device_t *device_for_irq(int irq)
{
    for (int i = 0; i < device_count; i++) {
        if (devices[i]->irq == irq) {
            return devices[i];
        }
    }
    return 0;
}

void device_irq_top_half(int irq)
{
    deferred_device_t *dev = device_for_irq(irq);
    if (dev == 0) {
        pic_eoi(irq);
        return;
    }

    if (dev->isr_pull != 0) {
        (void)dev->isr_pull();
    }

    dev->pending = 1;
    if (dev->mask_irq != 0) {
        dev->mask_irq();
    }
    pic_eoi(irq);
}

int device_bottom_half_step(deferred_device_t *dev)
{
    if (dev == 0 || !dev->pending) {
        return 0;
    }

    int drained = 0;
    while (drained < DEVICE_DRAIN_SLICE_MAX) {
        if (dev->has_more == 0 || !dev->has_more()) {
            break;
        }
        if (dev->drain_one != 0) {
            dev->drain_one();
        }
        drained++;
    }

    if (dev->has_more != 0 && dev->has_more()) {
        return 1;
    }

    if (dev->unmask_irq != 0) {
        dev->unmask_irq();
    }

    if (dev->has_more != 0 && dev->has_more()) {
        if (dev->drain_one != 0) {
            dev->drain_one();
        }
        if (dev->has_more()) {
            return 1;
        }
    }

    dev->pending = 0;
    return 0;
}

void device_poll_registered(void)
{
    for (int i = 0; i < device_count; i++) {
        deferred_device_t *dev = devices[i];
        if (dev->has_more != 0 && dev->has_more()) {
            dev->pending = 1;
        }
    }
}

int device_any_pending(void)
{
    for (int i = 0; i < device_count; i++) {
        if (devices[i]->pending) {
            return 1;
        }
    }
    return 0;
}

void arch_irq_dispatch(int irq)
{
    device_irq_top_half(irq);
}
