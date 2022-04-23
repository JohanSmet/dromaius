// utils.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "utils.h"
#include "crt.h"

#include <stb/stb_ds.h>
#include <cute/cute_files.h>

#include <stdarg.h>
#include <errno.h>

size_t file_load_binary_fixed(const char *filename, uint8_t *buffer, size_t max_len) {
	assert(filename);

	FILE* fp = NULL;
	if (dms_fopen(fp, filename, "rb")) {
		// FIXME: an error message would be nice
		return 0;
	}

	size_t read = dms_fread(buffer, 1, max_len, fp);
	dms_fclose(fp);

	return read;
}

size_t file_load_binary(const char* filename, int8_t** buffer) {
	enum { BLOCK_SIZE = 2048 };
	size_t total_size = 0;

	assert(filename);
	assert(buffer);

	FILE* fp = NULL;
	if (dms_fopen(fp, filename, "rb")) {
		// FIXME: an error message would be nice
		return total_size;
	}

	while (!dms_feof(fp)) {
		arrsetcap(*buffer, total_size + 2048);
		size_t read = dms_fread(*buffer + total_size, 1, BLOCK_SIZE, fp);
		total_size += read;
	}

	dms_fclose(fp);

	arrsetlen(*buffer, total_size);
	return total_size;
}

bool file_save_binary(const char *filename, int8_t *data, size_t size) {

	FILE* fp = NULL;
	if (dms_fopen(fp, filename, "wb")) {
		// FIXME: an error message would be nice
		return false;
	}

	size_t written = dms_fwrite(data, 1, size, fp);
	dms_fclose(fp);

	return written == size;
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
		if (prefix && dms_strncmp(file.name, prefix, dms_strlen(prefix)) != 0) {
			continue;
		}

		char** list = (char **) *file_list;
		arrpush(list, dms_strdup(file.name));
	}
}

char *arr__printf(char *array, const char *fmt, ...) {
	va_list args;
	size_t avail = arrcap(array) - arrlenu(array);

	va_start(args, fmt);
    int result = dms_vsnprintf(array + arrlenu(array), avail, fmt, args);
    va_end(args);

    assert(result >= 0);
	size_t n = (size_t) result + 1;

	if (n > avail) {
		// array too small, grow and print again
		arrsetcap(array, arrlenu(array) + n);
		avail = arrcap(array) - arrlenu(array);

		va_start(args, fmt);
		result = dms_vsnprintf(array + arrlenu(array), avail, fmt, args);
		va_end(args);

		assert(result >= 0);
		n = (size_t) result + 1;
    }

    // don't count zero-terminator in array length so it concatenates easily (terminator is always present!)
	arrsetlen(array, arrlenu(array) + n - 1);
    return array;
}

bool string_to_hexint(const char* str, int64_t* val) {
	errno = 0;
	*val = strtoll(str, NULL, 16);
	return errno == 0;
}

