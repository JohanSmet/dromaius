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

#define SIGNAL_PREFIX		SIG_P2001N_
#define SIGNAL_OWNER		device

#undef  SIGNAL_CHIP_ID
#define SIGNAL_CHIP_ID		chip->id

#undef  SIGNAL_CHIP_LAYER
#define SIGNAL_CHIP_LAYER	chip->signal_layer

///////////////////////////////////////////////////////////////////////////////
//
// internal - glue logic
//

#define CHIP_GLUE_LOGIC_MAX_PIN_COUNT 40

typedef struct ChipGlueLogic {
	CHIP_DECLARE_BASE

	DevCommodorePet *device;
	Signal			signals[CHIP_GLUE_LOGIC_MAX_PIN_COUNT];
} ChipGlueLogic;

static void glue_logic_destroy(ChipGlueLogic *chip);
static uint32_t glue_logic_init_signals_01(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_01(ChipGlueLogic *chip);
static void glue_logic_process_01(ChipGlueLogic *chip);
static uint32_t glue_logic_init_signals_03(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_03(ChipGlueLogic *chip);
static void glue_logic_process_03(ChipGlueLogic *chip);
static uint32_t glue_logic_init_signals_05(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_05(ChipGlueLogic *chip);
static void glue_logic_process_05(ChipGlueLogic *chip);
static uint32_t glue_logic_init_signals_06(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_06(ChipGlueLogic *chip);
static void glue_logic_process_06(ChipGlueLogic *chip);
static uint32_t glue_logic_init_signals_07(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_07(ChipGlueLogic *chip);
static void glue_logic_process_07(ChipGlueLogic *chip);
static uint32_t glue_logic_init_signals_08(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_08(ChipGlueLogic *chip);
static void glue_logic_process_08(ChipGlueLogic *chip);

static void glue_logic_process_07_lite(ChipGlueLogic *chip);
static uint32_t glue_logic_init_signals_07_lite(ChipGlueLogic *chip);
static void glue_logic_register_dependencies_07_lite(ChipGlueLogic *chip);

static ChipGlueLogic *glue_logic_create(DevCommodorePet *device, int sheet) {
	ChipGlueLogic *chip = (ChipGlueLogic *) calloc(1, sizeof(ChipGlueLogic));
	chip->device = device;

	if (sheet == 1) {
		uint32_t count = glue_logic_init_signals_01(chip);
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_01, glue_logic_destroy, glue_logic_register_dependencies_01);
		CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, count);
	} else if (sheet == 3) {
		uint32_t count = glue_logic_init_signals_03(chip);
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_03, glue_logic_destroy, glue_logic_register_dependencies_03);
		CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, count);
	} else if (sheet == 5) {
		uint32_t count = glue_logic_init_signals_05(chip);
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_05, glue_logic_destroy, glue_logic_register_dependencies_05);
		CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, count);
	} else if (sheet == 6) {
		uint32_t count = glue_logic_init_signals_06(chip);
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_06, glue_logic_destroy, glue_logic_register_dependencies_06);
		CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, count);
	} else if (sheet == 7) {
		uint32_t count = glue_logic_init_signals_07(chip);
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_07, glue_logic_destroy, glue_logic_register_dependencies_07);
		CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, count);
	} else if (sheet == 8) {
		uint32_t count = glue_logic_init_signals_08(chip);
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_08, glue_logic_destroy, glue_logic_register_dependencies_08);
		CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, count);
	} else if (sheet == 17) {
		uint32_t count = glue_logic_init_signals_07_lite(chip);
		CHIP_SET_FUNCTIONS(chip, glue_logic_process_07_lite, glue_logic_destroy, glue_logic_register_dependencies_07_lite);
		CHIP_SET_VARIABLES(chip, device->simulator, chip->signals, count);
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

static uint32_t glue_logic_init_signals_01(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	uint32_t pin = 0;
	chip->signals[pin++] = SIGNAL(BA8);
	chip->signals[pin++] = SIGNAL(BA9);
	chip->signals[pin++] = SIGNAL(BA10);
	chip->signals[pin++] = SIGNAL(BA15);
	chip->signals[pin++] = SIGNAL(SEL8_B);
	chip->signals[pin++] = SIGNAL(RW);
	chip->signals[pin++] = SIGNAL(RESET_BTN_B);
	chip->signals[pin++] = SIGNAL(RESET_B);
	chip->signals[pin++] = SIGNAL(RESET);
	chip->signals[pin++] = SIGNAL(INIT_B);
	chip->signals[pin++] = SIGNAL(INIT);
	chip->signals[pin++] = SIGNAL(IFC_B);
	chip->signals[pin++] = SIGNAL(SEL8);
	chip->signals[pin++] = SIGNAL(BA11);
	chip->signals[pin++] = SIGNAL(BA11_B);
	chip->signals[pin++] = SIGNAL(X8XX);
	chip->signals[pin++] = SIGNAL(88XX_B);
	chip->signals[pin++] = SIGNAL(ROMA_B);
	chip->signals[pin++] = SIGNAL(RAMR_B);
	chip->signals[pin++] = SIGNAL(RAMW_B);
	chip->signals[pin++] = SIGNAL(BRW);
	chip->signals[pin++] = SIGNAL(BRW_B);
	chip->signals[pin++] = SIGNAL(RAMRW);
	chip->signals[pin++] = SIGNAL(PHI2);
	chip->signals[pin++] = SIGNAL(BPHI2);
	chip->signals[pin++] = SIGNAL(CPHI2);
	chip->signals[pin++] = SIGNAL(CS1);
	assert(pin <= CHIP_GLUE_LOGIC_MAX_PIN_COUNT);

	return pin;
}

static void glue_logic_register_dependencies_01(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_DEPENDENCY(RESET_B);
	SIGNAL_DEPENDENCY(SEL8_B);
	SIGNAL_DEPENDENCY(SELE_B);
	SIGNAL_DEPENDENCY(BA6);
	SIGNAL_DEPENDENCY(BA8);
	SIGNAL_DEPENDENCY(BA9);
	SIGNAL_DEPENDENCY(BA10);
	SIGNAL_DEPENDENCY(BA11);
	SIGNAL_DEPENDENCY(BA15);
	SIGNAL_DEPENDENCY(RW);
	SIGNAL_DEPENDENCY(CLK1);
}

static void glue_logic_process_01(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	bool ba8 = SIGNAL_READ(BA8);
	bool ba9 = SIGNAL_READ(BA9);
	bool ba10 = SIGNAL_READ(BA10);
	bool ba15 = SIGNAL_READ(BA15);
	bool sel8_b = SIGNAL_READ(SEL8_B);
	bool rw   = SIGNAL_READ(RW);

	SIGNAL_WRITE(RESET_BTN_B, !device->in_reset);
	device->in_reset = false;

	// A3 (1, 2)
	bool reset_b = SIGNAL_READ(RESET_B);
	SIGNAL_WRITE(RESET, !reset_b);

	SIGNAL_WRITE(INIT_B, reset_b);
	SIGNAL_WRITE(INIT, !reset_b);
	SIGNAL_WRITE(IFC_B, reset_b);

	// A3 (11, 10)
	bool sel8 = !sel8_b;
	SIGNAL_WRITE(SEL8, sel8);

	// A3 (3, 4)
	bool ba11_b = !SIGNAL_READ(BA11);
	SIGNAL_WRITE(BA11_B, ba11_b);

	// B2 (1,2,4,5,6)
	bool x8xx = !(ba8 | ba9 | ba10 | ba11_b);
	SIGNAL_WRITE(X8XX, x8xx);

	// A4 (4,5,6)
	bool addr_88xx_b = !(sel8 && x8xx);
	SIGNAL_WRITE(88XX_B, addr_88xx_b);

	// A4 (1,2,3)
	bool rom_addr_b = !(ba15 && sel8_b);
	SIGNAL_WRITE(ROMA_B, rom_addr_b);

	// A5 (3,4,5,6)
	bool ram_read_b = !(rom_addr_b && addr_88xx_b && rw);
	SIGNAL_WRITE(RAMR_B, ram_read_b);
	SIGNAL_WRITE(RAMW_B, !ram_read_b);

	// A10 (3,4)
	SIGNAL_WRITE(BRW, rw);

	// A3 (12,13)
	SIGNAL_WRITE(BRW_B, !rw);

	// A3 (9,8)
	SIGNAL_WRITE(RAMRW, rw);

	// FIXME: cpu doesn't output phi2 clock signal
	bool phi2 = SIGNAL_READ(CLK1);
	SIGNAL_WRITE(PHI2, phi2);
	SIGNAL_WRITE(BPHI2, phi2);
	SIGNAL_WRITE(CPHI2, phi2);

	// >> pia logic
	bool cs1 = x8xx && SIGNAL_READ(BA6);
	SIGNAL_WRITE(CS1, cs1);
}

// glue-logic: cassette & keyboard

static uint32_t glue_logic_init_signals_03(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	uint32_t pin = 0;
	chip->signals[pin++] = SIGNAL(CASS_MOTOR_1);
	chip->signals[pin++] = SIGNAL(CASS_MOTOR_1_B);
	chip->signals[pin++] = SIGNAL(CASS_MOTOR_2);
	chip->signals[pin++] = SIGNAL(CASS_MOTOR_2_B);
	assert(pin <= CHIP_GLUE_LOGIC_MAX_PIN_COUNT);

	return pin;
}

static void glue_logic_register_dependencies_03(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_DEPENDENCY(CASS_MOTOR_1_B);
	SIGNAL_DEPENDENCY(CASS_MOTOR_2_B);
}

static void glue_logic_process_03(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_WRITE(CASS_MOTOR_1, !SIGNAL_READ(CASS_MOTOR_1_B));
	SIGNAL_WRITE(CASS_MOTOR_2, !SIGNAL_READ(CASS_MOTOR_2_B));
}

// glue-logic: rams
static uint32_t glue_logic_init_signals_05(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	uint32_t pin = 0;
	chip->signals[pin++] = SIGNAL(BA15);
	chip->signals[pin++] = SIGNAL(BANKSEL);
	chip->signals[pin++] = SIGNAL(BRW);
	assert(pin <= CHIP_GLUE_LOGIC_MAX_PIN_COUNT);

	return pin;
}

static void glue_logic_register_dependencies_05(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_DEPENDENCY(BA15);
	SIGNAL_DEPENDENCY(BRW);
}

static void glue_logic_process_05(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// B2: 8, 9, 10, 12, 13 (4-Input NOR-gate with strobe)
	bool banksel = !(true && (false || false || false || SIGNAL_READ(BA15)));
	SIGNAL_WRITE(BANKSEL, banksel);

	// G7: 8, 9, 10, 11
	bool g78 = !(true && banksel && SIGNAL_READ(BRW));
	SIGNAL_WRITE(G7_8, g78);
}

// glue-logic: master timing
static uint32_t glue_logic_init_signals_06(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	uint32_t pin = 0;
	chip->signals[pin++] = SIGNAL(BPHI2A_B);
	chip->signals[pin++] = SIGNAL(BPHI2A);
	chip->signals[pin++] = SIGNAL(BPHI2B_B);
	chip->signals[pin++] = SIGNAL(BPHI2B);
	chip->signals[pin++] = SIGNAL(BPHI2F_B);
	chip->signals[pin++] = SIGNAL(BPHI2F);
	chip->signals[pin++] = SIGNAL(BPHI2G_B);
	chip->signals[pin++] = SIGNAL(BPHI2G);
	chip->signals[pin++] = SIGNAL(RA1AND3);
	chip->signals[pin++] = SIGNAL(RA1);
	chip->signals[pin++] = SIGNAL(RA3);
	chip->signals[pin++] = SIGNAL(RA4AND6);
	chip->signals[pin++] = SIGNAL(RA4);
	chip->signals[pin++] = SIGNAL(RA6);
	chip->signals[pin++] = SIGNAL(RA5AND6_B);
	chip->signals[pin++] = SIGNAL(RA5);
	chip->signals[pin++] = SIGNAL(RA6_B);
	chip->signals[pin++] = SIGNAL(BPHI2F);
	chip->signals[pin++] = SIGNAL(BPHI2H);
	chip->signals[pin++] = SIGNAL(VIDEO_LATCH);
	chip->signals[pin++] = SIGNAL(H8Q);
	chip->signals[pin++] = SIGNAL(H8Q2);
	chip->signals[pin++] = SIGNAL(VIDEO_ON);
	chip->signals[pin++] = SIGNAL(H8Q2_B);
	chip->signals[pin++] = SIGNAL(H8Q_B);
	chip->signals[pin++] = SIGNAL(VERT_DRIVE);
	chip->signals[pin++] = SIGNAL(BANKSEL);
	chip->signals[pin++] = SIGNAL(BPHI2);
	chip->signals[pin++] = SIGNAL(H53);
	chip->signals[pin++] = SIGNAL(H1Q1_B);
	chip->signals[pin++] = SIGNAL(H1Q2_B);
	chip->signals[pin++] = SIGNAL(RAS0_B);
	chip->signals[pin++] = SIGNAL(H4Y4);
	chip->signals[pin++] = SIGNAL(BA14);
	chip->signals[pin++] = SIGNAL(CAS1_B);
	chip->signals[pin++] = SIGNAL(BA14);
	chip->signals[pin++] = SIGNAL(BA14_B);
	chip->signals[pin++] = SIGNAL(H4Y4);
	chip->signals[pin++] = SIGNAL(CAS0_B);
	assert(pin <= CHIP_GLUE_LOGIC_MAX_PIN_COUNT);

	return pin;
}

static void glue_logic_register_dependencies_06(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_DEPENDENCY(BPHI2A);
	SIGNAL_DEPENDENCY(BPHI2B);
	SIGNAL_DEPENDENCY(BPHI2F);
	SIGNAL_DEPENDENCY(BPHI2G);
	SIGNAL_DEPENDENCY(BPHI2H);

	SIGNAL_DEPENDENCY(RA1);
	SIGNAL_DEPENDENCY(RA3);
	SIGNAL_DEPENDENCY(RA4);
	SIGNAL_DEPENDENCY(RA5);
	SIGNAL_DEPENDENCY(RA6);
	SIGNAL_DEPENDENCY(RA6_B);

	SIGNAL_DEPENDENCY(H8Q);
	SIGNAL_DEPENDENCY(H8Q_B);
	SIGNAL_DEPENDENCY(H8Q2);
	SIGNAL_DEPENDENCY(H8Q2_B);

	SIGNAL_DEPENDENCY(BPHI2);
	SIGNAL_DEPENDENCY(BANKSEL);

	SIGNAL_DEPENDENCY(H1Q1_B);
	SIGNAL_DEPENDENCY(H1Q2_B);

	SIGNAL_DEPENDENCY(H4Y4);
	SIGNAL_DEPENDENCY(BA14);
}

static void glue_logic_process_06(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// H2 (1,2) + (3,4) + (11,10) + (13,12)
	SIGNAL_WRITE(BPHI2A_B, !SIGNAL_READ(BPHI2A));
	SIGNAL_WRITE(BPHI2B_B, !SIGNAL_READ(BPHI2B));
	SIGNAL_WRITE(BPHI2F_B, !SIGNAL_READ(BPHI2F));
	SIGNAL_WRITE(BPHI2G_B, !SIGNAL_READ(BPHI2G));

	// F1 (8,9,10)
	SIGNAL_WRITE(RA1AND3, SIGNAL_READ(RA1) & SIGNAL_READ(RA3));

	// H10 (4,5,6)
	SIGNAL_WRITE(RA4AND6, SIGNAL_READ(RA4) & SIGNAL_READ(RA6));

	// H10 (1,2,3)
	SIGNAL_WRITE(RA5AND6_B, SIGNAL_READ(RA5) & SIGNAL_READ(RA6_B));

	// G1 (8,9,10)
	bool video_latch = (!SIGNAL_READ(BPHI2F) & SIGNAL_READ(BPHI2H));
	SIGNAL_WRITE(VIDEO_LATCH, video_latch);

	// H10 (11,12,13)
	bool video_on = SIGNAL_READ(H8Q) & SIGNAL_READ(H8Q2);
	SIGNAL_WRITE(VIDEO_ON, video_on);

	// G10 (11,12,13)
	bool vert_drive = !(SIGNAL_READ(H8Q2_B) & SIGNAL_READ(H8Q_B));
	SIGNAL_WRITE(VERT_DRIVE, vert_drive);

	// H5 (1,2,3)
	bool h53 = !(SIGNAL_READ(BANKSEL) && SIGNAL_READ(BPHI2));
	SIGNAL_WRITE(H53, h53);

	// G1 (11,12,13)
	bool ras0_b = SIGNAL_READ(H1Q1_B) && SIGNAL_READ(H1Q2_B);
	SIGNAL_WRITE(RAS0_B, ras0_b);

	// G7 (3,4,5,6)
	bool cas1_b = !(SIGNAL_READ(H4Y4) && SIGNAL_READ(BA14));
	SIGNAL_WRITE(CAS1_B, cas1_b);

	// H2 (8,9)
	bool ba14_b = !SIGNAL_READ(BA14);
	SIGNAL_WRITE(BA14_B, ba14_b);

	// G7 (1,2,12,13)
	bool cas0_b = !(SIGNAL_READ(H4Y4) && ba14_b);
	SIGNAL_WRITE(CAS0_B, cas0_b);
}

// glue-logic: display logic

static uint32_t glue_logic_init_signals_07(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	uint32_t pin = 0;
	chip->signals[pin++] = SIGNAL(BRW);
	chip->signals[pin++] = SIGNAL(BA11_B);
	chip->signals[pin++] = SIGNAL(SEL8);
	chip->signals[pin++] = SIGNAL(TV_SEL);
	chip->signals[pin++] = SIGNAL(TV_READ_B);
	chip->signals[pin++] = SIGNAL(BPHI2);
	chip->signals[pin++] = SIGNAL(A5_12);
	chip->signals[pin++] = SIGNAL(VIDEO_ON_B);
	chip->signals[pin++] = SIGNAL(VIDEO_ON);
	chip->signals[pin++] = SIGNAL(GA6);
	chip->signals[pin++] = SIGNAL(PULLUP_2);
	chip->signals[pin++] = SIGNAL(VIDEO_ON);
	chip->signals[pin++] = SIGNAL(RA9);
	chip->signals[pin++] = SIGNAL(LINES_20_B);
	chip->signals[pin++] = SIGNAL(LGA6);
	chip->signals[pin++] = SIGNAL(LGA7);
	chip->signals[pin++] = SIGNAL(LGA8);
	chip->signals[pin++] = SIGNAL(LGA9);
	chip->signals[pin++] = SIGNAL(LGA_HI_B);
	chip->signals[pin++] = SIGNAL(LGA_HI);
	chip->signals[pin++] = SIGNAL(LGA3);
	chip->signals[pin++] = SIGNAL(LGA_HI);
	chip->signals[pin++] = SIGNAL(LINES_200_B);
	chip->signals[pin++] = SIGNAL(LINE_220);
	chip->signals[pin++] = SIGNAL(HORZ_DISP_OFF);
	chip->signals[pin++] = SIGNAL(W220_OFF);
	chip->signals[pin++] = SIGNAL(RELOAD_B);
	chip->signals[pin++] = SIGNAL(NEXT_B);
	chip->signals[pin++] = SIGNAL(RELOAD_NEXT);
	assert(pin <= CHIP_GLUE_LOGIC_MAX_PIN_COUNT);

	return pin;
}

static void glue_logic_register_dependencies_07(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_DEPENDENCY(BRW);
	SIGNAL_DEPENDENCY(BA11_B);
	SIGNAL_DEPENDENCY(BPHI2);
	SIGNAL_DEPENDENCY(GA6);
	SIGNAL_DEPENDENCY(LGA3);
	SIGNAL_DEPENDENCY(LGA6);
	SIGNAL_DEPENDENCY(LGA7);
	SIGNAL_DEPENDENCY(LGA8);
	SIGNAL_DEPENDENCY(LGA9);
	SIGNAL_DEPENDENCY(RA9);
	SIGNAL_DEPENDENCY(SEL8);
	SIGNAL_DEPENDENCY(VIDEO_ON);
	SIGNAL_DEPENDENCY(PULLUP_2);
	SIGNAL_DEPENDENCY(HORZ_DISP_OFF);
	SIGNAL_DEPENDENCY(RELOAD_B);
	SIGNAL_DEPENDENCY(NEXT_B);
}

static void glue_logic_process_07(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	bool buf_rw = SIGNAL_READ(BRW);

	// >> F1 (4,5,6)
	bool tv_sel = SIGNAL_READ(BA11_B) && SIGNAL_READ(SEL8);
	SIGNAL_WRITE(TV_SEL, tv_sel);

	// >> A5 (8,9,10,11)
	bool tv_read_b = !(true && buf_rw && tv_sel);
	SIGNAL_WRITE(TV_READ_B, tv_read_b);

	// >> A5 (1,2,12,13)
	bool a5_12 = !(tv_sel && !buf_rw && SIGNAL_READ(BPHI2));
	SIGNAL_WRITE(A5_12, a5_12);

	// >> I1 (5,6)
	SIGNAL_WRITE(VIDEO_ON_B, !SIGNAL_READ(VIDEO_ON));

	// >> G2 (1,2,4,5,6)
	bool lines_20_b = !(SIGNAL_READ(GA6) && SIGNAL_READ(PULLUP_2) && !SIGNAL_READ(VIDEO_ON) && SIGNAL_READ(RA9));
	SIGNAL_WRITE(LINES_20_B, lines_20_b);

	// >> G2 (8,9,10,12,13)
	bool lga_hi_b = !(SIGNAL_READ(LGA6) && SIGNAL_READ(LGA7) && SIGNAL_READ(LGA8) && SIGNAL_READ(LGA9));
	SIGNAL_WRITE(LGA_HI_B, lga_hi_b);

	// >> I1 (8,9)
	SIGNAL_WRITE(LGA_HI, !lga_hi_b);

	// >> H5 (4,5,6)
	bool lines_200_b = !(SIGNAL_READ(LGA3) && SIGNAL_READ(LGA_HI));
	SIGNAL_WRITE(LINES_200_B, lines_200_b);

	// >> H5 (8,9,10)
	bool line_220 = !(lines_200_b && lines_20_b);
	SIGNAL_WRITE(LINE_220, line_220);

	// >> G3 (1,2,3)
	bool w220_off = line_220 && SIGNAL_READ(HORZ_DISP_OFF);
	SIGNAL_WRITE(W220_OFF, w220_off);

	// >> H5 (11,12,13)
	bool reload_next = !(SIGNAL_READ(RELOAD_B) && SIGNAL_READ(NEXT_B));
	SIGNAL_WRITE(RELOAD_NEXT, reload_next);
}

static uint32_t glue_logic_init_signals_07_lite(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	uint32_t pin = 0;
	chip->signals[pin++] = SIGNAL(BA11_B);
	chip->signals[pin++] = SIGNAL(SEL8);
	chip->signals[pin++] = SIGNAL(TV_SEL);
	chip->signals[pin++] = SIGNAL(BRW);
	chip->signals[pin++] = SIGNAL(TV_READ_B);
	assert(pin <= CHIP_GLUE_LOGIC_MAX_PIN_COUNT);

	return pin;
}

static void glue_logic_register_dependencies_07_lite(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_DEPENDENCY(BA11_B);
	SIGNAL_DEPENDENCY(SEL8);
	SIGNAL_DEPENDENCY(BRW);
}

static void glue_logic_process_07_lite(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// >> F1 (4,5,6)
	bool tv_sel = SIGNAL_READ(BA11_B) && SIGNAL_READ(SEL8);
	SIGNAL_WRITE(TV_SEL, tv_sel);

	// >> A5 (8,9,10,11)
	bool tv_read_b = !(true && SIGNAL_READ(BRW) && tv_sel);
	SIGNAL_WRITE(TV_READ_B, tv_read_b);
}

// glue-logic: display rams
static uint32_t glue_logic_init_signals_08(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	uint32_t pin = 0;
	chip->signals[pin++] = SIGNAL(HORZ_DISP_ON);
	chip->signals[pin++] = SIGNAL(RA7);
	chip->signals[pin++] = SIGNAL(RA8);
	chip->signals[pin++] = SIGNAL(RA9);
	chip->signals[pin++] = SIGNAL(RELOAD_B);
	chip->signals[pin++] = SIGNAL(G9Q);
	chip->signals[pin++] = SIGNAL(E11QH);
	chip->signals[pin++] = SIGNAL(G106);
	chip->signals[pin++] = SIGNAL(G9Q_B);
	chip->signals[pin++] = SIGNAL(E11QH_B);
	chip->signals[pin++] = SIGNAL(G108);
	chip->signals[pin++] = SIGNAL(H108);
	chip->signals[pin++] = SIGNAL(VIDEO_ON);
	chip->signals[pin++] = SIGNAL(HORZ_DISP_ON);
	chip->signals[pin++] = SIGNAL(VIDEO);
	assert(pin <= CHIP_GLUE_LOGIC_MAX_PIN_COUNT);

	return pin;
}

static void glue_logic_register_dependencies_08(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	SIGNAL_DEPENDENCY(HORZ_DISP_ON);
	SIGNAL_DEPENDENCY(RA7);
	SIGNAL_DEPENDENCY(RA8);
	SIGNAL_DEPENDENCY(RA9);

	SIGNAL_DEPENDENCY(G9Q);
	SIGNAL_DEPENDENCY(G9Q_B);
	SIGNAL_DEPENDENCY(E11QH);
	SIGNAL_DEPENDENCY(E11QH);

	SIGNAL_DEPENDENCY(VIDEO_ON);
	SIGNAL_DEPENDENCY(HORZ_DISP_ON);
}

static void glue_logic_process_08(ChipGlueLogic *chip) {
	assert(chip);
	DevCommodorePet *device = chip->device;

	// g11 (8,9,10,12,13)
	bool reload_b = !(SIGNAL_READ(HORZ_DISP_ON) && SIGNAL_READ(RA7) && SIGNAL_READ(RA8) && SIGNAL_READ(RA9));
	SIGNAL_WRITE(RELOAD_B, reload_b);

	// g10 (4,5,6)
	bool g106 = !(SIGNAL_READ(G9Q) && SIGNAL_READ(E11QH));
	SIGNAL_WRITE(G106, g106);

	// g10 (8,9,10)
	bool g108 = !(SIGNAL_READ(G9Q_B) && SIGNAL_READ(E11QH_B));
	SIGNAL_WRITE(G108, g108);

	// h10 (8,9,10)
	bool h108 = g106 && g108;
	SIGNAL_WRITE(H108, h108);

	// g11 (1,2,5,4,6)
	bool video = !(h108 && true && SIGNAL_READ(VIDEO_ON) && SIGNAL_READ(HORZ_DISP_ON));
	SIGNAL_WRITE(VIDEO, video);
}

///////////////////////////////////////////////////////////////////////////////
//
// internal - pet-lite 'fake' display
//

typedef struct ChipLiteDisplay {
	CHIP_DECLARE_BASE

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

	if (SIGNAL_READ(VIDEO_ON)) {
		// redraw screen
		pet_lite_fake_display(chip);

		// signal vertical retrace
		SIGNAL_WRITE(VIDEO_ON, false);

		// hold vertical retrace for a bit
		chip->schedule_timestamp = chip->simulator->current_tick + chip->retrace_hold;
	} else {
		// end of vertical retrace
		SIGNAL_WRITE(VIDEO_ON, true);
		chip->schedule_timestamp = chip->simulator->current_tick + chip->refresh_delay;
	}
}

///////////////////////////////////////////////////////////////////////////////
//
// internal - rom functions
//

static Chip63xxRom *load_rom(DevCommodorePet *device, const char *filename, size_t num_lines, Signal rom_cs1_b) {

	Chip63xxSignals signals = {
		[CHIP_6332_A0] = SIGNAL(BA0),
		[CHIP_6332_A1] = SIGNAL(BA1),
		[CHIP_6332_A2] = SIGNAL(BA2),
		[CHIP_6332_A3] = SIGNAL(BA3),
		[CHIP_6332_A4] = SIGNAL(BA4),
		[CHIP_6332_A5] = SIGNAL(BA5),
		[CHIP_6332_A6] = SIGNAL(BA6),
		[CHIP_6332_A7] = SIGNAL(BA7),
		[CHIP_6332_A8] = SIGNAL(BA8),
		[CHIP_6332_A9] = SIGNAL(BA9),
		[CHIP_6332_A10] = SIGNAL(BA10),
		[CHIP_6332_A11] = SIGNAL(BA11),			// used as cs2_b for 6316 rom

		[CHIP_6332_D0] = SIGNAL(D0),
		[CHIP_6332_D1] = SIGNAL(D1),
		[CHIP_6332_D2] = SIGNAL(D2),
		[CHIP_6332_D3] = SIGNAL(D3),
		[CHIP_6332_D4] = SIGNAL(D4),
		[CHIP_6332_D5] = SIGNAL(D5),
		[CHIP_6332_D6] = SIGNAL(D6),
		[CHIP_6332_D7] = SIGNAL(D7),

		[CHIP_6332_CS1_B] = rom_cs1_b,
		[CHIP_6332_CS3] = SIGNAL(HIGH)
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
										[CHIP_6316_CS1_B] = SIGNAL(LOW),
										[CHIP_6316_CS2_B] = SIGNAL(LOW),
										[CHIP_6316_CS3] = SIGNAL(INIT_B),
										[CHIP_6316_A0] = SIGNAL(RA7),
										[CHIP_6316_A1] = SIGNAL(RA8),
										[CHIP_6316_A2] = SIGNAL(RA9),
										[CHIP_6316_A3] = SIGNAL(LSD0),
										[CHIP_6316_A4] = SIGNAL(LSD1),
										[CHIP_6316_A5] = SIGNAL(LSD2),
										[CHIP_6316_A6] = SIGNAL(LSD3),
										[CHIP_6316_A7] = SIGNAL(LSD4),
										[CHIP_6316_A8] = SIGNAL(LSD5),
										[CHIP_6316_A9] = SIGNAL(LSD6),
										[CHIP_6316_A10] = SIGNAL(GRAPHIC),
										[CHIP_6316_D0] = SIGNAL(CD0),
										[CHIP_6316_D1] = SIGNAL(CD1),
										[CHIP_6316_D2] = SIGNAL(CD2),
										[CHIP_6316_D3] = SIGNAL(CD3),
										[CHIP_6316_D4] = SIGNAL(CD4),
										[CHIP_6316_D5] = SIGNAL(CD5),
										[CHIP_6316_D6] = SIGNAL(CD6),
										[CHIP_6316_D7] = SIGNAL(CD7)
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
											[CHIP_POR_TRIGGER_B] = SIGNAL(RESET_BTN_B),
											[CHIP_POR_RESET_B] = SIGNAL(RESET_B)
	}));

	// cpu
	device->cpu = cpu_6502_create(device->simulator, (Cpu6502Signals) {
										[PIN_6502_AB0]  = SIGNAL(AB0),
										[PIN_6502_AB1]  = SIGNAL(AB1),
										[PIN_6502_AB2]  = SIGNAL(AB2),
										[PIN_6502_AB3]  = SIGNAL(AB3),
										[PIN_6502_AB4]  = SIGNAL(AB4),
										[PIN_6502_AB5]  = SIGNAL(AB5),
										[PIN_6502_AB6]  = SIGNAL(AB6),
										[PIN_6502_AB7]  = SIGNAL(AB7),
										[PIN_6502_AB8]  = SIGNAL(AB8),
										[PIN_6502_AB9]  = SIGNAL(AB9),
										[PIN_6502_AB10] = SIGNAL(AB10),
										[PIN_6502_AB11] = SIGNAL(AB11),
										[PIN_6502_AB12] = SIGNAL(AB12),
										[PIN_6502_AB13] = SIGNAL(AB13),
										[PIN_6502_AB14] = SIGNAL(AB14),
										[PIN_6502_AB15] = SIGNAL(AB15),

										[PIN_6502_DB0]  = SIGNAL(D0),
										[PIN_6502_DB1]  = SIGNAL(D1),
										[PIN_6502_DB2]  = SIGNAL(D2),
										[PIN_6502_DB3]  = SIGNAL(D3),
										[PIN_6502_DB4]  = SIGNAL(D4),
										[PIN_6502_DB5]  = SIGNAL(D5),
										[PIN_6502_DB6]  = SIGNAL(D6),
										[PIN_6502_DB7]  = SIGNAL(D7),

										[PIN_6502_CLK]   = SIGNAL(CLK1),
										[PIN_6502_RES_B] = SIGNAL(RESET_B),
										[PIN_6502_RW]	 = SIGNAL(RW),
										[PIN_6502_IRQ_B] = SIGNAL(IRQ_B),
										[PIN_6502_NMI_B] = SIGNAL(NMI_B),
										[PIN_6502_SYNC]  = SIGNAL(SYNC),
										[PIN_6502_RDY]   = SIGNAL(RDY)
	});
	DEVICE_REGISTER_CHIP("C4", device->cpu);

	// >> c3 - octal buffer
	DEVICE_REGISTER_CHIP("C3", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(LOW),
										[CHIP_74244_G2_B] = SIGNAL(LOW),
										[CHIP_74244_A11] = SIGNAL(AB0),
										[CHIP_74244_A24] = SIGNAL(AB1),
										[CHIP_74244_A12] = SIGNAL(AB2),
										[CHIP_74244_A23] = SIGNAL(AB3),
										[CHIP_74244_A13] = SIGNAL(AB4),
										[CHIP_74244_A22] = SIGNAL(AB5),
										[CHIP_74244_A14] = SIGNAL(AB6),
										[CHIP_74244_A21] = SIGNAL(AB7),
										[CHIP_74244_Y11] = SIGNAL(BA0),
										[CHIP_74244_Y24] = SIGNAL(BA1),
										[CHIP_74244_Y12] = SIGNAL(BA2),
										[CHIP_74244_Y23] = SIGNAL(BA3),
										[CHIP_74244_Y13] = SIGNAL(BA4),
										[CHIP_74244_Y22] = SIGNAL(BA5),
										[CHIP_74244_Y14] = SIGNAL(BA6),
										[CHIP_74244_Y21] = SIGNAL(BA7),

	}));

	// >> b3 - octal buffer
	DEVICE_REGISTER_CHIP("B3", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(LOW),	// 01
										[CHIP_74244_G2_B] = SIGNAL(LOW),	// 19
										[CHIP_74244_A11]  = SIGNAL(AB8),	// 02
										[CHIP_74244_A24]  = SIGNAL(AB9),	// 17
										[CHIP_74244_A12]  = SIGNAL(AB10),	// 04
										[CHIP_74244_A23]  = SIGNAL(AB11),	// 15
										[CHIP_74244_A13]  = SIGNAL(AB12),	// 06
										[CHIP_74244_A22]  = SIGNAL(AB13),	// 13
										[CHIP_74244_A14]  = SIGNAL(AB14),	// 08
										[CHIP_74244_A21]  = SIGNAL(AB15),	// 11
										[CHIP_74244_Y11]  = SIGNAL(BA8),	// 18
										[CHIP_74244_Y24]  = SIGNAL(BA9),	// 03
										[CHIP_74244_Y12]  = SIGNAL(BA10),	// 16
										[CHIP_74244_Y23]  = SIGNAL(BA11),	// 05
										[CHIP_74244_Y13]  = SIGNAL(BA12),	// 14
										[CHIP_74244_Y22]  = SIGNAL(BA13),	// 07
										[CHIP_74244_Y14]  = SIGNAL(BA14),	// 12
										[CHIP_74244_Y21]  = SIGNAL(BA15),	// 09

	}));

	// >> d2 - 4-to-16 decoder
	DEVICE_REGISTER_CHIP("D2", chip_74154_decoder_create(device->simulator, (Chip74154Signals) {
										[CHIP_74154_G1_B] = SIGNAL(LOW),
										[CHIP_74154_G2_B] = SIGNAL(LOW),
										[CHIP_74154_A] = SIGNAL(BA12),
										[CHIP_74154_B] = SIGNAL(BA13),
										[CHIP_74154_C] = SIGNAL(BA14),
										[CHIP_74154_D] = SIGNAL(BA15),
										[CHIP_74154_Y0_B] = SIGNAL(SEL0_B),
										[CHIP_74154_Y1_B] = SIGNAL(SEL1_B),
										[CHIP_74154_Y2_B] = SIGNAL(SEL2_B),
										[CHIP_74154_Y3_B] = SIGNAL(SEL3_B),
										[CHIP_74154_Y4_B] = SIGNAL(SEL4_B),
										[CHIP_74154_Y5_B] = SIGNAL(SEL5_B),
										[CHIP_74154_Y6_B] = SIGNAL(SEL6_B),
										[CHIP_74154_Y7_B] = SIGNAL(SEL7_B),
										[CHIP_74154_Y8_B] = SIGNAL(SEL8_B),
										[CHIP_74154_Y9_B] = SIGNAL(SEL9_B),
										[CHIP_74154_Y10_B] = SIGNAL(SELA_B),
										[CHIP_74154_Y11_B] = SIGNAL(SELB_B),
										[CHIP_74154_Y12_B] = SIGNAL(SELC_B),
										[CHIP_74154_Y13_B] = SIGNAL(SELD_B),
										[CHIP_74154_Y14_B] = SIGNAL(SELE_B),
										[CHIP_74154_Y15_B] = SIGNAL(SELF_B),
	}));

	// >> e9 - octal buffer
	DEVICE_REGISTER_CHIP("E9", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(RAMW_B),	// 01
										[CHIP_74244_G2_B] = SIGNAL(RAMR_B),	// 19
										[CHIP_74244_A11]  = SIGNAL(D0),		// 02
										[CHIP_74244_Y24]  = SIGNAL(D0),		// 03
										[CHIP_74244_A12]  = SIGNAL(D1),		// 04
										[CHIP_74244_Y23]  = SIGNAL(D1),		// 05
										[CHIP_74244_A13]  = SIGNAL(D2),		// 06
										[CHIP_74244_Y22]  = SIGNAL(D2),		// 07
										[CHIP_74244_A14]  = SIGNAL(D3),		// 08
										[CHIP_74244_Y21]  = SIGNAL(D3),		// 09

										[CHIP_74244_Y11]  = SIGNAL(BD0),	// 18
										[CHIP_74244_A24]  = SIGNAL(BD0),	// 17
										[CHIP_74244_Y12]  = SIGNAL(BD1),	// 16
										[CHIP_74244_A23]  = SIGNAL(BD1),	// 15
										[CHIP_74244_Y13]  = SIGNAL(BD2),	// 14
										[CHIP_74244_A22]  = SIGNAL(BD2),	// 13
										[CHIP_74244_Y14]  = SIGNAL(BD3),	// 12
										[CHIP_74244_A21]  = SIGNAL(BD3)		// 11
	}));

	// >> e10 - octal buffer
	DEVICE_REGISTER_CHIP("E10", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G1_B] = SIGNAL(RAMW_B),	// 01
										[CHIP_74244_G2_B] = SIGNAL(RAMR_B),	// 19
										[CHIP_74244_A11]  = SIGNAL(D4),		// 02
										[CHIP_74244_Y24]  = SIGNAL(D4),		// 03
										[CHIP_74244_A12]  = SIGNAL(D5),		// 04
										[CHIP_74244_Y23]  = SIGNAL(D5),		// 05
										[CHIP_74244_A13]  = SIGNAL(D6),		// 06
										[CHIP_74244_Y22]  = SIGNAL(D6),		// 07
										[CHIP_74244_A14]  = SIGNAL(D7),		// 08
										[CHIP_74244_Y21]  = SIGNAL(D7),		// 09

										[CHIP_74244_Y11]  = SIGNAL(BD4),	// 18
										[CHIP_74244_A24]  = SIGNAL(BD4),	// 17
										[CHIP_74244_Y12]  = SIGNAL(BD5),	// 16
										[CHIP_74244_A23]  = SIGNAL(BD5),	// 15
										[CHIP_74244_Y13]  = SIGNAL(BD6),	// 14
										[CHIP_74244_A22]  = SIGNAL(BD6),	// 13
										[CHIP_74244_Y14]  = SIGNAL(BD7),	// 12
										[CHIP_74244_A21]  = SIGNAL(BD7)		// 11
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC1", glue_logic_create(device, 1));
}

