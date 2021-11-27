// test/test_chip_6522.c - Johan Smet - BSD-3-Clause (see LICENSE)

#include "munit/munit.h"
#include "chip_6522.h"
#include "simulator.h"

#include "stb/stb_ds.h"

enum Test6522DeviceSignalAssignment {
	SIG_6522_PA0 = 0,
	SIG_6522_PA1,
	SIG_6522_PA2,
	SIG_6522_PA3,
	SIG_6522_PA4,
	SIG_6522_PA5,
	SIG_6522_PA6,
	SIG_6522_PA7,
	SIG_6522_PB0,
	SIG_6522_PB1,
	SIG_6522_PB2,
	SIG_6522_PB3,
	SIG_6522_PB4,
	SIG_6522_PB5,
	SIG_6522_PB6,
	SIG_6522_PB7,
	SIG_6522_D0,
	SIG_6522_D1,
	SIG_6522_D2,
	SIG_6522_D3,
	SIG_6522_D4,
	SIG_6522_D5,
	SIG_6522_D6,
	SIG_6522_D7,
	SIG_6522_CA1,
	SIG_6522_CA2,
	SIG_6522_CB1,
	SIG_6522_CB2,
	SIG_6522_RS0,
	SIG_6522_RS1,
	SIG_6522_RS2,
	SIG_6522_RS3,
	SIG_6522_RESET_B,
	SIG_6522_PHI2,
	SIG_6522_CS1,
	SIG_6522_CS2_B,
	SIG_6522_RW,
	SIG_6522_IRQ_B,

	SIG_6522_SIGNAL_COUNT
};

typedef struct TestFixture6522 {
	Simulator *		simulator;
	Signal			signals[SIG_6522_SIGNAL_COUNT];
	Chip6522 *		via;

	SignalGroup		sg_port_a;
	SignalGroup		sg_port_b;
	SignalGroup		sg_data;
} TestFixture6522;

#define SIGNAL_PREFIX		SIG_6522_
#define SIGNAL_OWNER		fixture

#undef SIGNAL_POOL
#define SIGNAL_POOL			fixture->simulator->signal_pool

#define VIA_CYCLE_START				\
	for (int i = 0; i < 2; ++i) {
#define VIA_CYCLE_END				\
		half_clock_cycle(fixture);	\
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

	// create test fixture
	TestFixture6522 *fixture = (TestFixture6522 *) calloc(1, sizeof(TestFixture6522));
	fixture->simulator = simulator_create(NS_TO_PS(100));
	signal_pool_set_layer_count(fixture->simulator->signal_pool, 2);

	// >> setup signals
	fixture->sg_port_a = signal_group_create();
	fixture->sg_port_b = signal_group_create();
	fixture->sg_data = signal_group_create();

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(PA0 + i, port_a);
	}

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(PB0 + i, port_b);
	}

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(D0 + i, data);
	}

	SIGNAL_DEFINE_DEFAULT(CA1,		false);
	SIGNAL_DEFINE_DEFAULT(CA2,		false);
	SIGNAL_DEFINE_DEFAULT(CB1,		false);
	SIGNAL_DEFINE_DEFAULT(CB2,		false);
	SIGNAL_DEFINE_DEFAULT(IRQ_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS0,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS1,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS2,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS3,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RESET_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(PHI2,		true);
	SIGNAL_DEFINE_DEFAULT(CS1,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(CS2_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RW,		true);

	// create via
	fixture->via = chip_6522_create(fixture->simulator, (Chip6522Signals) {
										[CHIP_6522_D0] = SIGNAL(D0),
										[CHIP_6522_D1] = SIGNAL(D1),
										[CHIP_6522_D2] = SIGNAL(D2),
										[CHIP_6522_D3] = SIGNAL(D3),
										[CHIP_6522_D4] = SIGNAL(D4),
										[CHIP_6522_D5] = SIGNAL(D5),
										[CHIP_6522_D6] = SIGNAL(D6),
										[CHIP_6522_D7] = SIGNAL(D7),
										[CHIP_6522_PHI2] = SIGNAL(PHI2),
										[CHIP_6522_RESET_B] = SIGNAL(RESET_B),
										[CHIP_6522_RW] = SIGNAL(RW),
										[CHIP_6522_CS1] = SIGNAL(CS1),
										[CHIP_6522_CS2_B] = SIGNAL(CS2_B),
										[CHIP_6522_RS0] = SIGNAL(RS0),
										[CHIP_6522_RS1] = SIGNAL(RS1),
										[CHIP_6522_RS2] = SIGNAL(RS2),
										[CHIP_6522_RS3] = SIGNAL(RS3),
										[CHIP_6522_CA1] = SIGNAL(CA1),
										[CHIP_6522_CA2] = SIGNAL(CA2),
										[CHIP_6522_PA0] = SIGNAL(PA0),
										[CHIP_6522_PA1] = SIGNAL(PA1),
										[CHIP_6522_PA2] = SIGNAL(PA2),
										[CHIP_6522_PA3] = SIGNAL(PA3),
										[CHIP_6522_PA4] = SIGNAL(PA4),
										[CHIP_6522_PA5] = SIGNAL(PA5),
										[CHIP_6522_PA6] = SIGNAL(PA6),
										[CHIP_6522_PA7] = SIGNAL(PA7),
										[CHIP_6522_PB0] = SIGNAL(PB0),
										[CHIP_6522_PB1] = SIGNAL(PB1),
										[CHIP_6522_PB2] = SIGNAL(PB2),
										[CHIP_6522_PB3] = SIGNAL(PB3),
										[CHIP_6522_PB4] = SIGNAL(PB4),
										[CHIP_6522_PB5] = SIGNAL(PB5),
										[CHIP_6522_PB6] = SIGNAL(PB6),
										[CHIP_6522_PB7] = SIGNAL(PB7),
										[CHIP_6522_CB1] = SIGNAL(CB1),
										[CHIP_6522_CB2] = SIGNAL(CB2),
										[CHIP_6522_IRQ_B] = SIGNAL(IRQ_B)
	});
	simulator_device_complete(fixture->simulator);

	// via should write to a different signal layer (can not use simulator_device_complete because fixture is not a chip)
	for (uint32_t s = 0; s < fixture->via->pin_count; ++s) {
		fixture->via->pins[s].layer = 1;
	}
	fixture->simulator->signal_pool->block_layer_count[0] = 2;

	// run chip with reset asserted
	SIGNAL_WRITE(PHI2, false);
	SIGNAL_WRITE(RESET_B, ACTLO_ASSERT);
	signal_pool_cycle(fixture->simulator->signal_pool);
	fixture->via->process(fixture->via);

	// deassert reset
	SIGNAL_WRITE(RESET_B, ACTLO_DEASSERT);

	return fixture;
}

static void chip_6522_teardown(void *munit_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) munit_fixture;

	simulator_destroy(fixture->simulator);
	fixture->via->destroy(fixture->via);
	free(fixture);
}

