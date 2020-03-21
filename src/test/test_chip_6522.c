// test/test_chip_6522.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_6522.h"

#include "stb/stb_ds.h"

#define SIGNAL_POOL			via->signal_pool
#define SIGNAL_COLLECTION	via->signals

#define VIA_CYCLE_START				\
	for (int i = 0; i < 2; ++i) {
#define VIA_CYCLE_END				\
		half_clock_cycle(via);		\
	}

#define VIA_CYCLE()					\
	VIA_CYCLE_START					\
	VIA_CYCLE_END

typedef enum Chip6522_regs {
	REG_IFR		= 1 << 0,
	REG_IER		= 1 << 1,
	REG_PCR		= 1 << 2,
	REG_ACR		= 1 << 3,
	REG_T1L_L	= 1 << 4,
	REG_T1L_H	= 1 << 5,
	REG_T1C		= 1 << 6,
	REG_T2L_L	= 1 << 7,
	REG_T2C		= 1 << 8,
	REG_ILA		= 1 << 9,
	REG_ORA		= 1 << 10,
	REG_DDRA	= 1 << 11,
	REG_ILB		= 1 << 12,
	REG_ORB		= 1 << 13,
	REG_DDRB	= 1 << 14,
	REG_SR		= 1 << 15,
} Chip6522_regs;

#define ASSERT_REGISTER_DEFAULT(exclude, reg, val, l)	\
	if (((exclude) & (reg)) == 0) {						\
		munit_assert_uint##l((val), ==, 0);				\
	}

#define ASSERT_REGISTERS_DEFAULT(exclude)							\
	ASSERT_REGISTER_DEFAULT(exclude, REG_IFR,	via->reg_ifr, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_IER,	via->reg_ier, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_PCR,	via->reg_pcr, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_ACR,	via->reg_acr, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_T1L_L, via->reg_t1l_l, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_T1L_H, via->reg_t1l_h, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_T1C,	via->reg_t1c, 16)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_T2L_L, via->reg_t2l_l, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_T2C,	via->reg_t2c, 16)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_ILA,	via->reg_ila, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_ORA,	via->reg_ora, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_DDRA,	via->reg_ddra, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_ILB,	via->reg_ilb, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_ORB,	via->reg_orb, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_DDRB,	via->reg_ddrb, 8)	\
	ASSERT_REGISTER_DEFAULT(exclude, REG_SR,	via->reg_sr, 8)


static void *chip_6522_setup(const MunitParameter params[], void *user_data) {
	Chip6522 *via = chip_6522_create(signal_pool_create(), (Chip6522Signals) {0});

	SIGNAL_SET_BOOL(enable, false);
	SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	signal_pool_cycle(via->signal_pool);
	chip_6522_process(via);
	return via;
}

static void chip_6522_teardown(void *fixture) {
	chip_6522_destroy((Chip6522 *) fixture);
}

static inline void strobe_via(Chip6522 *via, bool strobe) {
	SIGNAL_SET_BOOL(cs1, strobe);
	SIGNAL_SET_BOOL(cs2_b, !strobe);
}

static inline void half_clock_cycle(Chip6522 *via) {
	SIGNAL_SET_BOOL(enable, !SIGNAL_BOOL(enable));
	signal_pool_cycle(via->signal_pool);
	chip_6522_process(via);
}

static MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// force registers to junk values
	via->reg_ifr = 0xde;
	via->reg_ier = 0xad;
	via->reg_pcr = 0xbe;
	via->reg_acr = 0xef;
	via->reg_t1l_l = 0xde;
	via->reg_t1l_h = 0xad;
	via->reg_t1c = 0xbe;
	via->reg_t2l_l = 0xef;
	via->reg_t2c = 0xde;
	via->reg_ila = 0xad;
	via->reg_ora = 0xbe;
	via->reg_ddra = 0xef;
	via->reg_ilb = 0xde;
	via->reg_orb = 0xad;
	via->reg_ddrb = 0xbe;
	via->reg_sr = 0xef;

	// run via with reset asserted
	SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	signal_pool_cycle(via->signal_pool);
	chip_6522_process(via);

	// registers should have been cleared (except T1, T2 and SR)
	munit_assert_uint8(via->reg_ifr, ==, 0);
	munit_assert_uint8(via->reg_ier, ==, 0);
	munit_assert_uint8(via->reg_pcr, ==, 0);
	munit_assert_uint8(via->reg_acr, ==, 0);

	munit_assert_uint8(via->reg_ila, ==, 0);
	munit_assert_uint8(via->reg_ora, ==, 0);
	munit_assert_uint8(via->reg_ddra, ==, 0);

	munit_assert_uint8(via->reg_ilb, ==, 0);
	munit_assert_uint8(via->reg_orb, ==, 0);
	munit_assert_uint8(via->reg_ddrb, ==, 0);

	munit_assert_uint8(via->reg_t1l_l, ==, 0xde);
	munit_assert_uint8(via->reg_t1l_h, ==, 0xad);
	munit_assert_uint16(via->reg_t1c, ==, 0xbe);
	munit_assert_uint8(via->reg_t2l_l, ==, 0xef);
	munit_assert_uint16(via->reg_t2c, ==, 0xde);

	munit_assert_uint8(via->reg_sr, ==, 0xef);

	return MUNIT_OK;
}

