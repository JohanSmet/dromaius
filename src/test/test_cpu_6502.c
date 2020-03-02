// test/test_cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "cpu_6502.h"
#include "cpu_6502_opcodes.h"

#define SIGNAL_POOL			cpu->signal_pool
#define SIGNAL_COLLECTION	cpu->signals

#define CPU_CYCLE_START				\
	for (int i = 0; i < 4; ++i) {
#define CPU_HALF_CYCLE_START		\
	for (int i = 0; i < 2; ++i) {
#define CPU_CYCLE_END				\
		cpu_process(cpu, i & 1);	\
	}

#define CPU_HALF_CYCLE()			\
		CPU_HALF_CYCLE_START		\
		CPU_CYCLE_END

#define CPU_CYCLE()					\
		CPU_CYCLE_START				\
		CPU_CYCLE_END

#define DATABUS_WRITE(val)					\
	CPU_CYCLE_START							\
		SIGNAL_SET_UINT8(bus_data,(val));	\
	CPU_CYCLE_END


static inline void cpu_process(Cpu6502 *cpu, bool delayed) {
	SIGNAL_SET_BOOL(clock, SIGNAL_BOOL(clock) ^ !delayed);
	signal_pool_cycle(cpu->signal_pool);
	cpu_6502_process(cpu, delayed);
}

static void cpu_reset(Cpu6502 *cpu) {

	// run cpu with reset asserted
	CPU_HALF_CYCLE_START
		SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	CPU_CYCLE_END

	// ignore first 5 cycles 
	for (int i = 0; i < 5; ++i) {
		CPU_CYCLE()
	}

	// cpu should now read address 0xfffc - low byte of reset vector 
	CPU_HALF_CYCLE()
	DATABUS_WRITE(0x01);

	// cpu should now read address 0xfffd - high byte of reset vector
	DATABUS_WRITE(0x08);
}

static void *cpu_6502_setup(const MunitParameter params[], void *user_data) {

	// create the cpu with no signals connected to anything else
	Cpu6502 *cpu = cpu_6502_create(signal_pool_create(), (Cpu6502Signals) {0});

	// initialize the machine
	cpu_reset(cpu);

	return cpu;
}

static void cpu_6502_teardown(void *fixture) {
	cpu_6502_destroy((Cpu6502 *) fixture);
}

MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	
	Cpu6502 *cpu = cpu_6502_create(signal_pool_create(), (Cpu6502Signals) {0});
	munit_assert_not_null(cpu);

	// run cpu with reset asserted
	CPU_HALF_CYCLE_START
		SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	CPU_CYCLE_END

	// deassert reset - ignore first 5 cycles 
	for (int i = 0; i < 5; ++i) {
		CPU_HALF_CYCLE()
		munit_assert_true(SIGNAL_NEXT_BOOL(rw));

		CPU_HALF_CYCLE()
	}

	// cpu should now read address 0xfffc - low byte of reset vector 
	CPU_HALF_CYCLE()
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xfffc);
	DATABUS_WRITE(0x01);

	// cpu should now read address 0xfffd - high byte of reset vector
	munit_assert_uint8(LO_BYTE(cpu->reg_pc), ==, 0x01);
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xfffd);
	DATABUS_WRITE(0x08);

	// cpu should now read from address 0x0801 
	munit_assert_uint16(cpu->reg_pc, ==, 0x0801);
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);

	return MUNIT_OK;
}

MunitResult test_irq(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	// initialize registers
	cpu->reg_sp = 0xff;

	// execute a random opcode
	// -- fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_IMM);

	// assert the irq-line while the cpu decodes the opcode
	// -- fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE_START
		SIGNAL_SET_BOOL(irq_b, ACTLO_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x01);
	CPU_CYCLE_END
	munit_assert_uint8(cpu->reg_a, ==, 0x01);

	// cpu should now execute the interrupt sequence

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_BRK);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 02: force BRK instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_BRK);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 03: push program counter - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE()
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x08);
	CPU_HALF_CYCLE()
	munit_assert_int8(cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: push program counter - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE()
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x03);
	CPU_HALF_CYCLE()
	munit_assert_int8(cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: push processor satus
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fd);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE()
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, cpu->reg_p);
	CPU_HALF_CYCLE()
	munit_assert_int8(cpu->reg_sp, ==, 0xfc);

	// >> cycle 06: fetch vector low
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xfffe);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	DATABUS_WRITE(0x50);

	// >> cycle 07: fetch vector high
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xffff);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	DATABUS_WRITE(0x00);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0050);

	return MUNIT_OK;
}

