// utils.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "utils.h"
#include "stb/stb_ds.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

size_t file_load_binary_fixed(const char *filename, uint8_t **buffer, size_t max_len) {
	assert(filename);
	
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		// FIXME: an error message would be nice
		return 0;
	}

	arrsetcap(*buffer, max_len);

	size_t read = fread(*buffer, 1, max_len, fp);
	arrsetlen(*buffer, read);

	fclose(fp);

	return read;
}

char *arr__printf(char *array, const char *fmt, ...) {
	va_list args;
	size_t avail = arrcap(array) - arrlen(array);

	va_start(args, fmt);
    int result = vsnprintf(array + arrlen(array), avail, fmt, args);
    va_end(args);

    assert(result >= 0);
	size_t n = (size_t) result + 1;

	if (n > avail) {
		// array too small, grow and print again
		arrsetcap(array, arrlen(array) + n);
		avail = arrcap(array) - arrlen(array);

		va_start(args, fmt);
		result = vsnprintf(array + arrlen(array), avail, fmt, args);
		va_end(args);

		assert(result >= 0);
		n = (size_t) result + 1;
    }

    // don't count zero-terminator in array length so it concatenates easily (terminator is always present!)
	arrsetlen(array, arrlen(array) + n - 1);
    return array;
}