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

typedef struct {
    int open;
} tcp_session_t;

typedef int (*tcp_recv_complete_fn)(const uint8_t *buf, uint16_t len, void *ctx);

tcp_status_t tcp_session_open(
    tcp_session_t *session,
    ipv4_addr_t dest,
    uint16_t dest_port,
    uint32_t timeout_ms);

tcp_status_t tcp_session_send(tcp_session_t *session, const uint8_t *data, uint16_t len);

tcp_status_t tcp_session_recv_until(
    tcp_session_t *session,
    uint8_t *buf,
    uint16_t cap,
    uint16_t *len_out,
    tcp_recv_complete_fn complete,
    void *ctx,
    uint32_t timeout_ms);

void tcp_session_close(tcp_session_t *session);

int tcp_transport_inactive(void);

#endif
