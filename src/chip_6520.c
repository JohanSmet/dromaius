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

#define RW_READ  true
#define RW_WRITE false


//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(pia)	((Chip6520_private *) (pia))

static inline void write_register(Chip6520 *pia, uint8_t data) {
	int reg_addr = (SIGNAL_BOOL(rs1) << 1) | SIGNAL_BOOL(rs0);

	switch (reg_addr) {
		case 0:
			if (pia->reg_cra.bf_ddr_or_select) {
				pia->reg_ora = data;
			} else {
				pia->reg_ddra = data;
			}
			break;
		case 1:
			pia->reg_cra.reg = data;
			break;
		case 2:
			if (pia->reg_crb.bf_ddr_or_select) {
				pia->reg_orb = data;
			} else {
				pia->reg_ddrb = data;
			}
			break;
		case 3:
			pia->reg_crb.reg = data;
			break;
	}
}

static inline uint8_t read_register(Chip6520 *pia) {
	int reg_addr = (SIGNAL_BOOL(rs1) << 1) | SIGNAL_BOOL(rs0);

	switch (reg_addr) {
		case 0:
			if (pia->reg_cra.bf_ddr_or_select) {
				return SIGNAL_UINT8(port_a);
			} else {
				return pia->reg_ddra;
			}
		case 1:
			return pia->reg_cra.reg;
		case 2:
			if (pia->reg_crb.bf_ddr_or_select) {
				return SIGNAL_UINT8(port_b);
			} else {
				return pia->reg_ddrb;
			}
		case 3:
			return pia->reg_crb.reg;
	}
}

static inline void process_positive_clock_edge(Chip6520 *pia) {

}

void process_negative_clock_edge(Chip6520 *pia) {
	if (SIGNAL_BOOL(rw) == RW_WRITE) {
		write_register(pia, SIGNAL_UINT8(bus_data));
	} else {
		SIGNAL_SET_UINT8(bus_data, read_register(pia));
	}
}

static inline void process_end(Chip6520 *pia) {

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
		!ACTHI_ASSERTED(SIGNAL_BOOL(cs0)) || !ACTHI_ASSERTED(SIGNAL_BOOL(cs1)) || !ACTLO_ASSERTED(SIGNAL_BOOL(cs2_b)) ||
		clock == PRIVATE(pia)->prev_clock) {
		process_end(pia);
		return;
	}

	if (clock) {
		process_positive_clock_edge(pia);
	} else {
		process_negative_clock_edge(pia);
	}

	process_end(pia);
}
