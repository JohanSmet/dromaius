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
#include "perif_pet_crt.h"
#include "perif_datassette_1530.h"
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
static void glue_logic_register_dependencies_03(ChipGlueLogic *chip);
static void glue_logic_process_03(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_05(ChipGlueLogic *chip);
static void glue_logic_process_05(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_06(ChipGlueLogic *chip);
static void glue_logic_process_06(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_07(ChipGlueLogic *chip);
static void glue_logic_process_07(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_08(ChipGlueLogic *chip);
static void glue_logic_process_08(ChipGlueLogic *chip);

static void glue_logic_process_07_lite(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_07_lite(ChipGlueLogic *chip);

static ChipGlueLogic *glue_logic_create(DevCommodorePet *device, int sheet) {
	ChipGlueLogic *chip = (ChipGlueLogic *) calloc(1, sizeof(ChipGlueLogic));
	chip->device = device;

	if (sheet == 1) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_01, glue_logic_destroy, glue_logic_register_dependencies_01);
	} else if (sheet == 3) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_03, glue_logic_destroy, glue_logic_register_dependencies_03);
	} else if (sheet == 5) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_05, glue_logic_destroy, glue_logic_register_dependencies_05);
	} else if (sheet == 6) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_06, glue_logic_destroy, glue_logic_register_dependencies_06);
	} else if (sheet == 7) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_07, glue_logic_destroy, glue_logic_register_dependencies_07);
	} else if (sheet == 8) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_08, glue_logic_destroy, glue_logic_register_dependencies_08);
	} else if (sheet == 17) {
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_07_lite, glue_logic_destroy, glue_logic_register_dependencies_07_lite);
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

	signal_add_dependency(SIGNAL_POOL, device->pia_1->signals[CHIP_6520_IRQA_B], chip->id);
	signal_add_dependency(SIGNAL_POOL, device->pia_1->signals[CHIP_6520_IRQB_B], chip->id);
	signal_add_dependency(SIGNAL_POOL, device->pia_2->signals[CHIP_6520_IRQA_B], chip->id);
	signal_add_dependency(SIGNAL_POOL, device->pia_2->signals[CHIP_6520_IRQB_B], chip->id);
	signal_add_dependency(SIGNAL_POOL, device->via->signals[CHIP_6522_IRQ_B], chip->id);
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
	bool irq_b = signal_read_next_bool(SIGNAL_POOL, device->pia_1->signals[CHIP_6520_IRQA_B]) &&
				 signal_read_next_bool(SIGNAL_POOL, device->pia_1->signals[CHIP_6520_IRQB_B]) &&
				 signal_read_next_bool(SIGNAL_POOL, device->pia_2->signals[CHIP_6520_IRQA_B]) &&
				 signal_read_next_bool(SIGNAL_POOL, device->pia_2->signals[CHIP_6520_IRQB_B]) &&
				 signal_read_next_bool(SIGNAL_POOL, device->via->signals[CHIP_6522_IRQ_B]);
	SIGNAL_SET_BOOL(irq_b, irq_b);
}

// glue-logic: cassette & keyboard

