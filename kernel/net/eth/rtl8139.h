#ifndef AKOYA_NET_RTL8139_H
#define AKOYA_NET_RTL8139_H

#include "net/eth/eth.h"

int rtl8139_init(eth_device_t *dev);

void rtl8139_drain_tx(void);
void rtl8139_irq_mask(void);
void rtl8139_irq_unmask(void);
int rtl8139_has_pending_rx(void);

#endif
