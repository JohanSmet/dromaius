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

typedef struct Chip6520_pstate {			// port state
	bool			prev_cl1;
	bool			prev_cl2;
	bool			act_trans_cl1;
	bool			act_trans_cl2;
	bool			read_port;
	bool			write_port;
} Chip6520_pstate;

typedef struct Chip6520_private {
	Chip6520		intf;

	bool			strobe;
	bool			prev_enable;

	Chip6520_pstate	state_a;
	Chip6520_pstate	state_b;

	bool			internal_ca2;

	bool			out_irqa_b;
	bool			out_irqb_b;
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
				PRIVATE(pia)->state_a.write_port = true;
			} else {
				pia->reg_ddra = data;
			}
			break;
		case 1:
			pia->reg_cra.reg = (pia->reg_cra.reg & 0b11000000) | (data & 0b00111111);
			if (pia->reg_cra.bf_cl2_mode_select == 1 && pia->reg_cra.bf_cl2_output == 0) {
				PRIVATE(pia)->internal_ca2 = true;
			}
			break;
		case 2:
			if (pia->reg_crb.bf_ddr_or_select) {
				pia->reg_orb = data;
				PRIVATE(pia)->state_b.write_port = true;
			} else {
				pia->reg_ddrb = data;
			}
			break;
		case 3:
			pia->reg_crb.reg = (pia->reg_crb.reg & 0b11000000) | (data & 0b00111111);
			break;
	}
}

static inline uint8_t read_register(Chip6520 *pia) {
	int reg_addr = (SIGNAL_BOOL(rs1) << 1) | SIGNAL_BOOL(rs0);

	switch (reg_addr) {
		case 0:
			if (pia->reg_cra.bf_ddr_or_select) {
				PRIVATE(pia)->state_a.read_port = true;
				return SIGNAL_UINT8(port_a);
			} else {
				return pia->reg_ddra;
			}
		case 1:
			return pia->reg_cra.reg;
		case 2:
			if (pia->reg_crb.bf_ddr_or_select) {
				PRIVATE(pia)->state_b.read_port = true;
				return SIGNAL_UINT8(port_b);
			} else {
				return pia->reg_ddrb;
			}
		case 3:
			return pia->reg_crb.reg;
	}

	return 0;
}

static inline void control_register_irq_routine(ctrl_reg_t *reg_ctrl, bool cl1, bool cl2, Chip6520_pstate *state) {

	// check for active transition of the control lines
	state->act_trans_cl1 = (cl1 && !state->prev_cl1 && reg_ctrl->bf_irq1_pos_transition) ||
	                       (!cl1 && state->prev_cl1 && !reg_ctrl->bf_irq1_pos_transition);
	state->act_trans_cl2 = reg_ctrl->bf_cl2_mode_select == 0 &&
						   ((cl2 && !state->prev_cl2 && reg_ctrl->bf_irq2_pos_transition) ||
	                        (!cl2 && state->prev_cl2 && !reg_ctrl->bf_irq2_pos_transition));

	// a read of the peripheral I/O port resets both irq flags in reg_ctrl
	if (state->read_port) {
		reg_ctrl->bf_irq1 = ACTHI_DEASSERT;
		reg_ctrl->bf_irq2 = ACTHI_DEASSERT;
	}

	// an active transition of the cl1/cl2-lines sets the irq-flags in reg_ctrl
	reg_ctrl->bf_irq1 = reg_ctrl->bf_irq1 | state->act_trans_cl1;
	reg_ctrl->bf_irq2 = reg_ctrl->bf_irq2 | state->act_trans_cl2;

	// store state of interrupt input/peripheral control lines
	state->prev_cl1 = cl1;
	state->prev_cl2 = cl2;
}

static inline void process_positive_enable_edge(Chip6520 *pia) {
	if (PRIVATE(pia)->strobe && SIGNAL_BOOL(rw) == RW_READ) {
		SIGNAL_SET_UINT8(bus_data, read_register(pia));
	}
}

