// filter_6502_asm.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Dissambler for 6502 assembly

#include "filt_6502_asm.h"

#include "stb/stb_ds.h"
#include "cpu_6502_opcodes.h"
#include "utils.h"

#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
//
// lookup tables
//

// opcode => string
static const char OPCODE_NAMES[256][4] = {
//	 00     01     02     03     04     05     06     07     08     09     0A     0B     0C     0D     0E     0F
	"brk", "ora", "???", "???", "???", "ora", "asl", "???", "php", "ora", "asl", "???", "???", "ora", "asl", "???",
	"bpl", "ora", "???", "???", "???", "ora", "asl", "???", "clc", "ora", "???", "???", "???", "ora", "asl", "???",
	"jsr", "and", "???", "???", "bit", "and", "rol", "???", "plp", "and", "rol", "???", "bit", "and", "rol", "???",
	"bmi", "and", "???", "???", "???", "and", "rol", "???", "sec", "and", "???", "???", "???", "and", "rol", "???",
	"rti", "eor", "???", "???", "???", "eor", "lsr", "???", "pha", "eor", "lsr", "???", "jmp", "eor", "lsr", "???",
	"bvc", "eor", "???", "???", "???", "eor", "lsr", "???", "cli", "eor", "???", "???", "???", "eor", "lsr", "???",
	"rts", "adc", "???", "???", "???", "adc", "ror", "???", "pla", "adc", "ror", "???", "jmp", "adc", "ror", "???",
	"bvs", "adc", "???", "???", "???", "adc", "ror", "???", "sei", "adc", "???", "???", "???", "adc", "ror", "???",
	"???", "sta", "???", "???", "sty", "sta", "stx", "???", "dey", "???", "txa", "???", "sty", "sta", "stx", "???",
	"bcc", "sta", "???", "???", "sty", "sta", "stx", "???", "tya", "sta", "txs", "???", "???", "sta", "???", "???",
	"ldy", "lda", "ldx", "???", "ldy", "lda", "ldx", "???", "tay", "lda", "tax", "???", "ldy", "lda", "ldx", "???",
	"bcs", "lda", "???", "???", "ldy", "lda", "ldx", "???", "clv", "lda", "tsx", "???", "ldy", "lda", "ldx", "???",
	"cpy", "cmp", "???", "???", "cpy", "cmp", "dec", "???", "iny", "cmp", "dex", "???", "cpy", "cmp", "dec", "???",
	"bne", "cmp", "???", "???", "???", "cmp", "dec", "???", "cld", "cmp", "???", "???", "???", "cmp", "dec", "???",
	"cpx", "sbc", "???", "???", "cpx", "sbc", "inc", "???", "inx", "sbc", "nop", "???", "cpx", "sbc", "inc", "???",
	"beq", "sbc", "???", "???", "???", "sbc", "inc", "???", "sed", "sbc", "???", "???", "???", "sbc", "inc", "???"
};

// addressing modes
typedef enum ADDR_MODE {
	NONE = 0,
	ACC,
	IMPL,
	IMM,
	REL,
	ZP,
	ZPX,
	ZPY,
	ABS,
	ABSX,
	ABSY,
	IND,
	XIND,
	INDY
} ADDR_MODE;

