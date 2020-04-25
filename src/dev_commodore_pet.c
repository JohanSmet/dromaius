// dev_commodore_pet.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a Commodore PET (Universal Dynamic motherboard)

#include "dev_commodore_pet.h"

#include "utils.h"
#include "chip_6520.h"
#include "chip_6522.h"
#include "input_keypad.h"
#include "display_rgba.h"
#include "stb/stb_ds.h"

#include <assert.h>
#include <stdlib.h>

#define SIGNAL_POOL			device->signal_pool
#define SIGNAL_COLLECTION	device->signals

static Rom8d16a *load_rom(DevCommodorePet *device, const char *filename, uint32_t num_lines, Signal rom_ce_b) {
	Rom8d16a *rom = rom_8d16a_create(num_lines, device->signal_pool, (Rom8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_ba, 0, num_lines),
										.bus_data = SIGNAL(cpu_bus_data),
										.ce_b = rom_ce_b
	});

	if (file_load_binary_fixed(filename, rom->data_array, rom->data_size) == 0) {
		rom_8d16a_destroy(rom);
		return NULL;
	}

	return rom;
}

static Rom8d16a *load_character_rom(DevCommodorePet *device, const char *filename, uint32_t num_lines) {
	Rom8d16a *rom = rom_8d16a_create(num_lines, device->signal_pool, (Rom8d16aSignals) {
										.ce_b = SIGNAL(low)
	});

	if (file_load_binary_fixed(filename, rom->data_array, rom->data_size) == 0) {
		rom_8d16a_destroy(rom);
		return NULL;
	}

	return rom;
}


Cpu6502* dev_commodore_pet_get_cpu(DevCommodorePet *device) {
	assert(device);
	return device->cpu;
}

Clock* dev_commodore_pet_get_clock(DevCommodorePet *device) {
	assert(device);
	return device->clock;
}

