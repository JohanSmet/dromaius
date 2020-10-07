// dev_commodore_pet.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a Commodore PET 2001N

#include "dev_commodore_pet.h"

#include "utils.h"
#include "chip.h"
#include "chip_6520.h"
#include "chip_6522.h"
#include "chip_74xxx.h"
#include "chip_oscillator.h"
#include "chip_poweronreset.h"
#include "chip_ram_static.h"
#include "chip_ram_dynamic.h"
#include "chip_rom.h"
#include "cpu_6502.h"
#include "input_keypad.h"
#include "display_pet_crt.h"
#include "stb/stb_ds.h"

#include "ram_8d_16a.h"

#include <assert.h>
#include <stdlib.h>

#define SIGNAL_POOL			device->simulator->signal_pool
#define SIGNAL_COLLECTION	device->signals
#define SIGNAL_CHIP_ID		chip->id

///////////////////////////////////////////////////////////////////////////////
//
// internal - glue logic
//

typedef struct ChipGlueLogic {
	CHIP_DECLARE_FUNCTIONS

	DevCommodorePet *device;
} ChipGlueLogic;

static void glue_logic_destroy(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_01(ChipGlueLogic *chip);
static void glue_logic_process_01(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_05(ChipGlueLogic *chip);
static void glue_logic_process_05(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_06(ChipGlueLogic *chip);
static void glue_logic_process_06(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_07(ChipGlueLogic *chip);
static void glue_logic_process_07(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_08(ChipGlueLogic *chip);
static void glue_logic_process_08(ChipGlueLogic *chip);

static ChipGlueLogic *glue_logic_create(DevCommodorePet *device, int sheet) {
	ChipGlueLogic *chip = (ChipGlueLogic *) calloc(1, sizeof(ChipGlueLogic));
	chip->device = device;

	if (sheet == 1) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_01, glue_logic_destroy, glue_logic_register_dependencies_01);
	} else if (sheet == 5) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_05, glue_logic_destroy, glue_logic_register_dependencies_05);
	} else if (sheet == 6) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_06, glue_logic_destroy, glue_logic_register_dependencies_06);
	} else if (sheet == 7) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_07, glue_logic_destroy, glue_logic_register_dependencies_07);
	} else if (sheet == 8) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_08, glue_logic_destroy, glue_logic_register_dependencies_08);
	} else {
		assert(false && "bad programmer!");
	}

	return chip;
}

static void glue_logic_destroy(ChipGlueLogic *chip) {
	assert(chip);
	free(chip);
}

// glue-logic: micro-processor / memory expansion

static void glue_logic_register_dependencies_01(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	signal_add_dependency(SIGNAL_POOL, SIGNAL(reset_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(sel8_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(sele_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba6), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba8), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba9), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba10), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba11), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba15), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(cpu_rw), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(clk1), chip->id);

	signal_add_dependency(SIGNAL_POOL, device->pia_1->signals.irqa_b, chip->id);
	signal_add_dependency(SIGNAL_POOL, device->pia_1->signals.irqb_b, chip->id);
	signal_add_dependency(SIGNAL_POOL, device->pia_2->signals.irqa_b, chip->id);
	signal_add_dependency(SIGNAL_POOL, device->pia_2->signals.irqb_b, chip->id);
	signal_add_dependency(SIGNAL_POOL, device->via->signals.irq_b, chip->id);
}

static void glue_logic_process_01(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	bool ba8 = SIGNAL_BOOL(ba8);
	bool ba9 = SIGNAL_BOOL(ba9);
	bool ba10 = SIGNAL_BOOL(ba10);
	bool ba15 = SIGNAL_BOOL(ba15);
	bool sel8_b = SIGNAL_BOOL(sel8_b);
	bool rw   = SIGNAL_BOOL(cpu_rw);

	SIGNAL_SET_BOOL(reset_btn_b, !device->in_reset);
	device->in_reset = false;

	// A3 (1, 2)
	bool reset_b = SIGNAL_BOOL(reset_b);
	SIGNAL_SET_BOOL(reset, !reset_b);

	SIGNAL_SET_BOOL(init_b, reset_b);
	SIGNAL_SET_BOOL(init, !reset_b);
	SIGNAL_SET_BOOL(ifc_b, reset_b);

	// A3 (11, 10)
	bool sel8 = !sel8_b;
	SIGNAL_SET_BOOL(sel8, sel8);

	// A3 (3, 4)
	bool ba11_b = !SIGNAL_BOOL(ba11);
	SIGNAL_SET_BOOL(ba11_b, ba11_b);

	// B2 (1,2,4,5,6)
	bool x8xx = !(ba8 | ba9 | ba10 | ba11_b);
	SIGNAL_SET_BOOL(x8xx, x8xx);

	// A4 (4,5,6)
	bool addr_88xx_b = !(sel8 && x8xx);
	SIGNAL_SET_BOOL(s_88xx_b, addr_88xx_b);

	// A4 (1,2,3)
	bool rom_addr_b = !(ba15 && sel8_b);
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

	// >> pia logic
	bool cs1 = x8xx && SIGNAL_BOOL(ba6);
	SIGNAL_SET_BOOL(cs1, cs1);

	// >> irq logic: the 2001N wire-or's the irq lines of the chips and connects them to the cpu
	//		-> connecting the cpu's irq line to all the irq would just make us overwrite the values
	//		-> or the together explicitly
	bool irq_b = signal_read_next_bool(SIGNAL_POOL, device->pia_1->signals.irqa_b) &&
				 signal_read_next_bool(SIGNAL_POOL, device->pia_1->signals.irqb_b) &&
				 signal_read_next_bool(SIGNAL_POOL, device->pia_2->signals.irqa_b) &&
				 signal_read_next_bool(SIGNAL_POOL, device->pia_2->signals.irqb_b) &&
				 signal_read_next_bool(SIGNAL_POOL, device->via->signals.irq_b);
	SIGNAL_SET_BOOL(irq_b, irq_b);
}

// glue-logic: rams
static void glue_logic_register_dependencies_05(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba15), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(buf_rw), chip->id);
}

static void glue_logic_process_05(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// B2: 8, 9, 10, 12, 13 (4-Input NOR-gate with strobe)
	bool banksel = !(true && (false || false || false || SIGNAL_BOOL(ba15)));
	SIGNAL_SET_BOOL(banksel, banksel);

	// G7: 8, 9, 10, 11
	bool g78 = !(true && banksel && SIGNAL_BOOL(buf_rw));
	SIGNAL_SET_BOOL(g78, g78);
}

// glue-logic: master timing

static void glue_logic_register_dependencies_06(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	signal_add_dependency(SIGNAL_POOL, SIGNAL(bphi2a), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(bphi2b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(bphi2f), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(bphi2g), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(bphi2h), chip->id);

	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra1), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra3), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra4), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra5), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra6), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra6_b), chip->id);

	signal_add_dependency(SIGNAL_POOL, SIGNAL(h8q), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(h8q_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(h8q2), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(h8q2_b), chip->id);

	signal_add_dependency(SIGNAL_POOL, SIGNAL(bphi2), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(banksel), chip->id);

	signal_add_dependency(SIGNAL_POOL, SIGNAL(h1q1_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(h1q2_b), chip->id);

	signal_add_dependency(SIGNAL_POOL, SIGNAL(h4y4), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba14), chip->id);
}

static void glue_logic_process_06(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// H2 (1,2) + (3,4) + (11,10) + (13,12)
	SIGNAL_SET_BOOL(bphi2a_b, !SIGNAL_BOOL(bphi2a));
	SIGNAL_SET_BOOL(bphi2b_b, !SIGNAL_BOOL(bphi2b));
	SIGNAL_SET_BOOL(bphi2f_b, !SIGNAL_BOOL(bphi2f));
	SIGNAL_SET_BOOL(bphi2g_b, !SIGNAL_BOOL(bphi2g));

	// F1 (8,9,10)
	SIGNAL_SET_BOOL(ra1and3, SIGNAL_BOOL(ra1) & SIGNAL_BOOL(ra3));

	// H10 (4,5,6)
	SIGNAL_SET_BOOL(ra4and6, SIGNAL_BOOL(ra4) & SIGNAL_BOOL(ra6));

	// H10 (1,2,3)
	SIGNAL_SET_BOOL(ra5and6_b, SIGNAL_BOOL(ra5) & SIGNAL_BOOL(ra6_b));

	// G1 (8,9,10)
	bool video_latch = (!SIGNAL_BOOL(bphi2f) & SIGNAL_BOOL(bphi2h));
	SIGNAL_SET_BOOL(video_latch, video_latch);

	// H10 (11,12,13)
	bool video_on = SIGNAL_BOOL(h8q) & SIGNAL_BOOL(h8q2);
	SIGNAL_SET_BOOL(video_on, video_on);
	SIGNAL_SET_BOOL(video_on_2, video_on);

	// G10 (11,12,13)
	bool vert_drive = !(SIGNAL_BOOL(h8q2_b) & SIGNAL_BOOL(h8q_b));
	SIGNAL_SET_BOOL(vert_drive, vert_drive);

	// H5 (1,2,3)
	bool h53 = !(SIGNAL_BOOL(banksel) && SIGNAL_BOOL(bphi2));
	SIGNAL_SET_BOOL(h53, h53);

	// G1 (11,12,13)
	bool ras0_b = SIGNAL_BOOL(h1q1_b) && SIGNAL_BOOL(h1q2_b);
	SIGNAL_SET_BOOL(ras0_b, ras0_b);

	// G7 (3,4,5,6)
	bool cas1_b = !(SIGNAL_BOOL(h4y4) && SIGNAL_BOOL(ba14));
	SIGNAL_SET_BOOL(cas1_b, cas1_b);

	// H2 (8,9)
	bool ba14_b = !SIGNAL_BOOL(ba14);
	SIGNAL_SET_BOOL(ba14_b, ba14_b);

	// G7 (1,2,12,13)
	bool cas0_b = !(SIGNAL_BOOL(h4y4) && ba14_b);
	SIGNAL_SET_BOOL(cas0_b, cas0_b);
}

// glue-logic: display logic

static void glue_logic_register_dependencies_07(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	signal_add_dependency(SIGNAL_POOL, SIGNAL(buf_rw), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba11_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(bphi2), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ga6), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(lga3), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(lga6), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(lga7), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(lga8), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(lga9), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra9), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(sel8), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(video_on), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(pullup_2), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(horz_disp_off), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(reload_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(next_b), chip->id);
}

