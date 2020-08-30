// log.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>

void log_printf(int64_t tick, const char *fmt, ...) {
	va_list args;

	printf("%"PRId64": ", tick);

	va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

	printf("\n");
}
