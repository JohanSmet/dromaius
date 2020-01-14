// chip_6520.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the 6520 Peripheral Interface Adapter

#include "chip_6520.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_POOL			pia->signal_pool
#define SIGNAL_COLLECTION	pia->signals

//////////////////////////////////////////////////////////////////////////////
//
// internal types
//

typedef struct Chip6520_private {
	Chip6520		intf;

	bool			prev_clock;
} Chip6520_private;


//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(pia)	((Chip6520_private *) (pia))

void process_end(Chip6520 *pia) {

	// store state of the clock pin
	PRIVATE(pia)->prev_clock = SIGNAL_BOOL(clock);
}

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Chip6520 *chip_6520_create(SignalPool *signal_pool, Chip6520Signals signals) {
	Chip6520_private *priv = (Chip6520_private *) calloc(1, sizeof(Chip6520_private));

	Chip6520 *pia = &priv->intf;
	pia->signal_pool = signal_pool;

	memcpy(&pia->signals, &signals, sizeof(signals));
	SIGNAL_DEFINE(bus_data,		8);
	SIGNAL_DEFINE(port_a,		8);
	SIGNAL_DEFINE(port_b,		8);
	SIGNAL_DEFINE_BOOL(ca1,		1, false);
	SIGNAL_DEFINE_BOOL(ca2,		1, false);
	SIGNAL_DEFINE_BOOL(cb1,		1, false);
	SIGNAL_DEFINE_BOOL(cb2,		1, false);
	SIGNAL_DEFINE_BOOL(irqa_b,	1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(irqb_b,	1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(rs0,		1, ACTHI_DEASSERT);
	SIGNAL_DEFINE_BOOL(rs1,		1, ACTHI_DEASSERT);
	SIGNAL_DEFINE_BOOL(reset_b, 1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(clock,	1, true);
	SIGNAL_DEFINE_BOOL(cs0,		1, ACTHI_DEASSERT);
	SIGNAL_DEFINE_BOOL(cs1,		1, ACTHI_DEASSERT);
	SIGNAL_DEFINE_BOOL(cs2_b,	1, ACTLO_DEASSERT);
	SIGNAL_DEFINE_BOOL(rw,		1, true);

	return pia;
}

void chip_6520_destroy(Chip6520 *pia) {
	assert(pia);
	free(PRIVATE(pia));
}

void chip_6520_process(Chip6520 *pia) {
	assert(pia);

	bool reset_b = SIGNAL_BOOL(reset_b);

	if (ACTLO_ASSERTED(reset_b)) {
		// reset is asserted - clear registers
		pia->reg_ddra = 0;
		pia->reg_cra.reg = 0;
		pia->reg_ora = 0;
		pia->reg_ddrb = 0;
		pia->reg_crb.reg = 0;
		pia->reg_orb = 0;
	}

	// do nothing:
	//	- if reset is asserted or 
	//	- if cs0/cs1/cs2_b are not asserted
	//  - if not on the edge of a clock cycle
	bool clock = SIGNAL_BOOL(clock);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(reset_b)) ||
		ACTHI_ASSERTED(SIGNAL_BOOL(cs0)) || ACTHI_ASSERTED(SIGNAL_BOOL(cs1)) || ACTLO_ASSERTED(SIGNAL_BOOL(cs2_b)) ||
		clock == PRIVATE(pia)->prev_clock) {
		process_end(pia);
		return;
	}

	process_end(pia);
}