// sheet 02: IEEE-488 Interface
void circuit_create_02(DevCommodorePet *device) {

	// pia 1 (C6 - IEEE-488 interface)
	device->pia_1 = chip_6520_create(device->simulator, (Chip6520Signals) {
										[CHIP_6520_D0] = SIGNAL(D0),
										[CHIP_6520_D1] = SIGNAL(D1),
										[CHIP_6520_D2] = SIGNAL(D2),
										[CHIP_6520_D3] = SIGNAL(D3),
										[CHIP_6520_D4] = SIGNAL(D4),
										[CHIP_6520_D5] = SIGNAL(D5),
										[CHIP_6520_D6] = SIGNAL(D6),
										[CHIP_6520_D7] = SIGNAL(D7),
										[CHIP_6520_PHI2] = SIGNAL(CLK1),
										[CHIP_6520_RESET_B] = SIGNAL(RESET_B),
										[CHIP_6520_RW] = SIGNAL(RW),
										[CHIP_6520_CS0] = SIGNAL(X8XX),
										[CHIP_6520_CS1] = SIGNAL(BA5),
										[CHIP_6520_CS2_B] = SIGNAL(SELE_B),							// io_b on schematic (jumpered to SELE_B)
										[CHIP_6520_RS0] = SIGNAL(BA0),
										[CHIP_6520_RS1] = SIGNAL(BA1),
										[CHIP_6520_CA1] = SIGNAL(ATN_IN_B),
										[CHIP_6520_CA2] = SIGNAL(NDAC_OUT_B),
										[CHIP_6520_PA0] = SIGNAL(DI0),
										[CHIP_6520_PA1] = SIGNAL(DI1),
										[CHIP_6520_PA2] = SIGNAL(DI2),
										[CHIP_6520_PA3] = SIGNAL(DI3),
										[CHIP_6520_PA4] = SIGNAL(DI4),
										[CHIP_6520_PA5] = SIGNAL(DI5),
										[CHIP_6520_PA6] = SIGNAL(DI6),
										[CHIP_6520_PA7] = SIGNAL(DI7),
										[CHIP_6520_PB0] = SIGNAL(DO0),
										[CHIP_6520_PB1] = SIGNAL(DO1),
										[CHIP_6520_PB2] = SIGNAL(DO2),
										[CHIP_6520_PB3] = SIGNAL(DO3),
										[CHIP_6520_PB4] = SIGNAL(DO4),
										[CHIP_6520_PB5] = SIGNAL(DO5),
										[CHIP_6520_PB6] = SIGNAL(DO6),
										[CHIP_6520_PB7] = SIGNAL(DO7),
										[CHIP_6520_CB1] = SIGNAL(SRQ_IN_B),
										[CHIP_6520_CB2] = SIGNAL(DAV_OUT_B),
										[CHIP_6520_IRQA_B] = SIGNAL(IRQ_B),
										[CHIP_6520_IRQB_B] = SIGNAL(IRQ_B)
	});
	DEVICE_REGISTER_CHIP("C6", device->pia_1);
}

