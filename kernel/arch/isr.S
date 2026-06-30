.section .text

.extern arch_irq_dispatch

.macro IRQ_STUB num
.global irq_stub_\num
irq_stub_\num:
    pushl $\num
    call arch_irq_dispatch
    addl $4, %esp
    iret
.endm

IRQ_STUB 1
IRQ_STUB 11
