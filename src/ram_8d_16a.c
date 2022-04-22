// ram_8d_16a.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of a memory module with an 8-bit wide databus and a maximum of 16 datalines (64kb)

#include "ram_8d_16a.h"
#include "simulator.h"
#include "crt.h"

#define SIGNAL_PREFIX		CHIP_RAM8D16A_
#define SIGNAL_OWNER		ram

static uint8_t Ram8d16a_PinTypes[CHIP_RAM8D16A_PIN_COUNT] = {
	[CHIP_RAM8D16A_CE_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_WE_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_OE_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A0  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A1  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A2  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A3  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A4  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A5  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A6  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A7  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A8  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A9  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A10 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A11 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A12 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A13 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A14 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_A15 ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D0  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D1  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D2  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D3  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D4  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D5  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D6  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
	[CHIP_RAM8D16A_D7  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT | CHIP_PIN_TRIGGER,
};

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void ram_8d16a_destroy(Ram8d16a *ram);
static void ram_8d16a_process(Ram8d16a *ram);

Ram8d16a *ram_8d16a_create(uint8_t num_address_lines, Simulator *sim, Ram8d16aSignals signals) {
	assert(num_address_lines > 0 && num_address_lines <= 16);

	size_t data_size = (size_t) 1 << num_address_lines;
	Ram8d16a *ram = (Ram8d16a *) dms_calloc(1, sizeof(Ram8d16a) + data_size);

	CHIP_SET_FUNCTIONS(ram, ram_8d16a_process, ram_8d16a_destroy);
	CHIP_SET_VARIABLES(ram, sim, ram->signals, Ram8d16a_PinTypes, CHIP_RAM8D16A_PIN_COUNT);

	ram->signal_pool = sim->signal_pool;
	ram->data_size = data_size;

	dms_memcpy(ram->signals, signals, sizeof(Ram8d16aSignals));

	ram->sg_address = signal_group_create();
	ram->sg_data = signal_group_create();

	for (int i = 0; i < num_address_lines; ++i) {
		SIGNAL_DEFINE_GROUP(A0 + i, address);
	}

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(D0 + i, data);
	}

	SIGNAL_DEFINE(CE_B);
	SIGNAL_DEFINE(WE_B);
	SIGNAL_DEFINE(OE_B);

	// init cache variables
	ram->last_data = -1;

	return ram;
}

static void ram_8d16a_destroy(Ram8d16a *ram) {
	assert(ram);
	signal_group_destroy(ram->sg_address);
	signal_group_destroy(ram->sg_data);
	dms_free(ram);
}

static void ram_8d16a_process(Ram8d16a *ram) {
	assert(ram);

	if (!ACTLO_ASSERTED(SIGNAL_READ(CE_B))) {
		if (SIGNAL_CHANGED(CE_B)) {
			SIGNAL_GROUP_NO_WRITE(data);
			ram->last_data = -1;
		}
		return;
	}

	uint16_t address = SIGNAL_GROUP_READ_U16(address);

	if (ACTLO_ASSERTED(SIGNAL_READ(OE_B))) {
		if (ram->data_array[address] != ram->last_data) {
			SIGNAL_GROUP_WRITE(data, ram->data_array[address]);
			ram->last_data = ram->data_array[address];
		}
	} else {
		if (SIGNAL_CHANGED(OE_B)) {
			SIGNAL_GROUP_NO_WRITE(data);
			ram->last_data = -1;
		}
		if (ACTLO_ASSERTED(SIGNAL_READ(WE_B))) {
			ram->data_array[address] = SIGNAL_GROUP_READ_U8(data);
		}
	}
}
