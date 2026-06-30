#ifndef AKOYA_NET_LINK_H
#define AKOYA_NET_LINK_H

#include <stdint.h>

#include "net/eth/eth.h"
#include "net/nettypes.h"

#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IPV4 0x0800

typedef void (*link_ipv4_handler_t)(const uint8_t *packet, uint16_t length, void *ctx);
typedef void (*link_arp_handler_t)(const uint8_t *packet, uint16_t length, void *ctx);

void link_init(eth_device_t *dev);
eth_device_t *link_device(void);
int link_send_raw_frame(eth_addr_t dest, uint16_t ethertype, const uint8_t *payload, uint16_t length);
int link_send_ipv4(ipv4_addr_t next_hop, const uint8_t *ip_packet, uint16_t length, uint8_t protocol);
int link_resolve_ipv4(ipv4_addr_t dest_ip, eth_addr_t *mac_out, uint32_t timeout_ms);
void link_register_ipv4_handler(link_ipv4_handler_t handler, void *ctx);
void link_unregister_ipv4_handler(link_ipv4_handler_t handler, void *ctx);
void link_register_arp_handler(link_arp_handler_t handler, void *ctx);
void link_announce_ipv4(ipv4_addr_t address);
void link_poll(void);
void link_drain_rx(int max_frames);

typedef struct {
    int active;
    ipv4_addr_t target;
    uint32_t deadline_ms;
    uint32_t poll_until_ms;
    eth_addr_t mac;
    int found;
} arp_sm_t;

void arp_sm_begin(arp_sm_t *sm, ipv4_addr_t target, uint32_t timeout_ms);
int arp_sm_step(arp_sm_t *sm);
int arp_sm_done(const arp_sm_t *sm);
int arp_sm_found(const arp_sm_t *sm);
eth_addr_t arp_sm_mac(const arp_sm_t *sm);

#endif
