// context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_CONTEXT_H
#define DROMAIUS_CONTEXT_H

#include "types.h"

// types
struct DmsContext;
struct DevMinimal6502;

// functions
struct DmsContext *dms_create_context(void);
void dms_release_context(struct DmsContext *dms);

void dms_set_device(struct DmsContext *dms, struct DevMinimal6502 *device);
struct DevMinimal6502 *dms_get_device(struct DmsContext *dms);

void dms_start_execution(struct DmsContext *dms);
void dms_stop_execution(struct DmsContext *dms);

void dms_single_step(struct DmsContext *dms);
void dms_single_instruction(struct DmsContext *dms);
void dms_run(struct DmsContext *dms);
void dms_pause(struct DmsContext *dms);
void dms_reset(struct DmsContext *dms);

void dms_monitor_cmd(struct DmsContext *dms, const char *cmd, char **reply);

#endif // DROMAIUS_CONTEXT_H
