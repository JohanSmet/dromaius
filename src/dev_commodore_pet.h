// dev_commodore_pet.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulates a Commodore PET 2001N (Dynamic PET)
// Partial emulation with some shortcuts at the moment.

/* Address space:
   0x0000 - 0x7fff (32kb) : RAM
   0x8000 - 0x83e7 (1000) : Screen RAM (40x25)
   0xb000 - 0xdfff (3x4k) : Basic ROMs (version 4)
   0xe000 - 0xe7ff (2k)   : Editor ROM
   0xe800 - 0xefff (2k)	  : I/O Area (pia-1, pia-2, via)
   0xf000 - 0xffff (4k)   : Kernal ROM
*/

#ifndef DROMAIUS_DEV_COMMODORE_PET_H
#define DROMAIUS_DEV_COMMODORE_PET_H

#include "device.h"
#include "signal_line.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
enum DevCommodorePetSignalAssignment {

	// general
	SIG_P2001N_RESET_BTN_B = 0,
	SIG_P2001N_HIGH,				// always high ("VCC")
	SIG_P2001N_LOW,					// always low  ("GND")

	// sheet 1
	SIG_P2001N_RESET_B,				// reset line
	SIG_P2001N_RESET,
	SIG_P2001N_IRQ_B,				// interrupt request
	SIG_P2001N_NMI_B,				// non-maskable interrupt

	// -- 16-bit address bus (CPU)
	SIG_P2001N_AB0,
	SIG_P2001N_AB1,
	SIG_P2001N_AB2,
	SIG_P2001N_AB3,
	SIG_P2001N_AB4,
	SIG_P2001N_AB5,
	SIG_P2001N_AB6,
	SIG_P2001N_AB7,
	SIG_P2001N_AB8,
	SIG_P2001N_AB9,
	SIG_P2001N_AB10,
	SIG_P2001N_AB11,
	SIG_P2001N_AB12,
	SIG_P2001N_AB13,
	SIG_P2001N_AB14,
	SIG_P2001N_AB15,

	// -- 8-bit data bus (CPU)
	SIG_P2001N_D0,
	SIG_P2001N_D1,
	SIG_P2001N_D2,
	SIG_P2001N_D3,
	SIG_P2001N_D4,
	SIG_P2001N_D5,
	SIG_P2001N_D6,
	SIG_P2001N_D7,

	SIG_P2001N_RW,
	SIG_P2001N_SYNC,
	SIG_P2001N_RDY,

	// -- 16-bit buffered address bus
	SIG_P2001N_BA0,
	SIG_P2001N_BA1,
	SIG_P2001N_BA2,
	SIG_P2001N_BA3,
	SIG_P2001N_BA4,
	SIG_P2001N_BA5,
	SIG_P2001N_BA6,
	SIG_P2001N_BA7,
	SIG_P2001N_BA8,
	SIG_P2001N_BA9,
	SIG_P2001N_BA10,
	SIG_P2001N_BA11,
	SIG_P2001N_BA12,
	SIG_P2001N_BA13,
	SIG_P2001N_BA14,
	SIG_P2001N_BA15,

	SIG_P2001N_BA11_B,

	// -- 8-bit buffered data bus
	SIG_P2001N_BD0,
	SIG_P2001N_BD1,
	SIG_P2001N_BD2,
	SIG_P2001N_BD3,
	SIG_P2001N_BD4,
	SIG_P2001N_BD5,
	SIG_P2001N_BD6,
	SIG_P2001N_BD7,

	// -- memory area selection
	SIG_P2001N_SEL0_B,				// accessing memory $0xxx
	SIG_P2001N_SEL1_B,				// accessing memory $1xxx
	SIG_P2001N_SEL2_B,				// accessing memory $2xxx
	SIG_P2001N_SEL3_B,				// accessing memory $3xxx
	SIG_P2001N_SEL4_B,				// accessing memory $4xxx
	SIG_P2001N_SEL5_B,				// accessing memory $5xxx
	SIG_P2001N_SEL6_B,				// accessing memory $6xxx
	SIG_P2001N_SEL7_B,				// accessing memory $7xxx
	SIG_P2001N_SEL8_B,				// accessing memory $8xxx
	SIG_P2001N_SEL9_B,				// accessing memory $9xxx
	SIG_P2001N_SELA_B,				// accessing memory $axxx
	SIG_P2001N_SELB_B,				// accessing memory $bxxx
	SIG_P2001N_SELC_B,				// accessing memory $cxxx
	SIG_P2001N_SELD_B,				// accessing memory $dxxx
	SIG_P2001N_SELE_B,				// accessing memory $exxx
	SIG_P2001N_SELF_B,				// accessing memory $fxxx

