// context.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "context.h"
#include "dev_minimal_6502.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <threads.h>

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

	thrd_t			thr_id;
	DMS_STATE		state;
	mtx_t			mtx_wait;
	cnd_t			cnd_wait;
} DmsContext;

///////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

static int context_execution(DmsContext *dms) {

	while (true) {

		while (dms->state == DS_WAIT) {
			mtx_lock(&dms->mtx_wait);
			cnd_wait(&dms->cnd_wait, &dms->mtx_wait);
		}

		mtx_unlock(&dms->mtx_wait);
		
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
				// TODO: check for breakpoints
				break;
			case DS_RESET:
				dev_minimal_6502_reset(dms->device);
				dms->state = DS_WAIT;
				break;
			case DS_EXIT:
				return 0;
		}
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

DmsContext *dms_create_context(void) {
	DmsContext *ctx = (DmsContext *) malloc(sizeof(DmsContext));
	ctx->device = NULL;
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

void dms_start_execution(DmsContext *dms) {
	assert(dms);
	assert(dms->device);
	
	dms->state = DS_WAIT;
	mtx_init(&dms->mtx_wait, mtx_plain);
	cnd_init(&dms->cnd_wait);
	thrd_create(&dms->thr_id, (int(*)(void *)) context_execution, dms);
}

void dms_stop_execution(DmsContext *dms) {
	assert(dms);

	dms->state = DS_EXIT;

	int thread_res;
	thrd_join(dms->thr_id, &thread_res);
}

void dms_single_step(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		dms->state = DS_SINGLE_STEP;
		cnd_signal(&dms->cnd_wait);
	}
}

void dms_single_instruction(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		dms->state = DS_SINGLE_INSTRUCTION;
		cnd_signal(&dms->cnd_wait);
	}
}

void dms_run(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_WAIT) {
		dms->state = DS_RUN;
		cnd_signal(&dms->cnd_wait);
	}
}

void dms_pause(DmsContext *dms) {
	assert(dms);
	
	if (dms->state == DS_RUN) {
		dms->state = DS_WAIT;
		cnd_signal(&dms->cnd_wait);
	}
}

void dms_reset(DmsContext *dms) {
	assert(dms);

	dms->state = DS_RESET;
	cnd_signal(&dms->cnd_wait);
}
