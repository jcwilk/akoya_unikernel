#include "net/net_async.h"

#include "console/console.h"
#include "event/runtime.h"
#include "input/ps2_keyboard.h"
#include "net/dhcp/dhcp.h"
#include "net/eth/eth.h"
#include "net/eth/nic_device.h"
#include "net/http/http_chat.h"
#include "net/icmp/icmp.h"
#include "net/ipv4/ipv4.h"
#include "net/link/link.h"
#include "net/tcp/tcp.h"
#include "time/timer.h"
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

#ifndef AKOYA_CHAT_TIMEOUT_MS
#define AKOYA_CHAT_TIMEOUT_MS 60000U
#endif

#ifndef AKOYA_CHAT_PORT
#define AKOYA_CHAT_PORT 11435
#endif

static eth_device_t nic;

typedef enum {
    BOOT_LINK,
    BOOT_SETTLE,
    BOOT_DHCP,
    BOOT_IP_PRINT,
    BOOT_ANNOUNCE,
    BOOT_ARP,
    BOOT_CLEAR,
    BOOT_ICMP,
    BOOT_CHAT,
    BOOT_DONE
} boot_phase_t;

typedef struct {
    boot_phase_t phase;
    dhcp_sm_t dhcp;
    arp_sm_t arp;
    icmp_sm_t icmp;
    int regression_mode;
    int network_ok;
    uint32_t settle_until_ms;
    uint32_t announce_until_ms;
} boot_ctx_t;

typedef enum {
    CHAT_IDLE,
    CHAT_PROMPT,
    CHAT_INPUT,
    CHAT_TURN_DRAIN,
    CHAT_TURN_OPEN,
    CHAT_TURN_SEND,
    CHAT_TURN_RECV,
    CHAT_TURN_CLOSE,
    CHAT_TURN_POST,
    CHAT_GAP_WAIT,
    CHAT_EXIT
} chat_phase_t;

#define HTTP_CHAT_RECV_CAP 4096U

typedef struct {
    chat_phase_t phase;
    http_chat_history_t history;
    char line[PS2_LINE_MAX];
    int line_len;
    char reply[512];
    char request[512 + 8192];
    int request_len;
    uint8_t response[HTTP_CHAT_RECV_CAP];
    tcp_sm_t tcp;
    http_chat_status_t turn_status;
    int regression_mode;
    int regression_turn;
    int turn_failures;
    int gap_ready;
    int gap_timer_handle;
} chat_ctx_t;

static boot_ctx_t boot;
static chat_ctx_t chat;

static int http_recv_complete_wrapper(const uint8_t *buf, uint16_t len, void *ctx)
{
    (void)ctx;
    return http_chat_response_complete(buf, len);
}

static void gap_timer_done(void *ctx)
{
    (void)ctx;
    chat.gap_ready = 1;
}

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

static int str_eq_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        char ca = *a;
        char cb = *b;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (char)(ca + ('a' - 'A'));
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (char)(cb + ('a' - 'A'));
        }
        if (ca != cb) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
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
        chat.turn_failures++;
    }
}

static int boot_active(void)
{
    return boot.phase != BOOT_DONE;
}

