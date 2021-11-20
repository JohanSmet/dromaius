// chip_6522.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the 6522 Versatile Interface Adapter

// NOTE: not tested in a "real-world" scenario, only with the provided unit-test
//	- timing of operations might be off (e.g. start of timers)
//	- reading of IFR might be wrong: should IER be taken into account or not?
//	- unclear what should happen to the CB-flags in IFR when shift register is activated
//	- ...

#include "chip_6522.h"
#include "simulator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define SIGNAL_PREFIX		CHIP_6522_
#define SIGNAL_OWNER		via

//////////////////////////////////////////////////////////////////////////////
//
// internal types
//

typedef struct Chip6522_pstate {		// port state
	bool			prev_cl1;
	bool			prev_cl2;
	bool			act_trans_cl1;
	bool			act_trans_cl2;

	bool			latched;
	bool			port_read;
	bool			port_written;

	bool			cl1_is_output;
	bool			cl2_is_output;
	bool			out_cl1;
	bool			out_cl2;
} Chip6522_pstate;

typedef struct Chip6522_output {
	bool			drv_data;
	uint8_t			data;

	bool			irq;

	bool			drv_ca2;
	bool			drv_cb1;
	bool			drv_cb2;
	bool			ca2;
	bool			cb1;
	bool			cb2;

	uint8_t			reg_ora;
	uint8_t			reg_orb;
	uint8_t			reg_ddra;
	uint8_t			reg_ddrb;
} Chip6522_output;

typedef struct Chip6522_private {
	Chip6522		intf;

	bool			strobe;
	bool			prev_pb6;
	bool			prev_cb1;

	Chip6522_pstate	state_a;
	Chip6522_pstate	state_b;

	bool			t1_start;
	bool			t1_enabled;
	bool			t1_clr_irq;
	bool			t1_set_irq;

	bool			t2_start;
	bool			t2_enabled;
	bool			t2_irq_done;
	bool			t2_clr_irq;
	bool			t2_set_irq;
	uint8_t			reg_t2l_h;

	bool			sr_start;
	bool			sr_running;
	uint8_t			sr_count;
	uint8_t			sr_timer;
	bool			sr_clr_irq;
	bool			sr_set_irq;

	Chip6522_output output;
	Chip6522_output last_output;
} Chip6522_private;

typedef enum Chip6522_address {
	ADDR_ORB_IRB		= 0b0000,
	ADDR_ORA_IRA		= 0b0001,
	ADDR_DDRB			= 0b0010,
	ADDR_DDRA			= 0b0011,
	ADDR_T1C_L			= 0b0100,
	ADDR_T1C_H			= 0b0101,
	ADDR_T1L_L			= 0b0110,
	ADDR_T1L_H			= 0b0111,
	ADDR_T2C_L			= 0b1000,
	ADDR_T2C_H			= 0b1001,
	ADDR_SR				= 0b1010,
	ADDR_ACR			= 0b1011,
	ADDR_PCR			= 0b1100,
	ADDR_IFR			= 0b1101,
	ADDR_IER			= 0b1110,
	ADDR_ORA_IRA_NOHS	= 0b1111
} Chip6522_address;

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(via)	((Chip6522_private *) (via))

#define ACR_SETTING(m, v) MASK_IS_SET(via->reg_acr, MASK_6522_ACR_##m, VALUE_6522_ACR_##v)
#define PCR_SETTING(m, v) MASK_IS_SET(via->reg_pcr, MASK_6522_PCR_##m, VALUE_6522_PCR_##v)

#define RW_READ  true
#define RW_WRITE false

static void setup_shift_register(Chip6522 *via);

static inline Chip6522_address rs_to_addr(Chip6522 *via) {
	return (Chip6522_address) (SIGNAL_READ(RS0) | (SIGNAL_READ(RS1) << 1) | (SIGNAL_READ(RS2) << 2) | (SIGNAL_READ(RS3) << 3));
}

