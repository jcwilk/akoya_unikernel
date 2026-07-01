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

typedef struct {
    int active;
    int phase;
    uint32_t deadline_ms;
    uint32_t poll_until_ms;
    dhcp_status_t result;
    ipv4_config_t config;
} dhcp_sm_t;

void dhcp_sm_begin(dhcp_sm_t *sm, uint32_t timeout_ms);
int dhcp_sm_step(dhcp_sm_t *sm);
int dhcp_sm_done(const dhcp_sm_t *sm);
dhcp_status_t dhcp_sm_result(const dhcp_sm_t *sm);
const ipv4_config_t *dhcp_sm_config(const dhcp_sm_t *sm);

int dhcp_link_rx_active(void);

#endif
