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
	bool		pin_irq;
	bool		pin_nmi;
	bool		pin_sync;
	bool		pin_rdy;

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
	computer->pin_reset = ACTLO_ASSERT;
	computer->pin_clock = true;
	cpu_6502_process(computer->cpu);

	computer->pin_reset = ACTLO_DEASSERT;

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
	computer->pin_reset = ACTLO_ASSERT;
	computer->pin_rw = false;
	computer->pin_irq = true;
	computer->pin_nmi = true;
	computer->pin_rdy = true;

	computer->cpu = cpu_6502_create(
						&computer->bus_address, 
						&computer->bus_data,
						&computer->pin_clock,
						&computer->pin_reset,
						&computer->pin_rw, 
						&computer->pin_irq, 
						&computer->pin_nmi,
						&computer->pin_sync,
						&computer->pin_rdy);

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
		.pin_reset = ACTLO_ASSERT,
		.pin_clock = true,
		.pin_irq = true,
		.pin_nmi = true,
		.pin_rdy = true
	};
	computer.cpu = cpu_6502_create(
						&computer.bus_address, 
						&computer.bus_data,
						&computer.pin_clock,
						&computer.pin_reset,
						&computer.pin_rw,
						&computer.pin_irq,
						&computer.pin_nmi,
						&computer.pin_sync,
						&computer.pin_rdy);

	munit_assert_not_null(computer.cpu);

	/* deassert reset */
	cpu_6502_process(computer.cpu);
	computer.pin_reset = ACTLO_DEASSERT;

	/* ignore first 5 cycles */
	for (int i = 0; i < 5; ++i) {
		computer.pin_clock = false;
		cpu_6502_process(computer.cpu);
		munit_assert_true(computer.pin_rw);

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

	munit_assert_uint8(LO_BYTE(computer.cpu->reg_pc), ==, 0x01);
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

MunitResult test_irq(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	// initialize registers
	computer->cpu->reg_sp = 0xff;

	// execute a random opcode
	// -- fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_IMM);

	// assert the irq-line while the cpu decodes the opcode
	computer->pin_irq = false;

	// -- fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x01);

	// cpu should now execute the interrupt sequence

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_BRK;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 02: force BRK instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_BRK;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 03: push program counter - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x08);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: push program counter - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x03);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: push processor satus
	munit_assert_uint16(computer->bus_address, ==, 0x01fd);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, computer->cpu->reg_p);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfc);

	// >> cycle 06: fetch vector low
	munit_assert_uint16(computer->bus_address, ==, 0xfffe);
	munit_assert_true(computer->pin_rw);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 07: fetch vector high
	munit_assert_uint16(computer->bus_address, ==, 0xffff);
	munit_assert_true(computer->pin_rw);
	computer->bus_data = 0x00;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0050);

	return MUNIT_OK;
}

MunitResult test_nmi(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	// initialize registers
	computer->cpu->reg_sp = 0xff;

	// execute a random opcode
	// -- fetch opcode (+ pulse the nmi line)
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LDA_IMM;

	computer->pin_nmi = false;

	computer->pin_clock = true;
	cpu_6502_process(computer->cpu);

	computer->pin_nmi = true;

	computer->pin_clock = false;
	cpu_6502_process(computer->cpu);

	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LDA_IMM);

	// -- fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x01;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x01);

	// cpu should now execute the interrupt sequence

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_BRK;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 02: force BRK instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_BRK;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 03: push program counter - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x08);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: push program counter - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x03);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: push processor satus
	munit_assert_uint16(computer->bus_address, ==, 0x01fd);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, computer->cpu->reg_p);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfc);

	// >> cycle 06: fetch vector low
	munit_assert_uint16(computer->bus_address, ==, 0xfffa);
	munit_assert_true(computer->pin_rw);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 07: fetch vector high
	munit_assert_uint16(computer->bus_address, ==, 0xfffb);
	munit_assert_true(computer->pin_rw);
	computer->bus_data = 0x00;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0050);

	return MUNIT_OK;
}

