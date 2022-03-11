// htmlview/api/api.cpp - Johan Smet - BSD-3-Clause (see LICENSE)

#include <emscripten/bind.h>
#include <cassert>
#include <vector>
#include <map>

#include "context.h"
#include "cpu_6502.h"
#include "dev_commodore_pet.h"
#include "perif_pet_crt.h"
#include "perif_datassette_1530.h"
#include "perif_disk_2031.h"
#include "input_keypad.h"

#include "stb/stb_ds.h"

using namespace emscripten;

class DmsApi {
public:
	struct DisplayInfo {
		size_t width;
		size_t height;
	};

	using SignalSubscript = int32_t;

	struct SignalInfo {
		size_t									count;
		std::map<std::string, SignalSubscript>	names;
	};

	struct SignalDetails {
		SignalSubscript	signal;
		int32_t			value;
		int32_t			writer_id;
		std::string		writer_name;
	};

	struct ClockInfo {
		uint32_t current_tick_low;
		uint32_t current_tick_high;
		double time_elapsed_ns;
	};

	struct KeyInfo {
		uint32_t		row;
		uint32_t		column;
	};

public:
	DmsApi() = default;

	// device control
	void launch_commodore_pet(bool lite) {
		// create pet
		if (!lite) {
			pet_device = dev_commodore_pet_create();
		} else {
			pet_device = dev_commodore_pet_lite_create();
		}
		assert(pet_device);

		// create dromaius context
		dms_ctx = dms_create_context();
		assert(dms_ctx);
		dms_set_device(dms_ctx, reinterpret_cast<Device *>(pet_device));

		// default clock to step
		step_clock = pet_device->signals[SIG_P2001N_CLK1];

		// launch background thread
		#ifndef DMS_NO_THREADING
			dms_start_execution(dms_ctx);
		#endif // DMS_NO_THREADING
	}

	void stop_emulation() {
		// stop background thread
		#ifndef DMS_NO_THREADING
			dms_stop_execution(dms_ctx);
		#endif

		// destroy context
		if (dms_ctx) {
			dms_release_context(dms_ctx);
			dms_ctx = nullptr;
		}

		// destroy device
		if (pet_device) {
			pet_device->destroy(pet_device);
			pet_device = nullptr;
		}
	}

	// dromaius context
	bool context_execute() {
		assert(dms_ctx);

#ifdef DMS_NO_THREADING
		int64_t save_ts = pet_device->simulator->current_tick;

		dms_execute(dms_ctx);

		return save_ts != pet_device->simulator->current_tick;
#else
		return true;
#endif
	}

