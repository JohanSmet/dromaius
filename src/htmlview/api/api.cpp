// htmlview/api/api.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include <emscripten/bind.h>
#include <cassert>
#include <vector>
#include <map>

#include "context.h"
#include "cpu_6502.h"
#include "dev_commodore_pet.h"
#include "display_pet_crt.h"
#include "input_keypad.h"

#include "stb/stb_ds.h"

using namespace emscripten;

class DmsApi {
public:
	struct DisplayInfo {
		size_t width;
		size_t height;
	};

	struct SignalInfo {
		size_t							count;
		std::map<std::string, Signal>	names;
	};

	struct SignalDetails {
		Signal			signal;
		int32_t			value;
		int32_t			writer_id;
		std::string		writer_name;
	};

	struct ClockInfo {
		int32_t current_tick;
		double time_elapsed_ns;
	};

	struct KeyInfo {
		uint32_t		row;
		uint32_t		column;
	};

public:
	DmsApi() = default;

	// device control
	void launch_commodore_pet();

	// dromaius context
	bool context_execute() {
		assert(dms_ctx);

		int64_t save_ts = pet_device->signal_pool->current_tick;
		dms_execute(dms_ctx);

		return save_ts != pet_device->signal_pool->current_tick;
	}

	void context_select_step_clock(const std::string &signal_name) {
		auto signal = signal_by_name(pet_device->signal_pool, signal_name.c_str());
		if (signal.start != 0) {
			step_clock = signal;
		}
	}

	void context_single_step() {
		assert(dms_ctx);
		dms_single_step(dms_ctx);
	}

	void context_step_clock() {
		assert(dms_ctx);
		dms_step_signal(dms_ctx, step_clock, true, true);
	}

	void context_step_instruction() {
		assert(dms_ctx);
		dms_step_signal(dms_ctx, pet_device->signals.cpu_sync, true, false);
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
		pet_device->reset(pet_device);
	}

	// input
	int keyboard_num_rows() const {
		assert(pet_device);
		return pet_device->keypad->row_count;
	}

	int keyboard_num_columns() const {
		assert(pet_device);
		return pet_device->keypad->col_count;
	}

	void keyboard_key_pressed(int row, int col) {
		assert(pet_device);
		input_keypad_key_pressed(pet_device->keypad, row, col);
	}

	std::vector<KeyInfo> keyboard_keys_down() const {
		assert(pet_device);

		auto count = input_keypad_keys_down_count(pet_device->keypad);
		auto keys = input_keypad_keys_down(pet_device->keypad);

		std::vector<KeyInfo> result(count);

		for (size_t i = 0; i < count; ++i) {
			input_keypad_index_to_row_col(pet_device->keypad, keys[i], &result[i].row, &result[i].column);
		}

		return result;
	}

	// data access
	val display_data() const {
		assert(pet_device);
		return val(typed_memory_view(pet_device->crt->display->width * pet_device->crt->display->height * 4,
									 reinterpret_cast<uint8_t *> (pet_device->crt->display->frame)));
	}

	val signal_data() const {
		assert(pet_device);
		auto pool = pet_device->signal_pool;
		return val(typed_memory_view(arrlenu(pool->signals_next), reinterpret_cast<uint8_t *> (pool->signals_next)));
	}

	// breakpoints
	std::vector<std::string> breakpoint_signal_list() const {
		std::vector<std::string>	result;

		auto bps = dms_breakpoint_signal_list(dms_ctx);

		for (ptrdiff_t idx = 0; idx < arrlen(bps); ++idx) {
			result.push_back(pet_device->signal_pool->signals_name[bps[idx].start]);
		}

		return result;
	}

	void breakpoint_signal_set(const std::string &signal_name) {
		auto signal = signal_by_name(pet_device->signal_pool, signal_name.c_str());
		if (signal.count != 0) {
			dms_breakpoint_signal_set(dms_ctx, signal);
		}
	}

	void breakpoint_signal_clear(const std::string &signal_name) {
		auto signal = signal_by_name(pet_device->signal_pool, signal_name.c_str());
		if (signal.count != 0) {
			dms_breakpoint_signal_clear(dms_ctx, signal);
		}
	}

	// hardware info
	DisplayInfo display_info() {
		assert(pet_device);
		return DisplayInfo{pet_device->crt->display->width, pet_device->crt->display->height};
	}

	SignalInfo signal_info() {
		assert(pet_device);
		auto pool = pet_device->signal_pool;
		SignalInfo	result;

		result.count = arrlenu(pool->signals_curr);

		for (int s = 0; s < shlen(pool->signal_names); ++s) {
			std::string name = pool->signal_names[s].key;
			auto &signal = pool->signal_names[s].value;
			result.names[name] = signal;
		}

		return result;
	}

