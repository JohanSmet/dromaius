// utils.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "utils.h"

#include <assert.h>
#include <stdio.h>

size_t file_load_binary(const char *filename, uint8_t *buffer, size_t max_len) {
	assert(filename);
	assert(buffer);
	
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		// FIXME: an error message would be nice
		return 0;
	}

	size_t read = fread(buffer, max_len, 1, fp);
	fclose(fp);

	return read;
}

