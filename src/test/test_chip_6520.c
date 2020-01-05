// test/test_chip_6520.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_6520.h"

#include "stb/stb_ds.h"

#define SIGNAL_POOL			pia->signal_pool
#define SIGNAL_COLLECTION	pia->signals

#define PIA_CYCLE_START				\
	for (int i = 0; i < 2; ++i) {
#define PIA_CYCLE_END				\
		half_clock_cycle(pia);		\
	}

static void *chip_6520_setup(const MunitParameter params[], void *user_data) {
	Chip6520 *pia = chip_6520_create(signal_pool_create(), (Chip6520Signals) {0});

	SIGNAL_SET_BOOL(enable, false);
	SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	signal_pool_cycle(pia->signal_pool);
	chip_6520_process(pia);
	return pia;
}

static void chip_6520_teardown(void *fixture) {
	chip_6520_destroy((Chip6520 *) fixture);
}

static inline void strobe_pia(Chip6520 *pia, bool strobe) {
	SIGNAL_SET_BOOL(cs0, strobe);
	SIGNAL_SET_BOOL(cs1, strobe);
	SIGNAL_SET_BOOL(cs2_b, !strobe);
}

static inline void half_clock_cycle(Chip6520 *pia) {
	SIGNAL_SET_BOOL(enable, !SIGNAL_BOOL(enable));
	signal_pool_cycle(pia->signal_pool);
	chip_6520_process(pia);
}

static MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// force registers to junk values
	pia->reg_ddra = 0xde;
	pia->reg_cra.reg = 0xad;
	pia->reg_ora = 0xbe;

	pia->reg_ddrb = 0xef;
	pia->reg_crb.reg = 0xba;
	pia->reg_orb = 0xdd;

	// run pia with reset asserted
	SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	signal_pool_cycle(pia->signal_pool);
	chip_6520_process(pia);

	// registers should have been reset
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);

	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_read_ddra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_ddra = 0xa5;
	pia->reg_cra.bf_ddr_or_select = ACTHI_DEASSERT;

	// read the DDRA register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xa5);

	// try reading again from a disabled pia, bus_data shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_cra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_cra.reg = 0x5a;

	// read the CRA register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_cra.reg, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x5a);

	// try reading again from a disabled pia, output shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_cra.reg, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_porta(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_cra.bf_ddr_or_select = ACTHI_ASSERT;

	// read port-A
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(port_a, 0xaa);
		SIGNAL_SET_UINT8(port_b, 0x55);
	PIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xaa);

	// disable pia
	strobe_pia(pia, false);

	// try reading again from a disabled pia, output shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(port_a, 0xaa);
		SIGNAL_SET_UINT8(port_b, 0x55);
	PIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_ddrb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_ddrb = 0xa5;
	pia->reg_crb.bf_ddr_or_select = ACTHI_DEASSERT;

	// read the DDRB register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddrb, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0xa5);

	// try reading again from a disabled pia, output shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddrb, ==, 0xa5);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_crb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_crb.reg = 0x5a;

	// read the CRB register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_crb.reg, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x5a);

	// try reading again from a disabled pia, output shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_crb.reg, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_portb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_crb.bf_ddr_or_select = ACTHI_ASSERT;

	// read port-B
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(port_a, 0xaa);
		SIGNAL_SET_UINT8(port_b, 0x55);
	PIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x55);

	// try reading again from a disabled pia, output shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(port_a, 0xaa);
		SIGNAL_SET_UINT8(port_b, 0x55);
	PIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_ora(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// initialize registers
	pia->reg_cra.bf_ddr_or_select = ACTHI_ASSERT;

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);					// enable pia
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x07);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0b00000100);
	munit_assert_uint8(pia->reg_ora, ==, 7);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// try writing again with a disabled pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);				
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x15);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0b00000100);
	munit_assert_uint8(pia->reg_ora, ==, 0x07);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_write_ddra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// write to the DDRA register
	PIA_CYCLE_START
		strobe_pia(pia, true);					// enable pia
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x09);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0x09);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0b00000000);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// disable pia
	strobe_pia(pia, false);

	// try writing again with a disabled pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);	
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0x09);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0b00000000);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_write_cra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);				
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0x3f);		// top two bits controlled by pia
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// disable pia
	strobe_pia(pia, false);

	// try writing again with disabled pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);				
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x23);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0x3f);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}


static MunitResult test_write_orb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// initialize registers
	pia->reg_crb.bf_ddr_or_select = ACTHI_ASSERT;

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);				
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x07);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0b00000100);
	munit_assert_uint8(pia->reg_orb, ==, 0x07);

	// try writing again to disabled pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);				
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x15);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0b00000100);
	munit_assert_uint8(pia->reg_orb, ==, 0x07);

	return MUNIT_OK;
}

