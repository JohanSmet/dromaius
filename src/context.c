// context.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// main external interface for others to control a Dromaius instance
// define DMS_NO_THREADING to disable multithreading

#include "context.h"

#include "sys/threads.h"
#include "crt.h"
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

struct Config {
	DMS_STATE		state;

	int64_t			target_sim_real_ratio;		// (4 decimal places)

	SignalBreakpoint	step_signal;			// signal to use for the step-signal feature (normally a clock signal)
	SignalBreakpoint *	signal_breakpoints;		// dynamic array of signal breakpoints
	int64_t *			pc_breakpoints;			// dynamic array of program counter breakpoints
	bool				bp_updated;
};

typedef struct DmsContext {
	Simulator *		simulator;					// non-owning pointer to the simulator executing the device
	Device *		device;						// non-owning pointer to the device being simulator

	struct Config	config_usr;					// configuration that is set/changed by the user
	struct Config	config;						// configuration that is in use by the context
	bool			config_changed;				// ui-config was changed, context should update config

	// context-side variables
	Stopwatch *		stopwatch;
	int64_t			tick_start_run;
	int64_t			sync_tick_interval;			// sync sim & real-time at this interval

#ifndef DMS_NO_THREADING
	thread_t		thread;
	mutex_t			mtx_wait;
	cond_t			cnd_wait;

	mutex_t			mtx_config;
#endif // DMS_NO_THREADING

	// user-side variables
	bool			usr_break_irq;

	// feedback variables (context -> user): protect with config-mutex
	int64_t			actual_sim_real_ratio;		// (4 decimal places)

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
	for (int i = 0; i < arrlen(dms->config.pc_breakpoints); ++i) {
		if (dms->config.pc_breakpoints[i] == addr) {
			return i;
		}
	}

	return -1;
}

static inline int signal_breakpoint_index(DmsContext *dms, Signal signal) {
	for (int i = 0; i < arrlen(dms->config_usr.signal_breakpoints); ++i) {
		if (dms_memcmp(&dms->config_usr.signal_breakpoints[i].signal, &signal, sizeof(signal)) == 0) {
			return i;
		}
	}

	return -1;
}

static inline bool signal_breakpoint_check(DmsContext *dms, SignalBreakpoint *bp) {

	if (!signal_changed(dms->simulator->signal_pool, bp->signal)) {
		return false;
	}

	bool value = signal_read(dms->simulator->signal_pool, bp->signal);
	return ((!value && bp->neg_edge) || (value && bp->pos_edge));
}