	SIG_P2001N_SEL8,				// inverse of SEL8_B

	SIG_P2001N_X8XX,				// third nibble of address equals 8
	SIG_P2001N_88XX_B,				// low if upper byte of address == 88
	SIG_P2001N_ROMA_B,				// low if rom is being addressed

	SIG_P2001N_RAMR_B,				// low if cpu reads from RAM
	SIG_P2001N_RAMW_B,				// low if cpu writes to RAM

	SIG_P2001N_PHI2,				// CPU output clock signal phase2
	SIG_P2001N_BPHI2,				// buffered PHI2
	SIG_P2001N_CPHI2,				// double-buffered PHI2 (goes to memory expansion)

	SIG_P2001N_BRW,					// buffered RW
	SIG_P2001N_BRW_B,				// buffered inverse RW
	SIG_P2001N_RAMRW,				// RW that goes to the RAM-modules

	// sheet 2: IEEE-488 interface
	SIG_P2001N_ATN_IN_B,
	SIG_P2001N_NDAC_OUT_B,
	SIG_P2001N_IFC_B,
	SIG_P2001N_SRQ_IN_B,
	SIG_P2001N_DAV_OUT_B,

	SIG_P2001N_DI0,
	SIG_P2001N_DI1,
	SIG_P2001N_DI2,
	SIG_P2001N_DI3,
	SIG_P2001N_DI4,
	SIG_P2001N_DI5,
	SIG_P2001N_DI6,
	SIG_P2001N_DI7,

	SIG_P2001N_DO0,
	SIG_P2001N_DO1,
	SIG_P2001N_DO2,
	SIG_P2001N_DO3,
	SIG_P2001N_DO4,
	SIG_P2001N_DO5,
	SIG_P2001N_DO6,
	SIG_P2001N_DO7,

	SIG_P2001N_CS1,

	// sheet 3: Cassette & Keyboard
	SIG_P2001N_CA1,
	SIG_P2001N_CB2,
	SIG_P2001N_GRAPHIC,
	SIG_P2001N_EOI_OUT_B,

	SIG_P2001N_NDAC_IN_B,
	SIG_P2001N_NRFD_OUT_B,
	SIG_P2001N_ATN_OUT_B,
	SIG_P2001N_NRFD_IN_B,
	SIG_P2001N_DAV_IN_B,

	SIG_P2001N_KEYA,
	SIG_P2001N_KEYB,
	SIG_P2001N_KEYC,
	SIG_P2001N_KEYD,
	SIG_P2001N_EOI_IN_B,
	SIG_P2001N_DIAG,

	SIG_P2001N_CASS_MOTOR_1,
	SIG_P2001N_CASS_MOTOR_1_B,
	SIG_P2001N_CASS_MOTOR_2,
	SIG_P2001N_CASS_MOTOR_2_B,
	SIG_P2001N_CASS_READ_1,
	SIG_P2001N_CASS_READ_2,
	SIG_P2001N_CASS_SWITCH_1,
	SIG_P2001N_CASS_SWITCH_2,
	SIG_P2001N_CASS_WRITE,

	SIG_P2001N_PA0,
	SIG_P2001N_PA1,
	SIG_P2001N_PA2,
	SIG_P2001N_PA3,
	SIG_P2001N_PA4,
	SIG_P2001N_PA5,
	SIG_P2001N_PA6,
	SIG_P2001N_PA7,

	SIG_P2001N_KIN0,
	SIG_P2001N_KIN1,
	SIG_P2001N_KIN2,
	SIG_P2001N_KIN3,
	SIG_P2001N_KIN4,
	SIG_P2001N_KIN5,
	SIG_P2001N_KIN6,
	SIG_P2001N_KIN7,

	SIG_P2001N_KOUT0,
	SIG_P2001N_KOUT1,
	SIG_P2001N_KOUT2,
	SIG_P2001N_KOUT3,
	SIG_P2001N_KOUT4,
	SIG_P2001N_KOUT5,
	SIG_P2001N_KOUT6,
	SIG_P2001N_KOUT7,
	SIG_P2001N_KOUT8,
	SIG_P2001N_KOUT9,

	// sheet 4: ROMS (no unique signals)

	// sheet 5: RAMS
	SIG_P2001N_BANKSEL,
	SIG_P2001N_G7_8,

