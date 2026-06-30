#ifndef AKOYA_NET_RTL8139_H
#define AKOYA_NET_RTL8139_H

#include "net/eth/eth.h"

int rtl8139_init(eth_device_t *dev);

void rtl8139_drain_tx(void);

#endif