// sheet 03: cassette & keyboard
void circuit_create_03(DevCommodorePet *device) {

	// pia 2 (C7 - keyboard)
	device->pia_2 = chip_6520_create(device->simulator, (Chip6520Signals) {
										[CHIP_6520_D0] = SIGNAL(D0),
										[CHIP_6520_D1] = SIGNAL(D1),
										[CHIP_6520_D2] = SIGNAL(D2),
										[CHIP_6520_D3] = SIGNAL(D3),
										[CHIP_6520_D4] = SIGNAL(D4),
										[CHIP_6520_D5] = SIGNAL(D5),
										[CHIP_6520_D6] = SIGNAL(D6),
										[CHIP_6520_D7] = SIGNAL(D7),
										[CHIP_6520_PHI2] = SIGNAL(CLK1),
										[CHIP_6520_RESET_B] = SIGNAL(RESET_B),
										[CHIP_6520_RW] = SIGNAL(RW),
										[CHIP_6520_CS0] = SIGNAL(X8XX),
										[CHIP_6520_CS1] = SIGNAL(BA4),
										[CHIP_6520_CS2_B] = SIGNAL(SELE_B),							// io_b on schematic (jumpered to SELE_B)
										[CHIP_6520_RS0] = SIGNAL(BA0),
										[CHIP_6520_RS1] = SIGNAL(BA1),
										[CHIP_6520_CA1] = SIGNAL(CASS_READ_1),
										[CHIP_6520_CA2] = SIGNAL(EOI_OUT_B),
										[CHIP_6520_PA0] = SIGNAL(KEYA),
										[CHIP_6520_PA1] = SIGNAL(KEYB),
										[CHIP_6520_PA2] = SIGNAL(KEYC),
										[CHIP_6520_PA3] = SIGNAL(KEYD),
										[CHIP_6520_PA4] = SIGNAL(CASS_SWITCH_1),
										[CHIP_6520_PA5] = SIGNAL(CASS_SWITCH_2),
										[CHIP_6520_PA6] = SIGNAL(EOI_IN_B),
										[CHIP_6520_PA7] = SIGNAL(DIAG),
										[CHIP_6520_PB0] = SIGNAL(KIN0),
										[CHIP_6520_PB1] = SIGNAL(KIN1),
										[CHIP_6520_PB2] = SIGNAL(KIN2),
										[CHIP_6520_PB3] = SIGNAL(KIN3),
										[CHIP_6520_PB4] = SIGNAL(KIN4),
										[CHIP_6520_PB5] = SIGNAL(KIN5),
										[CHIP_6520_PB6] = SIGNAL(KIN6),
										[CHIP_6520_PB7] = SIGNAL(KIN7),
										[CHIP_6520_CB1] = SIGNAL(VIDEO_ON),
										[CHIP_6520_CB2] = SIGNAL(CASS_MOTOR_1_B),
										[CHIP_6520_IRQA_B] = SIGNAL(IRQ_B),
										[CHIP_6520_IRQB_B] = SIGNAL(IRQ_B)
	});
	DEVICE_REGISTER_CHIP("C7", device->pia_2);

	// via (C5)
	device->via = chip_6522_create(device->simulator, (Chip6522Signals) {
										[CHIP_6522_D0] = SIGNAL(D0),
										[CHIP_6522_D1] = SIGNAL(D1),
										[CHIP_6522_D2] = SIGNAL(D2),
										[CHIP_6522_D3] = SIGNAL(D3),
										[CHIP_6522_D4] = SIGNAL(D4),
										[CHIP_6522_D5] = SIGNAL(D5),
										[CHIP_6522_D6] = SIGNAL(D6),
										[CHIP_6522_D7] = SIGNAL(D7),
										[CHIP_6522_PHI2] = SIGNAL(CLK1),
										[CHIP_6522_RESET_B] = SIGNAL(RESET_B),
										[CHIP_6522_RW] = SIGNAL(RW),
										[CHIP_6522_CS1] = SIGNAL(CS1),
										[CHIP_6522_CS2_B] = SIGNAL(SELE_B),							// io_b on schematic (jumpered to SELE_B)
										[CHIP_6522_RS0] = SIGNAL(BA0),
										[CHIP_6522_RS1] = SIGNAL(BA1),
										[CHIP_6522_RS2] = SIGNAL(BA2),
										[CHIP_6522_RS3] = SIGNAL(BA3),
										[CHIP_6522_CA1] = SIGNAL(CA1),
										[CHIP_6522_CA2] = SIGNAL(GRAPHIC),
										[CHIP_6522_PA0] = SIGNAL(PA0),
										[CHIP_6522_PA1] = SIGNAL(PA1),
										[CHIP_6522_PA2] = SIGNAL(PA2),
										[CHIP_6522_PA3] = SIGNAL(PA3),
										[CHIP_6522_PA4] = SIGNAL(PA4),
										[CHIP_6522_PA5] = SIGNAL(PA5),
										[CHIP_6522_PA6] = SIGNAL(PA6),
										[CHIP_6522_PA7] = SIGNAL(PA7),
										[CHIP_6522_PB0] = SIGNAL(NDAC_IN_B),
										[CHIP_6522_PB1] = SIGNAL(NRFD_OUT_B),
										[CHIP_6522_PB2] = SIGNAL(ATN_OUT_B),
										[CHIP_6522_PB3] = SIGNAL(CASS_WRITE),
										[CHIP_6522_PB4] = SIGNAL(CASS_MOTOR_2_B),
										[CHIP_6522_PB5] = SIGNAL(VIDEO_ON),
										[CHIP_6522_PB6] = SIGNAL(NRFD_IN_B),
										[CHIP_6522_PB7] = SIGNAL(DAV_IN_B),
										[CHIP_6522_CB1] = SIGNAL(CASS_READ_2),
										[CHIP_6522_CB2] = SIGNAL(CB2),
										[CHIP_6522_IRQ_B] = SIGNAL(IRQ_B)
	});
	DEVICE_REGISTER_CHIP("C5", device->via);

	// >> c9 - bcd decoder
	DEVICE_REGISTER_CHIP("C9", chip_74145_bcd_decoder_create(device->simulator, (Chip74145Signals) {
										[CHIP_74145_A] = SIGNAL(KEYA),
										[CHIP_74145_B] = SIGNAL(KEYB),
										[CHIP_74145_C] = SIGNAL(KEYC),
										[CHIP_74145_D] = SIGNAL(KEYD),
										[CHIP_74145_Y0_B] = SIGNAL(KOUT0),
										[CHIP_74145_Y1_B] = SIGNAL(KOUT1),
										[CHIP_74145_Y2_B] = SIGNAL(KOUT2),
										[CHIP_74145_Y3_B] = SIGNAL(KOUT3),
										[CHIP_74145_Y4_B] = SIGNAL(KOUT4),
										[CHIP_74145_Y5_B] = SIGNAL(KOUT5),
										[CHIP_74145_Y6_B] = SIGNAL(KOUT6),
										[CHIP_74145_Y7_B] = SIGNAL(KOUT7),
										[CHIP_74145_Y8_B] = SIGNAL(KOUT8),
										[CHIP_74145_Y9_B] = SIGNAL(KOUT9)
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC3", glue_logic_create(device, 3));
}


// sheet 04: roms
void circuit_create_04(DevCommodorePet *device) {

	int rom_count = 0;
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-b000.901465-19.bin", 12, SIGNAL(SELB_B));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-c000.901465-20.bin", 12, SIGNAL(SELC_B));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/basic-4-d000.901465-21.bin", 12, SIGNAL(SELD_B));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/edit-4-n.901447-29.bin", 11, SIGNAL(SELE_B));
	device->roms[rom_count++] = load_rom(device, "runtime/commodore_pet/kernal-4.901465-22.bin", 12, SIGNAL(SELF_B));

	for (int i = 0; i < rom_count; ++i) {
		assert(device->roms[i]);
		DEVICE_REGISTER_CHIP("ROM", device->roms[i]);
	}
}

// sheet 05: RAMS
void circuit_create_05(DevCommodorePet *device) {

	DEVICE_REGISTER_CHIP("I11", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G2_B] = SIGNAL(BRW),	// 19
										[CHIP_74244_G1_B] = SIGNAL(G7_8),	// 01

										[CHIP_74244_A11] = SIGNAL(RD7),		// 02
										[CHIP_74244_Y24] = SIGNAL(RD7),		// 03
										[CHIP_74244_A12] = SIGNAL(RD6),		// 04
										[CHIP_74244_Y23] = SIGNAL(RD6),		// 05
										[CHIP_74244_A13] = SIGNAL(RD5),		// 06
										[CHIP_74244_Y22] = SIGNAL(RD5),		// 07
										[CHIP_74244_A14] = SIGNAL(RD4),		// 08
										[CHIP_74244_Y21] = SIGNAL(RD4),		// 09

										[CHIP_74244_Y11] = SIGNAL(BD7),		// 18
										[CHIP_74244_A24] = SIGNAL(BD7),		// 17
										[CHIP_74244_Y12] = SIGNAL(BD6),		// 16
										[CHIP_74244_A23] = SIGNAL(BD6),		// 15
										[CHIP_74244_Y13] = SIGNAL(BD5),		// 14
										[CHIP_74244_A22] = SIGNAL(BD5),		// 13
										[CHIP_74244_Y14] = SIGNAL(BD4),		// 12
										[CHIP_74244_A21] = SIGNAL(BD4)		// 11
	}));

	DEVICE_REGISTER_CHIP("I10", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G2_B] = SIGNAL(BRW),	// 19
										[CHIP_74244_G1_B] = SIGNAL(G7_8),	// 01

										[CHIP_74244_A11]  = SIGNAL(RD3),	// 02
										[CHIP_74244_Y24]  = SIGNAL(RD3),	// 03
										[CHIP_74244_A12]  = SIGNAL(RD2),	// 04
										[CHIP_74244_Y23]  = SIGNAL(RD2),	// 05
										[CHIP_74244_A13]  = SIGNAL(RD1),	// 06
										[CHIP_74244_Y22]  = SIGNAL(RD1),	// 07
										[CHIP_74244_A14]  = SIGNAL(RD0),	// 08
										[CHIP_74244_Y21]  = SIGNAL(RD0),	// 09

										[CHIP_74244_Y11]  = SIGNAL(BD3),	// 18
										[CHIP_74244_A24]  = SIGNAL(BD3),	// 17
										[CHIP_74244_Y12]  = SIGNAL(BD2),	// 16
										[CHIP_74244_A23]  = SIGNAL(BD2),	// 15
										[CHIP_74244_Y13]  = SIGNAL(BD1),	// 14
										[CHIP_74244_A22]  = SIGNAL(BD1),	// 13
										[CHIP_74244_Y14]  = SIGNAL(BD0),	// 12
										[CHIP_74244_A21]  = SIGNAL(BD0)		// 11
	}));

	DEVICE_REGISTER_CHIP("E3", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(LOW),		// pin 1
										[CHIP_74153_C10] = SIGNAL(RA7),		// pin 6
										[CHIP_74153_C11] = SIGNAL(RA7),		// pin 5
										[CHIP_74153_C12] = SIGNAL(BA0),		// pin 4
										[CHIP_74153_C13] = SIGNAL(BA13),	// pin 3
										[CHIP_74153_Y1] = SIGNAL(FA0),		// pin 7

										[CHIP_74153_A] = SIGNAL(MUXA),		// pin 14
										[CHIP_74153_B] = SIGNAL(CLK1),		// pin 2

										[CHIP_74153_G2] = SIGNAL(LOW),		// pin 15
										[CHIP_74153_C20] = SIGNAL(RA1),		// pin 10
										[CHIP_74153_C21] = SIGNAL(RA1),		// pin 11
										[CHIP_74153_C22] = SIGNAL(BA1),		// pin 12
										[CHIP_74153_C23] = SIGNAL(BA7),		// pin 13
										[CHIP_74153_Y2] = SIGNAL(FA1),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E4", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(LOW),		// pin 1
										[CHIP_74153_C10] = SIGNAL(RA2),		// pin 6
										[CHIP_74153_C11] = SIGNAL(RA2),		// pin 5
										[CHIP_74153_C12] = SIGNAL(BA2),		// pin 4
										[CHIP_74153_C13] = SIGNAL(BA8),		// pin 3
										[CHIP_74153_Y1] = SIGNAL(FA2),		// pin 7

										[CHIP_74153_A] = SIGNAL(MUXA),		// pin 14
										[CHIP_74153_B] = SIGNAL(CLK1),		// pin 2

										[CHIP_74153_G2] = SIGNAL(LOW),		// pin 15
										[CHIP_74153_C20] = SIGNAL(RA3),		// pin 10
										[CHIP_74153_C21] = SIGNAL(RA3),		// pin 11
										[CHIP_74153_C22] = SIGNAL(BA3),		// pin 12
										[CHIP_74153_C23] = SIGNAL(BA9),		// pin 13
										[CHIP_74153_Y2] = SIGNAL(FA3),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E5", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(LOW),		// pin 1
										[CHIP_74153_C10] = SIGNAL(RA4),		// pin 6
										[CHIP_74153_C11] = SIGNAL(RA4),		// pin 5
										[CHIP_74153_C12] = SIGNAL(BA4),		// pin 4
										[CHIP_74153_C13] = SIGNAL(BA10),	// pin 3
										[CHIP_74153_Y1] = SIGNAL(FA4),		// pin 7

										[CHIP_74153_A] = SIGNAL(MUXA),		// pin 14
										[CHIP_74153_B] = SIGNAL(CLK1),		// pin 2

										[CHIP_74153_G2] = SIGNAL(LOW),		// pin 15
										[CHIP_74153_C20] = SIGNAL(RA5),		// pin 10
										[CHIP_74153_C21] = SIGNAL(RA5),		// pin 11
										[CHIP_74153_C22] = SIGNAL(BA5),		// pin 12
										[CHIP_74153_C23] = SIGNAL(BA11),	// pin 13
										[CHIP_74153_Y2] = SIGNAL(FA5),		// pin 9
	}));

	DEVICE_REGISTER_CHIP("E6", chip_74153_multiplexer_create(device->simulator, (Chip74153Signals) {
										[CHIP_74153_G1] = SIGNAL(LOW),		// pin 1
										[CHIP_74153_C10] = SIGNAL(RA6),		// pin 6
										[CHIP_74153_C11] = SIGNAL(RA6),		// pin 5
										[CHIP_74153_C12] = SIGNAL(BA6),		// pin 4
										[CHIP_74153_C13] = SIGNAL(BA12),	// pin 3
										[CHIP_74153_Y1] = SIGNAL(FA6),		// pin 7

										[CHIP_74153_A] = SIGNAL(MUXA),		// pin 14
										[CHIP_74153_B] = SIGNAL(CLK1),		// pin 2
	}));

	DEVICE_REGISTER_CHIP("I2-9", chip_8x4116_dram_create(device->simulator, (Chip8x4116DRamSignals) {
										[CHIP_4116_A0] = SIGNAL(FA0),
										[CHIP_4116_A1] = SIGNAL(FA1),
										[CHIP_4116_A2] = SIGNAL(FA2),
										[CHIP_4116_A3] = SIGNAL(FA3),
										[CHIP_4116_A4] = SIGNAL(FA4),
										[CHIP_4116_A5] = SIGNAL(FA5),
										[CHIP_4116_A6] = SIGNAL(FA6),

										[CHIP_4116_DI0] = SIGNAL(RD0),
										[CHIP_4116_DI1] = SIGNAL(RD1),
										[CHIP_4116_DI2] = SIGNAL(RD2),
										[CHIP_4116_DI3] = SIGNAL(RD3),
										[CHIP_4116_DI4] = SIGNAL(RD4),
										[CHIP_4116_DI5] = SIGNAL(RD5),
										[CHIP_4116_DI6] = SIGNAL(RD6),
										[CHIP_4116_DI7] = SIGNAL(RD7),

										[CHIP_4116_DO0] = SIGNAL(RD0),
										[CHIP_4116_DO1] = SIGNAL(RD1),
										[CHIP_4116_DO2] = SIGNAL(RD2),
										[CHIP_4116_DO3] = SIGNAL(RD3),
										[CHIP_4116_DO4] = SIGNAL(RD4),
										[CHIP_4116_DO5] = SIGNAL(RD5),
										[CHIP_4116_DO6] = SIGNAL(RD6),
										[CHIP_4116_DO7] = SIGNAL(RD7),

										[CHIP_4116_WE_B] = SIGNAL(RAMRW),
										[CHIP_4116_RAS_B] = SIGNAL(RAS0_B),
										[CHIP_4116_CAS_B] = SIGNAL(CAS0_B)
	}));

	DEVICE_REGISTER_CHIP("J2-9", chip_8x4116_dram_create(device->simulator, (Chip8x4116DRamSignals) {
										[CHIP_4116_A0] = SIGNAL(FA0),
										[CHIP_4116_A1] = SIGNAL(FA1),
										[CHIP_4116_A2] = SIGNAL(FA2),
										[CHIP_4116_A3] = SIGNAL(FA3),
										[CHIP_4116_A4] = SIGNAL(FA4),
										[CHIP_4116_A5] = SIGNAL(FA5),
										[CHIP_4116_A6] = SIGNAL(FA6),

										[CHIP_4116_DI0] = SIGNAL(RD0),
										[CHIP_4116_DI1] = SIGNAL(RD1),
										[CHIP_4116_DI2] = SIGNAL(RD2),
										[CHIP_4116_DI3] = SIGNAL(RD3),
										[CHIP_4116_DI4] = SIGNAL(RD4),
										[CHIP_4116_DI5] = SIGNAL(RD5),
										[CHIP_4116_DI6] = SIGNAL(RD6),
										[CHIP_4116_DI7] = SIGNAL(RD7),

										[CHIP_4116_DO0] = SIGNAL(RD0),
										[CHIP_4116_DO1] = SIGNAL(RD1),
										[CHIP_4116_DO2] = SIGNAL(RD2),
										[CHIP_4116_DO3] = SIGNAL(RD3),
										[CHIP_4116_DO4] = SIGNAL(RD4),
										[CHIP_4116_DO5] = SIGNAL(RD5),
										[CHIP_4116_DO6] = SIGNAL(RD6),
										[CHIP_4116_DO7] = SIGNAL(RD7),

										[CHIP_4116_WE_B] = SIGNAL(RAMRW),
										[CHIP_4116_RAS_B] = SIGNAL(RAS0_B),
										[CHIP_4116_CAS_B] = SIGNAL(CAS1_B)
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC5", glue_logic_create(device, 5));
}

// sheet 06 - master timing
void circuit_create_06(DevCommodorePet *device) {

	// >> y1 - oscillator
	device->oscillator_y1 = oscillator_create(16000000, device->simulator, (OscillatorSignals) {
										[CHIP_OSCILLATOR_CLK_OUT] = SIGNAL(CLK16)
	});
	DEVICE_REGISTER_CHIP("Y1", device->oscillator_y1);

	// >> g5 - binary counter
	DEVICE_REGISTER_CHIP("G5", chip_74191_binary_counter_create(device->simulator, (Chip74191Signals) {
										[CHIP_74191_ENABLE_B] = SIGNAL(LOW),		// pin 04
										[CHIP_74191_D_U] = SIGNAL(LOW),				// pin 05
										[CHIP_74191_A] = SIGNAL(LOW),				// pin 15
										[CHIP_74191_B] = SIGNAL(LOW),				// pin 01
										[CHIP_74191_C] = SIGNAL(LOW),				// pin 10
										[CHIP_74191_D] = SIGNAL(LOW),				// pin 09
										[CHIP_74191_LOAD_B] = SIGNAL(INIT_B),		// pin 11
										[CHIP_74191_CLK] = SIGNAL(CLK16),			// pin 14
										[CHIP_74191_QA] = SIGNAL(CLK8),				// pin 03
										[CHIP_74191_QB] = SIGNAL(CLK4),				// pin 02
										[CHIP_74191_QC] = SIGNAL(CLK2),				// pin 06
										[CHIP_74191_QD] = SIGNAL(CLK1),				// pin 07
								//		[CHIP_74191_MAX_MIN] = not connected        // pin 12
								//		[CHIP_74191_RCO_B] = not connected          // pin 13
	}));

	// >> h3 - 8-bit shift register
	DEVICE_REGISTER_CHIP("H3", chip_74164_shift_register_create(device->simulator, (Chip74164Signals) {
										[CHIP_74164_A] = SIGNAL(CLK1),				// pin 01
										[CHIP_74164_B] = SIGNAL(HIGH),				// pin 02
										[CHIP_74164_CLK] = SIGNAL(CLK16),			// pin 08
										[CHIP_74164_CLEAR_B] = SIGNAL(INIT_B),		// pin 09
										[CHIP_74164_QA] = SIGNAL(BPHI2A),			// pin 03
										[CHIP_74164_QB] = SIGNAL(BPHI2B),			// pin 04
										[CHIP_74164_QC] = SIGNAL(BPHI2C),			// pin 05
										[CHIP_74164_QD] = SIGNAL(BPHI2D),			// pin 06
										[CHIP_74164_QE] = SIGNAL(BPHI2E),			// pin 10
										[CHIP_74164_QF] = SIGNAL(BPHI2F),			// pin 11
										[CHIP_74164_QG] = SIGNAL(BPHI2G),			// pin 12
										[CHIP_74164_QH] = SIGNAL(BPHI2H),			// pin 13
	}));

	// >> h6 - JK flip-flop
	DEVICE_REGISTER_CHIP("H6", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(INIT_B),		// pin 13
										[CHIP_74107_CLK1] = SIGNAL(BPHI2A_B),		// pin 12
										[CHIP_74107_J1] = SIGNAL(INIT_B),			// pin 1
										[CHIP_74107_K1] = SIGNAL(INIT_B),			// pin 4
										[CHIP_74107_Q1] = SIGNAL(RA1),				// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(RA1_B),			// pin 2

										[CHIP_74107_CLR2_B] = SIGNAL(INIT_B),		// pin 10
										[CHIP_74107_CLK2] = SIGNAL(RA5),			// pin 9
										[CHIP_74107_J2] = SIGNAL(INIT_B),			// pin 8
										[CHIP_74107_K2] = SIGNAL(INIT_B),			// pin 11
										[CHIP_74107_Q2] = SIGNAL(RA6),				// pin 5
										[CHIP_74107_Q2_B] = SIGNAL(RA6_B)			// pin 6
	}));

	// >> h9 - binary counter
	DEVICE_REGISTER_CHIP("H9", chip_7493_binary_counter_create(device->simulator, (Chip7493Signals) {
										[CHIP_7493_A_B] = SIGNAL(RA1),				// pin 14
										[CHIP_7493_B_B] = SIGNAL(RA2),				// pin 1
										[CHIP_7493_R01] = SIGNAL(INIT),				// pin 2
										[CHIP_7493_R02] = SIGNAL(INIT),				// pin 3
										[CHIP_7493_QA] = SIGNAL(RA2),				// pin 12
										[CHIP_7493_QB] = SIGNAL(RA3),				// pin 9
										[CHIP_7493_QC] = SIGNAL(RA4),				// pin 8
										[CHIP_7493_QD] = SIGNAL(RA5),				// pin 11
	}));

	// >> g9 - d flip-flop (2 flipflop is used on sheet 8)
	DEVICE_REGISTER_CHIP("G9", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										[CHIP_7474_D1] = SIGNAL(INIT_B),			// pin 2
										[CHIP_7474_CLR1_B] = SIGNAL(BPHI2B),		// pin 1
										[CHIP_7474_CLK1] = SIGNAL(BPHI2A_B),		// pin 3
										[CHIP_7474_PR1_B] = SIGNAL(HIGH),			// pin 4
										[CHIP_7474_Q1] = SIGNAL(LOAD_SR),			// pin 5
										[CHIP_7474_Q1_B] = SIGNAL(LOAD_SR_B),		// pin 6

										[CHIP_7474_D2] = SIGNAL(LSD7),				// pin 12
										[CHIP_7474_CLR2_B] = SIGNAL(INIT_B),		// pin 13
										[CHIP_7474_CLK2] = SIGNAL(LOAD_SR),			// pin 11
										[CHIP_7474_PR2_B] = SIGNAL(HIGH),			// pin 10
										[CHIP_7474_Q2] = SIGNAL(G9Q),				// pin 9
										[CHIP_7474_Q2_B] = SIGNAL(G9Q_B),			// pin 8
	}));

	// >> h7 - JK flip-flop
	DEVICE_REGISTER_CHIP("H7", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(INIT_B),		// pin 13
										[CHIP_74107_CLK1] = SIGNAL(RA1AND3),		// pin 12
										[CHIP_74107_J1] = SIGNAL(HORZ_DISP_ON),		// pin 1
										[CHIP_74107_K1] = SIGNAL(HORZ_DISP_OFF),	// pin 4
										[CHIP_74107_Q1] = SIGNAL(HORZ_DRIVE_B),		// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(HORZ_DRIVE),		// pin 2

										[CHIP_74107_CLR2_B] = SIGNAL(INIT_B),		// pin 10
										[CHIP_74107_CLK2] = SIGNAL(LOAD_SR_B),		// pin 9
										[CHIP_74107_J2] = SIGNAL(RA4AND6),			// pin 8
										[CHIP_74107_K2] = SIGNAL(RA5AND6_B),		// pin 11
										[CHIP_74107_Q2] = SIGNAL(HORZ_DISP_ON),		// pin 5
										[CHIP_74107_Q2_B] = SIGNAL(HORZ_DISP_OFF)	// pin 6
	}));

	// >> h8 - JK flip-flop
	DEVICE_REGISTER_CHIP("H8", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(INIT_B),		// pin 13
										[CHIP_74107_CLK1] = SIGNAL(NEXT),			// pin 12
										[CHIP_74107_J1] = SIGNAL(H8Q2_B),			// pin 1
										[CHIP_74107_K1] = SIGNAL(H8Q2),				// pin 4
										[CHIP_74107_Q1] = SIGNAL(H8Q),				// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(H8Q_B),			// pin 2

										[CHIP_74107_CLR2_B] = SIGNAL(INIT_B),		// pin 10
										[CHIP_74107_CLK2] = SIGNAL(NEXT),			// pin 9
										[CHIP_74107_J2] = SIGNAL(H8Q),				// pin 8
										[CHIP_74107_K2] = SIGNAL(H8Q_B),			// pin 11
										[CHIP_74107_Q2] = SIGNAL(H8Q2),				// pin 5
										[CHIP_74107_Q2_B] = SIGNAL(H8Q2_B)			// pin 6
	}));

	// >> h4 - Quad 2to1 multiplexer
	DEVICE_REGISTER_CHIP("H4", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0A] = SIGNAL(BPHI2B),			// pin 2
										[CHIP_74157_I1A] = SIGNAL(BPHI2A),			// pin 3
										[CHIP_74157_ZA] = SIGNAL(H4Y1),				// pin 4

										[CHIP_74157_I0D] = SIGNAL(BPHI2C),			// pin 11
										[CHIP_74157_I1D] = SIGNAL(BPHI2B),			// pin 10
										[CHIP_74157_ZD] = SIGNAL(MUXA),				// pin 9

										[CHIP_74157_I0C] = SIGNAL(BPHI2D),			// pin 14
										[CHIP_74157_I1C] = SIGNAL(BPHI2C),			// pin 13
										[CHIP_74157_ZC] = SIGNAL(H4Y4),				// pin 12

										[CHIP_74157_SEL] = SIGNAL(BRW),				// pin 1
										[CHIP_74157_ENABLE_B] = SIGNAL(H53)			// pin 15
	}));

	// >> h1 - d flipflop
	DEVICE_REGISTER_CHIP("H1", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										[CHIP_7474_D1] = SIGNAL(INIT_B),			// pin 2
										[CHIP_7474_CLR1_B] = SIGNAL(BPHI2G_B),		// pin 1
										[CHIP_7474_CLK1] = SIGNAL(H4Y1),			// pin 3
										[CHIP_7474_PR1_B] = SIGNAL(INIT_B),			// pin 4
										[CHIP_7474_Q1] = SIGNAL(H1Q1),				// pin 5
										[CHIP_7474_Q1_B] = SIGNAL(H1Q1_B),			// pin 6

										[CHIP_7474_D2] = SIGNAL(INIT_B),			// pin 12
										[CHIP_7474_CLR2_B] = SIGNAL(BPHI2F),		// pin 13
										[CHIP_7474_CLK2] = SIGNAL(BPHI2B_B),		// pin 11
										[CHIP_7474_PR2_B] = SIGNAL(PULLUP_2),		// pin 10
										[CHIP_7474_Q2] = SIGNAL(H1Q2),				// pin 9
										[CHIP_7474_Q2_B] = SIGNAL(H1Q2_B)			// pin 8
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC6", glue_logic_create(device, 6));
}