MunitResult test_nmi(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	// initialize registers
	cpu->reg_sp = 0xff;

	// execute a random opcode
	// -- fetch opcode (+ pulse the nmi line)
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	CPU_HALF_CYCLE_START
		SIGNAL_SET_UINT8(bus_data, OP_6502_LDA_IMM);
		SIGNAL_SET_BOOL(nmi_b, ACTLO_ASSERT);
	CPU_CYCLE_END

	CPU_HALF_CYCLE_START
		SIGNAL_SET_UINT8(bus_data, OP_6502_LDA_IMM);
		SIGNAL_SET_BOOL(nmi_b, ACTLO_DEASSERT);
	CPU_CYCLE_END

	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_IMM);

	// -- fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x01);

	// cpu should now execute the interrupt sequence

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_BRK);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 02: force BRK instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_BRK);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 03: push program counter - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x08);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: push program counter - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x03);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: push processor satus
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fd);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, cpu->reg_p);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfc);

	// >> cycle 06: fetch vector low
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xfffa);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	DATABUS_WRITE(0x50);

	// >> cycle 07: fetch vector high
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xfffb);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	DATABUS_WRITE(0x00);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0050);

	return MUNIT_OK;
}

MunitResult test_rdy(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	// initialize used registers
	cpu->reg_a = 254;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ABS);

	// deassert RDY - cpu should halt
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_HALF_CYCLE_START
		SIGNAL_SET_BOOL(rdy, ACTHI_DEASSERT);
	CPU_CYCLE_END
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);

	CPU_HALF_CYCLE_START
		SIGNAL_SET_BOOL(rdy, ACTHI_DEASSERT);
	CPU_CYCLE_END
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);

	// stop deassering RDY (default == asserted) - cpu continues operation

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(2);
	munit_assert_uint8(cpu->reg_a, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_adc(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: immediate operand
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 13;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	munit_assert_true(SIGNAL_NEXT_BOOL(sync));
	DATABUS_WRITE(OP_6502_ADC_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_IMM);
	munit_assert_false(SIGNAL_NEXT_BOOL(sync));

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(15);
	munit_assert_uint8(cpu->reg_a, ==, 28);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: zero page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 13;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(15);
	munit_assert_uint8(cpu->reg_a, ==, 29);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: zero page, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 254;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(6);
	munit_assert_uint8(cpu->reg_a, ==, 5);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	
	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 254;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(2);
	munit_assert_uint8(cpu->reg_a, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 5;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(7);
	munit_assert_uint8(cpu->reg_a, ==, 12);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 127;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(2);
	munit_assert_uint8(cpu->reg_a, ==, (uint8_t) -127);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 5;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(-3);
	munit_assert_uint8(cpu->reg_a, ==, 2);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 5;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(-7);
	munit_assert_uint8(cpu->reg_a, ==, (uint8_t) -2);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = -5;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	DATABUS_WRITE(-7);
	munit_assert_uint8(cpu->reg_a, ==, -12);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = -66;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the y register
	cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3026);
	DATABUS_WRITE(-65);
	munit_assert_uint8(cpu->reg_a, ==, 125);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = -66;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the y register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	DATABUS_WRITE(66);
	munit_assert_uint8(cpu->reg_a, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ADC: decimal addition
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = BCD_BYTE(7, 9); 
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_SET(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ADC_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(BCD_BYTE(1, 4));
	munit_assert_uint8(cpu->reg_a, ==, BCD_BYTE(9, 3));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_ADC_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	DATABUS_WRITE(BCD_BYTE(2, 1));
	munit_assert_uint8(cpu->reg_a, ==, BCD_BYTE(1, 4));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	cpu->reg_a = BCD_BYTE(9, 9); 
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0805);
	DATABUS_WRITE(OP_6502_ADC_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ADC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0806);
	DATABUS_WRITE(BCD_BYTE(9, 9));
	munit_assert_uint8(cpu->reg_a, ==, BCD_BYTE(9, 8));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_and(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: immediate operand
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: zero page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: zero page, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3026);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_AND_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_AND_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000010);

	/////////////////////////////////////////////////////////////////////////////
	//
	// AND: test interaction with flags
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_AND_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b00001111);

	munit_assert_uint8(cpu->reg_a, ==, 0b00000011);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_AND_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b00111100);

	munit_assert_uint8(cpu->reg_a, ==, 0x00);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 1));		// zero flag
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_AND_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b10001111);

	munit_assert_uint8(cpu->reg_a, ==, 0b10000011);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 1));	// zero flag
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}

MunitResult test_asl(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: accumulator + effect on flags
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b10100000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ASL_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_ASL_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b01000000);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 03: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_ASL_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_ASL_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000000);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 05: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_ASL_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	DATABUS_WRITE(OP_6502_ASL_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b00000000);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: zero-page addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ASL_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x65);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	DATABUS_WRITE(0b10101010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 04: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	CPU_CYCLE();

	// >> cycle 05: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b01010100);
	CPU_HALF_CYCLE();
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: zero-page addressing, x indexed
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ASL_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	DATABUS_WRITE(0b01010101);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b10101010);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: absolute addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ASL_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10101010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b01010100);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ASL_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0b01010101);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b10101010);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ASL: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ASL_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ASL_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0b01010101);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b10101010);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_bcc(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCC: branch not taken
	//

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BCC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BCC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCC: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BCC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BCC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCC: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BCC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BCC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bcs(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCS: branch not taken
	//

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BCS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BCS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCS: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BCS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BCS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BCS: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BCS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BCS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_beq(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BEQ: branch not taken
	//

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_ZERO_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BEQ);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BEQ);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BEQ: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BEQ);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BEQ);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BEQ: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BEQ);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BEQ);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bit(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BIT: zero page addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 0b00000010;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BIT_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BIT_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(0b01001111);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// takes value of bit 7
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));				// takes value of bit 6
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if memory & reg-a == 0

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BIT: absolute addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 0b00100000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BIT_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BIT_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10011111);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// takes value of bit 7
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));				// takes value of bit 6
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if memory & reg-a == 0

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}


