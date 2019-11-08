// test/test_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "cpu_6502.h"
#include "cpu_6502_opcodes.h"

typedef struct Computer {
	uint16_t	bus_address;
	uint8_t		bus_data;
	bool		pin_clock;
	bool		pin_reset;
	bool		pin_rw;

	Cpu6502 *	cpu;
} Computer;

static inline void computer_clock_cycle(Computer *computer) {
	munit_assert_not_null(computer);

	computer->pin_clock = true;
	cpu_6502_process(computer->cpu);

	computer->pin_clock = false;
	cpu_6502_process(computer->cpu);
}

static void computer_reset(Computer *computer) {
	computer->pin_reset = true;
	computer->pin_clock = true;
	cpu_6502_process(computer->cpu);

	computer->pin_reset = false;

	// ignore first 5 cycles 
	for (int i = 0; i < 5; ++i) {
		computer->pin_clock = false;
		cpu_6502_process(computer->cpu);

		computer->pin_clock = true;
		cpu_6502_process(computer->cpu);
	}

	// cpu should now read address 0xfffc - low byte of reset vector 
	computer->pin_clock = false;
	cpu_6502_process(computer->cpu);

	computer->bus_data = 0x01;

	computer->pin_clock = true;
	cpu_6502_process(computer->cpu);

	// cpu should now read address 0xfffd - high byte of reset vector
	computer->pin_clock = false;
	cpu_6502_process(computer->cpu);

	computer->bus_data = 0x08;

	computer->pin_clock = true;
	cpu_6502_process(computer->cpu);

	// end last init cycle - start fetch of first instruction 
	computer->pin_clock = false;
	cpu_6502_process(computer->cpu);
}

static void *cpu_6502_setup(const MunitParameter params[], void *user_data) {

	// create the machine
	Computer *computer = (Computer *) malloc(sizeof(Computer));
	computer->bus_address = 0;
	computer->bus_data = 0;
	computer->pin_clock = true;
	computer->pin_reset = true;
	computer->pin_rw = false;

	computer->cpu = cpu_6502_create(
						&computer->bus_address, 
						&computer->bus_data,
						&computer->pin_clock,
						&computer->pin_reset,
						&computer->pin_rw);

	// initialize the machine
	computer_reset(computer);

	return computer;
}

static void cpu_6502_teardown(void *fixture) {
	Computer *computer = (Computer *) fixture;
	free(computer);
}

MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	
	Computer computer = {
		.pin_reset = true,
		.pin_clock = true
	};
	computer.cpu = cpu_6502_create(
						&computer.bus_address, 
						&computer.bus_data,
						&computer.pin_clock,
						&computer.pin_reset,
						&computer.pin_rw);

	munit_assert_not_null(computer.cpu);

	/* deassert reset */
	cpu_6502_process(computer.cpu);
	computer.pin_reset = false;

	/* ignore first 5 cycles */
	for (int i = 0; i < 5; ++i) {
		computer.pin_clock = false;
		cpu_6502_process(computer.cpu);

		computer.pin_clock = true;
		cpu_6502_process(computer.cpu);
	}

	/* cpu should now read address 0xfffc - low byte of reset vector */
	computer.pin_clock = false;
	cpu_6502_process(computer.cpu);

	munit_assert_uint16(computer.bus_address, ==, 0xfffc);
	computer.bus_data = 0x01;

	computer.pin_clock = true;
	cpu_6502_process(computer.cpu);

	/* cpu should now read address 0xfffd - high byte of reset vector */
	computer.pin_clock = false;
	cpu_6502_process(computer.cpu);

	munit_assert_uint8(LOBYTE(computer.cpu->reg_pc), ==, 0x01);
	munit_assert_uint16(computer.bus_address, ==, 0xfffd);
	computer.bus_data = 0x08;

	computer.pin_clock = true;
	cpu_6502_process(computer.cpu);

	/* cpu should now read from address 0x0801 */
	computer.pin_clock = false;
	cpu_6502_process(computer.cpu);

	munit_assert_uint16(computer.cpu->reg_pc, ==, 0x0801);
	munit_assert_uint16(computer.bus_address, ==, 0x0801);

	return MUNIT_OK;
}

MunitResult test_lda(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: immediate operand
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: zero page addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 0x05;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x05);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: zero page, x indexed
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, (0x4a + 0xf0) & 0xff);
	computer->bus_data = 0x03;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x03);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 0x07;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x07);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x09;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x09);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc008);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer->bus_data = 0x11;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x11);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc019);
	computer->bus_data = 0x13;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x13);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing y-indexed, page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc009);
	computer->bus_data = 0x15;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc109);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x16);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add zp + index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x11;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x005a);
	computer->bus_data = 0x20;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x005b);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x2120);
	computer->bus_data = 0x19;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x19);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// force the value of the y register
	computer->cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(computer->bus_address, ==, 0x004b);
	computer->bus_data = 0x30;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x3026);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x21);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: indirect indexed addressing (index-y), page crossing
	//

	computer_reset(computer);

	// force the value of the y register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(computer->bus_address, ==, 0x004b);
	computer->bus_data = 0x30;
	computer_clock_cycle(computer);

	// >> cycle 05: add carry
	munit_assert_uint16(computer->bus_address, ==, 0x3006);
	computer->bus_data = 0x17;
	computer_clock_cycle(computer);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x3106);
	computer->bus_data = 0x23;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x23);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: test interaction with flags
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDA_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0x01);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 1));	// zero flag
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDA_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x00;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0x00);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 1));		// zero flag
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDA_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x81;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0x81);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 1));	// zero flag
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}