static inline void write_register(Chip6522 *via, uint8_t data) {

	switch (rs_to_addr(via)) {
		case ADDR_ORB_IRB:
			via->reg_orb = data;
			PRIVATE(via)->state_b.port_written = true;
			break;
		case ADDR_ORA_IRA:
			via->reg_ora = data;
			PRIVATE(via)->state_a.port_written = true;
			break;
		case ADDR_DDRB:
			via->reg_ddrb = data;
			// remove ourself as the active writer from all pins of port_b
			SIGNAL_GROUP_NO_WRITE(port_b);
			break;
		case ADDR_DDRA:
			via->reg_ddra = data;
			// remove ourself as the active writer from all pins of port_a
			SIGNAL_GROUP_NO_WRITE(port_a);
			break;
		case ADDR_T1C_L:
			via->reg_t1l_l = data;
			break;
		case ADDR_T1C_H:
			via->reg_t1l_h = data;
			PRIVATE(via)->t1_clr_irq = true;
			PRIVATE(via)->t1_start = true;
			break;
		case ADDR_T1L_L:
			via->reg_t1l_l = data;
			break;
		case ADDR_T1L_H:
			PRIVATE(via)->t1_clr_irq = true;
			via->reg_t1l_h = data;
			break;
		case ADDR_T2C_L:
			via->reg_t2l_l = data;
			break;
		case ADDR_T2C_H:
			PRIVATE(via)->reg_t2l_h = data;
			PRIVATE(via)->t2_clr_irq = true;
			PRIVATE(via)->t2_start = true;
			break;
		case ADDR_SR:
			via->reg_sr = data;
			PRIVATE(via)->sr_start = true;
			PRIVATE(via)->sr_clr_irq = true;
			break;
		case ADDR_ACR:
			via->reg_acr = data;
			setup_shift_register(via);
			break;
		case ADDR_PCR:
			via->reg_pcr = data;
			break;
		case ADDR_IFR:
			// Individual bits in the IFR my be cleared by writing a logic 1 into the appropriate bit of the IFR.
			// Bit 7 indicates the status of the irq_b output and can't be written to directly.
			via->reg_ifr = (uint8_t) (via->reg_ifr & ~(data & 0x7f));
			break;
		case ADDR_IER:
			// bit 7 of data indicates of bits should be cleared (0) or set (1)
			if (BIT_IS_SET(data, 7)) {
				via->reg_ier = (uint8_t) (via->reg_ier | (data & 0x7f));
			} else {
				via->reg_ier = (uint8_t) (via->reg_ier & ~data & 0x7f);
			}
			break;
		case ADDR_ORA_IRA_NOHS:
			via->reg_ora = data;
			break;
	}
}

static inline uint8_t read_register(Chip6522 *via) {

	switch (rs_to_addr(via)) {
		case ADDR_ORB_IRB:
			PRIVATE(via)->state_b.port_read = true;
			return (uint8_t) ((via->reg_ilb & ~via->reg_ddrb) | (via->reg_orb & via->reg_ddrb));
		case ADDR_ORA_IRA:
			PRIVATE(via)->state_a.port_read = true;
			return via->reg_ila;
		case ADDR_DDRB:
			return via->reg_ddrb;
		case ADDR_DDRA:
			return via->reg_ddra;
		case ADDR_T1C_L:
			PRIVATE(via)->t1_clr_irq = true;
			return LO_BYTE(via->reg_t1c);
		case ADDR_T1C_H:
			return HI_BYTE(via->reg_t1c);
		case ADDR_T1L_L:
			return via->reg_t1l_l;
		case ADDR_T1L_H:
			return via->reg_t1l_h;
		case ADDR_T2C_L:
			PRIVATE(via)->t2_clr_irq = true;
			return LO_BYTE(via->reg_t2c);
		case ADDR_T2C_H:
			return HI_BYTE(via->reg_t2c);
		case ADDR_SR:
			PRIVATE(via)->sr_clr_irq = true;
			PRIVATE(via)->sr_start = true;
			return via->reg_sr;
		case ADDR_ACR:
			return via->reg_acr;
		case ADDR_PCR:
			return via->reg_pcr;
		case ADDR_IFR:
			return via->reg_ifr;
		case ADDR_IER:
			return via->reg_ier | 0x80;
		case ADDR_ORA_IRA_NOHS:
			return SIGNAL_GROUP_READ_U8(port_a);
	}

	return 0;
}