MunitResult test_rdy(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 254;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ABS);

	// deassert RDY - cpu should halt
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	computer->pin_rdy = false;
	computer_clock_cycle(computer);
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	computer_clock_cycle(computer);
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	// reassert RDY - cpu continues operation
	computer->pin_rdy = true;

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
	computer->bus_data = 2;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_adc(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: immediate operand
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 13;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	munit_assert_true(computer->pin_sync);
	computer->bus_data = OP_6502_ADC_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_IMM);
	munit_assert_false(computer->pin_sync);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 15;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 28);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: zero page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 13;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 15;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 29);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: zero page, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 254;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ZPX);

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
	computer->bus_data = 6;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 5);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	
	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 254;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ABS);

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
	computer->bus_data = 2;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 5;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ABSX);

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
	computer->bus_data = 7;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 12);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 127;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ABSX);

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
	computer->bus_data = 2;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, (uint8_t) -127);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 5;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ABSY);

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
	computer->bus_data = -3;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 2);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing y-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 5;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_ABSY);

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
	computer->bus_data = -7;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, (uint8_t) -2);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = -5;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_INDX);

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
	computer->bus_data = -7;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, -12);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = -66;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// force the value of the y register
	computer->cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_INDY);

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
	computer->bus_data = -65;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 125);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: indirect indexed addressing (index-y), page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = -66;
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = false;

	// force the value of the y register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_INDY);

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
	computer->bus_data = 66;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: decimal addition
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = BCD_BYTE(7, 9); 
	computer->cpu->p_carry = false;
	computer->cpu->p_decimal_mode = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ADC_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = BCD_BYTE(1, 4);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, BCD_BYTE(9, 3));
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_ADC_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer->bus_data = BCD_BYTE(2, 1);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, BCD_BYTE(1, 4));
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	computer->cpu->reg_a = BCD_BYTE(9, 9); 
	computer->cpu->p_carry = false;
	munit_assert_uint16(computer->bus_address, ==, 0x0805);
	computer->bus_data = OP_6502_ADC_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ADC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0806);
	computer->bus_data = BCD_BYTE(9, 9);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, BCD_BYTE(9, 8));
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_and(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: immediate operand
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: zero page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: zero page, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_ZPX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_ABS);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_ABSX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_ABSX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_ABSY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing y-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_ABSY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_INDX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_INDY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: indirect indexed addressing (index-y), page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_AND_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_AND_INDY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: test interaction with flags
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_AND_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b00001111;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0b00000011);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_AND_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b00111100;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0x00);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 1));		// zero flag
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_AND_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b10001111;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000011);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);
	munit_assert_false(IS_BIT_SET(computer->cpu->reg_p, 1));	// zero flag
	munit_assert_true(IS_BIT_SET(computer->cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}

MunitResult test_asl(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: accumulator + effect on flags
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b10100000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ASL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_ASL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01000000);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 03: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_ASL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_ASL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000000);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> cycle 05: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_ASL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer->bus_data = OP_6502_ASL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b00000000);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: zero-page addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ASL_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x65;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 04: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer_clock_cycle(computer);

	// >> cycle 05: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0b01010100);
	munit_assert_true(computer->pin_rw);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: zero-page addressing, x indexed
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ASL_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer->bus_data = 0b01010101;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b10101010);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: absolute addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ASL_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b01010100);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ASL_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0b01010101;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b10101010);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ASL_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ASL_ABSX);

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

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer->bus_data = 0b01010101;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b10101010);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_bcc(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCC: branch not taken
	//

	// initialize registers
	computer->cpu->p_carry = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BCC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BCC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCC: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_carry = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BCC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BCC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCC: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_carry = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BCC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BCC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bcs(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCS: branch not taken
	//

	// initialize registers
	computer->cpu->p_carry = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BCS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BCS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCS: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_carry = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BCS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BCS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCS: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_carry = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BCS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BCS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_beq(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BEQ: branch not taken
	//

	// initialize registers
	computer->cpu->p_zero_result = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BEQ;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BEQ);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BEQ: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_zero_result = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BEQ;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BEQ);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BEQ: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_zero_result = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BEQ;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BEQ);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bit(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BIT: zero page addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 0b00000010;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BIT_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BIT_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 0b01001111;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_negative_result);		// takes value of bit 7
	munit_assert_true(computer->cpu->p_overflow);				// takes value of bit 6
	munit_assert_false(computer->cpu->p_zero_result);			// set if memory & reg-a == 0

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BIT: absolute addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 0b00100000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BIT_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BIT_ABS);

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
	computer->bus_data = 0b10011111;
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_negative_result);		// takes value of bit 7
	munit_assert_false(computer->cpu->p_overflow);				// takes value of bit 6
	munit_assert_true(computer->cpu->p_zero_result);			// set if memory & reg-a == 0

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}


