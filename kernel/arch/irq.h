#ifndef AKOYA_ARCH_IRQ_H
#define AKOYA_ARCH_IRQ_H

#include <stdint.h>

#define IRQ_KEYBOARD 1
#define IRQ_NIC      11

#define IDT_VECTOR_IRQ0 0x20

void arch_irq_init(void);
void arch_sti(void);
void arch_cli(void);

void pic_init(void);
void pic_eoi(int irq);
void pic_mask(int irq);
void pic_unmask(int irq);

void arch_irq_dispatch(int irq);

#endif