static inline void strobe_via(TestFixture6522 *fixture, bool strobe) {
	SIGNAL_WRITE(CS1, strobe);
	SIGNAL_WRITE(CS2_B, !strobe);
}

static inline void half_clock_cycle(TestFixture6522 *fixture) {
	SIGNAL_WRITE(PHI2, !SIGNAL_READ(PHI2));
	signal_pool_cycle(fixture->simulator->signal_pool);
	fixture->via->process(fixture->via);
}

static MunitResult test_create(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	munit_assert_ptr_not_null(via);
	munit_assert_ptr_not_null(via->process);
	munit_assert_ptr_not_null(via->destroy);

	return MUNIT_OK;
}

static MunitResult test_reset(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

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
	SIGNAL_WRITE(RESET_B, ACTLO_ASSERT);
	signal_pool_cycle(via->signal_pool);
	via->process(via);

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
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ddra = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddra, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddra, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_read_ddrb(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ddrb = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddrb, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddrb, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_ddra(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the DDRA register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x09);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddra, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRA);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x17);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_ddra, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRA);

	return MUNIT_OK;
}

static MunitResult test_write_ddrb(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the DDRA register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x09);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ddrb, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRB);

	// disable via
	strobe_via(fixture, false);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x17);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_ddrb, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_DDRB);

	return MUNIT_OK;
}

static MunitResult test_read_acr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_acr = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_acr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the ACR register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0x09);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_ACR);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0x17);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_acr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_ACR);

	return MUNIT_OK;
}

static MunitResult test_read_pcr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;


	// initialize registers
	via->reg_pcr = 0xa5;

	// read the DDRA register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xa5);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa5);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_pcr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the PCR register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0x09);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_PCR);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0x17);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_pcr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_PCR);

	return MUNIT_OK;
}

static MunitResult test_write_ora(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the ORA register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x7f);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ora, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORA);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x17);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_ora, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORA);

	return MUNIT_OK;
}

static MunitResult test_write_orb(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the ORA register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x7f);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_orb, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORB);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x17);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_orb, ==, 0x7f);
	ASSERT_REGISTERS_DEFAULT(REG_ORB);

	return MUNIT_OK;
}

static MunitResult test_porta_out(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ora = 0xf5;
	via->reg_ddra = 0xff;		// all pins configured as output

	// cycle clock ; entire ora-register should have been written to port-A
	VIA_CYCLE()
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(port_a), ==, 0xf5);

	// reconfigure ddra, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE_MASKED(port_a, 0x09, 0x0f);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0xf0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// cycle clock
	half_clock_cycle(fixture);
	SIGNAL_GROUP_WRITE_MASKED(port_a, 0x09, 0x0f);
	SIGNAL_WRITE(RW, true);
	half_clock_cycle(fixture);
	SIGNAL_GROUP_WRITE_MASKED(port_a, 0x09, 0x0f);

	munit_assert_uint8(via->reg_ddra, ==, 0xf0);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(port_a), ==, 0xf9);

	return MUNIT_OK;
}

