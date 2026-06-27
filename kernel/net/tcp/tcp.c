#include "net/tcp/tcp.h"

#include "net/ipv4/ipv4.h"
#include "net/link/link.h"
#include "time/time.h"

#define TCP_PROTO 6U

#define TCP_FLAG_FIN 0x01U
#define TCP_FLAG_SYN 0x02U
#define TCP_FLAG_RST 0x04U
#define TCP_FLAG_PSH 0x08U
#define TCP_FLAG_ACK 0x10U

struct tcp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t offset_reserved;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed));

struct tcp_pseudo {
    uint8_t src[4];
    uint8_t dst[4];
    uint8_t zero;
    uint8_t protocol;
    uint16_t length;
} __attribute__((packed));

static ipv4_addr_t conn_remote_ip;
static uint16_t conn_local_port;
static uint16_t conn_remote_port;
static uint32_t conn_seq;
static uint32_t conn_ack;
static int conn_established;
static int conn_remote_fin;
static int conn_reset;
static uint8_t *recv_buf;
static uint16_t recv_cap;
static uint16_t recv_len;

static uint16_t tcp_checksum(ipv4_addr_t src, ipv4_addr_t dst, const uint8_t *tcp, uint16_t length)
{
    uint8_t block[12 + 1500];
    struct tcp_pseudo *pseudo = (struct tcp_pseudo *)block;

    pseudo->src[0] = src.bytes[0];
    pseudo->src[1] = src.bytes[1];
    pseudo->src[2] = src.bytes[2];
    pseudo->src[3] = src.bytes[3];
    pseudo->dst[0] = dst.bytes[0];
    pseudo->dst[1] = dst.bytes[1];
    pseudo->dst[2] = dst.bytes[2];
    pseudo->dst[3] = dst.bytes[3];
    pseudo->zero = 0;
    pseudo->protocol = TCP_PROTO;
    pseudo->length = net_be16(length);

    if ((uint32_t)length + sizeof(struct tcp_pseudo) > sizeof(block)) {
        return 0;
    }

    for (uint16_t i = 0; i < length; i++) {
        block[sizeof(struct tcp_pseudo) + i] = tcp[i];
    }

    return ipv4_checksum(block, (int)(sizeof(struct tcp_pseudo) + length));
}

static int tcp_send_segment(uint8_t flags, const uint8_t *payload, uint16_t payload_len)
{
    const ipv4_config_t *cfg = ipv4_get_config();
    if (cfg == 0) {
        return -1;
    }

    uint8_t packet[1500];
    struct tcp_header *tcp = (struct tcp_header *)packet;
    uint16_t tcp_len = (uint16_t)(sizeof(struct tcp_header) + payload_len);

    if (tcp_len > sizeof(packet)) {
        return -1;
    }

    tcp->src_port = net_be16(conn_local_port);
    tcp->dst_port = net_be16(conn_remote_port);
    tcp->seq = net_be32(conn_seq);
    tcp->ack = net_be32(conn_ack);
    tcp->offset_reserved = (uint8_t)(5U << 4);
    tcp->flags = flags;
    tcp->window = net_be16(8192U);
    tcp->checksum = 0;
    tcp->urgent = 0;

    for (uint16_t i = 0; i < payload_len; i++) {
        packet[sizeof(struct tcp_header) + i] = payload[i];
    }

    uint16_t csum = tcp_checksum(cfg->address, conn_remote_ip, packet, tcp_len);
    tcp->checksum = net_be16(csum);

    return ipv4_send(conn_remote_ip, packet, tcp_len, TCP_PROTO);
}

static void tcp_ipv4_handler(const uint8_t *packet, uint16_t length, void *ctx)
{
    (void)ctx;

    if (length < 40) {
        return;
    }

    if (packet[9] != TCP_PROTO) {
        return;
    }

    uint8_t ihl = (uint8_t)((packet[0] & 0x0FU) * 4U);
    if (length < ihl + sizeof(struct tcp_header)) {
        return;
    }

    const struct tcp_header *tcp = (const struct tcp_header *)(packet + ihl);
    uint16_t src_port = net_be16(tcp->src_port);
    uint16_t dst_port = net_be16(tcp->dst_port);

    if (src_port != conn_remote_port || dst_port != conn_local_port) {
        return;
    }

    ipv4_addr_t src_ip;
    src_ip.bytes[0] = packet[12];
    src_ip.bytes[1] = packet[13];
    src_ip.bytes[2] = packet[14];
    src_ip.bytes[3] = packet[15];

    if (!ipv4_equal(src_ip, conn_remote_ip)) {
        return;
    }

    uint8_t data_offset = (uint8_t)((tcp->offset_reserved >> 4) * 4U);
    if (data_offset < sizeof(struct tcp_header)) {
        return;
    }

    uint16_t ip_total = (uint16_t)((packet[2] << 8) | packet[3]);
    if (ip_total < ihl + data_offset) {
        return;
    }

    uint16_t payload_len = (uint16_t)(ip_total - ihl - data_offset);
    const uint8_t *payload = packet + ihl + data_offset;
    uint32_t seq = net_be32(tcp->seq);
    uint8_t flags = tcp->flags;

    if (flags & TCP_FLAG_RST) {
        conn_reset = 1;
        return;
    }

    if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK)) {
        if (!conn_established) {
            conn_ack = seq + 1U;
            conn_established = 1;
        }
    }

    if (flags & TCP_FLAG_ACK) {
        (void)net_be32(tcp->ack);
    }

    if (payload_len > 0 && conn_established) {
        if (seq == conn_ack && recv_buf != 0) {
            uint16_t space = (recv_cap > recv_len) ? (uint16_t)(recv_cap - recv_len) : 0;
            uint16_t copy = (payload_len < space) ? payload_len : space;
            for (uint16_t i = 0; i < copy; i++) {
                recv_buf[recv_len + i] = payload[i];
            }
            recv_len = (uint16_t)(recv_len + copy);
            conn_ack = seq + payload_len;
            (void)tcp_send_segment(TCP_FLAG_ACK, 0, 0);
        }
    }

    if (flags & TCP_FLAG_FIN) {
        conn_remote_fin = 1;
        conn_ack = seq + 1U + payload_len;
        (void)tcp_send_segment(TCP_FLAG_ACK, 0, 0);
    }
}

