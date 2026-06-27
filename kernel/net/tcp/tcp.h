#ifndef AKOYA_NET_TCP_H
#define AKOYA_NET_TCP_H

#include <stdint.h>

#include "net/nettypes.h"

typedef enum {
    TCP_OK = 0,
    TCP_FAIL_CONNECT,
    TCP_FAIL_TIMEOUT,
    TCP_FAIL_SEND,
    TCP_FAIL_RECV
} tcp_status_t;

tcp_status_t tcp_connect_send_recv(
    ipv4_addr_t dest,
    uint16_t dest_port,
    const uint8_t *send_data,
    uint16_t send_len,
    uint8_t *recv_buf,
    uint16_t recv_cap,
    uint16_t *recv_len_out,
    uint32_t timeout_ms);

#endif