	SIG_P2001N_FA0,
	SIG_P2001N_FA1,
	SIG_P2001N_FA2,
	SIG_P2001N_FA3,
	SIG_P2001N_FA4,
	SIG_P2001N_FA5,
	SIG_P2001N_FA6,

	SIG_P2001N_RD0,
	SIG_P2001N_RD1,
	SIG_P2001N_RD2,
	SIG_P2001N_RD3,
	SIG_P2001N_RD4,
	SIG_P2001N_RD5,
	SIG_P2001N_RD6,
	SIG_P2001N_RD7,

	// sheet 6: master timing
	SIG_P2001N_INIT_B,				// 1-bit - initialization of the master timing section
	SIG_P2001N_INIT,				// 1-bit

	SIG_P2001N_CLK16,				// 1-bit - 16Mhz signal straight from the oscillator
	SIG_P2001N_CLK8,				// 1-bit - 8Mhz clock signal (used for display section)
	SIG_P2001N_CLK4,				// 1-bit - 4Mhz clock signal (visualisation only)
	SIG_P2001N_CLK2,				// 1-bit - 2Mhz clock signal (visualisation only)
	SIG_P2001N_CLK1,				// 1-bit - 1Mhz main clock

	SIG_P2001N_BPHI2A,				// 8 phases of clk1
	SIG_P2001N_BPHI2B,
	SIG_P2001N_BPHI2C,
	SIG_P2001N_BPHI2D,
	SIG_P2001N_BPHI2E,
	SIG_P2001N_BPHI2F,
	SIG_P2001N_BPHI2G,
	SIG_P2001N_BPHI2H,

	SIG_P2001N_BPHI2A_B,
	SIG_P2001N_BPHI2B_B,
	SIG_P2001N_BPHI2F_B,
	SIG_P2001N_BPHI2G_B,

	// -- ram refresh address
	SIG_P2001N_RA1,
	SIG_P2001N_RA2,
	SIG_P2001N_RA3,
	SIG_P2001N_RA4,
	SIG_P2001N_RA5,
	SIG_P2001N_RA6,
	SIG_P2001N_RA7,					// used on sheet 8
	SIG_P2001N_RA8,					// used on sheet 8
	SIG_P2001N_RA9,					// used on sheet 8

	SIG_P2001N_RA1_B,
	SIG_P2001N_RA6_B,

	SIG_P2001N_RA1AND3,
	SIG_P2001N_RA4AND6,
	SIG_P2001N_RA5AND6_B,

	SIG_P2001N_LOAD_SR,
	SIG_P2001N_LOAD_SR_B,

	SIG_P2001N_HORZ_DISP_ON,
	SIG_P2001N_HORZ_DISP_OFF,
	SIG_P2001N_HORZ_DRIVE,
	SIG_P2001N_HORZ_DRIVE_B,

	SIG_P2001N_H8Q,
	SIG_P2001N_H8Q_B,
	SIG_P2001N_H8Q2,
	SIG_P2001N_H8Q2_B,

	SIG_P2001N_VIDEO_LATCH,
	SIG_P2001N_VERT_DRIVE,

	SIG_P2001N_H53,
	SIG_P2001N_H4Y1,
	SIG_P2001N_MUXA,
	SIG_P2001N_H4Y4,

	SIG_P2001N_H1Q1,
	SIG_P2001N_H1Q1_B,
	SIG_P2001N_H1Q2,
	SIG_P2001N_H1Q2_B,

	SIG_P2001N_RAS0_B,
	SIG_P2001N_CAS0_B,
	SIG_P2001N_CAS1_B,
	SIG_P2001N_BA14_B,

	SIG_P2001N_VIDEO_ON,			// 1-bit - vblank

	// sheet 7: display logic
	SIG_P2001N_TV_SEL,
	SIG_P2001N_TV_READ_B,

	SIG_P2001N_G6_Q,
	SIG_P2001N_G6_Q_B,

	SIG_P2001N_TV_RAM_RW,
	SIG_P2001N_F6_Y3,

	// -- display ram address bus
	SIG_P2001N_SA0,
	SIG_P2001N_SA1,
	SIG_P2001N_SA2,
	SIG_P2001N_SA3,
	SIG_P2001N_SA4,
	SIG_P2001N_SA5,
	SIG_P2001N_SA6,
	SIG_P2001N_SA7,
	SIG_P2001N_SA8,
	SIG_P2001N_SA9,

	SIG_P2001N_GA2,
	SIG_P2001N_GA3,
	SIG_P2001N_GA4,
	SIG_P2001N_GA5,
	SIG_P2001N_GA6,
	SIG_P2001N_GA7,
	SIG_P2001N_GA8,
	SIG_P2001N_GA9,

