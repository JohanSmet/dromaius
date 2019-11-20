// utils.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_UTILS_H
#define DROMAIUS_UTILS_H

#include "types.h"

size_t file_load_binary_fixed(const char *filename, uint8_t **buffer, size_t max_len);

char *arr__printf(char *array, const char *fmt, ...);
#define arr_printf(a, ...) (a) = arr__printf((a), __VA_ARGS__)

#endif // DROMAIUS_UTILS_H
