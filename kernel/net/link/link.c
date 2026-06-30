#include "net/link/link.h"

#include "net/ipv4/ipv4.h"
#include "time/time.h"

#include <stddef.h>

#define ARP_HW_ETHER 1
#define ARP_PROTO_IPV4 0x0800

struct eth_header {
    eth_addr_t dest;
    eth_addr_t src;
    uint16_t ethertype;
} __attribute__((packed));

struct arp_packet {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_len;
    uint8_t proto_len;
    uint16_t operation;
    eth_addr_t sender_mac;
    ipv4_addr_t sender_ip;
    eth_addr_t target_mac;
    ipv4_addr_t target_ip;
} __attribute__((packed));

#define ARP_CACHE_SIZE 4
#define IPV4_HANDLER_MAX 8

static struct {
    link_ipv4_handler_t handler;
    void *ctx;
    int active;
} ipv4_handlers[IPV4_HANDLER_MAX];

static eth_device_t *nic;
static link_arp_handler_t arp_handler;
static void *arp_ctx;

static eth_addr_t broadcast_mac = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

static struct {
    ipv4_addr_t ip;
    eth_addr_t mac;
    int valid;
} arp_cache[ARP_CACHE_SIZE];

static void arp_cache_store(ipv4_addr_t ip, eth_addr_t mac)
{
    if (ipv4_is_zero(ip)) {
        return;
    }

    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && ipv4_equal(arp_cache[i].ip, ip)) {
            arp_cache[i].mac = mac;
            return;
        }
    }

    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            arp_cache[i].ip = ip;
            arp_cache[i].mac = mac;
            arp_cache[i].valid = 1;
            return;
        }
    }

    arp_cache[0].ip = ip;
    arp_cache[0].mac = mac;
    arp_cache[0].valid = 1;
}

static int arp_cache_lookup(ipv4_addr_t ip, eth_addr_t *mac_out)
{
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && ipv4_equal(arp_cache[i].ip, ip)) {
            *mac_out = arp_cache[i].mac;
            return 0;
        }
    }
    return -1;
}

static void arp_reply_to_request(const struct arp_packet *request)
{
    const ipv4_config_t *cfg = ipv4_get_config();
    if (cfg == 0 || ipv4_is_zero(cfg->address)) {
        return;
    }

    if (!ipv4_equal(request->target_ip, cfg->address)) {
        return;
    }

    struct arp_packet reply;
    reply.hw_type = net_be16(ARP_HW_ETHER);
    reply.proto_type = net_be16(ARP_PROTO_IPV4);
    reply.hw_len = ETH_ADDR_LEN;
    reply.proto_len = 4;
    reply.operation = net_be16(2);
    reply.sender_mac = *eth_mac(nic);
    reply.sender_ip = cfg->address;
    reply.target_mac = request->sender_mac;
    reply.target_ip = request->sender_ip;

    link_send_raw_frame(request->sender_mac, ETH_TYPE_ARP, (const uint8_t *)&reply, (uint16_t)sizeof(reply));
}

static void handle_arp(const uint8_t *frame, uint16_t length)
{
    if (length < sizeof(struct eth_header) + sizeof(struct arp_packet)) {
        return;
    }

    const struct arp_packet *arp = (const struct arp_packet *)(frame + sizeof(struct eth_header));
    if (arp->hw_type != net_be16(ARP_HW_ETHER)
        || arp->proto_type != net_be16(ARP_PROTO_IPV4)
        || arp->hw_len != ETH_ADDR_LEN
        || arp->proto_len != 4) {
        return;
    }

    if (net_be16(arp->operation) == 1) {
        arp_reply_to_request(arp);
    }

    if (net_be16(arp->operation) == 2) {
        arp_cache_store(arp->sender_ip, arp->sender_mac);
    }

    if (arp_handler != 0) {
        arp_handler((const uint8_t *)arp, (uint16_t)sizeof(struct arp_packet), arp_ctx);
    }
}

static void handle_ipv4(const uint8_t *frame, uint16_t length)
{
    if (length < sizeof(struct eth_header) + 20) {
        return;
    }

    const struct eth_header *hdr = (const struct eth_header *)frame;
    const uint8_t *ip = frame + sizeof(struct eth_header);
    ipv4_addr_t src_ip;
    src_ip.bytes[0] = ip[12];
    src_ip.bytes[1] = ip[13];
    src_ip.bytes[2] = ip[14];
    src_ip.bytes[3] = ip[15];
    arp_cache_store(src_ip, hdr->src);

    for (int i = 0; i < IPV4_HANDLER_MAX; i++) {
        if (ipv4_handlers[i].active && ipv4_handlers[i].handler != 0) {
            ipv4_handlers[i].handler(ip, (uint16_t)(length - sizeof(struct eth_header)), ipv4_handlers[i].ctx);
        }
    }
}