static void glue_logic_process_07(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	bool buf_rw = SIGNAL_BOOL(buf_rw);

	// >> F1 (4,5,6)
	bool tv_sel = SIGNAL_BOOL(ba11_b) && SIGNAL_BOOL(sel8);
	SIGNAL_SET_BOOL(tv_sel, tv_sel);

	// >> A5 (8,9,10,11)
	bool tv_read_b = !(true && buf_rw && tv_sel);
	SIGNAL_SET_BOOL(tv_read_b, tv_read_b);

	// >> A5 (1,2,12,13)
	bool a5_12 = !(tv_sel && !buf_rw && SIGNAL_BOOL(bphi2));
	SIGNAL_SET_BOOL(a5_12, a5_12);

	// >> I1 (5,6)
	SIGNAL_SET_BOOL(video_on_b, !SIGNAL_BOOL(video_on));

	// >> G2 (1,2,4,5,6)
	bool lines_20_b = !(SIGNAL_BOOL(ga6) && SIGNAL_BOOL(pullup_2) && !SIGNAL_BOOL(video_on) && SIGNAL_BOOL(ra9));
	SIGNAL_SET_BOOL(lines_20_b, lines_20_b);

	// >> G2 (8,9,10,12,13)
	bool lga_hi_b = !(SIGNAL_BOOL(lga6) && SIGNAL_BOOL(lga7) && SIGNAL_BOOL(lga8) && SIGNAL_BOOL(lga9));
	SIGNAL_SET_BOOL(lga_hi_b, lga_hi_b);

	// >> I1 (8,9)
	SIGNAL_SET_BOOL(lga_hi, !lga_hi_b);

	// >> H5 (4,5,6)
	bool lines_200_b = !(SIGNAL_BOOL(lga3) && SIGNAL_BOOL(lga_hi));
	SIGNAL_SET_BOOL(lines_200_b, lines_200_b);

	// >> H5 (8,9,10)
	bool line_220 = !(lines_200_b && lines_20_b);
	SIGNAL_SET_BOOL(line_220, line_220);

	// >> G3 (1,2,3)
	bool w220_off = line_220 && SIGNAL_BOOL(horz_disp_off);
	SIGNAL_SET_BOOL(w220_off, w220_off);

	// >> H5 (11,12,13)
	bool reload_next = !(SIGNAL_BOOL(reload_b) && SIGNAL_BOOL(next_b));
	SIGNAL_SET_BOOL(reload_next, reload_next);
}

// glue-logic: display rams

static void glue_logic_register_dependencies_08(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	signal_add_dependency(SIGNAL_POOL, SIGNAL(horz_disp_on), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra7), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra8), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(ra9), chip->id);

	signal_add_dependency(SIGNAL_POOL, SIGNAL(g9q), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(g9q_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(e11qh), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(e11qh), chip->id);

	signal_add_dependency(SIGNAL_POOL, SIGNAL(video_on), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(horz_disp_on), chip->id);
}

static void glue_logic_process_08(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// g11 (8,9,10,12,13)
	bool reload_b = !(SIGNAL_BOOL(horz_disp_on) && SIGNAL_BOOL(ra7) && SIGNAL_BOOL(ra8) && SIGNAL_BOOL(ra9));
	SIGNAL_SET_BOOL(reload_b, reload_b);

	// g10 (4,5,6)
	bool g106 = !(SIGNAL_BOOL(g9q) && SIGNAL_BOOL(e11qh));
	SIGNAL_SET_BOOL(g106, g106);

	// g10 (8,9,10)
	bool g108 = !(SIGNAL_BOOL(g9q_b) && SIGNAL_BOOL(e11qh_b));
	SIGNAL_SET_BOOL(g108, g108);

	// h10 (8,9,10)
	bool h108 = g106 && g108;
	SIGNAL_SET_BOOL(h108, h108);

	// g11 (1,2,5,4,6)
	bool video = !(h108 && true && SIGNAL_BOOL(video_on) && SIGNAL_BOOL(horz_disp_on));
	SIGNAL_SET_BOOL(video, video);
}

///////////////////////////////////////////////////////////////////////////////
//
// internal - rom functions
//

static Chip63xxRom *load_rom(DevCommodorePet *device, const char *filename, size_t num_lines, Signal rom_cs1_b) {

	Chip63xxSignals signals = {
		[CHIP_6332_A0] = signal_split(SIGNAL(bus_ba), 0, 1),
		[CHIP_6332_A1] = signal_split(SIGNAL(bus_ba), 1, 1),
		[CHIP_6332_A2] = signal_split(SIGNAL(bus_ba), 2, 1),
		[CHIP_6332_A3] = signal_split(SIGNAL(bus_ba), 3, 1),
		[CHIP_6332_A4] = signal_split(SIGNAL(bus_ba), 4, 1),
		[CHIP_6332_A5] = signal_split(SIGNAL(bus_ba), 5, 1),
		[CHIP_6332_A6] = signal_split(SIGNAL(bus_ba), 6, 1),
		[CHIP_6332_A7] = signal_split(SIGNAL(bus_ba), 7, 1),
		[CHIP_6332_A8] = signal_split(SIGNAL(bus_ba), 8, 1),
		[CHIP_6332_A9] = signal_split(SIGNAL(bus_ba), 9, 1),
		[CHIP_6332_A10] = signal_split(SIGNAL(bus_ba), 10, 1),
		[CHIP_6332_A11] = signal_split(SIGNAL(bus_ba), 11, 1),			// used as cs2_b for 6316 rom

		[CHIP_6332_D0] = signal_split(SIGNAL(cpu_bus_data), 0, 1),
		[CHIP_6332_D1] = signal_split(SIGNAL(cpu_bus_data), 1, 1),
		[CHIP_6332_D2] = signal_split(SIGNAL(cpu_bus_data), 2, 1),
		[CHIP_6332_D3] = signal_split(SIGNAL(cpu_bus_data), 3, 1),
		[CHIP_6332_D4] = signal_split(SIGNAL(cpu_bus_data), 4, 1),
		[CHIP_6332_D5] = signal_split(SIGNAL(cpu_bus_data), 5, 1),
		[CHIP_6332_D6] = signal_split(SIGNAL(cpu_bus_data), 6, 1),
		[CHIP_6332_D7] = signal_split(SIGNAL(cpu_bus_data), 7, 1),

		[CHIP_6332_CS1_B] = rom_cs1_b,
		[CHIP_6332_CS3] = SIGNAL(high)
	};

	Chip63xxRom *rom = (num_lines == 12) ?
		chip_6332_rom_create(device->simulator, signals) :
		chip_6316_rom_create(device->simulator, signals);

	if (file_load_binary_fixed(filename, rom->data_array, ROM_6332_DATA_SIZE) == 0) {
		rom->destroy(rom);
		return NULL;
	}

	return rom;
}

static Chip63xxRom *load_character_rom(DevCommodorePet *device, const char *filename) {

	Chip63xxRom *rom = chip_6316_rom_create(device->simulator, (Signal[24]) {
										[CHIP_6316_CS1_B] = SIGNAL(low),
										[CHIP_6316_CS2_B] = SIGNAL(low),
										[CHIP_6316_CS3] = SIGNAL(init_b),
										[CHIP_6316_A0] = SIGNAL(ra7),
										[CHIP_6316_A1] = SIGNAL(ra8),
										[CHIP_6316_A2] = SIGNAL(ra9),
										[CHIP_6316_A3] = signal_split(SIGNAL(bus_lsd), 0, 1),
										[CHIP_6316_A4] = signal_split(SIGNAL(bus_lsd), 1, 1),
										[CHIP_6316_A5] = signal_split(SIGNAL(bus_lsd), 2, 1),
										[CHIP_6316_A6] = signal_split(SIGNAL(bus_lsd), 3, 1),
										[CHIP_6316_A7] = signal_split(SIGNAL(bus_lsd), 4, 1),
										[CHIP_6316_A8] = signal_split(SIGNAL(bus_lsd), 5, 1),
										[CHIP_6316_A9] = signal_split(SIGNAL(bus_lsd), 6, 1),
										[CHIP_6316_A10] = SIGNAL(graphic),
										[CHIP_6316_D0] = signal_split(SIGNAL(bus_cd), 0, 1),
										[CHIP_6316_D1] = signal_split(SIGNAL(bus_cd), 1, 1),
										[CHIP_6316_D2] = signal_split(SIGNAL(bus_cd), 2, 1),
										[CHIP_6316_D3] = signal_split(SIGNAL(bus_cd), 3, 1),
										[CHIP_6316_D4] = signal_split(SIGNAL(bus_cd), 4, 1),
										[CHIP_6316_D5] = signal_split(SIGNAL(bus_cd), 5, 1),
										[CHIP_6316_D6] = signal_split(SIGNAL(bus_cd), 6, 1),
										[CHIP_6316_D7] = signal_split(SIGNAL(bus_cd), 7, 1)
	});

	if (file_load_binary_fixed(filename, rom->data_array, ROM_6316_DATA_SIZE) == 0) {
		rom->destroy(rom);
		return NULL;
	}

	return rom;
}

///////////////////////////////////////////////////////////////////////////////
//
// internal - circuit creation functions
//

// sheet 01: microprocessor & memory expansion
static void circuit_create_01(DevCommodorePet *device) {

	// power-on-reset
	DEVICE_REGISTER_CHIP("POR", poweronreset_create(US_TO_PS(500), device->simulator, (PowerOnResetSignals) {
											.trigger_b = SIGNAL(reset_btn_b),
											.reset_b = SIGNAL(reset_b)
	}));

	// cpu
	device->cpu = cpu_6502_create(device->simulator, (Cpu6502Signals) {
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
	DEVICE_REGISTER_CHIP("C4", device->cpu);

	// >> c3 - octal buffer
	DEVICE_REGISTER_CHIP("C3", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
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

	}));

	// >> b3 - octal buffer
	DEVICE_REGISTER_CHIP("B3", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
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

	}));

	// >> d2 - 4-to-16 decoder
	DEVICE_REGISTER_CHIP("D2", chip_74154_decoder_create(device->simulator, (Chip74154Signals) {
										.g1_b = SIGNAL(low),
										.g2_b = SIGNAL(low),
										.a  = signal_split(SIGNAL(bus_ba), 12, 1),
										.b  = signal_split(SIGNAL(bus_ba), 13, 1),
										.c  = signal_split(SIGNAL(bus_ba), 14, 1),
										.d  = signal_split(SIGNAL(bus_ba), 15, 1),
										.y0_b = SIGNAL(sel0_b),
										.y1_b = SIGNAL(sel1_b),
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
	}));

	// >> e9 - octal buffer
	DEVICE_REGISTER_CHIP("E9", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
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
	}));

	// >> e10 - octal buffer
	DEVICE_REGISTER_CHIP("E10", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
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
	}));
}

