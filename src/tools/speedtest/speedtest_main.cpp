// speedtest_main.cpp - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Performance analysis test harness

#include <cstdio>
#include <cassert>
#include <chrono>

#include "dev_commodore_pet.h"
#include "context.h"

namespace {

using namespace std::chrono;
steady_clock::time_point chrono_ref;

inline void chrono_reset() {
    chrono_ref = steady_clock::now();
}

inline double chrono_report() {
    duration<double> span = duration_cast<duration<double>>(steady_clock::now() - chrono_ref);
    return span.count();
}

} // unnamed namespace

int main(int argc, char *argv[]) {

	// basic (read: stupid) command line argument handling
	bool arg_lite = (argc == 2 && !strcmp(argv[1], "--lite"));

    std::printf("--- setting up Dromaius (%s PET)\n", (arg_lite) ? "lite" : "full");
    chrono_reset();

	auto pet_device = (!arg_lite) ? dev_commodore_pet_create() : dev_commodore_pet_lite_create();
	assert(pet_device);

	// create dromaius context
	auto dms_ctx = dms_create_context();
	assert(dms_ctx);
	dms_set_device(dms_ctx, reinterpret_cast<Device *>(pet_device));

    std::printf("+++ done (%f seconds)\n", chrono_report());

    std::printf("--- running Commodore PET until BASIC screen\n");
    chrono_reset();

	bool ready = false;

	dms_run(dms_ctx);

	while (!ready) {
		dms_execute_no_sync(dms_ctx);

		// check screen memory
		uint8_t first_char;
		pet_device->read_memory(pet_device, 0x8000, 1, &first_char);
		ready = first_char == 0x2a;
	}


    double duration = chrono_report();
	double sim_time = (pet_device->simulator->current_tick * pet_device->simulator->tick_duration_ps) / 1e12;
	double speed = (1000000 * sim_time) / duration;
    std::printf("+++ done (%f seconds) sim-time = %f seconds (speed = %.2f hz, %.3f Mhz)\n", duration, sim_time, speed, speed / 1000000.0);

    return 0;
}