DevCommodorePet *dev_commodore_pet_create() {
	DevCommodorePet *device = (DevCommodorePet *) calloc(1, sizeof(DevCommodorePet));

	// interface
	device->get_cpu = (DEVICE_GET_CPU) dev_commodore_pet_get_cpu;
	device->get_clock = (DEVICE_GET_CLOCK) dev_commodore_pet_get_clock;
	device->process = (DEVICE_PROCESS) dev_commodore_pet_process;
	device->reset = (DEVICE_RESET) dev_commodore_pet_reset;

	// signals
	device->signal_pool = signal_pool_create();

	SIGNAL_DEFINE_BOOL(reset_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(irq_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(nmi_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(clk1, 1, true);

	SIGNAL_DEFINE(cpu_bus_address, 16);
	SIGNAL_DEFINE(cpu_bus_data, 8);
	SIGNAL_DEFINE_BOOL(cpu_rw, 1, true);
	SIGNAL_DEFINE(cpu_sync, 1);
	SIGNAL_DEFINE_BOOL(cpu_rdy, 1, ACTHI_ASSERT);

	SIGNAL_DEFINE(bus_ba, 16);
	SIGNAL_DEFINE(bus_bd, 8);

	SIGNAL_DEFINE(sel2_b, 1);
	SIGNAL_DEFINE(sel3_b, 1);
	SIGNAL_DEFINE(sel4_b, 1);
	SIGNAL_DEFINE(sel5_b, 1);
	SIGNAL_DEFINE(sel6_b, 1);
	SIGNAL_DEFINE(sel7_b, 1);
	SIGNAL_DEFINE(sel8_b, 1);
	SIGNAL_DEFINE(sel9_b, 1);
	SIGNAL_DEFINE(sela_b, 1);
	SIGNAL_DEFINE(selb_b, 1);
	SIGNAL_DEFINE(selc_b, 1);
	SIGNAL_DEFINE(seld_b, 1);
	SIGNAL_DEFINE(sele_b, 1);
	SIGNAL_DEFINE(self_b, 1);

	SIGNAL_DEFINE(x8xx, 1);
	SIGNAL_DEFINE(ram_read_b, 1);
	SIGNAL_DEFINE(ram_write_b, 1);

	SIGNAL_DEFINE_BOOL(high, 1, true);
	SIGNAL_DEFINE_BOOL(low, 1, false);

	SIGNAL_DEFINE(cs1, 1);

	SIGNAL_DEFINE_BOOL(video_on, 1, true);

	device->signals.ba6 = signal_split(SIGNAL(bus_ba), 6, 1);
	device->signals.ba7 = signal_split(SIGNAL(bus_ba), 7, 1);
	device->signals.ba8 = signal_split(SIGNAL(bus_ba), 8, 1);
	device->signals.ba9 = signal_split(SIGNAL(bus_ba), 9, 1);
	device->signals.ba10 = signal_split(SIGNAL(bus_ba), 10, 1);
	device->signals.ba11 = signal_split(SIGNAL(bus_ba), 11, 1);
	device->signals.ba12 = signal_split(SIGNAL(bus_ba), 12, 1);
	device->signals.ba13 = signal_split(SIGNAL(bus_ba), 13, 1);
	device->signals.ba14 = signal_split(SIGNAL(bus_ba), 14, 1);
	device->signals.ba15 = signal_split(SIGNAL(bus_ba), 15, 1);

	// clock
	device->clock = clock_create(100000, device->signal_pool, (ClockSignals){
										.clock = SIGNAL(clk1)
	});

	// cpu
	device->cpu = cpu_6502_create(device->signal_pool, (Cpu6502Signals) {
										.bus_address = SIGNAL(cpu_bus_address),
										.bus_data = SIGNAL(cpu_bus_data),
										.clock = SIGNAL(clk1),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.irq_b = SIGNAL(irq_b),
										.nmi_b = SIGNAL(nmi_b),
										.sync = SIGNAL(cpu_sync),
										.rdy = SIGNAL(cpu_rdy)
	});

	// ram
	device->ram = ram_8d16a_create(15, device->signal_pool, (Ram8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_ba, 0, 15),
										.bus_data = SIGNAL(bus_bd),
										.ce_b = SIGNAL(ba15)
	});

	SIGNAL(ram_oe_b) = device->ram->signals.oe_b;
	SIGNAL(ram_we_b) = device->ram->signals.we_b;

	// video ram
	device->vram = ram_8d16a_create(10, device->signal_pool, (Ram8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_ba, 0, 10),
										.bus_data = SIGNAL(bus_bd),
										.ce_b = SIGNAL(sel8_b),
										.we_b = SIGNAL(ram_we_b)
	});

	SIGNAL(vram_oe_b) = device->vram->signals.oe_b;

	// basic roms
	int rom_count = 0;
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-b000.901465-19.bin", 12, SIGNAL(selb_b));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-c000.901465-20.bin", 12, SIGNAL(selc_b));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-d000.901465-21.bin", 12, SIGNAL(seld_b));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/edit-4-n.901447-29.bin", 11, (Signal) {0});
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/kernal-4.901465-22.bin", 12, SIGNAL(self_b));

	for (int i = 0; i < rom_count; ++i) {
		assert(device->roms[i]);
	}

	device->char_rom = load_character_rom(device, "runtime/commodore_pet/characters-2.901447-10.bin", 11);
	assert(device->char_rom);

	SIGNAL(rome_ce_b) = device->roms[3]->signals.ce_b;

	// pia 1 (C6 - IEEE-488 interface)
	device->pia_1 = chip_6520_create(device->signal_pool, (Chip6520Signals) {
										.bus_data = SIGNAL(cpu_bus_data),
										.enable = SIGNAL(clk1),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.cs0 = SIGNAL(x8xx),
										.cs1 = signal_split(SIGNAL(bus_ba), 5, 1),
										.cs2_b = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										.rs0 = signal_split(SIGNAL(bus_ba), 0, 1),
										.rs1 = signal_split(SIGNAL(bus_ba), 1, 1),
	});

	// pia 2 (C7 - keyboard)
	device->pia_2 = chip_6520_create(device->signal_pool, (Chip6520Signals) {
										.bus_data = SIGNAL(cpu_bus_data),
										.enable = SIGNAL(clk1),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.cs0 = SIGNAL(x8xx),
										.cs1 = signal_split(SIGNAL(bus_ba), 4, 1),
										.cs2_b = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										.rs0 = signal_split(SIGNAL(bus_ba), 0, 1),
										.rs1 = signal_split(SIGNAL(bus_ba), 1, 1),
										.cb1 = SIGNAL(video_on)
	});
	signal_default_uint8(device->signal_pool, device->pia_2->signals.port_a, 0xff);			// temporary until keyboard connected
	signal_default_uint8(device->signal_pool, device->pia_2->signals.port_b, 0xff);			// pull-up resistors R18-R25

	// keyboard
	device->keypad = input_keypad_create(device->signal_pool, false, 10, 8, (InputKeypadSignals) {
										.cols = device->pia_2->signals.port_b
	});

	// via (C5)
	device->via = chip_6522_create(device->signal_pool, (Chip6522Signals) {
										.bus_data = SIGNAL(cpu_bus_data),
										.enable = SIGNAL(clk1),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.cs1 = SIGNAL(cs1),
										.cs2_b = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										.rs0 = signal_split(SIGNAL(bus_ba), 0, 1),
										.rs1 = signal_split(SIGNAL(bus_ba), 1, 1),
										.rs2 = signal_split(SIGNAL(bus_ba), 2, 1),
										.rs3 = signal_split(SIGNAL(bus_ba), 3, 1),
	});

	// display
	device->display = display_rgba_create(40 * 8, 25 * 8);

	// glue logic
	device->c3 = chip_74244_octal_buffer_create(device->signal_pool, (Chip74244Signals) {
										.g1_b = SIGNAL(low),									// 01
										.g2_b = SIGNAL(low),									// 19
										.a11  = signal_split(SIGNAL(cpu_bus_address), 0, 1),	// 02
										.a24  = signal_split(SIGNAL(cpu_bus_address), 1, 1),	// 17
										.a12  = signal_split(SIGNAL(cpu_bus_address), 2, 1),	// 04
										.a23  = signal_split(SIGNAL(cpu_bus_address), 3, 1),	// 15
										.a13  = signal_split(SIGNAL(cpu_bus_address), 4, 1),	// 06
										.a22  = signal_split(SIGNAL(cpu_bus_address), 5, 1),	// 13
										.a14  = signal_split(SIGNAL(cpu_bus_address), 6, 1),	// 08
										.a21  = signal_split(SIGNAL(cpu_bus_address), 7, 1),	// 11
										.y11  = signal_split(SIGNAL(bus_ba), 0, 1),				// 18
										.y24  = signal_split(SIGNAL(bus_ba), 1, 1),				// 03
										.y12  = signal_split(SIGNAL(bus_ba), 2, 1),				// 16
										.y23  = signal_split(SIGNAL(bus_ba), 3, 1),				// 05
										.y13  = signal_split(SIGNAL(bus_ba), 4, 1),				// 14
										.y22  = signal_split(SIGNAL(bus_ba), 5, 1),				// 07
										.y14  = signal_split(SIGNAL(bus_ba), 6, 1),				// 12
										.y21  = signal_split(SIGNAL(bus_ba), 7, 1),				// 09

	});

	device->b3 = chip_74244_octal_buffer_create(device->signal_pool, (Chip74244Signals) {
										.g1_b = SIGNAL(low),									// 01
										.g2_b = SIGNAL(low),									// 19
										.a11  = signal_split(SIGNAL(cpu_bus_address), 8, 1),	// 02
										.a24  = signal_split(SIGNAL(cpu_bus_address), 9, 1),	// 17
										.a12  = signal_split(SIGNAL(cpu_bus_address), 10, 1),	// 04
										.a23  = signal_split(SIGNAL(cpu_bus_address), 11, 1),	// 15
										.a13  = signal_split(SIGNAL(cpu_bus_address), 12, 1),	// 06
										.a22  = signal_split(SIGNAL(cpu_bus_address), 13, 1),	// 13
										.a14  = signal_split(SIGNAL(cpu_bus_address), 14, 1),	// 08
										.a21  = signal_split(SIGNAL(cpu_bus_address), 15, 1),	// 11
										.y11  = signal_split(SIGNAL(bus_ba), 8, 1),				// 18
										.y24  = signal_split(SIGNAL(bus_ba), 9, 1),				// 03
										.y12  = signal_split(SIGNAL(bus_ba), 10, 1),			// 16
										.y23  = signal_split(SIGNAL(bus_ba), 11, 1),			// 05
										.y13  = signal_split(SIGNAL(bus_ba), 12, 1),			// 14
										.y22  = signal_split(SIGNAL(bus_ba), 13, 1),			// 07
										.y14  = signal_split(SIGNAL(bus_ba), 14, 1),			// 12
										.y21  = signal_split(SIGNAL(bus_ba), 15, 1),			// 09

	});

	device->d2 = chip_74154_decoder_create(device->signal_pool, (Chip74154Signals) {
										.g1_b = SIGNAL(low),
										.g2_b = SIGNAL(low),
										.a  = signal_split(SIGNAL(bus_ba), 12, 1),
										.b  = signal_split(SIGNAL(bus_ba), 13, 1),
										.c  = signal_split(SIGNAL(bus_ba), 14, 1),
										.d  = signal_split(SIGNAL(bus_ba), 15, 1),
										.y2_b = SIGNAL(sel2_b),
										.y3_b = SIGNAL(sel3_b),
										.y4_b = SIGNAL(sel4_b),
										.y5_b = SIGNAL(sel5_b),
										.y6_b = SIGNAL(sel6_b),
										.y7_b = SIGNAL(sel7_b),
										.y8_b = SIGNAL(sel8_b),
										.y9_b = SIGNAL(sel9_b),
										.y10_b = SIGNAL(sela_b),
										.y11_b = SIGNAL(selb_b),
										.y12_b = SIGNAL(selc_b),
										.y13_b = SIGNAL(seld_b),
										.y14_b = SIGNAL(sele_b),
										.y15_b = SIGNAL(self_b),
	});

	device->e9 = chip_74244_octal_buffer_create(device->signal_pool, (Chip74244Signals) {
										.g1_b = SIGNAL(ram_write_b),							// 01
										.g2_b = SIGNAL(ram_read_b),								// 19
										.a11  = signal_split(SIGNAL(cpu_bus_data), 0, 1),		// 02
										.y24  = signal_split(SIGNAL(cpu_bus_data), 0, 1),		// 03
										.a12  = signal_split(SIGNAL(cpu_bus_data), 1, 1),		// 04
										.y23  = signal_split(SIGNAL(cpu_bus_data), 1, 1),		// 05
										.a13  = signal_split(SIGNAL(cpu_bus_data), 2, 1),		// 06
										.y22  = signal_split(SIGNAL(cpu_bus_data), 2, 1),		// 07
										.a14  = signal_split(SIGNAL(cpu_bus_data), 3, 1),		// 08
										.y21  = signal_split(SIGNAL(cpu_bus_data), 3, 1),		// 09

										.y11  = signal_split(SIGNAL(bus_bd), 0, 1),				// 18
										.a24  = signal_split(SIGNAL(bus_bd), 0, 1),				// 17
										.y12  = signal_split(SIGNAL(bus_bd), 1, 1),				// 16
										.a23  = signal_split(SIGNAL(bus_bd), 1, 1),				// 15
										.y13  = signal_split(SIGNAL(bus_bd), 2, 1),				// 14
										.a22  = signal_split(SIGNAL(bus_bd), 2, 1),				// 13
										.y14  = signal_split(SIGNAL(bus_bd), 3, 1),				// 12
										.a21  = signal_split(SIGNAL(bus_bd), 3, 1)				// 11
	});

	device->e10 = chip_74244_octal_buffer_create(device->signal_pool, (Chip74244Signals) {
										.g1_b = SIGNAL(ram_write_b),							// 01
										.g2_b = SIGNAL(ram_read_b),								// 19
										.a11  = signal_split(SIGNAL(cpu_bus_data), 4, 1),		// 02
										.y24  = signal_split(SIGNAL(cpu_bus_data), 4, 1),		// 03
										.a12  = signal_split(SIGNAL(cpu_bus_data), 5, 1),		// 04
										.y23  = signal_split(SIGNAL(cpu_bus_data), 5, 1),		// 05
										.a13  = signal_split(SIGNAL(cpu_bus_data), 6, 1),		// 06
										.y22  = signal_split(SIGNAL(cpu_bus_data), 6, 1),		// 07
										.a14  = signal_split(SIGNAL(cpu_bus_data), 7, 1),		// 08
										.y21  = signal_split(SIGNAL(cpu_bus_data), 7, 1),		// 09

										.y11  = signal_split(SIGNAL(bus_bd), 4, 1),				// 18
										.a24  = signal_split(SIGNAL(bus_bd), 4, 1),				// 17
										.y12  = signal_split(SIGNAL(bus_bd), 5, 1),				// 16
										.a23  = signal_split(SIGNAL(bus_bd), 5, 1),				// 15
										.y13  = signal_split(SIGNAL(bus_bd), 6, 1),				// 14
										.a22  = signal_split(SIGNAL(bus_bd), 6, 1),				// 13
										.y14  = signal_split(SIGNAL(bus_bd), 7, 1),				// 12
										.a21  = signal_split(SIGNAL(bus_bd), 7, 1)				// 11
	});

	device->c9 = chip_74145_bcd_decoder_create(device->signal_pool, (Chip74145Signals) {
										.a = signal_split(device->pia_2->signals.port_a, 0, 1),
										.b = signal_split(device->pia_2->signals.port_a, 1, 1),
										.c = signal_split(device->pia_2->signals.port_a, 2, 1),
										.d = signal_split(device->pia_2->signals.port_a, 3, 1),
										.y0_b = signal_split(device->keypad->signals.rows, 0, 1),
										.y1_b = signal_split(device->keypad->signals.rows, 1, 1),
										.y2_b = signal_split(device->keypad->signals.rows, 2, 1),
										.y3_b = signal_split(device->keypad->signals.rows, 3, 1),
										.y4_b = signal_split(device->keypad->signals.rows, 4, 1),
										.y5_b = signal_split(device->keypad->signals.rows, 5, 1),
										.y6_b = signal_split(device->keypad->signals.rows, 6, 1),
										.y7_b = signal_split(device->keypad->signals.rows, 7, 1),
										.y8_b = signal_split(device->keypad->signals.rows, 8, 1),
										.y9_b = signal_split(device->keypad->signals.rows, 9, 1)
	});

	// power on reset
	dev_commodore_pet_reset(device);

	return device;
}