MunitResult test_bmi(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BMI: branch not taken
	//

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BMI);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BMI);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BMI: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BMI);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BMI);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BMI: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BMI);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BMI);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bne(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BNE: branch not taken
	//

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BNE);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BNE);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BNE: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_ZERO_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BNE);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BNE);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BNE: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_ZERO_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BNE);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BNE);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bpl(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BPL: branch not taken
	//

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BPL);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BPL);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BPL: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BPL);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BPL);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BPL: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BPL);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BPL);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_brk(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BRK: force interrupt
	//

	// initialize registers
	cpu->reg_p = 0;
	cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BRK);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 02: force BRK instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BRK);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BRK);

	// >> cycle 03: push program counter - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x08);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: push program counter - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x02);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: push processor satus
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fd);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, cpu->reg_p);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfc);

	// >> cycle 06: fetch vector low
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xfffe);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	DATABUS_WRITE(0x50);

	// >> cycle 07: fetch vector high
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xffff);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	DATABUS_WRITE(0x00);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0050);

	return MUNIT_OK;
}

MunitResult test_bvc(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVC: branch not taken
	//

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_OVERFLOW);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BVC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BVC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVC: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_OVERFLOW);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BVC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BVC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVC: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_OVERFLOW);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BVC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BVC);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_bvs(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVS: branch not taken
	//

	// initialize registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_OVERFLOW);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BVS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BVS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVS: branch taken, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_OVERFLOW);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BVS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BVS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0853);

	/////////////////////////////////////////////////////////////////////////////
	//
	// BVS: branch taken, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	FLAG_SET(cpu->reg_p, FLAG_6502_OVERFLOW);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_BVS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_BVS);

	// >> cycle 02: fetch relative address
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(-0x50);

	// >> cycle 03: add offset
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();

	// >> cycle 04: handle page crossing
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x08b3);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x07b3);

	return MUNIT_OK;
}

MunitResult test_clc(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLC: clear carry
	//

	// initialize registers
	cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CLC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CLC);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_uint8(cpu->reg_p, ==, 0b11111110);

	return MUNIT_OK;
}

MunitResult test_cld(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLC: clear decimal mode
	//

	// initialize registers
	cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CLD);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CLD);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_DECIMAL_MODE));
	munit_assert_uint8(cpu->reg_p, ==, 0b11110111);

	return MUNIT_OK;
}

MunitResult test_cli(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLI: clear interrupt disable bit
	//

	// initialize registers
	cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CLI);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CLI);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_INTERRUPT_DISABLE));
	munit_assert_uint8(cpu->reg_p, ==, 0b11111011);

	return MUNIT_OK;
}

MunitResult test_clv(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CLV: clear overflow flag
	//

	// initialize registers
	cpu->reg_p = 0b11111111;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CLV);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CLV);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_uint8(cpu->reg_p, ==, 0b10111111);

	return MUNIT_OK;
}

MunitResult test_cmp(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: immediate operand
	//

	// initialize registers
	cpu->reg_a = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(10);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: zero page addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(15);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: zero page, x indexed
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	DATABUS_WRITE(5);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(10);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(16);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(5);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(10);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(20);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the x register
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	DATABUS_WRITE(6);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the y register
	cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3026);
	DATABUS_WRITE(10);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CMP: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 10;

	// force the value of the y register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CMP_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CMP_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	DATABUS_WRITE(11);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	return MUNIT_OK;
}

