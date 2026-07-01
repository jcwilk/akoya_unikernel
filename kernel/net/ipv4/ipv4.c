#include "net/ipv4/ipv4.h"

#include "net/lwip/lwip_pump.h"

static ipv4_config_t fallback_config;

uint16_t ipv4_checksum(const void *data, int length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0;

    while (length > 1) {
        sum += (uint32_t)((bytes[0] << 8) | bytes[1]);
        bytes += 2;
        length -= 2;
    }

    if (length == 1) {
        sum += (uint32_t)bytes[0] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFFU) + (sum >> 16);
    }

    return (uint16_t)~sum;
}

void ipv4_init(void)
{
    fallback_config.address.bytes[0] = 0;
    fallback_config.address.bytes[1] = 0;
    fallback_config.address.bytes[2] = 0;
    fallback_config.address.bytes[3] = 0;
    fallback_config.subnet = fallback_config.address;
    fallback_config.gateway = fallback_config.address;
}

void ipv4_set_config(const ipv4_config_t *new_config)
{
    if (new_config == 0) {
        return;
    }

    fallback_config = *new_config;
    if (lwip_stack_device() != 0) {
        lwip_stack_apply_ipv4_config(new_config);
    }
}

const ipv4_config_t *ipv4_get_config(void)
{
    static ipv4_config_t live_config;

    if (lwip_stack_has_ip()) {
        live_config = lwip_stack_ipv4_config();
        return &live_config;
    }

    return &fallback_config;
}

int ipv4_send(ipv4_addr_t dest, const uint8_t *payload, uint16_t length, uint8_t protocol)
{
    (void)dest;
    (void)payload;
    (void)length;
    (void)protocol;
    return -1;
}
