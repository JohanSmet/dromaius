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

	bool			prev_enable;
	bool			prev_ca1;
	bool			prev_ca2;
	bool			prev_cb1;
	bool			prev_cb2;

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

	return 0;
}

static inline void control_register_irq_routine(ctrl_reg_t *reg_ctrl, bool cl1, bool cl2, bool prev_cl1, bool prev_cl2, bool read_port) {

	// >> check for active transition of the control lines
	bool cl1_act_trans = (cl1 && !prev_cl1 && reg_ctrl->bf_irq1_pos_transition) ||
	                     (!cl1 && prev_cl1 && !reg_ctrl->bf_irq1_pos_transition);
	bool cl2_act_trans = (cl2 && !prev_cl2 && reg_ctrl->bf_irq2_pos_transition) ||
	                     (!cl2 && prev_cl2 && !reg_ctrl->bf_irq2_pos_transition);

	// >> a read of the peripheral A I/O port resets both irq flags in reg_cra
	if (read_port) {
		reg_ctrl->bf_irq1 = ACTHI_DEASSERT;
		reg_ctrl->bf_irq1 = ACTHI_DEASSERT;
	}

	// >> an active transition of the ca1/ca2-lines sets the irq-flags in reg_cra
	reg_ctrl->bf_irq1 = reg_ctrl->bf_irq1 | cl1_act_trans;
	reg_ctrl->bf_irq2 = reg_ctrl->bf_irq2 | cl2_act_trans;
}

static inline void process_positive_enable_edge(Chip6520 *pia) {

}

void process_negative_enable_edge(Chip6520 *pia) {
	// read/write internal register
	if (SIGNAL_BOOL(rw) == RW_WRITE) {
		write_register(pia, SIGNAL_UINT8(bus_data));
	} else {
		SIGNAL_SET_UINT8(bus_data, read_register(pia));
	}

	// check if the output-ports are being read
	bool read_porta = SIGNAL_BOOL(rw) == RW_READ && !SIGNAL_BOOL(rs0) && !SIGNAL_BOOL(rs1) && pia->reg_cra.bf_ddr_or_select;
	bool read_portb = SIGNAL_BOOL(rw) == RW_READ && !SIGNAL_BOOL(rs0) && SIGNAL_BOOL(rs1) && pia->reg_crb.bf_ddr_or_select;

	// irq-A routine (FIXME pin_ca2 output mode)
	control_register_irq_routine(
			&pia->reg_cra,
			SIGNAL_BOOL(ca1), SIGNAL_BOOL(ca2),
			PRIVATE(pia)->prev_ca1, PRIVATE(pia)->prev_ca2,
			read_porta);

	// irq-B routine (FIXME pin_cb2 output mode)
	control_register_irq_routine(
			&pia->reg_crb,
			SIGNAL_BOOL(cb1), SIGNAL_BOOL(cb2),
			PRIVATE(pia)->prev_cb1, PRIVATE(pia)->prev_cb2,
			read_portb);

	// irq output lines
	PRIVATE(pia)->out_irqa_b = !((pia->reg_cra.bf_irq1 && pia->reg_cra.bf_irq1_enable) ||
							     (pia->reg_cra.bf_irq2 && pia->reg_cra.bf_irq2_enable));
	PRIVATE(pia)->out_irqb_b = !((pia->reg_crb.bf_irq1 && pia->reg_crb.bf_irq1_enable) ||
							     (pia->reg_crb.bf_irq2 && pia->reg_crb.bf_irq2_enable));

	// store state of interrupt input/peripheral control lines
	PRIVATE(pia)->prev_ca1 = SIGNAL_BOOL(ca1);
	PRIVATE(pia)->prev_ca2 = SIGNAL_BOOL(ca2);
	PRIVATE(pia)->prev_cb1 = SIGNAL_BOOL(cb1);
	PRIVATE(pia)->prev_cb2 = SIGNAL_BOOL(cb2);
}

static inline void process_end(Chip6520 *pia) {

	// always write to the non-tristate outputs
	SIGNAL_SET_BOOL(irqa_b, PRIVATE(pia)->out_irqa_b);
	SIGNAL_SET_BOOL(irqb_b, PRIVATE(pia)->out_irqb_b);

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

	// FIXME: this isn't correct - IRQ handling should still occur even if chip is not selected (csX)
	// do nothing:
	//	- if reset is asserted or 
	//	- if cs0/cs1/cs2_b are not asserted
	//  - if not on the edge of a clock cycle
	bool enable = SIGNAL_BOOL(enable);

	if (ACTLO_ASSERTED(SIGNAL_BOOL(reset_b)) ||
		!ACTHI_ASSERTED(SIGNAL_BOOL(cs0)) || !ACTHI_ASSERTED(SIGNAL_BOOL(cs1)) || !ACTLO_ASSERTED(SIGNAL_BOOL(cs2_b)) ||
		enable == PRIVATE(pia)->prev_enable) {
		process_end(pia);
		return;
	}

	if (enable) {
		process_positive_enable_edge(pia);
	} else {
		process_negative_enable_edge(pia);
	}

	process_end(pia);
}