static void glue_logic_register_dependencies_03(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	signal_add_dependency(SIGNAL_POOL, SIGNAL(cass_motor_1_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(cass_motor_2_b), chip->id);
}

static void glue_logic_process_03(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_SET_BOOL(cass_motor_1, !SIGNAL_BOOL(cass_motor_1_b));
	SIGNAL_SET_BOOL(cass_motor_2, !SIGNAL_BOOL(cass_motor_2_b));
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

static void glue_logic_register_dependencies_07_lite(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	signal_add_dependency(SIGNAL_POOL, SIGNAL(ba11_b), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(sel8), chip->id);
	signal_add_dependency(SIGNAL_POOL, SIGNAL(buf_rw), chip->id);
}

static void glue_logic_process_07_lite(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// >> F1 (4,5,6)
	bool tv_sel = SIGNAL_BOOL(ba11_b) && SIGNAL_BOOL(sel8);
	SIGNAL_SET_BOOL(tv_sel, tv_sel);

	// >> A5 (8,9,10,11)
	bool tv_read_b = !(true && SIGNAL_BOOL(buf_rw) && tv_sel);
	SIGNAL_SET_BOOL(tv_read_b, tv_read_b);
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
// internal - pet-lite 'fake' display
//

typedef struct ChipLiteDisplay {
	CHIP_DECLARE_FUNCTIONS

	DevCommodorePet *	device;
	Ram8d16a *			vram;
	uint8_t				char_rom[ROM_6316_DATA_SIZE];

	int64_t				refresh_delay;
	int64_t				retrace_hold;
} ChipLiteDisplay;

static void lite_display_destroy(ChipLiteDisplay *chip);
static void lite_display_register_dependencies(ChipLiteDisplay *chip);
static void lite_display_process(ChipLiteDisplay *chip);

static ChipLiteDisplay *lite_display_create(DevCommodorePet *device) {
	ChipLiteDisplay *chip = (ChipLiteDisplay *) calloc(1, sizeof(ChipLiteDisplay));

	CHIP_SET_FUNCTIONS(chip, lite_display_process, lite_display_destroy, lite_display_register_dependencies);
	chip->device = device;
	chip->vram = (Ram8d16a *) simulator_chip_by_name(device->simulator, "VRAM");

	file_load_binary_fixed("runtime/commodore_pet/characters-2.901447-10.bin", chip->char_rom, ROM_6316_DATA_SIZE);

	chip->refresh_delay = simulator_interval_to_tick_count(device->simulator, FREQUENCY_TO_PS(60));
	chip->retrace_hold = simulator_interval_to_tick_count(device->simulator, US_TO_PS(1));

	return chip;
}

static void lite_display_destroy(ChipLiteDisplay *chip) {
	assert(chip);
	free(chip);
}

static void lite_display_register_dependencies(ChipLiteDisplay *chip) {
	(void) chip;
}

void pet_lite_fake_display(ChipLiteDisplay *chip) {
	DevCommodorePet *device = chip->device;

	const uint32_t COLOR = 0xff55ff55;

	const size_t SCREEN_WIDTH = 40;
	const size_t SCREEN_HEIGHT = 25;
	const size_t CHAR_HEIGHT = 8;

	for (size_t pos_char_y = 0; pos_char_y < SCREEN_HEIGHT; ++pos_char_y) {
		uint8_t *vram_row = chip->vram->data_array + (pos_char_y * SCREEN_WIDTH);
		for (uint8_t pos_line = 0; pos_line < 8; ++pos_line) {
			uint32_t *screen_ptr = device->screen->frame + ((pos_char_y * CHAR_HEIGHT + pos_line) * device->screen->width);

			for (size_t pos_char_x = 0; pos_char_x < SCREEN_WIDTH; ++pos_char_x) {
				// get character to display & check for inverted char
				uint8_t value = vram_row[pos_char_x];
				unsigned int invert = (value >> 7) & 0x1;
				value &= 0x7f;

				// get approriate data from the character rom
				int rom_addr = (int) value << 3 | pos_line;
				uint8_t line_value = chip->char_rom[rom_addr];

				// write character line to screen
				for (int i = 7; i >= 0; --i) {
					*screen_ptr++ = COLOR * (unsigned int) (((line_value >> i) & 0x1) ^ invert) | 0xff000000;
				}
			}
		}
	}
}

static void lite_display_process(ChipLiteDisplay *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	if (SIGNAL_BOOL(video_on)) {
		// redraw screen
		pet_lite_fake_display(chip);

		// signal vertical retrace
		SIGNAL_SET_BOOL(video_on, false);

		// hold vertical retrace for a bit
		chip->schedule_timestamp = chip->simulator->current_tick + chip->retrace_hold;
	} else {
		// end of vertical retrace
		SIGNAL_SET_BOOL(video_on, true);
		chip->schedule_timestamp = chip->simulator->current_tick + chip->refresh_delay;
	}
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

	if (file_load_binary_fixed(filename, rom->data_array, (num_lines == 12) ? ROM_6332_DATA_SIZE : ROM_6316_DATA_SIZE) == 0) {
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
											[CHIP_POR_TRIGGER_B] = SIGNAL(reset_btn_b),
											[CHIP_POR_RESET_B] = SIGNAL(reset_b)
	}));

	// cpu
	device->cpu = cpu_6502_create(device->simulator, (Cpu6502Signals) {
										[PIN_6502_AB0]  = signal_split(SIGNAL(cpu_bus_address), 0, 1),
										[PIN_6502_AB1]  = signal_split(SIGNAL(cpu_bus_address), 1, 1),
										[PIN_6502_AB2]  = signal_split(SIGNAL(cpu_bus_address), 2, 1),
										[PIN_6502_AB3]  = signal_split(SIGNAL(cpu_bus_address), 3, 1),
										[PIN_6502_AB4]  = signal_split(SIGNAL(cpu_bus_address), 4, 1),
										[PIN_6502_AB5]  = signal_split(SIGNAL(cpu_bus_address), 5, 1),
										[PIN_6502_AB6]  = signal_split(SIGNAL(cpu_bus_address), 6, 1),
										[PIN_6502_AB7]  = signal_split(SIGNAL(cpu_bus_address), 7, 1),
										[PIN_6502_AB8]  = signal_split(SIGNAL(cpu_bus_address), 8, 1),
										[PIN_6502_AB9]  = signal_split(SIGNAL(cpu_bus_address), 9, 1),
										[PIN_6502_AB10] = signal_split(SIGNAL(cpu_bus_address), 10, 1),
										[PIN_6502_AB11] = signal_split(SIGNAL(cpu_bus_address), 11, 1),
										[PIN_6502_AB12] = signal_split(SIGNAL(cpu_bus_address), 12, 1),
										[PIN_6502_AB13] = signal_split(SIGNAL(cpu_bus_address), 13, 1),
										[PIN_6502_AB14] = signal_split(SIGNAL(cpu_bus_address), 14, 1),
										[PIN_6502_AB15] = signal_split(SIGNAL(cpu_bus_address), 15, 1),

										[PIN_6502_DB0]  = signal_split(SIGNAL(cpu_bus_data), 0, 1),
										[PIN_6502_DB1]  = signal_split(SIGNAL(cpu_bus_data), 1, 1),
										[PIN_6502_DB2]  = signal_split(SIGNAL(cpu_bus_data), 2, 1),
										[PIN_6502_DB3]  = signal_split(SIGNAL(cpu_bus_data), 3, 1),
										[PIN_6502_DB4]  = signal_split(SIGNAL(cpu_bus_data), 4, 1),
										[PIN_6502_DB5]  = signal_split(SIGNAL(cpu_bus_data), 5, 1),
										[PIN_6502_DB6]  = signal_split(SIGNAL(cpu_bus_data), 6, 1),
										[PIN_6502_DB7]  = signal_split(SIGNAL(cpu_bus_data), 7, 1),

										[PIN_6502_CLK]   = SIGNAL(clk1),
										[PIN_6502_RES_B] = SIGNAL(reset_b),
										[PIN_6502_RW]	 = SIGNAL(cpu_rw),
										[PIN_6502_IRQ_B] = SIGNAL(irq_b),
										[PIN_6502_NMI_B] = SIGNAL(nmi_b),
										[PIN_6502_SYNC]  = SIGNAL(cpu_sync),
										[PIN_6502_RDY]   = SIGNAL(cpu_rdy)
	});
	DEVICE_REGISTER_CHIP("C4", device->cpu);

	// >> c3 - octal buffer
	DEVICE_REGISTER_CHIP("C3", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(low),
										[CHIP_74244_G2_B] = SIGNAL(low),
										[CHIP_74244_A11] = signal_split(SIGNAL(cpu_bus_address), 0, 1),
										[CHIP_74244_A24] = signal_split(SIGNAL(cpu_bus_address), 1, 1),
										[CHIP_74244_A12] = signal_split(SIGNAL(cpu_bus_address), 2, 1),
										[CHIP_74244_A23] = signal_split(SIGNAL(cpu_bus_address), 3, 1),
										[CHIP_74244_A13] = signal_split(SIGNAL(cpu_bus_address), 4, 1),
										[CHIP_74244_A22] = signal_split(SIGNAL(cpu_bus_address), 5, 1),
										[CHIP_74244_A14] = signal_split(SIGNAL(cpu_bus_address), 6, 1),
										[CHIP_74244_A21] = signal_split(SIGNAL(cpu_bus_address), 7, 1),
										[CHIP_74244_Y11] = signal_split(SIGNAL(bus_ba), 0, 1),
										[CHIP_74244_Y24] = signal_split(SIGNAL(bus_ba), 1, 1),
										[CHIP_74244_Y12] = signal_split(SIGNAL(bus_ba), 2, 1),
										[CHIP_74244_Y23] = signal_split(SIGNAL(bus_ba), 3, 1),
										[CHIP_74244_Y13] = signal_split(SIGNAL(bus_ba), 4, 1),
										[CHIP_74244_Y22] = signal_split(SIGNAL(bus_ba), 5, 1),
										[CHIP_74244_Y14] = signal_split(SIGNAL(bus_ba), 6, 1),
										[CHIP_74244_Y21] = signal_split(SIGNAL(bus_ba), 7, 1),

	}));

	// >> b3 - octal buffer
	DEVICE_REGISTER_CHIP("B3", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(low),									// 01
										[CHIP_74244_G2_B] = SIGNAL(low),									// 19
										[CHIP_74244_A11]  = signal_split(SIGNAL(cpu_bus_address), 8, 1),	// 02
										[CHIP_74244_A24]  = signal_split(SIGNAL(cpu_bus_address), 9, 1),	// 17
										[CHIP_74244_A12]  = signal_split(SIGNAL(cpu_bus_address), 10, 1),	// 04
										[CHIP_74244_A23]  = signal_split(SIGNAL(cpu_bus_address), 11, 1),	// 15
										[CHIP_74244_A13]  = signal_split(SIGNAL(cpu_bus_address), 12, 1),	// 06
										[CHIP_74244_A22]  = signal_split(SIGNAL(cpu_bus_address), 13, 1),	// 13
										[CHIP_74244_A14]  = signal_split(SIGNAL(cpu_bus_address), 14, 1),	// 08
										[CHIP_74244_A21]  = signal_split(SIGNAL(cpu_bus_address), 15, 1),	// 11
										[CHIP_74244_Y11]  = signal_split(SIGNAL(bus_ba), 8, 1),				// 18
										[CHIP_74244_Y24]  = signal_split(SIGNAL(bus_ba), 9, 1),				// 03
										[CHIP_74244_Y12]  = signal_split(SIGNAL(bus_ba), 10, 1),			// 16
										[CHIP_74244_Y23]  = signal_split(SIGNAL(bus_ba), 11, 1),			// 05
										[CHIP_74244_Y13]  = signal_split(SIGNAL(bus_ba), 12, 1),			// 14
										[CHIP_74244_Y22]  = signal_split(SIGNAL(bus_ba), 13, 1),			// 07
										[CHIP_74244_Y14]  = signal_split(SIGNAL(bus_ba), 14, 1),			// 12
										[CHIP_74244_Y21]  = signal_split(SIGNAL(bus_ba), 15, 1),			// 09

	}));

	// >> d2 - 4-to-16 decoder
	DEVICE_REGISTER_CHIP("D2", chip_74154_decoder_create(device->simulator, (Chip74154Signals) {
										[CHIP_74154_G1_B] = SIGNAL(low),
										[CHIP_74154_G2_B] = SIGNAL(low),
										[CHIP_74154_A]  = signal_split(SIGNAL(bus_ba), 12, 1),
										[CHIP_74154_B]  = signal_split(SIGNAL(bus_ba), 13, 1),
										[CHIP_74154_C]  = signal_split(SIGNAL(bus_ba), 14, 1),
										[CHIP_74154_D]  = signal_split(SIGNAL(bus_ba), 15, 1),
										[CHIP_74154_Y0_B] = SIGNAL(sel0_b),
										[CHIP_74154_Y1_B] = SIGNAL(sel1_b),
										[CHIP_74154_Y2_B] = SIGNAL(sel2_b),
										[CHIP_74154_Y3_B] = SIGNAL(sel3_b),
										[CHIP_74154_Y4_B] = SIGNAL(sel4_b),
										[CHIP_74154_Y5_B] = SIGNAL(sel5_b),
										[CHIP_74154_Y6_B] = SIGNAL(sel6_b),
										[CHIP_74154_Y7_B] = SIGNAL(sel7_b),
										[CHIP_74154_Y8_B] = SIGNAL(sel8_b),
										[CHIP_74154_Y9_B] = SIGNAL(sel9_b),
										[CHIP_74154_Y10_B] = SIGNAL(sela_b),
										[CHIP_74154_Y11_B] = SIGNAL(selb_b),
										[CHIP_74154_Y12_B] = SIGNAL(selc_b),
										[CHIP_74154_Y13_B] = SIGNAL(seld_b),
										[CHIP_74154_Y14_B] = SIGNAL(sele_b),
										[CHIP_74154_Y15_B] = SIGNAL(self_b),
	}));

	// >> e9 - octal buffer
	DEVICE_REGISTER_CHIP("E9", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(ram_write_b),							// 01
										[CHIP_74244_G2_B] = SIGNAL(ram_read_b),								// 19
										[CHIP_74244_A11]  = signal_split(SIGNAL(cpu_bus_data), 0, 1),		// 02
										[CHIP_74244_Y24]  = signal_split(SIGNAL(cpu_bus_data), 0, 1),		// 03
										[CHIP_74244_A12]  = signal_split(SIGNAL(cpu_bus_data), 1, 1),		// 04
										[CHIP_74244_Y23]  = signal_split(SIGNAL(cpu_bus_data), 1, 1),		// 05
										[CHIP_74244_A13]  = signal_split(SIGNAL(cpu_bus_data), 2, 1),		// 06
										[CHIP_74244_Y22]  = signal_split(SIGNAL(cpu_bus_data), 2, 1),		// 07
										[CHIP_74244_A14]  = signal_split(SIGNAL(cpu_bus_data), 3, 1),		// 08
										[CHIP_74244_Y21]  = signal_split(SIGNAL(cpu_bus_data), 3, 1),		// 09

										[CHIP_74244_Y11]  = signal_split(SIGNAL(bus_bd), 0, 1),				// 18
										[CHIP_74244_A24]  = signal_split(SIGNAL(bus_bd), 0, 1),				// 17
										[CHIP_74244_Y12]  = signal_split(SIGNAL(bus_bd), 1, 1),				// 16
										[CHIP_74244_A23]  = signal_split(SIGNAL(bus_bd), 1, 1),				// 15
										[CHIP_74244_Y13]  = signal_split(SIGNAL(bus_bd), 2, 1),				// 14
										[CHIP_74244_A22]  = signal_split(SIGNAL(bus_bd), 2, 1),				// 13
										[CHIP_74244_Y14]  = signal_split(SIGNAL(bus_bd), 3, 1),				// 12
										[CHIP_74244_A21]  = signal_split(SIGNAL(bus_bd), 3, 1)				// 11
	}));

	// >> e10 - octal buffer
	DEVICE_REGISTER_CHIP("E10", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(ram_write_b),							// 01
										[CHIP_74244_G2_B] = SIGNAL(ram_read_b),								// 19
										[CHIP_74244_A11]  = signal_split(SIGNAL(cpu_bus_data), 4, 1),		// 02
										[CHIP_74244_Y24]  = signal_split(SIGNAL(cpu_bus_data), 4, 1),		// 03
										[CHIP_74244_A12]  = signal_split(SIGNAL(cpu_bus_data), 5, 1),		// 04
										[CHIP_74244_Y23]  = signal_split(SIGNAL(cpu_bus_data), 5, 1),		// 05
										[CHIP_74244_A13]  = signal_split(SIGNAL(cpu_bus_data), 6, 1),		// 06
										[CHIP_74244_Y22]  = signal_split(SIGNAL(cpu_bus_data), 6, 1),		// 07
										[CHIP_74244_A14]  = signal_split(SIGNAL(cpu_bus_data), 7, 1),		// 08
										[CHIP_74244_Y21]  = signal_split(SIGNAL(cpu_bus_data), 7, 1),		// 09

										[CHIP_74244_Y11]  = signal_split(SIGNAL(bus_bd), 4, 1),				// 18
										[CHIP_74244_A24]  = signal_split(SIGNAL(bus_bd), 4, 1),				// 17
										[CHIP_74244_Y12]  = signal_split(SIGNAL(bus_bd), 5, 1),				// 16
										[CHIP_74244_A23]  = signal_split(SIGNAL(bus_bd), 5, 1),				// 15
										[CHIP_74244_Y13]  = signal_split(SIGNAL(bus_bd), 6, 1),				// 14
										[CHIP_74244_A22]  = signal_split(SIGNAL(bus_bd), 6, 1),				// 13
										[CHIP_74244_Y14]  = signal_split(SIGNAL(bus_bd), 7, 1),				// 12
										[CHIP_74244_A21]  = signal_split(SIGNAL(bus_bd), 7, 1)				// 11
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC1", glue_logic_create(device, 1));
}

// sheet 02: IEEE-488 Interface
void circuit_create_02(DevCommodorePet *device) {

	// pia 1 (C6 - IEEE-488 interface)
	device->pia_1 = chip_6520_create(device->simulator, (Chip6520Signals) {
										[CHIP_6520_D0] = signal_split(SIGNAL(cpu_bus_data), 0, 1),
										[CHIP_6520_D1] = signal_split(SIGNAL(cpu_bus_data), 1, 1),
										[CHIP_6520_D2] = signal_split(SIGNAL(cpu_bus_data), 2, 1),
										[CHIP_6520_D3] = signal_split(SIGNAL(cpu_bus_data), 3, 1),
										[CHIP_6520_D4] = signal_split(SIGNAL(cpu_bus_data), 4, 1),
										[CHIP_6520_D5] = signal_split(SIGNAL(cpu_bus_data), 5, 1),
										[CHIP_6520_D6] = signal_split(SIGNAL(cpu_bus_data), 6, 1),
										[CHIP_6520_D7] = signal_split(SIGNAL(cpu_bus_data), 7, 1),
										[CHIP_6520_PHI2] = SIGNAL(clk1),
										[CHIP_6520_RESET_B] = SIGNAL(reset_b),
										[CHIP_6520_RW] = SIGNAL(cpu_rw),
										[CHIP_6520_CS0] = SIGNAL(x8xx),
										[CHIP_6520_CS1] = signal_split(SIGNAL(bus_ba), 5, 1),
										[CHIP_6520_CS2_B] = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										[CHIP_6520_RS0] = signal_split(SIGNAL(bus_ba), 0, 1),
										[CHIP_6520_RS1] = signal_split(SIGNAL(bus_ba), 1, 1),
										[CHIP_6520_CA1] = SIGNAL(atn_in_b),
										[CHIP_6520_CA2] = SIGNAL(ndac_out_b),
										[CHIP_6520_PA0] = signal_split(SIGNAL(bus_di), 0, 1),
										[CHIP_6520_PA1] = signal_split(SIGNAL(bus_di), 1, 1),
										[CHIP_6520_PA2] = signal_split(SIGNAL(bus_di), 2, 1),
										[CHIP_6520_PA3] = signal_split(SIGNAL(bus_di), 3, 1),
										[CHIP_6520_PA4] = signal_split(SIGNAL(bus_di), 4, 1),
										[CHIP_6520_PA5] = signal_split(SIGNAL(bus_di), 5, 1),
										[CHIP_6520_PA6] = signal_split(SIGNAL(bus_di), 6, 1),
										[CHIP_6520_PA7] = signal_split(SIGNAL(bus_di), 7, 1),
										[CHIP_6520_PB0] = signal_split(SIGNAL(bus_do), 0, 1),
										[CHIP_6520_PB1] = signal_split(SIGNAL(bus_do), 1, 1),
										[CHIP_6520_PB2] = signal_split(SIGNAL(bus_do), 2, 1),
										[CHIP_6520_PB3] = signal_split(SIGNAL(bus_do), 3, 1),
										[CHIP_6520_PB4] = signal_split(SIGNAL(bus_do), 4, 1),
										[CHIP_6520_PB5] = signal_split(SIGNAL(bus_do), 5, 1),
										[CHIP_6520_PB6] = signal_split(SIGNAL(bus_do), 6, 1),
										[CHIP_6520_PB7] = signal_split(SIGNAL(bus_do), 7, 1),
										[CHIP_6520_CB1] = SIGNAL(srq_in_b),
										[CHIP_6520_CB2] = SIGNAL(dav_out_b)
	});
	DEVICE_REGISTER_CHIP("C6", device->pia_1);
}

