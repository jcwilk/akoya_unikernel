#include "net/icmp/icmp.h"

#include "net/ipv4/ipv4.h"
#include "net/link/link.h"
#include "time/time.h"

struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed));

static uint16_t ping_id = 0x4B41U;
static uint16_t ping_seq;
static int reply_received;
static uint32_t ping_rtt_ms;

static void icmp_ipv4_handler(const uint8_t *packet, uint16_t length, void *ctx)
{
    (void)ctx;

    if (length < 28) {
        return;
    }

    if (packet[9] != 1) {
        return;
    }

    const struct icmp_header *icmp = (const struct icmp_header *)(packet + 20);
    if (icmp->type != 0) {
        return;
    }

    if (net_be16(icmp->identifier) != ping_id || net_be16(icmp->sequence) != ping_seq) {
        return;
    }

    reply_received = 1;
}

icmp_status_t icmp_ping(ipv4_addr_t target, uint32_t timeout_ms, uint32_t *rtt_ms_out)
{
    uint8_t payload[64];
    struct icmp_header *icmp = (struct icmp_header *)payload;

    ping_seq = (uint16_t)(time_millis() & 0xFFFFU);
    reply_received = 0;

    icmp->type = 8;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = net_be16(ping_id);
    icmp->sequence = net_be16(ping_seq);

    for (int i = sizeof(struct icmp_header); i < (int)sizeof(payload); i++) {
        payload[i] = (uint8_t)i;
    }

    {
        uint16_t csum = ipv4_checksum(payload, (int)sizeof(payload));
        icmp->checksum = net_be16(csum);
    }

    link_register_ipv4_handler(icmp_ipv4_handler, 0);

    uint32_t start = time_millis();
    uint32_t last_send = start;
    uint32_t deadline = start + timeout_ms;
    int sent_once = 0;
    while (time_millis() < deadline) {
        if (time_millis() - last_send >= 1000U) {
            if (ipv4_send(target, payload, (uint16_t)sizeof(payload), 1) != 0) {
                last_send = time_millis();
                link_poll();
                continue;
            }
            sent_once = 1;
            last_send = time_millis();
        }

        link_poll();
        if (reply_received) {
            uint32_t now = time_millis();
            ping_rtt_ms = (now >= start) ? (now - start) : 0U;
            if (rtt_ms_out != 0) {
                *rtt_ms_out = ping_rtt_ms;
            }
            link_register_ipv4_handler(0, 0);
            return ICMP_OK;
        }
    }

    link_register_ipv4_handler(0, 0);
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

    link_register_ipv4_handler(icmp_ipv4_handler, 0);
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
        link_unregister_ipv4_handler(icmp_ipv4_handler, 0);
        sm->active = 0;
        return 1;
    }

    if (reply_received) {
        sm->rtt_ms = (now >= sm->start_ms) ? (now - sm->start_ms) : 0U;
        sm->result = ICMP_OK;
        link_unregister_ipv4_handler(icmp_ipv4_handler, 0);
        sm->active = 0;
        return 1;
    }

    if (now - sm->last_send_ms >= 1000U) {
        uint8_t payload[64];
        struct icmp_header *icmp = (struct icmp_header *)payload;

        icmp->type = 8;
        icmp->code = 0;
        icmp->checksum = 0;
        icmp->identifier = net_be16(ping_id);
        icmp->sequence = net_be16(ping_seq);

        for (int i = sizeof(struct icmp_header); i < (int)sizeof(payload); i++) {
            payload[i] = (uint8_t)i;
        }

        uint16_t csum = ipv4_checksum(payload, (int)sizeof(payload));
        icmp->checksum = net_be16(csum);

        if (ipv4_send(sm->target, payload, (uint16_t)sizeof(payload), 1) == 0) {
            sm->sent_once = 1;
        }
        sm->last_send_ms = now;
    }

    return 0;
}