MunitResult test_cpx(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPX: immediate operand
	//

	// initialize registers
	cpu->reg_x = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CPX_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CPX_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(10);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPX: zero page addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_x = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CPX_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CPX_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(15);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPX: absolute addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_x = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CPX_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CPX_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(7);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_cpy(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPY: immediate operand
	//

	// initialize registers
	cpu->reg_y = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CPY_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CPY_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(10);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPY: zero page addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_y = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CPY_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CPY_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(15);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// CPY: absolute addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_y = 10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_CPY_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_CPY_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(7);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));			// set if equal
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));		// set if memory > reg-a
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));					// set if memory <= reg-a

	// next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_dec(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: zero-page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_DEC_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x65);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	DATABUS_WRITE(1);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 04: perform decrement
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	CPU_CYCLE();

	// >> cycle 05: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: zero-page addressing, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_DEC_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEC_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	DATABUS_WRITE(0);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform decrement
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE()
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, (uint8_t) -1);
	CPU_HALF_CYCLE()
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_DEC_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEC_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(15);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform decrement
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 14);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_DEC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(1);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEC: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_DEC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(5);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 4);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_dex(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEX: clear interrupt disable bit
	//

	// initialize registers
	cpu->reg_p = 0;
	cpu->reg_x = 2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_DEX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_x, ==, 1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_DEX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_x, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_DEX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_x, ==, (uint8_t) -1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_dey(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// DEY: clear interrupt disable bit
	//

	// initialize registers
	cpu->reg_p = 0;
	cpu->reg_y = 2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_DEY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_y, ==, 1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_DEY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_y, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_DEY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_DEY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_y, ==, (uint8_t) -1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_eor(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: immediate operand
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: zero page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: zero page, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3026);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_EOR_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_EOR_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b01101001);

	/////////////////////////////////////////////////////////////////////////////
	//
	// EOR: test interaction with flags
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_EOR_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b10001111);

	munit_assert_uint8(cpu->reg_a, ==, 0b01001100);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_EOR_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b11000011);

	munit_assert_uint8(cpu->reg_a, ==, 0x00);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_EOR_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b00001111);

	munit_assert_uint8(cpu->reg_a, ==, 0b11001100);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_inc(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: zero-page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_INC_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x65);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	DATABUS_WRITE(1);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 04: perform increment
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	CPU_CYCLE();

	// >> cycle 05: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 2);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: zero-page addressing, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_INC_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INC_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	DATABUS_WRITE(0xff);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform increment
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_INC_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INC_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(127);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform decrement
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x80);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_INC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(1);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform increment
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 2);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// INC: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_p = 0;
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_INC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(5);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform increment
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 6);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_inx(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// INX: clear interrupt disable bit
	//

	// initialize registers
	cpu->reg_p = 0;
	cpu->reg_x = (uint8_t) -2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_INX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_x, ==, (uint8_t) -1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_INX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_x, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_INX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INX);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_x, ==, 1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_iny(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// INY: clear interrupt disable bit
	//

	// initialize registers
	cpu->reg_p = 0;
	cpu->reg_y = (uint8_t) -2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_INY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_y, ==, (uint8_t) -1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_INY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_y, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_INY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_INY);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_y, ==, 1);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_jmp(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// JMP: absolute addressing
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_JMP_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_JMP_ABS);

	// >> cycle 02: fetch jump-address, low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: fetch jump-address, high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0x09);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0951);

	/////////////////////////////////////////////////////////////////////////////
	//
	// JMP: indirect addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_JMP_IND);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_JMP_IND);

	// >> cycle 02: fetch indirect-address, low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x10);

	// >> cycle 03: fetch indirect-address, high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0x0C);

	// >> cycle 04: fetch jump-address, low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0C10);
	DATABUS_WRITE(0x51);

	// >> cycle 05: fetch jump-address, high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0C11);
	DATABUS_WRITE(0x09);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0951);

	return MUNIT_OK;
}

MunitResult test_jsr(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// JSR
	//

	// init registers
	cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_JSR);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_JSR);

	// >> cycle 02: fetch jump-address, low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: store address in cpu for later use
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	CPU_CYCLE();

	// >> cycle 04: push program counter - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x08);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfe);

	// >> cycle 05: push program counter - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	CPU_HALF_CYCLE();
	munit_assert_int8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x03);
	CPU_HALF_CYCLE();
	munit_assert_int8(cpu->reg_sp, ==, 0xfd);

	// >> cycle 06: fetch jump-address, high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	DATABUS_WRITE(0x09);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0951);

	return MUNIT_OK;
}

MunitResult test_lda(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: immediate operand
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x01);
	munit_assert_uint8(cpu->reg_a, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: zero page addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(0x05);
	munit_assert_uint8(cpu->reg_a, ==, 0x05);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: zero page, x indexed
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(0x03);
	munit_assert_uint8(cpu->reg_a, ==, 0x03);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0x07);
	munit_assert_uint8(cpu->reg_a, ==, 0x07);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x09);
	munit_assert_uint8(cpu->reg_a, ==, 0x09);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0x11);
	munit_assert_uint8(cpu->reg_a, ==, 0x11);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(0x13);
	munit_assert_uint8(cpu->reg_a, ==, 0x13);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(0x16);
	munit_assert_uint8(cpu->reg_a, ==, 0x16);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	DATABUS_WRITE(0x19);
	munit_assert_uint8(cpu->reg_a, ==, 0x19);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// force the value of the y register
	cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3026);
	DATABUS_WRITE(0x21);
	munit_assert_uint8(cpu->reg_a, ==, 0x21);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// force the value of the y register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDA_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	DATABUS_WRITE(0x23);
	munit_assert_uint8(cpu->reg_a, ==, 0x23);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDA: test interaction with flags
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDA_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x01);

	munit_assert_uint8(cpu->reg_a, ==, 0x01);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 1));	// zero flag
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDA_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x00);

	munit_assert_uint8(cpu->reg_a, ==, 0x00);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 1));		// zero flag
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDA_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x81);

	munit_assert_uint8(cpu->reg_a, ==, 0x81);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 1));	// zero flag
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}

