#include "net/icmp/icmp.h"

#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/prot/icmp.h"
#include "lwip/raw.h"
#include "net/lwip/lwip_pump.h"
#include "time/time.h"

#define ICMP_PING_DATA_SIZE 32

static struct raw_pcb *ping_pcb;
static uint16_t ping_id = 0x4B41U;
static uint16_t ping_seq;
static int reply_received;
static uint32_t ping_rtt_ms;

static u8_t icmp_raw_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    struct icmp_echo_hdr *iecho;

    (void)arg;
    (void)pcb;
    (void)addr;

    if (p == 0) {
        return 0;
    }

    if (p->tot_len < (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) {
        return 0;
    }

    if (pbuf_remove_header(p, PBUF_IP_HLEN) != 0) {
        return 0;
    }

    iecho = (struct icmp_echo_hdr *)p->payload;
    if (ICMPH_TYPE(iecho) != ICMP_ER) {
        pbuf_add_header(p, PBUF_IP_HLEN);
        return 0;
    }

    if (iecho->id != lwip_htons(ping_id) || iecho->seqno != lwip_htons(ping_seq)) {
        pbuf_add_header(p, PBUF_IP_HLEN);
        return 0;
    }

    reply_received = 1;
    pbuf_free(p);
    return 1;
}

static void icmp_ping_prepare(struct icmp_echo_hdr *iecho, u16_t len)
{
    size_t data_len = len - sizeof(struct icmp_echo_hdr);
    size_t i;

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = lwip_htons(ping_id);
    iecho->seqno = lwip_htons(ping_seq);

    for (i = 0; i < data_len; i++) {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    iecho->chksum = inet_chksum(iecho, len);
}

static int icmp_ping_send(ipv4_addr_t target)
{
    ip4_addr_t ip4;
    ip_addr_t addr;
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    u16_t ping_size = (u16_t)(sizeof(struct icmp_echo_hdr) + ICMP_PING_DATA_SIZE);

    if (ping_pcb == 0) {
        return -1;
    }

    p = pbuf_alloc(PBUF_IP, ping_size, PBUF_RAM);
    if (p == 0 || p->len != p->tot_len || p->next != 0) {
        if (p != 0) {
            pbuf_free(p);
        }
        return -1;
    }

    iecho = (struct icmp_echo_hdr *)p->payload;
    icmp_ping_prepare(iecho, ping_size);

    ipv4_to_lwip_addr(target, &ip4);
    ip_addr_copy_from_ip4(addr, ip4);

    if (raw_sendto(ping_pcb, p, &addr) != ERR_OK) {
        pbuf_free(p);
        return -1;
    }

    pbuf_free(p);
    return 0;
}

static void icmp_ping_begin(ipv4_addr_t target)
{
    ping_seq = (uint16_t)(time_millis() & 0xFFFFU);
    reply_received = 0;

    if (ping_pcb != 0) {
        raw_remove(ping_pcb);
        ping_pcb = 0;
    }

    ping_pcb = raw_new(IP_PROTO_ICMP);
    if (ping_pcb == 0) {
        return;
    }

    raw_recv(ping_pcb, icmp_raw_recv, 0);
    raw_bind(ping_pcb, IP_ADDR_ANY);
    (void)icmp_ping_send(target);
}

static void icmp_ping_finish(void)
{
    if (ping_pcb != 0) {
        raw_remove(ping_pcb);
        ping_pcb = 0;
    }
}

icmp_status_t icmp_ping(ipv4_addr_t target, uint32_t timeout_ms, uint32_t *rtt_ms_out)
{
    uint32_t start = time_millis();
    uint32_t last_send = start;
    uint32_t deadline = start + timeout_ms;
    int sent_once = 0;

    icmp_ping_begin(target);
    if (ping_pcb == 0) {
        return ICMP_FAIL_UNREACHABLE;
    }
    sent_once = 1;

    while (time_millis() < deadline) {
        if (time_millis() - last_send >= 1000U) {
            if (icmp_ping_send(target) == 0) {
                sent_once = 1;
            }
            last_send = time_millis();
        }

        lwip_stack_pump(8);
        if (reply_received) {
            uint32_t now = time_millis();
            ping_rtt_ms = (now >= start) ? (now - start) : 0U;
            if (rtt_ms_out != 0) {
                *rtt_ms_out = ping_rtt_ms;
            }
            icmp_ping_finish();
            return ICMP_OK;
        }
    }

    icmp_ping_finish();
    return sent_once ? ICMP_FAIL_TIMEOUT : ICMP_FAIL_UNREACHABLE;
}

void icmp_sm_begin(icmp_sm_t *sm, ipv4_addr_t target, uint32_t timeout_ms)
{
    ping_seq = (uint16_t)(time_millis() & 0xFFFFU);
    reply_received = 0;

    sm->active = 1;
    sm->phase = 0;
    sm->target = target;
    sm->start_ms = time_millis();
    sm->deadline_ms = sm->start_ms + timeout_ms;
    sm->last_send_ms = sm->start_ms;
    sm->rtt_ms = 0;
    sm->result = ICMP_FAIL_TIMEOUT;
    sm->sent_once = 0;

    icmp_ping_begin(target);
    if (ping_pcb != 0) {
        sm->sent_once = 1;
    }
}

int icmp_sm_done(const icmp_sm_t *sm)
{
    return !sm->active;
}

icmp_status_t icmp_sm_result(const icmp_sm_t *sm)
{
    return sm->result;
}

uint32_t icmp_sm_rtt(const icmp_sm_t *sm)
{
    return sm->rtt_ms;
}

int icmp_sm_step(icmp_sm_t *sm)
{
    if (!sm->active) {
        return 1;
    }

    uint32_t now = time_millis();
    if (now >= sm->deadline_ms) {
        sm->result = sm->sent_once ? ICMP_FAIL_TIMEOUT : ICMP_FAIL_UNREACHABLE;
        icmp_ping_finish();
        sm->active = 0;
        return 1;
    }

    if (reply_received) {
        sm->rtt_ms = (now >= sm->start_ms) ? (now - sm->start_ms) : 0U;
        sm->result = ICMP_OK;
        icmp_ping_finish();
        sm->active = 0;
        return 1;
    }

    if (now - sm->last_send_ms >= 1000U) {
        if (ping_pcb == 0) {
            icmp_ping_begin(sm->target);
        }
        if (icmp_ping_send(sm->target) == 0) {
            sm->sent_once = 1;
        }
        sm->last_send_ms = now;
    }

    return 0;
}
