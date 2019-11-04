// cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the MOS 6502

#include "cpu_6502.h"

#include <assert.h>
#include <stdlib.h>

typedef struct Cpu6502_private {
	Cpu6502		intf;
	bool		prev_reset;
	uint16_t	internal_ab;			// internal address bus
} Cpu6502_private;

Cpu6502 *cpu_6502_create(uint16_t *addres_bus, uint8_t *data_bus, const bool *clock, const bool *reset, bool *rw) {
	assert(addres_bus);
	assert(data_bus);
	assert(clock);
	assert(reset);
	assert(rw);

	Cpu6502_private *cpu = (Cpu6502_private *) malloc(sizeof(Cpu6502_private));

	cpu->intf.bus_address = addres_bus;
	cpu->intf.bus_data = data_bus;
	cpu->intf.pin_clock = clock;
	cpu->intf.pin_reset = reset;
	cpu->intf.pin_rw = rw;
	
	return &cpu->intf;
}

void cpu_6502_process(Cpu6502 *cpu) {
	assert(cpu);
	Cpu6502_private *priv = (Cpu6502_private *) cpu;

	// handle reset line
	if (!priv->prev_reset && cpu->pin_reset) {
		// reset was just asserted
		priv->internal_ab = 0;
		priv->prev_reset = cpu->pin_reset;
	} else if (priv->prev_reset && !cpu->pin_reset) {
		// reset was just deasserted
		priv->internal_ab = 0;
		priv->prev_reset = cpu->pin_reset;
	}

	// always write to the output pins that aren't tristate
	*cpu->bus_address = priv->internal_ab;
}