// sheet 02: IEEE-488 Interface
void circuit_create_02(DevCommodorePet *device) {

	// pia 1 (C6 - IEEE-488 interface)
	device->pia_1 = chip_6520_create(device->simulator, (Chip6520Signals) {
										.bus_data = SIGNAL(cpu_bus_data),
										.enable = SIGNAL(clk1),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.cs0 = SIGNAL(x8xx),
										.cs1 = signal_split(SIGNAL(bus_ba), 5, 1),
										.cs2_b = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										.rs0 = signal_split(SIGNAL(bus_ba), 0, 1),
										.rs1 = signal_split(SIGNAL(bus_ba), 1, 1),
										.ca1 = SIGNAL(atn_in_b),
										.ca2 = SIGNAL(ndac_out_b),
										.port_a = SIGNAL(bus_di),
										.port_b = SIGNAL(bus_do),
										.cb1 = SIGNAL(srq_in_b),
										.cb2 = SIGNAL(dav_out_b)
	});
	DEVICE_REGISTER_CHIP("C6", device->pia_1);

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC1", glue_logic_create(device, 1));
}

// sheet 03: cassette & keyboard
void circuit_create_03(DevCommodorePet *device) {

	// pia 2 (C7 - keyboard)
	device->pia_2 = chip_6520_create(device->simulator, (Chip6520Signals) {
										.bus_data = SIGNAL(cpu_bus_data),
										.enable = SIGNAL(clk1),
										.reset_b = SIGNAL(reset_b),
										.rw = SIGNAL(cpu_rw),
										.cs0 = SIGNAL(x8xx),
										.cs1 = signal_split(SIGNAL(bus_ba), 4, 1),
										.cs2_b = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										.rs0 = signal_split(SIGNAL(bus_ba), 0, 1),
										.rs1 = signal_split(SIGNAL(bus_ba), 1, 1),
										.ca1 = SIGNAL(cass_read_1),
										.ca2 = SIGNAL(eoi_out_b),
										.port_a = SIGNAL(c7_porta),
										.port_b = SIGNAL(bus_kin),
										.cb1 = SIGNAL(video_on),
										.cb2 = SIGNAL(cass_motor_1)
	});
	DEVICE_REGISTER_CHIP("C7", device->pia_2);

	// via (C5)
	device->via = chip_6522_create(device->simulator, (Chip6522Signals) {
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
										.ca1 = SIGNAL(ca1),
										.ca2 = SIGNAL(graphic),
										.port_a = SIGNAL(bus_pa),
										.port_b = SIGNAL(c5_portb),
										.cb1 = SIGNAL(cass_read_2),
										.cb2 = SIGNAL(cb2)
	});
	DEVICE_REGISTER_CHIP("C5", device->via);

	// >> c9 - bcd decoder
	DEVICE_REGISTER_CHIP("C9", chip_74145_bcd_decoder_create(device->simulator, (Chip74145Signals) {
										.a = SIGNAL(keya),
										.b = SIGNAL(keyb),
										.c = SIGNAL(keyc),
										.d = SIGNAL(keyd),
										.y0_b = signal_split(SIGNAL(bus_kout), 0, 1),
										.y1_b = signal_split(SIGNAL(bus_kout), 1, 1),
										.y2_b = signal_split(SIGNAL(bus_kout), 2, 1),
										.y3_b = signal_split(SIGNAL(bus_kout), 3, 1),
										.y4_b = signal_split(SIGNAL(bus_kout), 4, 1),
										.y5_b = signal_split(SIGNAL(bus_kout), 5, 1),
										.y6_b = signal_split(SIGNAL(bus_kout), 6, 1),
										.y7_b = signal_split(SIGNAL(bus_kout), 7, 1),
										.y8_b = signal_split(SIGNAL(bus_kout), 8, 1),
										.y9_b = signal_split(SIGNAL(bus_kout), 9, 1)
	}));
}


// sheet 04: roms
void circuit_create_04(DevCommodorePet *device) {

	int rom_count = 0;
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-b000.901465-19.bin", 12, SIGNAL(selb_b));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-c000.901465-20.bin", 12, SIGNAL(selc_b));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-d000.901465-21.bin", 12, SIGNAL(seld_b));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/edit-4-n.901447-29.bin", 11, SIGNAL(sele_b));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/kernal-4.901465-22.bin", 12, SIGNAL(self_b));

	for (int i = 0; i < rom_count; ++i) {
		assert(device->roms[i]);
		DEVICE_REGISTER_CHIP("ROM", device->roms[i]);
	}
}

