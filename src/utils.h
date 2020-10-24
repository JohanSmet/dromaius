// utils.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_UTILS_H
#define DROMAIUS_UTILS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t file_load_binary_fixed(const char *filename, uint8_t *buffer, size_t max_len);
size_t file_load_binary(const char* filename, int8_t** buffer);
bool file_save_binary(const char *filename, int8_t *data, size_t size);

void dir_list_files(const char *path, const char *ext, const char *prefix, const char ***file_list);

char *arr__printf(char *array, const char *fmt, ...);
#define arr_printf(a, ...) (a) = arr__printf((a), __VA_ARGS__)

bool string_to_hexint(const char* str, int64_t* val);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_UTILS_H