static MunitResult test_portb_out(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_orb = 0xf5;
	via->reg_ddrb = 0xff;		// all pins configured as output

	// cycle clock ; entire ora-register should have been written to port-A
	VIA_CYCLE ()
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(port_b), ==, 0xf5);

	// reconfigure ddrb, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE_MASKED(port_b, 0x09, 0x0f);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0xf0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// cycle clock
	half_clock_cycle(fixture);
	SIGNAL_GROUP_WRITE_MASKED(port_b, 0x09, 0x0f);
	SIGNAL_WRITE(RW, true);
	half_clock_cycle(fixture);
	SIGNAL_GROUP_WRITE_MASKED(port_b, 0x09, 0x0f);
	munit_assert_uint8(via->reg_ddrb, ==, 0xf0);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(port_b), ==, 0xf9);

	return MUNIT_OK;
}

static MunitResult test_read_porta_input(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-A while pins are configured as input
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ora = 0xf5;
	via->reg_ddra = 0x00;		// all pins configured as input

	// cycle clock
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0x12);
	VIA_CYCLE_END

	// read from the port
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_a, 0x12);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x12);

	// enable latching on port-A (default == on negative edge)
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(data, 0b00000001);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0b00000001);

	// start writing to port-A, keep CA1 high
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_GROUP_WRITE(port_a, 0x3e);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// bring CA1 low, latching data on the port
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0x3e);
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// bring CA1 high, change data on port, internal register shouldn't change
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0x15);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x3e);

	// read from the port
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_GROUP_WRITE(port_a, 0x12);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x3e);

	// read unlatched port - ila should follow port again
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0x15);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x15);

	return MUNIT_OK;
}

static MunitResult test_read_porta_output(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-A while pins are configured as output - port-A reads values on the pins (or latched value)
// when all pins are configured as output one wouldn't normally expect external writes to the port_a signals,
// but the test does this to simulate pins being pulled down, thus returning a different value than is being output
// by the via-chip
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ora = 0xf5;
	via->reg_ddra = 0xff;		// all pins configured as output

	// read from the port
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(data), ==, 0xf5);

	// enable latching on port-A (default == on negative edge)
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(data, 0b00000001);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0b00000001);

	// start writing to port-A, keep CA1 high
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_GROUP_WRITE(port_a, 0x3e);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x34);

	// bring CA1 low, latching data on the port
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_a, 0x3e);
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x34);

	// bring CA1 high, change data on port, internal register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_a, 0x15);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x34);

	// read from the port
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_GROUP_WRITE(port_a, 0x12);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x34);

	// read unlatched port - ila should follow port again
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_a, 0x15);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ila, ==, 0x15);

	return MUNIT_OK;
}

static MunitResult test_read_portb_input(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-B while pins are configured as input
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_orb = 0xf5;
	via->reg_ddrb = 0x00;		// all pins configured as input

	// cycle clock
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_b, 0x12);
	VIA_CYCLE_END

	// read from the port
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_b, 0x12);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x12);

	// enable latching on port-B (default == on negative edge)
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(data, 0b00000010);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_acr, ==, 0b00000010);

	// start writing to port-B, keep CB1 high
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0x3e);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x3e);

	// bring CB1 low, latching data on the port
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0x3e);
		SIGNAL_WRITE(CB1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x3e);

	// bring CB1 high, change data on port, internal register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0x15);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x3e);

	// read from the port
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_GROUP_WRITE(port_b, 0x12);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x3e);

	// read unlatched port - ilb should follow port again
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0x15);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ilb, ==, 0x15);

	return MUNIT_OK;
}

static MunitResult test_read_portb_output(const MunitParameter params[], void *user_data_or_fixture) {
// read from port-B while pins are configured as output - contrary to port-A: pin level has NO effect. Always reads ORB-register.
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_orb = 0xf5;
	via->reg_ddrb = 0xff;		// all pins configured as output

	// read from the port - no pins pulled down
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xf5);

	// read from the port - some pins pulled down
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_b, 0b11101010);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xf5);

	// reconfigure ddrb, upper nibble = output, lower nibble = input + peripheral active on lower nibble
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0xf0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// read from the port - some pins pulled down (only affect lower nibble)
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_b, 0b11101010);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xfa);

	return MUNIT_OK;
}

static MunitResult test_hs_porta_read(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// configure CA2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00001000);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x08);

	// assert CA1 to indicate there's available data
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END

	// another cycle
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END

	// read port-A - CA2 shoukd go low
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);

		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xab);
	munit_assert_false(SIGNAL_READ_NEXT(CA2));

	// clock-cycle - CA2 stays low
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CA2));

	// clock-cycle - CA2 stays low
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CA2));

	// new data available on port-A: CA2 goes high
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0xba);
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CA2));

	return MUNIT_OK;
}

static MunitResult test_hs_porta_read_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// configure CA2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00001010);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x0a);

	// assert CA1 to indicate there's available data
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END

	// another cycle
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END

	// read port-A - CA2 shoukd go low
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);

		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xab);
	munit_assert_false(SIGNAL_READ_NEXT(CA2));

	// clock-cycle - CA2 goes high again
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_a, 0xab);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CA2));

	return MUNIT_OK;
}