static MunitResult test_write_ddrb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);				
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0x17);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// try writing again to a disabled pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);				
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0x17);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_write_crb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);				
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0x3f);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// disable pia
	strobe_pia(pia, false);

	// try writing again to a disable pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);				
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra.reg, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb.reg, ==, 0x3f);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_irqa_neg(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_cra.reg = 0b00001001;	// irqa1 & irqa2 enable on a negative transition

	// check initial state (irq not asserted)
	munit_assert_true(SIGNAL_BOOL(irqa_b));
	munit_assert_false(SIGNAL_BOOL(ca1));
	munit_assert_false(SIGNAL_BOOL(ca2));

	// transition ca1&ca2 to high - nothing should happen
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(pia->reg_cra.bf_irq1);
	munit_assert_false(pia->reg_cra.bf_irq2);

	// transition ca1 to low - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(pia->reg_cra.bf_irq1);
	munit_assert_false(pia->reg_cra.bf_irq2);

	// transition ca2 to low - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(pia->reg_cra.bf_irq1);
	munit_assert_true(pia->reg_cra.bf_irq2);

	// change CRA to enable read from port-A
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00001101);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(pia->reg_cra.bf_irq1);
	munit_assert_true(pia->reg_cra.bf_irq2);
	munit_assert_true(pia->reg_cra.bf_ddr_or_select);

	// read port-A - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(pia->reg_cra.bf_irq1);
	munit_assert_false(pia->reg_cra.bf_irq2);

	return MUNIT_OK;
}

static MunitResult test_irqa_pos(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END

	pia->reg_cra.reg = 0b00011011;	// irqa1 & irqa2 enable on a positive transition

	// check initial state (irq not asserted)
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(SIGNAL_BOOL(ca1));
	munit_assert_true(SIGNAL_BOOL(ca2));
	munit_assert_true(pia->reg_cra.bf_irq1_pos_transition);
	munit_assert_true(pia->reg_cra.bf_irq2_pos_transition);

	// transition ca1&ca2 to low - nothing should happen
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(pia->reg_cra.bf_irq1);
	munit_assert_false(pia->reg_cra.bf_irq2);

	// transition ca1 to high - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(pia->reg_cra.bf_irq1);
	munit_assert_false(pia->reg_cra.bf_irq2);

	// transition ca2 to high - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(pia->reg_cra.bf_irq1);
	munit_assert_true(pia->reg_cra.bf_irq2);

	// change CRA to enable read from port-A
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00011111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(pia->reg_cra.bf_irq1);
	munit_assert_true(pia->reg_cra.bf_irq2);
	munit_assert_true(pia->reg_cra.bf_ddr_or_select);

	// read port-A - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(pia->reg_cra.bf_irq1);
	munit_assert_false(pia->reg_cra.bf_irq2);

	return MUNIT_OK;
}

static MunitResult test_irqb_neg(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_crb.reg = 0b00001001;	// irqa1 & irqa2 enable on a negative transition

	// check initial state (irq not asserted)
	munit_assert_true(SIGNAL_BOOL(irqb_b));
	munit_assert_false(SIGNAL_BOOL(cb1));
	munit_assert_false(SIGNAL_BOOL(cb2));

	// transition cb1&cb2 to high - nothing should happen
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_false(pia->reg_crb.bf_irq1);
	munit_assert_false(pia->reg_crb.bf_irq2);

	// transition cb1 to low - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(pia->reg_crb.bf_irq1);
	munit_assert_false(pia->reg_crb.bf_irq2);

	// transition cb2 to low - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(pia->reg_crb.bf_irq1);
	munit_assert_true(pia->reg_crb.bf_irq2);

	// change CRB to enable read from port-B
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00001101);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(pia->reg_crb.bf_irq1);
	munit_assert_true(pia->reg_crb.bf_irq2);
	munit_assert_true(pia->reg_crb.bf_ddr_or_select);

	// read port-B - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_false(pia->reg_crb.bf_irq1);
	munit_assert_false(pia->reg_crb.bf_irq2);

	return MUNIT_OK;
}

static MunitResult test_irqb_pos(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	pia->reg_crb.reg = 0b00011011;	// irqb1 & irqb2 enable on a positive transition

	// check initial state (irq not asserted)
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(SIGNAL_BOOL(cb1));
	munit_assert_true(SIGNAL_BOOL(cb2));
	munit_assert_true(pia->reg_crb.bf_irq1_pos_transition);
	munit_assert_true(pia->reg_crb.bf_irq2_pos_transition);

	// transition cb1&cb2 to low - nothing should happen
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_false(pia->reg_crb.bf_irq1);
	munit_assert_false(pia->reg_crb.bf_irq2);

	// transition cb1 to high - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(pia->reg_crb.bf_irq1);
	munit_assert_false(pia->reg_crb.bf_irq2);

	// transition cb2 to high - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(pia->reg_crb.bf_irq1);
	munit_assert_true(pia->reg_crb.bf_irq2);

	// change CRB to enable read from port-B
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00011111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(pia->reg_crb.bf_irq1);
	munit_assert_true(pia->reg_crb.bf_irq2);
	munit_assert_true(pia->reg_crb.bf_ddr_or_select);

	// read port-B - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_false(pia->reg_crb.bf_irq1);
	munit_assert_false(pia->reg_crb.bf_irq2);

	return MUNIT_OK;
}

MunitTest chip_6520_tests[] = {
	{ "/reset", test_reset, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ddra", test_read_ddra, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_cra", test_read_cra, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_porta", test_read_porta, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ddrb", test_read_ddrb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_crb", test_read_crb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_portb", test_read_portb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ora", test_write_ora, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddra", test_write_ddra, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_cra", test_write_cra, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_orb", test_write_orb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddrb", test_write_ddrb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_crb", test_write_crb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/irqa_neg", test_irqa_neg, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/irqa_pos", test_irqa_pos, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/irqb_neg", test_irqb_neg, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/irqb_pos", test_irqb_pos, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