void dev_commodore_pet_destroy(DevCommodorePet *device) {
	assert(device);

	clock_destroy(device->clock);
	cpu_6502_destroy(device->cpu);
	ram_8d16a_destroy(device->ram);
	ram_8d16a_destroy(device->vram);
	chip_6520_destroy(device->pia_1);
	chip_6520_destroy(device->pia_2);
	chip_6522_destroy(device->via);
	display_rgba_destroy(device->display);

	for (int i = 0; device->roms[i] != NULL; ++i) {
		rom_8d16a_destroy(device->roms[i]);
	}

	chip_74244_octal_buffer_destroy(device->c3);
	chip_74244_octal_buffer_destroy(device->b3);
	chip_74244_octal_buffer_destroy(device->e9);
	chip_74244_octal_buffer_destroy(device->e10);
	chip_74154_decoder_destroy(device->d2);
	free(device);
}

#define NAND(a,b)	(!((a)&&(b)))
#define NOR(a,b)	(!((a)||(b)))

void dev_commodore_pet_fake_display(DevCommodorePet *device) {
// 'fake' display routine to test read from character rom + write to display before properly simulating all electronics required.

	static size_t char_x = 0;		// current position on screen
	static size_t char_y = 0;		//	in characters
	static size_t line = 0;			// current line in character (0 - 7)

	#define COLOR 0xff55ff55

	uint8_t value = device->vram->data_array[char_y * 40 + char_x];
	uint8_t invert = (value >> 7) & 0x1;
	value &= 0x7f;

	int rom_addr = value << 3 | (int) (line & 0b111);
	uint8_t line_value = device->char_rom->data_array[rom_addr];
	uint32_t *screen_ptr = device->display->frame + (char_x * 8) + ((char_y * 8 + line) * device->display->width);


	for (int i = 7; i >= 0; --i) {
		*screen_ptr++ = COLOR * (unsigned int) (((line_value >> i) & 0x1) ^ invert) | 0xff000000;
	}

	// update position
	if (++char_x >= 40) {
		char_x = 0;
		if (++line >= 8) {
			line = 0;
			if (++char_y >= 25) {
				char_y = 0;
			}
		}
	}
}

