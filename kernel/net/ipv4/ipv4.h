#ifndef AKOYA_NET_IPV4_H
#define AKOYA_NET_IPV4_H

#include <stdint.h>

#include "net/nettypes.h"

typedef struct {
    ipv4_addr_t address;
    ipv4_addr_t subnet;
    ipv4_addr_t gateway;
} ipv4_config_t;

void ipv4_init(void);
void ipv4_set_config(const ipv4_config_t *config);
const ipv4_config_t *ipv4_get_config(void);
uint16_t ipv4_checksum(const void *data, int length);
int ipv4_send(ipv4_addr_t dest, const uint8_t *payload, uint16_t length, uint8_t protocol);

#endif
