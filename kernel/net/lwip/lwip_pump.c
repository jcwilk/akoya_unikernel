#include "net/lwip/lwip_pump.h"

#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"

#define RX_STAGE_MAX 8

static eth_device_t *nic_dev;
static struct netif akoya_netif;

static struct {
    uint16_t len;
    uint8_t data[1518];
} rx_stage[RX_STAGE_MAX];

void lwip_addr_to_ipv4(const void *lwip_addr, ipv4_addr_t *out)
{
    const ip4_addr_t *addr = (const ip4_addr_t *)lwip_addr;
    out->bytes[0] = ip4_addr1(addr);
    out->bytes[1] = ip4_addr2(addr);
    out->bytes[2] = ip4_addr3(addr);
    out->bytes[3] = ip4_addr4(addr);
}

void ipv4_to_lwip_addr(ipv4_addr_t in, void *lwip_addr_out)
{
    ip4_addr_t *addr = (ip4_addr_t *)lwip_addr_out;
    IP4_ADDR(addr, in.bytes[0], in.bytes[1], in.bytes[2], in.bytes[3]);
}

static int tx_frames;
static int rx_frames;

int lwip_stack_tx_frames(void)
{
    return tx_frames;
}

int lwip_stack_rx_frames(void)
{
    return rx_frames;
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    (void)netif;

    if (nic_dev == 0) {
        return ERR_IF;
    }

    uint8_t frame[1518];
    uint16_t total = 0;

    for (struct pbuf *q = p; q != 0; q = q->next) {
        if ((uint32_t)total + q->len > sizeof(frame)) {
            return ERR_MEM;
        }
        for (uint16_t i = 0; i < q->len; i++) {
            frame[total++] = ((const uint8_t *)q->payload)[i];
        }
    }

    if (eth_send(nic_dev, frame, total) != 0) {
        return ERR_IF;
    }

    tx_frames++;
    eth_drain_tx(nic_dev);
    return ERR_OK;
}

static err_t netdev_init(struct netif *netif)
{
    netif->name[0] = 'e';
    netif->name[1] = 'n';
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;
    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    if (nic_dev != 0) {
        const eth_addr_t *mac = eth_mac(nic_dev);
        for (int i = 0; i < ETH_HWADDR_LEN; i++) {
            netif->hwaddr[i] = mac->bytes[i];
        }
    }

    return ERR_OK;
}

int lwip_stack_init(eth_device_t *dev)
{
    nic_dev = dev;
    lwip_init();

    ip4_addr_t ip = {0};
    ip4_addr_t mask = {0};
    ip4_addr_t gw = {0};

    if (netif_add(&akoya_netif, &ip, &mask, &gw, 0, netdev_init, ethernet_input) == 0) {
        return -1;
    }

    netif_set_default(&akoya_netif);
    netif_set_up(&akoya_netif);
    netif_set_link_up(&akoya_netif);
    return 0;
}

struct netif *lwip_stack_netif(void)
{
    return &akoya_netif;
}

eth_device_t *lwip_stack_device(void)
{
    return nic_dev;
}

static int netdev_collect_frames(int max_frames)
{
    int count = 0;
    int limit = max_frames;
    if (limit <= 0) {
        limit = 1;
    }
    if (limit > RX_STAGE_MAX) {
        limit = RX_STAGE_MAX;
    }

    while (count < limit) {
        uint16_t len = 0;
        if (nic_dev == 0
            || eth_poll(nic_dev, rx_stage[count].data, (uint16_t)sizeof(rx_stage[count].data), &len) != 0
            || len == 0) {
            break;
        }
        rx_stage[count].len = len;
        count++;
        rx_frames++;
    }

    return count;
}

static void netdev_deliver_staged(int count)
{
    for (int i = 0; i < count; i++) {
        struct pbuf *p = pbuf_alloc(PBUF_RAW, rx_stage[i].len, PBUF_POOL);
        if (p == 0) {
            continue;
        }
        if (pbuf_take(p, rx_stage[i].data, rx_stage[i].len) != ERR_OK) {
            pbuf_free(p);
            continue;
        }
        if (akoya_netif.input(p, &akoya_netif) != ERR_OK) {
            pbuf_free(p);
        }
    }
}

void lwip_stack_pump(int max_frames)
{
    int staged = netdev_collect_frames(max_frames);
    netdev_deliver_staged(staged);
    sys_check_timeouts();
}

void lwip_stack_pump_all(void)
{
    int staged;
    do {
        staged = netdev_collect_frames(RX_STAGE_MAX);
        netdev_deliver_staged(staged);
        sys_check_timeouts();
    } while (staged == RX_STAGE_MAX);
}

int lwip_stack_has_ip(void)
{
    if (nic_dev == 0) {
        return 0;
    }
    return !ip4_addr_isany(&akoya_netif.ip_addr);
}

ipv4_config_t lwip_stack_ipv4_config(void)
{
    ipv4_config_t cfg;
    lwip_addr_to_ipv4(&akoya_netif.ip_addr, &cfg.address);
    lwip_addr_to_ipv4(&akoya_netif.netmask, &cfg.subnet);
    lwip_addr_to_ipv4(&akoya_netif.gw, &cfg.gateway);
    return cfg;
}

void lwip_stack_apply_ipv4_config(const ipv4_config_t *config)
{
    if (config == 0) {
        return;
    }

    ip4_addr_t ip;
    ip4_addr_t mask;
    ip4_addr_t gw;
    ipv4_to_lwip_addr(config->address, &ip);
    ipv4_to_lwip_addr(config->subnet, &mask);
    ipv4_to_lwip_addr(config->gateway, &gw);
    netif_set_addr(&akoya_netif, &ip, &mask, &gw);
}

void lwip_stack_arp_prime(ipv4_addr_t dest)
{
    if (nic_dev == 0) {
        return;
    }

    ip4_addr_t ip;
    ipv4_to_lwip_addr(dest, &ip);

    for (int i = 0; i < 8; i++) {
        (void)etharp_query(&akoya_netif, &ip, 0);
        lwip_stack_pump(4);
    }
}