static inline bool signal_breakpoint_triggered(DmsContext *dms) {
	for (int i = 0; i < arrlen(dms->config.signal_breakpoints); ++i) {
		if (signal_breakpoint_check(dms, &dms->config.signal_breakpoints[i])) {
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
	dms->config_usr.state = new_state;
	MUTEX_CONFIG_UNLOCK(dms);
}

static inline void context_update_config(DmsContext *dms) {
	bool reset_stopwatch = false;

	MUTEX_CONFIG_LOCK(dms);
	if (dms->config_changed) {
		reset_stopwatch = dms->config.target_sim_real_ratio != dms->config_usr.target_sim_real_ratio;

		dms->config.state = dms->config_usr.state;
		dms->config.target_sim_real_ratio = dms->config_usr.target_sim_real_ratio;
		dms->config.step_signal = dms->config_usr.step_signal;

		if (dms->config_usr.bp_updated) {
			size_t len = arrlenu(dms->config_usr.signal_breakpoints);
			arrsetlen(dms->config.signal_breakpoints, len);
			dms_memcpy(dms->config.signal_breakpoints, dms->config_usr.signal_breakpoints, sizeof(SignalBreakpoint) * len);

			len = arrlenu(dms->config_usr.pc_breakpoints);
			arrsetlen(dms->config.pc_breakpoints, len);
			dms_memcpy(dms->config.pc_breakpoints, dms->config_usr.pc_breakpoints, sizeof(int64_t) * len);

			dms->config_usr.bp_updated = false;
		}

		dms->config_changed = false;
	}
	MUTEX_CONFIG_UNLOCK(dms);

	if (reset_stopwatch && dms->stopwatch->running) {
		stopwatch_stop(dms->stopwatch);
		stopwatch_start(dms->stopwatch);
		dms->tick_start_run = dms->simulator->current_tick;
	}
}

static inline bool context_check_breakpoints(DmsContext *dms, Cpu *cpu) {

	if (signal_breakpoint_triggered(dms)) {
		return true;
	}

	if (breakpoint_index(dms, cpu->program_counter(cpu)) >= 0 &&
		cpu->is_at_start_of_instruction(cpu)) {
		return true;
	}

	return false;
}

static bool context_execute(DmsContext *dms) {

	Cpu *cpu = dms->device->get_cpu(dms->device);
	int64_t tick_max = dms->simulator->current_tick + dms->sync_tick_interval;

	while (dms->config.state != DS_WAIT && dms->simulator->current_tick < tick_max) {

		// start stopwatch if not already activated
		if (dms->config.state == DS_RUN && !dms->stopwatch->running) {
			stopwatch_start(dms->stopwatch);
			dms->tick_start_run = dms->simulator->current_tick;
		}

		// process the device
		dms->device->process(dms->device);

		switch (dms->config.state) {
			case DS_SINGLE_STEP :
				context_change_state(dms, DS_WAIT);
				break;
			case DS_STEP_SIGNAL :
				if (signal_breakpoint_check(dms, &dms->config.step_signal)) {
					context_change_state(dms, DS_WAIT);
				}
				break;
			case DS_RUN :
				// check for breakpoints
				if (context_check_breakpoints(dms, cpu)) {
					context_change_state(dms, DS_WAIT);
				}
				break;
			case DS_EXIT:
				return false;
			case DS_WAIT:
				break;
		}
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
	context->config_usr.state = new_state;
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
	DmsContext *ctx = (DmsContext *) dms_calloc(1, sizeof(DmsContext));
	ctx->simulator = NULL;
	ctx->device = NULL;
	ctx->usr_break_irq = false;
	ctx->stopwatch = stopwatch_create();

	ctx->config.state = DS_WAIT;
	ctx->config.target_sim_real_ratio = 10000;
	ctx->config.pc_breakpoints = NULL;
	ctx->config.signal_breakpoints = NULL;
	ctx->config.bp_updated = false;
	dms_memcpy(&ctx->config_usr, &ctx->config, sizeof(ctx->config));

#ifndef DMS_NO_THREADING
	mutex_init_plain(&ctx->mtx_wait);
	mutex_init_plain(&ctx->mtx_config);
	cond_init(&ctx->cnd_wait);
#endif

	return ctx;
}

void dms_release_context(DmsContext *dms) {
	assert(dms);
	stopwatch_destroy(dms->stopwatch);
	dms_free(dms);
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

DMS_STATE dms_get_state(DmsContext *dms) {
	MUTEX_CONFIG_LOCK(dms);
		DMS_STATE result = dms->config_usr.state;
	MUTEX_CONFIG_UNLOCK(dms);
	return result;
}

#ifndef DMS_NO_THREADING

void dms_start_execution(DmsContext *dms) {
	assert(dms);
	assert(dms->device);

	// initialize context configuration
	dms->config.state = DS_WAIT;
	dms_memcpy(&dms->config_usr, &dms->config, sizeof(dms->config));

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
	dms->config_usr.state = DS_RUN;
	context_execute(dms);
}

void dms_single_step(DmsContext *dms) {
	assert(dms);

	if (dms->config_usr.state == DS_WAIT) {
		change_state(dms, DS_SINGLE_STEP);
	}
}

void dms_step_signal(struct DmsContext *dms, Signal signal, bool pos_edge, bool neg_edge) {
	assert(dms);

	if (dms->config_usr.state == DS_WAIT) {
		dms->config_usr.step_signal = (SignalBreakpoint) {
			.signal = signal,
			.pos_edge = pos_edge,
			.neg_edge = neg_edge
		};
		change_state(dms, DS_STEP_SIGNAL);
	}
}


void dms_run(DmsContext *dms) {
	assert(dms);

	if (dms->config_usr.state == DS_WAIT) {
		change_state(dms, DS_RUN);
	}
}

void dms_pause(DmsContext *dms) {
	assert(dms);

	if (dms->config_usr.state == DS_RUN) {
		change_state(dms, DS_WAIT);
	}
}

bool dms_is_paused(DmsContext *dms) {
	return dms->config_usr.state == DS_WAIT;
}

void dms_change_simulation_speed_ratio(DmsContext *dms, double ratio) {
	assert(dms);

	MUTEX_CONFIG_LOCK(dms);

		dms->config_usr.target_sim_real_ratio = (int64_t) (ratio * 10000);
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
	bool result;

	MUTEX_CONFIG_LOCK(dms);
		int idx = breakpoint_index(dms, addr);

		if (idx < 0) {
			arrpush(dms->config_usr.pc_breakpoints, addr);
			result = true;
		} else {
			arrdelswap(dms->config.pc_breakpoints, idx);
			result = false;
		}
		dms->config_usr.bp_updated = true;
		dms->config_changed = true;
	MUTEX_CONFIG_UNLOCK(dms);

	return result;
}

SignalBreakpoint *dms_breakpoint_signal_list(DmsContext *dms) {
	assert(dms);
	return dms->config_usr.signal_breakpoints;
}

void dms_breakpoint_signal_set(DmsContext *dms, Signal signal, bool pos_edge, bool neg_edge) {
	assert(dms);

	MUTEX_CONFIG_LOCK(dms);
		if (signal_breakpoint_index(dms, signal) < 0) {
			arrpush(dms->config_usr.signal_breakpoints, ((SignalBreakpoint) {signal, pos_edge, neg_edge}));
			dms->config_usr.bp_updated = true;
			dms->config_changed = true;
		}
	MUTEX_CONFIG_UNLOCK(dms);
}

void dms_breakpoint_signal_clear(DmsContext *dms, Signal signal) {
	assert(dms);

	MUTEX_CONFIG_LOCK(dms);
		int idx = signal_breakpoint_index(dms, signal);
		if (idx	>= 0) {
			arrdelswap(dms->config_usr.signal_breakpoints, idx);
			dms->config_usr.bp_updated = true;
			dms->config_changed = true;
		}
	MUTEX_CONFIG_UNLOCK(dms);
}

bool dms_breakpoint_signal_is_set(struct DmsContext *dms, Signal signal) {
	assert(dms);
	return signal_breakpoint_index(dms, signal) >= 0;
}

bool dms_toggle_signal_breakpoint(DmsContext *dms, Signal signal) {
	bool result;

	MUTEX_CONFIG_LOCK(dms);
		int idx = signal_breakpoint_index(dms, signal);
		if (idx < 0) {
			arrpush(dms->config_usr.signal_breakpoints, ((SignalBreakpoint) {signal, true, true}));
			result = true;
		} else {
			arrdelswap(dms->config_usr.signal_breakpoints, idx);
			result = false;
		}

		dms->config_usr.bp_updated = true;
		dms->config_changed = true;
	MUTEX_CONFIG_UNLOCK(dms);
	return result;
}

void dms_break_on_irq_set(struct DmsContext *dms) {
	assert(dms);

	SignalBreakpoint *irq_signals = NULL;
	size_t count = dms->device->get_irq_signals(dms->device, &irq_signals);

	for (size_t i = 0; i < count; ++i) {
		dms_breakpoint_signal_set(dms, irq_signals[i].signal, irq_signals[i].pos_edge, irq_signals[i].neg_edge);
	}
}

void dms_break_on_irq_clear(struct DmsContext *dms) {
	assert(dms);

	SignalBreakpoint *irq_signals = NULL;
	size_t count = dms->device->get_irq_signals(dms->device, &irq_signals);

	for (size_t i = 0; i < count; ++i) {
		dms_breakpoint_signal_clear(dms, irq_signals[i].signal);
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
		dms->usr_break_irq = !dms->usr_break_irq;
		if (dms->usr_break_irq) {
			dms_break_on_irq_set(dms);
		} else {
			dms_break_on_irq_clear(dms);
		}

		static const char *disp_enabled[] = {"disabled", "enabled"};
		arr_printf(*reply, "OK: break-on-irq %s", disp_enabled[dms->usr_break_irq]);
	} else if (cmd[0] == 'b' && cmd[1] == 's') {		// toggle "b"reak on signal change
		Signal signal = signal_by_name(dms->simulator->signal_pool, cmd + 3);

		if (!signal_is_undefined(signal)) {
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
	} else if (cmd[0] == 'h' || cmd[0] == '?') {		// help
		arr_printf(*reply, "g <address> : make CPU jump to specified address.\n");
		arr_printf(*reply, "bi          : set/clear breakpoint on IRQ assertion.\n");
		arr_printf(*reply, "bs <signal> : set/clear breakpoint on signal modification.\n");
		arr_printf(*reply, "b <address> : set/clear breakpoint on program counter.\n");
	} else {
		arr_printf(*reply, "NOK: invalid command");
	}
}