static MunitResult test_hs_porta_write(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ddra = 0xff;		// all pins configured as output

	// configure CA2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00001000);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x08);

	// write data to port-A - CA2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x7f);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CA2));
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(port_a), ==, 0x7f);

	// clock cycle - CA2 stays low
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CA2));

	// clock cycle - CA2 stays low
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CA2));

	// assert CA1 - CA2 goes high
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CA2));

	return MUNIT_OK;
}

static MunitResult test_hs_porta_write_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ddra = 0xff;		// all pins configured as output

	// configure CA2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00001010);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x0a);

	// write data to port-A - CA2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x7f);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CA2));
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(port_a), ==, 0x7f);

	// clock cycle - CA2 goes high again
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CA2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_read(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// configure CB2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b10000000);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x80);
	munit_assert_false(SIGNAL_READ_NEXT(CB2));

	// assert CB1 to indicate there's available data
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0xab);
		SIGNAL_WRITE(CB1, ACTLO_ASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	// another cycle
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0xab);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	// read port-B - CB2 should not change - no read handshake on port-B
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_b, 0xab);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);

		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xab);
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_read_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// configure CB2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b10100000);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa0);
	munit_assert_false(SIGNAL_READ_NEXT(CB2));

	// assert CB1 to indicate there's available data
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0xab);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB1, ACTLO_ASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	// another cycle
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_WRITE(port_b, 0xab);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	// read port-B - CB2 should not change - no read handshake on port-B
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_GROUP_WRITE(port_b, 0xab);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);

		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0xab);
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_write(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ddrb = 0xff;		// all pins configured as output

	// configure CB2 to operate in handshake-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b10000000);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0x80);

	// write data to port-B - CB2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x7f);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CB2));
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(port_b), ==, 0x7f);

	// clock cycle - CB2 stays low
	VIA_CYCLE_START
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_GROUP_NO_WRITE(data);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CB2));

	// clock cycle - CB2 stays low
	VIA_CYCLE_START
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CB2));

	// assert CB1 - CB2 goes high
	VIA_CYCLE_START
		SIGNAL_WRITE(CB1, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	return MUNIT_OK;
}

static MunitResult test_hs_portb_write_pulse(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ddrb = 0xff;		// all pins configured as output

	// configure CB2 to operate in pulse-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b10100000);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_pcr, ==, 0xa0);

	// write data to port-B - CB2 goes low to signal available data
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0x7f);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(CB2));
	munit_assert_uint8(SIGNAL_GROUP_READ_NEXT_U8(port_b), ==, 0x7f);

	// clock cycle - CB2 goes high again
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RW, true);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_GROUP_NO_WRITE(data);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB2));

	return MUNIT_OK;
}

static MunitResult test_read_ifr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ifr = 0x15;

	// read the IFR register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0x15);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x15);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0x15);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_ifr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// setup registers
	via->reg_ifr = 0b10111011;
	via->reg_ier = 0b01111111;

	// clear bits 1 & 2
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00000110);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0b10111001);
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	ASSERT_REGISTERS_DEFAULT(REG_IFR | REG_IER);

	// clear all bits
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01111111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ifr, ==, 0);
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	ASSERT_REGISTERS_DEFAULT(REG_IFR | REG_IER);

	return MUNIT_OK;
}