static void process_end(Chip6522 *via) {

	Chip6522_output *output = &PRIVATE(via)->output;
	Chip6522_output *last_output = &PRIVATE(via)->last_output;

	// the IRQ-line is open-drain; output is either low (tied to ground) or hi-z (floating, to be pulled up externally)
	if (output->irq != last_output->irq) {
		if (!output->irq) {
			SIGNAL_WRITE(IRQ_B, false);
		} else {
			SIGNAL_NO_WRITE(IRQ_B);
		}
		last_output->irq = output->irq;
	}

	// output on the ports
	if (via->reg_ora != last_output->reg_ora || via->reg_ddra != last_output->reg_ddra) {
		SIGNAL_GROUP_WRITE_MASKED(port_a, via->reg_ora, via->reg_ddra);
		last_output->reg_ora = via->reg_ora;
		last_output->reg_ddra = via->reg_ddra;
	}

	if (via->reg_orb != last_output->reg_orb || via->reg_ddrb != last_output->reg_ddrb) {
		SIGNAL_GROUP_WRITE_MASKED(port_b, via->reg_orb, via->reg_ddrb);
		last_output->reg_orb = via->reg_orb;
		last_output->reg_ddrb = via->reg_ddrb;
	}

	// optional output on databus
	if (output->drv_data) {
		// via is active - write if data changed or via just became active
		if (output->data != last_output->drv_data || !last_output->drv_data) {
			SIGNAL_GROUP_WRITE(data, output->data);
			last_output->data = output->data;
			last_output->drv_data = output->drv_data;
		}
	} else if (last_output->drv_data) {
		// via just went inactive
		SIGNAL_GROUP_NO_WRITE(data);
		last_output->drv_data = false;
	}

	// optional output on ca2
	if (PRIVATE(via)->state_a.cl2_is_output) {
		if (PRIVATE(via)->state_a.out_cl2 != last_output->ca2 || !last_output->drv_ca2) {
			SIGNAL_WRITE(CA2, PRIVATE(via)->state_a.out_cl2);
			last_output->drv_ca2 = true;
			last_output->ca2 = PRIVATE(via)->state_a.out_cl2;
		}
	} else if (last_output->drv_ca2) {
		SIGNAL_NO_WRITE(CA2);
		last_output->drv_ca2 = false;
	}

	// optional output on cb1
	if (PRIVATE(via)->state_b.cl1_is_output) {
		if (PRIVATE(via)->state_b.out_cl1 != last_output->cb1 || !last_output->drv_cb1) {
			SIGNAL_WRITE(CB1, PRIVATE(via)->state_b.out_cl1);
			last_output->drv_cb1 = true;
			last_output->cb1 = PRIVATE(via)->state_b.out_cl1;
		}
	} else if (last_output->drv_cb1) {
		SIGNAL_NO_WRITE(CB1);
		last_output->drv_cb1 = false;
	}

	// optional output on cb2
	if (PRIVATE(via)->state_b.cl2_is_output) {
		if (PRIVATE(via)->state_b.out_cl2 != last_output->cb2 || !last_output->drv_cb2) {
			SIGNAL_WRITE(CB2, PRIVATE(via)->state_b.out_cl2);
			last_output->drv_cb2 = true;
			last_output->cb2 = PRIVATE(via)->state_b.out_cl2;
		}
	} else if (last_output->drv_cb2) {
		SIGNAL_NO_WRITE(CB2);
		last_output->drv_cb2 = false;
	}
}