// sheet 07 - display logic components
void circuit_create_07(DevCommodorePet *device) {

	// >> g6 - JK flip-flop
	DEVICE_REGISTER_CHIP("G6", chip_74107_jk_flipflop_create(device->simulator, (Chip74107Signals) {
										[CHIP_74107_CLR1_B] = SIGNAL(HORZ_DISP_ON),	// pin 13
										[CHIP_74107_CLK1] = SIGNAL(RA1_B),			// pin 12
										[CHIP_74107_J1] = SIGNAL(INIT_B),			// pin 1
										[CHIP_74107_K1] = SIGNAL(INIT_B),			// pin 4
										[CHIP_74107_Q1] = SIGNAL(G6_Q),				// pin 3
										[CHIP_74107_Q1_B] = SIGNAL(G6_Q_B),			// pin 2
	}));

	// >> f6 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F6", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0D] = SIGNAL(HIGH),			// pin 11
										[CHIP_74157_I1D] = SIGNAL(HIGH),			// pin 10
										[CHIP_74157_I0B] = SIGNAL(HIGH),			// pin 05
										[CHIP_74157_I1B] = SIGNAL(A5_12),			// pin 06
										[CHIP_74157_I0C] = SIGNAL(RA1_B),			// pin 14
										[CHIP_74157_I1C] = SIGNAL(BA0),				// pin 13
										[CHIP_74157_I0A] = SIGNAL(G6_Q),			// pin 02
										[CHIP_74157_I1A] = SIGNAL(BA1),				// pin 03

										[CHIP_74157_SEL] = SIGNAL(CLK1),			// pin 01
										[CHIP_74157_ENABLE_B] = SIGNAL(LOW),		// pin 15

										[CHIP_74157_ZB] = SIGNAL(TV_RAM_RW),		// pin 07
										[CHIP_74157_ZD] = SIGNAL(F6_Y3),			// pin 09
										[CHIP_74157_ZC] = SIGNAL(SA0),				// pin 12
										[CHIP_74157_ZA] = SIGNAL(SA1)				// pin 04
	}));

	// >> f5 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F5", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0D] = SIGNAL(GA2),				// pin 11
										[CHIP_74157_I1D] = SIGNAL(BA2),				// pin 10
										[CHIP_74157_I0B] = SIGNAL(GA3),				// pin 05
										[CHIP_74157_I1B] = SIGNAL(BA3),				// pin 06
										[CHIP_74157_I0C] = SIGNAL(GA4),				// pin 14
										[CHIP_74157_I1C] = SIGNAL(BA4),				// pin 13
										[CHIP_74157_I0A] = SIGNAL(GA5),				// pin 02
										[CHIP_74157_I1A] = SIGNAL(BA5),				// pin 03

										[CHIP_74157_SEL] = SIGNAL(CLK1),			// pin 01
										[CHIP_74157_ENABLE_B] = SIGNAL(LOW),		// pin 15

										[CHIP_74157_ZD] = SIGNAL(SA2),				// pin 09
										[CHIP_74157_ZB] = SIGNAL(SA3),				// pin 07
										[CHIP_74157_ZC] = SIGNAL(SA4),				// pin 12
										[CHIP_74157_ZA] = SIGNAL(SA5)				// pin 04
	}));

	// >> f3 - quad 2-to-1 multiplexer
	DEVICE_REGISTER_CHIP("F3", chip_74157_multiplexer_create(device->simulator, (Chip74157Signals) {
										[CHIP_74157_I0A] = SIGNAL(GA6),				// pin 02
										[CHIP_74157_I1A] = SIGNAL(BA6),				// pin 03
										[CHIP_74157_I0C] = SIGNAL(GA7),				// pin 14
										[CHIP_74157_I1C] = SIGNAL(BA7),				// pin 13
										[CHIP_74157_I0B] = SIGNAL(GA8),				// pin 05
										[CHIP_74157_I1B] = SIGNAL(BA8),				// pin 06
										[CHIP_74157_I0D] = SIGNAL(GA9),				// pin 11
										[CHIP_74157_I1D] = SIGNAL(BA9),				// pin 10

										[CHIP_74157_SEL] = SIGNAL(CLK1),			// pin 01
										[CHIP_74157_ENABLE_B] = SIGNAL(LOW),		// pin 15

										[CHIP_74157_ZA] = SIGNAL(SA6),				// pin 04
										[CHIP_74157_ZC] = SIGNAL(SA7),				// pin 12
										[CHIP_74157_ZB] = SIGNAL(SA8),				// pin 07
										[CHIP_74157_ZD] = SIGNAL(SA9)				// pin 09
	}));

	// >> f4 - binary counter
	DEVICE_REGISTER_CHIP("F4", chip_74177_binary_counter_create(device->simulator, (Chip74177Signals) {
										[CHIP_74177_CLK2] = SIGNAL(GA2),				// pin 06
										[CHIP_74177_CLK1] = SIGNAL(G6_Q),				// pin 08
										[CHIP_74177_LOAD_B] = SIGNAL(HORZ_DISP_ON),		// pin 01
										[CHIP_74177_CLEAR_B] = SIGNAL(NEXT_B),			// pin 13
										[CHIP_74177_A] = SIGNAL(LGA2),					// pin 04
										[CHIP_74177_B] = SIGNAL(LGA3),					// pin 10
										[CHIP_74177_C] = SIGNAL(LGA4),					// pin 03
										[CHIP_74177_D] = SIGNAL(LGA5),					// pin 11

										[CHIP_74177_QA] = SIGNAL(GA2),					// pin 05
										[CHIP_74177_QB] = SIGNAL(GA3),					// pin 09
										[CHIP_74177_QC] = SIGNAL(GA4),					// pin 02
										[CHIP_74177_QD] = SIGNAL(GA5)					// pin 12
	}));

	// >> f2 - binary counter
	DEVICE_REGISTER_CHIP("F2", chip_74177_binary_counter_create(device->simulator, (Chip74177Signals) {
										[CHIP_74177_CLK2] = SIGNAL(GA6),				// pin 06
										[CHIP_74177_CLK1] = SIGNAL(GA5),				// pin 08
										[CHIP_74177_LOAD_B] = SIGNAL(HORZ_DISP_ON),		// pin 01
										[CHIP_74177_CLEAR_B] = SIGNAL(NEXT_B),			// pin 13
										[CHIP_74177_A] = SIGNAL(LGA6),					// pin 04
										[CHIP_74177_B] = SIGNAL(LGA7),					// pin 10
										[CHIP_74177_C] = SIGNAL(LGA8),					// pin 03
										[CHIP_74177_D] = SIGNAL(LGA9),					// pin 11

										[CHIP_74177_QA] = SIGNAL(GA6),					// pin 05
										[CHIP_74177_QB] = SIGNAL(GA7),					// pin 09
										[CHIP_74177_QC] = SIGNAL(GA8),					// pin 02
										[CHIP_74177_QD] = SIGNAL(GA9)					// pin 12
	}));

	// >> g3 - 8-bit latch
	DEVICE_REGISTER_CHIP("G3", chip_74373_latch_create(device->simulator, (Chip74373Signals) {
										[CHIP_74373_D5] = SIGNAL(GA2),			// pin 13
										[CHIP_74373_D8] = SIGNAL(GA3),			// pin 18
										[CHIP_74373_D6] = SIGNAL(GA4),			// pin 14
										[CHIP_74373_D7] = SIGNAL(GA5),			// pin 17
										[CHIP_74373_D2] = SIGNAL(GA6),			// pin 04
										[CHIP_74373_D3] = SIGNAL(GA7),			// pin 07
										[CHIP_74373_D1] = SIGNAL(GA8),			// pin 03
										[CHIP_74373_D4] = SIGNAL(GA9),			// pin 08

										[CHIP_74373_C] = SIGNAL(RELOAD_NEXT),	// pin 11 - enable input
										[CHIP_74373_OC_B] = SIGNAL(LOW),		// pin 01 - output control

										[CHIP_74373_Q5] = SIGNAL(LGA2),			// pin 12
										[CHIP_74373_Q8] = SIGNAL(LGA3),			// pin 19
										[CHIP_74373_Q6] = SIGNAL(LGA4),			// pin 15
										[CHIP_74373_Q7] = SIGNAL(LGA5),			// pin 16
										[CHIP_74373_Q2] = SIGNAL(LGA6),			// pin 05
										[CHIP_74373_Q3] = SIGNAL(LGA7),			// pin 06
										[CHIP_74373_Q1] = SIGNAL(LGA8),			// pin 02
										[CHIP_74373_Q4] = SIGNAL(LGA9)			// pin 09
	}));

	// >> g8 - d flip-flop
	DEVICE_REGISTER_CHIP("G8", chip_7474_d_flipflop_create(device->simulator, (Chip7474Signals) {
										// [CHIP_7474_D1] = not used,				// pin 2
										// [CHIP_7474_CLR1_B] = not used,			// pin 1
										// [CHIP_7474_CLK1] = not used,				// pin 3
										// [CHIP_7474_PR1_B] = not used,			// pin 4
										// [CHIP_7474_Q1] = not used,				// pin 5
										// [CHIP_7474_Q1_B] = not used,				// pin 6

										[CHIP_7474_PR2_B] = SIGNAL(PULLUP_1),		// pin 10
										[CHIP_7474_D2] = SIGNAL(W220_OFF),			// pin 12
										[CHIP_7474_CLK2] = SIGNAL(VIDEO_LATCH),		// pin 11
										[CHIP_7474_CLR2_B] = SIGNAL(BPHI2H),		// pin 13
										[CHIP_7474_Q2] = SIGNAL(NEXT),				// pin 9
										[CHIP_7474_Q2_B] = SIGNAL(NEXT_B)			// pin 8
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC7", glue_logic_create(device, 7));
}

// sheet 08: display rams components
void circuit_create_08(DevCommodorePet *device) {

	// >> h11 - binary counter
	DEVICE_REGISTER_CHIP("H11", chip_7493_binary_counter_create(device->simulator, (Chip7493Signals) {
										[CHIP_7493_A_B] = SIGNAL(LOW),				// pin 14
										[CHIP_7493_B_B] = SIGNAL(HORZ_DISP_ON),		// pin 1
										[CHIP_7493_R01] = SIGNAL(NEXT),				// pin 2
										[CHIP_7493_R02] = SIGNAL(NEXT),				// pin 3
										// [CHIP_7493_QA] = not used,				// pin 12
										[CHIP_7493_QB] = SIGNAL(RA7),				// pin 9
										[CHIP_7493_QC] = SIGNAL(RA8),				// pin 8
										[CHIP_7493_QD] = SIGNAL(RA9),				// pin 11
	}));

	// >> f7 - 1k x 4bit SRAM
	DEVICE_REGISTER_CHIP("F7", chip_6114_sram_create(device->simulator, (Chip6114SRamSignals) {
										[CHIP_6114_A0] = SIGNAL(SA0),
										[CHIP_6114_A1] = SIGNAL(SA1),
										[CHIP_6114_A2] = SIGNAL(SA2),
										[CHIP_6114_A3] = SIGNAL(SA3),
										[CHIP_6114_A4] = SIGNAL(SA4),
										[CHIP_6114_A5] = SIGNAL(SA5),
										[CHIP_6114_A6] = SIGNAL(SA6),
										[CHIP_6114_A7] = SIGNAL(SA7),
										[CHIP_6114_A8] = SIGNAL(SA8),
										[CHIP_6114_A9] = SIGNAL(SA9),

										[CHIP_6114_IO0] = SIGNAL(SD4),
										[CHIP_6114_IO1] = SIGNAL(SD5),
										[CHIP_6114_IO2] = SIGNAL(SD6),
										[CHIP_6114_IO3] = SIGNAL(SD7),

										[CHIP_6114_CE_B] = SIGNAL(LOW),
										[CHIP_6114_RW] = SIGNAL(TV_RAM_RW)
	}));

	// >> f8 - 1k x 4bit SRAM
	DEVICE_REGISTER_CHIP("F8", chip_6114_sram_create(device->simulator, (Chip6114SRamSignals) {
										[CHIP_6114_A0] = SIGNAL(SA0),
										[CHIP_6114_A1] = SIGNAL(SA1),
										[CHIP_6114_A2] = SIGNAL(SA2),
										[CHIP_6114_A3] = SIGNAL(SA3),
										[CHIP_6114_A4] = SIGNAL(SA4),
										[CHIP_6114_A5] = SIGNAL(SA5),
										[CHIP_6114_A6] = SIGNAL(SA6),
										[CHIP_6114_A7] = SIGNAL(SA7),
										[CHIP_6114_A8] = SIGNAL(SA8),
										[CHIP_6114_A9] = SIGNAL(SA9),

										[CHIP_6114_IO0] = SIGNAL(SD0),
										[CHIP_6114_IO1] = SIGNAL(SD1),
										[CHIP_6114_IO2] = SIGNAL(SD2),
										[CHIP_6114_IO3] = SIGNAL(SD3),

										[CHIP_6114_CE_B] = SIGNAL(LOW),
										[CHIP_6114_RW] = SIGNAL(TV_RAM_RW)
	}));

	// >> e8 - octal buffer
	DEVICE_REGISTER_CHIP("E8", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G2_B] = SIGNAL(TV_RAM_RW),	// 19
										[CHIP_74244_G1_B] = SIGNAL(TV_READ_B),	// 01

										[CHIP_74244_A11]  = SIGNAL(SD0),		// 02
										[CHIP_74244_Y24]  = SIGNAL(SD0),		// 03
										[CHIP_74244_A12]  = SIGNAL(SD1),		// 04
										[CHIP_74244_Y23]  = SIGNAL(SD1),		// 05
										[CHIP_74244_A13]  = SIGNAL(SD2),		// 06
										[CHIP_74244_Y22]  = SIGNAL(SD2),		// 07
										[CHIP_74244_A14]  = SIGNAL(SD3),		// 08
										[CHIP_74244_Y21]  = SIGNAL(SD3),		// 09

										[CHIP_74244_Y11]  = SIGNAL(BD0),		// 18
										[CHIP_74244_A24]  = SIGNAL(BD0),		// 17
										[CHIP_74244_Y12]  = SIGNAL(BD1),		// 16
										[CHIP_74244_A23]  = SIGNAL(BD1),		// 15
										[CHIP_74244_Y13]  = SIGNAL(BD2),		// 14
										[CHIP_74244_A22]  = SIGNAL(BD2),		// 13
										[CHIP_74244_Y14]  = SIGNAL(BD3),		// 12
										[CHIP_74244_A21]  = SIGNAL(BD3)			// 11
	}));

	// >> e7 - octal buffer
	DEVICE_REGISTER_CHIP("E7", chip_74244_octal_buffer_create(device->simulator, (Chip74244Signals) {
										[CHIP_74244_G2_B] = SIGNAL(TV_RAM_RW),	// 19
										[CHIP_74244_G1_B] = SIGNAL(TV_READ_B),	// 01

										[CHIP_74244_A11]  = SIGNAL(SD4),		// 02
										[CHIP_74244_Y24]  = SIGNAL(SD4),		// 03
										[CHIP_74244_A12]  = SIGNAL(SD5),		// 04
										[CHIP_74244_Y23]  = SIGNAL(SD5),		// 05
										[CHIP_74244_A13]  = SIGNAL(SD6),		// 06
										[CHIP_74244_Y22]  = SIGNAL(SD6),		// 07
										[CHIP_74244_A14]  = SIGNAL(SD7),		// 08
										[CHIP_74244_Y21]  = SIGNAL(SD7),		// 09

										[CHIP_74244_Y11]  = SIGNAL(BD4),		// 18
										[CHIP_74244_A24]  = SIGNAL(BD4),		// 17
										[CHIP_74244_Y12]  = SIGNAL(BD5),		// 16
										[CHIP_74244_A23]  = SIGNAL(BD5),		// 15
										[CHIP_74244_Y13]  = SIGNAL(BD6),		// 14
										[CHIP_74244_A22]  = SIGNAL(BD6),		// 13
										[CHIP_74244_Y14]  = SIGNAL(BD7),		// 12
										[CHIP_74244_A21]  = SIGNAL(BD7)			// 11
	}));

	// >> f9 - 8-bit latch
	DEVICE_REGISTER_CHIP("F9", chip_74373_latch_create(device->simulator, (Chip74373Signals) {
										[CHIP_74373_C] = SIGNAL(VIDEO_LATCH),	// 11
										[CHIP_74373_OC_B] = SIGNAL(LOW),		// 1
										[CHIP_74373_D1] = SIGNAL(SD0),			// 3
										[CHIP_74373_D8] = SIGNAL(SD1),			// 18
										[CHIP_74373_D2] = SIGNAL(SD2),			// 4
										[CHIP_74373_D7] = SIGNAL(SD3),			// 17
										[CHIP_74373_D3] = SIGNAL(SD4),			// 7
										[CHIP_74373_D6] = SIGNAL(SD5),			// 14
										[CHIP_74373_D4] = SIGNAL(SD6),			// 8
										[CHIP_74373_D5] = SIGNAL(SD7),			// 13

										[CHIP_74373_Q1] = SIGNAL(LSD0),			// 2
										[CHIP_74373_Q8] = SIGNAL(LSD1),			// 19
										[CHIP_74373_Q2] = SIGNAL(LSD2),			// 5
										[CHIP_74373_Q7] = SIGNAL(LSD3),			// 16
										[CHIP_74373_Q3] = SIGNAL(LSD4),			// 6
										[CHIP_74373_Q6] = SIGNAL(LSD5),			// 15
										[CHIP_74373_Q4] = SIGNAL(LSD6),			// 9
										[CHIP_74373_Q5] = SIGNAL(LSD7),			// 12
	}));

	// >> f10 - character rom
	Chip63xxRom *char_rom = load_character_rom(device, "runtime/commodore_pet/characters-2.901447-10.bin");
	assert(char_rom);
	DEVICE_REGISTER_CHIP("F10", char_rom);

	// >> e11 - shift register
	DEVICE_REGISTER_CHIP("E11", chip_74165_shift_register_create(device->simulator, (Chip74165Signals) {
										[CHIP_74165_SL		] = SIGNAL(LOAD_SR_B),	// 1
										[CHIP_74165_CLK	] = SIGNAL(CLK8),			// 2
										[CHIP_74165_CLK_INH] = SIGNAL(LOW),			// 15
										[CHIP_74165_SI		] = SIGNAL(CD7),		// 10
										[CHIP_74165_A		] = SIGNAL(CD0),		// 11
										[CHIP_74165_B		] = SIGNAL(CD1),		// 12
										[CHIP_74165_C		] = SIGNAL(CD2),		// 13
										[CHIP_74165_D		] = SIGNAL(CD3),		// 14
										[CHIP_74165_E		] = SIGNAL(CD4),		// 3
										[CHIP_74165_F		] = SIGNAL(CD5),		// 4
										[CHIP_74165_G		] = SIGNAL(CD6),		// 5
										[CHIP_74165_H		] = SIGNAL(CD7),		// 6
										[CHIP_74165_QH		] = SIGNAL(E11QH),		// 9
										[CHIP_74165_QH_B	] = SIGNAL(E11QH_B),	// 7
	}));

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC8", glue_logic_create(device, 8));
}

