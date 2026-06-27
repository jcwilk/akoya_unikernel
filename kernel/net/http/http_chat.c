#include "net/http/http_chat.h"

#include "console/console.h"
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
#define AKOYA_CHAT_HOST_IP3 110
#endif

#ifndef AKOYA_CHAT_PATH
#define AKOYA_CHAT_PATH "/v1/chat/completions"
#endif

#ifndef AKOYA_CHAT_USER_MSG
#define AKOYA_CHAT_USER_MSG "hi"
#endif

#ifndef AKOYA_CHAT_MODEL
#define AKOYA_CHAT_MODEL "default"
#endif

#ifndef AKOYA_CHAT_TIMEOUT_MS
#define AKOYA_CHAT_TIMEOUT_MS 60000U
#endif

#define HTTP_CHAT_RECV_CAP 4096U
#define HTTP_CHAT_REPLY_CONSOLE_MAX 256U

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
    console_write("chat_completion=fail reason=");
    console_write_line(reason);
}

static void emit_ok(const char *reply)
{
    console_write("chat_completion=ok reply=");
    console_write_reply_truncated(reply);
    console_write_line("");
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

static int build_request(char *buf, int cap, int *len_out)
{
    char json[256];
    int json_len = 0;
    const char *prefix = "{\"model\":\"";
    const char *mid = "\",\"messages\":[{\"role\":\"user\",\"content\":\"";
    const char *suffix = "\"}]}";

    const char *parts[] = { prefix, AKOYA_CHAT_MODEL, mid, AKOYA_CHAT_USER_MSG, suffix, 0 };
    for (int p = 0; parts[p] != 0; p++) {
        for (const char *s = parts[p]; *s != '\0'; s++) {
            if (json_len + 1 >= (int)sizeof(json)) {
                return -1;
            }
            json[json_len++] = *s;
        }
    }
    json[json_len] = '\0';

    char host[16];
    format_host_ip(host, (int)sizeof(host));

    char len_str[8];
    int len_val = json_len;
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
        "POST " AKOYA_CHAT_PATH " HTTP/1.0\r\n"
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

static const char *tcp_fail_reason(tcp_status_t status)
{
    if (status == TCP_FAIL_CONNECT) {
        return "connect";
    }
    if (status == TCP_FAIL_TIMEOUT) {
        return "timeout";
    }
    return "connect";
}

http_chat_status_t http_chat_probe(void)
{
    char request[512];
    int request_len = 0;
    if (build_request(request, (int)sizeof(request), &request_len) != 0) {
        emit_fail("parse");
        return HTTP_CHAT_FAIL_PARSE;
    }

    ipv4_addr_t host;
    host.bytes[0] = AKOYA_CHAT_HOST_IP0;
    host.bytes[1] = AKOYA_CHAT_HOST_IP1;
    host.bytes[2] = AKOYA_CHAT_HOST_IP2;
    host.bytes[3] = AKOYA_CHAT_HOST_IP3;

    static uint8_t response[HTTP_CHAT_RECV_CAP];
    uint16_t response_len = 0;

    tcp_status_t tcp_status = tcp_connect_send_recv(
        host,
        80,
        (const uint8_t *)request,
        (uint16_t)request_len,
        response,
        (uint16_t)sizeof(response),
        &response_len,
        AKOYA_CHAT_TIMEOUT_MS);

    if (tcp_status != TCP_OK) {
        emit_fail(tcp_fail_reason(tcp_status));
        if (tcp_status == TCP_FAIL_TIMEOUT) {
            return HTTP_CHAT_FAIL_TIMEOUT;
        }
        return HTTP_CHAT_FAIL_CONNECT;
    }

    if (response_len == 0) {
        emit_fail("http");
        return HTTP_CHAT_FAIL_HTTP;
    }

    int http_status = 0;
    if (parse_http_status((const char *)response, (int)response_len, &http_status) != 0
        || http_status < 200 || http_status >= 300) {
        emit_fail("http");
        return HTTP_CHAT_FAIL_HTTP;
    }

    int body_start = body_offset((const char *)response, (int)response_len);
    if (body_start < 0 || body_start >= (int)response_len) {
        emit_fail("parse");
        return HTTP_CHAT_FAIL_PARSE;
    }

    char reply[512];
    int body_len = (int)response_len - body_start;
    if (extract_json_content((const char *)response + body_start, body_len, reply, (int)sizeof(reply)) < 0) {
        emit_fail("parse");
        return HTTP_CHAT_FAIL_PARSE;
    }

    emit_ok(reply);
    return HTTP_CHAT_OK;
}