// sheet 05: RAMS
void circuit_create_05(DevCommodorePet *device) {

	DEVICE_REGISTER_CHIP("I11", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										.g2_b = SIGNAL(buf_rw),							// 19
										.g1_b = SIGNAL(g78),							// 01

										.a11  = signal_split(SIGNAL(bus_rd), 7, 1),		// 02
										.y24  = signal_split(SIGNAL(bus_rd), 7, 1),		// 03
										.a12  = signal_split(SIGNAL(bus_rd), 6, 1),		// 04
										.y23  = signal_split(SIGNAL(bus_rd), 6, 1),		// 05
										.a13  = signal_split(SIGNAL(bus_rd), 5, 1),		// 06
										.y22  = signal_split(SIGNAL(bus_rd), 5, 1),		// 07
										.a14  = signal_split(SIGNAL(bus_rd), 4, 1),		// 08
										.y21  = signal_split(SIGNAL(bus_rd), 4, 1),		// 09

										.y11  = signal_split(SIGNAL(bus_bd), 7, 1),		// 18
										.a24  = signal_split(SIGNAL(bus_bd), 7, 1),		// 17
										.y12  = signal_split(SIGNAL(bus_bd), 6, 1),		// 16
										.a23  = signal_split(SIGNAL(bus_bd), 6, 1),		// 15
										.y13  = signal_split(SIGNAL(bus_bd), 5, 1),		// 14
										.a22  = signal_split(SIGNAL(bus_bd), 5, 1),		// 13
										.y14  = signal_split(SIGNAL(bus_bd), 4, 1),		// 12
										.a21  = signal_split(SIGNAL(bus_bd), 4, 1)		// 11
	}));

	DEVICE_REGISTER_CHIP("I10", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										.g2_b = SIGNAL(buf_rw),							// 19
										.g1_b = SIGNAL(g78),							// 01

										.a11  = signal_split(SIGNAL(bus_rd), 3, 1),		// 02
										.y24  = signal_split(SIGNAL(bus_rd), 3, 1),		// 03
										.a12  = signal_split(SIGNAL(bus_rd), 2, 1),		// 04
										.y23  = signal_split(SIGNAL(bus_rd), 2, 1),		// 05
										.a13  = signal_split(SIGNAL(bus_rd), 1, 1),		// 06
										.y22  = signal_split(SIGNAL(bus_rd), 1, 1),		// 07
										.a14  = signal_split(SIGNAL(bus_rd), 0, 1),		// 08
										.y21  = signal_split(SIGNAL(bus_rd), 0, 1),		// 09

										.y11  = signal_split(SIGNAL(bus_bd), 3, 1),		// 18
										.a24  = signal_split(SIGNAL(bus_bd), 3, 1),		// 17
										.y12  = signal_split(SIGNAL(bus_bd), 2, 1),		// 16
										.a23  = signal_split(SIGNAL(bus_bd), 2, 1),		// 15
										.y13  = signal_split(SIGNAL(bus_bd), 1, 1),		// 14
										.a22  = signal_split(SIGNAL(bus_bd), 1, 1),		// 13
										.y14  = signal_split(SIGNAL(bus_bd), 0, 1),		// 12
										.a21  = signal_split(SIGNAL(bus_bd), 0, 1)		// 11
	}));

	DEVICE_REGISTER_CHIP("E3", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										.g1 = SIGNAL(low),								// pin 1
										.c10 = SIGNAL(ra7),								// pin 6
										.c11 = SIGNAL(ra7),								// pin 5
										.c12 = signal_split(SIGNAL(bus_ba), 0, 1),		// pin 4
										.c13 = signal_split(SIGNAL(bus_ba), 13, 1),		// pin 3
										.y1 = signal_split(SIGNAL(bus_fa), 0, 1),		// pin 7

										.a = SIGNAL(muxa),								// pin 14
										.b = SIGNAL(clk1),								// pin 2

										.g2 = SIGNAL(low),								// pin 15
										.c20 = SIGNAL(ra1),								// pin 10
										.c21 = SIGNAL(ra1),								// pin 11
										.c22 = signal_split(SIGNAL(bus_ba), 1, 1),		// pin 12
										.c23 = signal_split(SIGNAL(bus_ba), 7, 1),		// pin 13
										.y2 = signal_split(SIGNAL(bus_fa), 1, 1),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E4", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										.g1 = SIGNAL(low),								// pin 1
										.c10 = SIGNAL(ra2),								// pin 6
										.c11 = SIGNAL(ra2),								// pin 5
										.c12 = signal_split(SIGNAL(bus_ba), 2, 1),		// pin 4
										.c13 = signal_split(SIGNAL(bus_ba), 8, 1),		// pin 3
										.y1 = signal_split(SIGNAL(bus_fa), 2, 1),		// pin 7

										.a = SIGNAL(muxa),								// pin 14
										.b = SIGNAL(clk1),								// pin 2

										.g2 = SIGNAL(low),								// pin 15
										.c20 = SIGNAL(ra3),								// pin 10
										.c21 = SIGNAL(ra3),								// pin 11
										.c22 = signal_split(SIGNAL(bus_ba), 3, 1),		// pin 12
										.c23 = signal_split(SIGNAL(bus_ba), 9, 1),		// pin 13
										.y2 = signal_split(SIGNAL(bus_fa), 3, 1),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E5", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										.g1 = SIGNAL(low),								// pin 1
										.c10 = SIGNAL(ra4),								// pin 6
										.c11 = SIGNAL(ra4),								// pin 5
										.c12 = signal_split(SIGNAL(bus_ba), 4, 1),		// pin 4
										.c13 = signal_split(SIGNAL(bus_ba), 10, 1),		// pin 3
										.y1 = signal_split(SIGNAL(bus_fa), 4, 1),		// pin 7

										.a = SIGNAL(muxa),								// pin 14
										.b = SIGNAL(clk1),								// pin 2

										.g2 = SIGNAL(low),								// pin 15
										.c20 = SIGNAL(ra5),								// pin 10
										.c21 = SIGNAL(ra5),								// pin 11
										.c22 = signal_split(SIGNAL(bus_ba), 5, 1),		// pin 12
										.c23 = signal_split(SIGNAL(bus_ba), 11, 1),		// pin 13
										.y2 = signal_split(SIGNAL(bus_fa), 5, 1),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E6", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										.g1 = SIGNAL(low),								// pin 1
										.c10 = SIGNAL(ra6),								// pin 6
										.c11 = SIGNAL(ra6),								// pin 5
										.c12 = signal_split(SIGNAL(bus_ba), 6, 1),		// pin 4
										.c13 = signal_split(SIGNAL(bus_ba), 12, 1),		// pin 3
										.y1 = signal_split(SIGNAL(bus_fa), 6, 1),		// pin 7

										.a = SIGNAL(muxa),								// pin 14
										.b = SIGNAL(clk1),								// pin 2
	}));

	DEVICE_REGISTER_CHIP("I2-9", chip_8x4116_dram_create(device->simulator, (Chip8x4116DRamSignals) {
										.bus_address = SIGNAL(bus_fa),
										.we_b = SIGNAL(ram_rw),
										.ras_b = SIGNAL(ras0_b),
										.cas_b = SIGNAL(cas0_b),
										.bus_di = SIGNAL(bus_rd),
										.bus_do = SIGNAL(bus_rd)
	}));

	DEVICE_REGISTER_CHIP("J2-9", chip_8x4116_dram_create(device->simulator, (Chip8x4116DRamSignals) {
										.bus_address = SIGNAL(bus_fa),
										.we_b = SIGNAL(ram_rw),
										.ras_b = SIGNAL(ras0_b),
										.cas_b = SIGNAL(cas1_b),
										.bus_di = SIGNAL(bus_rd),
										.bus_do = SIGNAL(bus_rd)
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC5", glue_logic_create(device, 5));
}

// sheet 06 - master timing
void circuit_create_06(DevCommodorePet *device) {

	// >> y1 - oscillator
	device->oscillator_y1 = oscillator_create(16000000, device->simulator, (OscillatorSignals) {
										.clk_out = SIGNAL(clk16)
	});
	DEVICE_REGISTER_CHIP("Y1", device->oscillator_y1);

	// >> g5 - binary counter
	DEVICE_REGISTER_CHIP("G5", chip_74191_binary_counter_create(device->simulator, (Chip74191Signals) {
										.vcc = SIGNAL(high),			// pin 16
										.gnd = SIGNAL(low),				// pin 08
										.enable_b = SIGNAL(low),		// pin 04
										.d_u = SIGNAL(low),				// pin 05
										.a = SIGNAL(low),				// pin 15
										.b = SIGNAL(low),				// pin 01
										.c = SIGNAL(low),				// pin 10
										.d = SIGNAL(low),				// pin 09
										.load_b = SIGNAL(init_b),		// pin 11
										.clk = SIGNAL(clk16),			// pin 14
										.qa = SIGNAL(clk8),				// pin 03
										.qb = SIGNAL(clk4),				// pin 02
										.qc = SIGNAL(clk2),				// pin 06
										.qd = SIGNAL(clk1),				// pin 07
								//		.max_min = not connected        // pin 12
								//		.rco_b = not connected          // pin 13
	}));

	// >> h3 - 8-bit shift register
	DEVICE_REGISTER_CHIP("H3", chip_74164_shift_register_create(device->simulator, (Chip74164Signals) {
										.a = SIGNAL(clk1),				// pin 01
										.b = SIGNAL(high),				// pin 02
										.clk = SIGNAL(clk16),			// pin 08
										.clear_b = SIGNAL(init_b),		// pin 09
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
	}));

	// >> h6 - JK flip-flop
	DEVICE_REGISTER_CHIP("H6", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
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
	}));

	// >> h9 - binary counter
	DEVICE_REGISTER_CHIP("H9", chip_7493_binary_counter_create(device->simulator, (Chip7493Signals) {
										.gnd = SIGNAL(low),				// pin 10
										.vcc = SIGNAL(high),			// pin 5
										.a_b = SIGNAL(ra1),				// pin 14
										.b_b = SIGNAL(ra2),				// pin 1
										.r01 = SIGNAL(init),			// pin 2
										.r02 = SIGNAL(init),			// pin 3
										.qa = SIGNAL(ra2),				// pin 12
										.qb = SIGNAL(ra3),				// pin 9
										.qc = SIGNAL(ra4),				// pin 8
										.qd = SIGNAL(ra5),				// pin 11
	}));

	// >> g9 - d flip-flop (2 flipflop is used on sheet 8)
	DEVICE_REGISTER_CHIP("G9", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										.gnd = SIGNAL(low),				// pin 7
										.vcc = SIGNAL(high),			// pin 14

										.d1 = SIGNAL(init_b),			// pin 2
										.clr1_b = SIGNAL(bphi2b),		// pin 1
										.clk1 = SIGNAL(bphi2a_b),		// pin 3
										.pr1_b = SIGNAL(high),			// pin 4
										.q1 = SIGNAL(load_sr),			// pin 5
										.q1_b = SIGNAL(load_sr_b),		// pin 6

										.d2 = signal_split(SIGNAL(bus_lsd), 7, 1),	// pin 12
										.clr2_b = SIGNAL(init_b),					// pin 13
										.clk2 = SIGNAL(load_sr),					// pin 11
										.pr2_b = SIGNAL(high),						// pin 10
										.q2 = SIGNAL(g9q),							// pin 9
										.q2_b = SIGNAL(g9q_b),						// pin 8
	}));

	// >> h7 - JK flip-flop
	DEVICE_REGISTER_CHIP("H7", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										.gnd = SIGNAL(low),				// pin 7
										.vcc = SIGNAL(high),			// pin 14

										.clr1_b = SIGNAL(init_b),		// pin 13
										.clk1 = SIGNAL(ra1and3),		// pin 12
										.j1 = SIGNAL(horz_disp_on),		// pin 1
										.k1 = SIGNAL(horz_disp_off),	// pin 4
										.q1 = SIGNAL(horz_drive_b),		// pin 3
										.q1_b = SIGNAL(horz_drive),		// pin 2

										.clr2_b = SIGNAL(init_b),		// pin 10
										.clk2 = SIGNAL(load_sr_b),		// pin 9
										.j2 = SIGNAL(ra4and6),			// pin 8
										.k2 = SIGNAL(ra5and6_b),		// pin 11
										.q2 = SIGNAL(horz_disp_on),		// pin 5
										.q2_b = SIGNAL(horz_disp_off)	// pin 6
	}));

	// >> h8 - JK flip-flop
	DEVICE_REGISTER_CHIP("H8", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										.gnd = SIGNAL(low),				// pin 7
										.vcc = SIGNAL(high),			// pin 14

										.clr1_b = SIGNAL(init_b),		// pin 13
										.clk1 = SIGNAL(next),			// pin 12
										.j1 = SIGNAL(h8q2_b),			// pin 1
										.k1 = SIGNAL(h8q2),				// pin 4
										.q1 = SIGNAL(h8q),				// pin 3
										.q1_b = SIGNAL(h8q_b),			// pin 2

										.clr2_b = SIGNAL(init_b),		// pin 10
										.clk2 = SIGNAL(next),			// pin 9
										.j2 = SIGNAL(h8q),				// pin 8
										.k2 = SIGNAL(h8q_b),			// pin 11
										.q2 = SIGNAL(h8q2),				// pin 5
										.q2_b = SIGNAL(h8q2_b)			// pin 6
	}));

	// >> h4 - Quad 2to1 multiplexer
	DEVICE_REGISTER_CHIP("H4", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										.i0a = SIGNAL(bphi2b),			// pin 2
										.i1a = SIGNAL(bphi2a),			// pin 3
										.za = SIGNAL(h4y1),				// pin 4

										.i0d = SIGNAL(bphi2c),			// pin 11
										.i1d = SIGNAL(bphi2b),			// pin 10
										.zd = SIGNAL(muxa),				// pin 9

										.i0c = SIGNAL(bphi2d),			// pin 14
										.i1c = SIGNAL(bphi2c),			// pin 13
										.zc = SIGNAL(h4y4),				// pin 12

										.sel = SIGNAL(buf_rw),			// pin 1
										.enable_b = SIGNAL(h53)			// pin 15
	}));

	// >> h1 - d flipflop
	DEVICE_REGISTER_CHIP("H1", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										.gnd = SIGNAL(low),				// pin 7
										.vcc = SIGNAL(high),			// pin 14

										.d1 = SIGNAL(init_b),			// pin 2
										.clr1_b = SIGNAL(bphi2g_b),		// pin 1
										.clk1 = SIGNAL(h4y1),			// pin 3
										.pr1_b = SIGNAL(init_b),		// pin 4
										.q1 = SIGNAL(h1q1),				// pin 5
										.q1_b = SIGNAL(h1q1_b),			// pin 6

										.d2 = SIGNAL(init_b),			// pin 12
										.clr2_b = SIGNAL(bphi2f),		// pin 13
										.clk2 = SIGNAL(bphi2b_b),		// pin 11
										.pr2_b = SIGNAL(pullup_2),		// pin 10
										.q2 = SIGNAL(h1q2),				// pin 9
										.q2_b = SIGNAL(h1q2_b)			// pin 8
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC6", glue_logic_create(device, 6));
}

