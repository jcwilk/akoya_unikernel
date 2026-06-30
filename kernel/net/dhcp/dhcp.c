#include "net/dhcp/dhcp.h"

#include "net/eth/eth.h"
#include "net/link/link.h"
#include "net/ipv4/ipv4.h"
#include "time/time.h"

#include <stddef.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

struct udp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

struct dhcp_message {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    ipv4_addr_t ciaddr;
    ipv4_addr_t yiaddr;
    ipv4_addr_t siaddr;
    ipv4_addr_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint8_t options[312];
} __attribute__((packed));

static uint32_t transaction_id;
static ipv4_config_t offered_config;
static ipv4_addr_t dhcp_server_id;
static int offer_received;
static int ack_received;

static void write_u32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)((value >> 24) & 0xFFU);
    buffer[1] = (uint8_t)((value >> 16) & 0xFFU);
    buffer[2] = (uint8_t)((value >> 8) & 0xFFU);
    buffer[3] = (uint8_t)(value & 0xFFU);
}

static ipv4_addr_t read_ipv4(const uint8_t *bytes)
{
    ipv4_addr_t addr;
    addr.bytes[0] = bytes[0];
    addr.bytes[1] = bytes[1];
    addr.bytes[2] = bytes[2];
    addr.bytes[3] = bytes[3];
    return addr;
}

static void parse_dhcp_options(const uint8_t *options, int length, ipv4_config_t *out)
{
    int index = 0;
    while (index < length) {
        uint8_t code = options[index++];
        if (code == 255) {
            break;
        }
        if (code == 0) {
            continue;
        }
        if (index >= length) {
            break;
        }
        uint8_t opt_len = options[index++];
        if (index + opt_len > length) {
            break;
        }

        if (code == 1 && opt_len == 4) {
            out->subnet = read_ipv4(options + index);
        } else if (code == 3 && opt_len == 4) {
            out->gateway = read_ipv4(options + index);
        } else if (code == 54 && opt_len == 4) {
            dhcp_server_id = read_ipv4(options + index);
        }

        index += opt_len;
    }
}

static void dhcp_ipv4_handler(const uint8_t *packet, uint16_t length, void *ctx)
{
    (void)ctx;

    if (length < 28) {
        return;
    }

    if (packet[9] != 17) {
        return;
    }

    const struct udp_header *udp = (const struct udp_header *)(packet + 20);
    if (net_be16(udp->dest_port) != DHCP_CLIENT_PORT) {
        return;
    }

    const struct dhcp_message *msg = (const struct dhcp_message *)(packet + 20 + sizeof(struct udp_header));
    if (net_be32(msg->xid) != transaction_id) {
        return;
    }

    if (msg->options[0] != 99 || msg->options[1] != 130 || msg->options[2] != 83 || msg->options[3] != 99) {
        return;
    }

    uint8_t message_type = 0;
    int opt_index = 4;
    while (opt_index < 312) {
        uint8_t code = msg->options[opt_index++];
        if (code == 255) {
            break;
        }
        if (code == 0) {
            continue;
        }
        if (opt_index >= 312) {
            break;
        }
        uint8_t opt_len = msg->options[opt_index++];
        if (code == 53 && opt_len == 1) {
            message_type = msg->options[opt_index];
        }
        opt_index += opt_len;
    }

    if (message_type == 2) {
        offered_config.address = msg->yiaddr;
        offered_config.subnet.bytes[0] = 255;
        offered_config.subnet.bytes[1] = 255;
        offered_config.subnet.bytes[2] = 255;
        offered_config.subnet.bytes[3] = 0;
        offered_config.gateway = offered_config.address;
        dhcp_server_id = msg->siaddr;
        parse_dhcp_options(msg->options + 4, 308, &offered_config);
        if (ipv4_is_zero(dhcp_server_id)) {
            dhcp_server_id.bytes[0] = packet[12];
            dhcp_server_id.bytes[1] = packet[13];
            dhcp_server_id.bytes[2] = packet[14];
            dhcp_server_id.bytes[3] = packet[15];
        }
        if (ipv4_is_zero(offered_config.gateway) && !ipv4_is_zero(dhcp_server_id)) {
            offered_config.gateway = dhcp_server_id;
        }
        offer_received = 1;
    } else if (message_type == 5) {
        ack_received = 1;
    } else if (message_type == 6) {
        offer_received = 0;
        ack_received = 0;
    }
}