// peripherals
void circuit_create_peripherals(DevCommodorePet *device, bool lite) {

	// keyboard
	device->keypad = input_keypad_create(device->simulator, false, 10, 8, 500, 100,
										 (Signal[10]) {	 SIGNAL(KOUT0), SIGNAL(KOUT1),
														 SIGNAL(KOUT2), SIGNAL(KOUT3),
														 SIGNAL(KOUT4), SIGNAL(KOUT5),
														 SIGNAL(KOUT6), SIGNAL(KOUT7),
														 SIGNAL(KOUT8), SIGNAL(KOUT9)},
										 (Signal[8]) {	 SIGNAL(KIN0), SIGNAL(KIN1),
														 SIGNAL(KIN2), SIGNAL(KIN3),
														 SIGNAL(KIN4), SIGNAL(KIN5),
														 SIGNAL(KIN6), SIGNAL(KIN7)}
	);
	DEVICE_REGISTER_CHIP("KEYPAD", device->keypad);

	// display
	if (!lite) {
		device->crt = perif_pet_crt_create(device->simulator, (PerifPetCrtSignals) {
											[PIN_PETCRT_VIDEO_IN] = SIGNAL(VIDEO),
											[PIN_PETCRT_HORZ_DRIVE_IN] = SIGNAL(HORZ_DRIVE),
											[PIN_PETCRT_VERT_DRIVE_IN] = SIGNAL(VERT_DRIVE)
		});
		device->screen = device->crt->display;
		DEVICE_REGISTER_CHIP("CRT", device->crt);
	} else {
		device->screen = display_rgba_create(40 * 8, 25 * 8);
		DEVICE_REGISTER_CHIP("DISPLAY", lite_display_create(device));
	}

	// datassette
	device->datassette = perif_datassette_create(device->simulator, (PerifDatassetteSignals) {
											[PIN_DS1530_SENSE] = SIGNAL(CASS_SWITCH_1),
											[PIN_DS1530_MOTOR] = SIGNAL(CASS_MOTOR_1),
											[PIN_DS1530_DATA_FROM_DS] = SIGNAL(CASS_READ_1),
											[PIN_DS1530_DATA_TO_DS] = SIGNAL(CASS_WRITE)
	});
	DEVICE_REGISTER_CHIP("CASS1", device->datassette);
}