MunitResult test_bmi(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BMI: branch not taken
	//

	// initialize registers
	computer->cpu->p_negative_result = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BMI;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BMI);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BMI: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_negative_result = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BMI;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BMI);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BMI: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_negative_result = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BMI;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BMI);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bne(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BNE: branch not taken
	//

	// initialize registers
	computer->cpu->p_zero_result = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BNE;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BNE);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BNE: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_zero_result = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BNE;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BNE);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BNE: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_zero_result = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BNE;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BNE);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bpl(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BPL: branch not taken
	//

	// initialize registers
	computer->cpu->p_negative_result = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BPL;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BPL);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BPL: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_negative_result = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BPL;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BPL);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BPL: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_negative_result = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BPL;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BPL);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_brk(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BRK: force interrupt
	//

	// initialize registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BRK;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 02: force BRK instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BRK;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 03: push program counter - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x08);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: push program counter - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x02);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: push processor satus
	munit_assert_uint16(computer->bus_address, ==, 0x01fd);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, computer->cpu->reg_p);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfc);

	// >> cycle 06: fetch vector low
	munit_assert_uint16(computer->bus_address, ==, 0xfffe);
	munit_assert_true(computer->pin_rw);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 07: fetch vector high
	munit_assert_uint16(computer->bus_address, ==, 0xffff);
	munit_assert_true(computer->pin_rw);
	computer->bus_data = 0x00;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0050);

	return MUNIT_OK;
}

MunitResult test_bvc(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVC: branch not taken
	//

	// initialize registers
	computer->cpu->p_overflow = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BVC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BVC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVC: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_overflow = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BVC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BVC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVC: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_overflow = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BVC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BVC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bvs(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVS: branch not taken
	//

	// initialize registers
	computer->cpu->p_overflow = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BVS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BVS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVS: branch taken, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_overflow = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BVS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BVS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVS: branch taken, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->p_overflow = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_BVS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_BVS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = -0x50;
	computer_clock_cycle(computer);

	// >> cycle 03: add offset
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);

	// >> cycle 04: handle page crossing
	munit_assert_uint16(computer->bus_address, ==, 0x08b3);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_clc(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLC: clear carry
	//

	// initialize registers
	computer->cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CLC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CLC);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0b11111110);

	return MUNIT_OK;
}

MunitResult test_cld(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLC: clear decimal mode
	//

	// initialize registers
	computer->cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CLD;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CLD);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_decimal_mode);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0b11110111);

	return MUNIT_OK;
}

MunitResult test_cli(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLI: clear interrupt disable bit
	//

	// initialize registers
	computer->cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CLI;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CLI);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_interrupt_disable);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0b11111011);

	return MUNIT_OK;
}

MunitResult test_clv(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLV: clear overflow flag
	//

	// initialize registers
	computer->cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CLV;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CLV);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0b10111111);

	return MUNIT_OK;
}

