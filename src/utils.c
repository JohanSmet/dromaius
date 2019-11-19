// utils.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "utils.h"
#include "stb/stb_ds.h"

#include <assert.h>
#include <stdio.h>

size_t file_load_binary_fixed(const char *filename, uint8_t **buffer, size_t max_len) {
	assert(filename);
	
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		// FIXME: an error message would be nice
		return 0;
	}

	arrsetcap(*buffer, max_len);

	size_t read = fread(*buffer, max_len, 1, fp);
	arrsetlen(*buffer, read);

	fclose(fp);

	return read;
}

