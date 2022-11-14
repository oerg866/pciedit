#include <stdio.h>
#include <stdint.h>
#include <malloc.h>

/* Converts RG files from WPCREDIT to TXT files for PCIEDIT */

int main(int argc, char*argv[]) {

	FILE *f = NULL;
	FILE *out = NULL;
	size_t size;
	size_t i;
	char *data = NULL;

	if (argc != 3) {
		printf("RG2TXT (c) 2022 Eric \"oerg866\" Voirin\n");
		printf("Converts WPCREDIT RG files to register files for PCIEDIT\n");
		printf("Usage: RG2TXT.EXE <rg file> <output file>\n");
		return -1;
	}

	f = fopen(argv[1], "rb");
	out = fopen(argv[2], "w");

	if (ferror(f) || ferror(out)) {
		printf("Cannot open file\n");
		return -1;
	}

	fseek(f, 0, SEEK_END);

	size = ftell(f);

	rewind(f);

	if (size > 0x100) {
		printf("File too big!\n");
		return -1;
	}

	data = malloc(size);

	if (fread(data, 1, size, f) != size || ferror(f)) {
		printf("File read error\n");
		return -1;
	}

	for (i = 0; i < size; ++i) {
		fprintf(out, "%02x %02x\n", (int)i, (int)data[i]); 
	}

	free(data);

	fclose(out);
	fclose(f);
	return 0;
}