static MunitResult test_read_ier(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_ier = 0x15;

	// read the Interrupt Enable register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0x15);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x95);		// bit 7 is always set to 1

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0x15);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_ier(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// set a few bits
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b10000110);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00000110);
	ASSERT_REGISTERS_DEFAULT(REG_IER);

	// set some more bits
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b10001100);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00001110);
	ASSERT_REGISTERS_DEFAULT(REG_IER);

	// clear a bit
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00000010);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00001100);

	// clear all the bits
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01111111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_ier, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_irq_port(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// setup registers
	via->reg_ier = 0b00011011;	// enable

	///////////////////////////////////////////////////////////////////////////
	//
	// negative edge triggers interrupt (default)
	//

	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// negative edge triggers interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
		SIGNAL_WRITE(CA2, ACTLO_ASSERT);
		SIGNAL_WRITE(CB1, ACTLO_ASSERT);
		SIGNAL_WRITE(CB2, ACTLO_ASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 and ca2 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011000);

	// read ORB: clears cb1 and cb2 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	///////////////////////////////////////////////////////////////////////////
	//
	// independent negative edge triggers interrupt
	//

	// configure PCR
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00100010);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// negative edge triggers interrupt
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CA1, ACTLO_ASSERT);
		SIGNAL_WRITE(CA2, ACTLO_ASSERT);
		SIGNAL_WRITE(CB1, ACTLO_ASSERT);
		SIGNAL_WRITE(CB2, ACTLO_ASSERT);
		strobe_via(fixture, false);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, false);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011001);

	// read ORB: clears cb1 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10001001);

	// clear all the interrupts
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01111111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	///////////////////////////////////////////////////////////////////////////
	//
	// positive edge triggers interrupt
	//

	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CA2, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB1, ACTLO_DEASSERT);
		SIGNAL_WRITE(CB2, ACTLO_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01010101);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// clear all interrupts
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01111111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	// trigger interrupt
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CA1, ACTHI_ASSERT);
		SIGNAL_WRITE(CA2, ACTHI_ASSERT);
		SIGNAL_WRITE(CB1, ACTHI_ASSERT);
		SIGNAL_WRITE(CB2, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 and ca2 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011000);

	// read ORB: clears cb1 and cb2 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	// clear all the interrupts
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01111111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	///////////////////////////////////////////////////////////////////////////
	//
	// independent positive edge triggers interrupt
	//

	// configure PCR
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01110111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0);

	// clear all interrupts
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01111111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	// positive edge triggers interrupt
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CA1, ACTHI_ASSERT);
		SIGNAL_WRITE(CA2, ACTHI_ASSERT);
		SIGNAL_WRITE(CB1, ACTHI_ASSERT);
		SIGNAL_WRITE(CB2, ACTHI_ASSERT);
		SIGNAL_GROUP_NO_WRITE(data);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// deassert control lines, interrupt stays active
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011011);

	// read ORA: clears ca1 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10011001);

	// read ORB: clears cb1 interrupt
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10001001);

	// clear all the interrupts
	VIA_CYCLE_START
		SIGNAL_WRITE(CA1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CA2, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB1, ACTHI_DEASSERT);
		SIGNAL_WRITE(CB2, ACTHI_DEASSERT);
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01111111);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_once_nopb7(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00000000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 144);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 1);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 1);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// run until timer ends
	SIGNAL_WRITE(RW, true);
	SIGNAL_GROUP_NO_WRITE(data);
	strobe_via(fixture, false);

	for (int i = 400; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, i);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t1c, ==, 0xffff);
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b11000000);

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_freerun_nopb7(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b01000000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 50);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	SIGNAL_WRITE(RW, true);

	for (int j = 0; j < 3; ++j) {
		// clear t1 interrupt by reading t1c_l
		VIA_CYCLE_START
			strobe_via(fixture, true);					// enable via
			SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
			SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
			SIGNAL_WRITE(RS2, ACTHI_ASSERT);
			SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t1c, ==, 50);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

		// run until timer ends
		strobe_via(fixture, false);

		for (int i = 49; i >= 0; --i) {
			VIA_CYCLE();
			munit_assert_uint16(via->reg_t1c, ==, i);
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, 0xffff);
		munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_ifr, ==, 0b11000000);
	}

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_once_pb7(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1
	via->reg_ddrb = 0b10000000;

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b10000000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 144);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 1);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_false(SIGNAL_READ_NEXT(PB7));
	munit_assert_uint8(via->reg_t1l_l, ==, 144);
	munit_assert_uint8(via->reg_t1l_h, ==, 1);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// run until timer ends
	SIGNAL_WRITE(RW, true);
	strobe_via(fixture, false);

	for (int i = 400; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, i);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_false(SIGNAL_READ_NEXT(PB7));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t1c, ==, 0xffff);
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(SIGNAL_READ_NEXT(PB7));
	munit_assert_uint8(via->reg_ifr, ==, 0b11000000);

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t1_freerun_pb7(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;
	bool val_pb7 = false;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b01000000;					// enable irq on timer1
	via->reg_ddrb = 0b10000000;

	// setup t1-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b11000000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 50);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		SIGNAL_GROUP_WRITE(data, 0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t1l_l, ==, 50);
	munit_assert_uint8(via->reg_t1l_h, ==, 0);
	munit_assert_uint16(via->reg_t1c, ==, 0);
	munit_assert(SIGNAL_READ_NEXT(PB7) == val_pb7);

	SIGNAL_WRITE(RW, true);

	for (int j = 0; j < 3; ++j) {

		// clear t1 interrupt by reading t1c_l
		VIA_CYCLE_START
			strobe_via(fixture, true);					// enable via
			SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
			SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
			SIGNAL_WRITE(RS2, ACTHI_ASSERT);
			SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t1c, ==, 50);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

		// run until timer ends
		strobe_via(fixture, false);
		for (int i = 49; i >= 0; --i) {
			VIA_CYCLE();
			munit_assert_uint16(via->reg_t1c, ==, i);
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE();
		munit_assert_uint16(via->reg_t1c, ==, 0xffff);
		munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_ifr, ==, 0b11000000);

		// value of port_b[7] is inverted everytime the timer reaches zero
		val_pb7 = !val_pb7;
		munit_assert(SIGNAL_READ_NEXT(PB7) == val_pb7);
	}

	// clear t1 interrupt by reading t1c_l
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_ASSERT);
		SIGNAL_WRITE(RS3, ACTHI_DEASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);

	return MUNIT_OK;
}

static MunitResult test_t2_timed(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00100000;					// enable irq on timer2

	// setup t2-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00000000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 10);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// run until timer ends
	SIGNAL_WRITE(RW, true);
	strobe_via(fixture, false);

	for (int i = 10; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b10100000);

	// clear t2 interrupt by reading t2c_l
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);
	munit_assert_uint16(via->reg_t2c, ==, 0xfffe);

	// cycle until timer wraps again
	for (int i = 0xfffd; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));

	// write t2-h to re-enable irq
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);

	// run until timer ends
	SIGNAL_WRITE(RW, true);
	strobe_via(fixture, false);

	for (int i = 10; i >= 0; --i) {
		VIA_CYCLE();
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	}

	VIA_CYCLE();
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));

	return MUNIT_OK;
}