void process_negative_enable_edge(Chip6520 *pia) {

	if (PRIVATE(pia)->strobe) {
		// read/write internal register
		if (SIGNAL_BOOL(rw) == RW_WRITE) {
			write_register(pia, SIGNAL_UINT8(bus_data));
		} else {
			SIGNAL_SET_UINT8(bus_data, read_register(pia));
		}
	}

	// irq-A routine 
	control_register_irq_routine(
			&pia->reg_cra, 
			SIGNAL_BOOL(ca1), SIGNAL_BOOL(ca2),
			&PRIVATE(pia)->state_a);

	// irq-B routine (FIXME pin_cb2 output mode)
	control_register_irq_routine(
			&pia->reg_crb, 
			SIGNAL_BOOL(cb1), SIGNAL_BOOL(cb2),
			&PRIVATE(pia)->state_b);

	// irq output lines
	PRIVATE(pia)->out_irqa_b = !((pia->reg_cra.bf_irq1 && pia->reg_cra.bf_irq1_enable) ||
							     (pia->reg_cra.bf_irq2 && pia->reg_cra.bf_irq2_enable));
	PRIVATE(pia)->out_irqb_b = !((pia->reg_crb.bf_irq1 && pia->reg_crb.bf_irq1_enable) ||
							     (pia->reg_crb.bf_irq2 && pia->reg_crb.bf_irq2_enable));

	// pin_ca2 output mode
	if (pia->reg_cra.bf_cl2_mode_select == 1) {
		if (pia->reg_cra.bf_cl2_output == 1) {
			// ca2 follows the value written to bit 3 of cra (ca2 restore)
			PRIVATE(pia)->internal_ca2 = pia->reg_cra.bf_cl2_restore == 1;
		} else if (PRIVATE(pia)->state_a.read_port) {
			// ca2 goes low when porta is read, returning high depends on ca2-restore
			PRIVATE(pia)->internal_ca2 = 0;
		} else if (pia->reg_cra.bf_cl2_restore == 1) {
			// return high on the next cycle
			PRIVATE(pia)->internal_ca2 = 1;
		} else {
			// return high on next active CA1 transition
			PRIVATE(pia)->internal_ca2 = PRIVATE(pia)->internal_ca2 | PRIVATE(pia)->state_a.act_trans_cl1;
		}
	}
}

static inline void process_end(Chip6520 *pia) {

	// always write to the non-tristate outputs
	SIGNAL_SET_BOOL(irqa_b, PRIVATE(pia)->out_irqa_b);
	SIGNAL_SET_BOOL(irqb_b, PRIVATE(pia)->out_irqb_b);

	// output on the ports
	signal_write_uint8_masked(pia->signal_pool, SIGNAL(port_a), pia->reg_ora, pia->reg_ddra);
	signal_write_uint8_masked(pia->signal_pool, SIGNAL(port_b), pia->reg_orb, pia->reg_ddrb);

	// output on ca2 if in output mode
	if (pia->reg_cra.bf_cl2_mode_select) {
		SIGNAL_SET_BOOL(ca2, PRIVATE(pia)->internal_ca2);
	}

	// store state of the enable pin
	PRIVATE(pia)->prev_enable = SIGNAL_BOOL(enable);
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
	SIGNAL_DEFINE_BOOL(enable,	1, true);
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

	// reset flags
	PRIVATE(pia)->state_a.read_port = false;
	PRIVATE(pia)->state_a.write_port = false;
	PRIVATE(pia)->state_b.read_port = false;
	PRIVATE(pia)->state_b.write_port = false;

	if (ACTLO_ASSERTED(reset_b)) {
		// reset is asserted - clear registers
		pia->reg_ddra = 0;
		pia->reg_cra.reg = 0;
		pia->reg_ora = 0;
		pia->reg_ddrb = 0;
		pia->reg_crb.reg = 0;
		pia->reg_orb = 0;
		PRIVATE(pia)->out_irqa_b = ACTLO_DEASSERT;
		PRIVATE(pia)->out_irqb_b = ACTLO_DEASSERT;
	}

	// do nothing:
	//	- if reset is asserted or 
	//  - if not on the edge of a clock cycle
	bool enable = SIGNAL_BOOL(enable);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(reset_b)) || enable == PRIVATE(pia)->prev_enable) {
		process_end(pia);
		return;
	}

	PRIVATE(pia)->strobe = ACTHI_ASSERTED(SIGNAL_BOOL(cs0)) && ACTHI_ASSERTED(SIGNAL_BOOL(cs1)) && ACTLO_ASSERTED(SIGNAL_BOOL(cs2_b));

	if (enable) {
		process_positive_enable_edge(pia);
	} else {
		process_negative_enable_edge(pia);
	}

	process_end(pia);
}