static tcp_status_t tcp_wait_until(uint32_t deadline, int (*predicate)(void))
{
    while (time_millis() < deadline) {
        link_poll();
        if (predicate()) {
            return TCP_OK;
        }
    }
    return TCP_FAIL_TIMEOUT;
}

static int predicate_syn_ack(void)
{
    return conn_established || conn_reset;
}

static int predicate_recv_done(void)
{
    return conn_remote_fin || conn_reset
        || (recv_buf != 0 && recv_len >= recv_cap);
}

tcp_status_t tcp_connect_send_recv(
    ipv4_addr_t dest,
    uint16_t dest_port,
    const uint8_t *send_data,
    uint16_t send_len,
    uint8_t *recv_buffer,
    uint16_t recv_capacity,
    uint16_t *recv_len_out,
    uint32_t timeout_ms)
{
    for (int i = 0; i < 32; i++) {
        link_poll();
    }

    conn_remote_ip = dest;
    conn_remote_port = dest_port;
    conn_local_port = (uint16_t)(49152U + ((time_millis() >> 4) & 0x0FFFU));
    conn_seq = time_millis();
    conn_ack = 0;
    conn_established = 0;
    conn_remote_fin = 0;
    conn_reset = 0;
    recv_buf = recv_buffer;
    recv_cap = recv_capacity;
    recv_len = 0;

    if (recv_len_out != 0) {
        *recv_len_out = 0;
    }

    link_register_ipv4_handler(tcp_ipv4_handler, 0);

    uint32_t deadline = time_millis() + timeout_ms;

    if (tcp_send_segment(TCP_FLAG_SYN, 0, 0) != 0) {
        link_register_ipv4_handler(0, 0);
        return TCP_FAIL_CONNECT;
    }
    conn_seq++;

    if (tcp_wait_until(deadline, predicate_syn_ack) != TCP_OK) {
        link_register_ipv4_handler(0, 0);
        return TCP_FAIL_TIMEOUT;
    }

    if (conn_reset || !conn_established) {
        link_register_ipv4_handler(0, 0);
        return TCP_FAIL_CONNECT;
    }

    if (tcp_send_segment(TCP_FLAG_ACK, 0, 0) != 0) {
        link_register_ipv4_handler(0, 0);
        return TCP_FAIL_SEND;
    }

    if (send_len > 0) {
        if (tcp_send_segment((uint8_t)(TCP_FLAG_ACK | TCP_FLAG_PSH), send_data, send_len) != 0) {
            link_register_ipv4_handler(0, 0);
            return TCP_FAIL_SEND;
        }
        conn_seq += send_len;
    }

    if (tcp_wait_until(deadline, predicate_recv_done) != TCP_OK) {
        (void)tcp_send_segment(TCP_FLAG_FIN | TCP_FLAG_ACK, 0, 0);
        link_register_ipv4_handler(0, 0);
        if (recv_len_out != 0) {
            *recv_len_out = recv_len;
        }
        return TCP_FAIL_TIMEOUT;
    }

    if (conn_reset) {
        link_register_ipv4_handler(0, 0);
        return TCP_FAIL_RECV;
    }

    (void)tcp_send_segment(TCP_FLAG_FIN | TCP_FLAG_ACK, 0, 0);
    conn_seq++;

    uint32_t fin_deadline = time_millis() + 2000U;
    while (time_millis() < fin_deadline) {
        link_poll();
        if (conn_remote_fin) {
            break;
        }
    }

    link_register_ipv4_handler(0, 0);

    if (recv_len_out != 0) {
        *recv_len_out = recv_len;
    }

    return TCP_OK;
}