	void context_select_step_clock(const std::string &signal_name) {
		auto signal = signal_by_name(pet_device->simulator->signal_pool, signal_name.c_str());
		if (!signal_is_undefined(signal)) {
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
		dms_step_signal(dms_ctx, pet_device->signals[SIG_P2001N_SYNC], true, false);
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

	std::string context_status() {
		assert(dms_ctx);

		static constexpr const char *state_strings[] = {"wait", "single_step", "step_signal", "run"};

		auto state = dms_get_state(dms_ctx);

		if (state < 4) {
			return state_strings[state];
		} else {
			return "exiting";
		}
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

	int32_t keyboard_num_keys_down() const {
		assert(pet_device);
		return static_cast<int32_t>(input_keypad_keys_down_count(pet_device->keypad));
	}

	void keyboard_set_dwell_time(int32_t key_dwell_ms) const {
		assert(pet_device);
		input_keypad_set_dwell_time_ms(pet_device->keypad, key_dwell_ms);
	}

	// datassette control
	void datassette_load_tap(std::string tap_data) {
		perif_datassette_load_tap_from_memory(
				pet_device->datassette,
				reinterpret_cast<const int8_t *> (tap_data.c_str()),
				tap_data.size());
	}

	std::string datassette_status() {
		static constexpr const char *ds_state_strings[] = {"Idle", "Tape Loaded", "Playing", "Recording", "Rewinding", "Fast-Forwarding"};
		return ds_state_strings[pet_device->datassette->state];
	}

	void datassette_record() {
		perif_datassette_key_pressed(pet_device->datassette, DS_KEY_RECORD);
	}

	void datassette_play() {
		perif_datassette_key_pressed(pet_device->datassette, DS_KEY_PLAY);
	}

	void datassette_rewind() {
		perif_datassette_key_pressed(pet_device->datassette, DS_KEY_REWIND);
	}

	void datassette_fast_forward() {
		perif_datassette_key_pressed(pet_device->datassette, DS_KEY_FFWD);
	}

	void datassette_stop() {
		perif_datassette_key_pressed(pet_device->datassette, DS_KEY_STOP);
	}

	void datassette_eject() {
		perif_datassette_key_pressed(pet_device->datassette, DS_KEY_EJECT);
	}

	int datassette_valid_buttons() {
		return pet_device->datassette->valid_keys;
	}

	float datassette_position() {
		return PS_TO_S(pet_device->datassette->offset_ps);
	}

	// disk 2031 control
	void disk2031_load_d64(std::string d64_data) {
		perif_fd2031_load_d64_from_memory(
				pet_device->disk_2031,
				reinterpret_cast<const int8_t *>(d64_data.c_str()),
				d64_data.size());
	}

	void disk2031_set_address(int address) {
		pet_device->disk_2031->address = address;
	}

	// data access
	val display_data() const {
		assert(pet_device);
		return val(typed_memory_view(pet_device->screen->width * pet_device->screen->height * 4,
									 reinterpret_cast<uint8_t *> (pet_device->screen->frame)));
	}

	val signal_data() {
		assert(pet_device);
		auto pool = pet_device->simulator->signal_pool;

		signal_data_buffer.clear();

		for (uint32_t blk = 0; blk < SIGNAL_BLOCKS; ++blk) {
			for (int i = 0; i < SIGNAL_BLOCK_SIZE; ++i) {
				uint64_t signal_mask = 1ull << i;
				signal_data_buffer.push_back(FLAG_IS_SET(pool->signals_value[blk], signal_mask));
			}
		}

		return val(typed_memory_view(pool->signals_count, signal_data_buffer.data()));
	}

	// breakpoints
	std::vector<std::string> breakpoint_signal_list() const {
		std::vector<std::string>	result;

		auto bps = dms_breakpoint_signal_list(dms_ctx);
		auto pool = pet_device->simulator->signal_pool;

		for (ptrdiff_t idx = 0; idx < arrlen(bps); ++idx) {
			result.push_back(signal_get_name(pool, bps[idx].signal));
		}

		return result;
	}

	void breakpoint_signal_set(const std::string &signal_name) {
		auto signal = signal_by_name(pet_device->simulator->signal_pool, signal_name.c_str());
		if (!signal_is_undefined(signal)) {
			dms_breakpoint_signal_set(dms_ctx, signal, true, true);
		}
	}

	void breakpoint_signal_clear(const std::string &signal_name) {
		auto signal = signal_by_name(pet_device->simulator->signal_pool, signal_name.c_str());
		if (!signal_is_undefined(signal)) {
			dms_breakpoint_signal_clear(dms_ctx, signal);
		}
	}

	// hardware info
	DisplayInfo display_info() {
		assert(pet_device);
		return DisplayInfo{pet_device->screen->width, pet_device->screen->height};
	}

	SignalInfo signal_info() {
		assert(pet_device);
		auto pool = pet_device->simulator->signal_pool;
		SignalInfo	result;

		result.count = 1;

		for (int s = 0; s < shlen(pool->signal_names); ++s) {
			std::string name = pool->signal_names[s].key;
			auto &signal = pool->signal_names[s].value;
			result.names[name] = signal_array_subscript(signal);
		}

		return result;
	}

	SignalDetails signal_details(std::string name) {
		assert(pet_device);
		auto pool = pet_device->simulator->signal_pool;
		SignalDetails result;

		auto signal = shget(pool->signal_names, name.c_str());
		result.signal = signal_array_subscript(signal);
		result.value = 0;
		result.writer_id = -1;

		if (!signal_is_undefined(signal)) {
			// writer (FIXME: return multiple writers)
			result.writer_id = bit_lowest_set(simulator_signal_writers(pet_device->simulator, signal));
			result.writer_name = simulator_chip_name(pet_device->simulator, result.writer_id);

			// value
			result.value = signal_read_next(pet_device->simulator->signal_pool, signal);
		}

		return result;
	}

	Cpu6502 cpu_info() {
		assert(pet_device);
		return *pet_device->cpu;
	}

	ClockInfo clock_info() {
		return (ClockInfo) {
			static_cast<uint32_t>(pet_device->simulator->current_tick & 0xffffffff),
			static_cast<uint32_t>((pet_device->simulator->current_tick >> 32) & 0xffffffff),
			pet_device->simulator->current_tick * pet_device->simulator->tick_duration_ps / 1000.0
		};
	}

private:
	DmsContext *		dms_ctx = nullptr;
	DevCommodorePet *	pet_device = nullptr;
	Signal				step_clock;
	std::vector<uint8_t>	signal_data_buffer;
};


// binding code
EMSCRIPTEN_BINDINGS(DmsApiBindings) {

	register_vector<size_t>("VectorSize");
	register_vector<std::string>("VectorString");
	register_vector<DmsApi::KeyInfo>("VectorKeyInfo");
	register_map<std::string, DmsApi::SignalSubscript>("MapStringSignal");

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
		.field("current_tick_low", &DmsApi::ClockInfo::current_tick_low)
		.field("current_tick_high", &DmsApi::ClockInfo::current_tick_high)
		.field("time_elapsed_ns", &DmsApi::ClockInfo::time_elapsed_ns)
		;

	value_object<DmsApi::KeyInfo>("DmsApi__KeyInfo")
		.field("row", &DmsApi::KeyInfo::row)
		.field("column", &DmsApi::KeyInfo::column)
		;

	class_<DmsApi>("DmsApi")
		.constructor()
		.function("launch_commodore_pet", &DmsApi::launch_commodore_pet)
		.function("stop_emulation", &DmsApi::stop_emulation)
		.function("context_execute", &DmsApi::context_execute)
		.function("context_select_step_clock", &DmsApi::context_select_step_clock)
		.function("context_single_step", &DmsApi::context_single_step)
		.function("context_step_clock", &DmsApi::context_step_clock)
		.function("context_step_instruction", &DmsApi::context_step_instruction)
		.function("context_run", &DmsApi::context_run)
		.function("context_pause", &DmsApi::context_pause)
		.function("context_reset", &DmsApi::context_reset)
		.function("context_status", &DmsApi::context_status)
		.function("keyboard_num_rows", &DmsApi::keyboard_num_rows)
		.function("keyboard_num_columns", &DmsApi::keyboard_num_columns)
		.function("keyboard_key_pressed", &DmsApi::keyboard_key_pressed)
		.function("keyboard_keys_down", &DmsApi::keyboard_keys_down)
		.function("keyboard_num_keys_down", &DmsApi::keyboard_num_keys_down)
		.function("keyboard_set_dwell_time", &DmsApi::keyboard_set_dwell_time)
		.function("datassette_load_tap", &DmsApi::datassette_load_tap)
		.function("datassette_status", &DmsApi::datassette_status)
		.function("datassette_record", &DmsApi::datassette_record)
		.function("datassette_play", &DmsApi::datassette_play)
		.function("datassette_rewind", &DmsApi::datassette_rewind)
		.function("datassette_fast_forward", &DmsApi::datassette_fast_forward)
		.function("datassette_stop", &DmsApi::datassette_stop)
		.function("datassette_eject", &DmsApi::datassette_eject)
		.function("datassette_valid_buttons", &DmsApi::datassette_valid_buttons)
		.function("datassette_position", &DmsApi::datassette_position)
		.function("disk2031_load_d64", &DmsApi::disk2031_load_d64)
		.function("disk2031_set_address", &DmsApi::disk2031_set_address)
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