MunitResult test_cmp(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: immediate operand
	//

	// initialize registers
	computer->cpu->reg_a = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: zero page addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 15;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_true(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_false(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: zero page, x indexed
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer->bus_data = 5;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_ABS);

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
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_ABSX);

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
	computer->bus_data = 16;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_true(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_false(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_ABSX);

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
	computer->bus_data = 5;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_ABSY);

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
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing y-indexed, page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_ABSY);

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
	computer->bus_data = 20;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_true(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_false(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the x register
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_INDX);

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
	computer->bus_data = 6;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the y register
	computer->cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_INDY);

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
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: indirect indexed addressing (index-y), page crossing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_a = 10;

	// force the value of the y register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CMP_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CMP_INDY);

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
	computer->bus_data = 11;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_true(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_false(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	return MUNIT_OK;
}

MunitResult test_cpx(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPX: immediate operand
	//

	// initialize registers
	computer->cpu->reg_x = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CPX_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CPX_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPX: zero page addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_x = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CPX_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CPX_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 15;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_true(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_false(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPX: absolute addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_x = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CPX_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CPX_ABS);

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
	computer->bus_data = 7;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_cpy(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPY: immediate operand
	//

	// initialize registers
	computer->cpu->reg_y = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CPY_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CPY_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPY: zero page addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_y = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CPY_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CPY_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 15;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_true(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_false(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPY: absolute addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_y = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_CPY_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_CPY_ABS);

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
	computer->bus_data = 7;
	computer_clock_cycle(computer);
	munit_assert_false(computer->cpu->p_zero_result);			// set if equal
	munit_assert_false(computer->cpu->p_negative_result);		// set if memory > reg-a
	munit_assert_true(computer->cpu->p_carry);					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_dec(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: zero-page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_DEC_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x65;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer->bus_data = 1;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 04: perform decrement
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer_clock_cycle(computer);

	// >> cycle 05: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: zero-page addressing, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_DEC_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEC_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer->bus_data = 0;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform decrement
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, (uint8_t) -1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_DEC_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEC_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 15;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform decrement
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 14);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_DEC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 1;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_DEC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEC_ABSX);

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

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer->bus_data = 5;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 4);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_dex(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEX: clear interrupt disable bit
	//

	// initialize registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = 2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_DEX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_DEX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_DEX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, (uint8_t) -1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_dey(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEY: clear interrupt disable bit
	//

	// initialize registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_y = 2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_DEY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_DEY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_DEY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_DEY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, (uint8_t) -1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_eor(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: immediate operand
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: zero page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: zero page, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_ZPX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_ABS);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_ABSX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_ABSX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_ABSY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing y-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_ABSY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_INDX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_INDY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: indirect indexed addressing (index-y), page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_EOR_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_EOR_INDY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: test interaction with flags
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_EOR_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b10001111;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01001100);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_EOR_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b11000011;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0x00);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_EOR_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b00001111;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11001100);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_inc(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: zero-page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_INC_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x65;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer->bus_data = 1;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 04: perform increment
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer_clock_cycle(computer);

	// >> cycle 05: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 2);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: zero-page addressing, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_INC_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INC_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer->bus_data = 0xff;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform increment
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_INC_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INC_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 127;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform decrement
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0x80);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_INC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 1;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform increment
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 2);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_INC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INC_ABSX);

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

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer->bus_data = 5;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform increment
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 6);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_inx(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// INX: clear interrupt disable bit
	//

	// initialize registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_x = (uint8_t) -2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_INX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, (uint8_t) -1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_INX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 0);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_INX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_x, ==, 1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_iny(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// INY: clear interrupt disable bit
	//

	// initialize registers
	computer->cpu->reg_p = 0;
	computer->cpu->reg_y = (uint8_t) -2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_INY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, (uint8_t) -1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_INY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 0);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_INY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_INY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_y, ==, 1);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_jmp(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// JMP: absolute addressing
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_JMP_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_JMP_ABS);

	// >> cycle 02: fetch jump-address, low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch jump-address, high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0x09;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0951);

	/////////////////////////////////////////////////////////////////////////////
	//
	// JMP: indirect addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_JMP_IND;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_JMP_IND);

	// >> cycle 02: fetch indirect-address, low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch indirect-address, high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0x0C;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch jump-address, low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0C10);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch jump-address, high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0C11);
	computer->bus_data = 0x09;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0951);

	return MUNIT_OK;
}

