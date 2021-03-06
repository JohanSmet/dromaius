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

typedef struct DevCommodorePetSignals {

	// general
	Signal		reset_btn_b;
	Signal		high;				// 1-bit - always high
	Signal		low;				// 1-bit - always low

	// sheet 1
	Signal		reset;				// 1-bit
	Signal		reset_b;			// 1-bit - reset line
	Signal		irq_b;				// 1-bit - interrupt request
	Signal		nmi_b;				// 1-bit - non-maskable interrupt

	Signal		cpu_bus_address;	// 16-bit address bus
	Signal		cpu_bus_data;		// 8-bit data
	Signal		cpu_rw;				// 1-bit
	Signal		cpu_sync;			// 1-bit
	Signal		cpu_rdy;			// 1-bit

	Signal		bus_ba;				// buffered address bus
	Signal		bus_bd;				// buffered data bus

	Signal		sel0_b;				// 1-bit - accessing memory $0xxx
	Signal		sel1_b;				// 1-bit - accessing memory $1xxx
	Signal		sel2_b;				// 1-bit - accessing memory $2xxx
	Signal		sel3_b;				// 1-bit - accessing memory $3xxx
	Signal		sel4_b;				// 1-bit - accessing memory $4xxx
	Signal		sel5_b;				// 1-bit - accessing memory $5xxx
	Signal		sel6_b;				// 1-bit - accessing memory $6xxx
	Signal		sel7_b;				// 1-bit - accessing memory $7xxx
	Signal		sel8_b;				// 1-bit - accessing memory $8xxx
	Signal		sel9_b;				// 1-bit - accessing memory $9xxx
	Signal		sela_b;				// 1-bit - accessing memory $axxx
	Signal		selb_b;				// 1-bit - accessing memory $bxxx
	Signal		selc_b;				// 1-bit - accessing memory $cxxx
	Signal		seld_b;				// 1-bit - accessing memory $dxxx
	Signal		sele_b;				// 1-bit - accessing memory $exxx
	Signal		self_b;				// 1-bit - accessing memory $fxxx

	Signal		sel8;				// 1-bit - inverse of sel8_b

	Signal		x8xx;				// 1-bit - third nibble of address == 8
	Signal		s_88xx_b;			// 1-bit
	Signal		rom_addr_b;			// 1-bit - low if ROM is being addressed

	Signal		ram_read_b;			// 1-bit - cpu reads from ram
	Signal		ram_write_b;		// 1-bit - cpu writes to ram

	Signal		phi2;				// 1-bit - cpu output clock signal (phase 2)
	Signal		bphi2;				// 1-bit
	Signal		cphi2;				// 1-bit

	Signal		buf_rw;				// 1-bit - buffered RW
	Signal		buf_rw_b;			// 1-bit - buffered inverse RW
	Signal		ram_rw;				// 1-bit

	Signal		a5_12;

	Signal		ba6;				// 1-bit
	Signal		ba7;				// 1-bit
	Signal		ba8;				// 1-bit
	Signal		ba9;				// 1-bit
	Signal		ba10;				// 1-bit
	Signal		ba11;				// 1-bit
	Signal		ba12;				// 1-bit
	Signal		ba13;				// 1-bit
	Signal		ba14;				// 1-bit
	Signal		ba15;				// 1-bit - top bit of the buffered address bus

	Signal		ba11_b;				// 1-bit

	// sheet 2: IEEE-488 interface
	Signal		atn_in_b;
	Signal		ndac_out_b;
	Signal		ifc_b;
	Signal		srq_in_b;
	Signal		dav_out_b;
	Signal		bus_di;
	Signal		bus_do;
	Signal		cs1;				// 1-bit

	// sheet 3: Cassette & Keyboard
	Signal		ca1;				// 1-bit
	Signal		cb2;				// 1-bit
	Signal		graphic;			// 1-bit
	Signal		eoi_out_b;

	Signal		c5_portb;			// 8-bit
	Signal		ndac_in_b;			// 1-bit
	Signal		nrfd_out_b;			// 1-bit
	Signal		atn_out_b;			// 1-bit
	Signal		video_on_2;			// 1-bit copy of video_on
	Signal		nrfd_in_b;			// 1-bit
	Signal		dav_in_b;			// 1-bit

	Signal		c7_porta;			// 8-bit
	Signal		keya;				// 1-bit
	Signal		keyb;				// 1-bit
	Signal		keyc;				// 1-bit
	Signal		keyd;				// 1-bit
	Signal		eoi_in_b;
	Signal		diag;				// 1-bit

	Signal		cass_motor_1;		// 1-bit
	Signal		cass_motor_1_b;		// 1-bit
	Signal		cass_motor_2;		// 1-bit
	Signal		cass_motor_2_b;		// 1-bit
	Signal		cass_read_1;		// 1-bit
	Signal		cass_read_2;		// 1-bit
	Signal		cass_switch_1;		// 1-bit
	Signal		cass_switch_2;		// 1-bit
	Signal		cass_write;			// 1-bit

	Signal		bus_pa;				// 8-bit
	Signal		bus_kin;			// 8-bit
	Signal		bus_kout;			// 10-bit

	// sheet 5: RAMS
	Signal		banksel;
	Signal		g78;

	Signal		bus_fa;				// 7-bit
	Signal		bus_rd;				// 8-bit

	// sheet 6: master timing
	Signal		init_b;				// 1-bit - initialization of the master timing section
	Signal		init;				// 1-bit

	Signal		clk16;				// 1-bit - 16Mhz signal straight from the oscillator
	Signal		clk8;				// 1-bit - 8Mhz clock signal (used for display section)
	Signal		clk4;				// 1-bit - 4Mhz clock signal (visualisation only)
	Signal		clk2;				// 1-bit - 2Mhz clock signal (visualisation only)
	Signal		clk1;				// 1-bit - 1Mhz main clock

	Signal		bphi2a;				// 8 phases of clk1
	Signal		bphi2b;
	Signal		bphi2c;
	Signal		bphi2d;
	Signal		bphi2e;
	Signal		bphi2f;
	Signal		bphi2g;
	Signal		bphi2h;

	Signal		bphi2a_b;
	Signal		bphi2b_b;
	Signal		bphi2f_b;
	Signal		bphi2g_b;

	Signal		ra1;				// 1-bit - ram refresh address
	Signal		ra1_b;
	Signal		ra2;				// 1-bit - ram refresh address
	Signal		ra3;				// 1-bit - ram refresh address
	Signal		ra4;				// 1-bit - ram refresh address
	Signal		ra5;				// 1-bit - ram refresh address
	Signal		ra6;				// 1-bit - ram refresh address
	Signal		ra6_b;

	Signal		ra1and3;
	Signal		ra4and6;
	Signal		ra5and6_b;

	Signal		load_sr;
	Signal		load_sr_b;

	Signal		horz_disp_on;
	Signal		horz_disp_off;
	Signal		horz_drive;
	Signal		horz_drive_b;

	Signal		h8q;
	Signal		h8q_b;
	Signal		h8q2;
	Signal		h8q2_b;

	Signal		video_latch;
	Signal		vert_drive;

	Signal		h53;
	Signal		h4y1;
	Signal		muxa;
	Signal		h4y4;

	Signal		h1q1;
	Signal		h1q1_b;
	Signal		h1q2;
	Signal		h1q2_b;

	Signal		ras0_b;
	Signal		cas0_b;
	Signal		cas1_b;
	Signal		ba14_b;

	Signal		video_on;			// 1-bit - vblank

	// sheet 7: display logic
	Signal		tv_sel;
	Signal		tv_read_b;

	Signal		g6_q;
	Signal		g6_q_b;

	Signal		tv_ram_rw;
	Signal		f6_y3;

	Signal		bus_sa;				// 10-bit - display ram address bus

	Signal		ga2;
	Signal		ga3;
	Signal		ga4;
	Signal		ga5;
	Signal		ga6;
	Signal		ga7;
	Signal		ga8;
	Signal		ga9;

	Signal		lga2;
	Signal		lga3;
	Signal		lga4;
	Signal		lga5;
	Signal		lga6;
	Signal		lga7;
	Signal		lga8;
	Signal		lga9;

	Signal		next;
	Signal		next_b;

	Signal		reload_next;

	Signal		pullup_2;

	Signal		lines_20_b;
	Signal		lines_200_b;
	Signal		line_220;
	Signal		lga_hi_b;
	Signal		lga_hi;
	Signal		w220_off;

	Signal		video_on_b;

	// sheet 8: display rams
	Signal		ra7;				// 1-bit - ram refresh address
	Signal		ra8;				// 1-bit - ram refresh address
	Signal		ra9;				// 1-bit - ram refresh address
	Signal		reload_b;
	Signal		pullup_1;

	Signal		bus_sd;				// 8-bit - display ram databus
	Signal		bus_lsd;			// 8-bit - latched dispay ram databus
	Signal		bus_cd;				// 8-bit

	Signal		g9q;				// 1-bit
	Signal		g9q_b;				// 1-bit
	Signal		e11qh;				// 1-bit
	Signal		e11qh_b;			// 1-bit
	Signal		g106;				// 1-bit
	Signal		g108;				// 1-bit
	Signal		h108;				// 1-bit

	Signal		video;
} DevCommodorePetSignals;

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
	DevCommodorePetSignals	signals;
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
