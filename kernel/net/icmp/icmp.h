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

typedef struct {
    int active;
    int phase;
    ipv4_addr_t target;
    uint32_t deadline_ms;
    uint32_t last_send_ms;
    uint32_t start_ms;
    uint32_t rtt_ms;
    icmp_status_t result;
    int sent_once;
} icmp_sm_t;

void icmp_sm_begin(icmp_sm_t *sm, ipv4_addr_t target, uint32_t timeout_ms);
int icmp_sm_step(icmp_sm_t *sm);
int icmp_sm_done(const icmp_sm_t *sm);
icmp_status_t icmp_sm_result(const icmp_sm_t *sm);
uint32_t icmp_sm_rtt(const icmp_sm_t *sm);

#endif