MunitResult test_ldx(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: immediate operand
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDX_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDX_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x01);
	munit_assert_uint8(cpu->reg_x, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: zero page 
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDX_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDX_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x2a);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x002a);
	DATABUS_WRITE(0x01);
	munit_assert_uint8(cpu->reg_x, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: zero page, y indexed
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDX_ZPY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDX_ZPY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and y-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(0x03);
	munit_assert_uint8(cpu->reg_x, ==, 0x03);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: absolute addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDX_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDX_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0x07);
	munit_assert_uint8(cpu->reg_x, ==, 0x07);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDX_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDX_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(0x13);
	munit_assert_uint8(cpu->reg_x, ==, 0x13);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDX_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDX_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(0x16);
	munit_assert_uint8(cpu->reg_x, ==, 0x16);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDX: test interaction with flags
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDX_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x01);

	munit_assert_uint8(cpu->reg_x, ==, 0x01);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 1));	// zero flag
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDX_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x00);

	munit_assert_uint8(cpu->reg_x, ==, 0x00);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 1));		// zero flag
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDX_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x81);

	munit_assert_uint8(cpu->reg_x, ==, 0x81);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 1));	// zero flag
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}


MunitResult test_ldy(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: immediate operand
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDY_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDY_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x01);
	munit_assert_uint8(cpu->reg_y, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: zero page 
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDY_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDY_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x2a);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x002a);
	DATABUS_WRITE(0x01);
	munit_assert_uint8(cpu->reg_y, ==, 0x01);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: zero page, x indexed
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDY_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDY_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(0x03);
	munit_assert_uint8(cpu->reg_y, ==, 0x03);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: absolute addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDY_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDY_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0x07);
	munit_assert_uint8(cpu->reg_y, ==, 0x07);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDY_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDY_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(0x13);
	munit_assert_uint8(cpu->reg_y, ==, 0x13);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LDY_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LDY_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(0x16);
	munit_assert_uint8(cpu->reg_y, ==, 0x16);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LDY: test interaction with flags
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDY_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x01);

	munit_assert_uint8(cpu->reg_y, ==, 0x01);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 1));	// zero flag
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDY_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x00);

	munit_assert_uint8(cpu->reg_y, ==, 0x00);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 1));		// zero flag
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_LDY_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0x81);

	munit_assert_uint8(cpu->reg_y, ==, 0x81);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	munit_assert_false(BIT_IS_SET(cpu->reg_p, 1));	// zero flag
	munit_assert_true(BIT_IS_SET(cpu->reg_p, 7));	// negative result

	return MUNIT_OK;
}

MunitResult test_lsr(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: accumulator + effect on flags
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b00000101;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LSR_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_LSR_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b00000010);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 03: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_LSR_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_LSR_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b00000001);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 05: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_LSR_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	DATABUS_WRITE(OP_6502_LSR_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b00000000);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: zero-page addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LSR_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x65);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	DATABUS_WRITE(0b10001010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 04: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	CPU_CYCLE();

	// >> cycle 05: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b01000101);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: zero-page addressing, x indexed
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LSR_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	DATABUS_WRITE(0b01010011);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b00101001);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: absolute addressing
	//

	cpu_reset(cpu);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LSR_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10000010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b01000001);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LSR_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0b00010000);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b00001000);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// LSR: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_LSR_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_LSR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0b01000001);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b00100000);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_nop(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// NOP
	//

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_NOP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_NOP);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_ora(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: immediate operand
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: zero page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: zero page, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the x register
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3026);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b11000011;

	// force the value of the y register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ORA_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ORA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	DATABUS_WRITE(0b10101010);
	munit_assert_uint8(cpu->reg_a, ==, 0b11101011);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ORA: test interaction with flags
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b01000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_ORA_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b00001111);

	munit_assert_uint8(cpu->reg_a, ==, 0b01001111);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// initialize used registers
	cpu->reg_a = 0b00000000;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_ORA_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b00000000);

	munit_assert_uint8(cpu->reg_a, ==, 0x00);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// initialize used registers
	cpu->reg_a = 0b01000011;

	// >> cycle 01: fetch opcode
	DATABUS_WRITE(OP_6502_ORA_IMM);

	// >> cycle 02: fetch operand + execute
	DATABUS_WRITE(0b10001100);

	munit_assert_uint8(cpu->reg_a, ==, 0b11001111);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_pha(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PHA
	//

	// init registers
	cpu->reg_a  = 0x13;
	cpu->reg_p  = 0b10010101;
	cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_PHA);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_PHA);

	// >> cycle 02: fetch discard - decode pha
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: write a on the stack
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x13);
	CPU_HALF_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_php(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PHP
	//

	// init registers
	cpu->reg_a  = 0x13;
	cpu->reg_p  = 0b10010101;
	cpu->reg_sp = 0xff;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_PHP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_PHP);

	// >> cycle 02: fetch discard - decode php
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: write a on the stack
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b10010101);
	CPU_HALF_CYCLE();

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_pla(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PLA
	//

	// init registers
	cpu->reg_sp = 0xfe;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_PLA);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_PLA);

	// >> cycle 02: fetch discard - decode pla
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: read stack - increment stack pointer
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	DATABUS_WRITE(0x23);
	munit_assert_uint8(cpu->reg_sp, ==, 0xff);

	// >> cycle 04: fetch reg-a
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	DATABUS_WRITE(0x19);
	munit_assert_uint8(cpu->reg_a, ==, 0x19);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_plp(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// PLP
	//

	// init registers
	cpu->reg_sp = 0xfe;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_PLP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_PLP);

	// >> cycle 02: fetch discard - decode plp
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: read stack - increment stack pointer
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	DATABUS_WRITE(0x23);
	munit_assert_uint8(cpu->reg_sp, ==, 0xff);

	// >> cycle 04: fetch reg-a
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	DATABUS_WRITE(0x79);
	munit_assert_uint8(cpu->reg_p, ==, 0x79);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);

	return MUNIT_OK;
}

