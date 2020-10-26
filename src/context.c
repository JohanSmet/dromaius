// context.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// main external interface for others to control a Dromaius instance
// define DMS_NO_THREADING to disable multithreading

#include "context.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys/threads.h"

#include "utils.h"
#include <stb/stb_ds.h>

#include "cpu.h"
#include "device.h"
#include "stopwatch.h"
#include "simulator.h"

#define SYNC_MIN_DIFF_PS		MS_TO_PS(20)	/* required skew betweem sim & real-time before sleep */

///////////////////////////////////////////////////////////////////////////////
//
// internal types
//

typedef enum DMS_STATE {
	DS_WAIT = 0,
	DS_SINGLE_STEP = 1,
	DS_STEP_SIGNAL = 2,
	DS_RUN = 3,
	DS_EXIT = 99
} DMS_STATE;

// TODO: all variables that can be modified by a user should be moved to the config struct
struct Config {
	DMS_STATE		state;

	int64_t			target_sim_real_ratio;		// (4 decimal places)
};

typedef struct DmsContext {
	Simulator *		simulator;
	Device *		device;

	struct Config	config_ui;					// configuration that is set/changed by the user
	struct Config	config;						// configuration that is in use by the context
	bool			config_changed;				// ui-config was changed, context should update config

	Signal			step_signal;
	bool			step_pos_edge;
	bool			step_neg_edge;

	int64_t *		breakpoints;
	Signal *		signal_breakpoints;
	bool			break_on_irq;

	Stopwatch *		stopwatch;
	int64_t			tick_start_run;
	int64_t			actual_sim_real_ratio;		// (4 decimal places)

	int64_t			sync_tick_interval;			// sync sim & real-time at this interval

#ifndef DMS_NO_THREADING
	thread_t		thread;
	mutex_t			mtx_wait;
	cond_t			cnd_wait;

	mutex_t			mtx_config;
#endif // DMS_NO_THREADING
} DmsContext;

///////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#ifndef DMS_NO_THREADING
	#define MUTEX_LOCK(dms)				mutex_lock(&(dms)->mtx_wait)
	#define MUTEX_UNLOCK(dms)			mutex_unlock(&(dms)->mtx_wait)
	#define MUTEX_CONFIG_LOCK(dms)		mutex_lock(&(dms)->mtx_config)
	#define MUTEX_CONFIG_UNLOCK(dms)	mutex_unlock(&(dms)->mtx_config)
#else
	#define MUTEX_LOCK(dms)
	#define MUTEX_UNLOCK(dms)
	#define MUTEX_CONFIG_LOCK(dms)
	#define MUTEX_CONFIG_UNLOCK(dms)
#endif

static inline int breakpoint_index(DmsContext *dms, int64_t addr) {
	for (int i = 0; i < arrlen(dms->breakpoints); ++i) {
		if (dms->breakpoints[i] == addr) {
			return i;
		}
	}

	return -1;
}

static inline int signal_breakpoint_index(DmsContext *dms, Signal signal) {
	for (int i = 0; i < arrlen(dms->signal_breakpoints); ++i) {
		if (memcmp(&dms->signal_breakpoints[i], &signal, sizeof(signal)) == 0) {
			return i;
		}
	}

	return -1;
}

static inline bool signal_breakpoint_triggered(DmsContext *dms) {
	for (int i = 0; i < arrlen(dms->signal_breakpoints); ++i) {
		if (signal_changed(dms->simulator->signal_pool, dms->signal_breakpoints[i])) {
			return true;
		}
	}

	return false;
}

static inline void sync_simulation_with_real_time(DmsContext *dms) {
	// sync simulation time with realtime
	if (dms->config.state != DS_RUN) {
		return;
	}

	int64_t real_ps = stopwatch_time_elapsed_ps(dms->stopwatch);
	int64_t sim_ps  = (dms->simulator->current_tick - dms->tick_start_run) * dms->simulator->tick_duration_ps;

	MUTEX_CONFIG_LOCK(dms);
	dms->actual_sim_real_ratio = (sim_ps * 10000) / real_ps;
	MUTEX_CONFIG_UNLOCK(dms);

	int64_t delta = ((sim_ps * 10000) / dms->config.target_sim_real_ratio) - real_ps;

	if (delta > SYNC_MIN_DIFF_PS) {
		stopwatch_sleep(delta);
	}
}