static void process_control_line_port_input(bool cl1, bool cl2, uint8_t control, Chip6522_pstate *state) {
// NOTE: control is either the top or the bottom half of reg_pcr depending on which port is processed
//		=> function will use the CA-flags for everything

	// check for active transitions on control line 1
	bool cl1_pos = (control & MASK_6522_PCR_CA1_CONTROL) == VALUE_6522_PCR_CA1_POS_ACTIVE;
	state->act_trans_cl1 = (cl1 && !state->prev_cl1 && cl1_pos) || (!cl1 && state->prev_cl1 && !cl1_pos);

	// check for active transitions on control line 2 (if configured in input mode)
	state->cl2_is_output = (control & 0b1000) == 0b1000;

	if (!state->cl2_is_output) {
		bool cl2_pos = (control & 0b0100) == 0b0100;
		state->act_trans_cl2 = (cl2 && !state->prev_cl2 && cl2_pos) || (!cl2 && state->prev_cl2 && !cl2_pos);
	}

	// input latching
	state->latched = state->latched || state->act_trans_cl1;

	// store state of interrupt input/peripheral control lines
	state->prev_cl1 = cl1;
	state->prev_cl2 = cl2;
}

static void process_control_line_output(bool check_read, uint8_t control, Chip6522_pstate *state) {

	if ((control & MASK_6522_PCR_CA2_CONTROL) == VALUE_6522_PCR_CA2_OUT_HANDSHAKE) {
		// output low on a read or write of the output register
		if ((check_read && state->port_read) || state->port_written) {
			state->out_cl2 = false;
		}

		// output high on an activate transition of ca1
		if (state->act_trans_cl1) {
			state->out_cl2 = true;
		}
	} else if ((control & MASK_6522_PCR_CA2_CONTROL) == VALUE_6522_PCR_CA2_OUT_PULSE) {
		// go low for one cycle following a read or write of the output register
		state->out_cl2 = !((check_read && state->port_read) || state->port_written);
	} else {
		// manual output mode
		state->out_cl2 = control & 0b0010;
	}
}

static void process_edge_common(Chip6522 *via) {

	// peripharal data ports - from the data sheet:
	// - when input latching is disabled, ILA/ILB will reflect the logic levels present on the bus pins
	// - when input latching is enabled and the selected active transition on CA1/CB1 having occured, ILA/ILB
	//   will contain the data present on the bus lines at the time of transition. Once ILA/ILB has been read, it
	//   will appear transparant, reflecting the current state of the bus pins until the next latching transition.
	if (ACR_SETTING(PA_LATCH, PA_NO_LATCH) || !PRIVATE(via)->state_a.latched) {
		via->reg_ila = SIGNAL_GROUP_READ_U8(port_a);
	}

	if (ACR_SETTING(PB_LATCH, PB_NO_LATCH) || !PRIVATE(via)->state_b.latched) {
		via->reg_ilb = SIGNAL_GROUP_READ_U8(port_b);
	}
}

static void build_interrupt_register(Chip6522 *via) {

	// some events that happen can pull certain bits of the IFR high
	int new_ifr = ( PRIVATE(via)->state_a.act_trans_cl2 << 0 |
					PRIVATE(via)->state_a.act_trans_cl1 << 1 |
					PRIVATE(via)->sr_set_irq << 2 |
					PRIVATE(via)->state_b.act_trans_cl2 << 3 |
					PRIVATE(via)->state_b.act_trans_cl1 << 4 |
					PRIVATE(via)->t2_set_irq << 5 |
					PRIVATE(via)->t1_set_irq << 6
	);
	via->reg_ifr |= (uint8_t) new_ifr;

	// other events pull bits back down
	int clr_ifr =	PRIVATE(via)->state_a.port_read << 0 |
					PRIVATE(via)->state_a.port_written << 0 |
					PRIVATE(via)->state_a.port_read << 1 |
					PRIVATE(via)->state_a.port_written << 1 |
					PRIVATE(via)->sr_clr_irq << 2 |
					PRIVATE(via)->state_b.port_read << 3 |
					PRIVATE(via)->state_b.port_written << 3 |
					PRIVATE(via)->state_b.port_read << 4 |
					PRIVATE(via)->state_b.port_written << 4 |
					PRIVATE(via)->t2_clr_irq << 5 |
					PRIVATE(via)->t1_clr_irq << 6;

	if (PCR_SETTING(CA2_CONTROL, CA2_IN_NEG_INDEP) || PCR_SETTING(CA2_CONTROL, CA2_IN_POS_INDEP)) {
		clr_ifr &= 0b11111110;
	}

	if (PCR_SETTING(CB2_CONTROL, CB2_IN_NEG_INDEP) || PCR_SETTING(CB2_CONTROL, CB2_IN_POS_INDEP)) {
		clr_ifr &= 0b11110111;
	}

	via->reg_ifr &= (uint8_t) ~clr_ifr;

	PRIVATE(via)->t1_clr_irq = false;
	PRIVATE(via)->t1_set_irq = false;
	PRIVATE(via)->t2_clr_irq = false;
	PRIVATE(via)->t2_set_irq = false;
	PRIVATE(via)->sr_clr_irq = false;
	PRIVATE(via)->sr_set_irq = false;

	// set IFR[7] and output irq when any of the lower 7-bits are set
	if ((via->reg_ifr & via->reg_ier) > 0) {
		via->reg_ifr |= 0x80;
		PRIVATE(via)->output.irq = ACTLO_ASSERT;
	} else {
		via->reg_ifr &= 0x7f;
		PRIVATE(via)->output.irq = ACTLO_DEASSERT;
	}
}

