// log.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_LOG_H
#define DROMAIUS_LOG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void log_printf(int64_t tick, const char *fmt, ...);

#ifdef DMS_LOG_TRACE
	#define LOG_TRACE(f, ...)	log_printf(SIGNAL_POOL->current_tick, (f), __VA_ARGS__)
#else
	#define LOG_TRACE(f, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_LOG_H