static MunitResult test_read_ddra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ddra = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddra, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddra, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_ddrb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ddrb = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddrb, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddrb, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_ddra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the DDRA register
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x09);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddra, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRA);

	// disable via
	strobe_via(via, false);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_ddra, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRA);

	return MUNIT_OK;
}

static MunitResult test_write_ddrb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the DDRA register
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x09);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddrb, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRB);

	// disable via
	strobe_via(via, false);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_ddrb, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRB);

	return MUNIT_OK;
}

static MunitResult test_read_acr(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_acr = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_acr(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the ACR register
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x09);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_ACR);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_acr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_ACR);

	return MUNIT_OK;
}

static MunitResult test_read_pcr(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_pcr = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_pcr(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the PCR register
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x09);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_PCR);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_pcr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_PCR);

	return MUNIT_OK;
}

static MunitResult test_write_ora(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the ORA register
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ora, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORA);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_ora, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORA);

	return MUNIT_OK;
}

static MunitResult test_write_orb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the ORA register
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_orb, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORB);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_orb, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORB);

	return MUNIT_OK;
}

static MunitResult test_porta_out(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ora = 0xf5;
	via->reg_ddra = 0xff;		// all pins configured as output

	// cycle clock ; entire ora-register should have been written to port-A
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x12);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_a), ==, 0xf5);

	// reconfigure ddra, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8_MASKED(port_a, 0x09, 0x0f);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0xf0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// cycle clock
	VIA_CYCLE_START
		SIGNAL_SET_UINT8_MASKED(port_a, 0x09, 0x0f);
	VIA_CYCLE_END
	chip_6522_process(via);
	munit_assert_uint8(via->reg_ddra, ==, 0xf0);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_a), ==, 0xf9);

	return MUNIT_OK;
}

static MunitResult test_portb_out(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_orb = 0xf5;
	via->reg_ddrb = 0xff;		// all pins configured as output

	// cycle clock ; entire ora-register should have been written to port-A
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0x12);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_b), ==, 0xf5);

	// reconfigure ddrb, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8_MASKED(port_b, 0x09, 0x0f);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0xf0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// cycle clock
	VIA_CYCLE_START
		SIGNAL_SET_UINT8_MASKED(port_b, 0x09, 0x0f);
	VIA_CYCLE_END
	chip_6522_process(via);
	munit_assert_uint8(via->reg_ddrb, ==, 0xf0);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_b), ==, 0xf9);

	return MUNIT_OK;
}

static MunitResult test_read_porta_input(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-A while pins are configured as input
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ora = 0xf5;
	via->reg_ddra = 0x00;		// all pins configured as input

	// cycle clock
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x12);
	VIA_CYCLE_END

	// read from the port
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_a, 0x12);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x12);

	// enable latching on port-A (default == on negative edge)
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(bus_data, 0b00000001);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0b00000001);

	// start writing to port-A, keep CA1 high
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x3e);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// bring CA1 low, latching data on the port
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x3e);
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// bring CA1 high, change data on port, internal register shouldn't change
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x15);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// read from the port
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_UINT8(port_a, 0x12);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x3e);

	// read unlatched port - ila should follow port again
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x15);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x15);

	return MUNIT_OK;
}

