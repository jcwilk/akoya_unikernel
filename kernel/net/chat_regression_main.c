#include "net/chat_regression_main.h"

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

#ifndef AKOYA_CHAT_REGRESSION_TURNS
#define AKOYA_CHAT_REGRESSION_TURNS 3
#endif

#ifndef AKOYA_CHAT_REGRESSION_GAP_MS
#define AKOYA_CHAT_REGRESSION_GAP_MS 5000U
#endif

static eth_device_t nic;
static int turn_failures;

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

static const char *scheduled_message(int turn_index)
{
    static const char *messages[] = { "hi", "what", "next" };
    if (turn_index < 0 || turn_index >= (int)(sizeof(messages) / sizeof(messages[0]))) {
        return "hi";
    }
    return messages[turn_index];
}

static void report_turn(int turn_number, int pass, const char *reason)
{
    console_write("turn ");
    char num_buf[4];
    if (turn_number >= 10) {
        num_buf[0] = (char)('0' + (turn_number / 10));
        num_buf[1] = (char)('0' + (turn_number % 10));
        num_buf[2] = '\0';
    } else {
        num_buf[0] = (char)('0' + turn_number);
        num_buf[1] = '\0';
    }
    console_write(num_buf);
    if (pass) {
        console_write_line(": PASS");
    } else {
        console_write(": FAIL (");
        console_write(reason);
        console_write_line(")");
        turn_failures++;
    }
}

static int regression_network_bootstrap(void)
{
    time_init();
    ipv4_init();

    console_write_line("timed-gap-chat-regression: multi-turn chat regression runner");

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
        return 1;
    }
    if (ping_status == ICMP_FAIL_UNREACHABLE) {
        console_write_line(" unreachable");
    } else {
        console_write_line(" timeout");
    }
    return 0;
}

void chat_regression_run(void)
{
    turn_failures = 0;

    if (!regression_network_bootstrap()) {
        console_write_line("timed-gap-chat-regression: FAILED (network bootstrap)");
        return;
    }

    http_chat_history_t history;
    http_chat_history_init(&history);

    char reply[512];
    const int total_turns = (int)AKOYA_CHAT_REGRESSION_TURNS;

    for (int turn = 0; turn < total_turns; turn++) {
        if (turn > 0) {
            console_write_prompt("> ");
            time_delay_ms(AKOYA_CHAT_REGRESSION_GAP_MS);
        }

        const char *message = scheduled_message(turn);
        if (http_chat_history_would_overflow(&history, message)) {
            report_turn(turn + 1, 0, "overflow");
            continue;
        }
        if (http_chat_history_add_user(&history, message) != 0) {
            report_turn(turn + 1, 0, "overflow");
            continue;
        }

        http_chat_status_t status = http_chat_run_turn(&history, reply, (int)sizeof(reply));
        if (status == HTTP_CHAT_OK) {
            http_chat_history_set_assistant(&history, reply);
            http_chat_emit_ok(reply);
            report_turn(turn + 1, 1, 0);
        } else {
            http_chat_emit_fail(http_chat_status_reason(status));
            report_turn(turn + 1, 0, http_chat_status_reason(status));
        }
    }

    if (turn_failures == 0) {
        console_write_line("timed-gap-chat-regression: ALL PASS");
    } else {
        char count_buf[8];
        int count = turn_failures;
        int pos = 0;
        if (count == 0) {
            count_buf[pos++] = '0';
        } else {
            char tmp[8];
            int tmp_idx = 0;
            while (count > 0 && tmp_idx < (int)sizeof(tmp)) {
                tmp[tmp_idx++] = (char)('0' + (count % 10));
                count /= 10;
            }
            while (tmp_idx > 0 && pos < (int)sizeof(count_buf) - 1) {
                count_buf[pos++] = tmp[--tmp_idx];
            }
        }
        count_buf[pos] = '\0';
        console_write("timed-gap-chat-regression: FAILED (");
        console_write(count_buf);
        console_write_line(" failures)");
    }
}
