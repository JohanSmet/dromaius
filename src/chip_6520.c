// chip_6521.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the 6520 Peripheral Interface Adapter

#include "chip_6520.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

//#define DMS_LOG_TRACE
#define LOG_SIMULATOR		pia->simulator
#include "log.h"

#define SIGNAL_PREFIX		CHIP_6520_
#define SIGNAL_OWNER		pia

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
	int reg_addr = (SIGNAL_READ(RS1) << 1) | SIGNAL_READ(RS0);

	switch (reg_addr) {
		case 0:
			if (CR_FLAG(pia->reg_cra, DDR_OR_SELECT)) {
				LOG_TRACE("6520 [%s]: set reg_ora = 0x%.2x", pia->name, data);
				pia->reg_ora = data;
				PRIVATE(pia)->state_a.write_port = true;
			} else {
				LOG_TRACE("6520 [%s]: set reg_ddra = 0x%.2x", pia->name, data);
				pia->reg_ddra = data;
				// remove ourself as the active writer from all pins of port_a
				SIGNAL_GROUP_NO_WRITE(port_a);
			}
			break;
		case 1:
			LOG_TRACE("6520 [%s]: reg_cra = 0x%.2x", pia->name, pia->reg_cra);
			pia->reg_cra = (uint8_t)((pia->reg_cra & 0b11000000) | (data & 0b00111111));
			LOG_TRACE("6520 [%s]: set reg_cra = 0x%.2x", pia->name, pia->reg_cra);
			if (CR_FLAG(pia->reg_cra, CL2_MODE_SELECT) && !CR_FLAG(pia->reg_cra, CL2_OUTPUT)) {
				PRIVATE(pia)->internal_ca2 = true;
				LOG_TRACE("6520 [%s]: cra change causes internal_ca2 to become true", pia->name);
			}
			break;
		case 2:
			if (CR_FLAG(pia->reg_crb, DDR_OR_SELECT)) {
				LOG_TRACE("6520 [%s]: set reg_orb = 0x%.2x", pia->name, data);
				pia->reg_orb = data;
				PRIVATE(pia)->state_b.write_port = true;
			} else {
				LOG_TRACE("6520 [%s]: set reg_ddrb = 0x%.2x", pia->name, data);
				pia->reg_ddrb = data;
				// remove ourself as the active writer from all pins of port_b
				SIGNAL_GROUP_NO_WRITE(port_b);
			}
			break;
		case 3:
			LOG_TRACE("6520 [%s]: reg_crb = 0x%.2x", pia->name, pia->reg_crb);
			pia->reg_crb = (uint8_t)((pia->reg_crb & 0b11000000) | (data & 0b00111111));
			LOG_TRACE("6520 [%s]: set reg_crb = 0x%.2x", pia->name, pia->reg_crb);
			if (CR_FLAG(pia->reg_crb, CL2_MODE_SELECT) && !CR_FLAG(pia->reg_crb, CL2_OUTPUT)) {
				PRIVATE(pia)->internal_cb2 = true;
				LOG_TRACE("6520 [%s]: cra change causes internal_cb2 to become true", pia->name);
			}
			break;
	}
}

static inline uint8_t read_register(Chip6520 *pia) {
	int reg_addr = (SIGNAL_READ(RS1) << 1) | SIGNAL_READ(RS0);

	switch (reg_addr) {
		case 0:
			if (CR_FLAG(pia->reg_cra, DDR_OR_SELECT)) {
				PRIVATE(pia)->state_a.read_port = true;
				return SIGNAL_GROUP_READ_U8(port_a);
			} else {
				return pia->reg_ddra;
			}
		case 1:
			return pia->reg_cra;
		case 2:
			if (CR_FLAG(pia->reg_crb, DDR_OR_SELECT)) {
				PRIVATE(pia)->state_b.read_port = true;
				return SIGNAL_GROUP_READ_U8(port_b);
			} else {
				return pia->reg_ddrb;
			}
		case 3:
			return pia->reg_crb;
	}

	return 0;
}

