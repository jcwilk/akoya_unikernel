#ifndef AKOYA_NET_NIC_DEVICE_H
#define AKOYA_NET_NIC_DEVICE_H

#include "event/device.h"
#include "net/eth/eth.h"

void nic_device_init(eth_device_t *dev);
deferred_device_t *nic_device(void);

#endif
