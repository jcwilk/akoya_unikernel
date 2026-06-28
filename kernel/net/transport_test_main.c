#include "net/transport_test_main.h"

#include "console/console.h"
#include "net/dhcp/dhcp.h"
#include "net/eth/eth.h"
#include "net/icmp/icmp.h"
#include "net/ipv4/ipv4.h"
#include "net/link/link.h"
#include "net/tcp/tcp.h"
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
#ifndef AKOYA_CHAT_PORT
#define AKOYA_CHAT_PORT 11435
#endif

#ifndef AKOYA_TRANSPORT_REFUSED_PORT
#define AKOYA_TRANSPORT_REFUSED_PORT 19999
#endif

static eth_device_t nic;
static int scenario_failures;

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

static ipv4_addr_t chat_host_ip(void)
{
    ipv4_addr_t target;
    target.bytes[0] = AKOYA_CHAT_HOST_IP0;
    target.bytes[1] = AKOYA_CHAT_HOST_IP1;
    target.bytes[2] = AKOYA_CHAT_HOST_IP2;
    target.bytes[3] = AKOYA_CHAT_HOST_IP3;
    return target;
}

static void report_scenario(const char *name, int pass, const char *reason)
{
    console_write("scenario ");
    console_write(name);
    if (pass) {
        console_write_line(": PASS");
    } else {
        console_write(": FAIL (");
        console_write(reason);
        console_write_line(")");
        scenario_failures++;
    }
}

static const char *tcp_status_reason(tcp_status_t status)
{
    switch (status) {
    case TCP_OK:
        return "ok";
    case TCP_FAIL_CONNECT:
        return "connect";
    case TCP_FAIL_TIMEOUT:
        return "timeout";
    case TCP_FAIL_SEND:
        return "send";
    case TCP_FAIL_RECV:
        return "recv";
    default:
        return "unknown";
    }
}

static int tcp_connect_and_close(ipv4_addr_t dest, uint16_t port, uint32_t timeout_ms)
{
    tcp_session_t session;
    tcp_status_t status = tcp_session_open(&session, dest, port, timeout_ms);
    if (status != TCP_OK) {
        return 0;
    }
    tcp_session_close(&session);
    if (!tcp_drain_until_inactive(TCP_CLOSE_DRAIN_MS)) {
        return 0;
    }
    return 1;
}

static int transport_network_bootstrap(ipv4_addr_t *chat_host_out)
{
    time_init();
    ipv4_init();

    console_write_line("transport-test: network transport test runner");

    if (eth_init(&nic) != 0) {
        console_write_line("net_link=fail reason=nic");
        return 0;
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
        return 0;
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

    ipv4_addr_t target = chat_host_ip();
    if (chat_host_out != 0) {
        *chat_host_out = target;
    }

    console_clear();

    uint32_t rtt_ms = 0;
    icmp_status_t ping_status = icmp_ping(target, 30000U, &rtt_ms);

    console_write_ipv4(target);

    if (ping_status == ICMP_OK) {
        console_write_line(" reachable");
        return 1;
    }
    if (ping_status == ICMP_FAIL_UNREACHABLE) {
        console_write_line(" unreachable");
    } else {
        console_write_line(" timeout");
    }
    return 0;
}

static void scenario_sequential_same_host(ipv4_addr_t host, uint16_t port)
{
    const int attempts = 3;
    int ok = 1;

    for (int i = 0; i < attempts; i++) {
        if (!tcp_connect_and_close(host, port, 15000U)) {
            ok = 0;
            break;
        }
        time_delay_ms(500U);
    }

    if (ok) {
        report_scenario("sequential-same-host", 1, 0);
    } else {
        report_scenario("sequential-same-host", 0, "connect failed");
    }
}

static void scenario_delayed_reconnect(ipv4_addr_t host, uint16_t port)
{
    if (!tcp_connect_and_close(host, port, 15000U)) {
        report_scenario("delayed-reconnect", 0, "first connect failed");
        return;
    }

    time_delay_ms(3000U);

    if (tcp_connect_and_close(host, port, 15000U)) {
        report_scenario("delayed-reconnect", 1, 0);
    } else {
        report_scenario("delayed-reconnect", 0, "second connect failed");
    }
}

static void scenario_refused_or_timeout(ipv4_addr_t host)
{
    tcp_session_t session;
    uint16_t refused_port = AKOYA_TRANSPORT_REFUSED_PORT;
    tcp_status_t status = tcp_session_open(&session, host, refused_port, 5000U);

    if (status == TCP_OK) {
        tcp_session_close(&session);
        report_scenario("refused-or-timeout", 0, "unexpected connect success");
        return;
    }

    if (status == TCP_FAIL_CONNECT || status == TCP_FAIL_TIMEOUT) {
        report_scenario("refused-or-timeout", 1, 0);
        return;
    }

    report_scenario("refused-or-timeout", 0, tcp_status_reason(status));
}

void transport_test_run(void)
{
    ipv4_addr_t chat_host;
    scenario_failures = 0;

    if (!transport_network_bootstrap(&chat_host)) {
        console_write_line("transport-test: FAILED (network bootstrap)");
        return;
    }

    scenario_sequential_same_host(chat_host, (uint16_t)AKOYA_CHAT_PORT);
    scenario_delayed_reconnect(chat_host, (uint16_t)AKOYA_CHAT_PORT);
    scenario_refused_or_timeout(chat_host);

    if (scenario_failures == 0) {
        console_write_line("transport-test: ALL PASS");
    } else {
        char count_buf[4];
        int count = scenario_failures;
        if (count >= 100) {
            count_buf[0] = (char)('0' + (count / 100));
            count_buf[1] = (char)('0' + ((count / 10) % 10));
            count_buf[2] = (char)('0' + (count % 10));
            count_buf[3] = '\0';
        } else if (count >= 10) {
            count_buf[0] = (char)('0' + (count / 10));
            count_buf[1] = (char)('0' + (count % 10));
            count_buf[2] = '\0';
        } else {
            count_buf[0] = (char)('0' + count);
            count_buf[1] = '\0';
        }
        console_write("transport-test: FAILED (");
        console_write(count_buf);
        console_write_line(" failures)");
    }
}
