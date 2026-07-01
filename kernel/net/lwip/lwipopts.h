#ifndef LWIP_LWIPOPTS_H
#define LWIP_LWIPOPTS_H

#define NO_SYS                          1
#define SYS_LIGHTWEIGHT_PROT            0

#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
#define LWIP_ARP                        1
#define LWIP_ETHERNET                   1
#define LWIP_DHCP                       1
#define LWIP_DHCP_DOES_ACD_CHECK        0
#define LWIP_AUTOIP                     0
#define LWIP_ICMP                       1
#define LWIP_RAW                        1
#define LWIP_UDP                        1
#define LWIP_TCP                        1
#define LWIP_ALTCP                      1
#define LWIP_DNS                        0
#define LWIP_SOCKET                     0
#define LWIP_NETCONN                    0
#define LWIP_CALLBACK_API               1

#define LWIP_NETIF_HOSTNAME             0
#define LWIP_NETIF_STATUS_CALLBACK      0
#define LWIP_NETIF_LINK_CALLBACK        0

#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_TCP_PCB                2
#define MEMP_NUM_TCP_PCB_LISTEN         1
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_NETBUF                 2
#define MEMP_NUM_SYS_TIMEOUT            8
#define MEMP_NUM_FRAG_PBUF              4
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536

#define TCP_MSS                         1460
#define TCP_SND_BUF                     (4 * TCP_MSS)
#define TCP_WND                         (4 * TCP_MSS)
#define TCP_QUEUE_OOSEQ                 0
#define LWIP_TCP_TIMESTAMPS             0

#define LWIP_SINGLE_NETIF               1
#define LWIP_NETIF_TX_SINGLE_PBUF       1

#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                0
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0
#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_ICMP             0
#define LWIP_STATS                      0
#define LWIP_DEBUG                      0
#define LWIP_ERRNO_INCLUDE              "errno.h"

#include "time/time.h"

#define LWIP_RAND()                     ((u32_t)time_millis())

#include "lwip_hooks.h"

#endif