static MunitResult test_read_porta_output(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-A while pins are configured as output - port-A reads values on the pins (or latched value)
// when all pins are configured as output one wouldn't normally expect external writes to the port_a signals,
// but the test does this to simulate pins being pulled down, thus returning a different value than is being output
// by the via-chip
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ora = 0xf5;
	via->reg_ddra = 0xff;		// all pins configured as output

	// read from the port
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xf5);

	// enable latching on port-A (default == on negative edge)
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(bus_data, 0b00000001);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0b00000001);

	// start writing to port-A, keep CA1 high
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x3e);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// bring CA1 low, latching data on the port
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x3e);
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// bring CA1 high, change data on port, internal register shouldn't change
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x15);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// read from the port
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_UINT8(port_a, 0x12);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x3e);

	// read unlatched port - ila should follow port again
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x15);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x15);

	return MUNIT_OK;
}

static MunitResult test_read_portb_input(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-A while pins are configured as input
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_orb = 0xf5;
	via->reg_ddrb = 0x00;		// all pins configured as input

	// cycle clock
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0x12);
	VIA_CYCLE_END

	// read from the port
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_b, 0x12);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x12);

	// enable latching on port-B (default == on negative edge)
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(bus_data, 0b00000010);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0b00000010);

	// start writing to port-B, keep CB1 high
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0x3e);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x3e);

	// bring CB1 low, latching data on the port
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0x3e);
		SIGNAL_SET_BOOL(cb1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x3e);

	// bring CB1 high, change data on port, internal register shouldn't change
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0x15);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x3e);

	// read from the port
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_UINT8(port_b, 0x12);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x3e);

	// read unlatched port - ilb should follow port again
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0x15);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x15);

	return MUNIT_OK;
}

static MunitResult test_read_portb_output(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-B while pins are configured as output - contrary to port-A: pin level has NO effect. Always reads ORB-register.
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_orb = 0xf5;
	via->reg_ddrb = 0xff;		// all pins configured as output

	// read from the port - no pins pulled down
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xf5);

	// read from the port - some pins pulled down
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_b, 0b11101010);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xf5);

	// reconfigure ddrb, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0xf0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// read from the port - some pins pulled down (only affect lower nibble)
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_b, 0b11101010);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xfa);

	return MUNIT_OK;
}

static MunitResult test_hs_porta_read(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// configure CA2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00001000);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x08);

	// assert CA1 to indicate there's available data
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
	VIA_CYCLE_END

	// another cycle
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END

	// read port-A - CA2 shoukd go low
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);

		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xab);
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// clock-cycle - CA2 stays low
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// clock-cycle - CA2 stays low
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// new data available on port-A: CA2 goes high
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xba);
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	return MUNIT_OK;
}

static MunitResult test_hs_porta_read_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// configure CA2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00001010);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x0a);

	// assert CA1 to indicate there's available data
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
	VIA_CYCLE_END

	// another cycle
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END

	// read port-A - CA2 shoukd go low
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);

		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xab);
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// clock-cycle - CA2 goes high again
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0xab);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	return MUNIT_OK;
}

static MunitResult test_hs_porta_write(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ddra = 0xff;		// all pins configured as output

	// configure CA2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00001000);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x08);

	// write data to port-A - CA2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_a), ==, 0x7f);

	// clock cycle - CA2 stays low
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// clock cycle - CA2 stays low
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// assert CA1 - CA2 goes high
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	return MUNIT_OK;
}

static MunitResult test_hs_porta_write_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ddra = 0xff;		// all pins configured as output

	// configure CA2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00001010);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x0a);

	// write data to port-A - CA2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_a), ==, 0x7f);

	// clock cycle - CA2 goes high again
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_read(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// configure CB2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b10000000);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x80);
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// assert CB1 to indicate there's available data
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0xab);
		SIGNAL_SET_BOOL(cb1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// another cycle
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0xab);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// read port-B - CB2 should not change - no read handshake on port-B
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_b, 0xab);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);

		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xab);
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_read_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// configure CB2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b10100000);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa0);
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// assert CB1 to indicate there's available data
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0xab);
		SIGNAL_SET_BOOL(cb1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// another cycle
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0xab);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// read port-B - CB2 should not change - no read handshake on port-B
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_UINT8(port_b, 0xab);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);

		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xab);
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_write(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ddrb = 0xff;		// all pins configured as output

	// configure CB2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b10000000);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x80);

	// write data to port-B - CB2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_b), ==, 0x7f);

	// clock cycle - CB2 stays low
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// clock cycle - CB2 stays low
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// assert CB1 - CB2 goes high
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_write_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ddrb = 0xff;		// all pins configured as output

	// configure CB2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b10100000);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa0);

	// write data to port-B - CB2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_b), ==, 0x7f);

	// clock cycle - CB2 goes high again
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	return MUNIT_OK;
}