static void process_timer1(Chip6522 *via) {

	if (PRIVATE(via)->t1_start) {
		via->reg_t1c = MAKE_WORD(via->reg_t1l_h, via->reg_t1l_l);
		PRIVATE(via)->t1_start = false;
		PRIVATE(via)->t1_enabled = true;

		// make sure port_b[7] is low if configured in one-shot mode
		if (ACR_SETTING(T1_CONTROL, T1_LOAD_1SHOT)) {
			BIT_CLEAR(via->reg_orb, 7);
		}
		return;
	}

	if (!PRIVATE(via)->t1_enabled) {
		return;
	}

	// check if timer should restart
	if (via->reg_t1c == 0) {
		PRIVATE(via)->t1_set_irq = true;
		if (ACR_SETTING(T1_CONTROL, T1_CONT_SQWAVE)) {
			via->reg_orb ^= 0b10000000;
		} else if (ACR_SETTING(T1_CONTROL, T1_LOAD_1SHOT)) {
			BIT_SET(via->reg_orb, 7);
		}
		PRIVATE(via)->t1_start = BIT_IS_SET(via->reg_acr, 6);
		PRIVATE(via)->t1_enabled = false;
	}

	// decrement timer
	--via->reg_t1c;
}

static void process_timer2(Chip6522 *via) {

	if (PRIVATE(via)->t2_start) {
		via->reg_t2c = MAKE_WORD(PRIVATE(via)->reg_t2l_h, via->reg_t2l_l);
		PRIVATE(via)->t2_start = false;
		PRIVATE(via)->t2_enabled = true;
		PRIVATE(via)->t2_irq_done = false;
		PRIVATE(via)->prev_pb6 = BIT_IS_SET(via->reg_ilb, 6);
		return;
	}

	if (!PRIVATE(via)->t2_enabled) {
		return;
	}

	bool count_pb6 = BIT_IS_SET(via->reg_acr, 5);

	// trigger interrupt?
	if (via->reg_t2c == 0 && !PRIVATE(via)->t2_irq_done) {
		PRIVATE(via)->t2_set_irq = true;
		PRIVATE(via)->t2_irq_done = true;
	}

	// decrement timer
	bool pb6 = BIT_IS_SET(via->reg_ilb, 6);

	if (!count_pb6 || (PRIVATE(via)->prev_pb6 && !pb6)) {
		--via->reg_t2c;
	}

	PRIVATE(via)->prev_pb6 = pb6;
}

static void setup_shift_register(Chip6522 *via) {

	int mode = via->reg_acr & MASK_6522_ACR_SR_CONTROL;

	switch (mode) {
		case VALUE_6522_ACR_SR_IN_T2:
		case VALUE_6522_ACR_SR_IN_PHI2:
		case VALUE_6522_ACR_SR_OUT_FREE_T2:
		case VALUE_6522_ACR_SR_OUT_T2:
		case VALUE_6522_ACR_SR_OUT_PHI2:
			PRIVATE(via)->state_b.cl1_is_output = true;
			PRIVATE(via)->state_b.out_cl1 = true;
			break;

		case VALUE_6522_ACR_SR_DISABLED:
		case VALUE_6522_ACR_SR_OUT_EXT:
		case VALUE_6522_ACR_SR_IN_EXT:
			PRIVATE(via)->state_b.cl1_is_output = false;
			break;
	}
}

