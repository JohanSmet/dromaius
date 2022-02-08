// test/fixture_chip.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Test fixture that includes a chip to register the writes of the unit-test

#ifndef DROMAIUS_TEST_FIXTURE_CHIP_H
#define DROMAIUS_TEST_FIXTURE_CHIP_H

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct TestFixture {
	struct Simulator *	simulator;
	struct Chip *		chip_under_test;
	struct Chip *		chip_tester;
} TestFixture;

TestFixture *fixture_chip_create();
void fixture_chip_destroy(void *fixture);
void fixture_chip_create_tester(TestFixture *fixture);

#undef SIGNAL_COLLECTION
#define SIGNAL_COLLECTION	SIGNAL_OWNER->pins

#undef SIGNAL_POOL
#define SIGNAL_POOL			SIGNAL_OWNER->simulator->signal_pool


#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_TEST_FIXTURE_CHIP_H