static int send_dhcp(uint8_t message_type, ipv4_addr_t requested)
{
    eth_device_t *nic = link_device();
    if (nic == 0) {
        return -1;
    }

    uint8_t frame[600];
    struct eth_header {
        eth_addr_t dest;
        eth_addr_t src;
        uint16_t ethertype;
    } __attribute__((packed));

    struct eth_header *eth = (struct eth_header *)frame;
    eth->dest = (eth_addr_t){{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
    eth->src = *eth_mac(nic);
    eth->ethertype = net_be16(0x0800);

    uint8_t *ip = frame + sizeof(struct eth_header);
    ip[0] = 0x45;
    ip[1] = 0;
    uint16_t udp_len = (uint16_t)(sizeof(struct udp_header) + sizeof(struct dhcp_message));
    uint16_t total = (uint16_t)(20U + udp_len);
    ip[2] = (uint8_t)(total >> 8);
    ip[3] = (uint8_t)(total & 0xFFU);
    ip[4] = 0;
    ip[5] = 0;
    ip[6] = 0;
    ip[7] = 0;
    ip[8] = 64;
    ip[9] = 17;
    ip[10] = 0;
    ip[11] = 0;
    ip[12] = 0;
    ip[13] = 0;
    ip[14] = 0;
    ip[15] = 0;
    ip[16] = 0xFF;
    ip[17] = 0xFF;
    ip[18] = 0xFF;
    ip[19] = 0xFF;
    uint16_t ip_csum = ipv4_checksum(ip, 20);
    ip[10] = (uint8_t)(ip_csum >> 8);
    ip[11] = (uint8_t)(ip_csum & 0xFFU);

    struct udp_header *udp = (struct udp_header *)(ip + 20);
    udp->src_port = net_be16(DHCP_CLIENT_PORT);
    udp->dest_port = net_be16(DHCP_SERVER_PORT);
    udp->length = net_be16(udp_len);
    udp->checksum = 0;

    struct dhcp_message *msg = (struct dhcp_message *)(ip + 20 + sizeof(struct udp_header));
    for (size_t i = 0; i < sizeof(struct dhcp_message); i++) {
        ((uint8_t *)msg)[i] = 0;
    }
    msg->op = 1;
    msg->htype = 1;
    msg->hlen = ETH_ADDR_LEN;
    msg->hops = 0;
    msg->xid = net_be32(transaction_id);
    msg->secs = 0;
    msg->flags = net_be16(0x8000U);
    msg->ciaddr.bytes[0] = 0;
    msg->ciaddr.bytes[1] = 0;
    msg->ciaddr.bytes[2] = 0;
    msg->ciaddr.bytes[3] = 0;
    msg->yiaddr = msg->ciaddr;
    msg->siaddr = msg->ciaddr;
    msg->giaddr = msg->ciaddr;

    for (int i = 0; i < 16; i++) {
        msg->chaddr[i] = 0;
    }
    const eth_addr_t *mac = eth_mac(nic);
    for (int i = 0; i < ETH_ADDR_LEN; i++) {
        msg->chaddr[i] = mac->bytes[i];
    }

    for (int i = 0; i < 64; i++) {
        msg->sname[i] = 0;
    }
    for (int i = 0; i < 128; i++) {
        msg->file[i] = 0;
    }

    msg->options[0] = 99;
    msg->options[1] = 130;
    msg->options[2] = 83;
    msg->options[3] = 99;
    msg->options[4] = 53;
    msg->options[5] = 1;
    msg->options[6] = message_type;

    int opt = 7;
    if (message_type == 3 && !ipv4_is_zero(requested)) {
        msg->options[opt++] = 50;
        msg->options[opt++] = 4;
        write_u32(msg->options + opt, ((uint32_t)requested.bytes[0] << 24)
            | ((uint32_t)requested.bytes[1] << 16)
            | ((uint32_t)requested.bytes[2] << 8)
            | (uint32_t)requested.bytes[3]);
        opt += 4;

        ipv4_addr_t server_id = dhcp_server_id;
        if (ipv4_is_zero(server_id) && !ipv4_is_zero(offered_config.gateway)) {
            server_id = offered_config.gateway;
        }
        if (!ipv4_is_zero(server_id)) {
            msg->options[opt++] = 54;
            msg->options[opt++] = 4;
            write_u32(msg->options + opt, ((uint32_t)server_id.bytes[0] << 24)
                | ((uint32_t)server_id.bytes[1] << 16)
                | ((uint32_t)server_id.bytes[2] << 8)
                | (uint32_t)server_id.bytes[3]);
            opt += 4;
        }
    }
    msg->options[opt++] = 255;

    return eth_send(nic, frame, (uint16_t)(sizeof(struct eth_header) + total));
}

dhcp_status_t dhcp_acquire(ipv4_config_t *config_out, uint32_t timeout_ms)
{
    offer_received = 0;
    ack_received = 0;
    dhcp_server_id.bytes[0] = 0;
    dhcp_server_id.bytes[1] = 0;
    dhcp_server_id.bytes[2] = 0;
    dhcp_server_id.bytes[3] = 0;
    transaction_id = (uint32_t)(time_millis() | 0x01000000U);

    link_register_ipv4_handler(dhcp_ipv4_handler, 0);

    uint32_t deadline = time_millis() + timeout_ms;
    while (time_millis() < deadline && !offer_received) {
        if (send_dhcp(1, config_out->address) != 0) {
            link_register_ipv4_handler(0, 0);
            return DHCP_FAIL_TIMEOUT;
        }

        uint32_t poll_until = time_millis() + 1000U;
        while (time_millis() < poll_until && !offer_received) {
            link_poll();
        }
    }

    if (!offer_received) {
        link_register_ipv4_handler(0, 0);
        return DHCP_FAIL_NO_OFFER;
    }

    ack_received = 0;
    deadline = time_millis() + timeout_ms;
    while (time_millis() < deadline && !ack_received) {
        if (send_dhcp(3, offered_config.address) != 0) {
            link_register_ipv4_handler(0, 0);
            return DHCP_FAIL_TIMEOUT;
        }

        uint32_t poll_until = time_millis() + 1000U;
        while (time_millis() < poll_until && !ack_received) {
            link_poll();
        }
    }

    link_register_ipv4_handler(0, 0);

    if (!ack_received) {
        return DHCP_FAIL_NO_ACK;
    }

    *config_out = offered_config;
    return DHCP_OK;
}

void dhcp_sm_begin(dhcp_sm_t *sm, uint32_t timeout_ms)
{
    offer_received = 0;
    ack_received = 0;
    dhcp_server_id.bytes[0] = 0;
    dhcp_server_id.bytes[1] = 0;
    dhcp_server_id.bytes[2] = 0;
    dhcp_server_id.bytes[3] = 0;
    transaction_id = (uint32_t)(time_millis() | 0x01000000U);

    sm->config.address.bytes[0] = 0;
    sm->config.address.bytes[1] = 0;
    sm->config.address.bytes[2] = 0;
    sm->config.address.bytes[3] = 0;
    sm->config.subnet = sm->config.address;
    sm->config.gateway = sm->config.address;

    sm->active = 1;
    sm->phase = 0;
    sm->deadline_ms = time_millis() + timeout_ms;
    sm->poll_until_ms = 0;
    sm->result = DHCP_OK;

    link_register_ipv4_handler(dhcp_ipv4_handler, 0);
}

int dhcp_sm_done(const dhcp_sm_t *sm)
{
    return !sm->active;
}

dhcp_status_t dhcp_sm_result(const dhcp_sm_t *sm)
{
    return sm->result;
}

const ipv4_config_t *dhcp_sm_config(const dhcp_sm_t *sm)
{
    return &sm->config;
}

int dhcp_sm_step(dhcp_sm_t *sm)
{
    if (!sm->active) {
        return 1;
    }

    uint32_t now = time_millis();
    if (now >= sm->deadline_ms) {
        sm->result = sm->phase < 2 ? DHCP_FAIL_NO_OFFER : DHCP_FAIL_NO_ACK;
        link_unregister_ipv4_handler(dhcp_ipv4_handler, 0);
        sm->active = 0;
        return 1;
    }

    if (sm->phase == 0) {
        if (now >= sm->poll_until_ms) {
            if (send_dhcp(1, sm->config.address) != 0) {
                sm->result = DHCP_FAIL_TIMEOUT;
                link_unregister_ipv4_handler(dhcp_ipv4_handler, 0);
                sm->active = 0;
                return 1;
            }
            sm->poll_until_ms = now + 1000U;
        }
        if (offer_received) {
            sm->phase = 1;
            ack_received = 0;
            sm->poll_until_ms = 0;
        }
        return 0;
    }

    if (sm->phase == 1) {
        if (now >= sm->poll_until_ms) {
            if (send_dhcp(3, offered_config.address) != 0) {
                sm->result = DHCP_FAIL_TIMEOUT;
                link_unregister_ipv4_handler(dhcp_ipv4_handler, 0);
                sm->active = 0;
                return 1;
            }
            sm->poll_until_ms = now + 1000U;
        }
        if (ack_received) {
            sm->config = offered_config;
            sm->result = DHCP_OK;
            link_unregister_ipv4_handler(dhcp_ipv4_handler, 0);
            sm->active = 0;
            return 1;
        }
        return 0;
    }

    sm->active = 0;
    return 1;
}