// sheet 07 - display logic components
void circuit_create_07(DevCommodorePet *device) {

	// >> g6 - JK flip-flop
	DEVICE_REGISTER_CHIP("G6", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										.gnd = SIGNAL(low),				// pin 7
										.vcc = SIGNAL(high),			// pin 14

										.clr1_b = SIGNAL(horz_disp_on),	// pin 13
										.clk1 = SIGNAL(ra1_b),			// pin 12
										.j1 = SIGNAL(init_b),			// pin 1
										.k1 = SIGNAL(init_b),			// pin 4
										.q1 = SIGNAL(g6_q),				// pin 3
										.q1_b = SIGNAL(g6_q_b),			// pin 2
	}));

	// >> f6 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F6", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										.gnd = SIGNAL(low),				// pin 8
										.vcc = SIGNAL(high),			// pin 16

										.i0d = SIGNAL(high),						// pin 11
										.i1d = SIGNAL(high),						// pin 10
										.i0b = SIGNAL(high),						// pin 05
										.i1b = SIGNAL(a5_12),						// pin 06
										.i0c = SIGNAL(ra1_b),						// pin 14
										.i1c = signal_split(SIGNAL(bus_ba), 0, 1),	// pin 13
										.i0a = SIGNAL(g6_q),						// pin 02
										.i1a = signal_split(SIGNAL(bus_ba), 1, 1),	// pin 03

										.sel = SIGNAL(clk1),						// pin 01
										.enable_b = SIGNAL(low),					// pin 15

										.zb = SIGNAL(tv_ram_rw),					// pin 07
										.zd = SIGNAL(f6_y3),						// pin 09
										.zc = signal_split(SIGNAL(bus_sa), 0, 1),	// pin 12
										.za = signal_split(SIGNAL(bus_sa), 1, 1)	// pin 04
	}));

	// >> f5 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F5", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										.gnd = SIGNAL(low),				// pin 8
										.vcc = SIGNAL(high),			// pin 16

										.i0d = SIGNAL(ga2),							// pin 11
										.i1d = signal_split(SIGNAL(bus_ba), 2, 1),	// pin 10
										.i0b = SIGNAL(ga3),							// pin 05
										.i1b = signal_split(SIGNAL(bus_ba), 3, 1),	// pin 06
										.i0c = SIGNAL(ga4),							// pin 14
										.i1c = signal_split(SIGNAL(bus_ba), 4, 1),	// pin 13
										.i0a = SIGNAL(ga5),							// pin 02
										.i1a = signal_split(SIGNAL(bus_ba), 5, 1),	// pin 03

										.sel = SIGNAL(clk1),						// pin 01
										.enable_b = SIGNAL(low),					// pin 15

										.zd = signal_split(SIGNAL(bus_sa), 2, 1),	// pin 09
										.zb = signal_split(SIGNAL(bus_sa), 3, 1),	// pin 07
										.zc = signal_split(SIGNAL(bus_sa), 4, 1),	// pin 12
										.za = signal_split(SIGNAL(bus_sa), 5, 1)	// pin 04
	}));

	// >> f3 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F3", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										.gnd = SIGNAL(low),				// pin 8
										.vcc = SIGNAL(high),			// pin 16

										.i0a = SIGNAL(ga6),							// pin 02
										.i1a = signal_split(SIGNAL(bus_ba), 6, 1),	// pin 03
										.i0c = SIGNAL(ga7),							// pin 14
										.i1c = signal_split(SIGNAL(bus_ba), 7, 1),	// pin 13
										.i0b = SIGNAL(ga8),							// pin 05
										.i1b = signal_split(SIGNAL(bus_ba), 8, 1),	// pin 06
										.i0d = SIGNAL(ga9),							// pin 11
										.i1d = signal_split(SIGNAL(bus_ba), 9, 1),	// pin 10

										.sel = SIGNAL(clk1),						// pin 01
										.enable_b = SIGNAL(low),					// pin 15

										.za = signal_split(SIGNAL(bus_sa), 6, 1),	// pin 04
										.zc = signal_split(SIGNAL(bus_sa), 7, 1),	// pin 12
										.zb = signal_split(SIGNAL(bus_sa), 8, 1),	// pin 07
										.zd = signal_split(SIGNAL(bus_sa), 9, 1)	// pin 09
	}));

	// >> f4 - binary counter
	DEVICE_REGISTER_CHIP("F4", chip_74177_binary_counter_create(device->simulator, (Chip74177Signals) {
										.gnd = SIGNAL(low),					// pin 07
										.vcc = SIGNAL(high),				// pin 14

										.clk2 = SIGNAL(ga2),				// pin 06
										.clk1 = SIGNAL(g6_q),				// pin 08
										.load_b = SIGNAL(horz_disp_on),		// pin 01
										.clear_b = SIGNAL(next_b),			// pin 13
										.a = SIGNAL(lga2),					// pin 04
										.b = SIGNAL(lga3),					// pin 10
										.c = SIGNAL(lga4),					// pin 03
										.d = SIGNAL(lga5),					// pin 11

										.qa = SIGNAL(ga2),					// pin 05
										.qb = SIGNAL(ga3),					// pin 09
										.qc = SIGNAL(ga4),					// pin 02
										.qd = SIGNAL(ga5)					// pin 12
	}));

	// >> f2 - binary counter
	DEVICE_REGISTER_CHIP("F2", chip_74177_binary_counter_create(device->simulator, (Chip74177Signals) {
										.gnd = SIGNAL(low),					// pin 07
										.vcc = SIGNAL(high),				// pin 14

										.clk2 = SIGNAL(ga6),				// pin 06
										.clk1 = SIGNAL(ga5),				// pin 08
										.load_b = SIGNAL(horz_disp_on),		// pin 01
										.clear_b = SIGNAL(next_b),			// pin 13
										.a = SIGNAL(lga6),					// pin 04
										.b = SIGNAL(lga7),					// pin 10
										.c = SIGNAL(lga8),					// pin 03
										.d = SIGNAL(lga9),					// pin 11

										.qa = SIGNAL(ga6),					// pin 05
										.qb = SIGNAL(ga7),					// pin 09
										.qc = SIGNAL(ga8),					// pin 02
										.qd = SIGNAL(ga9)					// pin 12
	}));

	// >> g3 - 8-bit latch
	DEVICE_REGISTER_CHIP("G3", chip_74373_latch_create(device->simulator, (Chip74373Signals) {
										.gnd = SIGNAL(low),			// pin 10
										.vcc = SIGNAL(high),		// pin 20

										.d5 = SIGNAL(ga2),			// pin 13
										.d8 = SIGNAL(ga3),			// pin 18
										.d6 = SIGNAL(ga4),			// pin 14
										.d7 = SIGNAL(ga5),			// pin 17
										.d2 = SIGNAL(ga6),			// pin 04
										.d3 = SIGNAL(ga7),			// pin 07
										.d1 = SIGNAL(ga8),			// pin 03
										.d4 = SIGNAL(ga9),			// pin 08

										.c = SIGNAL(reload_next),	// pin 11 - enable input
										.oc_b = SIGNAL(low),		// pin 01 - output control

										.q5 = SIGNAL(lga2),			// pin 12
										.q8 = SIGNAL(lga3),			// pin 19
										.q6 = SIGNAL(lga4),			// pin 15
										.q7 = SIGNAL(lga5),			// pin 16
										.q2 = SIGNAL(lga6),			// pin 05
										.q3 = SIGNAL(lga7),			// pin 06
										.q1 = SIGNAL(lga8),			// pin 02
										.q4 = SIGNAL(lga9)			// pin 09
	}));

	// >> g8 - d flip-flop
	DEVICE_REGISTER_CHIP("G8", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										.gnd = SIGNAL(low),				// pin 7
										.vcc = SIGNAL(high),			// pin 14

										// .d1 = not used,				// pin 2
										// .clr1_b = not used,			// pin 1
										// .clk1 = not used,			// pin 3
										// .pr1_b = not used,			// pin 4
										// .q1 = not used,				// pin 5
										// .q1_b = not used,			// pin 6

										.pr2_b = SIGNAL(pullup_1),		// pin 10
										.d2 = SIGNAL(w220_off),			// pin 12
										.clk2 = SIGNAL(video_latch),	// pin 11
										.clr2_b = SIGNAL(bphi2h),		// pin 13
										.q2 = SIGNAL(next),				// pin 9
										.q2_b = SIGNAL(next_b)			// pin 8
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC7", glue_logic_create(device, 7));
}