// sheet 03: cassette & keyboard
void circuit_create_03(DevCommodorePet *device) {

	// pia 2 (C7 - keyboard)
	device->pia_2 = chip_6520_create(device->simulator, (Chip6520Signals) {
										[CHIP_6520_D0] = signal_split(SIGNAL(cpu_bus_data), 0, 1),
										[CHIP_6520_D1] = signal_split(SIGNAL(cpu_bus_data), 1, 1),
										[CHIP_6520_D2] = signal_split(SIGNAL(cpu_bus_data), 2, 1),
										[CHIP_6520_D3] = signal_split(SIGNAL(cpu_bus_data), 3, 1),
										[CHIP_6520_D4] = signal_split(SIGNAL(cpu_bus_data), 4, 1),
										[CHIP_6520_D5] = signal_split(SIGNAL(cpu_bus_data), 5, 1),
										[CHIP_6520_D6] = signal_split(SIGNAL(cpu_bus_data), 6, 1),
										[CHIP_6520_D7] = signal_split(SIGNAL(cpu_bus_data), 7, 1),
										[CHIP_6520_PHI2] = SIGNAL(clk1),
										[CHIP_6520_RESET_B] = SIGNAL(reset_b),
										[CHIP_6520_RW] = SIGNAL(cpu_rw),
										[CHIP_6520_CS0] = SIGNAL(x8xx),
										[CHIP_6520_CS1] = signal_split(SIGNAL(bus_ba), 4, 1),
										[CHIP_6520_CS2_B] = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										[CHIP_6520_RS0] = signal_split(SIGNAL(bus_ba), 0, 1),
										[CHIP_6520_RS1] = signal_split(SIGNAL(bus_ba), 1, 1),
										[CHIP_6520_CA1] = SIGNAL(cass_read_1),
										[CHIP_6520_CA2] = SIGNAL(eoi_out_b),
										[CHIP_6520_PA0] = signal_split(SIGNAL(c7_porta), 0, 1),
										[CHIP_6520_PA1] = signal_split(SIGNAL(c7_porta), 1, 1),
										[CHIP_6520_PA2] = signal_split(SIGNAL(c7_porta), 2, 1),
										[CHIP_6520_PA3] = signal_split(SIGNAL(c7_porta), 3, 1),
										[CHIP_6520_PA4] = signal_split(SIGNAL(c7_porta), 4, 1),
										[CHIP_6520_PA5] = signal_split(SIGNAL(c7_porta), 5, 1),
										[CHIP_6520_PA6] = signal_split(SIGNAL(c7_porta), 6, 1),
										[CHIP_6520_PA7] = signal_split(SIGNAL(c7_porta), 7, 1),
										[CHIP_6520_PB0] = signal_split(SIGNAL(bus_kin), 0, 1),
										[CHIP_6520_PB1] = signal_split(SIGNAL(bus_kin), 1, 1),
										[CHIP_6520_PB2] = signal_split(SIGNAL(bus_kin), 2, 1),
										[CHIP_6520_PB3] = signal_split(SIGNAL(bus_kin), 3, 1),
										[CHIP_6520_PB4] = signal_split(SIGNAL(bus_kin), 4, 1),
										[CHIP_6520_PB5] = signal_split(SIGNAL(bus_kin), 5, 1),
										[CHIP_6520_PB6] = signal_split(SIGNAL(bus_kin), 6, 1),
										[CHIP_6520_PB7] = signal_split(SIGNAL(bus_kin), 7, 1),
										[CHIP_6520_CB1] = SIGNAL(video_on),
										[CHIP_6520_CB2] = SIGNAL(cass_motor_1_b)
	});
	DEVICE_REGISTER_CHIP("C7", device->pia_2);

	// via (C5)
	device->via = chip_6522_create(device->simulator, (Chip6522Signals) {
										[CHIP_6522_D0] = signal_split(SIGNAL(cpu_bus_data), 0, 1),
										[CHIP_6522_D1] = signal_split(SIGNAL(cpu_bus_data), 1, 1),
										[CHIP_6522_D2] = signal_split(SIGNAL(cpu_bus_data), 2, 1),
										[CHIP_6522_D3] = signal_split(SIGNAL(cpu_bus_data), 3, 1),
										[CHIP_6522_D4] = signal_split(SIGNAL(cpu_bus_data), 4, 1),
										[CHIP_6522_D5] = signal_split(SIGNAL(cpu_bus_data), 5, 1),
										[CHIP_6522_D6] = signal_split(SIGNAL(cpu_bus_data), 6, 1),
										[CHIP_6522_D7] = signal_split(SIGNAL(cpu_bus_data), 7, 1),
										[CHIP_6522_PHI2] = SIGNAL(clk1),
										[CHIP_6522_RESET_B] = SIGNAL(reset_b),
										[CHIP_6522_RW] = SIGNAL(cpu_rw),
										[CHIP_6522_CS1] = SIGNAL(cs1),
										[CHIP_6522_CS2_B] = SIGNAL(sele_b),							// io_b on schematic (jumpered to sele_b)
										[CHIP_6522_RS0] = signal_split(SIGNAL(bus_ba), 0, 1),
										[CHIP_6522_RS1] = signal_split(SIGNAL(bus_ba), 1, 1),
										[CHIP_6522_RS2] = signal_split(SIGNAL(bus_ba), 2, 1),
										[CHIP_6522_RS3] = signal_split(SIGNAL(bus_ba), 3, 1),
										[CHIP_6522_CA1] = SIGNAL(ca1),
										[CHIP_6522_CA2] = SIGNAL(graphic),
										[CHIP_6522_PA0] = signal_split(SIGNAL(bus_pa), 0, 1),
										[CHIP_6522_PA1] = signal_split(SIGNAL(bus_pa), 1, 1),
										[CHIP_6522_PA2] = signal_split(SIGNAL(bus_pa), 2, 1),
										[CHIP_6522_PA3] = signal_split(SIGNAL(bus_pa), 3, 1),
										[CHIP_6522_PA4] = signal_split(SIGNAL(bus_pa), 4, 1),
										[CHIP_6522_PA5] = signal_split(SIGNAL(bus_pa), 5, 1),
										[CHIP_6522_PA6] = signal_split(SIGNAL(bus_pa), 6, 1),
										[CHIP_6522_PA7] = signal_split(SIGNAL(bus_pa), 7, 1),
										[CHIP_6522_PB0] = signal_split(SIGNAL(c5_portb), 0, 1),
										[CHIP_6522_PB1] = signal_split(SIGNAL(c5_portb), 1, 1),
										[CHIP_6522_PB2] = signal_split(SIGNAL(c5_portb), 2, 1),
										[CHIP_6522_PB3] = signal_split(SIGNAL(c5_portb), 3, 1),
										[CHIP_6522_PB4] = signal_split(SIGNAL(c5_portb), 4, 1),
										[CHIP_6522_PB5] = signal_split(SIGNAL(c5_portb), 5, 1),
										[CHIP_6522_PB6] = signal_split(SIGNAL(c5_portb), 6, 1),
										[CHIP_6522_PB7] = signal_split(SIGNAL(c5_portb), 7, 1),
										[CHIP_6522_CB1] = SIGNAL(cass_read_2),
										[CHIP_6522_CB2] = SIGNAL(cb2)
	});
	DEVICE_REGISTER_CHIP("C5", device->via);

	// >> c9 - bcd decoder
	DEVICE_REGISTER_CHIP("C9", chip_74145_bcd_decoder_create(device->simulator, (Chip74145Signals) {
										[CHIP_74145_A] = SIGNAL(keya),
										[CHIP_74145_B] = SIGNAL(keyb),
										[CHIP_74145_C] = SIGNAL(keyc),
										[CHIP_74145_D] = SIGNAL(keyd),
										[CHIP_74145_Y0_B] = signal_split(SIGNAL(bus_kout), 0, 1),
										[CHIP_74145_Y1_B] = signal_split(SIGNAL(bus_kout), 1, 1),
										[CHIP_74145_Y2_B] = signal_split(SIGNAL(bus_kout), 2, 1),
										[CHIP_74145_Y3_B] = signal_split(SIGNAL(bus_kout), 3, 1),
										[CHIP_74145_Y4_B] = signal_split(SIGNAL(bus_kout), 4, 1),
										[CHIP_74145_Y5_B] = signal_split(SIGNAL(bus_kout), 5, 1),
										[CHIP_74145_Y6_B] = signal_split(SIGNAL(bus_kout), 6, 1),
										[CHIP_74145_Y7_B] = signal_split(SIGNAL(bus_kout), 7, 1),
										[CHIP_74145_Y8_B] = signal_split(SIGNAL(bus_kout), 8, 1),
										[CHIP_74145_Y9_B] = signal_split(SIGNAL(bus_kout), 9, 1)
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC3", glue_logic_create(device, 3));
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
										[CHIP_74244_G2_B] = SIGNAL(buf_rw),							// 19
										[CHIP_74244_G1_B] = SIGNAL(g78),							// 01

										[CHIP_74244_A11] = signal_split(SIGNAL(bus_rd), 7, 1),		// 02
										[CHIP_74244_Y24] = signal_split(SIGNAL(bus_rd), 7, 1),		// 03
										[CHIP_74244_A12] = signal_split(SIGNAL(bus_rd), 6, 1),		// 04
										[CHIP_74244_Y23] = signal_split(SIGNAL(bus_rd), 6, 1),		// 05
										[CHIP_74244_A13] = signal_split(SIGNAL(bus_rd), 5, 1),		// 06
										[CHIP_74244_Y22] = signal_split(SIGNAL(bus_rd), 5, 1),		// 07
										[CHIP_74244_A14] = signal_split(SIGNAL(bus_rd), 4, 1),		// 08
										[CHIP_74244_Y21] = signal_split(SIGNAL(bus_rd), 4, 1),		// 09

										[CHIP_74244_Y11] = signal_split(SIGNAL(bus_bd), 7, 1),		// 18
										[CHIP_74244_A24] = signal_split(SIGNAL(bus_bd), 7, 1),		// 17
										[CHIP_74244_Y12] = signal_split(SIGNAL(bus_bd), 6, 1),		// 16
										[CHIP_74244_A23] = signal_split(SIGNAL(bus_bd), 6, 1),		// 15
										[CHIP_74244_Y13] = signal_split(SIGNAL(bus_bd), 5, 1),		// 14
										[CHIP_74244_A22] = signal_split(SIGNAL(bus_bd), 5, 1),		// 13
										[CHIP_74244_Y14] = signal_split(SIGNAL(bus_bd), 4, 1),		// 12
										[CHIP_74244_A21] = signal_split(SIGNAL(bus_bd), 4, 1)		// 11
	}));

	DEVICE_REGISTER_CHIP("I10", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G2_B] = SIGNAL(buf_rw),							// 19
										[CHIP_74244_G1_B] = SIGNAL(g78),							// 01

										[CHIP_74244_A11]  = signal_split(SIGNAL(bus_rd), 3, 1),		// 02
										[CHIP_74244_Y24]  = signal_split(SIGNAL(bus_rd), 3, 1),		// 03
										[CHIP_74244_A12]  = signal_split(SIGNAL(bus_rd), 2, 1),		// 04
										[CHIP_74244_Y23]  = signal_split(SIGNAL(bus_rd), 2, 1),		// 05
										[CHIP_74244_A13]  = signal_split(SIGNAL(bus_rd), 1, 1),		// 06
										[CHIP_74244_Y22]  = signal_split(SIGNAL(bus_rd), 1, 1),		// 07
										[CHIP_74244_A14]  = signal_split(SIGNAL(bus_rd), 0, 1),		// 08
										[CHIP_74244_Y21]  = signal_split(SIGNAL(bus_rd), 0, 1),		// 09

										[CHIP_74244_Y11]  = signal_split(SIGNAL(bus_bd), 3, 1),		// 18
										[CHIP_74244_A24]  = signal_split(SIGNAL(bus_bd), 3, 1),		// 17
										[CHIP_74244_Y12]  = signal_split(SIGNAL(bus_bd), 2, 1),		// 16
										[CHIP_74244_A23]  = signal_split(SIGNAL(bus_bd), 2, 1),		// 15
										[CHIP_74244_Y13]  = signal_split(SIGNAL(bus_bd), 1, 1),		// 14
										[CHIP_74244_A22]  = signal_split(SIGNAL(bus_bd), 1, 1),		// 13
										[CHIP_74244_Y14]  = signal_split(SIGNAL(bus_bd), 0, 1),		// 12
										[CHIP_74244_A21]  = signal_split(SIGNAL(bus_bd), 0, 1)		// 11
	}));

	DEVICE_REGISTER_CHIP("E3", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(low),								// pin 1
										[CHIP_74153_C10] = SIGNAL(ra7),								// pin 6
										[CHIP_74153_C11] = SIGNAL(ra7),								// pin 5
										[CHIP_74153_C12] = signal_split(SIGNAL(bus_ba), 0, 1),		// pin 4
										[CHIP_74153_C13] = signal_split(SIGNAL(bus_ba), 13, 1),		// pin 3
										[CHIP_74153_Y1] = signal_split(SIGNAL(bus_fa), 0, 1),		// pin 7

										[CHIP_74153_A] = SIGNAL(muxa),								// pin 14
										[CHIP_74153_B] = SIGNAL(clk1),								// pin 2

										[CHIP_74153_G2] = SIGNAL(low),								// pin 15
										[CHIP_74153_C20] = SIGNAL(ra1),								// pin 10
										[CHIP_74153_C21] = SIGNAL(ra1),								// pin 11
										[CHIP_74153_C22] = signal_split(SIGNAL(bus_ba), 1, 1),		// pin 12
										[CHIP_74153_C23] = signal_split(SIGNAL(bus_ba), 7, 1),		// pin 13
										[CHIP_74153_Y2] = signal_split(SIGNAL(bus_fa), 1, 1),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E4", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(low),								// pin 1
										[CHIP_74153_C10] = SIGNAL(ra2),								// pin 6
										[CHIP_74153_C11] = SIGNAL(ra2),								// pin 5
										[CHIP_74153_C12] = signal_split(SIGNAL(bus_ba), 2, 1),		// pin 4
										[CHIP_74153_C13] = signal_split(SIGNAL(bus_ba), 8, 1),		// pin 3
										[CHIP_74153_Y1] = signal_split(SIGNAL(bus_fa), 2, 1),		// pin 7

										[CHIP_74153_A] = SIGNAL(muxa),								// pin 14
										[CHIP_74153_B] = SIGNAL(clk1),								// pin 2

										[CHIP_74153_G2] = SIGNAL(low),								// pin 15
										[CHIP_74153_C20] = SIGNAL(ra3),								// pin 10
										[CHIP_74153_C21] = SIGNAL(ra3),								// pin 11
										[CHIP_74153_C22] = signal_split(SIGNAL(bus_ba), 3, 1),		// pin 12
										[CHIP_74153_C23] = signal_split(SIGNAL(bus_ba), 9, 1),		// pin 13
										[CHIP_74153_Y2] = signal_split(SIGNAL(bus_fa), 3, 1),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E5", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(low),								// pin 1
										[CHIP_74153_C10] = SIGNAL(ra4),								// pin 6
										[CHIP_74153_C11] = SIGNAL(ra4),								// pin 5
										[CHIP_74153_C12] = signal_split(SIGNAL(bus_ba), 4, 1),		// pin 4
										[CHIP_74153_C13] = signal_split(SIGNAL(bus_ba), 10, 1),		// pin 3
										[CHIP_74153_Y1] = signal_split(SIGNAL(bus_fa), 4, 1),		// pin 7

										[CHIP_74153_A] = SIGNAL(muxa),								// pin 14
										[CHIP_74153_B] = SIGNAL(clk1),								// pin 2

										[CHIP_74153_G2] = SIGNAL(low),								// pin 15
										[CHIP_74153_C20] = SIGNAL(ra5),								// pin 10
										[CHIP_74153_C21] = SIGNAL(ra5),								// pin 11
										[CHIP_74153_C22] = signal_split(SIGNAL(bus_ba), 5, 1),		// pin 12
										[CHIP_74153_C23] = signal_split(SIGNAL(bus_ba), 11, 1),		// pin 13
										[CHIP_74153_Y2] = signal_split(SIGNAL(bus_fa), 5, 1),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E6", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(low),								// pin 1
										[CHIP_74153_C10] = SIGNAL(ra6),								// pin 6
										[CHIP_74153_C11] = SIGNAL(ra6),								// pin 5
										[CHIP_74153_C12] = signal_split(SIGNAL(bus_ba), 6, 1),		// pin 4
										[CHIP_74153_C13] = signal_split(SIGNAL(bus_ba), 12, 1),		// pin 3
										[CHIP_74153_Y1] = signal_split(SIGNAL(bus_fa), 6, 1),		// pin 7

										[CHIP_74153_A] = SIGNAL(muxa),								// pin 14
										[CHIP_74153_B] = SIGNAL(clk1),								// pin 2
	}));

	DEVICE_REGISTER_CHIP("I2-9", chip_8x4116_dram_create(device->simulator, (Chip8x4116DRamSignals) {
										[CHIP_4116_A0] = signal_split(SIGNAL(bus_fa), 0, 1),
										[CHIP_4116_A1] = signal_split(SIGNAL(bus_fa), 1, 1),
										[CHIP_4116_A2] = signal_split(SIGNAL(bus_fa), 2, 1),
										[CHIP_4116_A3] = signal_split(SIGNAL(bus_fa), 3, 1),
										[CHIP_4116_A4] = signal_split(SIGNAL(bus_fa), 4, 1),
										[CHIP_4116_A5] = signal_split(SIGNAL(bus_fa), 5, 1),
										[CHIP_4116_A6] = signal_split(SIGNAL(bus_fa), 6, 1),

										[CHIP_4116_DI0] = signal_split(SIGNAL(bus_rd), 0, 1),
										[CHIP_4116_DI1] = signal_split(SIGNAL(bus_rd), 1, 1),
										[CHIP_4116_DI2] = signal_split(SIGNAL(bus_rd), 2, 1),
										[CHIP_4116_DI3] = signal_split(SIGNAL(bus_rd), 3, 1),
										[CHIP_4116_DI4] = signal_split(SIGNAL(bus_rd), 4, 1),
										[CHIP_4116_DI5] = signal_split(SIGNAL(bus_rd), 5, 1),
										[CHIP_4116_DI6] = signal_split(SIGNAL(bus_rd), 6, 1),
										[CHIP_4116_DI7] = signal_split(SIGNAL(bus_rd), 7, 1),

										[CHIP_4116_DO0] = signal_split(SIGNAL(bus_rd), 0, 1),
										[CHIP_4116_DO1] = signal_split(SIGNAL(bus_rd), 1, 1),
										[CHIP_4116_DO2] = signal_split(SIGNAL(bus_rd), 2, 1),
										[CHIP_4116_DO3] = signal_split(SIGNAL(bus_rd), 3, 1),
										[CHIP_4116_DO4] = signal_split(SIGNAL(bus_rd), 4, 1),
										[CHIP_4116_DO5] = signal_split(SIGNAL(bus_rd), 5, 1),
										[CHIP_4116_DO6] = signal_split(SIGNAL(bus_rd), 6, 1),
										[CHIP_4116_DO7] = signal_split(SIGNAL(bus_rd), 7, 1),

										[CHIP_4116_WE_B] = SIGNAL(ram_rw),
										[CHIP_4116_RAS_B] = SIGNAL(ras0_b),
										[CHIP_4116_CAS_B] = SIGNAL(cas0_b)
	}));

	DEVICE_REGISTER_CHIP("J2-9", chip_8x4116_dram_create(device->simulator, (Chip8x4116DRamSignals) {
										[CHIP_4116_A0] = signal_split(SIGNAL(bus_fa), 0, 1),
										[CHIP_4116_A1] = signal_split(SIGNAL(bus_fa), 1, 1),
										[CHIP_4116_A2] = signal_split(SIGNAL(bus_fa), 2, 1),
										[CHIP_4116_A3] = signal_split(SIGNAL(bus_fa), 3, 1),
										[CHIP_4116_A4] = signal_split(SIGNAL(bus_fa), 4, 1),
										[CHIP_4116_A5] = signal_split(SIGNAL(bus_fa), 5, 1),
										[CHIP_4116_A6] = signal_split(SIGNAL(bus_fa), 6, 1),

										[CHIP_4116_DI0] = signal_split(SIGNAL(bus_rd), 0, 1),
										[CHIP_4116_DI1] = signal_split(SIGNAL(bus_rd), 1, 1),
										[CHIP_4116_DI2] = signal_split(SIGNAL(bus_rd), 2, 1),
										[CHIP_4116_DI3] = signal_split(SIGNAL(bus_rd), 3, 1),
										[CHIP_4116_DI4] = signal_split(SIGNAL(bus_rd), 4, 1),
										[CHIP_4116_DI5] = signal_split(SIGNAL(bus_rd), 5, 1),
										[CHIP_4116_DI6] = signal_split(SIGNAL(bus_rd), 6, 1),
										[CHIP_4116_DI7] = signal_split(SIGNAL(bus_rd), 7, 1),

										[CHIP_4116_DO0] = signal_split(SIGNAL(bus_rd), 0, 1),
										[CHIP_4116_DO1] = signal_split(SIGNAL(bus_rd), 1, 1),
										[CHIP_4116_DO2] = signal_split(SIGNAL(bus_rd), 2, 1),
										[CHIP_4116_DO3] = signal_split(SIGNAL(bus_rd), 3, 1),
										[CHIP_4116_DO4] = signal_split(SIGNAL(bus_rd), 4, 1),
										[CHIP_4116_DO5] = signal_split(SIGNAL(bus_rd), 5, 1),
										[CHIP_4116_DO6] = signal_split(SIGNAL(bus_rd), 6, 1),
										[CHIP_4116_DO7] = signal_split(SIGNAL(bus_rd), 7, 1),

										[CHIP_4116_WE_B] = SIGNAL(ram_rw),
										[CHIP_4116_RAS_B] = SIGNAL(ras0_b),
										[CHIP_4116_CAS_B] = SIGNAL(cas1_b)
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC5", glue_logic_create(device, 5));
}

// sheet 06 - master timing
void circuit_create_06(DevCommodorePet *device) {

	// >> y1 - oscillator
	device->oscillator_y1 = oscillator_create(16000000, device->simulator, (OscillatorSignals) {
										[CHIP_OSCILLATOR_CLK_OUT] = SIGNAL(clk16)
	});
	DEVICE_REGISTER_CHIP("Y1", device->oscillator_y1);

	// >> g5 - binary counter
	DEVICE_REGISTER_CHIP("G5", chip_74191_binary_counter_create(device->simulator, (Chip74191Signals) {
										[CHIP_74191_ENABLE_B] = SIGNAL(low),		// pin 04
										[CHIP_74191_D_U] = SIGNAL(low),				// pin 05
										[CHIP_74191_A] = SIGNAL(low),				// pin 15
										[CHIP_74191_B] = SIGNAL(low),				// pin 01
										[CHIP_74191_C] = SIGNAL(low),				// pin 10
										[CHIP_74191_D] = SIGNAL(low),				// pin 09
										[CHIP_74191_LOAD_B] = SIGNAL(init_b),		// pin 11
										[CHIP_74191_CLK] = SIGNAL(clk16),			// pin 14
										[CHIP_74191_QA] = SIGNAL(clk8),				// pin 03
										[CHIP_74191_QB] = SIGNAL(clk4),				// pin 02
										[CHIP_74191_QC] = SIGNAL(clk2),				// pin 06
										[CHIP_74191_QD] = SIGNAL(clk1),				// pin 07
								//		[CHIP_74191_MAX_MIN] = not connected        // pin 12
								//		[CHIP_74191_RCO_B] = not connected          // pin 13
	}));

	// >> h3 - 8-bit shift register
	DEVICE_REGISTER_CHIP("H3", chip_74164_shift_register_create(device->simulator, (Chip74164Signals) {
										[CHIP_74164_A] = SIGNAL(clk1),				// pin 01
										[CHIP_74164_B] = SIGNAL(high),				// pin 02
										[CHIP_74164_CLK] = SIGNAL(clk16),			// pin 08
										[CHIP_74164_CLEAR_B] = SIGNAL(init_b),		// pin 09
										[CHIP_74164_QA] = SIGNAL(bphi2a),			// pin 03
										[CHIP_74164_QB] = SIGNAL(bphi2b),			// pin 04
										[CHIP_74164_QC] = SIGNAL(bphi2c),			// pin 05
										[CHIP_74164_QD] = SIGNAL(bphi2d),			// pin 06
										[CHIP_74164_QE] = SIGNAL(bphi2e),			// pin 10
										[CHIP_74164_QF] = SIGNAL(bphi2f),			// pin 11
										[CHIP_74164_QG] = SIGNAL(bphi2g),			// pin 12
										[CHIP_74164_QH] = SIGNAL(bphi2h),			// pin 13
	}));

	// >> h6 - JK flip-flop
	DEVICE_REGISTER_CHIP("H6", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(init_b),		// pin 13
										[CHIP_74107_CLK1] = SIGNAL(bphi2a_b),		// pin 12
										[CHIP_74107_J1] = SIGNAL(init_b),			// pin 1
										[CHIP_74107_K1] = SIGNAL(init_b),			// pin 4
										[CHIP_74107_Q1] = SIGNAL(ra1),				// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(ra1_b),			// pin 2

										[CHIP_74107_CLR2_B] = SIGNAL(init_b),		// pin 10
										[CHIP_74107_CLK2] = SIGNAL(ra5),			// pin 9
										[CHIP_74107_J2] = SIGNAL(init_b),			// pin 8
										[CHIP_74107_K2] = SIGNAL(init_b),			// pin 11
										[CHIP_74107_Q2] = SIGNAL(ra6),				// pin 5
										[CHIP_74107_Q2_B] = SIGNAL(ra6_b)			// pin 6
	}));

	// >> h9 - binary counter
	DEVICE_REGISTER_CHIP("H9", chip_7493_binary_counter_create(device->simulator, (Chip7493Signals) {
										[CHIP_7493_A_B] = SIGNAL(ra1),				// pin 14
										[CHIP_7493_B_B] = SIGNAL(ra2),				// pin 1
										[CHIP_7493_R01] = SIGNAL(init),			// pin 2
										[CHIP_7493_R02] = SIGNAL(init),			// pin 3
										[CHIP_7493_QA] = SIGNAL(ra2),				// pin 12
										[CHIP_7493_QB] = SIGNAL(ra3),				// pin 9
										[CHIP_7493_QC] = SIGNAL(ra4),				// pin 8
										[CHIP_7493_QD] = SIGNAL(ra5),				// pin 11
	}));

	// >> g9 - d flip-flop (2 flipflop is used on sheet 8)
	DEVICE_REGISTER_CHIP("G9", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										[CHIP_7474_D1] = SIGNAL(init_b),			// pin 2
										[CHIP_7474_CLR1_B] = SIGNAL(bphi2b),		// pin 1
										[CHIP_7474_CLK1] = SIGNAL(bphi2a_b),		// pin 3
										[CHIP_7474_PR1_B] = SIGNAL(high),			// pin 4
										[CHIP_7474_Q1] = SIGNAL(load_sr),			// pin 5
										[CHIP_7474_Q1_B] = SIGNAL(load_sr_b),		// pin 6

										[CHIP_7474_D2] = signal_split(SIGNAL(bus_lsd), 7, 1),	// pin 12
										[CHIP_7474_CLR2_B] = SIGNAL(init_b),					// pin 13
										[CHIP_7474_CLK2] = SIGNAL(load_sr),					// pin 11
										[CHIP_7474_PR2_B] = SIGNAL(high),						// pin 10
										[CHIP_7474_Q2] = SIGNAL(g9q),							// pin 9
										[CHIP_7474_Q2_B] = SIGNAL(g9q_b),						// pin 8
	}));

	// >> h7 - JK flip-flop
	DEVICE_REGISTER_CHIP("H7", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(init_b),		// pin 13
										[CHIP_74107_CLK1] = SIGNAL(ra1and3),		// pin 12
										[CHIP_74107_J1] = SIGNAL(horz_disp_on),		// pin 1
										[CHIP_74107_K1] = SIGNAL(horz_disp_off),	// pin 4
										[CHIP_74107_Q1] = SIGNAL(horz_drive_b),		// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(horz_drive),		// pin 2

										[CHIP_74107_CLR2_B] = SIGNAL(init_b),		// pin 10
										[CHIP_74107_CLK2] = SIGNAL(load_sr_b),		// pin 9
										[CHIP_74107_J2] = SIGNAL(ra4and6),			// pin 8
										[CHIP_74107_K2] = SIGNAL(ra5and6_b),		// pin 11
										[CHIP_74107_Q2] = SIGNAL(horz_disp_on),		// pin 5
										[CHIP_74107_Q2_B] = SIGNAL(horz_disp_off)	// pin 6
	}));

	// >> h8 - JK flip-flop
	DEVICE_REGISTER_CHIP("H8", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(init_b),		// pin 13
										[CHIP_74107_CLK1] = SIGNAL(next),			// pin 12
										[CHIP_74107_J1] = SIGNAL(h8q2_b),			// pin 1
										[CHIP_74107_K1] = SIGNAL(h8q2),				// pin 4
										[CHIP_74107_Q1] = SIGNAL(h8q),				// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(h8q_b),			// pin 2

										[CHIP_74107_CLR2_B] = SIGNAL(init_b),		// pin 10
										[CHIP_74107_CLK2] = SIGNAL(next),			// pin 9
										[CHIP_74107_J2] = SIGNAL(h8q),				// pin 8
										[CHIP_74107_K2] = SIGNAL(h8q_b),			// pin 11
										[CHIP_74107_Q2] = SIGNAL(h8q2),				// pin 5
										[CHIP_74107_Q2_B] = SIGNAL(h8q2_b)			// pin 6
	}));

	// >> h4 - Quad 2to1 multiplexer
	DEVICE_REGISTER_CHIP("H4", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0A] = SIGNAL(bphi2b),			// pin 2
										[CHIP_74157_I1A] = SIGNAL(bphi2a),			// pin 3
										[CHIP_74157_ZA] = SIGNAL(h4y1),				// pin 4

										[CHIP_74157_I0D] = SIGNAL(bphi2c),			// pin 11
										[CHIP_74157_I1D] = SIGNAL(bphi2b),			// pin 10
										[CHIP_74157_ZD] = SIGNAL(muxa),				// pin 9

										[CHIP_74157_I0C] = SIGNAL(bphi2d),			// pin 14
										[CHIP_74157_I1C] = SIGNAL(bphi2c),			// pin 13
										[CHIP_74157_ZC] = SIGNAL(h4y4),				// pin 12

										[CHIP_74157_SEL] = SIGNAL(buf_rw),			// pin 1
										[CHIP_74157_ENABLE_B] = SIGNAL(h53)			// pin 15
	}));

	// >> h1 - d flipflop
	DEVICE_REGISTER_CHIP("H1", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										[CHIP_7474_D1] = SIGNAL(init_b),			// pin 2
										[CHIP_7474_CLR1_B] = SIGNAL(bphi2g_b),		// pin 1
										[CHIP_7474_CLK1] = SIGNAL(h4y1),			// pin 3
										[CHIP_7474_PR1_B] = SIGNAL(init_b),		// pin 4
										[CHIP_7474_Q1] = SIGNAL(h1q1),				// pin 5
										[CHIP_7474_Q1_B] = SIGNAL(h1q1_b),			// pin 6

										[CHIP_7474_D2] = SIGNAL(init_b),			// pin 12
										[CHIP_7474_CLR2_B] = SIGNAL(bphi2f),		// pin 13
										[CHIP_7474_CLK2] = SIGNAL(bphi2b_b),		// pin 11
										[CHIP_7474_PR2_B] = SIGNAL(pullup_2),		// pin 10
										[CHIP_7474_Q2] = SIGNAL(h1q2),				// pin 9
										[CHIP_7474_Q2_B] = SIGNAL(h1q2_b)			// pin 8
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC6", glue_logic_create(device, 6));
}

