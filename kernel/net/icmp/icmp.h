#ifndef AKOYA_NET_ICMP_H
#define AKOYA_NET_ICMP_H

#include <stdint.h>

#include "net/nettypes.h"

typedef enum {
    ICMP_OK = 0,
    ICMP_FAIL_TIMEOUT,
    ICMP_FAIL_UNREACHABLE,
    ICMP_FAIL_NO_REPLY
} icmp_status_t;

icmp_status_t icmp_ping(ipv4_addr_t target, uint32_t timeout_ms, uint32_t *rtt_ms_out);

#endif
