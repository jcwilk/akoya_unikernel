#include "arch/irq.h"

#include "io/io.h"

#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

#define ICW1_INIT 0x11
#define ICW4_8086 0x01

void pic_init(void)
{
    outb(PIC1_CMD, ICW1_INIT);
    outb(PIC2_CMD, ICW1_INIT);
    outb(PIC1_DATA, IDT_VECTOR_IRQ0);
    outb(PIC2_DATA, (uint8_t)(IDT_VECTOR_IRQ0 + 8));
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_eoi(int irq)
{
    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);
    }
    outb(PIC1_CMD, 0x20);
}

static uint16_t pic_read_masks(void)
{
    uint8_t m1 = inb(PIC1_DATA);
    uint8_t m2 = inb(PIC2_DATA);
    return (uint16_t)((uint16_t)m2 << 8) | m1;
}

static void pic_write_masks(uint16_t masks)
{
    outb(PIC1_DATA, (uint8_t)(masks & 0xFFU));
    outb(PIC2_DATA, (uint8_t)((masks >> 8) & 0xFFU));
}

void pic_mask(int irq)
{
    uint16_t masks = pic_read_masks();
    masks |= (uint16_t)(1U << irq);
    pic_write_masks(masks);
}

void pic_unmask(int irq)
{
    uint16_t masks = pic_read_masks();
    masks &= (uint16_t)~(1U << irq);
    pic_write_masks(masks);
}