MunitResult test_jsr(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// JSR
	//

	// init registers
	computer->cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_JSR;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_JSR);

	// >> cycle 02: fetch jump-address, low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: store address in cpu for later use
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	computer_clock_cycle(computer);

	// >> cycle 04: push program counter - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x08);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfe);

	// >> cycle 05: push program counter - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_int8(computer->bus_data, ==, 0x03);
	munit_assert_int8(computer->cpu->reg_sp, ==, 0xfd);

	// >> cycle 06: fetch jump-address, high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	munit_assert_true(computer->pin_rw);
	computer->bus_data = 0x09;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0951);

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

MunitResult test_lsr(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: accumulator + effect on flags
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b00000101;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LSR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_LSR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b00000010);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 03: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_LSR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_LSR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b00000001);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 05: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_LSR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer->bus_data = OP_6502_LSR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b00000000);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: zero-page addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LSR_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x65;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer->bus_data = 0b10001010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 04: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer_clock_cycle(computer);

	// >> cycle 05: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b01000101);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: zero-page addressing, x indexed
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LSR_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer->bus_data = 0b01010011;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b00101001);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: absolute addressing
	//

	computer_reset(computer);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LSR_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 0b10000010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b01000001);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LSR_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0b00010000;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b00001000);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_LSR_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_LSR_ABSX);

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

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer->bus_data = 0b01000001;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b00100000);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_nop(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// NOP
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_NOP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_NOP);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_ora(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: immediate operand
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: zero page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: zero page, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_ZPX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_ABS);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_ABSX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_ABSX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_ABSY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing y-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_ABSY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the x register
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_INDX);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_INDY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: indirect indexed addressing (index-y), page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b11000011;

	// force the value of the y register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ORA_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ORA_INDY);

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
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: test interaction with flags
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b01000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_ORA_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b00001111;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01001111);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// initialize used registers
	computer->cpu->reg_a = 0b00000000;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_ORA_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b00000000;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0x00);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// initialize used registers
	computer->cpu->reg_a = 0b01000011;

	// >> cycle 01: fetch opcode
	computer->bus_data = OP_6502_ORA_IMM;
	computer_clock_cycle(computer);

	// >> cycle 02: fetch operand + execute
	computer->bus_data = 0b10001100;
	computer_clock_cycle(computer);

	munit_assert_uint8(computer->cpu->reg_a, ==, 0b11001111);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_pha(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PHA
	//

	// init registers
	computer->cpu->reg_a  = 0x13;
	computer->cpu->reg_p  = 0b10010101;
	computer->cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_PHA;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_PHA);

	// >> cycle 02: fetch discard - decode pha
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: write a on the stack
	munit_assert_false(computer->pin_rw);
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x13);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_php(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PHP
	//

	// init registers
	computer->cpu->reg_a  = 0x13;
	computer->cpu->reg_p  = 0b10010101;
	computer->cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_PHP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_PHP);

	// >> cycle 02: fetch discard - decode php
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: write a on the stack
	munit_assert_false(computer->pin_rw);
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0b10010101);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_pla(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PLA
	//

	// init registers
	computer->cpu->reg_sp = 0xfe;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_PLA;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_PLA);

	// >> cycle 02: fetch discard - decode pla
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: read stack - increment stack pointer
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	computer->bus_data = 0x23;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_sp, ==, 0xff);

	// >> cycle 04: fetch reg-a
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	computer->bus_data = 0x19;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x19);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_plp(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PLP
	//

	// init registers
	computer->cpu->reg_sp = 0xfe;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_PLP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_PLP);

	// >> cycle 02: fetch discard - decode plp
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: read stack - increment stack pointer
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	computer->bus_data = 0x23;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_sp, ==, 0xff);

	// >> cycle 04: fetch reg-a
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	computer->bus_data = 0x79;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0x79);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_rol(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: accumulator + effect on flags
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b10100000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_ROL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01000000);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 03: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_ROL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_ROL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000001);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> cycle 05: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_ROL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer->bus_data = OP_6502_ROL_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b00000010);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: zero-page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROL_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x65;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 04: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer_clock_cycle(computer);

	// >> cycle 05: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b01010101);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: zero-page addressing, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_x = 0xf0;
	computer->cpu->p_carry = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROL_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer->bus_data = 0b01010101;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b10101010);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROL_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 0b10101010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b01010100);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = false;
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROL_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0b01010101;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b10101010);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = false;
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROL_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROL_ABSX);

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

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer->bus_data = 0b01010101;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b10101010);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_ror(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: accumulator + effect on flags
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 0b00000101;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_ROR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b00000010);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 03: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = OP_6502_ROR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_ROR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b10000001);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> cycle 05: fetch operand
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_ROR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer->bus_data = OP_6502_ROR_ACC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0b01000000);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: zero-page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROR_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x65;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer->bus_data = 0b10001010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 04: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	computer_clock_cycle(computer);

	// >> cycle 05: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x0065);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b11000101);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: zero-page addressing, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = false;
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROR_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x4a;
	computer_clock_cycle(computer);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(computer->bus_address, ==, 0x004a);
	computer->bus_data = 0x21;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer->bus_data = 0b01010011;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0x003a);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b00101001);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROR_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer->bus_data = 0b10000010;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 05: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc016);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b01000001);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = false;
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROR_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x16;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = 0xc0;
	computer_clock_cycle(computer);

	// >> cycle 04: add carry
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0b00010000;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b00001000);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->p_carry = false;
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_ROR_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_ROR_ABSX);

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

	// >> cycle 05: fetch operand 
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer->bus_data = 0b01000001;
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);

	// >> cycle 06: perform rotate
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	computer_clock_cycle(computer);

	// >> cycle 06: write result set flags
	munit_assert_uint16(computer->bus_address, ==, 0xc108);
	munit_assert_false(computer->pin_rw);
	computer_clock_cycle(computer);
	munit_assert_true(computer->pin_rw);
	munit_assert_uint8(computer->bus_data, ==, 0b00100000);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_rts(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// RTS
	//

	// init registers
	computer->cpu->reg_sp = 0xfd;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_RTS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_RTS);

	// >> cycle 02: fetch discard - decode rts
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: increment stack pointer
	munit_assert_uint16(computer->bus_address, ==, 0x01fd);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: pop program counter - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 05: pop program counter - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	computer->bus_data = 0x09;
	computer_clock_cycle(computer);

	// >> cycle 06: increment pc
	munit_assert_uint16(computer->bus_address, ==, 0x0951);
	computer->bus_data = 0x12;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0952);

	return MUNIT_OK;
}