static inline void control_register_irq_routine(Chip6520 *pia, uint8_t *reg_ctrl, bool cl1, bool cl2, Chip6520_pstate *state) {

	(void) pia;

	// check for active transition of the control lines
	bool irq1_pt = CR_FLAG(*reg_ctrl, IRQ1_POS_TRANSITION);
	state->act_trans_cl1 = (cl1 && !state->prev_cl1 && irq1_pt) || (!cl1 && state->prev_cl1 && !irq1_pt);

	bool irq2_pt = CR_FLAG(*reg_ctrl, IRQ2_POS_TRANSITION);
	state->act_trans_cl2 = !CR_FLAG(*reg_ctrl, CL2_MODE_SELECT) &&
						   ((cl2 && !state->prev_cl2 && irq2_pt) || (!cl2 && state->prev_cl2 && !irq2_pt));

#ifdef DMS_LOG_TRACE
	if (state->act_trans_cl1 || state->act_trans_cl2) {
		LOG_TRACE("6520 [%s]: >> act_trans_cl1 = %d, act_trans_cl2 = %d", pia->name, state->act_trans_cl1, state->act_trans_cl2);
	}
	uint8_t save_ctrl = *reg_ctrl;
#endif

	// a read of the peripheral I/O port resets both irq flags in reg_ctrl
	if (state->read_port) {
		CR_CHANGE_FLAG(*reg_ctrl, IRQ1, ACTHI_DEASSERT);
		CR_CHANGE_FLAG(*reg_ctrl, IRQ2, ACTHI_DEASSERT);
		LOG_TRACE("6520 [%s]: >> port was read - reset irq flags, (reg_ctrl becomes 0x%.2x", pia->name, *reg_ctrl);
	}

	// an active transition of the cl1/cl2-lines sets the irq-flags in reg_ctrl
	*reg_ctrl = (uint8_t) (*reg_ctrl | (-(int8_t) state->act_trans_cl1 & FLAG_6520_IRQ1)
								     | (-(int8_t) state->act_trans_cl2 & FLAG_6520_IRQ2));

	// if cl2 is in output-mode, the irq2 flag is always zero
	if (CR_FLAG(*reg_ctrl, CL2_MODE_SELECT)) {
		CR_CHANGE_FLAG(*reg_ctrl, IRQ2, ACTHI_DEASSERT);
	}

#ifdef DMS_LOG_TRACE
	if (save_ctrl != *reg_ctrl) {
		LOG_TRACE("6520 [%s]: >> reg_ctrl was 0x%.2x, becomes 0x%.2x", pia->name, save_ctrl, *reg_ctrl);
	}
#endif

	// store state of interrupt input/peripheral control lines
	state->prev_cl1 = cl1;
	state->prev_cl2 = cl2;
}

static inline void process_positive_enable_edge(Chip6520 *pia) {
	if (PRIVATE(pia)->strobe && SIGNAL_READ(RW) == RW_READ) {
		PRIVATE(pia)->out_data = read_register(pia);
		PRIVATE(pia)->out_enabled = true;
	}
}

