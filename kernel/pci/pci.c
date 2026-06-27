#include "pci/pci.h"

#include "io/io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t pci_config_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t address = (uint32_t)(1U << 31)
        | ((uint32_t)bus << 16)
        | ((uint32_t)slot << 11)
        | ((uint32_t)func << 8)
        | (offset & 0xFCU);

    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

static void pci_config_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    uint32_t address = (uint32_t)(1U << 31)
        | ((uint32_t)bus << 16)
        | ((uint32_t)slot << 11)
        | ((uint32_t)func << 8)
        | (offset & 0xFCU);

    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

void pci_enable_device(const struct pci_device *dev)
{
    uint32_t command = pci_config_read(dev->bus, dev->slot, dev->func, 0x04);
    command |= 0x00000007U;
    pci_config_write(dev->bus, dev->slot, dev->func, 0x04, command);
}

int pci_find_device(uint16_t vendor_id, uint16_t device_id, struct pci_device *out)
{
    for (uint8_t bus = 0; bus < 8; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                uint32_t id = pci_config_read(bus, slot, func, 0x00);
                uint16_t found_vendor = (uint16_t)(id & 0xFFFFU);
                uint16_t found_device = (uint16_t)((id >> 16) & 0xFFFFU);

                if (found_vendor == 0xFFFFU) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                if (found_vendor != vendor_id || found_device != device_id) {
                    if (func == 0 && (pci_config_read(bus, slot, 0, 0x0C) & 0x00800000U) == 0) {
                        break;
                    }
                    continue;
                }

                out->bus = bus;
                out->slot = slot;
                out->func = func;
                out->vendor_id = found_vendor;
                out->device_id = found_device;

                uint32_t irq = pci_config_read(bus, slot, func, 0x3C);
                out->irq = (uint8_t)(irq & 0xFFU);

                for (int bar = 0; bar < 6; bar++) {
                    out->bar[bar] = pci_config_read(bus, slot, func, (uint8_t)(0x10 + bar * 4));
                }

                return 0;
            }
        }
    }

    return -1;
}
