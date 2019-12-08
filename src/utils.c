// utils.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "utils.h"
#include "stb/stb_ds.h"
#include <cute/cute_files.h>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

size_t file_load_binary_fixed(const char *filename, uint8_t *buffer, size_t max_len) {
	assert(filename);
	
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		// FIXME: an error message would be nice
		return 0;
	}

	size_t read = fread(buffer, 1, max_len, fp);
	fclose(fp);

	return read;
}

void dir_list_files(const char *path, const char *ext, const char *prefix, const char ***file_list) {

	cf_dir_t dir;

	for (cf_dir_open(&dir, path); dir.has_next != 0; cf_dir_next(&dir)) {
		cf_file_t file;
		cf_read_file(&dir, &file);

		// skip anything that isn't a regular file with the wanted extension
		if (file.is_reg == 0 || cf_match_ext(&file, ext) == 0) {
			continue;
		}

		// optionally check prefix
		if (prefix && strncmp(file.name, prefix, strlen(prefix)) != 0) {
			continue;
		}

		arrpush(*file_list, strdup(file.name));
	}
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

bool string_to_hexint(const char* str, int64_t* val) {
	errno = 0;
	*val = strtoll(str, NULL, 16);
	return errno == 0;
}

