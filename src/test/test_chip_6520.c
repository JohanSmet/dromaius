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

	SIGNAL_SET_BOOL(clock, false);
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
	SIGNAL_SET_BOOL(clock, !SIGNAL_BOOL(clock));
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
	munit_assert_uint8(pia->reg_cra.reg, ==, 0x7f);
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
	munit_assert_uint8(pia->reg_cra.reg, ==, 0x7f);
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
	munit_assert_uint8(pia->reg_crb.reg, ==, 0x7f);
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
	munit_assert_uint8(pia->reg_crb.reg, ==, 0x7f);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_irqa(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_cra.reg = 0b00001001;	// irqa1 & irqa2 enable on a negative transition

	// check initial state (irq not asserted)
	munit_assert_true(SIGNAL_BOOL(irqa_b));
	munit_assert_false(SIGNAL_BOOL(ca1));
	munit_assert_false(SIGNAL_BOOL(ca2));

	// transition ca1&ca2 to high - nothing should happen
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(pia->reg_cra.bf_irq1);
	munit_assert_false(pia->reg_cra.bf_irq2);

	// transition ca1 to low - triggers irq1
	PIA_CYCLE_START
		strobe_pia(pia, true);
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

	// read port-A - should reset the irq


	return MUNIT_OK;
}

MunitTest chip_6520_tests[] = {
	{ "/reset", test_reset, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ora", test_write_ora, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddra", test_write_ddra, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_cra", test_write_cra, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_orb", test_write_orb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddrb", test_write_ddrb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_crb", test_write_crb, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/irqa", test_irqa, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
