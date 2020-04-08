// context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_CONTEXT_H
#define DROMAIUS_CONTEXT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
struct DmsContext;
struct Device;

// functions
struct DmsContext *dms_create_context(void);
void dms_release_context(struct DmsContext *dms);

void dms_set_device(struct DmsContext *dms, struct Device *device);
struct Device *dms_get_device(struct DmsContext *dms);

#ifndef DMS_NO_THREADING
void dms_start_execution(struct DmsContext *dms);
void dms_stop_execution(struct DmsContext *dms);
#else
void dms_execute(struct DmsContext *dms);
#endif // DMS_NO_THREADING

void dms_single_step(struct DmsContext *dms);
void dms_single_instruction(struct DmsContext *dms);
void dms_run(struct DmsContext *dms);
void dms_pause(struct DmsContext *dms);
void dms_reset(struct DmsContext *dms);

void dms_monitor_cmd(struct DmsContext *dms, const char *cmd, char **reply);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CONTEXT_H
