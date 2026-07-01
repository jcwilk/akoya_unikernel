#include "net/tcp/tcp.h"

#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "lwip/tcp.h"
#include "net/lwip/lwip_pump.h"
#include "time/time.h"

#define TCP_POST_BODY_DRAIN_MS 2000U

static struct altcp_pcb *conn_pcb;
static ipv4_addr_t conn_remote_ip;
static uint16_t conn_local_port;
static uint16_t conn_remote_port;
static int conn_established;
static int conn_remote_closed;
static int conn_our_fin_sent;
static int conn_reset;
static uint8_t *recv_buf;
static uint16_t recv_cap;
static uint16_t recv_len;
static uint16_t next_local_port;

static void tcp_pump(void)
{
    lwip_stack_pump(8);
}

static err_t tcp_connected_cb(void *arg, struct altcp_pcb *pcb, err_t err)
{
    (void)arg;
    (void)pcb;

    if (err == ERR_OK) {
        conn_established = 1;
    } else {
        conn_reset = 1;
    }
    return ERR_OK;
}

static err_t tcp_recv_cb(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
    (void)arg;

    if (p == 0) {
        conn_remote_closed = 1;
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    if (recv_buf != 0) {
        uint16_t space = (recv_cap > recv_len) ? (uint16_t)(recv_cap - recv_len) : 0;
        uint16_t copy = (p->tot_len < space) ? p->tot_len : space;
        if (copy > 0) {
            pbuf_copy_partial(p, recv_buf + recv_len, copy, 0);
            recv_len = (uint16_t)(recv_len + copy);
        }
    }

    altcp_recved(pcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

static void tcp_err_cb(void *arg, err_t err)
{
    (void)arg;
    (void)err;
    conn_reset = 1;
    conn_pcb = 0;
}

static uint32_t tcp_entropy32(void)
{
    uint32_t lo;
    __asm__ volatile ("rdtsc" : "=a"(lo) : : "edx");
    return lo ^ time_millis();
}

static void tcp_seed_ports(void)
{
    if (next_local_port != 0) {
        return;
    }

    next_local_port = (uint16_t)(49152U + (tcp_entropy32() & 0x3FFFU));
    if (next_local_port < 49152U) {
        next_local_port = 49152U;
    }
}

static uint16_t tcp_alloc_local_port(void)
{
    tcp_seed_ports();
    uint16_t port = next_local_port++;
    if (next_local_port < 49152U || next_local_port > 65000U) {
        next_local_port = (uint16_t)(49152U + (tcp_entropy32() & 0x0FFFU));
    }
    return port;
}

static void tcp_reset_flow_state(void)
{
    conn_remote_ip.bytes[0] = 0;
    conn_remote_ip.bytes[1] = 0;
    conn_remote_ip.bytes[2] = 0;
    conn_remote_ip.bytes[3] = 0;
    conn_local_port = 0;
    conn_remote_port = 0;
    conn_established = 0;
    conn_remote_closed = 0;
    conn_our_fin_sent = 0;
    conn_reset = 0;
    recv_buf = 0;
    recv_cap = 0;
    recv_len = 0;
}

static void tcp_free_pcb(void)
{
    if (conn_pcb != 0) {
        altcp_arg(conn_pcb, 0);
        altcp_recv(conn_pcb, 0);
        altcp_err(conn_pcb, 0);
        altcp_abort(conn_pcb);
        conn_pcb = 0;
    }
}

static int tcp_begin_connect(ipv4_addr_t dest, uint16_t dest_port)
{
    altcp_allocator_t allocator;
    ip4_addr_t ip4;
    ip_addr_t addr;

    tcp_free_pcb();
    tcp_reset_flow_state();

    lwip_stack_arp_prime(dest);

    allocator.alloc = altcp_tcp_alloc;
    allocator.arg = 0;
    conn_pcb = altcp_new(&allocator);
    if (conn_pcb == 0) {
        return -1;
    }

    altcp_arg(conn_pcb, 0);
    altcp_recv(conn_pcb, tcp_recv_cb);
    altcp_err(conn_pcb, tcp_err_cb);

    conn_remote_ip = dest;
    conn_remote_port = dest_port;
    conn_local_port = tcp_alloc_local_port();

    if (altcp_bind(conn_pcb, IP_ADDR_ANY, conn_local_port) != ERR_OK) {
        tcp_free_pcb();
        return -1;
    }

    ipv4_to_lwip_addr(dest, &ip4);
    ip_addr_copy_from_ip4(addr, ip4);

    if (altcp_connect(conn_pcb, &addr, dest_port, tcp_connected_cb) != ERR_OK) {
        tcp_free_pcb();
        return -1;
    }

    return 0;
}

static void tcp_end_recv(void)
{
    recv_buf = 0;
    recv_cap = 0;
}

static void tcp_drain_stack(void)
{
    tcp_pump();
}

static tcp_status_t tcp_wait_until(uint32_t deadline, int (*predicate)(void))
{
    while (time_millis() < deadline) {
        tcp_pump();
        if (predicate()) {
            return TCP_OK;
        }
    }
    return TCP_FAIL_TIMEOUT;
}

static int predicate_connected(void)
{
    return conn_established || conn_reset;
}

static void tcp_begin_recv(uint8_t *buf, uint16_t cap, uint16_t *len_out)
{
    recv_buf = buf;
    recv_cap = cap;
    recv_len = 0;
    conn_remote_closed = 0;

    if (len_out != 0) {
        *len_out = 0;
    }
}

static void tcp_quiesce_connection(uint32_t deadline_ms)
{
    if (!conn_established || conn_reset || conn_pcb == 0) {
        return;
    }

    uint32_t deadline = time_millis() + deadline_ms;

    if (!conn_our_fin_sent) {
        if (altcp_close(conn_pcb) == ERR_OK) {
            conn_our_fin_sent = 1;
            conn_pcb = 0;
        } else {
            altcp_shutdown(conn_pcb, 0, 1);
            conn_our_fin_sent = 1;
        }
    }

    while (time_millis() < deadline) {
        tcp_pump();
        if (conn_reset) {
            break;
        }
        if (conn_remote_closed && conn_our_fin_sent) {
            break;
        }
    }

    tcp_drain_stack();
}

int tcp_transport_inactive(void)
{
    return conn_pcb == 0
        && !conn_established
        && !conn_reset
        && !conn_remote_closed
        && !conn_our_fin_sent
        && recv_buf == 0
        && recv_len == 0
        && conn_local_port == 0
        && conn_remote_port == 0
        && ipv4_is_zero(conn_remote_ip);
}

void tcp_transport_release(void)
{
    tcp_free_pcb();
    tcp_drain_stack();
    tcp_reset_flow_state();
}

int tcp_drain_until_inactive(uint32_t timeout_ms)
{
    uint32_t deadline = time_millis() + timeout_ms;

    while (time_millis() < deadline) {
        if (tcp_transport_inactive()) {
            return 1;
        }
        tcp_pump();
    }

    return tcp_transport_inactive();
}

int tcp_chat_drain_until_inactive(uint32_t timeout_ms)
{
    uint32_t deadline = time_millis() + timeout_ms;

    while (time_millis() < deadline) {
        if (tcp_transport_inactive()) {
            return 1;
        }
        tcp_pump();
    }

    return tcp_transport_inactive() ? 1 : 0;
}

tcp_status_t tcp_session_open(
    tcp_session_t *session,
    ipv4_addr_t dest,
    uint16_t dest_port,
    uint32_t timeout_ms)
{
    if (session == 0) {
        return TCP_FAIL_CONNECT;
    }

    if (tcp_begin_connect(dest, dest_port) != 0) {
        session->open = 0;
        return TCP_FAIL_CONNECT;
    }

    uint32_t deadline = time_millis() + timeout_ms;
    if (tcp_wait_until(deadline, predicate_connected) != TCP_OK) {
        tcp_transport_release();
        session->open = 0;
        return TCP_FAIL_TIMEOUT;
    }

    if (conn_reset || !conn_established) {
        tcp_transport_release();
        session->open = 0;
        return TCP_FAIL_CONNECT;
    }

    session->open = 1;
    return TCP_OK;
}

tcp_status_t tcp_session_send(tcp_session_t *session, const uint8_t *data, uint16_t len)
{
    if (session == 0 || !session->open || !conn_established || conn_reset || conn_pcb == 0) {
        return TCP_FAIL_SEND;
    }

    if (len == 0) {
        return TCP_OK;
    }

    if (altcp_write(conn_pcb, data, len, TCP_WRITE_FLAG_COPY) != ERR_OK) {
        return TCP_FAIL_SEND;
    }

    if (altcp_output(conn_pcb) != ERR_OK) {
        return TCP_FAIL_SEND;
    }

    tcp_drain_stack();
    return TCP_OK;
}

tcp_status_t tcp_session_recv_until(
    tcp_session_t *session,
    uint8_t *buf,
    uint16_t cap,
    uint16_t *len_out,
    tcp_recv_complete_fn complete,
    void *ctx,
    uint32_t timeout_ms)
{
    if (session == 0 || !session->open) {
        return TCP_FAIL_RECV;
    }

    tcp_begin_recv(buf, cap, len_out);

    uint32_t deadline = time_millis() + timeout_ms;
    int body_complete = 0;
    uint32_t body_complete_deadline = 0;

    while (time_millis() < deadline) {
        tcp_pump();

        if (conn_reset) {
            if (len_out != 0) {
                *len_out = recv_len;
            }
            tcp_end_recv();
            session->open = 0;
            tcp_transport_release();
            return TCP_FAIL_RECV;
        }

        if (!body_complete && complete != 0 && complete(buf, recv_len, ctx)) {
            body_complete = 1;
            body_complete_deadline = time_millis() + TCP_POST_BODY_DRAIN_MS;
            tcp_end_recv();
        }

        if (body_complete) {
            if (conn_remote_closed || conn_reset) {
                if (len_out != 0) {
                    *len_out = recv_len;
                }
                return TCP_OK;
            }
            if (time_millis() >= body_complete_deadline) {
                if (len_out != 0) {
                    *len_out = recv_len;
                }
                return TCP_OK;
            }
            continue;
        }

        if (conn_remote_closed) {
            if (len_out != 0) {
                *len_out = recv_len;
            }
            tcp_end_recv();
            session->open = 0;
            return TCP_OK;
        }
    }

    if (len_out != 0) {
        *len_out = recv_len;
    }

    return TCP_FAIL_TIMEOUT;
}

void tcp_session_close(tcp_session_t *session)
{
    if (session != 0 && session->open) {
        tcp_quiesce_connection(TCP_CLOSE_DRAIN_MS);

        uint32_t deadline = time_millis() + TCP_CLOSE_DRAIN_MS;
        while (time_millis() < deadline) {
            tcp_pump();
            if (conn_reset || (conn_our_fin_sent && conn_remote_closed)) {
                break;
            }
        }

        if (conn_established && !conn_reset && !(conn_our_fin_sent && conn_remote_closed) && conn_pcb != 0) {
            altcp_abort(conn_pcb);
            conn_pcb = 0;
            conn_reset = 1;
            tcp_drain_stack();
        }
    } else if (session == 0) {
        tcp_quiesce_connection(TCP_CLOSE_DRAIN_MS);
    }

    tcp_drain_stack();
    tcp_transport_release();

    if (session != 0) {
        session->open = 0;
    }
}

void tcp_sm_init(tcp_sm_t *sm)
{
    sm->state = TCP_SM_IDLE;
    sm->result = TCP_OK;
    sm->session.open = 0;
    sm->chat_drain = 0;
    sm->open_syn_sent = 0;
}

void tcp_sm_begin_drain(tcp_sm_t *sm, uint32_t timeout_ms, int chat_drain)
{
    sm->state = TCP_SM_DRAIN;
    sm->deadline_ms = time_millis() + timeout_ms;
    sm->chat_drain = chat_drain;
    sm->result = TCP_OK;
}

void tcp_sm_begin_open(tcp_sm_t *sm, ipv4_addr_t dest, uint16_t port, uint32_t timeout_ms)
{
    sm->state = TCP_SM_OPEN;
    sm->dest = dest;
    sm->dest_port = port;
    sm->deadline_ms = time_millis() + timeout_ms;
    sm->session.open = 0;
    sm->open_syn_sent = 0;
    sm->result = TCP_OK;
}

void tcp_sm_begin_send(tcp_sm_t *sm, const uint8_t *data, uint16_t len)
{
    sm->state = TCP_SM_SEND;
    sm->send_data = data;
    sm->send_len = len;
    sm->result = TCP_OK;
}

void tcp_sm_begin_recv(
    tcp_sm_t *sm,
    uint8_t *buf,
    uint16_t cap,
    tcp_recv_complete_fn complete,
    void *ctx,
    uint32_t timeout_ms)
{
    sm->state = TCP_SM_RECV;
    sm->recv_buf = buf;
    sm->recv_cap = cap;
    sm->recv_len = 0;
    sm->recv_complete = complete;
    sm->recv_ctx = ctx;
    sm->body_complete = 0;
    sm->deadline_ms = time_millis() + timeout_ms;
    sm->result = TCP_OK;
}

void tcp_sm_begin_close(tcp_sm_t *sm)
{
    sm->state = TCP_SM_CLOSE;
    sm->deadline_ms = time_millis() + TCP_CLOSE_DRAIN_MS;
}

int tcp_sm_finished(const tcp_sm_t *sm)
{
    return sm->state == TCP_SM_DONE;
}

tcp_status_t tcp_sm_result(const tcp_sm_t *sm)
{
    return sm->result;
}

uint16_t tcp_sm_recv_len(const tcp_sm_t *sm)
{
    return sm->recv_len;
}

int tcp_sm_step(tcp_sm_t *sm)
{
    if (sm->state == TCP_SM_IDLE || sm->state == TCP_SM_DONE) {
        return 1;
    }

    if (time_millis() >= sm->deadline_ms
        && sm->state != TCP_SM_DRAIN
        && sm->state != TCP_SM_POST_DRAIN
        && sm->state != TCP_SM_CLOSE) {
        sm->result = TCP_FAIL_TIMEOUT;
        sm->state = TCP_SM_DONE;
        return 1;
    }

    switch (sm->state) {
    case TCP_SM_DRAIN: {
        int inactive = sm->chat_drain
            ? tcp_chat_drain_until_inactive(1U)
            : tcp_drain_until_inactive(1U);
        if (inactive) {
            sm->state = TCP_SM_DONE;
            return 1;
        }
        if (time_millis() >= sm->deadline_ms) {
            sm->result = TCP_FAIL_RECV;
            sm->state = TCP_SM_DONE;
            return 1;
        }
        return 0;
    }
    case TCP_SM_OPEN: {
        if (!sm->open_syn_sent) {
            if (tcp_begin_connect(sm->dest, sm->dest_port) != 0) {
                sm->result = TCP_FAIL_CONNECT;
                tcp_transport_release();
                sm->state = TCP_SM_POST_DRAIN;
                sm->deadline_ms = time_millis() + TCP_CLOSE_DRAIN_MS;
                sm->chat_drain = 1;
                return 0;
            }
            sm->open_syn_sent = 1;
            return 0;
        }

        if (conn_reset) {
            sm->result = TCP_FAIL_CONNECT;
            tcp_transport_release();
            sm->state = TCP_SM_POST_DRAIN;
            sm->deadline_ms = time_millis() + TCP_CLOSE_DRAIN_MS;
            sm->chat_drain = 1;
            return 0;
        }

        if (!conn_established) {
            if (time_millis() >= sm->deadline_ms) {
                sm->result = TCP_FAIL_TIMEOUT;
                tcp_transport_release();
                sm->state = TCP_SM_POST_DRAIN;
                sm->deadline_ms = time_millis() + TCP_CLOSE_DRAIN_MS;
                sm->chat_drain = 1;
            }
            return 0;
        }

        sm->session.open = 1;
        sm->state = TCP_SM_DONE;
        return 1;
    }
    case TCP_SM_SEND: {
        tcp_status_t st = tcp_session_send(&sm->session, sm->send_data, sm->send_len);
        if (st != TCP_OK) {
            sm->result = st;
            sm->state = TCP_SM_DONE;
            return 1;
        }
        sm->state = TCP_SM_DONE;
        return 1;
    }
    case TCP_SM_RECV: {
        if (sm->recv_buf != 0 && recv_buf == 0) {
            tcp_begin_recv(sm->recv_buf, sm->recv_cap, &sm->recv_len);
        }

        if (conn_reset) {
            sm->result = TCP_FAIL_RECV;
            tcp_end_recv();
            sm->session.open = 0;
            tcp_transport_release();
            sm->state = TCP_SM_DONE;
            return 1;
        }

        if (!sm->body_complete && sm->recv_complete != 0
            && sm->recv_complete(sm->recv_buf, recv_len, sm->recv_ctx)) {
            sm->body_complete = 1;
            sm->body_complete_deadline = time_millis() + TCP_POST_BODY_DRAIN_MS;
            sm->recv_len = recv_len;
            tcp_end_recv();
        }

        if (sm->body_complete) {
            if (conn_remote_closed || conn_reset || time_millis() >= sm->body_complete_deadline) {
                sm->state = TCP_SM_DONE;
                return 1;
            }
            return 0;
        }

        if (conn_remote_closed) {
            sm->recv_len = recv_len;
            tcp_end_recv();
            sm->session.open = 0;
            sm->state = TCP_SM_DONE;
            return 1;
        }

        if (time_millis() >= sm->deadline_ms) {
            sm->result = TCP_FAIL_TIMEOUT;
            tcp_end_recv();
            sm->state = TCP_SM_DONE;
            return 1;
        }
        return 0;
    }
    case TCP_SM_CLOSE: {
        if (sm->session.open || conn_established) {
            if (!conn_our_fin_sent && conn_pcb != 0) {
                if (altcp_close(conn_pcb) == ERR_OK) {
                    conn_our_fin_sent = 1;
                    conn_pcb = 0;
                } else {
                    altcp_shutdown(conn_pcb, 0, 1);
                    conn_our_fin_sent = 1;
                }
            }
            if (conn_reset || (conn_our_fin_sent && conn_remote_closed)) {
                tcp_transport_release();
                sm->session.open = 0;
                sm->state = TCP_SM_POST_DRAIN;
                sm->deadline_ms = time_millis() + TCP_CLOSE_DRAIN_MS;
                sm->chat_drain = 1;
                return 0;
            }
            if (time_millis() >= sm->deadline_ms) {
                if (conn_pcb != 0) {
                    altcp_abort(conn_pcb);
                    conn_pcb = 0;
                }
                tcp_transport_release();
                sm->session.open = 0;
                sm->state = TCP_SM_POST_DRAIN;
                sm->deadline_ms = time_millis() + TCP_CLOSE_DRAIN_MS;
                sm->chat_drain = 1;
            }
            return 0;
        }
        sm->state = TCP_SM_POST_DRAIN;
        sm->deadline_ms = time_millis() + TCP_CLOSE_DRAIN_MS;
        sm->chat_drain = 1;
        return 0;
    }
    case TCP_SM_POST_DRAIN: {
        int inactive = sm->chat_drain
            ? tcp_chat_drain_until_inactive(1U)
            : tcp_drain_until_inactive(1U);
        if (inactive) {
            sm->state = TCP_SM_DONE;
            return 1;
        }
        if (time_millis() >= sm->deadline_ms) {
            sm->state = TCP_SM_DONE;
            return 1;
        }
        return 0;
    }
    default:
        sm->state = TCP_SM_DONE;
        return 1;
    }
}
