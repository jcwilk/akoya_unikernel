#include "net/http/http_chat.h"

#include "console/console.h"
#include "input/ps2_keyboard.h"
#include "input/ps2_readline.h"
#include "net/tcp/tcp.h"

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
#define AKOYA_CHAT_HOST_IP3 2
#endif

#ifndef AKOYA_CHAT_PATH
#define AKOYA_CHAT_PATH "/v1/chat/completions"
#endif

#ifndef AKOYA_CHAT_MODEL
#define AKOYA_CHAT_MODEL "fast-text-qwen3-8b"
#endif

#ifndef AKOYA_CHAT_TIMEOUT_MS
#define AKOYA_CHAT_TIMEOUT_MS 60000U
#endif

#ifndef AKOYA_CHAT_PORT
#define AKOYA_CHAT_PORT 11435
#endif

#ifndef AKOYA_CHAT_MAX_TOKENS
#define AKOYA_CHAT_MAX_TOKENS 500
#endif

#define HTTP_CHAT_RECV_CAP 4096U
#define HTTP_CHAT_REPLY_CONSOLE_MAX 256U
#define CHAT_JSON_BUDGET 8192U

static int find_substr(const char *haystack, int hay_len, const char *needle)
{
    int needle_len = 0;
    while (needle[needle_len] != '\0') {
        needle_len++;
    }
    if (needle_len == 0 || hay_len < needle_len) {
        return -1;
    }

    for (int i = 0; i <= hay_len - needle_len; i++) {
        int match = 1;
        for (int j = 0; j < needle_len; j++) {
            if (haystack[i + j] != needle[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            return i;
        }
    }
    return -1;
}

static int parse_http_status(const char *response, int length, int *status_out)
{
    if (length < 12) {
        return -1;
    }
    if (response[0] != 'H' || response[1] != 'T' || response[2] != 'T' || response[3] != 'P') {
        return -1;
    }

    int i = 0;
    while (i < length && response[i] != ' ') {
        i++;
    }
    if (i >= length) {
        return -1;
    }
    i++;

    int status = 0;
    int digits = 0;
    while (i < length && response[i] >= '0' && response[i] <= '9') {
        status = status * 10 + (response[i] - '0');
        digits++;
        i++;
    }
    if (digits != 3) {
        return -1;
    }

    *status_out = status;
    return 0;
}

static int body_offset(const char *response, int length)
{
    for (int i = 0; i + 3 < length; i++) {
        if (response[i] == '\r' && response[i + 1] == '\n'
            && response[i + 2] == '\r' && response[i + 3] == '\n') {
            return i + 4;
        }
    }
    return -1;
}

static int extract_json_content(const char *body, int body_len, char *out, int out_cap)
{
    int key_pos = find_substr(body, body_len, "\"content\"");
    if (key_pos < 0) {
        return -1;
    }

    int i = key_pos + 9;
    while (i < body_len && (body[i] == ' ' || body[i] == '\t' || body[i] == ':')) {
        i++;
    }
    if (i >= body_len || body[i] != '"') {
        return -1;
    }
    i++;

    int out_len = 0;
    while (i < body_len && body[i] != '"') {
        char c = body[i];
        if (c == '\\' && i + 1 < body_len) {
            i++;
            c = body[i];
        }
        if (out_len + 1 >= out_cap) {
            break;
        }
        out[out_len++] = c;
        i++;
    }

    if (out_len == 0) {
        return -1;
    }

    out[out_len] = '\0';
    return out_len;
}

static void console_write_reply_truncated(const char *reply)
{
    int written = 0;
    for (const char *p = reply; *p != '\0' && written < (int)HTTP_CHAT_REPLY_CONSOLE_MAX; p++) {
        char ch[2] = { *p, '\0' };
        if (*p == '\n' || *p == '\r') {
            ch[0] = ' ';
        }
        console_write(ch);
        written++;
    }
}

static void emit_fail(const char *reason)
{
    console_write("chat failed: ");
    console_write_line(reason);
}

void http_chat_emit_fail(const char *reason)
{
    emit_fail(reason);
}

static void emit_ok(const char *reply)
{
    console_write_reply_truncated(reply);
    console_newline();
}

void http_chat_emit_ok(const char *reply)
{
    emit_ok(reply);
}

static void format_host_ip(char *host, int cap)
{
    int pos = 0;
    uint8_t octets[4] = {
        AKOYA_CHAT_HOST_IP0,
        AKOYA_CHAT_HOST_IP1,
        AKOYA_CHAT_HOST_IP2,
        AKOYA_CHAT_HOST_IP3
    };

    for (int o = 0; o < 4; o++) {
        uint8_t value = octets[o];
        if (value >= 100) {
            if (pos + 3 >= cap) {
                return;
            }
            host[pos++] = (char)('0' + (value / 100));
            host[pos++] = (char)('0' + ((value / 10) % 10));
            host[pos++] = (char)('0' + (value % 10));
        } else if (value >= 10) {
            if (pos + 2 >= cap) {
                return;
            }
            host[pos++] = (char)('0' + (value / 10));
            host[pos++] = (char)('0' + (value % 10));
        } else {
            if (pos + 1 >= cap) {
                return;
            }
            host[pos++] = (char)('0' + value);
        }
        if (o < 3 && pos < cap - 1) {
            host[pos++] = '.';
        }
    }
    if (pos < cap) {
        host[pos] = '\0';
    }
}

static int str_len(const char *s)
{
    int len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
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

static int is_exit_command(const char *line)
{
    return str_eq_ci(line, "quit") || str_eq_ci(line, "exit");
}

static int estimate_json_size(const http_chat_history_t *history);

void http_chat_history_init(http_chat_history_t *history)
{
    history->count = 0;
}

int http_chat_history_add_user(http_chat_history_t *history, const char *msg)
{
    if (history->count >= (int)HTTP_CHAT_HISTORY_MAX_TURNS) {
        return -1;
    }

    http_chat_turn_t *turn = &history->turns[history->count];
    int i = 0;
    while (msg[i] != '\0' && i < (int)HTTP_CHAT_MSG_MAX_LEN - 1) {
        turn->user[i] = msg[i];
        i++;
    }
    turn->user[i] = '\0';
    turn->assistant[0] = '\0';
    turn->has_assistant = 0;
    history->count++;
    return 0;
}

void http_chat_history_set_assistant(http_chat_history_t *history, const char *reply)
{
    if (history->count <= 0) {
        return;
    }

    http_chat_turn_t *turn = &history->turns[history->count - 1];
    int i = 0;
    while (reply[i] != '\0' && i < (int)HTTP_CHAT_MSG_MAX_LEN - 1) {
        turn->assistant[i] = reply[i];
        i++;
    }
    turn->assistant[i] = '\0';
    turn->has_assistant = 1;
}

int http_chat_history_would_overflow(const http_chat_history_t *history, const char *new_user_msg)
{
    if (history->count >= (int)HTTP_CHAT_HISTORY_MAX_TURNS) {
        return 1;
    }

    int projected = estimate_json_size(history) + 32 + str_len(new_user_msg);
    return projected >= (int)CHAT_JSON_BUDGET;
}

static int json_append_escaped(char *buf, int cap, int *pos, const char *text)
{
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c == '"' || c == '\\') {
            if (*pos + 2 >= cap) {
                return -1;
            }
            buf[(*pos)++] = '\\';
            buf[(*pos)++] = c;
        } else if (c >= 32 && c <= 126) {
            if (*pos + 1 >= cap) {
                return -1;
            }
            buf[(*pos)++] = c;
        }
    }
    return 0;
}

static int estimate_json_size(const http_chat_history_t *history)
{
    int size = 32 + str_len(AKOYA_CHAT_MODEL) + 24;
    for (int t = 0; t < history->count; t++) {
        size += 32 + str_len(history->turns[t].user);
        if (history->turns[t].has_assistant) {
            size += 32 + str_len(history->turns[t].assistant);
        }
    }
    return size;
}

static int chat_history_would_overflow(const http_chat_history_t *history, const char *new_user_msg)
{
    return http_chat_history_would_overflow(history, new_user_msg);
}

static int build_messages_json(const http_chat_history_t *history, char *json, int cap)
{
    int pos = 0;
    const char *prefix = "{\"model\":\"";
    const char *mid = "\",\"messages\":[";

    for (const char *p = prefix; *p != '\0'; p++) {
        if (pos + 1 >= cap) {
            return -1;
        }
        json[pos++] = *p;
    }
    for (const char *p = AKOYA_CHAT_MODEL; *p != '\0'; p++) {
        if (pos + 1 >= cap) {
            return -1;
        }
        json[pos++] = *p;
    }
    for (const char *p = mid; *p != '\0'; p++) {
        if (pos + 1 >= cap) {
            return -1;
        }
        json[pos++] = *p;
    }

    for (int t = 0; t < history->count; t++) {
        if (t > 0) {
            if (pos + 1 >= cap) {
                return -1;
            }
            json[pos++] = ',';
        }

        const char *user_prefix = "{\"role\":\"user\",\"content\":\"";
        for (const char *p = user_prefix; *p != '\0'; p++) {
            if (pos + 1 >= cap) {
                return -1;
            }
            json[pos++] = *p;
        }
        if (json_append_escaped(json, cap, &pos, history->turns[t].user) != 0) {
            return -1;
        }
        if (pos + 2 >= cap) {
            return -1;
        }
        json[pos++] = '"';
        json[pos++] = '}';

        if (history->turns[t].has_assistant) {
            if (pos + 1 >= cap) {
                return -1;
            }
            json[pos++] = ',';
            const char *asst_prefix = "{\"role\":\"assistant\",\"content\":\"";
            for (const char *p = asst_prefix; *p != '\0'; p++) {
                if (pos + 1 >= cap) {
                    return -1;
                }
                json[pos++] = *p;
            }
            if (json_append_escaped(json, cap, &pos, history->turns[t].assistant) != 0) {
                return -1;
            }
            if (pos + 2 >= cap) {
                return -1;
            }
            json[pos++] = '"';
            json[pos++] = '}';
        }
    }

    if (pos + 2 >= cap) {
        return -1;
    }
    json[pos++] = ']';

    const char *max_prefix = ",\"max_tokens\":";
    for (const char *p = max_prefix; *p != '\0'; p++) {
        if (pos + 1 >= cap) {
            return -1;
        }
        json[pos++] = *p;
    }

    unsigned int max_tokens = (unsigned int)AKOYA_CHAT_MAX_TOKENS;
    if (max_tokens == 0U) {
        if (pos + 1 >= cap) {
            return -1;
        }
        json[pos++] = '0';
    } else {
        char tmp[12];
        int tmp_idx = 0;
        while (max_tokens > 0U && tmp_idx < (int)sizeof(tmp)) {
            tmp[tmp_idx++] = (char)('0' + (max_tokens % 10U));
            max_tokens /= 10U;
        }
        while (tmp_idx > 0) {
            if (pos + 1 >= cap) {
                return -1;
            }
            json[pos++] = tmp[--tmp_idx];
        }
    }

    if (pos + 1 >= cap) {
        return -1;
    }
    json[pos++] = '}';
    json[pos] = '\0';
    return pos;
}

static int parse_content_length(const char *response, int length, int hdr_end, int *cl_out)
{
    const char *needle = "content-length:";
    int needle_len = 15;

    for (int i = 0; i + needle_len < hdr_end; i++) {
        int match = 1;
        for (int j = 0; j < needle_len; j++) {
            char a = response[i + j];
            char b = needle[j];
            if (a >= 'A' && a <= 'Z') {
                a = (char)(a + ('a' - 'A'));
            }
            if (a != b) {
                match = 0;
                break;
            }
        }
        if (!match) {
            continue;
        }

        int p = i + needle_len;
        while (p < hdr_end && (response[p] == ' ' || response[p] == '\t')) {
            p++;
        }

        int value = 0;
        int digits = 0;
        while (p < hdr_end && response[p] >= '0' && response[p] <= '9') {
            value = value * 10 + (response[p] - '0');
            digits++;
            p++;
        }

        if (digits == 0) {
            return -1;
        }

        *cl_out = value;
        return 0;
    }

    return -1;
}

static int http_response_complete(const uint8_t *buf, uint16_t len, void *ctx)
{
    (void)ctx;

    if (len < 16) {
        return 0;
    }

    int hdr_end = body_offset((const char *)buf, (int)len);
    if (hdr_end < 0) {
        return 0;
    }

    int content_length = 0;
    if (parse_content_length((const char *)buf, (int)len, hdr_end, &content_length) != 0) {
        return 0;
    }

    return ((int)len - hdr_end) >= content_length;
}

static int json_string_len(const char *json)
{
    int n = 0;
    while (json[n] != '\0') {
        n++;
    }
    return n;
}

static int build_request(const char *json, int json_len, char *buf, int cap, int *len_out)
{
    (void)json_len;
    char host[24];
    format_host_ip(host, (int)sizeof(host));
    if (AKOYA_CHAT_PORT != 80) {
        char port_suffix[8];
        int port = AKOYA_CHAT_PORT;
        int port_idx = 0;
        if (port == 0) {
            port_suffix[0] = '0';
            port_suffix[1] = '\0';
        } else {
            char tmp[8];
            int tmp_idx = 0;
            while (port > 0 && tmp_idx < (int)sizeof(tmp) - 1) {
                tmp[tmp_idx++] = (char)('0' + (port % 10));
                port /= 10;
            }
            port_suffix[0] = ':';
            port_idx = 1;
            while (tmp_idx > 0 && port_idx < (int)sizeof(port_suffix) - 1) {
                port_suffix[port_idx++] = tmp[--tmp_idx];
            }
            port_suffix[port_idx] = '\0';
        }
        int host_len = 0;
        while (host[host_len] != '\0') {
            host_len++;
        }
        for (int p = 0; port_suffix[p] != '\0' && host_len + 1 < (int)sizeof(host); p++) {
            host[host_len++] = port_suffix[p];
        }
        host[host_len] = '\0';
    }

    int body_len = json_string_len(json);
    if (body_len <= 0) {
        return -1;
    }

    char len_str[8];
    int len_val = body_len;
    int len_idx = 0;
    if (len_val == 0) {
        len_str[0] = '0';
        len_str[1] = '\0';
    } else {
        char tmp[8];
        int tmp_idx = 0;
        while (len_val > 0 && tmp_idx < (int)sizeof(tmp) - 1) {
            tmp[tmp_idx++] = (char)('0' + (len_val % 10));
            len_val /= 10;
        }
        while (tmp_idx > 0) {
            len_str[len_idx++] = tmp[--tmp_idx];
        }
        len_str[len_idx] = '\0';
    }

    int pos = 0;
    const char *req_prefix =
        "POST " AKOYA_CHAT_PATH " HTTP/1.1\r\n"
        "Host: ";
    const char *req_mid =
        "\r\n"
        "Content-Type: application/json\r\n"
        "Connection: close\r\n"
        "Content-Length: ";
    const char *req_suffix = "\r\n\r\n";

    const char *sections[] = { req_prefix, host, req_mid, len_str, req_suffix, json, 0 };
    for (int s = 0; sections[s] != 0; s++) {
        for (const char *p = sections[s]; *p != '\0'; p++) {
            if (pos + 1 >= cap) {
                return -1;
            }
            buf[pos++] = *p;
        }
    }
    buf[pos] = '\0';
    *len_out = pos;
    return 0;
}

static const char *http_chat_fail_reason(http_chat_status_t status)
{
    if (status == HTTP_CHAT_FAIL_TIMEOUT) {
        return "timeout";
    }
    if (status == HTTP_CHAT_FAIL_HTTP) {
        return "http";
    }
    if (status == HTTP_CHAT_FAIL_PARSE) {
        return "parse";
    }
    if (status == HTTP_CHAT_FAIL_OVERFLOW) {
        return "overflow";
    }
    if (status == HTTP_CHAT_FAIL_TRANSPORT_LIFECYCLE) {
        return "transport-lifecycle";
    }
    return "connect";
}

const char *http_chat_status_reason(http_chat_status_t status)
{
    return http_chat_fail_reason(status);
}

static http_chat_status_t http_chat_await_inactive_transport(void)
{
    if (!tcp_chat_drain_until_inactive(TCP_CLOSE_DRAIN_MS)) {
        return HTTP_CHAT_FAIL_TRANSPORT_LIFECYCLE;
    }
    return HTTP_CHAT_OK;
}

static http_chat_status_t http_chat_turn_exchange(
    tcp_session_t *session,
    http_chat_history_t *history,
    char *reply,
    int reply_cap)
{
    static char json[CHAT_JSON_BUDGET];
    int json_len = build_messages_json(history, json, (int)sizeof(json));
    if (json_len < 0) {
        return HTTP_CHAT_FAIL_OVERFLOW;
    }

    static char request[512 + CHAT_JSON_BUDGET];
    int request_len = 0;
    if (build_request(json, json_len, request, (int)sizeof(request), &request_len) != 0) {
        return HTTP_CHAT_FAIL_PARSE;
    }

    static uint8_t response[HTTP_CHAT_RECV_CAP];
    uint16_t response_len = 0;

    tcp_status_t tcp_status = tcp_session_send(session, (const uint8_t *)request, (uint16_t)request_len);
    if (tcp_status != TCP_OK) {
        return HTTP_CHAT_FAIL_CONNECT;
    }

    tcp_status = tcp_session_recv_until(
        session,
        response,
        (uint16_t)sizeof(response),
        &response_len,
        http_response_complete,
        0,
        AKOYA_CHAT_TIMEOUT_MS);

    if (tcp_status == TCP_FAIL_TIMEOUT) {
        return HTTP_CHAT_FAIL_TIMEOUT;
    }
    if (tcp_status != TCP_OK) {
        return HTTP_CHAT_FAIL_CONNECT;
    }

    if (response_len == 0) {
        return HTTP_CHAT_FAIL_HTTP;
    }

    int http_status = 0;
    if (parse_http_status((const char *)response, (int)response_len, &http_status) != 0
        || http_status < 200 || http_status >= 300) {
        return HTTP_CHAT_FAIL_HTTP;
    }

    int body_start = body_offset((const char *)response, (int)response_len);
    if (body_start < 0 || body_start >= (int)response_len) {
        return HTTP_CHAT_FAIL_PARSE;
    }

    int body_len = (int)response_len - body_start;
    if (extract_json_content((const char *)response + body_start, body_len, reply, reply_cap) < 0) {
        return HTTP_CHAT_FAIL_PARSE;
    }

    return HTTP_CHAT_OK;
}

http_chat_status_t http_chat_run_turn(http_chat_history_t *history, char *reply, int reply_cap)
{
    http_chat_status_t inactive = http_chat_await_inactive_transport();
    if (inactive != HTTP_CHAT_OK) {
        return inactive;
    }

    ipv4_addr_t host;
    host.bytes[0] = AKOYA_CHAT_HOST_IP0;
    host.bytes[1] = AKOYA_CHAT_HOST_IP1;
    host.bytes[2] = AKOYA_CHAT_HOST_IP2;
    host.bytes[3] = AKOYA_CHAT_HOST_IP3;

    tcp_session_t session;
    session.open = 0;

    tcp_status_t open_status = tcp_session_open(
        &session,
        host,
        (uint16_t)AKOYA_CHAT_PORT,
        AKOYA_CHAT_TIMEOUT_MS);

    if (open_status != TCP_OK) {
        tcp_session_close(&session);
        inactive = http_chat_await_inactive_transport();
        if (inactive != HTTP_CHAT_OK) {
            return inactive;
        }
        if (open_status == TCP_FAIL_TIMEOUT) {
            return HTTP_CHAT_FAIL_TIMEOUT;
        }
        return HTTP_CHAT_FAIL_CONNECT;
    }

    http_chat_status_t status = http_chat_turn_exchange(&session, history, reply, reply_cap);

    tcp_session_close(&session);

    inactive = http_chat_await_inactive_transport();
    if (inactive != HTTP_CHAT_OK) {
        return inactive;
    }

    return status;
}

int http_chat_build_turn_request(const http_chat_history_t *history, char *buf, int cap, int *len_out)
{
    static char json[CHAT_JSON_BUDGET];
    int json_len = build_messages_json(history, json, (int)sizeof(json));
    if (json_len < 0) {
        return -1;
    }
    return build_request(json, json_len, buf, cap, len_out);
}

int http_chat_response_complete(const uint8_t *buf, uint16_t len)
{
    return http_response_complete(buf, len, 0);
}

http_chat_status_t http_chat_parse_turn_response(const uint8_t *response, uint16_t len, char *reply, int reply_cap)
{
    if (len == 0) {
        return HTTP_CHAT_FAIL_HTTP;
    }

    int http_status = 0;
    if (parse_http_status((const char *)response, (int)len, &http_status) != 0
        || http_status < 200 || http_status >= 300) {
        return HTTP_CHAT_FAIL_HTTP;
    }

    int body_start = body_offset((const char *)response, (int)len);
    if (body_start < 0 || body_start >= (int)len) {
        return HTTP_CHAT_FAIL_PARSE;
    }

    int body_len = (int)len - body_start;
    if (extract_json_content((const char *)response + body_start, body_len, reply, reply_cap) < 0) {
        return HTTP_CHAT_FAIL_PARSE;
    }

    return HTTP_CHAT_OK;
}

void http_chat_session(void)
{
    http_chat_history_t history;
    http_chat_history_init(&history);

    ps2_keyboard_init();

    char line[PS2_LINE_MAX];
    char reply[512];

    for (;;) {
        console_write_prompt("> ");

        int len = ps2_read_line(line, (int)sizeof(line));
        if (len <= 0) {
            continue;
        }

        if (is_exit_command(line)) {
            console_write_line("chat_session=exit");
            break;
        }

        if (chat_history_would_overflow(&history, line)) {
            emit_fail("overflow");
            continue;
        }

        if (http_chat_history_add_user(&history, line) != 0) {
            emit_fail("overflow");
            continue;
        }

        http_chat_status_t status = http_chat_run_turn(&history, reply, (int)sizeof(reply));
        if (status == HTTP_CHAT_OK) {
            http_chat_history_set_assistant(&history, reply);
            emit_ok(reply);
        } else {
            emit_fail(http_chat_fail_reason(status));
        }
    }
}