MunitResult test_rti(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// RTI
	//

	// init registers
	computer->cpu->reg_sp = 0xfc;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_RTI;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_RTI);

	// >> cycle 02: fetch discard - decode rts
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 03: increment stack pointer
	munit_assert_uint16(computer->bus_address, ==, 0x01fc);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: pop processor status
	munit_assert_uint16(computer->bus_address, ==, 0x01fd);
	computer->bus_data = 0x00;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0x00);

	// >> cycle 05: pop program counter - low byte
	munit_assert_uint16(computer->bus_address, ==, 0x01fe);
	computer->bus_data = 0x51;
	computer_clock_cycle(computer);

	// >> cycle 06: pop program counter - high byte
	munit_assert_uint16(computer->bus_address, ==, 0x01ff);
	computer->bus_data = 0x09;
	computer_clock_cycle(computer);

	// >> next instruction
	munit_assert_uint16(computer->bus_address, ==, 0x0951);

	return MUNIT_OK;
}

MunitResult test_sbc(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// true: immediate operand
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 13;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 3);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: zero page addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 13;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	computer->bus_data = 15;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, (uint8_t) -2);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: zero page, x indexed
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 254;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_ZPX);

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
	computer->bus_data = 6;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 248);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);
	
	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 2;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_ABS);

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
	computer->bus_data = 2;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 5;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_ABSX);

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
	computer->bus_data = 7;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, -2);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing x-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = -120;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_ABSX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_ABSX);

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
	computer->bus_data = 10;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 126);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 5;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the y register
	computer->cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_ABSY);

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
	computer->bus_data = -3;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 8);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SSB: absolute addressing y-indexed, page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = 5;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_ABSY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_ABSY);

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
	computer->bus_data = -7;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 12);
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: indexed indirect addressing (index-x)
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = -5;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the x register
	computer->cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_INDX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_INDX);

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
	computer->bus_data = -7;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 2);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = -66;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the y register
	computer->cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_INDY);

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
	computer->bus_data = 65;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 125);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_true(computer->cpu->p_overflow);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: indirect indexed addressing (index-y), page crossing
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = -66;
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = false;

	// force the value of the y register
	computer->cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_INDY;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_INDY);

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
	computer->bus_data = -66;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_overflow);
	munit_assert_true(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: decimal subtraction
	//

	computer_reset(computer);

	// initialize used registers
	computer->cpu->reg_a = BCD_BYTE(4, 4); 
	computer->cpu->p_carry = true;
	computer->cpu->p_decimal_mode = true;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SBC_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = BCD_BYTE(2, 9);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, BCD_BYTE(1, 5));
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0803);
	computer->bus_data = OP_6502_SBC_IMM;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SBC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(computer->bus_address, ==, 0x0804);
	computer->bus_data = BCD_BYTE(2, 9);
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, BCD_BYTE(8, 6));
	munit_assert_false(computer->cpu->p_carry);
	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_true(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_sec(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// SEC: set carry
	//

	// initialize registers
	computer->cpu->reg_p = 0b00000000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SEC;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SEC);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_carry);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0b00000001);

	return MUNIT_OK;
}

