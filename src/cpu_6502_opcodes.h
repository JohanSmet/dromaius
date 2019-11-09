// cpu_6502_opcodes.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// list all support 6502 opcodes

#ifndef DROMAIUS_CPU_6502_OPCODES_H
#define DROMAIUS_CPU_6502_OPCODES_H

#include "types.h"

/* The 6502 instruction set is laid out according to a pattern "aaabbbcc".
   Typically a and c denote the instruction type and b specifies the addressing mode.
*/

//                      aaabbbcc
#define AC_6502_MASK  0b11100011
#define AC_6502_AND	  0b00100001
#define AC_6502_EOR	  0b01000001
#define AC_6502_LDA   0b10100001
#define AC_6502_LDX   0b10100010
#define AC_6502_LDY   0b10100000
#define AC_6502_ORA	  0b00000001
#define AC_6502_STA	  0b10000001

//                        aaabbbcc
#define ADDR_6502_MASK    0b00011100
#define ADDR_6502_G1_INDX 0b00000000
#define ADDR_6502_G1_ZP   0b00000100
#define ADDR_6502_G1_IMM  0b00001000
#define ADDR_6502_G1_ABS  0b00001100
#define ADDR_6502_G1_INDY 0b00010000
#define ADDR_6502_G1_ZPX  0b00010100
#define ADDR_6502_G1_ABSY 0b00011000
#define ADDR_6502_G1_ABSX 0b00011100


// opcodes
typedef enum OPCODES_6502_ {

	OP_6502_AND_IMM		= 0x29,
	OP_6502_AND_ZP		= 0x25,
	OP_6502_AND_ZPX		= 0x35,
	OP_6502_AND_ABS		= 0x2d,
	OP_6502_AND_ABSX	= 0x3d,
	OP_6502_AND_ABSY	= 0x39,
	OP_6502_AND_INDX	= 0x21,
	OP_6502_AND_INDY	= 0x31,

	OP_6502_BCS			= 0xb0,

	OP_6502_CLC			= 0x18,		// clear carry  flag
	OP_6502_CLD			= 0xd8,		// clear decimal mode
	OP_6502_CLI			= 0x58,		// clear interrupt disable bit
	OP_6502_CLV			= 0xb8,		// clear overflow flag

	OP_6502_EOR_IMM		= 0x49,
	OP_6502_EOR_ZP		= 0x45,
	OP_6502_EOR_ZPX		= 0x55,
	OP_6502_EOR_ABS		= 0x4d,
	OP_6502_EOR_ABSX	= 0x5d,
	OP_6502_EOR_ABSY	= 0x59,
	OP_6502_EOR_INDX	= 0x41,
	OP_6502_EOR_INDY	= 0x51,

	OP_6502_LDA_IMM		= 0xa9,
	OP_6502_LDA_ZP		= 0xa5,
	OP_6502_LDA_ZPX		= 0xb5,
	OP_6502_LDA_ABS		= 0xad,
	OP_6502_LDA_ABSX	= 0xbd,
	OP_6502_LDA_ABSY	= 0xb9,
	OP_6502_LDA_INDX	= 0xa1,
	OP_6502_LDA_INDY	= 0xb1,

	OP_6502_LDX_IMM		= 0xa2,
	OP_6502_LDX_ZP		= 0xa6,
	OP_6502_LDX_ZPY		= 0xb6,
	OP_6502_LDX_ABS		= 0xae,
	OP_6502_LDX_ABSY	= 0xbe,

	OP_6502_LDY_IMM		= 0xa0,
	OP_6502_LDY_ZP		= 0xa4,
	OP_6502_LDY_ZPX		= 0xb4,
	OP_6502_LDY_ABS		= 0xac,
	OP_6502_LDY_ABSX	= 0xbc,

	OP_6502_ORA_IMM		= 0x09,
	OP_6502_ORA_ZP		= 0x05,
	OP_6502_ORA_ZPX		= 0x15,
	OP_6502_ORA_ABS		= 0x0d,
	OP_6502_ORA_ABSX	= 0x1d,
	OP_6502_ORA_ABSY	= 0x19,
	OP_6502_ORA_INDX	= 0x01,
	OP_6502_ORA_INDY	= 0x11,

	OP_6502_SEC			= 0x38,		// set cary flag
	OP_6502_SED			= 0xf8,		// set decimal mode
	OP_6502_SEI			= 0x78,		// set interrupt disable status

	OP_6502_STA_ZP		= 0x85,
	OP_6502_STA_ZPX		= 0x95,
	OP_6502_STA_ABS		= 0x8D,
	OP_6502_STA_ABSX	= 0x9D,
	OP_6502_STA_ABSY	= 0x99,
	OP_6502_STA_INDX	= 0x81,
	OP_6502_STA_INDY	= 0x91,

	OP_6502_STX_ZP		= 0x86,
	OP_6502_STX_ZPY		= 0x96,
	OP_6502_STX_ABS		= 0x8e,

	OP_6502_STY_ZP		= 0x84,
	OP_6502_STY_ZPX		= 0x94,
	OP_6502_STY_ABS		= 0x8c,

	OP_6502_TAX			= 0xaa,
	OP_6502_TAY			= 0xa8,
	OP_6502_TSX			= 0xba,
} OPCODES_6502;


#endif // DROMAIUS_CPU_6502_OPCODES_H
