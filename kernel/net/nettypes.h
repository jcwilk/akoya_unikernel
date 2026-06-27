#ifndef AKOYA_NET_TYPES_H
#define AKOYA_NET_TYPES_H

#include <stdint.h>

#define ETH_ADDR_LEN 6

typedef struct {
    uint8_t bytes[ETH_ADDR_LEN];
} eth_addr_t;

typedef struct {
    uint8_t bytes[4];
} ipv4_addr_t;

static inline uint16_t net_be16(uint16_t value)
{
    return (uint16_t)(((value & 0x00FFU) << 8) | ((value & 0xFF00U) >> 8));
}

static inline uint32_t net_be32(uint32_t value)
{
    return ((value & 0x000000FFU) << 24)
        | ((value & 0x0000FF00U) << 8)
        | ((value & 0x00FF0000U) >> 8)
        | ((value & 0xFF000000U) >> 24);
}

static inline int ipv4_is_zero(ipv4_addr_t addr)
{
    return addr.bytes[0] == 0 && addr.bytes[1] == 0
        && addr.bytes[2] == 0 && addr.bytes[3] == 0;
}

static inline int ipv4_equal(ipv4_addr_t a, ipv4_addr_t b)
{
    return a.bytes[0] == b.bytes[0] && a.bytes[1] == b.bytes[1]
        && a.bytes[2] == b.bytes[2] && a.bytes[3] == b.bytes[3];
}

#endif
