#include "net/ipv4/ipv4.h"

#include "net/link/link.h"

static ipv4_config_t config;

static int on_same_subnet(ipv4_addr_t dest)
{
    return (dest.bytes[0] & config.subnet.bytes[0]) == (config.address.bytes[0] & config.subnet.bytes[0])
        && (dest.bytes[1] & config.subnet.bytes[1]) == (config.address.bytes[1] & config.subnet.bytes[1])
        && (dest.bytes[2] & config.subnet.bytes[2]) == (config.address.bytes[2] & config.subnet.bytes[2])
        && (dest.bytes[3] & config.subnet.bytes[3]) == (config.address.bytes[3] & config.subnet.bytes[3]);
}

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
    config.address.bytes[0] = 0;
    config.address.bytes[1] = 0;
    config.address.bytes[2] = 0;
    config.address.bytes[3] = 0;
    config.subnet = config.address;
    config.gateway = config.address;
}

void ipv4_set_config(const ipv4_config_t *new_config)
{
    config = *new_config;
}

const ipv4_config_t *ipv4_get_config(void)
{
    return &config;
}

int ipv4_send(ipv4_addr_t dest, const uint8_t *payload, uint16_t length, uint8_t protocol)
{
    uint8_t packet[1600];
    if ((uint32_t)length + 20U > sizeof(packet)) {
        return -1;
    }

    packet[0] = 0x45;
    packet[1] = 0;
    uint16_t total = (uint16_t)(20U + length);
    packet[2] = (uint8_t)(total >> 8);
    packet[3] = (uint8_t)(total & 0xFFU);
    packet[4] = 0;
    packet[5] = 0;
    packet[6] = 0;
    packet[7] = 0;
    packet[8] = 64;
    packet[9] = protocol;
    packet[10] = 0;
    packet[11] = 0;
    packet[12] = config.address.bytes[0];
    packet[13] = config.address.bytes[1];
    packet[14] = config.address.bytes[2];
    packet[15] = config.address.bytes[3];
    packet[16] = dest.bytes[0];
    packet[17] = dest.bytes[1];
    packet[18] = dest.bytes[2];
    packet[19] = dest.bytes[3];

    for (uint16_t i = 0; i < length; i++) {
        packet[20 + i] = payload[i];
    }

    uint16_t csum = ipv4_checksum(packet, 20);
    packet[10] = (uint8_t)(csum >> 8);
    packet[11] = (uint8_t)(csum & 0xFFU);

    ipv4_addr_t next_hop = on_same_subnet(dest) ? dest : config.gateway;
    return link_send_ipv4(next_hop, packet, total, protocol);
}