MunitResult test_rol(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: accumulator + effect on flags
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b10100000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROL_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_ROL_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b01000000);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 03: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_ROL_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_ROL_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000001);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 05: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_ROL_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	DATABUS_WRITE(OP_6502_ROL_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b00000010);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: zero-page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROL_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x65);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	DATABUS_WRITE(0b10101010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 04: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	CPU_CYCLE();

	// >> cycle 05: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b01010101);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: zero-page addressing, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_x = 0xf0;
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROL_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	DATABUS_WRITE(0b01010101);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b10101010);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROL_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10101010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b01010100);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROL_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0b01010101);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b10101010);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROL: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROL_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROL_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0b01010101);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b10101010);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_ror(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: accumulator + effect on flags
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 0b00000101;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROR_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ACC);

	// >> cycle 02: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_ROR_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b00000010);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 03: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(OP_6502_ROR_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ACC);

	// >> cycle 04: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_ROR_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b10000001);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 05: fetch operand
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_ROR_ACC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ACC);

	// >> cycle 06: execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	DATABUS_WRITE(OP_6502_ROR_ACC);
	munit_assert_uint8(cpu->reg_a, ==, 0b01000000);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: zero-page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROR_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x65);

	// >> cycle 03: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	DATABUS_WRITE(0b10001010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 04: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	CPU_CYCLE();

	// >> cycle 05: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0065);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b11000101);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: zero-page addressing, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROR_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	DATABUS_WRITE(0b01010011);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x003a);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b00101001);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROR_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(0b10000010);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 05: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b01000001);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROR_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0b00010000);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b00001000);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	/////////////////////////////////////////////////////////////////////////////
	//
	// ROR: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_CARRY);
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_ROR_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_ROR_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(0b01000001);
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));

	// >> cycle 06: perform rotate
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	CPU_CYCLE();

	// >> cycle 06: write result set flags
	CPU_HALF_CYCLE();
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0b00100000);
	CPU_HALF_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(rw));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);

	return MUNIT_OK;
}

MunitResult test_rts(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// RTS
	//

	// init registers
	cpu->reg_sp = 0xfd;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_RTS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_RTS);

	// >> cycle 02: fetch discard - decode rts
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: increment stack pointer
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fd);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_sp, ==, 0xfe);

	// >> cycle 04: pop program counter - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	DATABUS_WRITE(0x51);

	// >> cycle 05: pop program counter - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	DATABUS_WRITE(0x09);

	// >> cycle 06: increment pc
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0951);
	DATABUS_WRITE(0x12);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0952);

	return MUNIT_OK;
}

MunitResult test_rti(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// RTI
	//

	// init registers
	cpu->reg_sp = 0xfc;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_RTI);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_RTI);

	// >> cycle 02: fetch discard - decode rts
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x51);

	// >> cycle 03: increment stack pointer
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fc);
	CPU_CYCLE();
	munit_assert_uint8(cpu->reg_sp, ==, 0xfd);

	// >> cycle 05: pop processor status
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fd);
	DATABUS_WRITE(0x00);
	munit_assert_uint8(cpu->reg_p, ==, 0x00);

	// >> cycle 05: pop program counter - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01fe);
	DATABUS_WRITE(0x51);

	// >> cycle 06: pop program counter - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x01ff);
	DATABUS_WRITE(0x09);

	// >> next instruction
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0951);

	return MUNIT_OK;
}

