#ifndef AKOYA_NET_TCP_H
#define AKOYA_NET_TCP_H

#include <stdint.h>

#include "net/nettypes.h"

#define TCP_CLOSE_DRAIN_MS 5000U

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

void tcp_transport_release(void);

int tcp_drain_until_inactive(uint32_t timeout_ms);

int tcp_chat_drain_until_inactive(uint32_t timeout_ms);

typedef enum {
    TCP_SM_IDLE = 0,
    TCP_SM_DRAIN,
    TCP_SM_OPEN,
    TCP_SM_SEND,
    TCP_SM_RECV,
    TCP_SM_CLOSE,
    TCP_SM_POST_DRAIN,
    TCP_SM_DONE
} tcp_sm_state_t;

typedef struct {
    tcp_sm_state_t state;
    tcp_status_t result;
    tcp_session_t session;
    ipv4_addr_t dest;
    uint16_t dest_port;
    uint32_t deadline_ms;
    const uint8_t *send_data;
    uint16_t send_len;
    uint8_t *recv_buf;
    uint16_t recv_cap;
    uint16_t recv_len;
    tcp_recv_complete_fn recv_complete;
    void *recv_ctx;
    int body_complete;
    uint32_t body_complete_deadline;
    int chat_drain;
    int open_syn_sent;
} tcp_sm_t;

void tcp_sm_init(tcp_sm_t *sm);
void tcp_sm_begin_drain(tcp_sm_t *sm, uint32_t timeout_ms, int chat_drain);
void tcp_sm_begin_open(tcp_sm_t *sm, ipv4_addr_t dest, uint16_t port, uint32_t timeout_ms);
void tcp_sm_begin_send(tcp_sm_t *sm, const uint8_t *data, uint16_t len);
void tcp_sm_begin_recv(tcp_sm_t *sm, uint8_t *buf, uint16_t cap, tcp_recv_complete_fn complete, void *ctx, uint32_t timeout_ms);
void tcp_sm_begin_close(tcp_sm_t *sm);
int tcp_sm_step(tcp_sm_t *sm);
int tcp_sm_finished(const tcp_sm_t *sm);
tcp_status_t tcp_sm_result(const tcp_sm_t *sm);
uint16_t tcp_sm_recv_len(const tcp_sm_t *sm);

#endif