static MunitResult test_t2_count_pb6(const MunitParameter params[], void *user_data_or_fixture) {

	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00100000;					// enable irq on timer2

	// setup t2-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00100000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	// load low-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 10);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// load high-byte of timer counter
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, 10);
	munit_assert_uint16(via->reg_t2c, ==, 0);

	// cycle until timer is done
	SIGNAL_WRITE(RW, true);
	strobe_via(fixture, false);

	for (int i = 10; i > 0; --i) {
		VIA_CYCLE_START
			SIGNAL_GROUP_WRITE(port_b, 0b01000000);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t2c, ==, i);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));

		VIA_CYCLE_START
			SIGNAL_GROUP_WRITE(port_b, 0b00000000);
		VIA_CYCLE_END
		munit_assert_uint16(via->reg_t2c, ==, i - 1);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	}

	// cycle
	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_b, 0b01000000);
	VIA_CYCLE_END
	munit_assert_uint16(via->reg_t2c, ==, 0);
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));


	VIA_CYCLE_START
		SIGNAL_GROUP_WRITE(port_b, 0b00000000);
	VIA_CYCLE_END
	munit_assert_uint16(via->reg_t2c, ==, 0xffff);
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));

	// clear t2 interrupt by reading t2c_l
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_ifr, ==, 0b00000000);
	//munit_assert_uint16(via->reg_t2c, ==, 0xfffe);

	return MUNIT_OK;
}

static MunitResult test_read_sr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// initialize registers
	via->reg_sr = 0x15;

	// read the SR register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0x15);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x15);

	// try reading again from a disabled via, bus_data shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0x15);
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0x00);

	return MUNIT_OK;
}

static MunitResult test_write_sr(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// write to the SR register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0x09);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_SR);

	// try writing again with a disabled via, register shouldn't change
	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0x17);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END

	munit_assert_uint8(via->reg_sr, ==, 0x09);
	ASSERT_REGISTERS_DEFAULT(REG_SR);

	return MUNIT_OK;
}

static MunitResult test_sr_shift_in_phi2(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00000100;					// enable irq on shift register

	// setup sr-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00001000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));

	// read shift register to active shift in
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b00000000);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CB2, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b00000000);

	// shift in 8 bits
	uint8_t data_1 = 0b10110011;
	uint8_t done_1 = 0;

	for (int i = 0; i < 8; i++) {
		bool bit = BIT_IS_SET(data_1, 7 - i);

		VIA_CYCLE_START
			SIGNAL_WRITE(CB2, bit);
		VIA_CYCLE_END
		munit_assert_false(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_sr, ==, done_1);

		done_1 = (done_1 << 1) | bit;

		VIA_CYCLE_START
			SIGNAL_WRITE(CB2, bit);
		VIA_CYCLE_END
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_sr, ==, done_1);
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(via->reg_sr, ==, data_1);

	// read shift register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0b10110011);
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CB2, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);

	// shift in another 8-bits
	uint8_t data_2 = 0b11110000;
	uint8_t done_2 = 0b10110011;

	for (int i = 0; i < 8; i++) {
		bool bit = BIT_IS_SET(data_2, 7 - i);

		VIA_CYCLE_START
			SIGNAL_WRITE(CB2, bit);
		VIA_CYCLE_END
		munit_assert_false(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_sr, ==, done_2);

		done_2 = (done_2 << 1) | bit;

		VIA_CYCLE_START
			SIGNAL_WRITE(CB2, bit);
		VIA_CYCLE_END
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert_uint8(via->reg_sr, ==, done_2);
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(via->reg_sr, ==, data_2);

	return MUNIT_OK;
}

static MunitResult test_sr_shift_in_t2(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	const int sr_pulses = 4;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00000100;					// enable irq on shift register

	// setup t2 enough for the shift register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, sr_pulses);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, sr_pulses);

	// setup sr-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00000100);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));

	// read shift register to active shift in
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b00000000);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CB2, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b00000000);

	// shift in 8 bits
	uint8_t data_1 = 0b10110011;
	uint8_t done_1 = 0;

	for (int i = 0; i < 8; i++) {
		bool bit = BIT_IS_SET(data_1, 7 - i);

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_false(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_1);
		}

		done_1 = (done_1 << 1) | bit;

		for (int j = 0; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_1);
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(via->reg_sr, ==, data_1);

	// read shift register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0b10110011);
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CB2, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);

	// shift in another 8-bits
	uint8_t data_2 = 0b11110000;
	uint8_t done_2 = 0b10110011;

	for (int i = 0; i < 8; i++) {
		bool bit = BIT_IS_SET(data_2, 7 - i);

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_false(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_2);
		}

		done_2 = (done_2 << 1) | bit;

		for (int j = 0; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_2);
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(via->reg_sr, ==, data_2);

	return MUNIT_OK;
}