// lite-PET: RAM circuitry
void circuit_lite_create_ram(DevCommodorePet *device) {

	Ram8d16a *ram = ram_8d16a_create(15, device->simulator, (Ram8d16aSignals) {
										[CHIP_RAM8D16A_A0] = SIGNAL(BA0),
										[CHIP_RAM8D16A_A1] = SIGNAL(BA1),
										[CHIP_RAM8D16A_A2] = SIGNAL(BA2),
										[CHIP_RAM8D16A_A3] = SIGNAL(BA3),
										[CHIP_RAM8D16A_A4] = SIGNAL(BA4),
										[CHIP_RAM8D16A_A5] = SIGNAL(BA5),
										[CHIP_RAM8D16A_A6] = SIGNAL(BA6),
										[CHIP_RAM8D16A_A7] = SIGNAL(BA7),
										[CHIP_RAM8D16A_A8] = SIGNAL(BA8),
										[CHIP_RAM8D16A_A9] = SIGNAL(BA9),
										[CHIP_RAM8D16A_A10] = SIGNAL(BA10),
										[CHIP_RAM8D16A_A11] = SIGNAL(BA11),
										[CHIP_RAM8D16A_A12] = SIGNAL(BA12),
										[CHIP_RAM8D16A_A13] = SIGNAL(BA13),
										[CHIP_RAM8D16A_A14] = SIGNAL(BA14),
										[CHIP_RAM8D16A_A15] = SIGNAL(BA15),

										[CHIP_RAM8D16A_D0] = SIGNAL(BD0),
										[CHIP_RAM8D16A_D1] = SIGNAL(BD1),
										[CHIP_RAM8D16A_D2] = SIGNAL(BD2),
										[CHIP_RAM8D16A_D3] = SIGNAL(BD3),
										[CHIP_RAM8D16A_D4] = SIGNAL(BD4),
										[CHIP_RAM8D16A_D5] = SIGNAL(BD5),
										[CHIP_RAM8D16A_D6] = SIGNAL(BD6),
										[CHIP_RAM8D16A_D7] = SIGNAL(BD7),

										[CHIP_RAM8D16A_CE_B] = SIGNAL(BA15),
										[CHIP_RAM8D16A_OE_B] = SIGNAL(G7_8),
										[CHIP_RAM8D16A_WE_B] = SIGNAL(RAMRW)
	});
	DEVICE_REGISTER_CHIP("RAM", ram);

	// glue-logic
	DEVICE_REGISTER_CHIP("LOGIC5", glue_logic_create(device, 5));
}