MunitResult test_sbc(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// true: immediate operand
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 13;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(10);
	munit_assert_uint8(cpu->reg_a, ==, 3);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: zero page addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 13;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	DATABUS_WRITE(15);
	munit_assert_uint8(cpu->reg_a, ==, (uint8_t) -2);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: zero page, x indexed
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 254;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	DATABUS_WRITE(6);
	munit_assert_uint8(cpu->reg_a, ==, 248);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	
	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 2;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	DATABUS_WRITE(2);
	munit_assert_uint8(cpu->reg_a, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 5;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(7);
	munit_assert_uint8(cpu->reg_a, ==, -2);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = -120;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	DATABUS_WRITE(10);
	munit_assert_uint8(cpu->reg_a, ==, 126);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 5;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the y register
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc019);
	DATABUS_WRITE(-3);
	munit_assert_uint8(cpu->reg_a, ==, 8);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SSB: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = 5;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_y = 0xf3;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc009);
	DATABUS_WRITE(0x15);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc109);
	DATABUS_WRITE(-7);
	munit_assert_uint8(cpu->reg_a, ==, 12);
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = -5;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the x register
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	DATABUS_WRITE(-7);
	munit_assert_uint8(cpu->reg_a, ==, 2);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = -66;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the y register
	cpu->reg_y = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3026);
	DATABUS_WRITE(65);
	munit_assert_uint8(cpu->reg_a, ==, 125);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = -66;
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_CLEAR(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// force the value of the y register
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	DATABUS_WRITE(-66);
	munit_assert_uint8(cpu->reg_a, ==, 0);
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_OVERFLOW));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	/////////////////////////////////////////////////////////////////////////////
	//
	// SBC: decimal subtraction
	//

	cpu_reset(cpu);

	// initialize used registers
	cpu->reg_a = BCD_BYTE(4, 4); 
	FLAG_SET(cpu->reg_p, FLAG_6502_CARRY);
	FLAG_SET(cpu->reg_p, FLAG_6502_DECIMAL_MODE);

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SBC_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(BCD_BYTE(2, 9));
	munit_assert_uint8(cpu->reg_a, ==, BCD_BYTE(1, 5));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(OP_6502_SBC_IMM);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SBC_IMM);

	// >> cycle 02: fetch operand + execute
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0804);
	DATABUS_WRITE(BCD_BYTE(2, 9));
	munit_assert_uint8(cpu->reg_a, ==, BCD_BYTE(8, 6));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_sec(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// SEC: set carry
	//

	// initialize registers
	cpu->reg_p = 0b00000000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SEC);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SEC);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY));
	munit_assert_uint8(cpu->reg_p, ==, 0b00000001);

	return MUNIT_OK;
}

MunitResult test_sed(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// SED: set decimal flag
	//

	// initialize registers
	cpu->reg_p = 0b00000000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SED);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SED);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_DECIMAL_MODE));
	munit_assert_uint8(cpu->reg_p, ==, 0b00001000);

	return MUNIT_OK;
}

MunitResult test_sei(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// SEI: set interrupt disable status
	//

	// initialize registers
	cpu->reg_p = 0b00000000;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_SEI);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_SEI);

	// >> cycle 02: execute opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	CPU_CYCLE();
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_INTERRUPT_DISABLE));
	munit_assert_uint8(cpu->reg_p, ==, 0b00000100);

	return MUNIT_OK;
}

MunitResult test_sta(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: zero page addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_a = 0x63;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x63);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: zero page, x indexed
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x65;
	cpu->reg_x = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x65);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x67;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x67);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing x-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x67;
	cpu->reg_x = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x67);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing x-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x67;
	cpu->reg_x = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_ABSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_ABSX);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x67);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing y-indexed, no page crossing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x69;
	cpu->reg_y = 0x02;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	DATABUS_WRITE(0x10);

	// >> cycle 05: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc018);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x69);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: absolute addressing y-indexed, page crossing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x69;
	cpu->reg_y = 0xf2;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_ABSY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_ABSY);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc008);
	DATABUS_WRITE(0x10);

	// >> cycle 05: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc108);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x69);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: indexed indirect addressing (index-x)
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x71;
	cpu->reg_x = 0x10;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_INDX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_INDX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add zp + index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x11);

	// >> cycle 04: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005a);
	DATABUS_WRITE(0x20);

	// >> cycle 05: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x005b);
	DATABUS_WRITE(0x21);

	// >> cycle 06: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x2120);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x71);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: indirect indexed addressing (index-y), no page crossing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x73;
	cpu->reg_y = 0x03;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3019);
	DATABUS_WRITE(0x17);

	// >> cycle 06: store value to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3019);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x73);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STA: indirect indexed addressing (index-y), page crossing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_a = 0x73;
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STA_INDY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STA_INDY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: fetch address low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x16);

	// >> cycle 04: fetch address high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004b);
	DATABUS_WRITE(0x30);

	// >> cycle 05: add carry
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3006);
	DATABUS_WRITE(0x17);

	// >> cycle 06: store value to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x3106);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x73);
	CPU_HALF_CYCLE();

	return MUNIT_OK;
}

