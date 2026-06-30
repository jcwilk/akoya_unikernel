#ifndef AKOYA_EVENT_RUNTIME_H
#define AKOYA_EVENT_RUNTIME_H

typedef void (*runtime_step_fn)(void);
typedef int (*runtime_active_fn)(void);

struct deferred_device;

void runtime_init(void);
void runtime_set_keyboard_device(struct deferred_device *dev);
void runtime_set_nic_device(struct deferred_device *dev);
void runtime_register_slot(runtime_step_fn step, runtime_active_fn active);
void runtime_request_halt(void);
void runtime_run_forever(void);
void runtime_run_until_halt(void);
int runtime_pump_once(void);

#endif
