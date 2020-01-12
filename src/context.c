// context.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// main external interface for others to control a Dromaius instance
// define DMS_NO_THREADING to disable multithreading

#include "context.h"
#include "dev_minimal_6502.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys/threads.h"

#include "utils.h"
#include <stb/stb_ds.h>

#include "dev_minimal_6502.h"
#include "clock.h"

///////////////////////////////////////////////////////////////////////////////
//
// internal types
//

typedef enum DMS_STATE {
	DS_WAIT = 0,
	DS_SINGLE_STEP = 1,
	DS_SINGLE_INSTRUCTION = 2,
	DS_RUN = 3,
	DS_RESET = 98,
	DS_EXIT = 99
} DMS_STATE;

typedef struct DmsContext {
	DevMinimal6502 *device;	
	DMS_STATE		state;
	uint64_t *		breakpoints;

#ifndef DMS_NO_THREADING
	thread_t		thread;
	mutex_t			mtx_wait;
	cond_t			cnd_wait;
#endif // DMS_NO_THREADING
} DmsContext;

///////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

static inline int breakpoint_index(DmsContext *dms, uint64_t addr) {
	for (int i = 0; i < arrlen(dms->breakpoints); ++i) {
		if (dms->breakpoints[i] == addr) { 
			return i;
		}
	}

	return -1;
}

static bool context_execute(DmsContext *dms) {

	clock_mark(dms->device->clock);

	while (dms->state != DS_WAIT && !clock_is_caught_up(dms->device->clock)) {

		dev_minimal_6502_process(dms->device);

		bool prev_sync = signal_read_bool(dms->device->signal_pool, dms->device->sig_cpu_sync);
		bool cpu_sync  = signal_read_next_bool(dms->device->signal_pool, dms->device->sig_cpu_sync);

		switch (dms->state) {
			case DS_SINGLE_STEP :
				dms->state = DS_WAIT;
				break;
			case DS_SINGLE_INSTRUCTION :
				if (!prev_sync && cpu_sync) {
					dms->state = DS_WAIT;
				}
				break;
			case DS_RUN :
				// check for breakpoints
				if (cpu_sync && breakpoint_index(dms, dms->device->cpu->reg_pc) >= 0) {
					dms->state = DS_WAIT;
				}
				break;
			case DS_RESET:
				dev_minimal_6502_reset(dms->device);
				dms->state = DS_WAIT;
				break;
			case DS_EXIT:
				return false;
		}
	}

	return true;
}

#ifndef DMS_NO_THREADING

static void *context_background_thread(DmsContext *dms) {

	bool keep_running = true;

	while (keep_running) {

		mutex_lock(&dms->mtx_wait);

		while (dms->state == DS_WAIT) {
			cond_wait(&dms->cnd_wait, &dms->mtx_wait);
		}
		
		keep_running = context_execute(dms);

		mutex_unlock(&dms->mtx_wait);
	}

	return NULL;
}

static inline void change_state(DmsContext *context, DMS_STATE new_state) {
	mutex_lock(&context->mtx_wait);
	context->state = new_state;
	mutex_unlock(&context->mtx_wait);
	cond_signal(&context->cnd_wait);
}

#else

static inline void change_state(DmsContext *context, DMS_STATE new_state) {
	context->state = new_state;
}

#endif // DMS_NO_THREADING

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

DmsContext *dms_create_context(void) {
	DmsContext *ctx = (DmsContext *) malloc(sizeof(DmsContext));
	ctx->device = NULL;
	ctx->breakpoints = NULL;
	ctx->state = DS_WAIT;
	return ctx;
}

void dms_release_context(DmsContext *dms) {
	assert(dms);
	free(dms);
}

void dms_set_device(DmsContext *dms, DevMinimal6502 *device) {
	assert(dms);
	dms->device = device;
}

struct DevMinimal6502 *dms_get_device(struct DmsContext *dms) {
	assert(dms);
	return dms->device;
}

#ifndef DMS_NO_THREADING

void dms_start_execution(DmsContext *dms) {
	assert(dms);
	assert(dms->device);
	
	dms->state = DS_WAIT;
	mutex_init_plain(&dms->mtx_wait);
	cond_init(&dms->cnd_wait);
	thread_create_joinable(&dms->thread, (int(*)(void*)) context_background_thread, dms);
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
}

#endif // DMS_NO_THREADING

void dms_single_step(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		change_state(dms, DS_SINGLE_STEP);
	}
}

void dms_single_instruction(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		change_state(dms, DS_SINGLE_INSTRUCTION);
	}
}

void dms_run(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		change_state(dms, DS_RUN);
	}
}

void dms_pause(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_RUN) {
		change_state(dms, DS_WAIT);
	}
}

void dms_reset(DmsContext *dms) {
	assert(dms);

	change_state(dms, DS_RESET);
}


bool dms_toggle_breakpoint(DmsContext *dms, uint64_t addr) {
	int idx = breakpoint_index(dms, addr);

	if (idx < 0) {
		arrpush(dms->breakpoints, addr);
		return true;
	} else {
		arrdelswap(dms->breakpoints, idx);
		return false;
	}
}

void dms_monitor_cmd(struct DmsContext *dms, const char *cmd, char **reply) {
	assert(dms);
	assert(cmd);

	if (cmd[0] == 'g') {		// "g"oto address
		int64_t addr;

		if (string_to_hexint(cmd + 1, &addr)) {
			DevMinimal6502 *dev = dms_get_device(dms);

			// tell the cpu to override the location of the next instruction
			cpu_6502_override_next_instruction_address(dev->cpu, addr & 0xffff);

			// run computer until current instruction is finished
			bool cpu_sync  = signal_read_next_bool(dms->device->signal_pool, dms->device->sig_cpu_sync);

			while (!(cpu_sync && dev->cpu->reg_pc == addr)) {
				dev_minimal_6502_process(dev);
				cpu_sync  = signal_read_next_bool(dms->device->signal_pool, dms->device->sig_cpu_sync);
			}

			arr_printf(*reply, "OK: pc changed to 0x%lx", addr);
		} else {
			arr_printf(*reply, "NOK: invalid address specified");
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
