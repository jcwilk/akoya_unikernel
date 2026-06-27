#ifndef AKOYA_NET_ETH_H
#define AKOYA_NET_ETH_H

#include <stdint.h>

#include "net/nettypes.h"

struct eth_device;

typedef struct eth_device {
    eth_addr_t mac;
    int (*init)(struct eth_device *dev);
    int (*send)(struct eth_device *dev, const uint8_t *frame, uint16_t length);
    int (*poll)(struct eth_device *dev, uint8_t *frame, uint16_t capacity, uint16_t *length_out);
} eth_device_t;

int eth_init(eth_device_t *dev);
int eth_send(eth_device_t *dev, const uint8_t *frame, uint16_t length);
int eth_poll(eth_device_t *dev, uint8_t *frame, uint16_t capacity, uint16_t *length_out);
const eth_addr_t *eth_mac(const eth_device_t *dev);

#endif