static inline void context_change_state(DmsContext *dms, DMS_STATE new_state) {
	MUTEX_CONFIG_LOCK(dms);
	dms->config.state = new_state;
	dms->config_ui.state = new_state;
	MUTEX_CONFIG_UNLOCK(dms);
}

static inline void context_update_config(DmsContext *dms) {
	bool reset_stopwatch = false;

	MUTEX_CONFIG_LOCK(dms);
	if (dms->config_changed) {
		reset_stopwatch = dms->config.target_sim_real_ratio != dms->config_ui.target_sim_real_ratio;
		memcpy(&dms->config, &dms->config_ui, sizeof(struct Config));
		dms->config_changed = false;
	}
	MUTEX_CONFIG_UNLOCK(dms);

	if (reset_stopwatch && dms->stopwatch->running) {
		stopwatch_stop(dms->stopwatch);
		stopwatch_start(dms->stopwatch);
		dms->tick_start_run = dms->simulator->current_tick;
	}
}

static bool context_execute(DmsContext *dms) {

	Cpu *cpu = dms->device->get_cpu(dms->device);
	int64_t tick_max = dms->simulator->current_tick + dms->sync_tick_interval;

	// check for changed config by UI
	context_update_config(dms);

	while (dms->config.state != DS_WAIT && dms->simulator->current_tick < tick_max) {

		// start stopwatch if not already activated
		if (dms->config.state == DS_RUN && !dms->stopwatch->running) {
			stopwatch_start(dms->stopwatch);
			dms->tick_start_run = dms->simulator->current_tick;
		}

		// save some stuff (XXX: is this still necessary -- signal_changed is available and cheap)
		bool prev_irq = cpu->irq_is_asserted(cpu);

		// process the device
		dms->device->process(dms->device);

		// post-process state changes
		bool cpu_sync = cpu->is_at_start_of_instruction(cpu);
		bool irq = cpu->irq_is_asserted(cpu);

		switch (dms->config.state) {
			case DS_SINGLE_STEP :
				context_change_state(dms, DS_WAIT);
				break;
			case DS_STEP_SIGNAL :
				if (signal_changed(dms->simulator->signal_pool, dms->step_signal)) {
					bool value = signal_read_bool(dms->simulator->signal_pool, dms->step_signal);
					if ((!value && dms->step_neg_edge) || (value && dms->step_pos_edge)) {
						context_change_state(dms, DS_WAIT);
					}
				}
				break;
			case DS_RUN :
				// check for breakpoints
				if ((!prev_irq && irq && dms->break_on_irq) ||
					(cpu_sync && breakpoint_index(dms, cpu->program_counter(cpu)) >= 0) ||
					(signal_breakpoint_triggered(dms))) {
					context_change_state(dms, DS_WAIT);
				}
				break;
			case DS_EXIT:
				return false;
			case DS_WAIT:
				break;
		}

		// check for changed config by UI
		context_update_config(dms);
	}

	if (dms->config.state == DS_WAIT) {
		stopwatch_stop(dms->stopwatch);
		dms->actual_sim_real_ratio = 0;
	}

	return true;
}

#ifndef DMS_NO_THREADING

static int context_background_thread(DmsContext *dms) {

	bool keep_running = true;

	while (keep_running) {

		mutex_lock(&dms->mtx_wait);

		// wait until execution is requested
		context_update_config(dms);
		while (dms->config.state == DS_WAIT) {
			cond_wait(&dms->cnd_wait, &dms->mtx_wait);
			context_update_config(dms);
		}

		keep_running = context_execute(dms);

		mutex_unlock(&dms->mtx_wait);

		sync_simulation_with_real_time(dms);
	}

	return 0;
}