static void process_shift_register(Chip6522 *via) {

	if (PRIVATE(via)->sr_start) {
		PRIVATE(via)->sr_start = false;
		PRIVATE(via)->sr_running = true;
		PRIVATE(via)->sr_count = 0;
		PRIVATE(via)->sr_timer = 1;
		PRIVATE(via)->state_b.out_cl1 = true;
		return;
	}

	if (ACR_SETTING(SR_CONTROL, SR_DISABLED) || !PRIVATE(via)->sr_running) {
		return;
	}

	if (PRIVATE(via)->sr_count == 8) {
		if (!ACR_SETTING(SR_CONTROL, SR_OUT_FREE_T2)) {
			PRIVATE(via)->sr_set_irq = true;
			PRIVATE(via)->sr_running = false;
			return;
		} else {
			PRIVATE(via)->sr_count = 0;
		}
	}

	bool shift_out = BIT_IS_SET(via->reg_acr, 4);
	bool do_shift = false;
	int mode = (via->reg_acr & 0b00001100) >> 2;

	switch (mode) {
		case 0b10:			// shifting controlled by PHI2
			PRIVATE(via)->state_b.out_cl1 = !PRIVATE(via)->state_b.out_cl1;
			do_shift = PRIVATE(via)->state_b.out_cl1;
			break;
		case 0b11:			// shifting controlled by extern clock (CB1)
			do_shift = !PRIVATE(via)->prev_cb1 && SIGNAL_READ(CB1);
			break;
		default:			// shifting controlled by T2
			--PRIVATE(via)->sr_timer;
			if (PRIVATE(via)->sr_timer == 0) {
				PRIVATE(via)->state_b.out_cl1 = !PRIVATE(via)->state_b.out_cl1;
				do_shift = PRIVATE(via)->state_b.out_cl1;
				PRIVATE(via)->sr_timer = via->reg_t2l_l;
			}
			break;
	}

	if (!do_shift) {
		return;
	}

	if (shift_out) {
		PRIVATE(via)->state_b.cl2_is_output = true;
		PRIVATE(via)->state_b.out_cl2 = BIT_IS_SET(via->reg_sr, 7);
		via->reg_sr = (uint8_t) ((via->reg_sr << 1) | PRIVATE(via)->state_b.out_cl2);
	} else {
		via->reg_sr = (uint8_t) ((via->reg_sr << 1) | SIGNAL_READ(CB2));
	}

	++PRIVATE(via)->sr_count;
}

static void process_positive_enable_edge(Chip6522 *via) {
	process_edge_common(via);

	if (PRIVATE(via)->strobe && SIGNAL_READ(RW) == RW_READ) {
		PRIVATE(via)->output.data = read_register(via);
		PRIVATE(via)->output.drv_data = true;
	}
}

