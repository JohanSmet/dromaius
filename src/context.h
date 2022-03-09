// context.h - Johan Smet - BSD-3-Clause (see LICENSE)

#ifndef DROMAIUS_CONTEXT_H
#define DROMAIUS_CONTEXT_H

#include "types.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types

typedef enum DMS_STATE {
	DS_WAIT = 0,
	DS_SINGLE_STEP = 1,
	DS_STEP_SIGNAL = 2,
	DS_RUN = 3,
	DS_EXIT = 99
} DMS_STATE;

struct DmsContext;
struct Device;

// functions
struct DmsContext *dms_create_context(void);
void dms_release_context(struct DmsContext *dms);

void dms_set_device(struct DmsContext *dms, struct Device *device);
struct Device *dms_get_device(struct DmsContext *dms);

DMS_STATE dms_get_state(struct DmsContext *dms);

#ifndef DMS_NO_THREADING
void dms_start_execution(struct DmsContext *dms);
void dms_stop_execution(struct DmsContext *dms);
#else
void dms_execute(struct DmsContext *dms);
#endif // DMS_NO_THREADING

void dms_execute_no_sync(struct DmsContext *dms);

void dms_single_step(struct DmsContext *dms);
void dms_step_signal(struct DmsContext *dms, Signal signal, bool pos_edge, bool neg_edge);
void dms_run(struct DmsContext *dms);
void dms_pause(struct DmsContext *dms);

bool dms_is_paused(struct DmsContext *dms);

void dms_change_simulation_speed_ratio(struct DmsContext *dms, double ratio);
double dms_simulation_speed_ratio(struct DmsContext *dms);

SignalBreakpoint *dms_breakpoint_signal_list(struct DmsContext *dms);
void dms_breakpoint_signal_set(struct DmsContext *dms, Signal signal, bool pos_edge, bool neg_edge);
void dms_breakpoint_signal_clear(struct DmsContext *dms, Signal signal);
bool dms_breakpoint_signal_is_set(struct DmsContext *dms, Signal signal);
bool dms_toggle_signal_breakpoint(struct DmsContext *dms, Signal signal);

void dms_break_on_irq_set(struct DmsContext *dms);
void dms_break_on_irq_clear(struct DmsContext *dms);

void dms_monitor_cmd(struct DmsContext *dms, const char *cmd, char **reply);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_CONTEXT_H
