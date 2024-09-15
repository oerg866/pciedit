/* PCI Functions for 16-Bit Realtime Compilers */

#include "PCI.H"

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <malloc.h>
#include <assert.h>

#include "TYPES.H"


#if !defined(outportb) && defined(_outp)
    #define outportb _outp
#endif

#if !defined(outportl)

static u32 inportl(u16 port)
/* Emulates 32-bit I/O port reads using
   manual prefixed 32-bit instructions */
{
    u16 retl, reth;
    __asm {
        db 0x50                     /* push eax */
        push bx
        push dx
        mov dx, port
        db 0x66, 0xED               /* in  eax, dx */
        mov retl, ax
        db 0x66, 0xC1, 0xE8, 0x10   /* shr eax, 16 */
        mov reth, ax
        pop dx
        pop bx
        db 0x58                     /* pop eax */
    }
    return (u32) retl | ((u32) reth << 16);
}

static void outportl(u16 port, u32 value)
/* Emulates 32-Bit I/O port writes using
   manual prefixed 32-bit instructions. */
{
    u16 vall = (u16) value;
    u16 valh = (u16) (value >> 16);

    __asm {
        db 0x50                     /* push eax */
        db 0x53                     /* push ebx */
        push dx

        mov ax, valh
        db 0x66, 0xC1, 0xE0, 0x10   /* shl eax, 16 */
        mov dx, vall

        db 0x66, 0x0F, 0xB7, 0xDA   /* movzx ebx, dx */
        db 0x66, 0x09, 0xD8         /* or eax, ebx */

        mov dx, port
        db 0x66, 0xEF               /* out dx, eax */

        pop dx
        db 0x5B                     /* pop ebx */
        db 0x58                     /* pop eax */
    }
}
#endif

static inline int pci_is_device(PCIDEVICE device) {
    return (pci_get_vendor(device) == 0xFFFF) ? 0 : 1;
}

static inline int pci_is_multifunction_device(PCIDEVICE device) {
    return pci_is_device(device) ? pci_read_8(device, 0x0E) >> 7 : 0;
}

void pci_debug_info(PCIDEVICE device) {
    u8 header_type = pci_read_8(device, 0x0E);
    printf("[%02x:%02x:%02x] [%04x:%04x] Header Type [%02x] %s\n",
        device.bus, device.slot, device.func,
        pci_get_vendor(device),
        pci_get_device(device),
        header_type,
        header_type & 0x80 ? "(Multi-Function)" : "");
}

u32 pci_read_32(PCIDEVICE device, u32 offset)
/* Reads DWORD from PCI config space. Assumes offset is DWORD-aligned. */
{
    u32 address = ((u32)device.bus << 16) | ((u32)device.slot << 11)
        | ((u32)device.func << 8) | (offset & 0xFC)
        | 0x80000000UL;
    outportl(0xCF8, address);
    return inportl(0xCFC);
}


u16 pci_read_16(PCIDEVICE device, u32 offset)
/* Reads WORD from PCI config space. Assumes offset is WORD-aligned. */
{
    return (offset & 2) ? (u16) (pci_read_32(device, offset) >> 16)
            : (u16) (pci_read_32(device, offset));
}

u8 pci_read_8(PCIDEVICE device, u32 offset)
/* Reads BYTE from PCI config space. */
{
    switch (offset & 3) {
    case 3: return (u8) (pci_read_32(device, offset) >> 24);
    case 2: return (u8) (pci_read_32(device, offset) >> 16);
    case 1: return (u8) (pci_read_32(device, offset) >>  8);
    case 0: return (u8) (pci_read_32(device, offset) >>  0);
    default: return 0; /* to silence the compiler warning... */
    }
}

void pci_read_bytes(PCIDEVICE device, u8 *buffer, u32 offset, u32 count) {
    u32 i;
    for (i = 0; i < count; i++) {
        buffer[i] = pci_read_8(device, offset + i);
    }
}

