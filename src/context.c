// context.c - Johan Smet - BSD-3-Clause (see LICENSE)

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
} _Atomic DMS_STATE;

typedef struct DmsContext {
	DevMinimal6502 *device;	

	thread_t		thread;
	DMS_STATE		state;
	mutex_t			mtx_wait;
	cond_t			cnd_wait;

	uint64_t *		breakpoints;
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

static void *context_execution(DmsContext *dms) {

	while (true) {

		while (dms->state == DS_WAIT) {
			mutex_lock(&dms->mtx_wait);
			cond_wait(&dms->cnd_wait, &dms->mtx_wait);
		}

		mutex_unlock(&dms->mtx_wait);
		
		bool prev_sync = dms->device->line_cpu_sync;

		dev_minimal_6502_process(dms->device);

		int cur_state = dms->state;
		switch (cur_state) {
			case DS_SINGLE_STEP :
				dms->state = DS_WAIT;
				break;
			case DS_SINGLE_INSTRUCTION :
				if (!prev_sync && dms->device->line_cpu_sync) {
					dms->state = DS_WAIT;
				}
				break;
			case DS_RUN :
				// check for breakpoints
				if (dms->device->line_cpu_sync && breakpoint_index(dms, dms->device->cpu->reg_pc) >= 0) {
					dms->state = DS_WAIT;
				}
				break;
			case DS_RESET:
				dev_minimal_6502_reset(dms->device);
				dms->state = DS_WAIT;
				break;
			case DS_EXIT:
				return NULL;
		}
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

DmsContext *dms_create_context(void) {
	DmsContext *ctx = (DmsContext *) malloc(sizeof(DmsContext));
	ctx->device = NULL;
	ctx->breakpoints = NULL;
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

void dms_start_execution(DmsContext *dms) {
	assert(dms);
	assert(dms->device);
	
	dms->state = DS_WAIT;
	mutex_init_plain(&dms->mtx_wait);
	cond_init(&dms->cnd_wait);
	thread_create_joinable(&dms->thread, (int(*)(void*)) context_execution, dms);
}

void dms_stop_execution(DmsContext *dms) {
	assert(dms);

	dms->state = DS_EXIT;

	int thread_res;
	thread_join(dms->thread, &thread_res);
}

void dms_single_step(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		dms->state = DS_SINGLE_STEP;
		cond_signal(&dms->cnd_wait);
	}
}

void dms_single_instruction(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		dms->state = DS_SINGLE_INSTRUCTION;
		cond_signal(&dms->cnd_wait);
	}
}

void dms_run(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		dms->state = DS_RUN;
		cond_signal(&dms->cnd_wait);
	}
}

void dms_pause(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_RUN) {
		dms->state = DS_WAIT;
		cond_signal(&dms->cnd_wait);
	}
}

void dms_reset(DmsContext *dms) {
	assert(dms);

	dms->state = DS_RESET;
	cond_signal(&dms->cnd_wait);
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
		if (sscanf(cmd + 1, "%lx", &addr) == 1) {
			DevMinimal6502 *dev = dms_get_device(dms);

			// change clock toggle each time _process is called
			int32_t save_freq = dev->clock->conf_frequency;
			clock_set_frequency(dev->clock, 0);

			// tell the cpu to override the location of the next instruction
			cpu_6502_override_next_instruction_address(dev->cpu, addr);

			// run computer until current instruction is finished
			while (!(dev->line_cpu_sync && dev->cpu->reg_pc == addr)) {
				dev_minimal_6502_process(dev);
			}

			// restore clock frequency
			clock_set_frequency(dev->clock, save_freq);

			arr_printf(*reply, "OK: pc changed to 0x%lx", addr);
		} else {
			arr_printf(*reply, "NOK: invalid address specified");
		}
	} else if (cmd[0] == 'b') {		// toggle "b"reak-point
		int64_t addr;
		if (sscanf(cmd + 1, "%lx", &addr) == 1) {
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
