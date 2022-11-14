/*
 * PCIEDIT
 *
 * (C) 2022 Eric "Oerg866" Voirin
 *
 * LICENSE: CC-BY-NC 3.0
 *
 * Tool to set PCI configuration registers in a given PCI device.
 *
 * Refer to README.MD
 */

/* 
	!!!!!!!!!!!!! TODO !!!!!!!!!!!!!!
	Allow this tool to work on ANY function, i.e. not just the first 
	function device of a pci slot.
*/

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <stdint.h>

#include "TYPES.H"
#include "PCI.H"

#define VERSION "0.1"

void print_regs(u8 bus, u8 slot) 
/* Prints the PCI config registers of a given bus/slot number */
{
	u32 offset;

	printf("   | x0 x1 x2 x3 x4 x5 x6 x7    x8 x9 xA xB xC xD xE xF\n");
	printf("---|---------------------------------------------------\n");

	for (offset = 0; offset <= 255; ++offset) {
		if ((offset % 16) == 0) /* after a new line, write header column */
			printf("%1xx | ", (int)offset >> 4);

		/* Read and print actual PCI config register content */
		printf("%02x ", (int)pci_read_8(bus, slot, 0, offset));

		if ((offset % 16) == 7) /* gap after 8 registers */
			printf("   ");

		if ((offset % 16) == 15) /* new line after 16 registers */
			printf("\n");
	}
	printf("\n");
}

int process_regs_txt(char *infile, u8 bus, u8 slot) 
/* Process register text file and set the contents accordingly */
{
	FILE *f = fopen(infile, "r");
	char *line = NULL;
	unsigned int bufsize = 0;
	int index = 0;
	int value = 0;


	if (ferror(f)) {
		printf("Could not open file %s\n", infile);
		return 0;
	}

	/* Process until the end of the file */

	while (getline(&line, &bufsize, f) >= 0) {
		/* expectedf ormat xx yy where xx is the register and yy is the value
		   in hexadeicmal format, eg. ab cd */

		sscanf(line, "%x %x", &index, &value);
		if (index >= 0 && index <= 255 && value >= 0 && value <= 255) {
			/* Valid line, we can write to PCI register now */
			pci_write_8(bus, slot, 0, index, value);
		} else {
			/* invalid line, ignore */
			printf("Malformed line '%s'\n", line);
		}
	} 

	fclose(f);

	return 1;
}

int main(int argc, char *argv[]) {
	int ven = 0;
	int dev = 0;
	u8 bus = 0;
	u8 slot = 0;

	printf("PCIEDIT Version %s\n", VERSION);
	printf("(C)2022 Eric \"oerg866\" Voirin\n");
	printf("Message me on Discord: EricV#9999\n");
	printf("----------------------------------------\n");


	/* Usage when parameters are insufficient */

	if (argc < 3) {
		printf("PCIEDIT lets you set configuration registers\n");
		printf("Usage: PCIEDIT.EXE <ven> <dev> [register list]\n");
		printf("Example: PCIEDIT.EXE 10DE 2044 REGISTERS.TXT\n");
		printf("\n");
		printf("If register list is missing, the current configuration register contents\n");
		printf("of this device are printed)\n");
		return -1;
	}

	/* Note: No guarding against weird and invalid inputs are made...
	if you wanted to you could exploit the hell out of this program's
	poor parameter parsing. Whatever */

	printf("\n\n");

	/* Check if PCI bus is accessible on this machine */

	if (!pci_test()) {
		printf("ERROR enumerating PCI bus. Quitting...\n");
		return -1;
	}

	/* Check if requested vendor / device ID is present in te system */

	ven = (u16)strtoul(argv[1], NULL, 16);
	dev = (u16)strtoul(argv[2], NULL, 16); 

	if (!pci_find_dev_by_id(ven, dev, &bus, &slot)) {
		printf("Device %04x:%04x not found.\n", (int)ven, (int)dev);
		return -1; 
	}

	printf("\n");

	/* Print regs *before* changing anything */

	print_regs(bus, slot);

	/* If parameter for register file is missing, quit here */

	if (argc == 3) {
		return 0;
	}

	/* Parse register file */

	if (process_regs_txt(argv[3], bus, slot)) {
		/* Success, print register contents *after* our modification */
		printf("Registers AFTER writing:\n\n");
		print_regs(bus, slot);
		return 0;
	} else {
		return -1;
	}

}

