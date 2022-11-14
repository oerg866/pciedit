#ifndef PCI_H
#define PCI_H

#include "types.h"

/* PCI functions for MS-DOS */

#define PCI_BUS_MAX     255     /* Maximum of 256 PCI buses per machine */
#define PCI_SLOT_MAX    31      /* Maximum of 32 PCI Slots per bus */

u32 pci_read_32(u32 bus, u32 slot, u32 func, u32 offset);
u16 pci_read_16(u32 bus, u32 slot, u32 func, u32 offset);
u8  pci_read_8 (u32 bus, u32 slot, u32 func, u32 offset);

void pci_write_32(u32 bus, u32 slot, u32 func, u32 offset, u32 value);
void pci_write_16(u32 bus, u32 slot, u32 func, u32 offset, u16 value);
void pci_write_8 (u32 bus, u32 slot, u32 func, u32 offset, u8 value);


u16 pci_get_vendor(u8 bus, u8 slot, u8 func);
u16 pci_get_device(u8 bus, u8 slot, u8 func);

int pci_enum_dev(u8 bus, u8 slot);
int pci_find_dev_by_id(u16 ven, u16 dev, u8 *bus, u8 *slot);

int pci_test();

#endif