static int frame_for_us(const struct eth_header *hdr, const eth_addr_t *our_mac)
{
    if (hdr->dest.bytes[0] == 0xFF) {
        return 1;
    }
    if (hdr->dest.bytes[0] & 0x01U) {
        return 1;
    }

    for (int i = 0; i < ETH_ADDR_LEN; i++) {
        if (hdr->dest.bytes[i] != our_mac->bytes[i]) {
            return 0;
        }
    }

    return 1;
}

static void dispatch_frame(const uint8_t *frame, uint16_t length)
{
    if (length < sizeof(struct eth_header)) {
        return;
    }

    const struct eth_header *hdr = (const struct eth_header *)frame;
    if (!frame_for_us(hdr, eth_mac(nic))) {
        return;
    }

    uint16_t ethertype = net_be16(hdr->ethertype);
    if (ethertype == ETH_TYPE_ARP) {
        handle_arp(frame, length);
    } else if (ethertype == ETH_TYPE_IPV4) {
        handle_ipv4(frame, length);
    }
}

eth_device_t *link_device(void)
{
    return nic;
}

void link_init(eth_device_t *dev)
{
    nic = dev;
    for (int i = 0; i < IPV4_HANDLER_MAX; i++) {
        ipv4_handlers[i].active = 0;
        ipv4_handlers[i].handler = 0;
        ipv4_handlers[i].ctx = 0;
    }
    arp_handler = 0;
    arp_ctx = 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_cache[i].valid = 0;
    }
}

int link_send_raw_frame(eth_addr_t dest, uint16_t ethertype, const uint8_t *payload, uint16_t length)
{
    uint8_t frame[1600];
    struct eth_header *hdr = (struct eth_header *)frame;

    if ((uint32_t)length + sizeof(struct eth_header) > sizeof(frame)) {
        return -1;
    }

    hdr->dest = dest;
    hdr->src = *eth_mac(nic);
    hdr->ethertype = net_be16(ethertype);

    for (uint16_t i = 0; i < length; i++) {
        frame[sizeof(struct eth_header) + i] = payload[i];
    }

    return eth_send(nic, frame, (uint16_t)(sizeof(struct eth_header) + length));
}

int link_send_arp_request(ipv4_addr_t target_ip)
{
    struct arp_packet arp;
    ipv4_addr_t sender_ip;
    const ipv4_config_t *cfg = ipv4_get_config();

    arp.hw_type = net_be16(ARP_HW_ETHER);
    arp.proto_type = net_be16(ARP_PROTO_IPV4);
    arp.hw_len = ETH_ADDR_LEN;
    arp.proto_len = 4;
    arp.operation = net_be16(1);
    arp.sender_mac = *eth_mac(nic);
    if (cfg != 0 && !ipv4_is_zero(cfg->address)) {
        sender_ip = cfg->address;
    } else {
        sender_ip.bytes[0] = 0;
        sender_ip.bytes[1] = 0;
        sender_ip.bytes[2] = 0;
        sender_ip.bytes[3] = 0;
    }
    arp.sender_ip = sender_ip;
    arp.target_mac = (eth_addr_t){{0, 0, 0, 0, 0, 0}};
    arp.target_ip = target_ip;

    return link_send_raw_frame(broadcast_mac, ETH_TYPE_ARP, (const uint8_t *)&arp, (uint16_t)sizeof(arp));
}

int link_send_ipv4(ipv4_addr_t next_hop, const uint8_t *ip_packet, uint16_t length, uint8_t protocol)
{
    (void)protocol;
    eth_addr_t dest_mac;
    if (link_resolve_ipv4(next_hop, &dest_mac, 3000) != 0) {
        return -1;
    }

    return link_send_raw_frame(dest_mac, ETH_TYPE_IPV4, ip_packet, length);
}

typedef struct {
    ipv4_addr_t target;
    eth_addr_t mac;
    int found;
} arp_wait_t;

static void arp_wait_handler(const uint8_t *packet, uint16_t length, void *ctx)
{
    (void)length;
    arp_wait_t *wait = (arp_wait_t *)ctx;
    const struct arp_packet *arp = (const struct arp_packet *)packet;

    if (net_be16(arp->operation) != 2) {
        return;
    }

    if (!ipv4_equal(arp->sender_ip, wait->target)) {
        return;
    }

    wait->mac = arp->sender_mac;
    wait->found = 1;
}

int link_resolve_ipv4(ipv4_addr_t dest_ip, eth_addr_t *mac_out, uint32_t timeout_ms)
{
    if (arp_cache_lookup(dest_ip, mac_out) == 0) {
        return 0;
    }

    arp_wait_t wait;
    wait.target = dest_ip;
    wait.found = 0;

    link_register_arp_handler(arp_wait_handler, &wait);

    uint32_t deadline = time_millis() + timeout_ms;
    while (time_millis() < deadline) {
        if (link_send_arp_request(dest_ip) != 0) {
            link_register_arp_handler(0, 0);
            return -1;
        }

        uint32_t attempt_deadline = time_millis() + 500U;
        while (time_millis() < attempt_deadline) {
            link_poll();
            if (wait.found) {
                *mac_out = wait.mac;
                link_register_arp_handler(0, 0);
                return 0;
            }
        }
    }

    link_register_arp_handler(0, 0);
    return -1;
}

