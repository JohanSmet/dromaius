#include <emscripten.h>
#include <stdlib.h>
#include <assert.h>
#include "stb/stb_ds.h"
#include <stdio.h>

#include "context.h"
#include "cpu_6502.h"
#include "dev_commodore_pet.h"
#include "display_rgba.h"

#include "utils.h"

typedef struct DmsApiContext {
	struct DmsContext *	dms_ctx;
	DevCommodorePet *	pet_device;
} DmsApiContext;

EMSCRIPTEN_KEEPALIVE
DmsApiContext *dmsapi_create_context(void) {
	DmsApiContext *context = (DmsApiContext *) calloc(1, sizeof(DmsApiContext));

	return context;
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_release_context(DmsApiContext *context) {
	assert(context);
	free(context);
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_launch_commodore_pet(DmsApiContext *context) {
	assert(context);

	// create pet
	context->pet_device = dev_commodore_pet_create();
	assert(context->pet_device);

	// create dromaius context
	context->dms_ctx = dms_create_context();
	dms_set_device(context->dms_ctx, (Device *) context->pet_device);

	// reset device
	context->pet_device->reset(context->pet_device);
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_context_execute(DmsApiContext *context) {
	assert(context);
	assert(context->dms_ctx);
	dms_execute(context->dms_ctx);
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_context_step(DmsApiContext *context) {
	assert(context);
	assert(context->dms_ctx);
	dms_single_instruction(context->dms_ctx);
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_context_run(DmsApiContext *context) {
	assert(context);
	assert(context->dms_ctx);
	dms_run(context->dms_ctx);
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_context_pause(DmsApiContext *context) {
	assert(context);
	assert(context->dms_ctx);
	dms_pause(context->dms_ctx);
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_context_reset(DmsApiContext *context) {
	assert(context);
	assert(context->dms_ctx);
	dms_reset(context->dms_ctx);
}

EMSCRIPTEN_KEEPALIVE
const char *dmsapi_cpu_6502_info(DmsApiContext *context) {
	char *json = NULL;

	Cpu6502 *cpu = context->pet_device->cpu;

	arr_printf(json, "{");
	arr_printf(json, "\"reg_ir\": %d,", cpu->reg_ir);
	arr_printf(json, "\"reg_pc\": %d", cpu->reg_pc);
	arr_printf(json, "}");

	return json;
}

EMSCRIPTEN_KEEPALIVE
const char *dmsapi_display_info(DmsApiContext *context) {
	char *json = NULL;

	DisplayRGBA *display = context->pet_device->display;

	arr_printf(json, "{");
	arr_printf(json, "\"width\": %d,", display->width);
	arr_printf(json, "\"height\": %d", display->height);
	arr_printf(json, "}");

	return json;
}

EMSCRIPTEN_KEEPALIVE
const char *dmsapi_signal_info(DmsApiContext *context) {
	assert(context);

	char *json = NULL;

	SignalPool *pool = context->pet_device->signal_pool;

	arr_printf(json, "{");
	arr_printf(json, "\"count\": [");
	for (int d = 0; d < pool->num_domains; ++d) {
		if (d > 0) {
			arr_printf(json, ",");
		}
		arr_printf(json, "%d", arrlenu(pool->domains[d].signals_curr));
	}
	arr_printf(json, "],");

	arr_printf(json, "\"names\": [");
	for (int d = 0; d < pool->num_domains; ++d) {
		SignalDomain *domain = &pool->domains[d];
		if (d > 0) {
			arr_printf(json, ",");
		}
		arr_printf(json, "{");

		bool output = false;

		for (int s = 0; s < arrlenu(domain->signals_curr); ++s) {
			const char *name = domain->signals_name + (s * MAX_SIGNAL_NAME);
			if (*name != '\0') {
				if (output) {
					arr_printf(json, ",");
				}
				arr_printf(json, "\"%d\": \"%s\"", s, name);
				output = true;
			}
		}

		arr_printf(json, "}");
	}
	arr_printf(json, "]}");

	return json;
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_free_json(char *json) {
	arrfree(json);
}

EMSCRIPTEN_KEEPALIVE
void dmsapi_display_data(DmsApiContext *context, uint32_t *memory) {
	assert(context);
	memcpy(memory, context->pet_device->display->frame, 320 * 200 * 4);
}

EMSCRIPTEN_KEEPALIVE
int dmsapi_signal_data(DmsApiContext *context, int8_t domain, uint8_t *memory) {
	assert(context);
	assert(domain > 0 && domain < context->pet_device->signal_pool->num_domains);

	bool *signals = context->pet_device->signal_pool->domains[domain].signals_curr;
	memcpy(memory, signals, arrlenu(signals));

	return arrlenu(signals);
}
