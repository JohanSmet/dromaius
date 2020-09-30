// test/test_chip_6520.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_6520.h"
#include "simulator.h"

#include "stb/stb_ds.h"

#define SIGNAL_POOL			pia->signal_pool
#define SIGNAL_COLLECTION	pia->signals
#define SIGNAL_CHIP_ID		pia->id

#define PIA_CYCLE_START				\
	for (int i = 0; i < 2; ++i) {
#define PIA_CYCLE_END				\
		half_clock_cycle(pia);		\
	}

#define PIA_CYCLE()					\
	PIA_CYCLE_START					\
	PIA_CYCLE_END

static void *chip_6520_setup(const MunitParameter params[], void *user_data) {
	Chip6520 *pia = chip_6520_create(simulator_create(NS_TO_PS(100)), (Chip6520Signals) {0});

	// run chip with reset asserted
	SIGNAL_SET_BOOL(enable, false);
	SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	signal_pool_cycle(pia->signal_pool, 1);
	chip_6520_process(pia);

	// deassert reset
	SIGNAL_SET_BOOL(reset_b, ACTLO_DEASSERT);

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
	signal_pool_cycle(pia->signal_pool, 1);
	chip_6520_process(pia);
}

static MunitResult test_create(const MunitParameter params[], void *user_data_or_fixture) {

	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;
	munit_assert_ptr_not_null(pia);
	munit_assert_ptr_equal(pia->process, chip_6520_process);
	munit_assert_ptr_equal(pia->destroy, chip_6520_destroy);

	return MUNIT_OK;
}

static MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// force registers to junk values
	pia->reg_ddra = 0xde;
	pia->reg_cra = 0xad;
	pia->reg_ora = 0xbe;

	pia->reg_ddrb = 0xef;
	pia->reg_crb = 0xba;
	pia->reg_orb = 0xdd;

	// run pia with reset asserted
	SIGNAL_SET_BOOL(reset_b, ACTLO_ASSERT);
	signal_pool_cycle(pia->signal_pool, 1);
	chip_6520_process(pia);

	// registers should have been reset
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);

	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_read_ddra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_ddra = 0xa5;
	FLAG_SET_CLEAR_U8(pia->reg_cra, FLAG_6520_DDR_OR_SELECT, ACTHI_DEASSERT);

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
	pia->reg_cra = 0x5a;

	// read the CRA register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_cra, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x5a);

	// try reading again from a disabled pia, output shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_cra, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_porta(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	FLAG_SET_CLEAR_U8(pia->reg_cra, FLAG_6520_DDR_OR_SELECT, ACTHI_ASSERT);

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
	FLAG_SET_CLEAR_U8(pia->reg_crb, FLAG_6520_DDR_OR_SELECT, ACTHI_DEASSERT);

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
	pia->reg_crb = 0x5a;

	// read the CRB register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_crb, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x5a);

	// try reading again from a disabled pia, output shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_crb, ==, 0x5a);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(bus_data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_portb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	FLAG_SET_CLEAR_U8(pia->reg_crb, FLAG_6520_DDR_OR_SELECT, ACTHI_ASSERT);

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
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// initialize registers
	FLAG_SET_CLEAR_U8(pia->reg_cra, FLAG_6520_DDR_OR_SELECT, ACTHI_ASSERT);

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);					// enable pia
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x07);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0b00000100);
	munit_assert_uint8(pia->reg_ora, ==, 7);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
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
	munit_assert_uint8(pia->reg_cra, ==, 0b00000100);
	munit_assert_uint8(pia->reg_ora, ==, 0x07);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_write_ddra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
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
	munit_assert_uint8(pia->reg_cra, ==, 0b00000000);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// try writing again with a disabled pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x17);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0x09);
	munit_assert_uint8(pia->reg_cra, ==, 0b00000000);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_write_cra(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x5f);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0x1f);		// top two bits controlled by pia
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// try writing again with disabled pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x23);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0x1f);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}


static MunitResult test_write_orb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// initialize registers
	FLAG_SET_CLEAR_U8(pia->reg_crb, FLAG_6520_DDR_OR_SELECT, ACTHI_ASSERT);

	// write to the ORA register
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x07);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0b00000100);
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
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0b00000100);
	munit_assert_uint8(pia->reg_orb, ==, 0x07);

	return MUNIT_OK;
}

static MunitResult test_write_ddrb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
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
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0x17);
	munit_assert_uint8(pia->reg_crb, ==, 0);
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
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0x17);
	munit_assert_uint8(pia->reg_crb, ==, 0);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_write_crb(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// assert initial state
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0);
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
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0x3f);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	// try writing again to a disable pia, register shouldn't change
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0x7f);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_uint8(pia->reg_ddra, ==, 0);
	munit_assert_uint8(pia->reg_cra, ==, 0);
	munit_assert_uint8(pia->reg_ora, ==, 0);
	munit_assert_uint8(pia->reg_ddrb, ==, 0);
	munit_assert_uint8(pia->reg_crb, ==, 0x3f);
	munit_assert_uint8(pia->reg_orb, ==, 0);

	return MUNIT_OK;
}