static void boot_step(void)
{
    switch (boot.phase) {
    case BOOT_LINK:
        time_init();
        ipv4_init();
        if (eth_init(&nic) != 0) {
            console_write_line("net_link=fail reason=nic");
            boot.phase = BOOT_DONE;
            return;
        }
        link_init(&nic);
        ps2_keyboard_init();
        ps2_keyboard_register_device();
        runtime_set_keyboard_device(ps2_keyboard_device());
        nic_device_init(&nic);
        runtime_set_nic_device(nic_device());
        console_write_line("net_link=ok");
        boot.settle_until_ms = time_millis() + 500U;
        boot.phase = BOOT_SETTLE;
        return;
    case BOOT_SETTLE:
        if (time_millis() < boot.settle_until_ms) {
            return;
        }
        dhcp_sm_begin(&boot.dhcp, 30000U);
        boot.phase = BOOT_DHCP;
        return;
    case BOOT_DHCP:
        if (!dhcp_sm_step(&boot.dhcp)) {
            return;
        }
        if (dhcp_sm_result(&boot.dhcp) != DHCP_OK) {
            console_write("net_ip=fail reason=");
            if (dhcp_sm_result(&boot.dhcp) == DHCP_FAIL_NO_OFFER) {
                console_write_line("no-offer");
            } else if (dhcp_sm_result(&boot.dhcp) == DHCP_FAIL_NO_ACK) {
                console_write_line("no-ack");
            } else {
                console_write_line("timeout");
            }
            boot.phase = BOOT_DONE;
            return;
        }
        ipv4_set_config(dhcp_sm_config(&boot.dhcp));
        console_write("net_ip=");
        console_write_ipv4(dhcp_sm_config(&boot.dhcp)->address);
        console_write_line("");

        {
            ipv4_config_t config = *ipv4_get_config();
            if (ipv4_is_zero(config.gateway)) {
                config.gateway = config.address;
                config.gateway.bytes[3] = 254U;
                ipv4_set_config(&config);
            }
        }

        link_announce_ipv4(ipv4_get_config()->address);
        boot.announce_until_ms = time_millis() + 1000U;
        boot.phase = BOOT_ANNOUNCE;
        return;
    case BOOT_ANNOUNCE:
        if (time_millis() < boot.announce_until_ms) {
            return;
        }
        if (!ipv4_is_zero(ipv4_get_config()->gateway)) {
            arp_sm_begin(&boot.arp, ipv4_get_config()->gateway, 3000U);
            boot.phase = BOOT_ARP;
        } else {
            boot.phase = BOOT_CLEAR;
        }
        return;
    case BOOT_ARP:
        if (!arp_sm_step(&boot.arp)) {
            return;
        }
        boot.phase = BOOT_CLEAR;
        return;
    case BOOT_CLEAR:
        console_clear();
        icmp_sm_begin(&boot.icmp, chat_host_ip(), 30000U);
        boot.phase = BOOT_ICMP;
        return;
    case BOOT_ICMP: {
        if (!icmp_sm_step(&boot.icmp)) {
            return;
        }
        ipv4_addr_t target = chat_host_ip();
        console_write_ipv4(target);
        if (icmp_sm_result(&boot.icmp) == ICMP_OK) {
            console_write_line(" reachable");
            boot.network_ok = 1;
            boot.phase = BOOT_CHAT;
        } else if (icmp_sm_result(&boot.icmp) == ICMP_FAIL_UNREACHABLE) {
            console_write_line(" unreachable");
            boot.phase = BOOT_DONE;
        } else {
            console_write_line(" timeout");
            boot.phase = BOOT_DONE;
        }
        return;
    }
    case BOOT_CHAT:
        if (boot.regression_mode) {
            chat.regression_mode = 1;
            chat.regression_turn = 0;
            chat.phase = CHAT_GAP_WAIT;
            chat.gap_ready = 1;
            chat.gap_timer_handle = TIMER_HANDLE_INVALID;
        } else {
            chat.phase = CHAT_PROMPT;
        }
        boot.phase = BOOT_DONE;
        return;
    default:
        return;
    }
}

static int chat_active(void)
{
    return chat.phase != CHAT_IDLE && chat.phase != CHAT_EXIT;
}

static void chat_begin_turn(const char *message)
{
    if (http_chat_history_would_overflow(&chat.history, message)) {
        chat.turn_status = HTTP_CHAT_FAIL_OVERFLOW;
        chat.phase = CHAT_PROMPT;
        return;
    }
    if (http_chat_history_add_user(&chat.history, message) != 0) {
        chat.turn_status = HTTP_CHAT_FAIL_OVERFLOW;
        chat.phase = CHAT_PROMPT;
        return;
    }

    tcp_sm_init(&chat.tcp);
    tcp_sm_begin_drain(&chat.tcp, TCP_CLOSE_DRAIN_MS, 1);
    chat.phase = CHAT_TURN_DRAIN;
}

