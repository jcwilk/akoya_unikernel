#ifndef AKOYA_EVENT_DEVICE_H
#define AKOYA_EVENT_DEVICE_H

typedef struct deferred_device {
    int irq;
    volatile int pending;
    int (*isr_pull)(void);
    int (*has_more)(void);
    void (*drain_one)(void);
    void (*mask_irq)(void);
    void (*unmask_irq)(void);
} deferred_device_t;

#define DEVICE_DRAIN_SLICE_MAX 4

void device_init(void);
void device_register(deferred_device_t *dev);
void device_irq_top_half(int irq);
int device_bottom_half_step(deferred_device_t *dev);
void device_poll_registered(void);
int device_any_pending(void);

#endif
