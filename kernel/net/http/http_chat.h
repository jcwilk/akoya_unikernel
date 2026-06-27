#ifndef AKOYA_NET_HTTP_CHAT_H
#define AKOYA_NET_HTTP_CHAT_H

#include <stdint.h>

#include "net/nettypes.h"

typedef enum {
    HTTP_CHAT_OK = 0,
    HTTP_CHAT_FAIL_CONNECT,
    HTTP_CHAT_FAIL_TIMEOUT,
    HTTP_CHAT_FAIL_HTTP,
    HTTP_CHAT_FAIL_PARSE
} http_chat_status_t;

http_chat_status_t http_chat_probe(void);

#endif
