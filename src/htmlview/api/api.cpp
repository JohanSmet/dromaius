// htmlview/api/api.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include <emscripten/bind.h>
#include <cassert>
#include <vector>
#include <map>

#include "context.h"
#include "dev_commodore_pet.h"
#include "display_rgba.h"
#include "stb/stb_ds.h"

using namespace emscripten;

class DmsApi {
public:
	struct DisplayInfo {
		size_t width;
		size_t height;
	};

	struct SignalInfo {
		std::vector<size_t>				count;
		std::map<std::string, Signal>	names;
	};

public:
	DmsApi() = default;

	// device control
	void launch_commodore_pet();

	// dromaius context
	void context_execute() {
		assert(dms_ctx);
		dms_execute(dms_ctx);
	}

	void context_step() {
		assert(dms_ctx);
		dms_single_instruction(dms_ctx);
	}

	void context_run() {
		assert(dms_ctx);
		dms_run(dms_ctx);
	}

	void context_pause() {
		assert(dms_ctx);
		dms_pause(dms_ctx);
	}

	void context_reset() {
		assert(dms_ctx);
		dms_reset(dms_ctx);
	}

	// data access
	val display_data() const {
		assert(pet_device);
		return val(typed_memory_view(pet_device->display->width * pet_device->display->height * 4,
									 reinterpret_cast<uint8_t *> (pet_device->display->frame)));
	}

	val signal_data(int8_t domain) const {
		assert(pet_device);
		auto &d = pet_device->signal_pool->domains[domain];
		return val(typed_memory_view(arrlenu(d.signals_next), reinterpret_cast<uint8_t *> (d.signals_next)));
	}

	// hardware info
	DisplayInfo display_info() {
		assert(pet_device);
		return DisplayInfo{pet_device->display->width, pet_device->display->height};
	}

	SignalInfo signal_info() {
		assert(pet_device);
		auto pool = pet_device->signal_pool;
		SignalInfo	result;

		for (int d = 0; d < pool->num_domains; ++d) {
			result.count.push_back(arrlenu(pool->domains[d].signals_curr));
		}

		for (int s = 0; s < shlen(pool->signal_names); ++s) {
			std::string name = pool->signal_names[s].key;
			auto &signal = pool->signal_names[s].value;
			result.names[name] = signal;
		}

		return result;
	}

private:
	DmsContext *	dms_ctx;
	DevCommodorePet *	pet_device;
};

void DmsApi::launch_commodore_pet() {
	// create pet
	pet_device = dev_commodore_pet_create();
	assert(pet_device);

	// create dromaius context
	dms_ctx = dms_create_context();
	assert(dms_ctx);
	dms_set_device(dms_ctx, reinterpret_cast<Device *>(pet_device));

	// reset device
	pet_device->reset(pet_device);
}

// binding code
EMSCRIPTEN_BINDINGS(DmsApiBindings) {

	register_vector<size_t>("VectorSize");
	register_vector<std::string>("VectorString");
	register_map<std::string, Signal>("MapStringSignal");

	value_object<Signal>("Signal")
		.field("domain", &Signal::domain)
		.field("start", &Signal::start)
		.field("count", &Signal::count)
		;

	value_object<DmsApi::DisplayInfo>("DmsApi__DisplayInfo")
		.field("width", &DmsApi::DisplayInfo::width)
		.field("height", &DmsApi::DisplayInfo::height)
		;

	value_object<DmsApi::SignalInfo>("DmsApi__SignalInfo")
		.field("count", &DmsApi::SignalInfo::count)
		.field("names", &DmsApi::SignalInfo::names)
		;

	class_<DmsApi>("DmsApi")
		.constructor()
		.function("launch_commodore_pet", &DmsApi::launch_commodore_pet)
		.function("context_execute", &DmsApi::context_execute)
		.function("context_step", &DmsApi::context_step)
		.function("context_run", &DmsApi::context_run)
		.function("context_pause", &DmsApi::context_pause)
		.function("context_reset", &DmsApi::context_reset)
		.function("display_data", &DmsApi::display_data)
		.function("signal_data", &DmsApi::signal_data)
		.function("display_info", &DmsApi::display_info)
		.function("signal_info", &DmsApi::signal_info)
		;
}