static void arp_sm_handler(const uint8_t *packet, uint16_t length, void *ctx)
{
    (void)length;
    arp_sm_t *sm = (arp_sm_t *)ctx;
    const struct arp_packet *arp = (const struct arp_packet *)packet;

    if (net_be16(arp->operation) != 2) {
        return;
    }

    if (!ipv4_equal(arp->sender_ip, sm->target)) {
        return;
    }

    sm->mac = arp->sender_mac;
    sm->found = 1;
}

void arp_sm_begin(arp_sm_t *sm, ipv4_addr_t target, uint32_t timeout_ms)
{
    sm->active = 1;
    sm->target = target;
    sm->deadline_ms = time_millis() + timeout_ms;
    sm->poll_until_ms = 0;
    sm->found = 0;
    sm->mac = (eth_addr_t){{0, 0, 0, 0, 0, 0}};

    if (arp_cache_lookup(target, &sm->mac) == 0) {
        sm->found = 1;
        sm->active = 0;
        return;
    }

    link_register_arp_handler(arp_sm_handler, sm);
}

int arp_sm_done(const arp_sm_t *sm)
{
    return !sm->active;
}

int arp_sm_found(const arp_sm_t *sm)
{
    return sm->found;
}

eth_addr_t arp_sm_mac(const arp_sm_t *sm)
{
    return sm->mac;
}

int arp_sm_step(arp_sm_t *sm)
{
    if (!sm->active) {
        return 1;
    }

    uint32_t now = time_millis();
    if (now >= sm->deadline_ms) {
        link_register_arp_handler(0, 0);
        sm->active = 0;
        return 1;
    }

    if (sm->found) {
        link_register_arp_handler(0, 0);
        sm->active = 0;
        return 1;
    }

    if (now >= sm->poll_until_ms) {
        if (link_send_arp_request(sm->target) != 0) {
            link_register_arp_handler(0, 0);
            sm->active = 0;
            return 1;
        }
        sm->poll_until_ms = now + 500U;
    }

    return 0;
}

void link_register_ipv4_handler(link_ipv4_handler_t handler, void *ctx)
{
    if (handler == 0) {
        for (int i = 0; i < IPV4_HANDLER_MAX; i++) {
            ipv4_handlers[i].active = 0;
            ipv4_handlers[i].handler = 0;
            ipv4_handlers[i].ctx = 0;
        }
        return;
    }

    for (int i = 0; i < IPV4_HANDLER_MAX; i++) {
        if (!ipv4_handlers[i].active) {
            ipv4_handlers[i].handler = handler;
            ipv4_handlers[i].ctx = ctx;
            ipv4_handlers[i].active = 1;
            return;
        }
    }
}

void link_unregister_ipv4_handler(link_ipv4_handler_t handler, void *ctx)
{
    for (int i = 0; i < IPV4_HANDLER_MAX; i++) {
        if (ipv4_handlers[i].active
            && ipv4_handlers[i].handler == handler
            && ipv4_handlers[i].ctx == ctx) {
            ipv4_handlers[i].active = 0;
            ipv4_handlers[i].handler = 0;
            ipv4_handlers[i].ctx = 0;
            return;
        }
    }
}

void link_register_arp_handler(link_arp_handler_t handler, void *ctx)
{
    arp_handler = handler;
    arp_ctx = ctx;
}

void link_announce_ipv4(ipv4_addr_t address)
{
    struct arp_packet arp;
    arp.hw_type = net_be16(ARP_HW_ETHER);
    arp.proto_type = net_be16(ARP_PROTO_IPV4);
    arp.hw_len = ETH_ADDR_LEN;
    arp.proto_len = 4;
    arp.operation = net_be16(1);
    arp.sender_mac = *eth_mac(nic);
    arp.sender_ip = address;
    arp.target_mac = (eth_addr_t){{0, 0, 0, 0, 0, 0}};
    arp.target_ip = address;
    arp_cache_store(address, arp.sender_mac);

    link_send_raw_frame(broadcast_mac, ETH_TYPE_ARP, (const uint8_t *)&arp, (uint16_t)sizeof(arp));
}

#define LINK_RX_DRAIN_ALL 0

void link_poll(void)
{
    time_poll();
    link_drain_rx(LINK_RX_DRAIN_ALL);
}

void link_drain_rx(int max_frames)
{
    uint8_t frame[1600];
    uint16_t length = 0;
    int drained = 0;

    for (;;) {
        if (max_frames > 0 && drained >= max_frames) {
            break;
        }

        int status = eth_poll(nic, frame, (uint16_t)sizeof(frame), &length);
        if (status != 0) {
            break;
        }
        dispatch_frame(frame, length);
        drained++;
    }
}