void process_negative_enable_edge(Chip6520 *pia) {

	if (PRIVATE(pia)->strobe) {
		// read/write internal register
		if (SIGNAL_READ(RW) == RW_WRITE) {
			write_register(pia, SIGNAL_GROUP_READ_U8(data));
		} else {
			PRIVATE(pia)->out_data = read_register(pia);
			PRIVATE(pia)->out_enabled = true;
		}
	}

	// irq-A routine
	LOG_TRACE("6520 [%s]: control_register_irq_routine() for reg_cra", pia->name);
	control_register_irq_routine(pia,
			&pia->reg_cra,
			SIGNAL_READ(CA1), SIGNAL_READ(CA2),
			&PRIVATE(pia)->state_a);

	// irq-B routine
	LOG_TRACE("6520 [%s]: control_register_irq_routine() for reg_cra", pia->name);
	control_register_irq_routine(pia,
			&pia->reg_crb,
			SIGNAL_READ(CB1), SIGNAL_READ(CB2),
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

	// the IRQ-lines are open-drain; output is either low (tied to ground) or hi-z (floating, to be pulled up externally)
	// be careful when the IRQ-lines are tied to the same signal
	if (SIGNAL(IRQA_B) != SIGNAL(IRQB_B)) {
		if (!PRIVATE(pia)->out_irqa_b) {
			SIGNAL_WRITE(IRQA_B, false);
		} else {
			SIGNAL_NO_WRITE(IRQA_B);
		}

		if (!PRIVATE(pia)->out_irqb_b) {
			SIGNAL_WRITE(IRQB_B, false);
		} else {
			SIGNAL_NO_WRITE(IRQB_B);
		}
	} else {
		if (!PRIVATE(pia)->out_irqa_b || !PRIVATE(pia)->out_irqb_b) {
			SIGNAL_WRITE(IRQA_B, false);
		} else {
			SIGNAL_NO_WRITE(IRQA_B);
		}
	}

	// output on the ports
	SIGNAL_GROUP_WRITE_MASKED(port_a, pia->reg_ora, pia->reg_ddra);
	SIGNAL_GROUP_WRITE_MASKED(port_b, pia->reg_orb, pia->reg_ddrb);

	// output on ca2 if in output mode
	if (CR_FLAG(pia->reg_cra, CL2_MODE_SELECT)) {
		SIGNAL_WRITE(CA2, PRIVATE(pia)->internal_ca2);
	} else {
		SIGNAL_NO_WRITE(CA2);
	}

	// output on cb2 if in output mode
	if (CR_FLAG(pia->reg_crb, CL2_MODE_SELECT)) {
		SIGNAL_WRITE(CB2, PRIVATE(pia)->internal_cb2);
	} else  {
		SIGNAL_NO_WRITE(CB2);
	}

	// output on databus
	if (PRIVATE(pia)->out_enabled) {
		SIGNAL_GROUP_WRITE(data, PRIVATE(pia)->out_data);
	} else {
		SIGNAL_GROUP_NO_WRITE(data);
	}
}

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void chip_6520_register_dependencies(Chip6520 *pia);
static void chip_6520_destroy(Chip6520 *pia);
static void chip_6520_process(Chip6520 *pia);

Chip6520 *chip_6520_create(Simulator *sim, Chip6520Signals signals) {
	Chip6520_private *priv = (Chip6520_private *) calloc(1, sizeof(Chip6520_private));

	Chip6520 *pia = &priv->intf;
	pia->simulator = sim;
	pia->signal_pool = sim->signal_pool;
	CHIP_SET_FUNCTIONS(pia, chip_6520_process, chip_6520_destroy, chip_6520_register_dependencies);

	memcpy(pia->signals, signals, sizeof(Chip6520Signals));

	pia->sg_port_a = signal_group_create();
	pia->sg_port_b = signal_group_create();
	pia->sg_data = signal_group_create();

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(PA0 + i, port_a);
	}
	SIGNAL_GROUP_DEFAULTS(port_a, 0xff);		// port-A has passive resistive pull-ups

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(PB0 + i, port_b);
	}

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(D0 - i, data);
	}

	SIGNAL_DEFINE_DEFAULT(CA1,		false);
	SIGNAL_DEFINE_DEFAULT(CA2,		false);
	SIGNAL_DEFINE_DEFAULT(CB1,		false);
	SIGNAL_DEFINE_DEFAULT(CB2,		false);

	SIGNAL_DEFINE_DEFAULT(IRQA_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(IRQB_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS0,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS1,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RESET_B,	ACTLO_DEASSERT);

	SIGNAL_DEFINE_DEFAULT(PHI2,		true);
	SIGNAL_DEFINE_DEFAULT(CS0,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(CS1,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(CS2_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RW,		true);

	return pia;
}

static void chip_6520_register_dependencies(Chip6520 *pia) {
	SIGNAL_DEPENDENCY(RESET_B);
	SIGNAL_DEPENDENCY(PHI2);
}

static void chip_6520_destroy(Chip6520 *pia) {
	assert(pia);
	free(PRIVATE(pia));
}

static void chip_6520_process(Chip6520 *pia) {
	assert(pia);

	bool reset_b = SIGNAL_READ(RESET_B);

	// reset flags
	PRIVATE(pia)->state_a.read_port = false;
	PRIVATE(pia)->state_a.write_port = false;
	PRIVATE(pia)->state_b.read_port = false;
	PRIVATE(pia)->state_b.write_port = false;

	if (ACTLO_ASSERTED(reset_b)) {
		LOG_TRACE("6520 [%s]: reset asserted", pia->name);
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
	if (ACTLO_ASSERTED(SIGNAL_READ(RESET_B)) || !SIGNAL_CHANGED(PHI2)) {
		LOG_TRACE("6520 [%s]: exit without processing", pia->name);
		process_end(pia);
		return;
	}

	PRIVATE(pia)->strobe =	ACTHI_ASSERTED(SIGNAL_READ(CS0)) &&
							ACTHI_ASSERTED(SIGNAL_READ(CS1)) &&
							ACTLO_ASSERTED(SIGNAL_READ(CS2_B));
	PRIVATE(pia)->out_enabled = false;

	if (SIGNAL_READ(PHI2)) {
		process_positive_enable_edge(pia);
	} else {
		process_negative_enable_edge(pia);
	}

	process_end(pia);
}