void pci_write_32(PCIDEVICE device, u32 offset, u32 value)
/* Writes DWORD to PCI config space. Assumes offset is DWORD-aligned. */
{
    u32 address = ((u32)device.bus << 16) | ((u32)device.slot << 11)
        | ((u32)device.func << 8) | (offset & 0xFC)
        | 0x80000000UL;
    outportl(0xCF8, address);
    outportl(0xCFC, value);
}

void pci_write_16(PCIDEVICE device, u32 offset, u16 value)
/* Writes WORD to PCI config space. Assumes offset is WORD-aligned. */
{
    u32 temp = pci_read_32(device, offset);
    temp = (offset & 2) ? ((u32) value << 16) | (temp & 0xFFFF)
            : ((u32) value) | (temp << 16);
    pci_write_32(device, offset, temp);
}

void pci_write_8(PCIDEVICE device, u32 offset, u8 value)
/* Writes BYTE to PCI config space. */
{
    u32 temp = pci_read_32(device, offset);
    switch (offset & 3) {
    case 3: temp = (temp & 0x00FFFFFFUL) | ((u32) value << 24); break;
    case 2: temp = (temp & 0xFF00FFFFUL) | ((u32) value << 16); break;
    case 1: temp = (temp & 0xFFFF00FFUL) | ((u32) value <<  8); break;
    case 0: temp = (temp & 0xFFFFFF00UL) | ((u32) value <<  0); break;
    }
    pci_write_32(device, offset, temp);
}

u16 pci_get_vendor(PCIDEVICE device)
/* Gets a PCI device's vendor ID for given device struct (bus/slot/function). */
{
    return pci_read_16(device, 0);
}

u16 pci_get_device(PCIDEVICE device)
/* Gets a PCI device's device ID for given device struct (bus/slot/function). */
{
    if (pci_get_vendor(device) != 0xFFFF) {
        return pci_read_16(device, 2);
    } else {
        return 0xFFFF;
    }
}

int pci_find_dev_by_id(u16 ven, u16 dev, PCIDEVICE *device)
/* Checks if a device with the given Vendor / device ID is present on the bus.
   Returns 1 if the device is found, 0 if not.
   If it is, the device's bus, slot and function IDs are written to the struct
   pointed to by 'device' */
{
    PCIDEVICE *current = NULL;

    while (NULL != (current = pci_get_next_device(current))) {
        pci_debug_info(*current);

        if (pci_get_vendor(*current) == ven && pci_get_device(*current) == dev) {
            *device = *current;
            return 1;
        }
    }

    return 0;
}

PCIDEVICE *pci_get_next_device(PCIDEVICE *device) {
    /* first iteration = call with NULL pointer, we will allocate,
       else base our search on the given device */
    if (device == NULL) {
        device = calloc(1, sizeof(PCIDEVICE));
        assert(device != NULL);
    } else {
        goto restart; /* jump to the end of the for loop with existigng value */
    }

    for (device->bus = 0; device->bus < PCI_BUS_MAX; ++device->bus) {
        for (device->slot = 0; device->slot <= PCI_SLOT_MAX; ++device->slot) {
            for (device->func = 0; device->func <= PCI_FUNC_MAX; ++device->func) {
                if (pci_is_device(*device)) return device;

/* Lord, for-give me for I have sinned */
restart:
                if (!pci_is_multifunction_device(*device)) break;
            }
        }
    }

    /* Last device was handled, no device found, dealloc and return NULL */
    free(device);
    return NULL;
}

int pci_test()
/* Tests if the PCI config space can be accessed like we expect.
   Returns 1 if successful, 0 if not. */
{
    u32 test = 0;

    /* Concept stolen from linux kernel :P */

    outportb(0xCFB, 0x01);
    test = inportl(0xCF8);
    outportl(0xCF8, 0x80000000UL);

    test = inportl(0xCF8);

    if (test != 0x80000000UL) {
        printf("ERROR while testing PCI configuration space access!\n");
        printf("Expected 0x80000000, got 0x%08lx\n", test);
        return 0;
    }
    return 1;
}
