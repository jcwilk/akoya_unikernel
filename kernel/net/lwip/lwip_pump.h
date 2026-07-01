#ifndef AKOYA_NET_LWIP_PUMP_H
#define AKOYA_NET_LWIP_PUMP_H

#include <stdint.h>

#include "net/eth/eth.h"
#include "net/ipv4/ipv4.h"
#include "net/nettypes.h"

struct netif;

int lwip_stack_init(eth_device_t *dev);
struct netif *lwip_stack_netif(void);
eth_device_t *lwip_stack_device(void);

void lwip_stack_pump(int max_frames);
void lwip_stack_pump_all(void);

int lwip_stack_tx_frames(void);

int lwip_stack_rx_frames(void);

int lwip_stack_has_ip(void);
ipv4_config_t lwip_stack_ipv4_config(void);
void lwip_stack_apply_ipv4_config(const ipv4_config_t *config);
void lwip_stack_arp_prime(ipv4_addr_t dest);

void lwip_addr_to_ipv4(const void *lwip_addr, ipv4_addr_t *out);
void ipv4_to_lwip_addr(ipv4_addr_t in, void *lwip_addr_out);

#endif
