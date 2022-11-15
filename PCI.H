#ifndef PCI_H
#define PCI_H

#include "types.h"

/* PCI functions for MS-DOS */

#define PCI_BUS_MAX     255     /* Maximum of 256 PCI buses per machine */
#define PCI_SLOT_MAX    31      /* Maximum of 32 PCI Slots per bus */
#define PCI_FUNC_MAX    7       /* Maximum of 8 Functions per PCI device */

#pragma pack(1)
typedef struct {
	u8 bus;
	u8 slot;
	u8 func;
	u8 dummy;
} PCIDEVICE;
#pragma pack()

u32 pci_read_32(PCIDEVICE device, u32 offset);
u16 pci_read_16(PCIDEVICE device, u32 offset);
u8  pci_read_8 (PCIDEVICE device, u32 offset);

void pci_write_32(PCIDEVICE device, u32 offset, u32 value);
void pci_write_16(PCIDEVICE device, u32 offset, u16 value);
void pci_write_8 (PCIDEVICE device, u32 offset, u8 value);


u16 pci_get_vendor(PCIDEVICE device);
u16 pci_get_device(PCIDEVICE device);

int pci_enum_dev(PCIDEVICE device);
int pci_find_dev_by_id(u16 ven, u16 dev, PCIDEVICE *device);

int pci_test();

#endif