#endif // DMS_NO_THREADING

static inline void change_state(DmsContext *context, DMS_STATE new_state) {

	MUTEX_CONFIG_LOCK(context);
	context->config_ui.state = new_state;
	context->config_changed = true;
	MUTEX_CONFIG_UNLOCK(context);
	#ifndef DMS_NO_THREADING
		cond_signal(&context->cnd_wait);
	#endif // DMS_NO_THREADING
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

DmsContext *dms_create_context() {
	DmsContext *ctx = (DmsContext *) malloc(sizeof(DmsContext));
	ctx->simulator = NULL;
	ctx->device = NULL;
	ctx->breakpoints = NULL;
	ctx->signal_breakpoints = NULL;
	ctx->break_on_irq = false;
	ctx->config.state = DS_WAIT;
	ctx->config.target_sim_real_ratio = 10000;
	memcpy(&ctx->config_ui, &ctx->config, sizeof(ctx->config));
	ctx->stopwatch = stopwatch_create();
	return ctx;
}

void dms_release_context(DmsContext *dms) {
	assert(dms);
	stopwatch_destroy(dms->stopwatch);
	free(dms);
}

void dms_set_device(DmsContext *dms, Device *device) {
	assert(dms);
	dms->device = device;
	dms->simulator = device->simulator;

	dms->sync_tick_interval = simulator_interval_to_tick_count(dms->simulator, US_TO_PS(500));
}

struct Device *dms_get_device(struct DmsContext *dms) {
	assert(dms);
	return dms->device;
}

#ifndef DMS_NO_THREADING

void dms_start_execution(DmsContext *dms) {
	assert(dms);
	assert(dms->device);

	// initialize context configuration
	dms->config.state = DS_WAIT;
	memcpy(&dms->config_ui, &dms->config, sizeof(dms->config));

	mutex_init_plain(&dms->mtx_wait);
	mutex_init_plain(&dms->mtx_config);
	cond_init(&dms->cnd_wait);
	thread_create_joinable(&dms->thread, (thread_func_t) context_background_thread, dms);
}

void dms_stop_execution(DmsContext *dms) {
	assert(dms);

	change_state(dms, DS_EXIT);

	int thread_res;
	thread_join(dms->thread, &thread_res);
}

#else

void dms_execute(DmsContext *dms) {
	context_execute(dms);
	sync_simulation_with_real_time(dms);
}

#endif // DMS_NO_THREADING

void dms_execute_no_sync(DmsContext *dms) {
	dms->config.state = DS_RUN;
	context_execute(dms);
}

void dms_single_step(DmsContext *dms) {
	assert(dms);

	if (dms->config_ui.state == DS_WAIT) {
		change_state(dms, DS_SINGLE_STEP);
	}
}

void dms_step_signal(struct DmsContext *dms, struct Signal signal, bool pos_edge, bool neg_edge) {
	assert(dms);

	if (dms->config_ui.state == DS_WAIT) {
		dms->step_signal = signal;
		dms->step_pos_edge = pos_edge;
		dms->step_neg_edge = neg_edge;
		change_state(dms, DS_STEP_SIGNAL);
	}
}


void dms_run(DmsContext *dms) {
	assert(dms);

	if (dms->config_ui.state == DS_WAIT) {
		change_state(dms, DS_RUN);
	}
}

void dms_pause(DmsContext *dms) {
	assert(dms);

	if (dms->config_ui.state == DS_RUN) {
		change_state(dms, DS_WAIT);
	}
}

void dms_change_simulation_speed_ratio(DmsContext *dms, double ratio) {
	assert(dms);

	MUTEX_CONFIG_LOCK(dms);

		dms->config_ui.target_sim_real_ratio = (int64_t) (ratio * 10000);
		dms->config_changed = true;

	MUTEX_CONFIG_UNLOCK(dms);

}

double dms_simulation_speed_ratio(DmsContext *dms) {
	assert(dms);

	MUTEX_CONFIG_LOCK(dms);
	int64_t ratio = dms->actual_sim_real_ratio;
	MUTEX_CONFIG_UNLOCK(dms);

	return (double) ratio / 10000.0;
}

bool dms_toggle_breakpoint(DmsContext *dms, int64_t addr) {
	int idx = breakpoint_index(dms, addr);

	if (idx < 0) {
		arrpush(dms->breakpoints, addr);
		return true;
	} else {
		arrdelswap(dms->breakpoints, idx);
		return false;
	}
}

Signal *dms_breakpoint_signal_list(DmsContext *dms) {
	assert(dms);
	return dms->signal_breakpoints;
}

void dms_breakpoint_signal_set(DmsContext *dms, Signal signal) {
	assert(dms);

	if (signal_breakpoint_index(dms, signal) < 0) {
		arrpush(dms->signal_breakpoints, signal);
	}
}

void dms_breakpoint_signal_clear(DmsContext *dms, Signal signal) {
	assert(dms);

	int idx = signal_breakpoint_index(dms, signal);
	if (idx	>= 0) {
		arrdelswap(dms->signal_breakpoints, idx);
	}
}

bool dms_toggle_signal_breakpoint(DmsContext *dms, Signal signal) {
	int idx = signal_breakpoint_index(dms, signal);

	if (idx < 0) {
		arrpush(dms->signal_breakpoints, signal);
		return true;
	} else {
		arrdelswap(dms->signal_breakpoints, idx);
		return false;
	}
}

void dms_monitor_cmd(struct DmsContext *dms, const char *cmd, char **reply) {
	assert(dms);
	assert(cmd);

	Cpu *cpu = dms->device->get_cpu(dms->device);

	if (cmd[0] == 'g') {		// "g"oto address
		int64_t addr;

		if (string_to_hexint(cmd + 1, &addr)) {

			// tell the cpu to override the location of the next instruction
			cpu->override_next_instruction_address(cpu, (uint16_t) (addr & 0xffff));

			// run computer until current instruction is finished
			bool cpu_sync  = cpu->is_at_start_of_instruction(cpu);

			while (!(cpu_sync && cpu->program_counter(cpu) == addr)) {
				dms->device->process(dms->device);
				cpu_sync  = cpu->is_at_start_of_instruction(cpu);
			}

			arr_printf(*reply, "OK: pc changed to 0x%lx", addr);
		} else {
			arr_printf(*reply, "NOK: invalid address specified");
		}
	} else if (cmd[0] == 'b' && cmd[1] == 'i') {		// toggle "b"reak on "i"rq
		dms->break_on_irq = !dms->break_on_irq;

		static const char *disp_enabled[] = {"disabled", "enabled"};
		arr_printf(*reply, "OK: break-on-irq %s", disp_enabled[dms->break_on_irq]);
	} else if (cmd[0] == 'b' && cmd[1] == 's') {		// toggle "b"reak on signal change
		Signal signal = signal_by_name(dms->simulator->signal_pool, cmd + 3);

		if (signal.count != 0) {
			static const char *disp_break[] = {"unset", "set"};
			bool set = dms_toggle_signal_breakpoint(dms, signal);
			arr_printf(*reply, "OK: signal-change breakpoint on %s %s", cmd + 3, disp_break[set]);
		} else {
			arr_printf(*reply, "NOK: signal '%s' is not known", cmd + 3);
		}
	} else if (cmd[0] == 'b') {		// toggle "b"reak-point
		int64_t addr;
		if (string_to_hexint(cmd + 1, &addr)) {
			static const char *disp_break[] = {"unset", "set"};
			bool set = dms_toggle_breakpoint(dms, addr);
			arr_printf(*reply, "OK: breakpoint at 0x%lx %s", addr, disp_break[set]);
		} else {
			arr_printf(*reply, "NOK: invalid address specified");
		}
	} else {
		arr_printf(*reply, "NOK: invalid command");
	}
}