static MunitResult test_irqa_neg(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_cra = 0b00001001;	// irqa1 & irqa2 enable on a negative transition

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
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

	// transition ca1 to low - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

	// transition ca2 to low - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

	// change CRA to enable read from port-A
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00001101);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_DDR_OR_SELECT));

	// read port-A - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_NO_WRITE(bus_data);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

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

	pia->reg_cra = 0b00011011;	// irqa1 & irqa2 enable on a positive transition

	// check initial state (irq not asserted)
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(SIGNAL_BOOL(ca1));
	munit_assert_true(SIGNAL_BOOL(ca2));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1_POS_TRANSITION));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2_POS_TRANSITION));

	// transition ca1&ca2 to low - nothing should happen
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

	// transition ca1 to high - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

	// transition ca2 to high - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(ca2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

	// change CRA to enable read from port-A
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00011111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));
	munit_assert_true(FLAG_IS_SET(pia->reg_cra, FLAG_6520_DDR_OR_SELECT));

	// read port-A - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_NO_WRITE(bus_data);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqa_b));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_cra, FLAG_6520_IRQ2));

	return MUNIT_OK;
}

static MunitResult test_irqb_neg(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_crb = 0b00001001;	// irqa1 & irqa2 enable on a negative transition

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
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	// transition cb1 to low - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	// transition cb2 to low - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	// change CRB to enable read from port-B
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00001101);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_DDR_OR_SELECT));

	// read port-B - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	return MUNIT_OK;
}

static MunitResult test_irqb_pos(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	pia->reg_crb = 0b00011011;	// irqb1 & irqb2 enable on a positive transition

	// check initial state (irq not asserted)
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(SIGNAL_BOOL(cb1));
	munit_assert_true(SIGNAL_BOOL(cb2));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1_POS_TRANSITION));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2_POS_TRANSITION));

	// transition cb1&cb2 to low - nothing should happen
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	// transition cb1 to high - triggers irq1
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	// transition cb2 to high - triggers irq2
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(cb2, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	// change CRB to enable read from port-B
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00011111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));
	munit_assert_true(FLAG_IS_SET(pia->reg_crb, FLAG_6520_DDR_OR_SELECT));

	// read port-B - should reset the irq
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(irqb_b));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ1));
	munit_assert_false(FLAG_IS_SET(pia->reg_crb, FLAG_6520_IRQ2));

	return MUNIT_OK;
}

static MunitResult test_porta_out(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_ora = 0xf5;
	pia->reg_ddra = 0xff;		// all pins configured as output

	// cycle clock ; entire ora-register should have been written to port-A
	PIA_CYCLE_START
		SIGNAL_SET_UINT8(port_a, 0x12);
	PIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_a), ==, 0xf5);

	// reconfigure ddra, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8_MASKED(port_a, 0x09, 0x0f);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_UINT8(bus_data, 0xf0);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END

	// cycle clock
	half_clock_cycle(pia);
	SIGNAL_SET_UINT8_MASKED(port_a, 0x09, 0x0f);
	SIGNAL_SET_BOOL(rw, true);

	half_clock_cycle(pia);
	SIGNAL_SET_UINT8_MASKED(port_a, 0x09, 0x0f);

	munit_assert_uint8(pia->reg_ddra, ==, 0xf0);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_a), ==, 0xf9);

	return MUNIT_OK;
}

static MunitResult test_portb_out(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_orb = 0xf5;
	pia->reg_ddrb = 0xff;		// all pins configured as output

	// cycle clock ; entire ora-register should have been written to port-B
	PIA_CYCLE_START
		SIGNAL_SET_UINT8(port_b, 0x12);
	PIA_CYCLE_END
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_b), ==, 0xf5);

	// reconfigure ddrb, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8_MASKED(port_b, 0x09, 0x0f);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_UINT8(bus_data, 0xf0);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END

	// cycle clock
	half_clock_cycle(pia);
	SIGNAL_SET_UINT8_MASKED(port_b, 0x09, 0x0f);
	SIGNAL_SET_BOOL(rw, true);

	half_clock_cycle(pia);
	SIGNAL_SET_UINT8_MASKED(port_b, 0x09, 0x0f);

	munit_assert_uint8(pia->reg_ddrb, ==, 0xf0);
	munit_assert_uint8(SIGNAL_NEXT_UINT8(port_b), ==, 0xf9);

	return MUNIT_OK;
}

