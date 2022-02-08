// test/fixture_chip.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Test fixture that includes a chip to register the writes of the unit-test

#include "fixture_chip.h"
#include "chip.h"
#include "simulator.h"
#include "signal_pool.h"

#include <assert.h>
#include <stdlib.h>

static void chip_tester_destroy(Chip *chip) {
	assert(chip);
	free(chip);
}

static void chip_tester_process(Chip *chip) {
	assert(chip);
}

TestFixture *fixture_chip_create() {
	TestFixture *fixture = (TestFixture *) calloc(1, sizeof(TestFixture));
	fixture->simulator = simulator_create(NS_TO_PS(100));
	signal_pool_set_layer_count(fixture->simulator->signal_pool, 2);
	return fixture;
}

void fixture_chip_destroy(void *test_fixture) {
	TestFixture *fixture = (TestFixture *) test_fixture;
	simulator_destroy(fixture->simulator);
	free(fixture);
}

void fixture_chip_create_tester(TestFixture *fixture) {
	fixture->chip_tester = (Chip *) calloc(1, sizeof(Chip));

	CHIP_SET_FUNCTIONS(fixture->chip_tester, chip_tester_process, chip_tester_destroy);
	fixture->chip_tester->simulator = fixture->simulator;
	fixture->chip_tester->pin_count = fixture->chip_under_test->pin_count;
	fixture->chip_tester->pins = (Signal *) malloc(sizeof(Signal) * fixture->chip_tester->pin_count);
	memcpy(fixture->chip_tester->pins, fixture->chip_under_test->pins, sizeof(Signal) * fixture->chip_tester->pin_count);
	fixture->chip_tester->pin_types = (uint8_t *) malloc(sizeof(uint8_t) * fixture->chip_tester->pin_count);
	memset(fixture->chip_tester->pin_types, CHIP_PIN_OUTPUT, fixture->chip_tester->pin_count);
	simulator_register_chip(fixture->simulator, fixture->chip_tester, "tester");

	simulator_device_complete(fixture->simulator);
}
