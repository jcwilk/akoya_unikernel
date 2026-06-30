#ifndef AKOYA_NET_HTTP_CHAT_H
#define AKOYA_NET_HTTP_CHAT_H

#include <stdint.h>

#include "net/nettypes.h"

#define HTTP_CHAT_MSG_MAX_LEN 128
#define HTTP_CHAT_HISTORY_MAX_TURNS 16

typedef enum {
    HTTP_CHAT_OK = 0,
    HTTP_CHAT_FAIL_CONNECT,
    HTTP_CHAT_FAIL_TIMEOUT,
    HTTP_CHAT_FAIL_HTTP,
    HTTP_CHAT_FAIL_PARSE,
    HTTP_CHAT_FAIL_OVERFLOW,
    HTTP_CHAT_FAIL_TRANSPORT_LIFECYCLE
} http_chat_status_t;

typedef struct {
    char user[HTTP_CHAT_MSG_MAX_LEN];
    char assistant[HTTP_CHAT_MSG_MAX_LEN];
    int has_assistant;
} http_chat_turn_t;

typedef struct {
    http_chat_turn_t turns[HTTP_CHAT_HISTORY_MAX_TURNS];
    int count;
} http_chat_history_t;

void http_chat_history_init(http_chat_history_t *history);
int http_chat_history_add_user(http_chat_history_t *history, const char *msg);
void http_chat_history_set_assistant(http_chat_history_t *history, const char *reply);
int http_chat_history_would_overflow(const http_chat_history_t *history, const char *msg);
const char *http_chat_status_reason(http_chat_status_t status);
http_chat_status_t http_chat_run_turn(http_chat_history_t *history, char *reply, int reply_cap);
void http_chat_emit_ok(const char *reply);
void http_chat_emit_fail(const char *reason);

void http_chat_session(void);

#endif
