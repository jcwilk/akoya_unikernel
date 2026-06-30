#include "arch/irq.h"

#include <stdint.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];

static void idt_set_gate(int vector, void (*handler)(void))
{
    uint32_t addr = (uint32_t)(uintptr_t)handler;
    idt[vector].offset_low = (uint16_t)(addr & 0xFFFFU);
    idt[vector].selector = 0x08;
    idt[vector].zero = 0;
    idt[vector].type_attr = 0x8E;
    idt[vector].offset_high = (uint16_t)((addr >> 16) & 0xFFFFU);
}

static void idt_load(void)
{
    struct idt_ptr ptr;
    ptr.limit = (uint16_t)(sizeof(idt) - 1U);
    ptr.base = (uint32_t)(uintptr_t)idt;
    __asm__ volatile ("lidt %0" : : "m"(ptr));
}

extern void irq_stub_1(void);
extern void irq_stub_11(void);

void arch_irq_init(void)
{
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0);
    }

    idt_set_gate(IDT_VECTOR_IRQ0 + IRQ_KEYBOARD, irq_stub_1);
    idt_set_gate(IDT_VECTOR_IRQ0 + IRQ_NIC, irq_stub_11);

    idt_load();
    pic_init();
}

void arch_sti(void)
{
    __asm__ volatile ("sti");
}

void arch_cli(void)
{
    __asm__ volatile ("cli");
}
