// simulator.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Core simulation functions

#ifndef DROMAIUS_SIMULATOR_H
#define DROMAIUS_SIMULATOR_H

#include "types.h"
#include "signal_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct ChipEvent {
	int32_t				chip_id;
	int64_t				timestamp;
	struct ChipEvent *	next;
} ChipEvent;

typedef struct Simulator {
	struct SignalPool *		signal_pool;
	ChipEvent *				event_schedule;			// scheduled event

	// time keeping
	int64_t			current_tick;
	int64_t			tick_duration_ps;		// in pico-seconds

	// signal history
	struct SignalHistory *	signal_history;
} Simulator;

struct Chip;

// construction
Simulator *simulator_create(int64_t tick_duration_ps);
void simulator_destroy(Simulator *sim);

struct Chip *simulator_register_chip(Simulator *sim, struct Chip *chip, const char *name);
struct Chip *simulator_chip_by_name(Simulator *sim, const char *name);
void simulator_device_complete(Simulator *sim);
const char *simulator_chip_name(Simulator *sim, int32_t chip_id);

// simulation
void simulator_simulate_timestep(Simulator *sim);
uint64_t simulator_signal_writers(Simulator *sim, Signal signal);

// scheduler
void simulator_schedule_event(Simulator *sim, int32_t chip_id, int64_t timestamp);
int32_t simulator_pop_scheduled_event(Simulator *sim, int64_t timestamp);

// time keeping
static inline int64_t simulator_interval_to_tick_count(Simulator *sim, int64_t interval_ps) {
	return interval_ps / sim->tick_duration_ps;
}


#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_SIMULATOR_H