MunitResult test_sed(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// SED: set decimal flag
	//

	// initialize registers
	computer->cpu->reg_p = 0b00000000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SED;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SED);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_decimal_mode);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0b00001000);

	return MUNIT_OK;
}

MunitResult test_sei(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// SEI: set interrupt disable status
	//

	// initialize registers
	computer->cpu->reg_p = 0b00000000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_SEI;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_SEI);

	// >> cycle 02: execute opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer_clock_cycle(computer);
	munit_assert_true(computer->cpu->p_interrupt_disable);
	munit_assert_uint8(computer->cpu->reg_p, ==, 0b00000100);

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
	// STA: absolute addressing x-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x67;
	computer->cpu->reg_x = 0x02;

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
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x67);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing x-indexed, page crossing
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
	// STA: absolute addressing y-indexed, no page crossing
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x69;
	computer->cpu->reg_y = 0x02;

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
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	computer->bus_data = 0x10;
	computer_clock_cycle(computer);

	// >> cycle 05: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0xc018);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x69);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing y-indexed, page crossing
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
	// STA: indirect indexed addressing (index-y), no page crossing
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_a = 0x73;
	computer->cpu->reg_y = 0x03;

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
	munit_assert_uint16(computer->bus_address, ==, 0x3019);
	computer->bus_data = 0x17;
	computer_clock_cycle(computer);

	// >> cycle 06: store value to memory
	munit_assert_uint16(computer->bus_address, ==, 0x3019);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x73);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: indirect indexed addressing (index-y), page crossing
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