static MunitResult test_read_ifr(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ifr = 0x15;

	// read the IFR register
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0x15);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x15);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0x15);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_ifr(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// setup registers
	via->reg_ifr = 0b10111011;
	via->reg_ier = 0b01111111;

	// clear bits 1 & 2
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00000110);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0b10111001);
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	ASSERT_REGISTERS_DEFAULT(REG_IFR | REG_IER);

	// clear all bits
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01111111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0);
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	ASSERT_REGISTERS_DEFAULT(REG_IFR | REG_IER);

	return MUNIT_OK;
}

static MunitResult test_read_ier(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// initialize registers
	via->reg_ier = 0x15;

	// read the Interrupt Enable register
	VIA_CYCLE_START
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0x15);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x95);		// bit 7 is always set to 1

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(via, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0x15);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_ier(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// set a few bits
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b10000110);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00000110);
	ASSERT_REGISTERS_DEFAULT(REG_IER);

	// set some more bits
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b10001100);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00001110);
	ASSERT_REGISTERS_DEFAULT(REG_IER);

	// clear a bit
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00000010);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00001100);

	// clear all the bits
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01111111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_irq_port(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// setup registers
	via->reg_ier = 0b00011011;	// enable

	///////////////////////////////////////////////////////////////////////////
	//
	// negative edge triggers interrupt (default)
	//

	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// negative edge triggers interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 and ca2 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011000);

	// read ORB: clears cb1 and cb2 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	///////////////////////////////////////////////////////////////////////////
	//
	// independent negative edge triggers interrupt
	//

	// configure PCR
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00100010);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// negative edge triggers interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011001);

	// read ORB: clears cb1 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10001001);

	// clear all the interrupts
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01111111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	///////////////////////////////////////////////////////////////////////////
	//
	// positive edge triggers interrupt
	//

	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTLO_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTLO_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01010101);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// clear all interrupts
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01111111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	// trigger interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 and ca2 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011000);

	// read ORB: clears cb1 and cb2 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	// clear all the interrupts
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01111111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	///////////////////////////////////////////////////////////////////////////
	//
	// independent positive edge triggers interrupt
	//

	// configure PCR
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01110111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// clear all interrupts
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01111111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	// positive edge triggers interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011001);

	// read ORB: clears cb1 interrupt
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10001001);

	// clear all the interrupts
	VIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01111111);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_once_nopb7(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00000000);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 144);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 1);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 1);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// run until timer ends
	for (int i = 400; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, i);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t1c, ==, 0xffff);
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b11000000);

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_freerun_nopb7(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b01000000);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 50);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	for (int j = 0; j < 3; ++j) {
		// clear t1 interrupt by reading t1c_l
		VIA_CYCLE_START
			strobe_via(via, true);					// enable via
			SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
			SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
			SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
			SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t1c, ==, 50);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
		munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

		// run until timer ends
		for (int i = 49; i >= 0; --i) {
			VIA_CYCLE();
			munit_assert_uint16(via->reg_t1c, ==, i);
			munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
		}

		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, 0xffff);
		munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
		munit_assert_uint8(via->reg_ifr, ==, 0b11000000);
	}

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_once_pb7(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6522 *via = (Chip6522 *) user_data_or_fixture;
	Signal pb7 = signal_split(SIGNAL(port_b), 7, 1);

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1
	via->reg_ddrb = 0b10000000;

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b10000000);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 144);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 1);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_false(signal_read_next_bool(via->signal_pool, pb7));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 1);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// run until timer ends
	for (int i = 400; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, i);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
		munit_assert_false(signal_read_next_bool(via->signal_pool, pb7));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t1c, ==, 0xffff);
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_true(signal_read_next_bool(via->signal_pool, pb7));
	munit_assert_uint8(via->reg_ifr, ==, 0b11000000);

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_freerun_pb7(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6522 *via = (Chip6522 *) user_data_or_fixture;
	Signal pb7 = signal_split(SIGNAL(port_b), 7, 1);
	bool val_pb7 = false;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1
	via->reg_ddrb = 0b10000000;

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b11000000);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 50);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);
	munit_assert(signal_read_next_bool(via->signal_pool, pb7) == val_pb7);

	for (int j = 0; j < 3; ++j) {

		// clear t1 interrupt by reading t1c_l
		VIA_CYCLE_START
			strobe_via(via, true);					// enable via
			SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
			SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
			SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
			SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t1c, ==, 50);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
		munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

		// run until timer ends
		for (int i = 49; i >= 0; --i) {
			VIA_CYCLE();
			munit_assert_uint16(via->reg_t1c, ==, i);
			munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
		}

		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, 0xffff);
		munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
		munit_assert_uint8(via->reg_ifr, ==, 0b11000000);

		// value of port_b[7] is inverted everytime the timer reaches zero
		val_pb7 = !val_pb7;
		munit_assert(signal_read_next_bool(via->signal_pool, pb7) == val_pb7);
	}

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t2_timed(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00100000;					// enable irq on timer2

	// setup t2-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00000000);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 10);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// run until timer ends
	for (int i = 10; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b10100000);

	// clear t2 interrupt by reading t2c_l
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);
	munit_assert_uint16(via->reg_t2c, ==, 0xfffe);

	// cycle until timer wraps again
	for (int i = 0xfffd; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));

	// write t2-h to re-enable irq
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);

	// run until timer ends
	for (int i = 10; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));

	return MUNIT_OK;
}