static void process_negative_enable_edge(Chip6522 *via) {

	process_edge_common(via);

	// port control lines
	process_control_line_port_input(SIGNAL_READ(CA1), SIGNAL_READ(CA2), via->reg_pcr & 0x0f, &PRIVATE(via)->state_a);
	process_control_line_port_input(SIGNAL_READ(CB1), SIGNAL_READ(CB2), via->reg_pcr >> 4, &PRIVATE(via)->state_b);

	// timers
	process_timer1(via);
	process_timer2(via);

	// shift register
	process_shift_register(via);

	// read/write register
	if (PRIVATE(via)->strobe) {
		// read/write internal register
		if (SIGNAL_READ(RW) == RW_WRITE) {
			write_register(via, SIGNAL_GROUP_READ_U8(data));
		} else {
			PRIVATE(via)->output.data = read_register(via);
			PRIVATE(via)->output.drv_data = true;

			// unlatch if port was read
			PRIVATE(via)->state_a.latched = PRIVATE(via)->state_a.latched && !PRIVATE(via)->state_a.port_read;
			PRIVATE(via)->state_b.latched = PRIVATE(via)->state_b.latched && !PRIVATE(via)->state_b.port_read;
		}
	}

	// pin ca2 output mode
	if (PRIVATE(via)->state_a.cl2_is_output) {
		process_control_line_output(true, via->reg_pcr & 0x0f, &PRIVATE(via)->state_a);
	}

	if (PRIVATE(via)->state_b.cl2_is_output && !BIT_IS_SET(via->reg_acr, 4)) {
		process_control_line_output(false, via->reg_pcr >> 4, &PRIVATE(via)->state_b);
	}

	// interrupt
	build_interrupt_register(via);

	// save signal state at negative edge
	PRIVATE(via)->prev_cb1 = SIGNAL_READ(CB1);
}

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void chip_6522_register_dependencies(Chip6522 *via);
static void chip_6522_destroy(Chip6522 *via);
static void chip_6522_process(Chip6522 *via);

Chip6522 *chip_6522_create(Simulator *sim, Chip6522Signals signals) {
	Chip6522_private *priv = (Chip6522_private *) calloc(1, sizeof(Chip6522_private));

	Chip6522 *via = &priv->intf;
	CHIP_SET_VARIABLES(via, sim, via->signals, CHIP_6522_PIN_COUNT);
	CHIP_SET_FUNCTIONS(via, chip_6522_process, chip_6522_destroy, chip_6522_register_dependencies);
	via->signal_pool = sim->signal_pool;

	memcpy(via->signals, signals, sizeof(Chip6522Signals));
	via->sg_port_a = signal_group_create();
	via->sg_port_b = signal_group_create();
	via->sg_data = signal_group_create();

	for (int i = 0; i < 8; ++i) {
		SIGNAL_DEFINE_GROUP(PA0 + i, port_a);
	}

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
	SIGNAL_DEFINE_DEFAULT(IRQ_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS0,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS1,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS2,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RS3,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RESET_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(PHI2,		true);
	SIGNAL_DEFINE_DEFAULT(CS1,		ACTHI_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(CS2_B,	ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(RW,		true);

	priv->state_a.out_cl2 = true;
	priv->state_b.out_cl2 = true;
	priv->last_output.irq = true;

	return via;
}

static void chip_6522_register_dependencies(Chip6522 *via) {
	assert(via);
	SIGNAL_DEPENDENCY(RESET_B);
	SIGNAL_DEPENDENCY(PHI2);
}

static void chip_6522_destroy(Chip6522 *via) {
	assert(via);
	free(PRIVATE(via));
}

static void chip_6522_process(Chip6522 *via) {
	assert(via);

	// reset operation flags
	PRIVATE(via)->state_a.port_read = false;
	PRIVATE(via)->state_a.port_written = false;
	PRIVATE(via)->state_b.port_read = false;
	PRIVATE(via)->state_b.port_written = false;

	if (ACTLO_ASSERTED(SIGNAL_READ(RESET_B))) {
		// reset is asserted - clear registers & exit
		via->reg_ifr = 0;
		via->reg_ier = 0;
		via->reg_pcr = 0;
		via->reg_acr = 0;
		via->reg_ila = 0;
		via->reg_ora = 0;
		via->reg_ddra = 0;
		via->reg_ilb = 0;
		via->reg_orb = 0;
		via->reg_ddrb = 0;
		process_end(via);
		return;
	}

	// do nothing: if not on a clock edge
	if (!SIGNAL_CHANGED(PHI2)) {
		process_end(via);
		return;
	}

	PRIVATE(via)->strobe = ACTHI_ASSERTED(SIGNAL_READ(CS1)) && ACTLO_ASSERTED(SIGNAL_READ(CS2_B));
	PRIVATE(via)->output.drv_data = false;

	if (SIGNAL_READ(PHI2)) {
		process_positive_enable_edge(via);
	} else {
		process_negative_enable_edge(via);
	}

	process_end(via);
}