static MunitResult test_sr_shift_in_cb1(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	const int sr_pulses = 4;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00000100;					// enable irq on shift register

	// setup sr-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00001100);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, true);
	VIA_CYCLE_END

	// read shift register to active shift in
	VIA_CYCLE_START
		SIGNAL_GROUP_NO_WRITE(data);
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_WRITE(RW, true);
		SIGNAL_WRITE(CB1, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b00000000);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CB1, true);
		SIGNAL_WRITE(CB2, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b00000000);

	// shift in 8 bits
	uint8_t data_1 = 0b10110011;
	uint8_t done_1 = 0;

	for (int i = 0; i < 8; i++) {
		bool bit = BIT_IS_SET(data_1, 7 - i);

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, false);
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_1);
		}

		done_1 = (done_1 << 1) | bit;

		for (int j = 0; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, true);
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_1);
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(via->reg_sr, ==, data_1);

	// read shift register
	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_WRITE(CB1, true);
	VIA_CYCLE_END
	munit_assert_uint8(SIGNAL_GROUP_READ_U8(data), ==, 0b10110011);
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_WRITE(CB1, true);
		SIGNAL_WRITE(CB2, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);

	// shift in another 8-bits
	uint8_t data_2 = 0b11110000;
	uint8_t done_2 = 0b10110011;

	for (int i = 0; i < 8; i++) {
		bool bit = BIT_IS_SET(data_2, 7 - i);

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, false);
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_2);
		}

		done_2 = (done_2 << 1) | bit;

		for (int j = 0; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, true);
				SIGNAL_WRITE(CB2, bit);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
			munit_assert_uint8(via->reg_sr, ==, done_2);
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(via->reg_sr, ==, data_2);

	return MUNIT_OK;
}

static MunitResult test_sr_shift_out_phi2(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00000100;					// enable irq on shift register

	// setup sr-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00011000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));

	// write shift register to active shift
	uint8_t data_1 = 0b10110011;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_1);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB2, true);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);
	SIGNAL_NO_WRITE(CB2);

	// shift out 8 bits
	uint8_t done_1 = 0;

	for (int i = 0; i < 8; i++) {

		VIA_CYCLE();
		munit_assert_false(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));

		VIA_CYCLE();
		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_1, 7 - i));

		done_1 = (done_1 << 1) | bit;
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(done_1, ==, data_1);
	munit_assert_uint8(via->reg_sr, ==, data_1);

	// write shift register to active shift
	uint8_t data_2 = 0b11110000;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_2);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB2, true);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);
	SIGNAL_NO_WRITE(CB2);

	// shift out 8 bits
	uint8_t done_2 = 0;

	for (int i = 0; i < 8; i++) {

		VIA_CYCLE();
		munit_assert_false(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));

		VIA_CYCLE();
		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_2, 7 - i));

		done_2 = (done_2 << 1) | bit;
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(done_2, ==, data_2);
	munit_assert_uint8(via->reg_sr, ==, data_2);

	return MUNIT_OK;
}

static MunitResult test_sr_shift_out_t2(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	const int sr_pulses = 4;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00000100;					// enable irq on shift register

	// setup t2 enough for the shift register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, sr_pulses);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, sr_pulses);

	// setup sr-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00010100);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));

	// write shift register to active shift
	uint8_t data_1 = 0b10110011;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_1);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB2, true);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);
	SIGNAL_NO_WRITE(CB2);

	// shift out 8 bits
	uint8_t done_1 = 0;

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_false(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE();
		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_1, 7 - i));
		done_1 = (done_1 << 1) | bit;

		for (int j = 1; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE();
			munit_assert_true(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(done_1, ==, data_1);
	munit_assert_uint8(via->reg_sr, ==, data_1);

	// write shift register to active shift
	uint8_t data_2 = 0b11110000;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_2);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(RW, true);
		SIGNAL_WRITE(CB2, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);
	SIGNAL_NO_WRITE(CB2);

	// shift out 8 bits
	uint8_t done_2 = 0;

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_false(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE();
		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_2, 7 - i));
		done_2 = (done_2 << 1) | bit;

		for (int j = 1; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE();
			munit_assert_true(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(done_2, ==, data_2);
	munit_assert_uint8(via->reg_sr, ==, data_2);

	return MUNIT_OK;
}

static MunitResult test_sr_shift_out_t2_free(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	const int sr_pulses = 4;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00000100;					// enable irq on shift register

	// setup t2 enough for the shift register
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, sr_pulses);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_uint8(via->reg_t2l_l, ==, sr_pulses);

	// setup sr-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00010000);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));

	// write shift register to active shift
	uint8_t data_1 = 0b10110011;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_1);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB2, true);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);
	SIGNAL_NO_WRITE(CB2);

	// shift out 8 bits
	uint8_t done_1 = 0;

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_false(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE();
		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_1, 7 - i));
		done_1 = (done_1 << 1) | bit;

		for (int j = 1; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_true(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}
	}

	munit_assert_uint8(done_1, ==, data_1);

	// shift out the same 8 bits
	uint8_t done_1b = 0;

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_false(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE();
		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_1, 7 - i));
		done_1b = (done_1b << 1) | bit;

		for (int j = 1; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_true(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}
	}

	munit_assert_uint8(done_1b, ==, data_1);

	// write shift register to change output
	uint8_t data_2 = 0b11110000;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_2);
		SIGNAL_WRITE(RW, false);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB2, true);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_true(SIGNAL_READ_NEXT(CB1));
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);
	SIGNAL_NO_WRITE(CB2);

	// shift out 8 bits
	uint8_t done_2 = 0;

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_false(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE();
		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(CB1));
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_2, 7 - i));
		done_2 = (done_2 << 1) | bit;

		for (int j = 1; j < sr_pulses; ++j) {
			VIA_CYCLE();
			munit_assert_true(SIGNAL_READ_NEXT(CB1));
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}
	}

	munit_assert_uint8(done_2, ==, data_2);

	return MUNIT_OK;
}

