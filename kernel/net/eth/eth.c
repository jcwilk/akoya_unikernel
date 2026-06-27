#include "net/eth/eth.h"

#include "net/eth/rtl8139.h"

int eth_init(eth_device_t *dev)
{
    return rtl8139_init(dev);
}

int eth_send(eth_device_t *dev, const uint8_t *frame, uint16_t length)
{
    if (dev == 0 || dev->send == 0) {
        return -1;
    }
    return dev->send(dev, frame, length);
}

int eth_poll(eth_device_t *dev, uint8_t *frame, uint16_t capacity, uint16_t *length_out)
{
    if (dev == 0 || dev->poll == 0) {
        return -1;
    }
    return dev->poll(dev, frame, capacity, length_out);
}

const eth_addr_t *eth_mac(const eth_device_t *dev)
{
    if (dev == 0) {
        return 0;
    }
    return &dev->mac;
}
