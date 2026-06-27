#ifndef AKOYA_PCI_H
#define AKOYA_PCI_H

#include <stdint.h>

struct pci_device {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t irq;
    uint32_t bar[6];
};

int pci_find_device(uint16_t vendor_id, uint16_t device_id, struct pci_device *out);
void pci_enable_device(const struct pci_device *dev);

#endif