// opcode => addressing mode
static const ADDR_MODE OPCODE_ADDRESS_MODES[256] = {
	IMPL,	XIND,	NONE,	NONE,	NONE,	ZP,		ZP,		NONE,	IMPL,	IMM,	ACC,	NONE,	NONE,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	NONE,	ZPX,	ZPX,	NONE,	IMPL,	ABSY,	NONE,	NONE,	NONE,	ABSX,	ABSX,	NONE,
	ABS,	XIND,	NONE,	NONE,	ZP,		ZP,		ZP,		NONE,	IMPL,	IMM,	ACC,	NONE,	ABS,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	NONE,	ZPX,	ZPX,	NONE,	IMPL,	ABSY,	NONE,	NONE,	NONE,	ABSX,	ABSX,	NONE,
	IMPL,	XIND,	NONE,	NONE,	NONE,	ZP,		ZP,		NONE,	IMPL,	IMM,	ACC,	NONE,	ABS,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	NONE,	ZPX,	ZPX,	NONE,	IMPL,	ABSY,	NONE,	NONE,	NONE,	ABSX,	ABSX,	NONE,
	IMPL,	XIND,	NONE,	NONE,	NONE,	ZP,		ZP,		NONE,	IMPL,	IMM,	ACC,	NONE,	IND,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	NONE,	ZPX,	ZPX,	NONE,	IMPL,	ABSY,	NONE,	NONE,	NONE,	ABSX,	ABSX,	NONE,
	NONE,	XIND,	NONE,	NONE,	ZP,		ZP,		ZP,		NONE,	IMPL,	NONE,	IMPL,	NONE,	ABS,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	ZPX,	ZPX,	ZPY,	NONE,	IMPL,	ABSY,	IMPL,	NONE,	NONE,	ABSX,	NONE,	NONE,
	IMM,	XIND,	IMM,	NONE,	ZP,		ZP,		ZP,		NONE,	IMPL,	IMM,	IMPL,	NONE,	ABS,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	ZPX,	ZPX,	ZPY,	NONE,	IMPL,	ABSY,	IMPL,	NONE,	ABSX,	ABSX,	ABSY,	NONE,
	IMM,	XIND,	NONE,	NONE,	ZP,		ZP,		ZP,		NONE,	IMPL,	IMM,	IMPL,	NONE,	ABS,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	NONE,	ZPX,	ZPX,	NONE,	IMPL,	ABSY,	NONE,	NONE,	NONE,	ABSX,	ABSX,	NONE,
	IMM,	XIND,	NONE,	NONE,	ZP,		ZP,		ZP,		NONE,	IMPL,	IMM,	IMPL,	NONE,	ABS,	ABS,	ABS,	NONE,
	REL,	INDY,	NONE,	NONE,	NONE,	ZPX,	ZPX,	NONE,	IMPL,	ABSY,	NONE,	NONE,	NONE,	ABSX,	ABSX,	NONE
};

// addressing mode => number of arguments
static const unsigned int ADDRESS_MODE_LEN[14] = {
	0,	// NONE
	0,	// ACC
	0,	// IMPL
	1,	// IMM
	1,	// REL
	1,	// ZP
	1,	// ZPX
	1,	// ZPY
	2,	// ABS
	2,	// ABSX
	2,	// ABSY
	2,	// IND
	1,	// XIND
	1	// INDY
};

///////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

size_t filt_6502_asm_line(const uint8_t *binary, size_t bin_size, size_t bin_index, size_t bin_offset, char **line) {
	assert(binary);
	assert(bin_index < bin_size);

	uint8_t op = binary[bin_index];

	// address + opcode
	arr_printf(*line, "%.4x: %s", bin_index + bin_offset, OPCODE_NAMES[op]);

	// argument
	unsigned arg_size = ADDRESS_MODE_LEN[OPCODE_ADDRESS_MODES[op]];

	if (bin_index + arg_size >= bin_size) {
		arr_printf(*line, " (opcode incomplete)");
		return 1 + arg_size;
	}

	switch (OPCODE_ADDRESS_MODES[op]) {
		case IMM:	
			arr_printf(*line, " #$%.2x", binary[bin_index+1]);
			break;
		case REL:
		case ZP:
			arr_printf(*line, " $%.2x", binary[bin_index+1]);
			break;
		case ZPX:
			arr_printf(*line, " $%.2x,x", binary[bin_index+1]);
			break;
		case ZPY:
			arr_printf(*line, " $%.2x,y", binary[bin_index+1]);
			break;
		case ABS:
			arr_printf(*line, " $%.2x%.2x", binary[bin_index+2], binary[bin_index+1]);
			break;
		case ABSX:
			arr_printf(*line, " $%.2x%.2x,x", binary[bin_index+2], binary[bin_index+1]);
			break;
		case ABSY:
			arr_printf(*line, " $%.2x%.2x,y", binary[bin_index+2], binary[bin_index+1]);
			break;
		case IND:
			arr_printf(*line, " ($%.2x%.2x)", binary[bin_index+2], binary[bin_index+1]);
			break;
		case XIND:
			arr_printf(*line, " ($%.2x,x)", binary[bin_index+1]);
			break;
		case INDY:
			arr_printf(*line, " ($%.2x),y", binary[bin_index+1]);
			break;
		default:
			break;
	}

	return 1 + arg_size;
}

size_t filt_6502_asm_count_instruction(const uint8_t *binary, size_t bin_size, size_t from, size_t until) {
	assert(binary);
	assert(from < bin_size);
	assert(until <= bin_size);

	if (until > bin_size) {
		until = bin_size;
	}

	size_t count = 0;

	for (size_t i = from; i < until; i += 1) {
		unsigned arg_size = ADDRESS_MODE_LEN[OPCODE_ADDRESS_MODES[binary[i]]];
		i += arg_size;
		++count;
	}

	return count;
}
