#ifndef AKOYA_NET_DHCP_H
#define AKOYA_NET_DHCP_H

#include "net/ipv4/ipv4.h"

typedef enum {
    DHCP_OK = 0,
    DHCP_FAIL_TIMEOUT,
    DHCP_FAIL_NO_OFFER,
    DHCP_FAIL_NO_ACK
} dhcp_status_t;

dhcp_status_t dhcp_acquire(ipv4_config_t *config_out, uint32_t timeout_ms);

#endif