// sheet 07 - display logic components
void circuit_create_07(DevCommodorePet *device) {

	// >> g6 - JK flip-flop
	DEVICE_REGISTER_CHIP("G6", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(horz_disp_on),	// pin 13
										[CHIP_74107_CLK1] = SIGNAL(ra1_b),			// pin 12
										[CHIP_74107_J1] = SIGNAL(init_b),			// pin 1
										[CHIP_74107_K1] = SIGNAL(init_b),			// pin 4
										[CHIP_74107_Q1] = SIGNAL(g6_q),				// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(g6_q_b),			// pin 2
	}));

	// >> f6 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F6", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0D] = SIGNAL(high),						// pin 11
										[CHIP_74157_I1D] = SIGNAL(high),						// pin 10
										[CHIP_74157_I0B] = SIGNAL(high),						// pin 05
										[CHIP_74157_I1B] = SIGNAL(a5_12),						// pin 06
										[CHIP_74157_I0C] = SIGNAL(ra1_b),						// pin 14
										[CHIP_74157_I1C] = signal_split(SIGNAL(bus_ba), 0, 1),	// pin 13
										[CHIP_74157_I0A] = SIGNAL(g6_q),						// pin 02
										[CHIP_74157_I1A] = signal_split(SIGNAL(bus_ba), 1, 1),	// pin 03

										[CHIP_74157_SEL] = SIGNAL(clk1),						// pin 01
										[CHIP_74157_ENABLE_B] = SIGNAL(low),					// pin 15

										[CHIP_74157_ZB] = SIGNAL(tv_ram_rw),					// pin 07
										[CHIP_74157_ZD] = SIGNAL(f6_y3),						// pin 09
										[CHIP_74157_ZC] = signal_split(SIGNAL(bus_sa), 0, 1),	// pin 12
										[CHIP_74157_ZA] = signal_split(SIGNAL(bus_sa), 1, 1)	// pin 04
	}));

	// >> f5 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F5", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0D] = SIGNAL(ga2),							// pin 11
										[CHIP_74157_I1D] = signal_split(SIGNAL(bus_ba), 2, 1),	// pin 10
										[CHIP_74157_I0B] = SIGNAL(ga3),							// pin 05
										[CHIP_74157_I1B] = signal_split(SIGNAL(bus_ba), 3, 1),	// pin 06
										[CHIP_74157_I0C] = SIGNAL(ga4),							// pin 14
										[CHIP_74157_I1C] = signal_split(SIGNAL(bus_ba), 4, 1),	// pin 13
										[CHIP_74157_I0A] = SIGNAL(ga5),							// pin 02
										[CHIP_74157_I1A] = signal_split(SIGNAL(bus_ba), 5, 1),	// pin 03

										[CHIP_74157_SEL] = SIGNAL(clk1),						// pin 01
										[CHIP_74157_ENABLE_B] = SIGNAL(low),					// pin 15

										[CHIP_74157_ZD] = signal_split(SIGNAL(bus_sa), 2, 1),	// pin 09
										[CHIP_74157_ZB] = signal_split(SIGNAL(bus_sa), 3, 1),	// pin 07
										[CHIP_74157_ZC] = signal_split(SIGNAL(bus_sa), 4, 1),	// pin 12
										[CHIP_74157_ZA] = signal_split(SIGNAL(bus_sa), 5, 1)	// pin 04
	}));

	// >> f3 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F3", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0A] = SIGNAL(ga6),							// pin 02
										[CHIP_74157_I1A] = signal_split(SIGNAL(bus_ba), 6, 1),	// pin 03
										[CHIP_74157_I0C] = SIGNAL(ga7),							// pin 14
										[CHIP_74157_I1C] = signal_split(SIGNAL(bus_ba), 7, 1),	// pin 13
										[CHIP_74157_I0B] = SIGNAL(ga8),							// pin 05
										[CHIP_74157_I1B] = signal_split(SIGNAL(bus_ba), 8, 1),	// pin 06
										[CHIP_74157_I0D] = SIGNAL(ga9),							// pin 11
										[CHIP_74157_I1D] = signal_split(SIGNAL(bus_ba), 9, 1),	// pin 10

										[CHIP_74157_SEL] = SIGNAL(clk1),						// pin 01
										[CHIP_74157_ENABLE_B] = SIGNAL(low),					// pin 15

										[CHIP_74157_ZA] = signal_split(SIGNAL(bus_sa), 6, 1),	// pin 04
										[CHIP_74157_ZC] = signal_split(SIGNAL(bus_sa), 7, 1),	// pin 12
										[CHIP_74157_ZB] = signal_split(SIGNAL(bus_sa), 8, 1),	// pin 07
										[CHIP_74157_ZD] = signal_split(SIGNAL(bus_sa), 9, 1)	// pin 09
	}));

	// >> f4 - binary counter
	DEVICE_REGISTER_CHIP("F4", chip_74177_binary_counter_create(device->simulator, (Chip74177Signals) {
										[CHIP_74177_CLK2] = SIGNAL(ga2),				// pin 06
										[CHIP_74177_CLK1] = SIGNAL(g6_q),				// pin 08
										[CHIP_74177_LOAD_B] = SIGNAL(horz_disp_on),		// pin 01
										[CHIP_74177_CLEAR_B] = SIGNAL(next_b),			// pin 13
										[CHIP_74177_A] = SIGNAL(lga2),					// pin 04
										[CHIP_74177_B] = SIGNAL(lga3),					// pin 10
										[CHIP_74177_C] = SIGNAL(lga4),					// pin 03
										[CHIP_74177_D] = SIGNAL(lga5),					// pin 11

										[CHIP_74177_QA] = SIGNAL(ga2),					// pin 05
										[CHIP_74177_QB] = SIGNAL(ga3),					// pin 09
										[CHIP_74177_QC] = SIGNAL(ga4),					// pin 02
										[CHIP_74177_QD] = SIGNAL(ga5)					// pin 12
	}));

	// >> f2 - binary counter
	DEVICE_REGISTER_CHIP("F2", chip_74177_binary_counter_create(device->simulator, (Chip74177Signals) {
										[CHIP_74177_CLK2] = SIGNAL(ga6),				// pin 06
										[CHIP_74177_CLK1] = SIGNAL(ga5),				// pin 08
										[CHIP_74177_LOAD_B] = SIGNAL(horz_disp_on),		// pin 01
										[CHIP_74177_CLEAR_B] = SIGNAL(next_b),			// pin 13
										[CHIP_74177_A] = SIGNAL(lga6),					// pin 04
										[CHIP_74177_B] = SIGNAL(lga7),					// pin 10
										[CHIP_74177_C] = SIGNAL(lga8),					// pin 03
										[CHIP_74177_D] = SIGNAL(lga9),					// pin 11

										[CHIP_74177_QA] = SIGNAL(ga6),					// pin 05
										[CHIP_74177_QB] = SIGNAL(ga7),					// pin 09
										[CHIP_74177_QC] = SIGNAL(ga8),					// pin 02
										[CHIP_74177_QD] = SIGNAL(ga9)					// pin 12
	}));

	// >> g3 - 8-bit latch
	DEVICE_REGISTER_CHIP("G3", chip_74373_latch_create(device->simulator, (Chip74373Signals) {
										[CHIP_74373_D5] = SIGNAL(ga2),			// pin 13
										[CHIP_74373_D8] = SIGNAL(ga3),			// pin 18
										[CHIP_74373_D6] = SIGNAL(ga4),			// pin 14
										[CHIP_74373_D7] = SIGNAL(ga5),			// pin 17
										[CHIP_74373_D2] = SIGNAL(ga6),			// pin 04
										[CHIP_74373_D3] = SIGNAL(ga7),			// pin 07
										[CHIP_74373_D1] = SIGNAL(ga8),			// pin 03
										[CHIP_74373_D4] = SIGNAL(ga9),			// pin 08

										[CHIP_74373_C] = SIGNAL(reload_next),	// pin 11 - enable input
										[CHIP_74373_OC_B] = SIGNAL(low),		// pin 01 - output control

										[CHIP_74373_Q5] = SIGNAL(lga2),			// pin 12
										[CHIP_74373_Q8] = SIGNAL(lga3),			// pin 19
										[CHIP_74373_Q6] = SIGNAL(lga4),			// pin 15
										[CHIP_74373_Q7] = SIGNAL(lga5),			// pin 16
										[CHIP_74373_Q2] = SIGNAL(lga6),			// pin 05
										[CHIP_74373_Q3] = SIGNAL(lga7),			// pin 06
										[CHIP_74373_Q1] = SIGNAL(lga8),			// pin 02
										[CHIP_74373_Q4] = SIGNAL(lga9)			// pin 09
	}));

	// >> g8 - d flip-flop
	DEVICE_REGISTER_CHIP("G8", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										// [CHIP_7474_D1] = not used,				// pin 2
										// [CHIP_7474_CLR1_B] = not used,			// pin 1
										// [CHIP_7474_CLK1] = not used,			// pin 3
										// [CHIP_7474_PR1_B] = not used,			// pin 4
										// [CHIP_7474_Q1] = not used,				// pin 5
										// [CHIP_7474_Q1_B] = not used,			// pin 6

										[CHIP_7474_PR2_B] = SIGNAL(pullup_1),		// pin 10
										[CHIP_7474_D2] = SIGNAL(w220_off),			// pin 12
										[CHIP_7474_CLK2] = SIGNAL(video_latch),	// pin 11
										[CHIP_7474_CLR2_B] = SIGNAL(bphi2h),		// pin 13
										[CHIP_7474_Q2] = SIGNAL(next),				// pin 9
										[CHIP_7474_Q2_B] = SIGNAL(next_b)			// pin 8
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC7", glue_logic_create(device, 7));
}