// lite-PET: master timing
void circuit_lite_create_timing(DevCommodorePet *device) {
	device->oscillator_y1 = oscillator_create(1000000, device->simulator, (OscillatorSignals) {
										[CHIP_OSCILLATOR_CLK_OUT] = SIGNAL(CLK1)
	});
	DEVICE_REGISTER_CHIP("OSC", device->oscillator_y1);
}

// lite-PET: display ram
void circuit_lite_create_vram(DevCommodorePet *device) {
	assert(device);
	DEVICE_REGISTER_CHIP("VRAM", ram_8d16a_create(10, device->simulator, (Ram8d16aSignals) {
										[CHIP_RAM8D16A_A0] = SIGNAL(BA0),
										[CHIP_RAM8D16A_A1] = SIGNAL(BA1),
										[CHIP_RAM8D16A_A2] = SIGNAL(BA2),
										[CHIP_RAM8D16A_A3] = SIGNAL(BA3),
										[CHIP_RAM8D16A_A4] = SIGNAL(BA4),
										[CHIP_RAM8D16A_A5] = SIGNAL(BA5),
										[CHIP_RAM8D16A_A6] = SIGNAL(BA6),
										[CHIP_RAM8D16A_A7] = SIGNAL(BA7),
										[CHIP_RAM8D16A_A8] = SIGNAL(BA8),
										[CHIP_RAM8D16A_A9] = SIGNAL(BA9),

										[CHIP_RAM8D16A_D0] = SIGNAL(BD0),
										[CHIP_RAM8D16A_D1] = SIGNAL(BD1),
										[CHIP_RAM8D16A_D2] = SIGNAL(BD2),
										[CHIP_RAM8D16A_D3] = SIGNAL(BD3),
										[CHIP_RAM8D16A_D4] = SIGNAL(BD4),
										[CHIP_RAM8D16A_D5] = SIGNAL(BD5),
										[CHIP_RAM8D16A_D6] = SIGNAL(BD6),
										[CHIP_RAM8D16A_D7] = SIGNAL(BD7),

										[CHIP_RAM8D16A_CE_B] = SIGNAL(SEL8_B),
										[CHIP_RAM8D16A_OE_B] = SIGNAL(TV_READ_B),
										[CHIP_RAM8D16A_WE_B] = SIGNAL(RAMRW)
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
size_t dev_commodore_pet_get_irq_signals(DevCommodorePet *device, SignalBreakpoint **irq_signals);

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

	device->signal_pool = device->simulator->signal_pool;

	//
	// signals - general
	//

	SIGNAL_DEFINE_DEFAULT_N(RESET_BTN_B, ACTLO_DEASSERT, "RESBTN");
	SIGNAL_DEFINE_DEFAULT_N(HIGH, true, "VCC");
	SIGNAL_DEFINE_DEFAULT_N(LOW, false, "GND");

	//
	// signals - sheet 1: microprocessor / memory expansion
	//

	SIGNAL_DEFINE_DEFAULT_N(RESET_B, ACTLO_ASSERT, "/RES");
	SIGNAL_DEFINE_N(RESET, "RES");
	SIGNAL_DEFINE_DEFAULT_N(IRQ_B, ACTLO_DEASSERT, "/IRQ");
	SIGNAL_DEFINE_DEFAULT_N(NMI_B, ACTLO_DEASSERT, "/NMI");

	SIGNAL_GROUP_NEW_N(cpu_address, 16, "cpu_address", "AB%d");
	memcpy(&SIGNAL(AB0), SIGNAL_GROUP(cpu_address), 16 * sizeof(Signal));

	SIGNAL_GROUP_NEW_N(cpu_data, 8, "cpu_data", "D%d");
	memcpy(&SIGNAL(D0), SIGNAL_GROUP(cpu_data), 8 * sizeof(Signal));

	SIGNAL_DEFINE_DEFAULT_N(RW, true, "RW");
	SIGNAL_DEFINE_N(SYNC, "SYNC");
	SIGNAL_DEFINE_DEFAULT_N(RDY, ACTHI_ASSERT, "RDY");

	SIGNAL_GROUP_NEW_N(buf_address, 16, "buffered_address", "BA%d");
	memcpy(&SIGNAL(BA0), SIGNAL_GROUP(buf_address), 16 * sizeof(Signal));

	SIGNAL_GROUP_NEW_N(buf_data, 8, "buffered_data", "BD%d");
	memcpy(&SIGNAL(BD0), SIGNAL_GROUP(buf_data), 8 * sizeof(Signal));

	SIGNAL_GROUP_NEW_N(mem_sel, 16, "mem_sel", "/SEL%X");
	memcpy(&SIGNAL(SEL0_B), SIGNAL_GROUP(mem_sel), 16 * sizeof(Signal));

	SIGNAL_DEFINE_N(SEL8, "SEL8");

	SIGNAL_DEFINE_N(X8XX, "X8XX");
	SIGNAL_DEFINE_N(88XX_B, "/88XX");
	SIGNAL_DEFINE_N(ROMA_B, "/ROMA");

	SIGNAL_DEFINE_N(RAMR_B, "/RAMR");
	SIGNAL_DEFINE_N(RAMW_B, "/RAMW");

	SIGNAL_DEFINE_N(PHI2, "PHI2");
	SIGNAL_DEFINE_N(BPHI2, "BPHI2");
	SIGNAL_DEFINE_N(CPHI2, "CPHI2");

	SIGNAL_DEFINE_N(BRW, "BRW");
	SIGNAL_DEFINE_N(BRW_B, "/BRW");
	SIGNAL_DEFINE_N(RAMRW, "RAMRW");

	SIGNAL_DEFINE_N(A5_12, "A512");

	SIGNAL_DEFINE_N(BA11_B, "/BA11");

	//
	// signals - sheet 2: IEEE-488 interface
	//

	SIGNAL_DEFINE_N(ATN_IN_B, "/ATNIN");
	SIGNAL_DEFINE_N(NDAC_OUT_B, "/NDACOUT");
	SIGNAL_DEFINE_N(IFC_B, "/IFC");
	SIGNAL_DEFINE_N(SRQ_IN_B, "/SRQIN");
	SIGNAL_DEFINE_N(DAV_OUT_B, "/DAVOUT");

	SIGNAL_GROUP_NEW(ieee488_di, 8);
	signal_group_set_name(SIGNAL_POOL, SIGNAL_GROUP(ieee488_di), "ieee488_di", "DI%d", 1);
	memcpy(&SIGNAL(DI0), SIGNAL_GROUP(ieee488_di), 8 * sizeof(Signal));

	SIGNAL_GROUP_NEW(ieee488_do, 8);
	signal_group_set_name(SIGNAL_POOL, SIGNAL_GROUP(ieee488_do), "ieee488_do", "DO%d", 1);
	memcpy(&SIGNAL(DI0), SIGNAL_GROUP(ieee488_do), 8 * sizeof(Signal));

	SIGNAL_DEFINE_N(CS1, "CS1");

	//
	// signals - sheet 3: Cassette & Keyboard
	//

	SIGNAL_DEFINE_N(CA1, "CA1");
	SIGNAL_DEFINE_N(GRAPHIC, "GRAPHIC");
	SIGNAL_DEFINE_N(CASS_MOTOR_2, "CASSMOTOR2");

	SIGNAL_DEFINE_N(NDAC_IN_B, "/NDACIN");
	SIGNAL_DEFINE_N(NRFD_OUT_B, "/NRFDOUT");
	SIGNAL_DEFINE_N(ATN_OUT_B, "/ATNOUT");
	SIGNAL_DEFINE_N(CASS_WRITE, "CASSWRITE");
	SIGNAL_DEFINE_N(CASS_MOTOR_2_B, "/CASSMOTOR2");
	SIGNAL_DEFINE_N(NRFD_IN_B, "/NRFDIN");
	SIGNAL_DEFINE_N(DAV_IN_B, "/DAVIN");

	SIGNAL_DEFINE_N(KEYA, "KEYA");
	SIGNAL_DEFINE_N(KEYB, "KEYB");
	SIGNAL_DEFINE_N(KEYC, "KEYC");
	SIGNAL_DEFINE_N(KEYD, "KEYD");
	SIGNAL_DEFINE_N(CASS_SWITCH_1, "CASSSWITCH1");
	SIGNAL_DEFINE_N(CASS_SWITCH_2, "CASSSWITCH2");
	SIGNAL_DEFINE_N(EOI_IN_B, "/EOIIN");
	SIGNAL_DEFINE_DEFAULT_N(DIAG, true, "DIAG");

	SIGNAL_DEFINE_N(CB2, "CB2");
	SIGNAL_DEFINE_N(EOI_OUT_B, "/EOIOUT");
	SIGNAL_DEFINE_N(CASS_READ_1, "CASSREAD1");
	SIGNAL_DEFINE_N(CASS_READ_2, "CASSREAD2");
	SIGNAL_DEFINE_N(CASS_MOTOR_1, "CASSMOTOR1");
	SIGNAL_DEFINE_N(CASS_MOTOR_1_B, "/CASSMOTOR1");

	SIGNAL_GROUP_NEW_N(pa, 8, "c5_pa", "PA%d");
	memcpy(&SIGNAL(PA0), SIGNAL_GROUP(pa), 8 * sizeof(Signal));

	SIGNAL_GROUP_NEW_N(keyboard_in, 8, "keyboard_in", "KIN%d");
	SIGNAL_GROUP_DEFAULTS(keyboard_in, 0xff);							// pull-up resistors R18-R25
	memcpy(&SIGNAL(KIN0), SIGNAL_GROUP(keyboard_in), 8 * sizeof(Signal));

	SIGNAL_GROUP_NEW_N(keyboard_out, 10, "keyboard_out", "KOUT%d");
	memcpy(&SIGNAL(KOUT0), SIGNAL_GROUP(keyboard_out), 10 * sizeof(Signal));

	//
	// signals - sheet 5: RAMS
	//

	SIGNAL_DEFINE_N(BANKSEL, "BANKSEL");
	SIGNAL_DEFINE_N(G7_8, "G78");

	SIGNAL_GROUP_NEW_N(ram_address, 7, "ram_address", "FA%d");
	memcpy(&SIGNAL(FA0), SIGNAL_GROUP(ram_address), 7 * sizeof(Signal));

	SIGNAL_GROUP_NEW_N(ram_data, 8, "ram_data", "RD%d");
	memcpy(&SIGNAL(RD0), SIGNAL_GROUP(ram_data), 8 * sizeof(Signal));

	//
	// signals - sheet 6: Master timing
	//

	SIGNAL_DEFINE_DEFAULT_N(INIT_B, ACTLO_ASSERT, "/INIT");
	SIGNAL_DEFINE_DEFAULT_N(INIT, ACTHI_ASSERT, "INIT");

	SIGNAL_DEFINE_DEFAULT_N(CLK1, true, "CLK1");

	SIGNAL_DEFINE_N(VIDEO_ON, "VIDEOON");

	if (!lite) {

		SIGNAL_DEFINE_DEFAULT_N(CLK16, true, "CLK16");
		SIGNAL_DEFINE_DEFAULT_N(CLK8, true, "CLK8");
		SIGNAL_DEFINE_DEFAULT_N(CLK4, true, "CLK4");
		SIGNAL_DEFINE_DEFAULT_N(CLK2, true, "CLK2");

		SIGNAL_DEFINE_N(BPHI2A, "BPHI2A");
		SIGNAL_DEFINE_N(BPHI2B, "BPHI2B");
		SIGNAL_DEFINE_N(BPHI2C, "BPHI2C");
		SIGNAL_DEFINE_N(BPHI2D, "BPHI2D");
		SIGNAL_DEFINE_N(BPHI2E, "BPHI2E");
		SIGNAL_DEFINE_N(BPHI2F, "BPHI2F");
		SIGNAL_DEFINE_N(BPHI2G, "BPHI2G");
		SIGNAL_DEFINE_N(BPHI2H, "BPHI2H");

		SIGNAL_DEFINE_N(BPHI2A_B, "/BPHI2A");
		SIGNAL_DEFINE_N(BPHI2B_B, "/BPHI2B");
		SIGNAL_DEFINE_N(BPHI2F_B, "/BPHI2F");
		SIGNAL_DEFINE_N(BPHI2G_B, "/BPHI2G");

		SIGNAL_DEFINE_N(RA1, "RA1");
		SIGNAL_DEFINE_N(RA2, "RA2");
		SIGNAL_DEFINE_N(RA3, "RA3");
		SIGNAL_DEFINE_N(RA4, "RA4");
		SIGNAL_DEFINE_N(RA5, "RA5");
		SIGNAL_DEFINE_N(RA6, "RA6");
		SIGNAL_DEFINE_N(RA7, "RA7");
		SIGNAL_DEFINE_N(RA8, "RA8");
		SIGNAL_DEFINE_N(RA9, "RA9");
		SIGNAL_DEFINE_N(RA1_B, "/RA1");
		SIGNAL_DEFINE_N(RA6_B, "/RA6");

		SIGNAL_DEFINE_N(RA1AND3, "RA1AND3");
		SIGNAL_DEFINE_N(RA4AND6, "RA4AND6");
		SIGNAL_DEFINE_N(RA5AND6_B, "RA5AND/6");

		SIGNAL_DEFINE_N(LOAD_SR, "LOADSR");
		SIGNAL_DEFINE_N(LOAD_SR_B, "/LOADSR");

		SIGNAL_DEFINE_N(HORZ_DISP_ON, "HORZDISPON");
		SIGNAL_DEFINE_N(HORZ_DISP_OFF, "HORZDISPOFF");
		SIGNAL_DEFINE_N(HORZ_DRIVE, "HORZDRIVE");
		SIGNAL_DEFINE_N(HORZ_DRIVE_B, "/HORZDRIVE");

		SIGNAL_DEFINE_N(H8Q, "H8Q");
		SIGNAL_DEFINE_N(H8Q_B, "/H8Q");
		SIGNAL_DEFINE_N(H8Q2, "H8Q2");
		SIGNAL_DEFINE_N(H8Q2_B, "/H8Q2");

		SIGNAL_DEFINE_N(VIDEO_LATCH, "VIDEOLATCH");
		SIGNAL_DEFINE_N(VERT_DRIVE, "VERTDRIVE");

		SIGNAL_DEFINE_N(H53, "H53");
		SIGNAL_DEFINE_N(H4Y1, "H4Y1");
		SIGNAL_DEFINE_N(MUXA, "MUXA");
		SIGNAL_DEFINE_N(H4Y4, "H4Y4");

		SIGNAL_DEFINE_N(H1Q1, "H1Q1");
		SIGNAL_DEFINE_N(H1Q1_B, "/H1Q1");
		SIGNAL_DEFINE_N(H1Q2, "H1Q2");
		SIGNAL_DEFINE_N(H1Q2_B, "/H1Q2");

		SIGNAL_DEFINE_N(RAS0_B, "/RAS0");
		SIGNAL_DEFINE_N(CAS0_B, "/CAS0");
		SIGNAL_DEFINE_N(CAS1_B, "/CAS1");
		SIGNAL_DEFINE_N(BA14_B, "/BA14");
	}

	//
	// signals - sheet 7: display logic
	//

	SIGNAL_DEFINE_N(TV_SEL, "TVSEL");
	SIGNAL_DEFINE_N(TV_READ_B, "/TVREAD");

	if (!lite) {
		SIGNAL_DEFINE_N(G6_Q, "G6Q");
		SIGNAL_DEFINE_N(G6_Q_B, "/G6Q");

		SIGNAL_DEFINE_N(TV_RAM_RW, "TVRAMRW");
		SIGNAL_DEFINE_N(F6_Y3, "F6Y3");

		SIGNAL_GROUP_NEW_N(vram_address, 10, "vram_address", "SA%d");
		memcpy(&SIGNAL(SA0), SIGNAL_GROUP(vram_address), 10 * sizeof(Signal));

		SIGNAL_DEFINE_N(GA2, "GA2");
		SIGNAL_DEFINE_N(GA3, "GA3");
		SIGNAL_DEFINE_N(GA4, "GA4");
		SIGNAL_DEFINE_N(GA5, "GA5");
		SIGNAL_DEFINE_N(GA6, "GA6");
		SIGNAL_DEFINE_N(GA7, "GA7");
		SIGNAL_DEFINE_N(GA8, "GA8");
		SIGNAL_DEFINE_N(GA9, "GA9");

		SIGNAL_DEFINE_N(LGA2, "LGA2");
		SIGNAL_DEFINE_N(LGA3, "LGA3");
		SIGNAL_DEFINE_N(LGA4, "LGA4");
		SIGNAL_DEFINE_N(LGA5, "LGA5");
		SIGNAL_DEFINE_N(LGA6, "LGA6");
		SIGNAL_DEFINE_N(LGA7, "LGA7");
		SIGNAL_DEFINE_N(LGA8, "LGA8");
		SIGNAL_DEFINE_N(LGA9, "LGA9");

		SIGNAL_DEFINE_DEFAULT_N(NEXT, true, "NEXT");
		SIGNAL_DEFINE_DEFAULT_N(NEXT_B, false, "/NEXT");

		SIGNAL_DEFINE_N(RELOAD_NEXT, "RELOADNEXT");

		SIGNAL_DEFINE_DEFAULT_N(PULLUP_2, true, "PULLUP2");

		SIGNAL_DEFINE_N(LINES_20_B, "/20LINES");
		SIGNAL_DEFINE_N(LINES_200_B, "/200LINES");
		SIGNAL_DEFINE_N(LINE_220, "220LINE");
		SIGNAL_DEFINE_N(LGA_HI_B, "/LGAHI");
		SIGNAL_DEFINE_N(LGA_HI, "LGAHI");
		SIGNAL_DEFINE_N(W220_OFF, "220OFF");

		SIGNAL_DEFINE_N(VIDEO_ON_B, "/VIDEOON");
	}

	//
	// signals - sheet 8: display rams
	//

	if (!lite) {
		SIGNAL_DEFINE_N(RELOAD_B, "/RELOAD");
		SIGNAL_DEFINE_DEFAULT_N(PULLUP_1, true, "PULLUP1");

		SIGNAL_GROUP_NEW_N(vram_data, 8, "vram_data", "SD%d");
		memcpy(&SIGNAL(SD0), SIGNAL_GROUP(vram_data), 8 * sizeof(Signal));

		SIGNAL_GROUP_NEW_N(latched_vram_data, 8, "latched_vram_data", "LSD%d");
		memcpy(&SIGNAL(LSD0), SIGNAL_GROUP(latched_vram_data), 8 * sizeof(Signal));

		SIGNAL_GROUP_NEW_N(char_data, 8, "char_data", "CD%d");
		memcpy(&SIGNAL(CD0), SIGNAL_GROUP(char_data), 8 * sizeof(Signal));

		SIGNAL_DEFINE_N(G9Q, "G9Q");
		SIGNAL_DEFINE_N(G9Q_B, "/G9Q");
		SIGNAL_DEFINE_N(E11QH, "E11QH");
		SIGNAL_DEFINE_N(E11QH_B, "/E11QH");
		SIGNAL_DEFINE_N(G106, "G106");
		SIGNAL_DEFINE_N(G108, "G108");
		SIGNAL_DEFINE_N(H108, "H108");

		SIGNAL_DEFINE_N(VIDEO, "VIDEO");
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
	bool prev_clk1 = SIGNAL_READ(CLK1);
	do {
		device->process(device);
	} while (prev_clk1 == SIGNAL_READ(CLK1));
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

size_t dev_commodore_pet_get_irq_signals(DevCommodorePet *device, SignalBreakpoint **irq_signals) {
	assert(device);
	assert(irq_signals);

	static SignalBreakpoint pet_irq[1] = {0};

	if (pet_irq[0].signal == 0) {
		pet_irq[0] = (SignalBreakpoint) {SIGNAL(IRQ_B), false, true};
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