// sheet 08: display rams components
void circuit_create_08(DevCommodorePet *device) {

	// >> h11 - binary counter
	DEVICE_REGISTER_CHIP("H11", chip_7493_binary_counter_create(device->simulator, (Chip7493Signals) {
										.gnd = SIGNAL(low),				// pin 10
										.vcc = SIGNAL(high),			// pin 5

										.a_b = SIGNAL(low),				// pin 14
										.b_b = SIGNAL(horz_disp_on),	// pin 1
										.r01 = SIGNAL(next),			// pin 2
										.r02 = SIGNAL(next),			// pin 3
										// .qa = not used,				// pin 12
										.qb = SIGNAL(ra7),				// pin 9
										.qc = SIGNAL(ra8),				// pin 8
										.qd = SIGNAL(ra9),				// pin 11
	}));

	// >> f7 - 1k x 4bit SRAM
	DEVICE_REGISTER_CHIP("F7", chip_6114_sram_create(device->simulator, (Chip6114SRamSignals) {
										.bus_address = SIGNAL(bus_sa),
										.bus_io = signal_split(SIGNAL(bus_sd), 4, 4),
										.ce_b = SIGNAL(low),
										.rw = SIGNAL(tv_ram_rw)
	}));

	// >> f8 - 1k x 4bit SRAM
	DEVICE_REGISTER_CHIP("F8", chip_6114_sram_create(device->simulator, (Chip6114SRamSignals) {
										.bus_address = SIGNAL(bus_sa),
										.bus_io = signal_split(SIGNAL(bus_sd), 0, 4),
										.ce_b = SIGNAL(low),
										.rw = SIGNAL(tv_ram_rw)
	}));

	// >> e8 - octal buffer
	DEVICE_REGISTER_CHIP("E8", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										.g2_b = SIGNAL(tv_ram_rw),						// 19
										.g1_b = SIGNAL(tv_read_b),						// 01

										.a11  = signal_split(SIGNAL(bus_sd), 0, 1),		// 02
										.y24  = signal_split(SIGNAL(bus_sd), 0, 1),		// 03
										.a12  = signal_split(SIGNAL(bus_sd), 1, 1),		// 04
										.y23  = signal_split(SIGNAL(bus_sd), 1, 1),		// 05
										.a13  = signal_split(SIGNAL(bus_sd), 2, 1),		// 06
										.y22  = signal_split(SIGNAL(bus_sd), 2, 1),		// 07
										.a14  = signal_split(SIGNAL(bus_sd), 3, 1),		// 08
										.y21  = signal_split(SIGNAL(bus_sd), 3, 1),		// 09

										.y11  = signal_split(SIGNAL(bus_bd), 0, 1),		// 18
										.a24  = signal_split(SIGNAL(bus_bd), 0, 1),		// 17
										.y12  = signal_split(SIGNAL(bus_bd), 1, 1),		// 16
										.a23  = signal_split(SIGNAL(bus_bd), 1, 1),		// 15
										.y13  = signal_split(SIGNAL(bus_bd), 2, 1),		// 14
										.a22  = signal_split(SIGNAL(bus_bd), 2, 1),		// 13
										.y14  = signal_split(SIGNAL(bus_bd), 3, 1),		// 12
										.a21  = signal_split(SIGNAL(bus_bd), 3, 1)		// 11
	}));

	// >> e7 - octal buffer
	DEVICE_REGISTER_CHIP("E7", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										.g2_b = SIGNAL(tv_ram_rw),						// 19
										.g1_b = SIGNAL(tv_read_b),						// 01

										.a11  = signal_split(SIGNAL(bus_sd), 4, 1),		// 02
										.y24  = signal_split(SIGNAL(bus_sd), 4, 1),		// 03
										.a12  = signal_split(SIGNAL(bus_sd), 5, 1),		// 04
										.y23  = signal_split(SIGNAL(bus_sd), 5, 1),		// 05
										.a13  = signal_split(SIGNAL(bus_sd), 6, 1),		// 06
										.y22  = signal_split(SIGNAL(bus_sd), 6, 1),		// 07
										.a14  = signal_split(SIGNAL(bus_sd), 7, 1),		// 08
										.y21  = signal_split(SIGNAL(bus_sd), 7, 1),		// 09

										.y11  = signal_split(SIGNAL(bus_bd), 4, 1),		// 18
										.a24  = signal_split(SIGNAL(bus_bd), 4, 1),		// 17
										.y12  = signal_split(SIGNAL(bus_bd), 5, 1),		// 16
										.a23  = signal_split(SIGNAL(bus_bd), 5, 1),		// 15
										.y13  = signal_split(SIGNAL(bus_bd), 6, 1),		// 14
										.a22  = signal_split(SIGNAL(bus_bd), 6, 1),		// 13
										.y14  = signal_split(SIGNAL(bus_bd), 7, 1),		// 12
										.a21  = signal_split(SIGNAL(bus_bd), 7, 1)		// 11
	}));

	// >> f9 - 8-bit latch
	DEVICE_REGISTER_CHIP("F9", chip_74373_latch_create(device->simulator, (Chip74373Signals) {
										.c	  = SIGNAL(video_latch),					// 11
										.oc_b = SIGNAL(low),							// 1
										.d1   = signal_split(SIGNAL(bus_sd), 0, 1),		// 3
										.d8   = signal_split(SIGNAL(bus_sd), 1, 1),		// 18
										.d2   = signal_split(SIGNAL(bus_sd), 2, 1),		// 4
										.d7   = signal_split(SIGNAL(bus_sd), 3, 1),		// 17
										.d3   = signal_split(SIGNAL(bus_sd), 4, 1),		// 7
										.d6   = signal_split(SIGNAL(bus_sd), 5, 1),		// 14
										.d4   = signal_split(SIGNAL(bus_sd), 6, 1),		// 8
										.d5   = signal_split(SIGNAL(bus_sd), 7, 1),		// 13

										.q1   = signal_split(SIGNAL(bus_lsd), 0, 1),	// 2
										.q8   = signal_split(SIGNAL(bus_lsd), 1, 1),	// 19
										.q2   = signal_split(SIGNAL(bus_lsd), 2, 1),	// 5
										.q7   = signal_split(SIGNAL(bus_lsd), 3, 1),	// 16
										.q3   = signal_split(SIGNAL(bus_lsd), 4, 1),	// 6
										.q6   = signal_split(SIGNAL(bus_lsd), 5, 1),	// 15
										.q4   = signal_split(SIGNAL(bus_lsd), 6, 1),	// 9
										.q5   = signal_split(SIGNAL(bus_lsd), 7, 1),	// 12
	}));

	// >> f10 - character rom
	Chip63xxRom *char_rom = load_character_rom(device, "runtime/commodore_pet/characters-2.901447-10.bin");
	assert(char_rom);
	DEVICE_REGISTER_CHIP("F10", char_rom);

	// >> e11 - shift register
	DEVICE_REGISTER_CHIP("E11", chip_74165_shift_register_create(device->simulator, (Chip74165Signals) {
										.sl		 = SIGNAL(load_sr_b),					// 1
										.clk	 = SIGNAL(clk8),						// 2
										.clk_inh = SIGNAL(low),							// 15
										.si		 = signal_split(SIGNAL(bus_cd), 7, 1),	// 10
										.a		 = signal_split(SIGNAL(bus_cd), 0, 1),	// 11
										.b		 = signal_split(SIGNAL(bus_cd), 1, 1),	// 12
										.c		 = signal_split(SIGNAL(bus_cd), 2, 1),	// 13
										.d		 = signal_split(SIGNAL(bus_cd), 3, 1),	// 14
										.e		 = signal_split(SIGNAL(bus_cd), 4, 1),	// 3
										.f		 = signal_split(SIGNAL(bus_cd), 5, 1),	// 4
										.g		 = signal_split(SIGNAL(bus_cd), 6, 1),	// 5
										.h		 = signal_split(SIGNAL(bus_cd), 7, 1),	// 6
										.qh		 = SIGNAL(e11qh),						// 9
										.qh_b	 = SIGNAL(e11qh_b),						// 7
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC8", glue_logic_create(device, 8));
}

// peripherals
void circuit_create_pheriperals(DevCommodorePet *device) {

	// keyboard
	device->keypad = input_keypad_create(device->simulator, false, 10, 8, 500, 100, (InputKeypadSignals) {
										.cols = SIGNAL(bus_kin),
										.rows = SIGNAL(bus_kout)
	});
	DEVICE_REGISTER_CHIP("KEYPAD", device->keypad);


	// display
	device->crt = display_pet_crt_create(device->simulator, (DisplayPetCrtSignals) {
										.video_in = SIGNAL(video),
										.horz_drive_in = SIGNAL(horz_drive),
										.vert_drive_in = SIGNAL(vert_drive)
	});
	DEVICE_REGISTER_CHIP("CRT", device->crt);

}

// lite-PET: RAM circuitry
void circuit_lite_create_ram(DevCommodorePet *device) {

	Ram8d16a *ram = ram_8d16a_create(15, device->simulator, (Ram8d16aSignals) {
										.bus_address = signal_split(device->signals.bus_ba, 0, 15),
										.bus_data = SIGNAL(bus_bd),
										.ce_b = SIGNAL(ba15),
										.oe_b = SIGNAL(g78),
										.we_b = SIGNAL(ram_rw)
	});
	DEVICE_REGISTER_CHIP("RAM", ram);

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC5", glue_logic_create(device, 5));
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

void dev_commodore_pet_read_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *output);
void dev_commodore_pet_write_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *input);
void dev_commodore_pet_lite_read_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *output);
void dev_commodore_pet_lite_write_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *input);

Cpu6502* dev_commodore_pet_get_cpu(DevCommodorePet *device) {
	assert(device);
	return device->cpu;
}

