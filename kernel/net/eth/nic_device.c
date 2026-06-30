#include "net/eth/nic_device.h"

#include "arch/irq.h"
#include "net/eth/rtl8139.h"
#include "net/link/link.h"

static eth_device_t *nic;
static deferred_device_t nic_dev;

static int nic_has_more(void)
{
    return rtl8139_has_pending_rx();
}

static void nic_drain_one(void)
{
    link_drain_rx(1);
}

static void nic_mask_irq(void)
{
    rtl8139_irq_mask();
}

static void nic_unmask_irq(void)
{
    rtl8139_irq_unmask();
}

void nic_device_init(eth_device_t *dev)
{
    nic = dev;
    nic_dev.irq = IRQ_NIC;
    nic_dev.pending = 0;
    nic_dev.isr_pull = 0;
    nic_dev.has_more = nic_has_more;
    nic_dev.drain_one = nic_drain_one;
    nic_dev.mask_irq = nic_mask_irq;
    nic_dev.unmask_irq = nic_unmask_irq;
    device_register(&nic_dev);
    pic_unmask(IRQ_NIC);
}

deferred_device_t *nic_device(void)
{
    return &nic_dev;
}
