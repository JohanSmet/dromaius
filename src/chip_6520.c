// chip_6521.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the 6520 Peripheral Interface Adapter

#include "chip_6520.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

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
	bool			internal_cb2;

	bool			out_irqa_b;
	bool			out_irqb_b;

	bool			out_enabled;
	uint8_t			out_data;
} Chip6520_private;

#define RW_READ  true
#define RW_WRITE false


//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(pia)	((Chip6520_private *) (pia))

#define CR_FLAG(r,f)	FLAG_IS_SET((r), FLAG_6520_##f)
#define CR_CHANGE_FLAG(reg, flag, cond)			\
	FLAG_SET_CLEAR_U8(reg, FLAG_6520_##flag, cond)

static inline void write_register(Chip6520 *pia, uint8_t data) {
	int reg_addr = (SIGNAL_BOOL(rs1) << 1) | SIGNAL_BOOL(rs0);

	switch (reg_addr) {
		case 0:
			if (CR_FLAG(pia->reg_cra, DDR_OR_SELECT)) {
				pia->reg_ora = data;
				PRIVATE(pia)->state_a.write_port = true;
			} else {
				pia->reg_ddra = data;
			}
			break;
		case 1:
			pia->reg_cra = (uint8_t)((pia->reg_cra & 0b11000011) | (data & 0b00111111));
			if (CR_FLAG(pia->reg_cra, CL2_MODE_SELECT) && !CR_FLAG(pia->reg_cra, CL2_OUTPUT)) {
				PRIVATE(pia)->internal_ca2 = true;
			}
			break;
		case 2:
			if (CR_FLAG(pia->reg_crb, DDR_OR_SELECT)) {
				pia->reg_orb = data;
				PRIVATE(pia)->state_b.write_port = true;
			} else {
				pia->reg_ddrb = data;
			}
			break;
		case 3:
			pia->reg_crb = (uint8_t)((pia->reg_crb & 0b11000000) | (data & 0b00111111));
			if (CR_FLAG(pia->reg_crb, CL2_MODE_SELECT) && !CR_FLAG(pia->reg_crb, CL2_OUTPUT)) {
				PRIVATE(pia)->internal_cb2 = true;
			}
			break;
	}
}

static inline uint8_t read_register(Chip6520 *pia) {
	int reg_addr = (SIGNAL_BOOL(rs1) << 1) | SIGNAL_BOOL(rs0);

	switch (reg_addr) {
		case 0:
			if (CR_FLAG(pia->reg_cra, DDR_OR_SELECT)) {
				PRIVATE(pia)->state_a.read_port = true;
				return SIGNAL_UINT8(port_a);
			} else {
				return pia->reg_ddra;
			}
		case 1:
			return pia->reg_cra;
		case 2:
			if (CR_FLAG(pia->reg_crb, DDR_OR_SELECT)) {
				PRIVATE(pia)->state_b.read_port = true;
				return SIGNAL_UINT8(port_b);
			} else {
				return pia->reg_ddrb;
			}
		case 3:
			return pia->reg_crb;
	}

	return 0;
}

static inline void control_register_irq_routine(uint8_t *reg_ctrl, bool cl1, bool cl2, Chip6520_pstate *state) {

	// check for active transition of the control lines
	bool irq1_pt = CR_FLAG(*reg_ctrl, IRQ1_POS_TRANSITION);
	state->act_trans_cl1 = (cl1 && !state->prev_cl1 && irq1_pt) || (!cl1 && state->prev_cl1 && !irq1_pt);

	bool irq2_pt = CR_FLAG(*reg_ctrl, IRQ2_POS_TRANSITION);
	state->act_trans_cl2 = !CR_FLAG(*reg_ctrl, CL2_MODE_SELECT) &&
						   ((cl2 && !state->prev_cl2 && irq2_pt) || (!cl2 && state->prev_cl2 && !irq2_pt));

	// a read of the peripheral I/O port resets both irq flags in reg_ctrl
	if (state->read_port) {
		CR_CHANGE_FLAG(*reg_ctrl, IRQ1, ACTHI_DEASSERT);
		CR_CHANGE_FLAG(*reg_ctrl, IRQ2, ACTHI_DEASSERT);
	}

	// an active transition of the cl1/cl2-lines sets the irq-flags in reg_ctrl
	*reg_ctrl = (uint8_t) (*reg_ctrl | (-(int8_t) state->act_trans_cl1 & FLAG_6520_IRQ1)
								     | (-(int8_t) state->act_trans_cl2 & FLAG_6520_IRQ2));

	// store state of interrupt input/peripheral control lines
	state->prev_cl1 = cl1;
	state->prev_cl2 = cl2;
}

static inline void process_positive_enable_edge(Chip6520 *pia) {
	if (PRIVATE(pia)->strobe && SIGNAL_BOOL(rw) == RW_READ) {
		PRIVATE(pia)->out_data = read_register(pia);
		PRIVATE(pia)->out_enabled = true;
	}
}

void process_negative_enable_edge(Chip6520 *pia) {

	if (PRIVATE(pia)->strobe) {
		// read/write internal register
		if (SIGNAL_BOOL(rw) == RW_WRITE) {
			write_register(pia, SIGNAL_UINT8(bus_data));
		} else {
			PRIVATE(pia)->out_data = read_register(pia);
			PRIVATE(pia)->out_enabled = true;
		}
	}

	// irq-A routine
	control_register_irq_routine(
			&pia->reg_cra,
			SIGNAL_BOOL(ca1), SIGNAL_BOOL(ca2),
			&PRIVATE(pia)->state_a);

	// irq-B routine
	control_register_irq_routine(
			&pia->reg_crb,
			SIGNAL_BOOL(cb1), SIGNAL_BOOL(cb2),
			&PRIVATE(pia)->state_b);

	// irq output lines
	PRIVATE(pia)->out_irqa_b = !((CR_FLAG(pia->reg_cra, IRQ1) && CR_FLAG(pia->reg_cra, IRQ1_ENABLE)) ||
								 (CR_FLAG(pia->reg_cra, IRQ2) && CR_FLAG(pia->reg_cra, IRQ2_ENABLE)));
	PRIVATE(pia)->out_irqb_b = !((CR_FLAG(pia->reg_crb, IRQ1) && CR_FLAG(pia->reg_crb, IRQ1_ENABLE)) ||
								 (CR_FLAG(pia->reg_crb, IRQ2) && CR_FLAG(pia->reg_crb, IRQ2_ENABLE)));

	// pin_ca2 output mode
	if (CR_FLAG(pia->reg_cra, CL2_MODE_SELECT)) {
		if (CR_FLAG(pia->reg_cra, CL2_OUTPUT)) {
			// ca2 follows the value written to bit 3 of cra (ca2 restore)
			PRIVATE(pia)->internal_ca2 = CR_FLAG(pia->reg_cra, CL2_RESTORE);
		} else if (PRIVATE(pia)->state_a.read_port) {
			// ca2 goes low when porta is read, returning high depends on ca2-restore
			PRIVATE(pia)->internal_ca2 = 0;
		} else if (CR_FLAG(pia->reg_cra, CL2_RESTORE)) {
			// return high on the next cycle
			PRIVATE(pia)->internal_ca2 = 1;
		} else {
			// return high on next active CA1 transition
			PRIVATE(pia)->internal_ca2 = PRIVATE(pia)->internal_ca2 | PRIVATE(pia)->state_a.act_trans_cl1;
		}
	}

	// pin_cb2 output mode
	if (CR_FLAG(pia->reg_crb, CL2_MODE_SELECT)) {
		if (CR_FLAG(pia->reg_crb, CL2_OUTPUT)) {
			// cb2 follows the value written to bit 3 of crb (cb2 restore)
			PRIVATE(pia)->internal_cb2 = CR_FLAG(pia->reg_crb, CL2_RESTORE);
		} else if (PRIVATE(pia)->state_b.write_port) {
			// ca2 goes low when port-B is written, returning high depends on cb2-restore
			PRIVATE(pia)->internal_cb2 = 0;
		} else if (CR_FLAG(pia->reg_crb, CL2_RESTORE)) {
			// return high on the next cycle
			PRIVATE(pia)->internal_cb2 = 1;
		} else {
			// return high on next active CA1 transition
			PRIVATE(pia)->internal_cb2 = PRIVATE(pia)->internal_cb2 | PRIVATE(pia)->state_b.act_trans_cl1;
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
	if (CR_FLAG(pia->reg_cra, CL2_MODE_SELECT)) {
		SIGNAL_SET_BOOL(ca2, PRIVATE(pia)->internal_ca2);
	}

	// output on cb2 if in output mode
	if (CR_FLAG(pia->reg_crb, CL2_MODE_SELECT)) {
		SIGNAL_SET_BOOL(cb2, PRIVATE(pia)->internal_cb2);
	}

	// output on databus
	if (PRIVATE(pia)->out_enabled) {
		SIGNAL_SET_UINT8(bus_data, PRIVATE(pia)->out_data);
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
	CHIP_SET_FUNCTIONS(pia, chip_6520_process, chip_6520_destroy, chip_6520_register_dependencies);

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

void chip_6520_register_dependencies(Chip6520 *pia) {
	signal_add_dependency(pia->signal_pool, SIGNAL(reset_b), pia->id);
	signal_add_dependency(pia->signal_pool, SIGNAL(enable), pia->id);
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
		pia->reg_cra = 0;
		pia->reg_ora = 0;
		pia->reg_ddrb = 0;
		pia->reg_crb = 0;
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
	PRIVATE(pia)->out_enabled = false;

	if (enable) {
		process_positive_enable_edge(pia);
	} else {
		process_negative_enable_edge(pia);
	}

	process_end(pia);
}