MunitResult test_ldx(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: immediate operand
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDX_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDX_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: zero page 
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDX_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDX_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x2a;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x002a);
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: zero page, y indexed
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDX_ZPY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDX_ZPY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and y-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, (0x4a + 0xf0) & 0xff);
	computer->bus_data = 0x03;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x03);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: absolute addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDX_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDX_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 0x07;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x07);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDX_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDX_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc019);
	computer->bus_data = 0x13;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x13);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDX_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDX_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc009);
	computer->bus_data = 0x15;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc109);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x16);

	/////////////////////////////////////////////////////////////////////////////
	//
	// TAX: transfer accumulator to index X
	//

	computer_reset(computer);

	// force values of the used registers
	computer->cpu->reg_a = 0x27;
	computer->cpu->reg_x = 0x00;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_TAX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_TAX);

	// >> cycle 02: execute 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfc;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x27);

	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// TSX: transfer stack pointer to index X
	//

	computer_reset(computer);

	// force values of the used registers
	computer->cpu->reg_sp = 0x8e;
	computer->cpu->reg_x = 0x00;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_TSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_TSX);

	// >> cycle 02: execute 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfc;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0x8e);

	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: test interaction with flags
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDX_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_x, ==, 0x01);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 1));	// zero flag
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDX_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x00;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_x, ==, 0x00);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 1));		// zero flag
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDX_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x81;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_x, ==, 0x81);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 1));	// zero flag
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}


MunitResult test_ldy(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: immediate operand
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDY_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDY_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: zero page 
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDY_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDY_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x2a;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x002a);
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: zero page, x indexed
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDY_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDY_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, (0x4a + 0xf0) & 0xff);
	computer->bus_data = 0x03;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0x03);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: absolute addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDY_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDY_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 0x07;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0x07);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDY_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDY_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc019);
	computer->bus_data = 0x13;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0x13);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDY_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDY_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc009);
	computer->bus_data = 0x15;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0xc109);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0x16);

	/////////////////////////////////////////////////////////////////////////////
	//
	// TAY: transfer accumulator to index Y
	//

	computer_reset(computer);

	// force values of the used registers
	computer->cpu->reg_a = 0x27;
	computer->cpu->reg_y = 0x00;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_TAY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_TAY);

	// >> cycle 02: execute 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfc;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0x27);

	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: test interaction with flags
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDY_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_y, ==, 0x01);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 1));	// zero flag
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDY_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x00;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_y, ==, 0x00);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 1));		// zero flag
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_LDY_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0x81;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_y, ==, 0x81);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 1));	// zero flag
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}

MunitResult test_sta(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: zero page addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 0x63;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STA_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STA_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x63);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: zero page, x indexed
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x65;
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STA_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STA_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: store to memory
	munit_assert_uint16(computer->bus_address, ==, (0x4a + 0xf0) & 0xff);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x65);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x67;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STA_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STA_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x67);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing x-indexed
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x67;
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STA_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc008);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x67);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing y-indexed
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x69;
	computer->cpu->reg_y = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STA_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc008);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x69);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x71;
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STA_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STA_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add zp + index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x11;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x005a);
	computer->bus_data = 0x20;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x005b);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 06: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0x2120);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x71);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: indirect indexed addressing (index-y)
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x73;
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STA_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(computer->bus_address, ==, 0x004b);
	computer->bus_data = 0x30;
	computer_clock_cycle(computer);

	// >> cycle 05: add carry
	munit_assert_uint16(computer->bus_address, ==, 0x3006);
	computer->bus_data = 0x17;
	computer_clock_cycle(computer);

	// >> cycle 06: store value to memory
	munit_assert_uint16(computer->bus_address, ==, 0x3106);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x73);

	return MUNIT_OK;
}

MunitResult test_stx(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// STX: zero page addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_x = 0x51;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STX_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STX_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x51);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STX: zero page, y indexed
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_x = 0x53;
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STX_ZPY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STX_ZPY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: store to memory
	munit_assert_uint16(computer->bus_address, ==, (0x4a + 0xf0) & 0xff);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x53);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STX: absolute addressing
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_x = 0x55;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STX_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STX_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x55);

	return MUNIT_OK;
}

MunitTest cpu_6502_tests[] = {
	{ "/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/lda", test_lda, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ldx", test_ldx, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ldy", test_ldy, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sta", test_sta, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/stx", test_stx, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