	SIG_P2001N_LGA2,
	SIG_P2001N_LGA3,
	SIG_P2001N_LGA4,
	SIG_P2001N_LGA5,
	SIG_P2001N_LGA6,
	SIG_P2001N_LGA7,
	SIG_P2001N_LGA8,
	SIG_P2001N_LGA9,

	SIG_P2001N_NEXT,
	SIG_P2001N_NEXT_B,

	SIG_P2001N_RELOAD_NEXT,

	SIG_P2001N_PULLUP_2,

	SIG_P2001N_LINES_20_B,
	SIG_P2001N_LINES_200_B,
	SIG_P2001N_LINE_220,
	SIG_P2001N_LGA_HI_B,
	SIG_P2001N_LGA_HI,
	SIG_P2001N_W220_OFF,

	SIG_P2001N_VIDEO_ON_B,

	SIG_P2001N_A5_12,				// pin 12 of chip A5 (output of 3-input NAND)

	// sheet 8: display rams
	SIG_P2001N_RELOAD_B,
	SIG_P2001N_PULLUP_1,

	// -- screen data bus
	SIG_P2001N_SD0,
	SIG_P2001N_SD1,
	SIG_P2001N_SD2,
	SIG_P2001N_SD3,
	SIG_P2001N_SD4,
	SIG_P2001N_SD5,
	SIG_P2001N_SD6,
	SIG_P2001N_SD7,

	// -- latched screen data bus
	SIG_P2001N_LSD0,
	SIG_P2001N_LSD1,
	SIG_P2001N_LSD2,
	SIG_P2001N_LSD3,
	SIG_P2001N_LSD4,
	SIG_P2001N_LSD5,
	SIG_P2001N_LSD6,
	SIG_P2001N_LSD7,

	// -- character-ROM data
	SIG_P2001N_CD0,
	SIG_P2001N_CD1,
	SIG_P2001N_CD2,
	SIG_P2001N_CD3,
	SIG_P2001N_CD4,
	SIG_P2001N_CD5,
	SIG_P2001N_CD6,
	SIG_P2001N_CD7,

	SIG_P2001N_G9Q,
	SIG_P2001N_G9Q_B,
	SIG_P2001N_E11QH,
	SIG_P2001N_E11QH_B,
	SIG_P2001N_G106,
	SIG_P2001N_G108,
	SIG_P2001N_H108,

	SIG_P2001N_VIDEO,

	SIG_P2001N_SIGNAL_COUNT
};

typedef Signal DevCommodorePetSignals[SIG_P2001N_SIGNAL_COUNT];

typedef struct DevCommodorePet {
	DEVICE_DECLARE_FUNCTIONS

	// major components
	struct Cpu6502 *		cpu;
	struct Chip63xxRom *	roms[7];
	struct Oscillator *		oscillator_y1;
	struct Chip6520 *		pia_1;
	struct Chip6520 *		pia_2;
	struct Chip6522 *		via;

	struct InputKeypad *	keypad;
	struct PerifPetCrt *	crt;
	struct DisplayRGBA *	screen;
	struct PerifDatassette *datassette;

	bool					in_reset;

	// signals
	SignalPool *			signal_pool;
	DevCommodorePetSignals	signals;

	SignalGroup				sg_cpu_address;
	SignalGroup				sg_cpu_data;
	SignalGroup				sg_buf_address;
	SignalGroup				sg_buf_data;
	SignalGroup				sg_mem_sel;
	SignalGroup				sg_ieee488_di;
	SignalGroup				sg_ieee488_do;
	SignalGroup				sg_pa;
	SignalGroup				sg_keyboard_in;
	SignalGroup				sg_keyboard_out;
	SignalGroup				sg_ram_address;
	SignalGroup				sg_ram_data;
	SignalGroup				sg_vram_address;
	SignalGroup				sg_vram_data;
	SignalGroup				sg_latched_vram_data;
	SignalGroup				sg_char_data;
} DevCommodorePet;

// functions
DevCommodorePet *dev_commodore_pet_create(void);
DevCommodorePet *dev_commodore_pet_lite_create(void);
void dev_commodore_pet_destroy(DevCommodorePet *device);
void dev_commodore_pet_process(DevCommodorePet *device);
void dev_commodore_pet_process_clk1(DevCommodorePet *device);
void dev_commodore_pet_reset(DevCommodorePet *device);

bool dev_commodore_pet_load_prg(DevCommodorePet* device, const char* filename, bool use_prg_address);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_DEV_COMMODORE_PET_H