MunitResult test_sty(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// STY: zero page addressing
	//

	computer_reset(computer);

	// initialize registers
	computer->cpu->reg_y = 0x41;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STY_ZP;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STY_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfe;
	computer_clock_cycle(computer);

	// >> cycle 03: store to memory
	munit_assert_uint16(computer->bus_address, ==, 0x00fe);
	munit_assert_false(computer->pin_rw);				// writing
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->bus_data, ==, 0x41);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STY: zero page, x indexed
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_x = 0xf0;
	computer->cpu->reg_y = 0x43;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STY_ZPX;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STY_ZPX);

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
	munit_assert_uint8(computer->bus_data, ==, 0x43);

	/////////////////////////////////////////////////////////////////////////////
	//
	// STY: absolute addressing
	//

	computer_reset(computer);

	// force the value of the used registers
	computer->cpu->reg_y = 0x45;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_STY_ABS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_STY_ABS);

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
	munit_assert_uint8(computer->bus_data, ==, 0x45);

	return MUNIT_OK;
}

MunitResult test_tax(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

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

	return MUNIT_OK;
}

MunitResult test_tsx(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

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
	return MUNIT_OK;
}

MunitResult test_tay(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

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

	return MUNIT_OK;
}

MunitResult test_txa(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TXA: transfer index X to accumulator 
	//

	computer_reset(computer);

	// force values of the used registers
	computer->cpu->reg_a = 0x00;
	computer->cpu->reg_x = 0x27;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_TXA;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_TXA);

	// >> cycle 02: execute 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfc;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x27);

	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitResult test_txs(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TXS: transfer index X to stack pointer
	//

	computer_reset(computer);

	// force values of the used registers
	computer->cpu->reg_sp = 0x00;
	computer->cpu->reg_x = 0x8a;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_TXS;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_TXS);

	// >> cycle 02: execute 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfc;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_sp, ==, 0x8a);

	return MUNIT_OK;
}

MunitResult test_tya(const MunitParameter params[], void *user_data_or_fixture) {

	Computer *computer = (Computer *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TYA: transfer index Y to accumulator
	//

	computer_reset(computer);

	// force values of the used registers
	computer->cpu->reg_a = 0x00;
	computer->cpu->reg_y = 0x27;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(computer->bus_address, ==, 0x0801);
	computer->bus_data = OP_6502_TYA;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_ir, ==, OP_6502_TYA);

	// >> cycle 02: execute 
	munit_assert_uint16(computer->bus_address, ==, 0x0802);
	computer->bus_data = 0xfc;
	computer_clock_cycle(computer);
	munit_assert_uint8(computer->cpu->reg_a, ==, 0x27);

	munit_assert_false(computer->cpu->p_zero_result);
	munit_assert_false(computer->cpu->p_negative_result);

	return MUNIT_OK;
}

MunitTest cpu_6502_tests[] = {
	{ "/reset", test_reset, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/irq", test_irq, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/nmi", test_nmi, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/rdy", test_rdy, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/adc", test_adc, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/and", test_and, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/asl", test_asl, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bcc", test_bcc, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bcs", test_bcs, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/beq", test_beq, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bit", test_bit, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bmi", test_bmi, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bne", test_bne, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bpl", test_bpl, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/brk", test_brk, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bvc", test_bvc, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/bvs", test_bvs, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/clc", test_clc, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cld", test_cld, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cli", test_cli, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/clv", test_clv, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cmp", test_cmp, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cpx", test_cpx, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cpy", test_cpy, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/dec", test_dec, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/dex", test_dex, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/dey", test_dey, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/eor", test_eor, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/inc", test_inc, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/inx", test_inx, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/iny", test_iny, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/jmp", test_jmp, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/jsr", test_jsr, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/lda", test_lda, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ldx", test_ldx, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ldy", test_ldy, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/lsr", test_lsr, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/nop", test_nop, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ora", test_ora, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/pha", test_pha, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/php", test_php, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/pla", test_pla, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/plp", test_plp, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/rol", test_rol, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ror", test_ror, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/rts", test_rts, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/rti", test_rti, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sbc", test_sbc, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sec", test_sec, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sed", test_sed, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sei", test_sei, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sta", test_sta, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/stx", test_stx, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sty", test_sty, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/tax", test_tax, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/tay", test_tay, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/tsx", test_tsx, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/txa", test_txa, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/txs", test_txs, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/tya", test_tya, cpu_6502_setup, cpu_6502_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
