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
#include "cpu_6502.h"
#include "ram_8d_16a.h"
#include "rom_8d_16a.h"
#include "chip_74xxx.h"
#include "clock.h"

#ifdef __cplusplus
extern "C" {
#endif

// types

typedef enum DevCommodorePetDomains {
	PET_DOMAIN_CLOCK = 0,
	PET_DOMAIN_1MHZ = 1
} DevCommodorePetDomains;

typedef struct DevCommodorePetSignals {

	Signal		reset_b;			// 1-bit - reset line
	Signal		irq_b;				// 1-bit - interrupt request
	Signal		nmi_b;				// 1-bit - non-maskable interrupt
	Signal		clk1;				// 1-bit - main clock

	Signal		cpu_bus_address;	// 16-bit address bus
	Signal		cpu_bus_data;		// 8-bit data
	Signal		cpu_rw;				// 1-bit
	Signal		cpu_sync;			// 1-bit
	Signal		cpu_rdy;			// 1-bit

	Signal		bus_ba;				// buffered address bus
	Signal		bus_bd;				// buffered data bus

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

	Signal		high;				// 1-bit - always high
	Signal		low;				// 1-bit - always low

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
	Signal		reset;				// 1-bit

	Signal		ram_oe_b;			// 1-bit
	Signal		ram_we_b;			// 1-bit

	Signal		vram_oe_b;			// 1-bit

	Signal		rome_ce_b;			// 1-bit

	Signal		cs1;				// 1-bit

	Signal		video_on;			// 1-bit - vblank

} DevCommodorePetSignals;

typedef struct DevCommodorePet {
	DEVICE_DECLARE_FUNCTIONS

	// major components
	Cpu6502 *		cpu;
	Ram8d16a *		ram;
	Ram8d16a *      vram;
	Rom8d16a *		roms[7];
	Rom8d16a *		char_rom;
	Clock *			clock;
	struct Chip6520 *		pia_1;
	struct Chip6520 *		pia_2;
	struct Chip6522 *		via;

	struct InputKeypad *	keypad;
	struct DisplayRGBA *	display;

	bool					in_reset;
	int						vblank_counter;

	// glue logic
	Chip74244OctalBuffer *	e9;			// data-bus buffer 1
	Chip74244OctalBuffer *	e10;		// data-bus buffer 2
	Chip74244OctalBuffer *	c3;			// address line buffer 1
	Chip74244OctalBuffer *	b3;			// address line buffer 2
	Chip74154Decoder *		d2;			// selection of memory areas
	Chip74145BcdDecoder *	c9;			// keyboard row decoding

	// signals
	DevCommodorePetSignals	signals;
} DevCommodorePet;

// functions
DevCommodorePet *dev_commodore_pet_create();
void dev_commodore_pet_destroy(DevCommodorePet *device);
void dev_commodore_pet_process(DevCommodorePet *device);
void dev_commodore_pet_reset(DevCommodorePet *device);

bool dev_commodore_pet_load_prg(DevCommodorePet* device, const char* filename, bool use_prg_address);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_DEV_COMMODORE_PET_H