DevCommodorePet *create_pet_device(bool lite) {
	DevCommodorePet *device = (DevCommodorePet *) calloc(1, sizeof(DevCommodorePet));

	// interface
	device->get_cpu = (DEVICE_GET_CPU) dev_commodore_pet_get_cpu;
	device->process = (DEVICE_PROCESS) device_process;
	device->reset = (DEVICE_RESET) dev_commodore_pet_reset;
	device->destroy = (DEVICE_DESTROY) dev_commodore_pet_destroy;
	device->read_memory = (DEVICE_READ_MEMORY) ((!lite) ? dev_commodore_pet_read_memory : dev_commodore_pet_lite_read_memory);
	device->write_memory = (DEVICE_WRITE_MEMORY) ((!lite) ? dev_commodore_pet_write_memory : dev_commodore_pet_lite_write_memory);

	device->simulator = simulator_create(6250);		// // 6.25 ns - 160 Mhz

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// signals
	//

	//
	// signals - general
	//

	SIGNAL_DEFINE_BOOL_N(reset_btn_b, 1, ACTLO_DEASSERT, "RESBTN");
	SIGNAL_DEFINE_BOOL_N(high, 1, true, "VCC");
	SIGNAL_DEFINE_BOOL_N(low, 1, false, "GND");

	//
	// signals - sheet 1: microprocessor / memory expansion
	//

	SIGNAL_DEFINE_BOOL_N(reset_b, 1, ACTLO_ASSERT, "/RES");
	SIGNAL_DEFINE_N(reset, 1, "RES");
	SIGNAL_DEFINE_BOOL_N(irq_b, 1, ACTLO_DEASSERT, "/IRQ");
	SIGNAL_DEFINE_BOOL_N(nmi_b, 1, ACTLO_DEASSERT, "/NMI");

	SIGNAL_DEFINE_N(cpu_bus_address, 16, "AB%d");
	SIGNAL_DEFINE_N(cpu_bus_data, 8, "D%d");
	SIGNAL_DEFINE_BOOL_N(cpu_rw, 1, true, "RW");
	SIGNAL_DEFINE_N(cpu_sync, 1, "SYNC");
	SIGNAL_DEFINE_BOOL_N(cpu_rdy, 1, ACTHI_ASSERT, "RDY");

	SIGNAL_DEFINE_N(bus_ba, 16, "BA%d");
	SIGNAL_DEFINE_N(bus_bd, 8, "BD%d");

	SIGNAL_DEFINE_N(sel0_b, 1, "/SEL0");
	SIGNAL_DEFINE_N(sel1_b, 1, "/SEL1");
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

	SIGNAL_DEFINE_N(a5_12, 1, "A512");

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

	//
	// signals - sheet 2: IEEE-488 interface
	//

	SIGNAL_DEFINE_N(atn_in_b, 1, "/ATNIN");
	SIGNAL_DEFINE_N(ndac_out_b, 1, "/NDACOUT");
	SIGNAL_DEFINE_N(ifc_b, 1, "/IFC");
	SIGNAL_DEFINE_N(srq_in_b, 1, "/SRQIN");
	SIGNAL_DEFINE_N(dav_out_b, 1, "/DAVOUT");

	SIGNAL_DEFINE(bus_di, 8);
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 0, 1), "DI1");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 1, 1), "DI2");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 2, 1), "DI3");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 3, 1), "DI4");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 4, 1), "DI5");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 5, 1), "DI6");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 6, 1), "DI7");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_di), 7, 1), "DI8");

	SIGNAL_DEFINE(bus_do, 8);
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 0, 1), "DO1");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 1, 1), "DO2");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 2, 1), "DO3");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 3, 1), "DO4");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 4, 1), "DO5");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 5, 1), "DO6");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 6, 1), "DO7");
	signal_set_name(SIGNAL_POOL, signal_split(SIGNAL(bus_do), 7, 1), "DO8");

	SIGNAL_DEFINE_N(cs1, 1, "CS1");

	//
	// signals - sheet 3: Cassette & Keyboard
	//

	SIGNAL_DEFINE_N(ca1, 1, "CA1");
	SIGNAL_DEFINE_N(graphic, 1, "GRAPHIC");

	SIGNAL_DEFINE(c5_portb, 8);
	SIGNAL(ndac_in_b) = signal_split(SIGNAL(c5_portb), 0, 1);
	SIGNAL(nrfd_out_b) = signal_split(SIGNAL(c5_portb), 1, 1);
	SIGNAL(atn_out_b) = signal_split(SIGNAL(c5_portb), 2, 1);
	SIGNAL(cass_write) = signal_split(SIGNAL(c5_portb), 3, 1);
	SIGNAL(cass_motor_2) = signal_split(SIGNAL(c5_portb), 4, 1);
	SIGNAL(video_on_2) = signal_split(SIGNAL(c5_portb), 5, 1);
	SIGNAL(nrfd_in_b) = signal_split(SIGNAL(c5_portb), 6, 1);
	SIGNAL(dav_in_b) = signal_split(SIGNAL(c5_portb), 7, 1);
	signal_set_name(SIGNAL_POOL, SIGNAL(ndac_in_b), "/NDACIN");
	signal_set_name(SIGNAL_POOL, SIGNAL(nrfd_out_b), "/NRFDOUT");
	signal_set_name(SIGNAL_POOL, SIGNAL(atn_out_b), "/ATNOUT");
	signal_set_name(SIGNAL_POOL, SIGNAL(cass_write), "CASSWRITE");
	signal_set_name(SIGNAL_POOL, SIGNAL(cass_motor_2), "CASSMOTOR2");
	signal_set_name(SIGNAL_POOL, SIGNAL(nrfd_in_b), "/NRFDIN");
	signal_set_name(SIGNAL_POOL, SIGNAL(dav_in_b), "/DAVIN");

	SIGNAL_DEFINE(c7_porta, 8);
	SIGNAL(keya) = signal_split(SIGNAL(c7_porta), 0, 1);
	SIGNAL(keyb) = signal_split(SIGNAL(c7_porta), 1, 1);
	SIGNAL(keyc) = signal_split(SIGNAL(c7_porta), 2, 1);
	SIGNAL(keyd) = signal_split(SIGNAL(c7_porta), 3, 1);
	SIGNAL(cass_switch_1) = signal_split(SIGNAL(c7_porta), 4, 1);
	SIGNAL(cass_switch_2) = signal_split(SIGNAL(c7_porta), 5, 1);
	SIGNAL(eoi_in_b) = signal_split(SIGNAL(c7_porta), 6, 1);
	SIGNAL(diag) = signal_split(SIGNAL(c7_porta), 7, 1);

	signal_set_name(SIGNAL_POOL, SIGNAL(keya), "KEYA");
	signal_set_name(SIGNAL_POOL, SIGNAL(keyb), "KEYB");
	signal_set_name(SIGNAL_POOL, SIGNAL(keyc), "KEYC");
	signal_set_name(SIGNAL_POOL, SIGNAL(keyd), "KEYD");
	signal_set_name(SIGNAL_POOL, SIGNAL(cass_switch_1), "CASSSWITCH1");
	signal_set_name(SIGNAL_POOL, SIGNAL(cass_switch_2), "CASSSWITCH2");
	signal_set_name(SIGNAL_POOL, SIGNAL(eoi_in_b), "/EOIIN");
	signal_set_name(SIGNAL_POOL, SIGNAL(diag), "DIAG");

	SIGNAL_DEFINE_N(cb2, 1, "CB2");
	SIGNAL_DEFINE_N(eoi_out_b, 1, "/EOIOUT");
	SIGNAL_DEFINE_N(cass_read_1, 1, "CASSREAD1");
	SIGNAL_DEFINE_N(cass_read_2, 1, "CASSREAD2");
	SIGNAL_DEFINE_N(cass_motor_1, 1, "CASSMOTOR1");

	SIGNAL_DEFINE_N(bus_pa, 8, "PA%d");
	SIGNAL_DEFINE_N(bus_kin, 8, "KIN%d");
	SIGNAL_DEFINE_N(bus_kout, 10, "KOUT%d");

	signal_default_uint8(SIGNAL_POOL, SIGNAL(bus_kin), 0xff);			// pull-up resistors R18-R25
	signal_default_bool(SIGNAL_POOL, SIGNAL(diag), true);

	//
	// signals - sheet 5: RAMS
	//

	SIGNAL_DEFINE_N(banksel, 1, "BANKSEL");
	SIGNAL_DEFINE_N(g78, 1, "G78");

	SIGNAL_DEFINE_N(bus_fa, 7, "FA%d");
	SIGNAL_DEFINE_N(bus_rd, 8, "RD%d");

	//
	// signals - sheet 6: Master timing
	//

	SIGNAL_DEFINE_BOOL_N(init_b, 1, ACTLO_ASSERT, "/INIT");
	SIGNAL_DEFINE_BOOL_N(init, 1, ACTHI_ASSERT, "INIT");

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
	SIGNAL_DEFINE_N(ra1_b, 1, "/RA1");
	SIGNAL_DEFINE_N(ra2, 1, "RA2");
	SIGNAL_DEFINE_N(ra3, 1, "RA3");
	SIGNAL_DEFINE_N(ra4, 1, "RA4");
	SIGNAL_DEFINE_N(ra5, 1, "RA5");
	SIGNAL_DEFINE_N(ra6, 1, "RA6");
	SIGNAL_DEFINE_N(ra6_b, 1, "/RA6");

	SIGNAL_DEFINE_N(ra1and3, 1, "RA1AND3");
	SIGNAL_DEFINE_N(ra4and6, 1, "RA4AND6");
	SIGNAL_DEFINE_N(ra5and6_b, 1, "RA5AND/6");

	SIGNAL_DEFINE_N(load_sr, 1, "LOADSR");
	SIGNAL_DEFINE_N(load_sr_b, 1, "/LOADSR");

	SIGNAL_DEFINE_N(horz_disp_on, 1, "HORZDISPON");
	SIGNAL_DEFINE_N(horz_disp_off, 1, "HORZDISPOFF");
	SIGNAL_DEFINE_N(horz_drive, 1, "HORZDRIVE");
	SIGNAL_DEFINE_N(horz_drive_b, 1, "/HORZDRIVE");

	SIGNAL_DEFINE_N(h8q, 1, "H8Q");
	SIGNAL_DEFINE_N(h8q_b, 1, "/H8Q");
	SIGNAL_DEFINE_N(h8q2, 1, "H8Q2");
	SIGNAL_DEFINE_N(h8q2_b, 1, "/H8Q2");

	SIGNAL_DEFINE_N(video_latch, 1, "VIDEOLATCH");
	SIGNAL_DEFINE_N(vert_drive, 1, "VERTDRIVE");

	SIGNAL_DEFINE_N(h53, 1, "H53");
	SIGNAL_DEFINE_N(h4y1, 1, "H4Y1");
	SIGNAL_DEFINE_N(muxa, 1, "MUXA");
	SIGNAL_DEFINE_N(h4y4, 1, "H4Y4");

	SIGNAL_DEFINE_N(h1q1, 1, "H1Q1");
	SIGNAL_DEFINE_N(h1q1_b, 1, "/H1Q1");
	SIGNAL_DEFINE_N(h1q2, 1, "H1Q2");
	SIGNAL_DEFINE_N(h1q2_b, 1, "/H1Q2");

	SIGNAL_DEFINE_N(ras0_b, 1, "/RAS0");
	SIGNAL_DEFINE_N(cas0_b, 1, "/CAS0");
	SIGNAL_DEFINE_N(cas1_b, 1, "/CAS1");
	SIGNAL_DEFINE_N(ba14_b, 1, "/BA14");

	SIGNAL_DEFINE_N(video_on, 1, "VIDEOON");

	//
	// signals - sheet 7: display logic
	//

	SIGNAL_DEFINE_N(tv_sel, 1, "TVSEL");
	SIGNAL_DEFINE_N(tv_read_b, 1, "/TVREAD");
	SIGNAL_DEFINE_N(g6_q, 1, "G6Q");
	SIGNAL_DEFINE_N(g6_q_b, 1, "/G6Q");

	SIGNAL_DEFINE_N(tv_ram_rw, 1, "TVRAMRW");
	SIGNAL_DEFINE_N(f6_y3, 1, "F6Y3");
	SIGNAL_DEFINE_N(bus_sa, 10, "SA%d");

	SIGNAL_DEFINE_N(ga2, 1, "GA2");
	SIGNAL_DEFINE_N(ga3, 1, "GA3");
	SIGNAL_DEFINE_N(ga4, 1, "GA4");
	SIGNAL_DEFINE_N(ga5, 1, "GA5");
	SIGNAL_DEFINE_N(ga6, 1, "GA6");
	SIGNAL_DEFINE_N(ga7, 1, "GA7");
	SIGNAL_DEFINE_N(ga8, 1, "GA8");
	SIGNAL_DEFINE_N(ga9, 1, "GA9");

	SIGNAL_DEFINE_N(lga2, 1, "LGA2");
	SIGNAL_DEFINE_N(lga3, 1, "LGA3");
	SIGNAL_DEFINE_N(lga4, 1, "LGA4");
	SIGNAL_DEFINE_N(lga5, 1, "LGA5");
	SIGNAL_DEFINE_N(lga6, 1, "LGA6");
	SIGNAL_DEFINE_N(lga7, 1, "LGA7");
	SIGNAL_DEFINE_N(lga8, 1, "LGA8");
	SIGNAL_DEFINE_N(lga9, 1, "LGA9");

	SIGNAL_DEFINE_BOOL_N(next, 1, true, "NEXT");
	SIGNAL_DEFINE_BOOL_N(next_b, 1, false, "/NEXT");

	SIGNAL_DEFINE_N(reload_next, 1, "RELOADNEXT");

	SIGNAL_DEFINE_BOOL_N(pullup_2, 1, true, "PULLUP2");

	SIGNAL_DEFINE_N(lines_20_b, 1, "/20LINES");
	SIGNAL_DEFINE_N(lines_200_b, 1, "/200LINES");
	SIGNAL_DEFINE_N(line_220, 1, "220LINE");
	SIGNAL_DEFINE_N(lga_hi_b, 1, "/LGAHI");
	SIGNAL_DEFINE_N(lga_hi, 1, "LGAHI");
	SIGNAL_DEFINE_N(w220_off, 1, "220OFF");

	SIGNAL_DEFINE_N(video_on_b, 1, "/VIDEOON");

	//
	// signals - sheet 8: display rams
	//

	SIGNAL_DEFINE_N(ra7, 1, "RA7");
	SIGNAL_DEFINE_N(ra8, 1, "RA8");
	SIGNAL_DEFINE_N(ra9, 1, "RA9");
	SIGNAL_DEFINE_N(reload_b, 1, "/RELOAD");
	SIGNAL_DEFINE_BOOL_N(pullup_1, 1, true, "PULLUP1");

	SIGNAL_DEFINE_N(bus_sd, 8, "SD%d");
	SIGNAL_DEFINE_N(bus_lsd, 8, "LSD%d");
	SIGNAL_DEFINE_N(bus_cd, 8, "CD%d");

	SIGNAL_DEFINE_N(g9q, 1, "G9Q");
	SIGNAL_DEFINE_N(g9q_b, 1, "/G9Q");
	SIGNAL_DEFINE_N(e11qh, 1, "E11QH");
	SIGNAL_DEFINE_N(e11qh_b, 1, "/E11QH");
	SIGNAL_DEFINE_N(g106, 1, "G106");
	SIGNAL_DEFINE_N(g108, 1, "G108");
	SIGNAL_DEFINE_N(h108, 1, "H108");

	SIGNAL_DEFINE_N(video, 1, "VIDEO");

	//////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// components
	//

	// sheet 01: microprocessor & memory expansion
	circuit_create_01(device);

	// sheet 02: IEEE-488 Interface
	circuit_create_02(device);

	// sheet 03: cassette & keyboard
	circuit_create_03(device);

	// sheet 04: ROMS
	circuit_create_04(device);

	// sheet 05: RAMS
	if (!lite) {
		circuit_create_05(device);
	} else {
		circuit_lite_create_ram(device);
	}

	// sheet 06: master timing
	circuit_create_06(device);

	// sheet 07: display logic components
	circuit_create_07(device);

	// sheet 08: display rams components
	circuit_create_08(device);

	// peripherals
	circuit_create_pheriperals(device);

	// let the simulator know no more chips will be added
	simulator_device_complete(device->simulator);

	return device;
}

