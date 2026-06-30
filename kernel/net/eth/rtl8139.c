#include "net/eth/rtl8139.h"

#include "io/io.h"
#include "pci/pci.h"
#include "time/time.h"

#include <stddef.h>

#define RTL8139_VENDOR 0x10EC
#define RTL8139_DEVICE 0x8139

#define RTL8139_IDR0      0x00
#define RTL8139_MAR0      0x08
#define RTL8139_RBSTART   0x30
#define RTL8139_CMD       0x37
#define RTL8139_CAPR      0x38
#define RTL8139_CBR       0x3A
#define RTL8139_ISR       0x3E
#define RTL8139_IMR       0x3C
#define RTL8139_TSD0      0x10
#define RTL8139_TSAD0     0x20
#define RTL8139_RCR       0x44
#define RTL8139_TCR       0x40
#define RTL8139_CONFIG1   0x52
#define RTL8139_HLTCLK    0x5B
#define RTL8139_BMSR      0x64

#define RTL8139_CMD_RESET 0x10
#define RTL8139_CMD_RX_EN 0x08
#define RTL8139_CMD_TX_EN 0x04

#define RTL8139_ISR_RX_OK       0x0001U
#define RTL8139_ISR_RX_OVERFLOW 0x0008U

#define RTL8139_RX_IN_PROGRESS  0xFFF0U
#define RTL8139_RX_FRAME_MAX    1600U

#define RX_BUF_SIZE 8192
#define TX_BUF_SIZE 1792

static uint16_t io_base;
static uint8_t tx_cur;
static uint8_t rx_buffer[RX_BUF_SIZE + 16] __attribute__((aligned(4)));
static uint8_t tx_slots[4][TX_BUF_SIZE] __attribute__((aligned(4)));
static uint16_t rx_offset;

static inline void rtl8139_memory_barrier(void)
{
    __asm__ volatile ("" ::: "memory");
}

static uint16_t rtl8139_read16(uint16_t offset)
{
    return inw((uint16_t)(io_base + offset));
}

static void rtl8139_write8(uint16_t offset, uint8_t value)
{
    outb((uint16_t)(io_base + offset), value);
}

static void rtl8139_write16(uint16_t offset, uint16_t value)
{
    outw((uint16_t)(io_base + offset), value);
}

static void rtl8139_write32(uint16_t offset, uint32_t value)
{
    outl((uint16_t)(io_base + offset), value);
}

static void rtl8139_accept_all_multicast(void)
{
    /* MAR must be written as 32-bit accesses (OSDev / datasheet). */
    rtl8139_write32(RTL8139_MAR0, 0xFFFFFFFFU);
    rtl8139_write32(RTL8139_MAR0 + 4U, 0xFFFFFFFFU);
}

static int rtl8139_link_is_up(void)
{
    return (rtl8139_read16(RTL8139_BMSR) & 0x0004U) != 0;
}

static void rtl8139_wait_for_link(uint32_t timeout_ms)
{
    uint32_t deadline = time_millis() + timeout_ms;

    while (time_millis() < deadline) {
        if (rtl8139_link_is_up()) {
            return;
        }
        time_delay_ms(100U);
    }
}

static void rtl8139_program_tx_slots(void)
{
    for (int i = 0; i < 4; i++) {
        rtl8139_write32((uint16_t)(RTL8139_TSAD0 + (uint16_t)i * 4U),
                        (uint32_t)(uintptr_t)tx_slots[i]);
    }
}

static int rtl8139_reset(void)
{
    rtl8139_write8(RTL8139_CMD, RTL8139_CMD_RESET);
    for (int i = 0; i < 1000; i++) {
        if ((inb((uint16_t)(io_base + RTL8139_CMD)) & RTL8139_CMD_RESET) == 0) {
            return 0;
        }
        time_delay_ms(1);
    }
    return -1;
}

static void rtl8139_rx_soft_reset(void)
{
    uint8_t cmd = inb((uint16_t)(io_base + RTL8139_CMD));

    rtl8139_write8(RTL8139_CMD, (uint8_t)(cmd & (uint8_t)~RTL8139_CMD_RX_EN));
    rx_offset = 0;
    rtl8139_write16(RTL8139_CAPR, 0xFFF0U);
    rtl8139_write16(RTL8139_ISR, 0xFFFFU);
    rtl8139_write8(RTL8139_CMD, (uint8_t)(cmd | RTL8139_CMD_RX_EN));
    rtl8139_memory_barrier();
    rx_offset = 0;
}

static void rtl8139_service_isr(void)
{
    uint16_t isr = rtl8139_read16(RTL8139_ISR);
    if (isr == 0U) {
        return;
    }

    if ((isr & RTL8139_ISR_RX_OVERFLOW) != 0U) {
        rtl8139_rx_soft_reset();
        rtl8139_write16(RTL8139_ISR, RTL8139_ISR_RX_OK);
        return;
    }

    rtl8139_write16(RTL8139_ISR, isr);
}

static uint32_t rtl8139_read32(uint16_t offset)
{
    return inl((uint16_t)(io_base + offset));
}