void dev_commodore_pet_process(DevCommodorePet *device) {
	assert(device);

	// clock tick
	clock_process(device->clock);

	// vblank 'fake' logic
	bool video_on = true;
	device->vblank_counter += SIGNAL_NEXT_BOOL(clk1);

	if (device->vblank_counter >= 16666) {		// about 60 hz if clock is running at the normal 1Mhz
		device->vblank_counter = 0;
		video_on = false;
	}

	// run process twice, once at the time of the clock edge and once slightly later (after the address hold time)
	for (int time = 0; time < 2; ++time) {

		// start the next cycle
		signal_pool_cycle(device->signal_pool);

		// cpu
		device->cpu->process(device->cpu, time & 1);

		// ram
		device->ram->process(device->ram);

		// video ram
		device->vram->process(device->vram);

		// roms
		for (int i = 0; device->roms[i] != NULL; ++i) {
			device->roms[i]->process(device->roms[i]);
		}

		// pia-1 / IEEE-488
		chip_6520_process(device->pia_1);

		// pia-2 / keyboard
		chip_6520_process(device->pia_2);

		// via
		chip_6522_process(device->via);

		// keyboard
		input_keypad_process(device->keypad);

		// make the clock output its signal again
		clock_refresh(device->clock);

		// run all the combinatorial logic just before beginning the next cycle, as these values should be available immediatly
		//	NOTE: reading/writing into the same buffer - order matters, mind the dependencies

		// >> cpu & memory expansion
		chip_74244_octal_buffer_process(device->c3);
		chip_74244_octal_buffer_process(device->b3);
		chip_74154_decoder_process(device->d2);

		bool ba6 = SIGNAL_NEXT_BOOL(ba6);
		bool ba8 = SIGNAL_NEXT_BOOL(ba8);
		bool ba9 = SIGNAL_NEXT_BOOL(ba9);
		bool ba10 = SIGNAL_NEXT_BOOL(ba10);
		bool ba11 = SIGNAL_NEXT_BOOL(ba11);
		bool ba15 = SIGNAL_NEXT_BOOL(ba15);
		bool sel8_b = SIGNAL_NEXT_BOOL(sel8_b);
		bool rw   = SIGNAL_NEXT_BOOL(cpu_rw);

		// B2 (1,2,4,5,6)
		bool addr_x8xx = !(ba8 | ba9 | ba10 | !ba11);
		SIGNAL_SET_BOOL(x8xx, addr_x8xx);

		// A4 (4,5,6)
		bool addr_88xx_b = NAND(!sel8_b, addr_x8xx);

		// A4 (1,2,3)
		bool rom_addr_b = NAND(ba15, sel8_b);

		// A5 (3,4,5,6)
		bool ram_read_b = !(rom_addr_b && addr_88xx_b && rw);
		SIGNAL_SET_BOOL(ram_read_b, ram_read_b);
		SIGNAL_SET_BOOL(ram_write_b, !ram_read_b);

		chip_74244_octal_buffer_process(device->e9);
		chip_74244_octal_buffer_process(device->e10);

		// >> ram logic - simplified from schematic because were not simulating dram (4116 chips) yet.
		//  - ce_b: assert when top bit of address isn't set (copy of ba15)
		//	- oe_b: assert when cpu_rw is high
		//	- we_b: assert when cpu_rw is low and clock is low
		bool next_rw = SIGNAL_NEXT_BOOL(cpu_rw);
		SIGNAL_SET_BOOL(ram_oe_b, !next_rw);
		SIGNAL_SET_BOOL(ram_we_b, next_rw || !SIGNAL_NEXT_BOOL(clk1));

		// >> video-ram logic - simplified
		SIGNAL_SET_BOOL(vram_oe_b, !next_rw);

		// >> rom logic: roms are enabled by the selx_b signals
		//	- rom-E (editor) is special because it's only a 2k ram and it uses address line 11 as an extra enable to only
		//	  respond in the lower 2k
		SIGNAL_SET_BOOL(rome_ce_b, SIGNAL_NEXT_BOOL(sele_b) || ba11);

		// >> pia logic
		bool cs1 = addr_x8xx && ba6;
		SIGNAL_SET_BOOL(cs1, cs1);

		// >> video logic
		SIGNAL_SET_BOOL(video_on, video_on);

		// >> irq logic: the 2001N wire-or's the irq lines of the chips and connects them to the cpu
		//		-> connecting the cpu's irq line to all the irq would just make us overwrite the values
		//		-> or the together explicitly
		bool irq_b = signal_read_next_bool(device->signal_pool, device->pia_1->signals.irqa_b) &&
					 signal_read_next_bool(device->signal_pool, device->pia_1->signals.irqb_b) &&
					 signal_read_next_bool(device->signal_pool, device->pia_2->signals.irqa_b) &&
					 signal_read_next_bool(device->signal_pool, device->pia_2->signals.irqb_b) &&
					 signal_read_next_bool(device->signal_pool, device->via->signals.irq_b);
		SIGNAL_SET_BOOL(irq_b, irq_b);

		// >> keyboard logic
		chip_74145_bcd_decoder_process(device->c9);
	}

	if ((device->vblank_counter & 1) == 1) {
		dev_commodore_pet_fake_display(device);
	}
}

void dev_commodore_pet_reset(DevCommodorePet *device) {

	// run for a few cycles while reset is asserted
	for (int i = 0; i < 4; ++i) {
		SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
		dev_commodore_pet_process(device);
	}

	// run CPU init cycle
	for (int i = 0; i < 15; ++i) {
		dev_commodore_pet_process(device);
	}

	// reset clock
	device->clock->cycle_count = 0;

	device->vblank_counter = 0;
}

bool dev_commodore_pet_load_prg(DevCommodorePet* device, const char* filename, bool use_prg_address) {

	int8_t * prg_buffer = NULL;
	size_t prg_size = file_load_binary(filename, &prg_buffer);

	if (prg_size == 0) {
		arrfree(prg_buffer);
		return false;
	}

	uint16_t ram_offset = 0x401;
	if (use_prg_address) {
		ram_offset = *((uint16_t*)prg_buffer);
	}

	memcpy(device->ram->data_array + ram_offset, prg_buffer + 2, prg_size - 2);
	arrfree(prg_buffer);
	return true;
}