// sheet 08: display rams components
void circuit_create_08(DevCommodorePet *device) {

	// >> h11 - binary counter
	DEVICE_REGISTER_CHIP("H11", chip_7493_binary_counter_create(device->simulator, (Chip7493Signals) {
										[CHIP_7493_A_B] = SIGNAL(low),				// pin 14
										[CHIP_7493_B_B] = SIGNAL(horz_disp_on),	// pin 1
										[CHIP_7493_R01] = SIGNAL(next),			// pin 2
										[CHIP_7493_R02] = SIGNAL(next),			// pin 3
										// [CHIP_7493_QA] = not used,				// pin 12
										[CHIP_7493_QB] = SIGNAL(ra7),				// pin 9
										[CHIP_7493_QC] = SIGNAL(ra8),				// pin 8
										[CHIP_7493_QD] = SIGNAL(ra9),				// pin 11
	}));

	// >> f7 - 1k x 4bit SRAM
	DEVICE_REGISTER_CHIP("F7", chip_6114_sram_create(device->simulator, (Chip6114SRamSignals) {
										[CHIP_6114_A0] = signal_split(SIGNAL(bus_sa), 0, 1),
										[CHIP_6114_A1] = signal_split(SIGNAL(bus_sa), 1, 1),
										[CHIP_6114_A2] = signal_split(SIGNAL(bus_sa), 2, 1),
										[CHIP_6114_A3] = signal_split(SIGNAL(bus_sa), 3, 1),
										[CHIP_6114_A4] = signal_split(SIGNAL(bus_sa), 4, 1),
										[CHIP_6114_A5] = signal_split(SIGNAL(bus_sa), 5, 1),
										[CHIP_6114_A6] = signal_split(SIGNAL(bus_sa), 6, 1),
										[CHIP_6114_A7] = signal_split(SIGNAL(bus_sa), 7, 1),
										[CHIP_6114_A8] = signal_split(SIGNAL(bus_sa), 8, 1),
										[CHIP_6114_A9] = signal_split(SIGNAL(bus_sa), 9, 1),

										[CHIP_6114_IO0] = signal_split(SIGNAL(bus_sd), 4, 1),
										[CHIP_6114_IO1] = signal_split(SIGNAL(bus_sd), 5, 1),
										[CHIP_6114_IO2] = signal_split(SIGNAL(bus_sd), 6, 1),
										[CHIP_6114_IO3] = signal_split(SIGNAL(bus_sd), 7, 1),

										[CHIP_6114_CE_B] = SIGNAL(low),
										[CHIP_6114_RW] = SIGNAL(tv_ram_rw)
	}));

	// >> f8 - 1k x 4bit SRAM
	DEVICE_REGISTER_CHIP("F8", chip_6114_sram_create(device->simulator, (Chip6114SRamSignals) {
										[CHIP_6114_A0] = signal_split(SIGNAL(bus_sa), 0, 1),
										[CHIP_6114_A1] = signal_split(SIGNAL(bus_sa), 1, 1),
										[CHIP_6114_A2] = signal_split(SIGNAL(bus_sa), 2, 1),
										[CHIP_6114_A3] = signal_split(SIGNAL(bus_sa), 3, 1),
										[CHIP_6114_A4] = signal_split(SIGNAL(bus_sa), 4, 1),
										[CHIP_6114_A5] = signal_split(SIGNAL(bus_sa), 5, 1),
										[CHIP_6114_A6] = signal_split(SIGNAL(bus_sa), 6, 1),
										[CHIP_6114_A7] = signal_split(SIGNAL(bus_sa), 7, 1),
										[CHIP_6114_A8] = signal_split(SIGNAL(bus_sa), 8, 1),
										[CHIP_6114_A9] = signal_split(SIGNAL(bus_sa), 9, 1),

										[CHIP_6114_IO0] = signal_split(SIGNAL(bus_sd), 0, 1),
										[CHIP_6114_IO1] = signal_split(SIGNAL(bus_sd), 1, 1),
										[CHIP_6114_IO2] = signal_split(SIGNAL(bus_sd), 2, 1),
										[CHIP_6114_IO3] = signal_split(SIGNAL(bus_sd), 3, 1),

										[CHIP_6114_CE_B] = SIGNAL(low),
										[CHIP_6114_RW] = SIGNAL(tv_ram_rw)
	}));

	// >> e8 - octal buffer
	DEVICE_REGISTER_CHIP("E8", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G2_B] = SIGNAL(tv_ram_rw),						// 19
										[CHIP_74244_G1_B] = SIGNAL(tv_read_b),						// 01

										[CHIP_74244_A11]  = signal_split(SIGNAL(bus_sd), 0, 1),		// 02
										[CHIP_74244_Y24]  = signal_split(SIGNAL(bus_sd), 0, 1),		// 03
										[CHIP_74244_A12]  = signal_split(SIGNAL(bus_sd), 1, 1),		// 04
										[CHIP_74244_Y23]  = signal_split(SIGNAL(bus_sd), 1, 1),		// 05
										[CHIP_74244_A13]  = signal_split(SIGNAL(bus_sd), 2, 1),		// 06
										[CHIP_74244_Y22]  = signal_split(SIGNAL(bus_sd), 2, 1),		// 07
										[CHIP_74244_A14]  = signal_split(SIGNAL(bus_sd), 3, 1),		// 08
										[CHIP_74244_Y21]  = signal_split(SIGNAL(bus_sd), 3, 1),		// 09

										[CHIP_74244_Y11]  = signal_split(SIGNAL(bus_bd), 0, 1),		// 18
										[CHIP_74244_A24]  = signal_split(SIGNAL(bus_bd), 0, 1),		// 17
										[CHIP_74244_Y12]  = signal_split(SIGNAL(bus_bd), 1, 1),		// 16
										[CHIP_74244_A23]  = signal_split(SIGNAL(bus_bd), 1, 1),		// 15
										[CHIP_74244_Y13]  = signal_split(SIGNAL(bus_bd), 2, 1),		// 14
										[CHIP_74244_A22]  = signal_split(SIGNAL(bus_bd), 2, 1),		// 13
										[CHIP_74244_Y14]  = signal_split(SIGNAL(bus_bd), 3, 1),		// 12
										[CHIP_74244_A21]  = signal_split(SIGNAL(bus_bd), 3, 1)		// 11
	}));

	// >> e7 - octal buffer
	DEVICE_REGISTER_CHIP("E7", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G2_B] = SIGNAL(tv_ram_rw),						// 19
										[CHIP_74244_G1_B] = SIGNAL(tv_read_b),						// 01

										[CHIP_74244_A11]  = signal_split(SIGNAL(bus_sd), 4, 1),		// 02
										[CHIP_74244_Y24]  = signal_split(SIGNAL(bus_sd), 4, 1),		// 03
										[CHIP_74244_A12]  = signal_split(SIGNAL(bus_sd), 5, 1),		// 04
										[CHIP_74244_Y23]  = signal_split(SIGNAL(bus_sd), 5, 1),		// 05
										[CHIP_74244_A13]  = signal_split(SIGNAL(bus_sd), 6, 1),		// 06
										[CHIP_74244_Y22]  = signal_split(SIGNAL(bus_sd), 6, 1),		// 07
										[CHIP_74244_A14]  = signal_split(SIGNAL(bus_sd), 7, 1),		// 08
										[CHIP_74244_Y21]  = signal_split(SIGNAL(bus_sd), 7, 1),		// 09

										[CHIP_74244_Y11]  = signal_split(SIGNAL(bus_bd), 4, 1),		// 18
										[CHIP_74244_A24]  = signal_split(SIGNAL(bus_bd), 4, 1),		// 17
										[CHIP_74244_Y12]  = signal_split(SIGNAL(bus_bd), 5, 1),		// 16
										[CHIP_74244_A23]  = signal_split(SIGNAL(bus_bd), 5, 1),		// 15
										[CHIP_74244_Y13]  = signal_split(SIGNAL(bus_bd), 6, 1),		// 14
										[CHIP_74244_A22]  = signal_split(SIGNAL(bus_bd), 6, 1),		// 13
										[CHIP_74244_Y14]  = signal_split(SIGNAL(bus_bd), 7, 1),		// 12
										[CHIP_74244_A21]  = signal_split(SIGNAL(bus_bd), 7, 1)		// 11
	}));

	// >> f9 - 8-bit latch
	DEVICE_REGISTER_CHIP("F9", chip_74373_latch_create(device->simulator, (Chip74373Signals) {
										[CHIP_74373_C] = SIGNAL(video_latch),					// 11
										[CHIP_74373_OC_B] = SIGNAL(low),							// 1
										[CHIP_74373_D1] = signal_split(SIGNAL(bus_sd), 0, 1),		// 3
										[CHIP_74373_D8] = signal_split(SIGNAL(bus_sd), 1, 1),		// 18
										[CHIP_74373_D2] = signal_split(SIGNAL(bus_sd), 2, 1),		// 4
										[CHIP_74373_D7] = signal_split(SIGNAL(bus_sd), 3, 1),		// 17
										[CHIP_74373_D3] = signal_split(SIGNAL(bus_sd), 4, 1),		// 7
										[CHIP_74373_D6] = signal_split(SIGNAL(bus_sd), 5, 1),		// 14
										[CHIP_74373_D4] = signal_split(SIGNAL(bus_sd), 6, 1),		// 8
										[CHIP_74373_D5] = signal_split(SIGNAL(bus_sd), 7, 1),		// 13

										[CHIP_74373_Q1] = signal_split(SIGNAL(bus_lsd), 0, 1),	// 2
										[CHIP_74373_Q8] = signal_split(SIGNAL(bus_lsd), 1, 1),	// 19
										[CHIP_74373_Q2] = signal_split(SIGNAL(bus_lsd), 2, 1),	// 5
										[CHIP_74373_Q7] = signal_split(SIGNAL(bus_lsd), 3, 1),	// 16
										[CHIP_74373_Q3] = signal_split(SIGNAL(bus_lsd), 4, 1),	// 6
										[CHIP_74373_Q6] = signal_split(SIGNAL(bus_lsd), 5, 1),	// 15
										[CHIP_74373_Q4] = signal_split(SIGNAL(bus_lsd), 6, 1),	// 9
										[CHIP_74373_Q5] = signal_split(SIGNAL(bus_lsd), 7, 1),	// 12
	}));

	// >> f10 - character rom
	Chip63xxRom *char_rom = load_character_rom(device, "runtime/commodore_pet/characters-2.901447-10.bin");
	assert(char_rom);
	DEVICE_REGISTER_CHIP("F10", char_rom);

	// >> e11 - shift register
	DEVICE_REGISTER_CHIP("E11", chip_74165_shift_register_create(device->simulator, (Chip74165Signals) {
										[CHIP_74165_SL		] = SIGNAL(load_sr_b),					// 1
										[CHIP_74165_CLK	] = SIGNAL(clk8),						// 2
										[CHIP_74165_CLK_INH] = SIGNAL(low),							// 15
										[CHIP_74165_SI		] = signal_split(SIGNAL(bus_cd), 7, 1),	// 10
										[CHIP_74165_A		] = signal_split(SIGNAL(bus_cd), 0, 1),	// 11
										[CHIP_74165_B		] = signal_split(SIGNAL(bus_cd), 1, 1),	// 12
										[CHIP_74165_C		] = signal_split(SIGNAL(bus_cd), 2, 1),	// 13
										[CHIP_74165_D		] = signal_split(SIGNAL(bus_cd), 3, 1),	// 14
										[CHIP_74165_E		] = signal_split(SIGNAL(bus_cd), 4, 1),	// 3
										[CHIP_74165_F		] = signal_split(SIGNAL(bus_cd), 5, 1),	// 4
										[CHIP_74165_G		] = signal_split(SIGNAL(bus_cd), 6, 1),	// 5
										[CHIP_74165_H		] = signal_split(SIGNAL(bus_cd), 7, 1),	// 6
										[CHIP_74165_QH		] = SIGNAL(e11qh),						// 9
										[CHIP_74165_QH_B	] = SIGNAL(e11qh_b),						// 7
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC8", glue_logic_create(device, 8));
}

// peripherals
void circuit_create_peripherals(DevCommodorePet *device, bool lite) {

	// keyboard
	device->keypad = input_keypad_create(device->simulator, false, 10, 8, 500, 100,
										 (Signal[10]) {	 signal_split(SIGNAL(bus_kout), 0, 1), signal_split(SIGNAL(bus_kout), 1, 1),
														 signal_split(SIGNAL(bus_kout), 2, 1), signal_split(SIGNAL(bus_kout), 3, 1),
														 signal_split(SIGNAL(bus_kout), 4, 1), signal_split(SIGNAL(bus_kout), 5, 1),
														 signal_split(SIGNAL(bus_kout), 6, 1), signal_split(SIGNAL(bus_kout), 7, 1),
														 signal_split(SIGNAL(bus_kout), 8, 1), signal_split(SIGNAL(bus_kout), 9, 1)},
										 (Signal[8]) {	 signal_split(SIGNAL(bus_kin), 0, 1), signal_split(SIGNAL(bus_kin), 1, 1),
														 signal_split(SIGNAL(bus_kin), 2, 1), signal_split(SIGNAL(bus_kin), 3, 1),
														 signal_split(SIGNAL(bus_kin), 4, 1), signal_split(SIGNAL(bus_kin), 5, 1),
														 signal_split(SIGNAL(bus_kin), 6, 1), signal_split(SIGNAL(bus_kin), 7, 1)}
	);
	DEVICE_REGISTER_CHIP("KEYPAD", device->keypad);

	// display
	if (!lite) {
		device->crt = perif_pet_crt_create(device->simulator, (PerifPetCrtSignals) {
											[PIN_PETCRT_VIDEO_IN] = SIGNAL(video),
											[PIN_PETCRT_HORZ_DRIVE_IN] = SIGNAL(horz_drive),
											[PIN_PETCRT_VERT_DRIVE_IN] = SIGNAL(vert_drive)
		});
		device->screen = device->crt->display;
		DEVICE_REGISTER_CHIP("CRT", device->crt);
	} else {
		device->screen = display_rgba_create(40 * 8, 25 * 8);
		DEVICE_REGISTER_CHIP("DISPLAY", lite_display_create(device));
	}

	// datassette
	device->datassette = perif_datassette_create(device->simulator, (PerifDatassetteSignals) {
											.sense = SIGNAL(cass_switch_1),
											.motor = SIGNAL(cass_motor_1),
											.data_from_ds = SIGNAL(cass_read_1),
											.data_to_ds = SIGNAL(cass_write)
	});
	DEVICE_REGISTER_CHIP("CASS1", device->datassette);
}

// lite-PET: RAM circuitry
void circuit_lite_create_ram(DevCommodorePet *device) {

	Ram8d16a *ram = ram_8d16a_create(15, device->simulator, (Ram8d16aSignals) {
										[CHIP_RAM8D16A_A0] = signal_split(device->signals.bus_ba, 0, 1),
										[CHIP_RAM8D16A_A1] = signal_split(device->signals.bus_ba, 1, 1),
										[CHIP_RAM8D16A_A2] = signal_split(device->signals.bus_ba, 2, 1),
										[CHIP_RAM8D16A_A3] = signal_split(device->signals.bus_ba, 3, 1),
										[CHIP_RAM8D16A_A4] = signal_split(device->signals.bus_ba, 4, 1),
										[CHIP_RAM8D16A_A5] = signal_split(device->signals.bus_ba, 5, 1),
										[CHIP_RAM8D16A_A6] = signal_split(device->signals.bus_ba, 6, 1),
										[CHIP_RAM8D16A_A7] = signal_split(device->signals.bus_ba, 7, 1),
										[CHIP_RAM8D16A_A8] = signal_split(device->signals.bus_ba, 8, 1),
										[CHIP_RAM8D16A_A9] = signal_split(device->signals.bus_ba, 9, 1),
										[CHIP_RAM8D16A_A10] = signal_split(device->signals.bus_ba, 10, 1),
										[CHIP_RAM8D16A_A11] = signal_split(device->signals.bus_ba, 11, 1),
										[CHIP_RAM8D16A_A12] = signal_split(device->signals.bus_ba, 12, 1),
										[CHIP_RAM8D16A_A13] = signal_split(device->signals.bus_ba, 13, 1),
										[CHIP_RAM8D16A_A14] = signal_split(device->signals.bus_ba, 14, 1),
										[CHIP_RAM8D16A_A15] = signal_split(device->signals.bus_ba, 15, 1),

										[CHIP_RAM8D16A_D0] = signal_split(SIGNAL(bus_bd), 0, 1),
										[CHIP_RAM8D16A_D1] = signal_split(SIGNAL(bus_bd), 1, 1),
										[CHIP_RAM8D16A_D2] = signal_split(SIGNAL(bus_bd), 2, 1),
										[CHIP_RAM8D16A_D3] = signal_split(SIGNAL(bus_bd), 3, 1),
										[CHIP_RAM8D16A_D4] = signal_split(SIGNAL(bus_bd), 4, 1),
										[CHIP_RAM8D16A_D5] = signal_split(SIGNAL(bus_bd), 5, 1),
										[CHIP_RAM8D16A_D6] = signal_split(SIGNAL(bus_bd), 6, 1),
										[CHIP_RAM8D16A_D7] = signal_split(SIGNAL(bus_bd), 7, 1),

										[CHIP_RAM8D16A_CE_B] = SIGNAL(ba15),
										[CHIP_RAM8D16A_OE_B] = SIGNAL(g78),
										[CHIP_RAM8D16A_WE_B] = SIGNAL(ram_rw)
	});
	DEVICE_REGISTER_CHIP("RAM", ram);

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC5", glue_logic_create(device, 5));
}

// lite-PET: master timing
void circuit_lite_create_timing(DevCommodorePet *device) {
	device->oscillator_y1 = oscillator_create(1000000, device->simulator, (OscillatorSignals) {
										[CHIP_OSCILLATOR_CLK_OUT] = SIGNAL(clk1)
	});
	DEVICE_REGISTER_CHIP("OSC", device->oscillator_y1);
}

// lite-PET: display ram
void circuit_lite_create_vram(DevCommodorePet *device) {
	assert(device);
	DEVICE_REGISTER_CHIP("VRAM", ram_8d16a_create(10, device->simulator, (Ram8d16aSignals) {
										[CHIP_RAM8D16A_A0] = signal_split(device->signals.bus_ba, 0, 1),
										[CHIP_RAM8D16A_A1] = signal_split(device->signals.bus_ba, 1, 1),
										[CHIP_RAM8D16A_A2] = signal_split(device->signals.bus_ba, 2, 1),
										[CHIP_RAM8D16A_A3] = signal_split(device->signals.bus_ba, 3, 1),
										[CHIP_RAM8D16A_A4] = signal_split(device->signals.bus_ba, 4, 1),
										[CHIP_RAM8D16A_A5] = signal_split(device->signals.bus_ba, 5, 1),
										[CHIP_RAM8D16A_A6] = signal_split(device->signals.bus_ba, 6, 1),
										[CHIP_RAM8D16A_A7] = signal_split(device->signals.bus_ba, 7, 1),
										[CHIP_RAM8D16A_A8] = signal_split(device->signals.bus_ba, 8, 1),
										[CHIP_RAM8D16A_A9] = signal_split(device->signals.bus_ba, 9, 1),

										[CHIP_RAM8D16A_D0] = signal_split(SIGNAL(bus_bd), 0, 1),
										[CHIP_RAM8D16A_D1] = signal_split(SIGNAL(bus_bd), 1, 1),
										[CHIP_RAM8D16A_D2] = signal_split(SIGNAL(bus_bd), 2, 1),
										[CHIP_RAM8D16A_D3] = signal_split(SIGNAL(bus_bd), 3, 1),
										[CHIP_RAM8D16A_D4] = signal_split(SIGNAL(bus_bd), 4, 1),
										[CHIP_RAM8D16A_D5] = signal_split(SIGNAL(bus_bd), 5, 1),
										[CHIP_RAM8D16A_D6] = signal_split(SIGNAL(bus_bd), 6, 1),
										[CHIP_RAM8D16A_D7] = signal_split(SIGNAL(bus_bd), 7, 1),

										[CHIP_RAM8D16A_CE_B] = SIGNAL(sel8_b),
										[CHIP_RAM8D16A_OE_B] = SIGNAL(tv_read_b),
										[CHIP_RAM8D16A_WE_B] = SIGNAL(ram_rw)
	}));

	DEVICE_REGISTER_CHIP("LOGIC7", glue_logic_create(device, 17));
}

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

void dev_commodore_pet_read_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *output);
void dev_commodore_pet_write_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *input);
void dev_commodore_pet_lite_read_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *output);
void dev_commodore_pet_lite_write_memory(DevCommodorePet *device, size_t start_address, size_t size, uint8_t *input);
size_t dev_commodore_pet_get_irq_signals(DevCommodorePet *device, struct SignalBreak **irq_signals);

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
	device->get_irq_signals = (DEVICE_GET_IRQ_SIGNALS) dev_commodore_pet_get_irq_signals;

	device->simulator = simulator_create(6250);		// 6.25 ns - 160 Mhz

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
	SIGNAL_DEFINE_N(cass_motor_2, 1, "CASSMOTOR2");

	SIGNAL_DEFINE(c5_portb, 8);
	SIGNAL(ndac_in_b) = signal_split(SIGNAL(c5_portb), 0, 1);
	SIGNAL(nrfd_out_b) = signal_split(SIGNAL(c5_portb), 1, 1);
	SIGNAL(atn_out_b) = signal_split(SIGNAL(c5_portb), 2, 1);
	SIGNAL(cass_write) = signal_split(SIGNAL(c5_portb), 3, 1);
	SIGNAL(cass_motor_2_b) = signal_split(SIGNAL(c5_portb), 4, 1);
	SIGNAL(video_on_2) = signal_split(SIGNAL(c5_portb), 5, 1);
	SIGNAL(nrfd_in_b) = signal_split(SIGNAL(c5_portb), 6, 1);
	SIGNAL(dav_in_b) = signal_split(SIGNAL(c5_portb), 7, 1);
	signal_set_name(SIGNAL_POOL, SIGNAL(ndac_in_b), "/NDACIN");
	signal_set_name(SIGNAL_POOL, SIGNAL(nrfd_out_b), "/NRFDOUT");
	signal_set_name(SIGNAL_POOL, SIGNAL(atn_out_b), "/ATNOUT");
	signal_set_name(SIGNAL_POOL, SIGNAL(cass_write), "CASSWRITE");
	signal_set_name(SIGNAL_POOL, SIGNAL(cass_motor_2_b), "/CASSMOTOR2");
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
	SIGNAL_DEFINE_N(cass_motor_1_b, 1, "/CASSMOTOR1");

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

	SIGNAL_DEFINE_BOOL_N(clk1, 1, true, "CLK1");

	SIGNAL_DEFINE_N(video_on, 1, "VIDEOON");

	if (!lite) {

		SIGNAL_DEFINE_BOOL_N(clk16, 1, true, "CLK16");
		SIGNAL_DEFINE_BOOL_N(clk8, 1, true, "CLK8");
		SIGNAL_DEFINE_BOOL_N(clk4, 1, true, "CLK4");
		SIGNAL_DEFINE_BOOL_N(clk2, 1, true, "CLK2");

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
	}

	//
	// signals - sheet 7: display logic
	//

	SIGNAL_DEFINE_N(tv_sel, 1, "TVSEL");
	SIGNAL_DEFINE_N(tv_read_b, 1, "/TVREAD");

	if (!lite) {
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
	}

	//
	// signals - sheet 8: display rams
	//

	if (!lite) {
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
	}

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

	if (!lite) {
		// sheet 05: RAMS
		circuit_create_05(device);

		// sheet 06: master timing
		circuit_create_06(device);

		// sheet 07: display logic components
		circuit_create_07(device);

		// sheet 08: display rams components
		circuit_create_08(device);
	} else {
		// main ram
		circuit_lite_create_ram(device);

		// master timing
		circuit_lite_create_timing(device);

		// display ram
		circuit_lite_create_vram(device);
	}

	// peripherals
	circuit_create_peripherals(device, lite);

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
				Ram8d16a *vram = (Ram8d16a *) simulator_chip_by_name(device->simulator, "VRAM");
				memcpy(output + done, vram->data_array + region_offset, to_copy);
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
				Ram8d16a *vram = (Ram8d16a *) simulator_chip_by_name(device->simulator, "VRAM");
				memcpy(vram->data_array + region_offset, input + done, to_copy);
				break;
			}
		}

		remain -= to_copy;
		addr += to_copy;
		done += to_copy;
	}
}

size_t dev_commodore_pet_get_irq_signals(DevCommodorePet *device, struct SignalBreak **irq_signals) {
	assert(device);
	assert(irq_signals);

	static SignalBreak pet_irq[1] = {0};

	if (pet_irq[0].signal.count == 0) {
		pet_irq[0] = (SignalBreak) {device->signals.irq_b, false, true};
	}

	*irq_signals = pet_irq;
	return sizeof(pet_irq) / sizeof(pet_irq[0]);
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