DevCommodorePet *dev_commodore_pet_create(void) {
	return create_pet_device(false);
}

DevCommodorePet *dev_commodore_pet_lite_create(void) {
	return create_pet_device(true);
}

void dev_commodore_pet_destroy(DevCommodorePet *device) {
	assert(device);

	simulator_destroy(device->simulator);
	free(device);
}

void dev_commodore_pet_process_clk1(DevCommodorePet *device) {
	bool prev_clk1 = SIGNAL_BOOL(clk1);
	do {
		device->process(device);
	} while (prev_clk1 == SIGNAL_BOOL(clk1));
}

void dev_commodore_pet_reset(DevCommodorePet *device) {
	assert(device);
	device->in_reset = true;
}

void dev_commodore_pet_read_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *output) {
	assert(device);
	assert(output);

	// note: this function skips the free rom slots
	static const size_t	REGION_START[] = {0x0000, 0x4000, 0x8000, 0x8800, 0xb000, 0xc000, 0xd000, 0xe000, 0xe800, 0xf000};
	static const size_t REGION_SIZE[]  = {0x4000, 0x4000, 0x0800, 0x2800, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x1000};
	static const int NUM_REGIONS = sizeof(REGION_START) / sizeof(REGION_START[0]);

	if (start_address > 0xffff) {
		memset(output, 0, size);
		return;
	}

	// find start region
	int sr = NUM_REGIONS - 1;
	while (start_address < REGION_START[sr] && sr > 0) {
		sr -= 1;
	}

	size_t remain = size;
	size_t done = 0;
	size_t addr = start_address;

	for (int region = sr; remain > 0 && addr <= 0xffff; ++region) {
		size_t region_offset = addr - REGION_START[region];
		size_t to_copy = MIN(remain, REGION_SIZE[region] - region_offset);

		switch (region) {
			case 0:	{			// RAM (0-16k)
				Chip8x4116DRam *ram = (Chip8x4116DRam *) simulator_chip_by_name(device->simulator, "I2-9");
				for (size_t i = 0; i < to_copy; ++i) {
					size_t row = (region_offset + i) & 0x007f;
					size_t col = ((region_offset + i) & 0x3f80) >> 7;
					size_t physical = (row << 7) | ((col & 0x003f) << 1) | ((col & 0x0040) >> 6);
					output[done+i] = ram->data_array[physical];
				}
				break;
			}
			case 1:	{			// RAM (16-32k)
				Chip8x4116DRam *ram = (Chip8x4116DRam *) simulator_chip_by_name(device->simulator, "J2-9");
				for (size_t i = 0; i < to_copy; ++i) {
					size_t row = (region_offset + i) & 0x007f;
					size_t col = ((region_offset + i) & 0x3f80) >> 7;
					size_t physical = (row << 7) | ((col & 0x003f) << 1) | ((col & 0x0040) >> 6);
					output[done+i] = ram->data_array[physical];
				}
				break;
			}
			case 2:	{			// Screen RAM
				Chip6114SRam *ram_hi = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F7");
				Chip6114SRam *ram_lo = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F8");

				for (size_t i = 0; i < to_copy; ++i) {
					output[done + i] = (uint8_t) (((ram_hi->data_array[region_offset + i] & 0xf) << 4) |
												   (ram_lo->data_array[region_offset + i] & 0xf));
				}
				break;
			}
			case 3:				// unused space between video memory and first rom
			case 8:				// I/O area (pia-1, pia-2, via)
				memset(output + done, 0, to_copy);
				break;
			case 4:				// basic-rom 1
			case 5:				// basic-rom 2
			case 6:				// basic-rom 3
			case 7:				// editor rom
				memcpy(output + done, device->roms[region-4]->data_array + region_offset, to_copy);
				break;
			case 9:				// kernal rom
				memcpy(output + done, device->roms[4]->data_array + region_offset, to_copy);
				break;

		}

		remain -= to_copy;
		addr += to_copy;
		done += to_copy;
	}
}

void dev_commodore_pet_write_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *input) {
	assert(device);
	assert(input);

	// note: this function only allows writes to the main RAM and video memory
	static const size_t	REGION_START[] = {0x0000, 0x4000, 0x8000};
	static const size_t REGION_SIZE[]  = {0x4000, 0x4000, 0x0800};
	static const int NUM_REGIONS = sizeof(REGION_START) / sizeof(REGION_START[0]);

	if (start_address > 0x87ff) {
		return;
	}

	// find start region
	int sr = NUM_REGIONS - 1;
	while (start_address < REGION_START[sr] && sr > 0) {
		sr -= 1;
	}

	size_t remain = size;
	size_t done = 0;
	size_t addr = start_address;

	for (int region = sr; remain > 0 && addr < 0x8800; ++region) {
		size_t region_offset = addr - REGION_START[region];
		size_t to_copy = MIN(remain, REGION_SIZE[region] - region_offset);

		switch (region) {
			case 0:	{			// RAM (0-16k)
				Chip8x4116DRam *ram = (Chip8x4116DRam *) simulator_chip_by_name(device->simulator, "I2-9");
				for (size_t i = 0; i < to_copy; ++i) {
					size_t row = (region_offset + i) & 0x007f;
					size_t col = ((region_offset + i) & 0x3f80) >> 7;
					size_t physical = (row << 7) | ((col & 0x003f) << 1) | ((col & 0x0040) >> 6);
					ram->data_array[physical] = input[done+i];
				}
				break;
			}
			case 1:	{			// RAM (16-32k)
				Chip8x4116DRam *ram = (Chip8x4116DRam *) simulator_chip_by_name(device->simulator, "J2-9");
				for (size_t i = 0; i < to_copy; ++i) {
					size_t row = (region_offset + i) & 0x007f;
					size_t col = ((region_offset + i) & 0x3f80) >> 7;
					size_t physical = (row << 7) | ((col & 0x003f) << 1) | ((col & 0x0040) >> 6);
					ram->data_array[physical] = input[done+i];
				}
				break;
			}
			case 2:	{			// Screen RAM
				Chip6114SRam *ram_hi = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F7");
				Chip6114SRam *ram_lo = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F8");

				for (size_t i = 0; i < to_copy; ++i) {
					ram_hi->data_array[region_offset+i] = (input[done+i] & 0xf0) >> 4;
					ram_lo->data_array[region_offset+i] = (input[done+i] & 0x0f);
				}
				break;
			}
		}

		remain -= to_copy;
		addr += to_copy;
		done += to_copy;
	}
}


void dev_commodore_pet_lite_read_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *output) {
	assert(device);
	assert(output);

	// note: this function skips the free rom slots
	static const size_t	REGION_START[] = {0x0000, 0x8000, 0x8800, 0xb000, 0xc000, 0xd000, 0xe000, 0xe800, 0xf000};
	static const size_t REGION_SIZE[]  = {0x8000, 0x0800, 0x2800, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x1000};
	static const int NUM_REGIONS = sizeof(REGION_START) / sizeof(REGION_START[0]);

	if (start_address > 0xffff) {
		memset(output, 0, size);
		return;
	}

	// find start region
	int sr = NUM_REGIONS - 1;
	while (start_address < REGION_START[sr] && sr > 0) {
		sr -= 1;
	}

	size_t remain = size;
	size_t done = 0;
	size_t addr = start_address;

	for (int region = sr; remain > 0 && addr <= 0xffff; ++region) {
		size_t region_offset = addr - REGION_START[region];
		size_t to_copy = MIN(remain, REGION_SIZE[region] - region_offset);

		switch (region) {
			case 0:	{			// RAM (0-32k)
				Ram8d16a *ram = (Ram8d16a *) simulator_chip_by_name(device->simulator, "RAM");
				memcpy(output + done, ram->data_array + region_offset, to_copy);
				break;
			}
			case 1:	{			// Screen RAM
				Chip6114SRam *ram_hi = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F7");
				Chip6114SRam *ram_lo = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F8");

				for (size_t i = 0; i < to_copy; ++i) {
					output[done + i] = (uint8_t) (((ram_hi->data_array[region_offset + i] & 0xf) << 4) |
												   (ram_lo->data_array[region_offset + i] & 0xf));
				}
				break;
			}
			case 2:				// unused space between video memory and first rom
			case 7:				// I/O area (pia-1, pia-2, via)
				memset(output + done, 0, to_copy);
				break;
			case 3:				// basic-rom 1
			case 4:				// basic-rom 2
			case 5:				// basic-rom 3
			case 6:				// editor rom
				memcpy(output + done, device->roms[region-4]->data_array + region_offset, to_copy);
				break;
			case 8:				// kernal rom
				memcpy(output + done, device->roms[4]->data_array + region_offset, to_copy);
				break;
		}

		remain -= to_copy;
		addr += to_copy;
		done += to_copy;
	}
}

void dev_commodore_pet_lite_write_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *input) {
	assert(device);
	assert(input);

	// note: this function only allows writes to the main RAM and video memory
	static const size_t	REGION_START[] = {0x0000, 0x8000};
	static const size_t REGION_SIZE[]  = {0x8000, 0x0800};
	static const int NUM_REGIONS = sizeof(REGION_START) / sizeof(REGION_START[0]);

	if (start_address > 0x87ff) {
		return;
	}

	// find start region
	int sr = NUM_REGIONS - 1;
	while (start_address < REGION_START[sr] && sr > 0) {
		sr -= 1;
	}

	size_t remain = size;
	size_t done = 0;
	size_t addr = start_address;

	for (int region = sr; remain > 0 && addr < 0x8800; ++region) {
		size_t region_offset = addr - REGION_START[region];
		size_t to_copy = MIN(remain, REGION_SIZE[region] - region_offset);

		switch (region) {
			case 0:	{			// RAM (0-32k)
				Ram8d16a *ram = (Ram8d16a *) simulator_chip_by_name(device->simulator, "RAM");
				memcpy(ram->data_array + region_offset, input + done, to_copy);
				break;
			}
			case 1:	{			// Screen RAM
				Chip6114SRam *ram_hi = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F7");
				Chip6114SRam *ram_lo = (Chip6114SRam *) simulator_chip_by_name(device->simulator, "F8");

				for (size_t i = 0; i < to_copy; ++i) {
					ram_hi->data_array[region_offset+i] = (input[done+i] & 0xf0) >> 4;
					ram_lo->data_array[region_offset+i] = (input[done+i] & 0x0f);
				}
				break;
			}
		}

		remain -= to_copy;
		addr += to_copy;
		done += to_copy;
	}
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


	device->write_memory(device, ram_offset, prg_size - 2, (uint8_t *) prg_buffer + 2);

	arrfree(prg_buffer);
	return true;
}