static MunitResult test_t2_count_pb6(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6522 *via = (Chip6522 *) user_data_or_fixture;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00100000;					// enable irq on timer2

	// setup t2-mode
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0b00100000);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 10);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0);
		SIGNAL_SET_BOOL(rw, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// cycle until timer is done
	for (int i = 10; i > 0; --i) {
		VIA_CYCLE_START
			SIGNAL_SET_UINT8(port_b, 0b01000000);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));

		VIA_CYCLE_START
			SIGNAL_SET_UINT8(port_b, 0b00000000);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t2c, ==, i - 1);
		munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	}

	// cycle
	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0b01000000);
	VIA_CYCLE_END
	munit_assert_uint16(via->reg_t2c, ==, 0);
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));


	VIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0b00000000);
	VIA_CYCLE_END
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_false(SIGNAL_NEXT_BOOL(irq_b));

	// clear t2 interrupt by reading t2c_l
	VIA_CYCLE_START
		strobe_via(via, true);					// enable via
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs2, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irq_b));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);
	//munit_assert_uint16(via->reg_t2c, ==, 0xfffe);

	return MUNIT_OK;
}

MunitTest chip_6522_tests[] = {
	{ "/reset", test_reset, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ddra", test_read_ddra, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ddrb", test_read_ddrb, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddra", test_write_ddra, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddrb", test_write_ddra, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_acr", test_read_acr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_acr", test_write_acr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_pcr", test_read_pcr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_pcr", test_write_pcr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ora", test_write_ora, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_orb", test_write_orb, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_porta_out", test_porta_out, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_portb_out", test_portb_out, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_read_porta_input", test_read_porta_input, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_read_porta_output", test_read_porta_output, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_read_portb_input", test_read_portb_input, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_read_portb_output", test_read_portb_output, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_porta_read", test_hs_porta_read, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_porta_read_pulse", test_hs_porta_read_pulse, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_porta_write", test_hs_porta_write, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_porta_write_pulse", test_hs_porta_write_pulse, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_portb_read", test_hs_portb_read, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_portb_read_pulse", test_hs_portb_read_pulse, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_portb_write", test_hs_portb_write, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/test_hs_portb_write_pulse", test_hs_portb_write_pulse, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ifr", test_read_ifr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ifr", test_write_ifr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ier", test_read_ier, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ier", test_write_ier, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/irq_port", test_irq_port, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/t1_once_nopb7", test_t1_once_nopb7, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/t1_freerun_nopb7", test_t1_freerun_nopb7, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/t1_once_pb7", test_t1_once_pb7, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/t1_freerun_pb7", test_t1_freerun_pb7, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/t2_timed", test_t2_timed, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/t2_count_pb6", test_t2_count_pb6, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
