#ifndef AKOYA_LWIP_HOOKS_H
#define AKOYA_LWIP_HOOKS_H

#include "lwip/prot/dhcp.h"

#define LWIP_HOOK_DHCP_APPEND_OPTIONS(netif, dhcp, state, msg, msg_type, options_len_ptr) \
    do { \
        (void)(netif); \
        (void)(dhcp); \
        (void)(state); \
        (void)(options_len_ptr); \
        if ((msg_type) == DHCP_DISCOVER || (msg_type) == DHCP_REQUEST) { \
            (msg)->flags = PP_HTONS(0x8000U); \
        } \
    } while (0)

#endif