static MunitResult test_sr_shift_out_cb1(const MunitParameter params[], void *user_data_or_fixture) {
	TestFixture6522 *fixture = (TestFixture6522 *) user_data_or_fixture;
	Chip6522 *via = fixture->via;

	const int sr_pulses = 4;

	// assert initial state
	ASSERT_REGISTERS_DEFAULT(0);

	// initialize registers
	via->reg_ier = 0b00000100;					// enable irq on shift register

	// setup sr-mode
	VIA_CYCLE_START
		strobe_via(fixture, true);					// enable via
		SIGNAL_WRITE(RS0, ACTHI_ASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, 0b00011100);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, true);
	VIA_CYCLE_END

	// write shift register to active shift
	uint8_t data_1 = 0b10110011;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_1);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(CB1, true);
		SIGNAL_WRITE(CB2, true);
		SIGNAL_WRITE(RW, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b10110011);
	SIGNAL_NO_WRITE(CB2);

	// shift out 8 bits
	uint8_t done_1 = 0;

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, false);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE_START
			SIGNAL_WRITE(CB1, true);
		VIA_CYCLE_END

		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_1, 7 - i));
		done_1 = (done_1 << 1) | bit;

		for (int j = 1; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, true);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(done_1, ==, data_1);
	munit_assert_uint8(via->reg_sr, ==, data_1);

	// write shift register to active shift
	uint8_t data_2 = 0b11110000;

	VIA_CYCLE_START
		strobe_via(fixture, true);
		SIGNAL_WRITE(RS0, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS1, ACTHI_ASSERT);
		SIGNAL_WRITE(RS2, ACTHI_DEASSERT);
		SIGNAL_WRITE(RS3, ACTHI_ASSERT);
		SIGNAL_GROUP_WRITE(data, data_2);
		SIGNAL_WRITE(RW, false);
		SIGNAL_WRITE(CB1, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);

	VIA_CYCLE_START
		strobe_via(fixture, false);
		SIGNAL_GROUP_NO_WRITE(data);
		SIGNAL_WRITE(RW, true);
		SIGNAL_WRITE(CB1, true);
	VIA_CYCLE_END
	munit_assert_uint8(via->reg_sr, ==, 0b11110000);

	// shift out 8 bits
	uint8_t done_2 = 0;

	for (int i = 0; i < 8; i++) {

		for (int j = 0; j < sr_pulses; ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, false);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}

		VIA_CYCLE_START
			SIGNAL_WRITE(CB1, true);
		VIA_CYCLE_END

		bool bit = SIGNAL_READ_NEXT(CB2);
		munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		munit_assert(bit == BIT_IS_SET(data_2, 7 - i));
		done_2 = (done_2 << 1) | bit;

		for (int j = 1; j < ((i < 7) ? sr_pulses : 1); ++j) {
			VIA_CYCLE_START
				SIGNAL_WRITE(CB1, true);
			VIA_CYCLE_END
			munit_assert_true(SIGNAL_READ_NEXT(IRQ_B));
		}
	}

	// check irq
	VIA_CYCLE();
	munit_assert_false(SIGNAL_READ_NEXT(IRQ_B));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 7));
	munit_assert_true(BIT_IS_SET(via->reg_ifr, 2));
	munit_assert_uint8(done_2, ==, data_2);
	munit_assert_uint8(via->reg_sr, ==, data_2);

	return MUNIT_OK;
}

MunitTest chip_6522_tests[] = {
	{ "/create", test_create, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/reset", test_reset, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ddra", test_read_ddra, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/read_ddrb", test_read_ddrb, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddra", test_write_ddra, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_ddrb", test_write_ddrb, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
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
	{ "/read_sr", test_read_sr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/write_sr", test_write_sr, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sr_shift_in_phi2", test_sr_shift_in_phi2, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sr_shift_in_t2", test_sr_shift_in_t2, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sr_shift_in_cb1", test_sr_shift_in_cb1, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sr_shift_out_phi2", test_sr_shift_out_phi2, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sr_shift_out_t2", test_sr_shift_out_t2, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sr_shift_out_t2_free", test_sr_shift_out_t2_free, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ "/sr_shift_out_cb1", test_sr_shift_out_cb1, chip_6522_setup, chip_6522_teardown, MUNIT_TEST_OPTION_NONE, NULL },
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
