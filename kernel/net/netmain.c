#include "net/netmain.h"

#include "console/console.h"
#include "net/dhcp/dhcp.h"
#include "net/eth/eth.h"
#include "net/http/http_chat.h"
#include "net/icmp/icmp.h"
#include "net/ipv4/ipv4.h"
#include "net/link/link.h"
#include "time/time.h"

#ifndef AKOYA_CHAT_HOST_IP0
#define AKOYA_CHAT_HOST_IP0 192
#endif
#ifndef AKOYA_CHAT_HOST_IP1
#define AKOYA_CHAT_HOST_IP1 168
#endif
#ifndef AKOYA_CHAT_HOST_IP2
#define AKOYA_CHAT_HOST_IP2 1
#endif
#ifndef AKOYA_CHAT_HOST_IP3
#define AKOYA_CHAT_HOST_IP3 110
#endif

static eth_device_t nic;

static void console_write_ipv4(ipv4_addr_t addr)
{
    char buffer[4];
    for (int octet = 0; octet < 4; octet++) {
        uint8_t value = addr.bytes[octet];
        if (value >= 100) {
            buffer[0] = (char)('0' + (value / 100));
            buffer[1] = (char)('0' + ((value / 10) % 10));
            buffer[2] = (char)('0' + (value % 10));
            buffer[3] = '\0';
            console_write(buffer);
        } else if (value >= 10) {
            buffer[0] = (char)('0' + (value / 10));
            buffer[1] = (char)('0' + (value % 10));
            buffer[2] = '\0';
            console_write(buffer);
        } else {
            buffer[0] = (char)('0' + value);
            buffer[1] = '\0';
            console_write(buffer);
        }
        if (octet < 3) {
            console_write(".");
        }
    }
}

void net_bootstrap(void)
{
    time_init();
    ipv4_init();

    if (eth_init(&nic) != 0) {
        console_write_line("net_link=fail reason=nic");
        return;
    }

    link_init(&nic);
    console_write_line("net_link=ok");

    time_delay_ms(500U);

    ipv4_config_t config;
    config.address.bytes[0] = 0;
    config.address.bytes[1] = 0;
    config.address.bytes[2] = 0;
    config.address.bytes[3] = 0;
    config.subnet = config.address;
    config.gateway = config.address;

    dhcp_status_t dhcp_status = dhcp_acquire(&config, 30000U);
    if (dhcp_status != DHCP_OK) {
        console_write("net_ip=fail reason=");
        if (dhcp_status == DHCP_FAIL_NO_OFFER) {
            console_write_line("no-offer");
        } else if (dhcp_status == DHCP_FAIL_NO_ACK) {
            console_write_line("no-ack");
        } else {
            console_write_line("timeout");
        }
        return;
    }

    ipv4_set_config(&config);
    console_write("net_ip=");
    console_write_ipv4(config.address);
    console_write_line("");

    if (ipv4_is_zero(config.gateway)) {
        config.gateway = config.address;
        config.gateway.bytes[3] = 254U;
        ipv4_set_config(&config);
    }

    link_announce_ipv4(config.address);
    for (int i = 0; i < 100; i++) {
        link_poll();
        time_delay_ms(10U);
    }

    if (!ipv4_is_zero(config.gateway)) {
        eth_addr_t gw_mac;
        (void)link_resolve_ipv4(config.gateway, &gw_mac, 3000U);
    }

    ipv4_addr_t target;
    target.bytes[0] = AKOYA_CHAT_HOST_IP0;
    target.bytes[1] = AKOYA_CHAT_HOST_IP1;
    target.bytes[2] = AKOYA_CHAT_HOST_IP2;
    target.bytes[3] = AKOYA_CHAT_HOST_IP3;

    console_clear();

    uint32_t rtt_ms = 0;
    icmp_status_t ping_status = icmp_ping(target, 30000U, &rtt_ms);

    console_write_ipv4(target);

    if (ping_status == ICMP_OK) {
        console_write_line(" reachable");
        (void)http_chat_session();
    } else if (ping_status == ICMP_FAIL_UNREACHABLE) {
        console_write_line(" unreachable");
    } else {
        console_write_line(" timeout");
    }
}
