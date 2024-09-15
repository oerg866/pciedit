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

/*  Print debug info about a given device */
void pci_debug_info(PCIDEVICE device);

/*  Reads a 32-bit word from the given PCIDEVICE's configuration space */
u32 pci_read_32(PCIDEVICE device, u32 offset);
/*  Reads a 16-bit word from the given PCIDEVICE's configuration space */
u16 pci_read_16(PCIDEVICE device, u32 offset);
/*  Reads a 8-bit value from the given PCIDEVICE's configuration space */
u8  pci_read_8 (PCIDEVICE device, u32 offset);

/*  Reads <count> bytes at <offset> from <device>'s cfg space into <buffer> */
void pci_read_bytes(PCIDEVICE device, u8 *buffer, u32 offset, u32 count);

/*  Writes a 32-bit word to the given PCIDEVICE's configuration space */
void pci_write_32(PCIDEVICE device, u32 offset, u32 value);
/*  Writes a 16-bit word to the given PCIDEVICE's configuration space */
void pci_write_16(PCIDEVICE device, u32 offset, u16 value);
/*  Writes a 8-bit value to the given PCIDEVICE's configuration space */
void pci_write_8 (PCIDEVICE device, u32 offset, u8 value);

/*  Get the PCI Vendor ID for the given PCIDEVICE */
u16 pci_get_vendor(PCIDEVICE device);
/*  Get the PCI Device ID for the given PCIDEVICE */
u16 pci_get_device(PCIDEVICE device);

/*  Tries to find a PCI device with the given vendor/device ID.
    If successful, returns 1 and populates <device>, else 0. */
int pci_find_dev_by_id(u16 ven, u16 dev, PCIDEVICE *device);

/*  Get the next PCI device *after* <device>.
    Call with NULL pointer to get the first device.
    Returns NULL if no more devices are found.

    Can be used to iterate through all devices in the system. */
PCIDEVICE *pci_get_next_device(PCIDEVICE *device);

/*  Test if current machine's PCI bus can be read/written by this application.
    on the current system.
    Returns 1 if yes, 0 if not. */
int pci_test();

#endif