	SignalDetails signal_details(std::string name) {
		assert(pet_device);
		auto pool = pet_device->signal_pool;
		SignalDetails result;

		result.signal = shget(pool->signal_names, name.c_str());
		result.value = 0;
		result.writer_id = -1;

		if (result.signal.count > 0) {
			// writer
			result.writer_id = pool->signals_writer[result.signal.start];
			if (result.writer_id >= 0) {
				result.writer_name = pet_device->chips[result.writer_id]->name;
			}

			// value
			result.value = signal_read_next_uint16(pet_device->signal_pool, result.signal);
		}

		return result;
	}

	Cpu6502 cpu_info() {
		assert(pet_device);
		return *pet_device->cpu;
	}

	ClockInfo clock_info() {
		return (ClockInfo) {
			static_cast<int32_t>(pet_device->signal_pool->current_tick),		// fixme
			pet_device->signal_pool->current_tick * pet_device->signal_pool->tick_duration_ps / 1000.0
		};
	}

private:
	DmsContext *		dms_ctx;
	DevCommodorePet *	pet_device;
	Signal				step_clock;
};

void DmsApi::launch_commodore_pet() {
	// create pet
	pet_device = dev_commodore_pet_create();
	assert(pet_device);

	// create dromaius context
	dms_ctx = dms_create_context();
	assert(dms_ctx);
	dms_set_device(dms_ctx, reinterpret_cast<Device *>(pet_device));

	// default clock to step
	step_clock = pet_device->signals.clk1;
}

// binding code
EMSCRIPTEN_BINDINGS(DmsApiBindings) {

	register_vector<size_t>("VectorSize");
	register_vector<std::string>("VectorString");
	register_vector<DmsApi::KeyInfo>("VectorKeyInfo");
	register_map<std::string, Signal>("MapStringSignal");

	value_object<Signal>("Signal")
		.field("start", &Signal::start)
		.field("count", &Signal::count)
		;

	value_object<Cpu6502>("Cpu6502")
		.field("reg_a", &Cpu6502::reg_a)
		.field("reg_x", &Cpu6502::reg_x)
		.field("reg_y", &Cpu6502::reg_y)
		.field("reg_sp", &Cpu6502::reg_sp)
		.field("reg_ir", &Cpu6502::reg_ir)
		.field("reg_pc", &Cpu6502::reg_pc)
		.field("reg_p", &Cpu6502::reg_p)
		;

	value_object<DmsApi::DisplayInfo>("DmsApi__DisplayInfo")
		.field("width", &DmsApi::DisplayInfo::width)
		.field("height", &DmsApi::DisplayInfo::height)
		;

	value_object<DmsApi::SignalInfo>("DmsApi__SignalInfo")
		.field("count", &DmsApi::SignalInfo::count)
		.field("names", &DmsApi::SignalInfo::names)
		;

	value_object<DmsApi::SignalDetails>("DmsApi__SignalDetails")
		.field("signal", &DmsApi::SignalDetails::signal)
		.field("value", &DmsApi::SignalDetails::value)
		.field("writer_id", &DmsApi::SignalDetails::writer_id)
		.field("writer_name", &DmsApi::SignalDetails::writer_name)
		;

	value_object<DmsApi::ClockInfo>("DmsApi__ClockInfo")
		.field("current_tick", &DmsApi::ClockInfo::current_tick)
		.field("time_elapsed_ns", &DmsApi::ClockInfo::time_elapsed_ns)
		;

	value_object<DmsApi::KeyInfo>("DmsApi__KeyInfo")
		.field("row", &DmsApi::KeyInfo::row)
		.field("column", &DmsApi::KeyInfo::column)
		;

	class_<DmsApi>("DmsApi")
		.constructor()
		.function("launch_commodore_pet", &DmsApi::launch_commodore_pet)
		.function("context_execute", &DmsApi::context_execute)
		.function("context_select_step_clock", &DmsApi::context_select_step_clock)
		.function("context_single_step", &DmsApi::context_single_step)
		.function("context_step_clock", &DmsApi::context_step_clock)
		.function("context_step_instruction", &DmsApi::context_step_instruction)
		.function("context_run", &DmsApi::context_run)
		.function("context_pause", &DmsApi::context_pause)
		.function("context_reset", &DmsApi::context_reset)
		.function("keyboard_num_rows", &DmsApi::keyboard_num_rows)
		.function("keyboard_num_columns", &DmsApi::keyboard_num_columns)
		.function("keyboard_key_pressed", &DmsApi::keyboard_key_pressed)
		.function("keyboard_keys_down", &DmsApi::keyboard_keys_down)
		.function("display_data", &DmsApi::display_data)
		.function("signal_data", &DmsApi::signal_data)
		.function("breakpoint_signal_list", &DmsApi::breakpoint_signal_list)
		.function("breakpoint_signal_set", &DmsApi::breakpoint_signal_set)
		.function("breakpoint_signal_clear", &DmsApi::breakpoint_signal_clear)
		.function("display_info", &DmsApi::display_info)
		.function("signal_info", &DmsApi::signal_info)
		.function("signal_details", &DmsApi::signal_details)
		.function("cpu_info", &DmsApi::cpu_info)
		.function("clock_info", &DmsApi::clock_info)
		;
}