static int rtl8139_tx_done(uint8_t entry)
{
    uint32_t tsd = rtl8139_read32((uint16_t)(RTL8139_TSD0 + (uint16_t)entry * 4U));
    if (tsd == 0U) {
        return 1;
    }
    return (tsd & 0x00006000U) != 0U;
}

void rtl8139_drain_tx(void)
{
    for (int entry = 0; entry < 4; entry++) {
        for (int i = 0; i < 50000; i++) {
            if (rtl8139_tx_done((uint8_t)entry)) {
                break;
            }
        }
    }
}

static int rtl8139_send(eth_device_t *dev, const uint8_t *frame, uint16_t length)
{
    (void)dev;

    if (length > TX_BUF_SIZE) {
        return -1;
    }

    uint8_t entry = tx_cur;
    for (int i = 0; i < 50000; i++) {
        if (rtl8139_tx_done(entry)) {
            break;
        }
        if (i == 49999) {
            return -1;
        }
    }

    uint16_t tx_len = length;
    if (tx_len < 60U) {
        tx_len = 60U;
    }

    for (uint16_t i = 0; i < tx_len; i++) {
        tx_slots[entry][i] = (i < length) ? frame[i] : 0;
    }

    uint16_t tsad_reg = (uint16_t)(RTL8139_TSAD0 + (uint16_t)entry * 4U);
    uint16_t tsd_reg = (uint16_t)(RTL8139_TSD0 + (uint16_t)entry * 4U);

    rtl8139_write32(tsad_reg, (uint32_t)(uintptr_t)tx_slots[entry]);
    rtl8139_memory_barrier();
    rtl8139_write32(tsd_reg, tx_len);
    tx_cur = (uint8_t)((entry + 1U) % 4U);
    return 0;
}

static int rtl8139_poll(eth_device_t *dev, uint8_t *frame, uint16_t capacity, uint16_t *length_out)
{
    (void)dev;

    rtl8139_service_isr();

    if ((inb((uint16_t)(io_base + RTL8139_CMD)) & 0x01U) != 0) {
        return 1;
    }

    if (rx_offset >= RX_BUF_SIZE) {
        rx_offset = (uint16_t)(rx_offset - RX_BUF_SIZE);
    }

    uint32_t status = *(uint32_t *)(rx_buffer + rx_offset);
    if ((status & 0x00000001U) == 0) {
        return 1;
    }

    uint16_t total_len = (uint16_t)((status >> 16) & 0x0FFFU);
    if (total_len == RTL8139_RX_IN_PROGRESS) {
        return 1;
    }

    if (total_len < 4U || total_len > RTL8139_RX_FRAME_MAX
        || total_len > (uint16_t)(capacity + 4U)) {
        return 1;
    }

    uint16_t copy_len = (uint16_t)(total_len - 4U);
    for (uint16_t i = 0; i < copy_len; i++) {
        frame[i] = rx_buffer[rx_offset + 4U + i];
    }

    rx_offset = (uint16_t)(rx_offset + total_len + 4U);
    rx_offset = (uint16_t)((rx_offset + 3U) & ~0x0003U);
    if (rx_offset >= RX_BUF_SIZE) {
        rx_offset = (uint16_t)(rx_offset - RX_BUF_SIZE);
    }
    rtl8139_write16(RTL8139_CAPR, (uint16_t)(rx_offset - 0x10U));

    *length_out = copy_len;
    return 0;
}

int rtl8139_init(eth_device_t *dev)
{
    struct pci_device pci;
    if (pci_find_device(RTL8139_VENDOR, RTL8139_DEVICE, &pci) != 0) {
        return -1;
    }

    uint32_t bar0 = pci.bar[0];
    if ((bar0 & 0x00000001U) == 0) {
        return -1;
    }

    io_base = (uint16_t)(bar0 & 0xFFFCU);
    rx_offset = 0;
    tx_cur = 0;

    pci_enable_device(&pci);

    /* Power on / wake (QEMU ignores; real RTL8139 needs this). */
    rtl8139_write8(RTL8139_CONFIG1, 0x00);
    rtl8139_write8(RTL8139_HLTCLK, (uint8_t)'R');

    if (rtl8139_reset() != 0) {
        return -1;
    }

    for (int i = 0; i < ETH_ADDR_LEN; i++) {
        dev->mac.bytes[i] = inb((uint16_t)(io_base + RTL8139_IDR0 + i));
    }

    rtl8139_write32(RTL8139_RBSTART, (uint32_t)(uintptr_t)rx_buffer);
    rtl8139_write16(RTL8139_CAPR, 0xFFF0U);
    rtl8139_accept_all_multicast();
    rtl8139_write8(RTL8139_CMD, RTL8139_CMD_RX_EN | RTL8139_CMD_TX_EN);
    rtl8139_write32(RTL8139_RCR, 0x0000000FU | 0x00000080U);
    rtl8139_write32(RTL8139_TCR, 0x03000700U);
    rtl8139_program_tx_slots();
    rtl8139_write16(RTL8139_IMR, 0x0005U);
    rtl8139_write16(RTL8139_ISR, 0xFFFFU);

    rtl8139_wait_for_link(5000U);

    dev->init = rtl8139_init;
    dev->send = rtl8139_send;
    dev->poll = rtl8139_poll;
    return 0;
}
