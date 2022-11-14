/* PCI Functions for 16-Bit Realtime Compilers */

#include "PCI.H"

#include <stdio.h>
#include <dos.h>
#include <conio.h>

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


u32 pci_read_32(u32 bus, u32 slot, u32 func, u32 offset)
/* Reads DWORD from PCI config space. Assumes offset is DWORD-aligned. */
{
	u32 address = (bus << 16) | (slot << 11)
		| (func << 8) | (offset & 0xFC)
		| 0x80000000UL;
	outportl(0xCF8, address);
	return inportl(0xCFC);
}


u16 pci_read_16(u32 bus, u32 slot, u32 func, u32 offset) {
/* Reads WORD from PCI config space. Assumes offset is WORD-aligned. */

	return (offset & 2) ? (u16) (pci_read_32(bus, slot, func, offset) >> 16)
			: (u16) (pci_read_32(bus, slot, func, offset));
}

u8 pci_read_8(u32 bus, u32 slot, u32 func, u32 offset)
/* Reads BYTE from PCI config space. */
{
	switch (offset & 3) {
	case 3: return (u8) (pci_read_32(bus, slot, func, offset) >> 24);
	case 2: return (u8) (pci_read_32(bus, slot, func, offset) >> 16);
	case 1: return (u8) (pci_read_32(bus, slot, func, offset) >>  8);
	case 0: return (u8) (pci_read_32(bus, slot, func, offset) >>  0);
	default: return 0; /* to silence the compiler warning... */
	}
}

void pci_write_32(u32 bus, u32 slot, u32 func, u32 offset, u32 value)
/* Writes DWORD to PCI config space. Assumes offset is DWORD-aligned. */
{
	u32 address = (bus << 16) | (slot << 11)
		| (func << 8) | (offset & 0xFC)
		| 0x80000000UL;
	outportl(0xCF8, address);
	outportl(0xCFC, value);
}

void pci_write_16(u32 bus, u32 slot, u32 func, u32 offset, u16 value)
/* Writes WORD to PCI config space. Assumes offset is WORD-aligned. */
{
	u32 temp = pci_read_32(bus, slot, func, offset);
	temp = (offset & 2) ? ((u32) value << 16) | (temp & 0xFFFF)
			: ((u32) value) | (temp << 16);
	pci_write_32(bus, slot, func, offset, temp);
}

void pci_write_8(u32 bus, u32 slot, u32 func, u32 offset, u8 value)
/* Writes BYTE to PCI config space. */
{
	u32 temp = pci_read_32(bus, slot, func, offset);
	switch (offset & 3) {
	case 3: temp = (temp & 0x00FFFFFFUL) | ((u32) value << 24); break;
	case 2: temp = (temp & 0xFF00FFFFUL) | ((u32) value << 16); break;
	case 1: temp = (temp & 0xFFFF00FFUL) | ((u32) value <<  8); break;
	case 0: temp = (temp & 0xFFFFFF00UL) | ((u32) value <<  0); break;
	}
	pci_write_32(bus, slot, func, offset, temp);
}


u16 pci_get_vendor(u8 bus, u8 slot, u8 func)
/* Gets a PCI device's vendor ID for given bus, slot and function number. */
{
	return pci_read_16(bus, slot, func, 0);
}

u16 pci_get_device(u8 bus, u8 slot, u8 func)
/* Gets a PCI device's device ID for given bus, slot and function number. */
{
	if (pci_get_vendor(bus, slot, func) != 0xFFFF) {
		return pci_read_16(bus, slot, func, 2);
	} else {
		return 0xFFFF;
	}
}

int pci_enum_dev(u8 bus, u8 slot)
/* Enumerates a PCI device based on Bus & Slot numbers.
   Prints PCI Device Vendor and ID code on screen.
   Returns 1 if a device was found, 0 if not. */
{
	u16 ven = pci_get_vendor(bus, slot, 0);
	u16 dev;

	if (ven != 0xFFFF) {
		dev = pci_get_device (bus, slot, 0);
		printf("PCI Device %02x:%02x:%02x - VEN_%04x&DEV_%04x\n",
			   (int) bus,
			   (int) slot,
			   0,
			   ven,
			   dev);
		return 1;
	}

	return 0;
}

int pci_find_dev_by_id(u16 ven, u16 dev, u8 *bus, u8* slot)
/* Checks if a device with the given Vendor / device ID is present on the bus.
   Returns 1 if the device is found, 0 if not. 
   If it is, the device's bus and slot numbers are written to 'bus' and 'slot'*/
{
	u8 cur_bus;
	u8 cur_slot;
	u16 found_ven;
	u16 found_dev;

	for (cur_bus = 0; cur_bus < PCI_BUS_MAX; ++cur_bus) {
		for (cur_slot = 0; cur_slot <= PCI_SLOT_MAX; ++cur_slot) {

			found_ven = pci_get_vendor(cur_bus, cur_slot, 0);
			found_dev = pci_get_device(cur_bus, cur_slot, 0);

			if (found_ven == ven && found_dev == dev) {
				*bus = cur_bus;
				*slot = cur_slot;
				pci_enum_dev(cur_bus, cur_slot);
				return 1;
			}
		}
	}

	return 0;
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
