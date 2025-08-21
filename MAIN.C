/*
 * PCIEDIT
 *
 * (C) 2024 Eric Voirin (Oerg866)
 *
 * LICENSE: CC-BY-NC 3.0
 *
 * Tool to get/set PCI configuration registers in PCI devices.
 *
 * Refer to README.MD
 */

#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <stdint.h>
#include <string.h>

#include "TYPES.H"
#include "PCI.H"

#define VERSION "0.4"
/* Display the next <count> characters in <color).
   Small hack so we don't have to link with graph.lib */
static void text_color(u8 color, u16 count) {
    fflush(stdout);
    __asm {
        mov ah, 0x09
        mov al, 0x20
        mov bl, color
        mov bh, 0x00
        mov cx, count
        int 0x10
    }
}

/* Prints the PCI config registers of a given device */
static void print_regs(const u8 *before, const u8 *after)
{
    u32 offset;
    
    printf("\n");
    printf("  | 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F \n");
    printf("--|------------------------------------------------\n");

    for (offset = 0; offset < 256; ++offset) {
        if ((offset % 16) == 0) { /* after a new line, write header column */
            printf("%02x|", offset & 0xF0);
        }

        /* Odd even coloring, green if changed, white/gray if not. */

        if (before[offset] == after[offset]) {
            text_color(offset % 2 ? 7 : 15, 2);
        } else {
            text_color(offset % 2 ? 2 : 10, 2);
        }
        

        /* Read and print actual PCI config register content */
        printf("%02x ", after[offset]);

        if ((offset % 16) == 15) /* new line after 16 registers */
            printf("\r\n");


    }
    printf("\n");
}

static int process_regs_txt(char *infile, PCIDEVICE device)
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
            pci_write_8(device, index, value);
        } else {
            /* invalid line, ignore */
            printf("Malformed line '%s'\n", line);
        }
    } 

    fclose(f);

    return 1;
}

static int dump(const char *dir) {
    PCIDEVICE *device = NULL;
    char outpath[256] = { 0, };
    u8 config[256];
    FILE *out_bin = NULL;
    int device_count = 0;

    printf("Dumping devices to '%s'...\n", dir);

    while (NULL != (device = pci_get_next_device(device))) {
        /* Get PCI config space to buffer */

        pci_read_bytes(*device, config, 0, 256);

        snprintf(outpath, sizeof(outpath), "%s\\%02x_%02x_%02x.BIN",
            dir, device->bus, device->slot, device->func);

        printf("Writing File: %s\n", outpath);

        out_bin = fopen(outpath, "wb");

        if (out_bin == NULL) {
            printf("ERROR opening file, cancelling\n");
            return -1;
        }

        fwrite(config, 256, 1, out_bin);

        fclose(out_bin);

        device_count++;
    }

    return device_count;
}

static void list (void) {
    PCIDEVICE *current = NULL;

    printf("[Bus:Slot:Func] [Ven:Dev] --- PCI Device List\n");
    printf("----------------------------------------\n");

    while (NULL != (current = pci_get_next_device(current))) {
        pci_debug_info(*current);
    }
}

int main(int argc, char *argv[]) {
    PCIDEVICE   device;
    u16         ven = 0;
    u16         dev = 0;
    int         ret = 0;
    u8          regs_before[256];
    u8          regs_after [256];

    printf("PCIEDIT Version %s\n", VERSION);
    printf("(C)2024 E. Voirin (oerg866)\n");
    printf("http://github.com/oerg866\n");
    printf("----------------------------------------\n");

    /* Note: No guarding against weird and invalid inputs are made...
    if you wanted to you could exploit the hell out of this program's
    poor parameter parsing. Whatever */

    /* Check if PCI bus is accessible on this machine */

    if (!pci_test()) {
        printf("ERROR enumerating PCI bus. Quitting...\n");
        return -1;
    }

    /* Check if we wanna list */

    if (argc == 2 && strcmp(argv[1], "-l") == 0) {
        list();
        return 0;
    }

    /* Usage when parameters are insufficient */

    if (argc < 3) {
        printf("PCIEDIT lets you get/set PCI device configuration registers\n");
        printf("Usage:\n");
        printf("         PCIEDIT.EXE <ven> <dev> [register list]\n");
        printf("         PCIEDIT.EXE -d <dumpdir>\n");
        printf("         PCIEDIT.EXE -l\n");
        printf("Example: PCIEDIT.EXE 10DE 2044 REGISTERS.TXT\n");
        printf("\n");
        printf("If register list is missing, the current configuration register contents\n");
        printf("of this device are printed)\n");
        printf("\n");
        printf("-d dumps the configuration registers of ALL PCI devices\n");
        printf("   into the <dumpdir> directory\n");
        printf("-l lists all PCI devices in the system.\n");
        return -1;
    }

    printf("\n\n");

    /* Check if we wanna dump */

    if (strcmp(argv[1], "-d") == 0) {
        return dump(argv[2]);
    }

    /* Check if requested vendor / device ID is present in te system */

    ven = (u16)strtoul(argv[1], NULL, 16);
    dev = (u16)strtoul(argv[2], NULL, 16); 

    if (!pci_find_dev_by_id(ven, dev, &device)) {
        printf("Device %04x:%04x not found.\n", ven, dev);
        return -1; 
    }

    printf("\n");

    /* Take snapshot of current config space */
    pci_read_bytes(device, regs_before, 0, 256);


    /* If parameter for register file is missing, quit here */

    if (argc == 3) {
        return 0;
    }

    /* Parse register file */

    if (process_regs_txt(argv[3], device)) {
        /* Success, print register contents *after* our modification */
        pci_read_bytes(device, regs_after, 0, 256);
        print_regs(regs_before, regs_after);
        return 0;
    } else {
        return -1;
    }

}