MunitResult test_stx(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// STX: zero page addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_x = 0x51;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STX_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STX_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x51);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STX: zero page, y indexed
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_x = 0x53;
	cpu->reg_y = 0xf0;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STX_ZPY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STX_ZPY);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x53);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STX: absolute addressing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_x = 0x55;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STX_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STX_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x55);
	CPU_HALF_CYCLE();

	return MUNIT_OK;
}

MunitResult test_sty(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// STY: zero page addressing
	//

	cpu_reset(cpu);

	// initialize registers
	cpu->reg_y = 0x41;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STY_ZP);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STY_ZP);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfe);

	// >> cycle 03: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x00fe);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x41);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STY: zero page, x indexed
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_x = 0xf0;
	cpu->reg_y = 0x43;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STY_ZPX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STY_ZPX);

	// >> cycle 02: fetch zero page address 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x4a);

	// >> cycle 03: add addres and x-index
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x004a);
	DATABUS_WRITE(0x21);

	// >> cycle 04: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, (0x4a + 0xf0) & 0xff);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x43);
	CPU_HALF_CYCLE();

	/////////////////////////////////////////////////////////////////////////////
	//
	// STY: absolute addressing
	//

	cpu_reset(cpu);

	// force the value of the used registers
	cpu->reg_y = 0x45;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_STY_ABS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_STY_ABS);

	// >> cycle 02: fetch address - low byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0x16);

	// >> cycle 03: fetch address - high byte
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0803);
	DATABUS_WRITE(0xc0);

	// >> cycle 04: store to memory
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0xc016);
	munit_assert_false(SIGNAL_NEXT_BOOL(rw));				// writing
	CPU_HALF_CYCLE();
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x45);
	CPU_HALF_CYCLE();

	return MUNIT_OK;
}

MunitResult test_tax(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TAX: transfer accumulator to index X
	//

	cpu_reset(cpu);

	// force values of the used registers
	cpu->reg_a = 0x27;
	cpu->reg_x = 0x00;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_TAX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_TAX);

	// >> cycle 02: execute 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfc);
	munit_assert_uint8(cpu->reg_x, ==, 0x27);

	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_tsx(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TSX: transfer stack pointer to index X
	//

	cpu_reset(cpu);

	// force values of the used registers
	cpu->reg_sp = 0x8e;
	cpu->reg_x = 0x00;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_TSX);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_TSX);

	// >> cycle 02: execute 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfc);
	munit_assert_uint8(cpu->reg_x, ==, 0x8e);

	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_true(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));
	return MUNIT_OK;
}

MunitResult test_tay(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TAY: transfer accumulator to index Y
	//

	cpu_reset(cpu);

	// force values of the used registers
	cpu->reg_a = 0x27;
	cpu->reg_y = 0x00;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_TAY);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_TAY);

	// >> cycle 02: execute 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfc);
	munit_assert_uint8(cpu->reg_y, ==, 0x27);

	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_txa(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TXA: transfer index X to accumulator 
	//

	cpu_reset(cpu);

	// force values of the used registers
	cpu->reg_a = 0x00;
	cpu->reg_x = 0x27;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_TXA);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_TXA);

	// >> cycle 02: execute 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfc);
	munit_assert_uint8(cpu->reg_a, ==, 0x27);

	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

	return MUNIT_OK;
}

MunitResult test_txs(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TXS: transfer index X to stack pointer
	//

	cpu_reset(cpu);

	// force values of the used registers
	cpu->reg_sp = 0x00;
	cpu->reg_x = 0x8a;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_TXS);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_TXS);

	// >> cycle 02: execute 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfc);
	munit_assert_uint8(cpu->reg_sp, ==, 0x8a);

	return MUNIT_OK;
}

MunitResult test_tya(const MunitParameter params[], void *user_data_or_fixture) {

	Cpu6502 *cpu = (Cpu6502 *) user_data_or_fixture;

	/////////////////////////////////////////////////////////////////////////////
	//
	// TYA: transfer index Y to accumulator
	//

	cpu_reset(cpu);

	// force values of the used registers
	cpu->reg_a = 0x00;
	cpu->reg_y = 0x27;

	// >> cycle 01: fetch opcode
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0801);
	DATABUS_WRITE(OP_6502_TYA);
	munit_assert_uint8(cpu->reg_ir, ==, OP_6502_TYA);

	// >> cycle 02: execute 
	munit_assert_uint16(SIGNAL_NEXT_UINT16(bus_address), ==, 0x0802);
	DATABUS_WRITE(0xfc);
	munit_assert_uint8(cpu->reg_a, ==, 0x27);

	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_ZERO_RESULT));
	munit_assert_false(FLAG_IS_SET(cpu->reg_p, FLAG_6502_NEGATIVE_RESULT));

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
