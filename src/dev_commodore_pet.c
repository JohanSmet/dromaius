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

static inline void activate_reset(DevCommodorePet *device, bool reset) {
	device->in_reset = reset;
	SIGNAL_SET_BOOL(reset_b, !device->in_reset);
	SIGNAL_SET_BOOL(init_b, !device->in_reset);
	SIGNAL_SET_BOOL(init, device->in_reset);
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
	device->destroy = (DEVICE_DESTROY) dev_commodore_pet_destroy;

	// signals
	device->signal_pool = signal_pool_create(2);

	signal_pool_current_domain(device->signal_pool, PET_DOMAIN_CLOCK);
	SIGNAL_DEFINE_BOOL_N(init_b, 1, ACTLO_DEASSERT, "/INIT");
	SIGNAL_DEFINE_BOOL_N(init, 1, ACTHI_DEASSERT, "INIT");

	SIGNAL_DEFINE_BOOL_N(clk16, 1, true, "CLK16");
	SIGNAL_DEFINE_BOOL_N(clk8, 1, true, "CLK8");
	SIGNAL_DEFINE_BOOL_N(clk4, 1, true, "CLK4");
	SIGNAL_DEFINE_BOOL_N(clk2, 1, true, "CLK2");
	SIGNAL_DEFINE_BOOL_N(clk1, 1, true, "CLK1");

	SIGNAL_DEFINE_N(bphi2a, 1, "BPHI2A");
	SIGNAL_DEFINE_N(bphi2b, 1, "BPHI2B");
	SIGNAL_DEFINE_N(bphi2c, 1, "BPHI2C");
	SIGNAL_DEFINE_N(bphi2d, 1, "BPHI2D");
	SIGNAL_DEFINE_N(bphi2e, 1, "BPHI2E");
	SIGNAL_DEFINE_N(bphi2f, 1, "BPHI2F");
	SIGNAL_DEFINE_N(bphi2g, 1, "BPHI2G");
	SIGNAL_DEFINE_N(bphi2h, 1, "BPHI2H");

	SIGNAL_DEFINE_N(bphi2a_b, 1, "/BPHI2A");
	SIGNAL_DEFINE_N(bphi2b_b, 1, "/BPHI2B");
	SIGNAL_DEFINE_N(bphi2f_b, 1, "/BPHI2F");
	SIGNAL_DEFINE_N(bphi2g_b, 1, "/BPHI2G");

	SIGNAL_DEFINE_N(ra1, 1, "RA1");
	SIGNAL_DEFINE_N(ra2, 1, "RA2");
	SIGNAL_DEFINE_N(ra3, 1, "RA3");
	SIGNAL_DEFINE_N(ra4, 1, "RA4");
	SIGNAL_DEFINE_N(ra5, 1, "RA5");
	SIGNAL_DEFINE_N(ra6, 1, "RA6");

	SIGNAL_DEFINE_N(ra1_b, 1, "/RA1");
	SIGNAL_DEFINE_N(ra6_b, 1, "/RA6");

	signal_pool_current_domain(device->signal_pool, PET_DOMAIN_1MHZ);
	SIGNAL_DEFINE_BOOL_N(reset_b, 1, ACTLO_DEASSERT, "/RES");
	SIGNAL_DEFINE_BOOL_N(irq_b, 1, ACTLO_DEASSERT, "/IRQ");
	SIGNAL_DEFINE_BOOL_N(nmi_b, 1, ACTLO_DEASSERT, "/NMI");

	SIGNAL_DEFINE_N(cpu_bus_address, 16, "AB%d");
	SIGNAL_DEFINE_N(cpu_bus_data, 8, "D%d");
	SIGNAL_DEFINE_BOOL_N(cpu_rw, 1, true, "RW");
	SIGNAL_DEFINE_N(cpu_sync, 1, "SYNC");
	SIGNAL_DEFINE_BOOL_N(cpu_rdy, 1, ACTHI_ASSERT, "RDY");

	SIGNAL_DEFINE_N(bus_ba, 16, "BA%d");
	SIGNAL_DEFINE_N(bus_bd, 8, "BD%d");

	SIGNAL_DEFINE_N(sel2_b, 1, "/SEL2");
	SIGNAL_DEFINE_N(sel3_b, 1, "/SEL3");
	SIGNAL_DEFINE_N(sel4_b, 1, "/SEL4");
	SIGNAL_DEFINE_N(sel5_b, 1, "/SEL5");
	SIGNAL_DEFINE_N(sel6_b, 1, "/SEL6");
	SIGNAL_DEFINE_N(sel7_b, 1, "/SEL7");
	SIGNAL_DEFINE_N(sel8_b, 1, "/SEL8");
	SIGNAL_DEFINE_N(sel9_b, 1, "/SEL9");
	SIGNAL_DEFINE_N(sela_b, 1, "/SELA");
	SIGNAL_DEFINE_N(selb_b, 1, "/SELB");
	SIGNAL_DEFINE_N(selc_b, 1, "/SELC");
	SIGNAL_DEFINE_N(seld_b, 1, "/SELD");
	SIGNAL_DEFINE_N(sele_b, 1, "/SELE");
	SIGNAL_DEFINE_N(self_b, 1, "/SELF");

	SIGNAL_DEFINE_N(sel8, 1, "SEL8");

	SIGNAL_DEFINE_N(x8xx, 1, "X8XX");
	SIGNAL_DEFINE_N(s_88xx_b, 1, "/88XX");
	SIGNAL_DEFINE_N(rom_addr_b, 1, "/ROMA");

	SIGNAL_DEFINE_N(ram_read_b, 1, "/RAMR");
	SIGNAL_DEFINE_N(ram_write_b, 1, "/RAMW");

	SIGNAL_DEFINE_N(phi2, 1, "PHI2");
	SIGNAL_DEFINE_N(bphi2, 1, "BPHI2");
	SIGNAL_DEFINE_N(cphi2, 1, "CPHI2");

	SIGNAL_DEFINE_N(buf_rw, 1, "BRW");
	SIGNAL_DEFINE_N(buf_rw_b, 1, "/BRW");
	SIGNAL_DEFINE_N(ram_rw, 1, "RAMRW");

	SIGNAL_DEFINE_BOOL_N(high, 1, true, "VCC");
	SIGNAL_DEFINE_BOOL_N(low, 1, false, "GND");

	SIGNAL_DEFINE(cs1, 1);

	SIGNAL_DEFINE_BOOL_N(video_on, 1, true, "VIDEOON");

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

	SIGNAL_DEFINE_N(ba11_b, 1, "/BA11");
	SIGNAL_DEFINE_N(reset, 1, "RES");

	signal_pool_current_domain(device->signal_pool, PET_DOMAIN_CLOCK);

	// clock
	device->clock = clock_create(100000, device->signal_pool, (ClockSignals){
										.clock = SIGNAL(clk16)
	});

	device->g5 = chip_74191_binary_counter_create(device->signal_pool, (Chip74191Signals) {
										.vcc = SIGNAL(high),			// pin 16
										.gnd = SIGNAL(low),				// pin 08
										.enable_b = SIGNAL(low),		// pin 04
										.d_u = SIGNAL(low),				// pin 05
										.a = SIGNAL(low),				// pin 15
										.b = SIGNAL(low),				// pin 01
										.c = SIGNAL(low),				// pin 10
										.d = SIGNAL(low),				// pin 09
										.load_b = SIGNAL(high),			// pin 11 - FIXME: should be connected to the /INIT signal
										.clk = SIGNAL(clk16),			// pin 14
										.qa = SIGNAL(clk8),				// pin 03
										.qb = SIGNAL(clk4),				// pin 02
										.qc = SIGNAL(clk2),				// pin 06
										.qd = SIGNAL(clk1),				// pin 07
								//		.max_min = not connected        // pin 12
								//		.rco_b = not connected          // pin 13
	});

	device->h3 = chip_74164_shift_register_create(device->signal_pool, (Chip74164Signals) {
										.a = SIGNAL(clk1),				// pin 01
										.b = SIGNAL(high),				// pin 02
										.clk = SIGNAL(clk16),			// pin 08
										.clear_b = SIGNAL(high),		// pin 09
										.qa = SIGNAL(bphi2a),			// pin 03
										.qb = SIGNAL(bphi2b),			// pin 04
										.qc = SIGNAL(bphi2c),			// pin 05
										.qd = SIGNAL(bphi2d),			// pin 06
										.qe = SIGNAL(bphi2e),			// pin 10
										.qf = SIGNAL(bphi2f),			// pin 11
										.qg = SIGNAL(bphi2g),			// pin 12
										.qh = SIGNAL(bphi2h),			// pin 13
										.gnd = SIGNAL(low),				// pin 07
										.vcc = SIGNAL(high)				// pin 14
	});

	device->h6 = chip_74107_jk_flipflop_create(device->signal_pool, (Chip74107Signals) {
										.gnd = SIGNAL(low),				// pin 7
										.vcc = SIGNAL(high),			// pin 14

										.clr1_b = SIGNAL(init_b),		// pin 13
										.clk1 = SIGNAL(bphi2a_b),		// pin 12
										.j1 = SIGNAL(init_b),			// pin 1
										.k1 = SIGNAL(init_b),			// pin 4
										.q1 = SIGNAL(ra1),				// pin 3
										.q1_b = SIGNAL(ra1_b),			// pin 2

										.clr2_b = SIGNAL(init_b),		// pin 10
										.clk2 = SIGNAL(ra5),			// pin 9
										.j2 = SIGNAL(init_b),			// pin 8
										.k2 = SIGNAL(init_b),			// pin 11
										.q2 = SIGNAL(ra6),				// pin 5
										.q2_b = SIGNAL(ra6_b)			// pin 6
	});


	signal_pool_current_domain(device->signal_pool, PET_DOMAIN_1MHZ);

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

	chip_74191_binary_counter_destroy(device->g5);
	chip_74164_shift_register_destroy(device->h3);
	chip_74107_jk_flipflop_destroy(device->h6);

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

static inline void process_glue_logic(DevCommodorePet *device, bool video_on) {

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

	// reset circuit
	SIGNAL_SET_BOOL(reset_b, !device->in_reset);
	SIGNAL_SET_BOOL(init_b, !device->in_reset);
	SIGNAL_SET_BOOL(init, device->in_reset);

	// A3 (1, 2)
	bool reset_b = SIGNAL_NEXT_BOOL(reset_b);
	SIGNAL_SET_BOOL(reset, !reset_b);

	// A3 (11, 10)
	bool sel8 = !sel8_b;
	SIGNAL_SET_BOOL(sel8, sel8);

	// A3 (3, 4)
	bool ba11_b = !ba11;
	SIGNAL_SET_BOOL(ba11_b, ba11_b);

	// B2 (1,2,4,5,6)
	bool addr_x8xx = !(ba8 | ba9 | ba10 | ba11_b);
	SIGNAL_SET_BOOL(x8xx, addr_x8xx);

	// A4 (4,5,6)
	bool addr_88xx_b = NAND(sel8, addr_x8xx);
	SIGNAL_SET_BOOL(s_88xx_b, addr_88xx_b);

	// A4 (1,2,3)
	bool rom_addr_b = NAND(ba15, sel8_b);
	SIGNAL_SET_BOOL(rom_addr_b, rom_addr_b);

	// A5 (3,4,5,6)
	bool ram_read_b = !(rom_addr_b && addr_88xx_b && rw);
	SIGNAL_SET_BOOL(ram_read_b, ram_read_b);
	SIGNAL_SET_BOOL(ram_write_b, !ram_read_b);

	// A10 (3,4)
	SIGNAL_SET_BOOL(buf_rw, rw);

	// A3 (12,13)
	SIGNAL_SET_BOOL(buf_rw_b, !rw);

	// A3 (9,8)
	SIGNAL_SET_BOOL(ram_rw, rw);

	// FIXME: cpu doesn't output phi2 clock signal
	bool phi2 = SIGNAL_BOOL(clk1);
	SIGNAL_SET_BOOL(phi2, phi2);
	SIGNAL_SET_BOOL(bphi2, phi2);
	SIGNAL_SET_BOOL(cphi2, phi2);

	chip_74244_octal_buffer_process(device->e9);
	chip_74244_octal_buffer_process(device->e10);

	// >> ram logic - simplified from schematic because were not simulating dram (4116 chips) yet.
	//  - ce_b: assert when top bit of address isn't set (copy of ba15)
	//	- oe_b: assert when cpu_rw is high
	//	- we_b: assert when cpu_rw is low and clock is low
	bool next_rw = SIGNAL_NEXT_BOOL(cpu_rw);
	SIGNAL_SET_BOOL(ram_oe_b, !next_rw);
	SIGNAL_SET_BOOL(ram_we_b, next_rw || !SIGNAL_BOOL(clk1));

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

static inline void process_master_timing(DevCommodorePet *device) {

	// cycle signal domain
	signal_pool_cycle_domain(device->signal_pool, PET_DOMAIN_CLOCK);

	// >> 16Mhz oscillator (no dependencies)
	clock_process(device->clock);

	// >> clock divider (depends on clk16)
	chip_74191_binary_counter_process(device->g5);

	// >> clk phase splitter (depends on clk1 and clk16)
	chip_74164_shift_register_process(device->h3);

	// >> invert clock phases (depends on h3)
	SIGNAL_SET_BOOL(bphi2a_b, !SIGNAL_NEXT_BOOL(bphi2a));
	SIGNAL_SET_BOOL(bphi2b_b, !SIGNAL_NEXT_BOOL(bphi2b));
	SIGNAL_SET_BOOL(bphi2f_b, !SIGNAL_NEXT_BOOL(bphi2f));
	SIGNAL_SET_BOOL(bphi2g_b, !SIGNAL_NEXT_BOOL(bphi2g));

	// jk-flip flop (depends on /BPHI2A)
	chip_74107_jk_flipflop_process(device->h6);
}

void dev_commodore_pet_process(DevCommodorePet *device) {
	assert(device);

	// master timing (schematic sheet 6)
	process_master_timing(device);

	if (signal_changed(device->signal_pool, SIGNAL(clk1))) {
		// vblank 'fake' logic
		bool video_on = true;
		device->vblank_counter += SIGNAL_BOOL(clk1);

		if (device->vblank_counter >= 16666)  {		// about 60 hz if clock is running at the normal 1Mhz
			device->vblank_counter = 0;
			video_on = false;
		}

		// run all chips on the clock cycle
		signal_pool_cycle_domain(device->signal_pool, PET_DOMAIN_1MHZ);

		// cpu
		device->cpu->process(device->cpu, false);

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

		// >> glue logic
		process_glue_logic(device, video_on);

		// run the cpu again on the same clock cycle - after the address hold time
		signal_pool_cycle_domain_no_reset(device->signal_pool, PET_DOMAIN_1MHZ);

		// >> cpu
		device->cpu->process(device->cpu, true);

		// >> glue logic
		process_glue_logic(device, video_on);

		// video out
		if ((device->vblank_counter & 1) == 1) {
			dev_commodore_pet_fake_display(device);
		}
	}
}

void dev_commodore_pet_process_clk1(DevCommodorePet *device) {
	do {
		dev_commodore_pet_process(device);
	} while (!signal_changed(device->signal_pool, SIGNAL(clk1)));
}

void dev_commodore_pet_reset(DevCommodorePet *device) {

	// run for a few cycles while reset is asserted
	activate_reset(device, true);

	for (int i = 0; i < 32; ++i) {
		dev_commodore_pet_process(device);
	}

	// run CPU init cycle
	activate_reset(device, false);

	do {
		dev_commodore_pet_process(device);
	} while (cpu_6502_in_initialization(device->cpu));

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