static void chat_step(void)
{
    switch (chat.phase) {
    case CHAT_PROMPT:
        console_write_prompt("> ");
        chat.line_len = 0;
        chat.line[0] = '\0';
        chat.phase = CHAT_INPUT;
        return;
    case CHAT_INPUT: {
        unsigned char scan = 0;
        if (ps2_keyboard_pop_scancode(&scan) != 0) {
            return;
        }
        if (ps2_process_scancode(scan, chat.line, (int)sizeof(chat.line), &chat.line_len) != 0) {
            if (str_eq_ci(chat.line, "quit") || str_eq_ci(chat.line, "exit")) {
                console_write_line("chat_session=exit");
                chat.phase = CHAT_EXIT;
                runtime_request_halt();
                return;
            }
            chat_begin_turn(chat.line);
        }
        return;
    }
    case CHAT_TURN_DRAIN:
        if (!tcp_sm_step(&chat.tcp)) {
            return;
        }
        if (tcp_sm_result(&chat.tcp) != TCP_OK) {
            chat.turn_status = HTTP_CHAT_FAIL_TRANSPORT_LIFECYCLE;
            chat.phase = CHAT_PROMPT;
            return;
        }
        tcp_sm_init(&chat.tcp);
        tcp_sm_begin_open(&chat.tcp, chat_host_ip(), (uint16_t)AKOYA_CHAT_PORT, AKOYA_CHAT_TIMEOUT_MS);
        chat.phase = CHAT_TURN_OPEN;
        return;
    case CHAT_TURN_OPEN:
        if (!tcp_sm_step(&chat.tcp)) {
            return;
        }
        if (tcp_sm_result(&chat.tcp) != TCP_OK) {
            chat.turn_status = (tcp_sm_result(&chat.tcp) == TCP_FAIL_TIMEOUT)
                ? HTTP_CHAT_FAIL_TIMEOUT
                : HTTP_CHAT_FAIL_CONNECT;
            tcp_sm_init(&chat.tcp);
            tcp_sm_begin_drain(&chat.tcp, TCP_CLOSE_DRAIN_MS, 1);
            chat.phase = CHAT_TURN_POST;
            return;
        }
        if (http_chat_build_turn_request(&chat.history, chat.request, (int)sizeof(chat.request), &chat.request_len) != 0) {
            chat.turn_status = HTTP_CHAT_FAIL_OVERFLOW;
            tcp_sm_init(&chat.tcp);
            tcp_sm_begin_close(&chat.tcp);
            chat.phase = CHAT_TURN_CLOSE;
            return;
        }
        tcp_sm_begin_send(&chat.tcp, (const uint8_t *)chat.request, (uint16_t)chat.request_len);
        chat.phase = CHAT_TURN_SEND;
        return;
    case CHAT_TURN_SEND:
        if (!tcp_sm_step(&chat.tcp)) {
            return;
        }
        if (tcp_sm_result(&chat.tcp) != TCP_OK) {
            chat.turn_status = HTTP_CHAT_FAIL_CONNECT;
            tcp_sm_init(&chat.tcp);
            tcp_sm_begin_close(&chat.tcp);
            chat.phase = CHAT_TURN_CLOSE;
            return;
        }
        tcp_sm_begin_recv(&chat.tcp, chat.response, (uint16_t)sizeof(chat.response),
            http_recv_complete_wrapper, 0, AKOYA_CHAT_TIMEOUT_MS);
        chat.phase = CHAT_TURN_RECV;
        return;
    case CHAT_TURN_RECV:
        if (!tcp_sm_step(&chat.tcp)) {
            return;
        }
        if (tcp_sm_result(&chat.tcp) != TCP_OK) {
            chat.turn_status = (tcp_sm_result(&chat.tcp) == TCP_FAIL_TIMEOUT)
                ? HTTP_CHAT_FAIL_TIMEOUT
                : HTTP_CHAT_FAIL_CONNECT;
        } else {
            chat.turn_status = http_chat_parse_turn_response(
                chat.response, tcp_sm_recv_len(&chat.tcp), chat.reply, (int)sizeof(chat.reply));
        }
        tcp_sm_init(&chat.tcp);
        tcp_sm_begin_close(&chat.tcp);
        chat.phase = CHAT_TURN_CLOSE;
        return;
    case CHAT_TURN_CLOSE:
        if (!tcp_sm_step(&chat.tcp)) {
            return;
        }
        tcp_sm_init(&chat.tcp);
        tcp_sm_begin_drain(&chat.tcp, TCP_CLOSE_DRAIN_MS, 1);
        chat.phase = CHAT_TURN_POST;
        return;
    case CHAT_TURN_POST:
        if (!tcp_sm_step(&chat.tcp)) {
            return;
        }
        if (chat.turn_status == HTTP_CHAT_OK) {
            http_chat_history_set_assistant(&chat.history, chat.reply);
            http_chat_emit_ok(chat.reply);
        } else {
            http_chat_emit_fail(http_chat_status_reason(chat.turn_status));
        }
        if (chat.regression_mode) {
            report_turn(chat.regression_turn + 1, chat.turn_status == HTTP_CHAT_OK,
                http_chat_status_reason(chat.turn_status));
            chat.regression_turn++;
            if (chat.regression_turn >= (int)AKOYA_CHAT_REGRESSION_TURNS) {
                if (chat.turn_failures == 0) {
                    console_write_line("timed-gap-chat-regression: ALL PASS");
                } else {
                    console_write_line("timed-gap-chat-regression: FAILED");
                }
                chat.phase = CHAT_EXIT;
                runtime_request_halt();
            } else {
                chat.phase = CHAT_GAP_WAIT;
                chat.gap_ready = 0;
                chat.gap_timer_handle = TIMER_HANDLE_INVALID;
            }
        } else {
            chat.phase = CHAT_PROMPT;
        }
        return;
    case CHAT_GAP_WAIT:
        if (chat.gap_ready) {
            chat_begin_turn(scheduled_message(chat.regression_turn));
            chat.gap_ready = 0;
            return;
        }
        if (chat.gap_timer_handle == TIMER_HANDLE_INVALID) {
            console_write_prompt("> ");
            chat.gap_timer_handle = timer_schedule(AKOYA_CHAT_REGRESSION_GAP_MS, gap_timer_done, 0);
        }
        return;
    default:
        return;
    }
}

static void runtime_common_init(int regression)
{
    runtime_init();
    boot.phase = BOOT_LINK;
    boot.network_ok = 0;
    boot.regression_mode = regression;
    chat.phase = CHAT_IDLE;
    chat.turn_failures = 0;
    chat.gap_timer_handle = TIMER_HANDLE_INVALID;
    http_chat_history_init(&chat.history);

    runtime_register_slot(boot_step, boot_active);
    runtime_register_slot(chat_step, chat_active);
}

void net_async_start(void)
{
    runtime_common_init(0);
    runtime_run_until_halt();
}

void chat_regression_async_start(void)
{
    console_write_line("timed-gap-chat-regression: multi-turn chat regression runner");
    runtime_common_init(1);
    runtime_run_until_halt();
}