static MunitResult test_ca2_out_manual(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_cra = 0b00110011;	// ca2 = output, output control = 1, irqa1 enabled on positive transition

	// assert initial condition
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// change nothing, ca2 low
	PIA_CYCLE_START
		strobe_pia(pia, true);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// write 1 to bit 3 of cra: ca2 goes high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00111011);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// change nothing, ca2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// disable pia, ca2 still high
	PIA_CYCLE_START
		strobe_pia(pia, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// re-enable pia, ca2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// write 0 to bit 3 of cra: ca2 low
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00110011);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	return MUNIT_OK;
}

static MunitResult test_ca2_out_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_cra = 0b00101011;	// ca2 = output, output control = 0, restore control = 1, irqa1 enabled on positive transition

	// read ddra, ca2 high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// change cra to switch from ddr to port-a, ca2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00101111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// read port-A, ca2 low (for one clock-cycle)
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rw, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// cycle clock, don't read port-A, ca2 returns to high
	PIA_CYCLE_START
		strobe_pia(pia, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	return MUNIT_OK;
}

static MunitResult test_ca2_out_handshake(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize CRA
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00100011); // ca2 = output, output control = 0, restore control = 0, irqa1 enabled on positive transition
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// read ddra, ca2 high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rw, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// change cra to switch from ddr to port-a, ca2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00100111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// read port-A, ca2 low (until active transition on ca1)
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rw, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// cycle clock, don't read port-A, ca2 stays low
	PIA_CYCLE_START
		strobe_pia(pia, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// cycle clock, don't read port-A, ca2 stays low
	PIA_CYCLE()
	munit_assert_false(SIGNAL_NEXT_BOOL(ca2));

	// assert ca1, ca2 goes high
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// deassert ca1, ca2 stays high
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(ca1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	// cycle clock, don't read port-A, ca2 stays high
	PIA_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(ca2));

	return MUNIT_OK;
}

static MunitResult test_cb2_out_manual(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_crb = 0b00110011;	// cb2 = output, output control = 1, irqa1 enabled on positive transition

	// assert initial condition
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// change nothing, cb2 low
	PIA_CYCLE_START
		strobe_pia(pia, true);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// write 1 to bit 3 of cra: cb2 goes high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00111011);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// change nothing, cb2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// disable pia, cb2 still high
	PIA_CYCLE_START
		strobe_pia(pia, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// re-enable pia, cb2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// write 0 to bit 3 of cra: cb2 low
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00110011);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	return MUNIT_OK;
}

static MunitResult test_cb2_out_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize registers
	pia->reg_crb = 0b00101011;	// cb2 = output, output control = 0, restore control = 1, irqa1 enabled on positive transition

	// read ddrb, cb2 high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// change crb to switch from ddr to port-a, cb2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00101111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// write port-B, cb2 low (for one clock-cycle)
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// cycle clock, don't write port-B, cb2 returns to high
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	return MUNIT_OK;
}

static MunitResult test_cb2_out_handshake(const MunitParameter params[], void *user_data_or_fixture) {
	Chip6520 *pia = (Chip6520 *) user_data_or_fixture;

	// initialize CRB
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00100011); // cb2 = output, output control = 0, restore control = 0, irqa1 enabled on positive transition
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// read ddrb, cb2 high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rw, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// change crb to switch from ddr to port-b, cb2 still high
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_UINT8(bus_data, 0b00100111);
		SIGNAL_SET_BOOL(rs0, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// write port-B, cb2 low (until active transition on cb1)
	PIA_CYCLE_START
		strobe_pia(pia, true);
		SIGNAL_SET_BOOL(rs0, ACTHI_DEASSERT);
		SIGNAL_SET_BOOL(rs1, ACTHI_ASSERT);
		SIGNAL_SET_BOOL(rw, false);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// cycle clock, don't read port-A, cb2 stays low
	PIA_CYCLE_START
		strobe_pia(pia, false);
		SIGNAL_SET_BOOL(rw, true);
	PIA_CYCLE_END
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// cycle clock, don't read port-A, cb2 stays low
	PIA_CYCLE()
	munit_assert_false(SIGNAL_NEXT_BOOL(cb2));

	// assert cb1, cb2 goes high
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_ASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// deassert cb1, cb2 stays high
	PIA_CYCLE_START
		SIGNAL_SET_BOOL(cb1, ACTHI_DEASSERT);
	PIA_CYCLE_END
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	// cycle clock, don't write port-B, cb2 stays high
	PIA_CYCLE();
	munit_assert_true(SIGNAL_NEXT_BOOL(cb2));

	return MUNIT_OK;
}

MunitTest chip_6520_tests[] = {
	{ "/create", test_create, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
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
	{ "/porta_out", test_porta_out, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/portb_out", test_portb_out, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ca2_out_manual", test_ca2_out_manual, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ca2_out_pulse", test_ca2_out_pulse, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/ca2_out_handshake", test_ca2_out_handshake, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cb2_out_manual", test_cb2_out_manual, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cb2_out_pulse", test_cb2_out_pulse, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/cb2_out_handshake", test_cb2_out_handshake, chip_6520_setup, chip_6520_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
