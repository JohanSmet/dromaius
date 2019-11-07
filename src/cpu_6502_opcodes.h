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
#define AC_6502_LDA   0b10100001
#define AC_6502_LDX   0b10100010
#define AC_6502_LDY   0b10100000

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
	OP_6502_LDA_IMM		= 0xa9,
	OP_6502_LDA_ZP		= 0xa5,
	OP_6502_LDA_ZPX		= 0xb5,
	OP_6502_LDA_ABS		= 0xad,
	OP_6502_LDA_ABSX	= 0xbd,
	OP_6502_LDA_ABSY	= 0xb9,
	OP_6502_LDA_INDX	= 0xa1,
	OP_6502_LDA_INDY	= 0xb1
} OPCODES_6502;

#endif // DROMAIUS_CPU_6502_OPCODES_H
