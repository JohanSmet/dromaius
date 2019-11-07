// cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the MOS 6502

#include "cpu_6502.h"
#include "cpu_6502_opcodes.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/*

Timing diagram for the 6502 microprocessor.

   READ CYCLE
   ----------

_________ |                                      _________________________________  |
CLK      \|          tPWL                      |/              tPWH                \|
PHI2      |\__________________________________/|                                    |\______
          |                                    |                                    |
__________|  _____________________  ________________________________________________|  _____
Address   |\/     Stabilization   \/ Address is valid                               |\/
__________|/\_____________________/\________________________________________________|/\_____
          |                                    |                                    |
__________|  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _  ______________|  _____
Data      |\/                                                       \/  Data Valid  |\/
__________|/\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _/\______________|/\_____
          |                                    |                                    |
         (1)                                  (2)                                  (3)

(1) CPU starts changing the address
(2) Address is garantueed to be stable. Memory/IO operation can safely start.
(3) CPU reads data.


   WRITE CYCLE
   -----------

_________ |                                      _________________________________  |
CLK      \|                                    |/                                  \|
PHI2      |\__________________________________/|                                    |\______
          |                                    |                                    |
__________|  _____________________  ________________________________________________|  _____
Address   |\/     Stabilization   \/ Address is valid                               |\/
__________|/\_____________________/\________________________________________________|/\_____
          |                                    |                                    |
__________|  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _________________  __________________|  _____
Data      |\/                                    Stabilization  \/  Data Valid      |\/
__________|/\_ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _________________/\__________________|/\_____
          |                                    |                                    |
         (1)                                  (2)                                  (3)

(1) CPU starts changing the address
(2) Address is garantueed to be stable. CPU starts making data available on the bus.
(3) Data is garantueed to be stable. Memory/IO operation should complete.


Notes:
- The 6502 keeps the previous address stable at the beginning of the clock 
  cycle for at leat 10ns, depending on the operating frequency. (tAH/adress hold time)

*/

//////////////////////////////////////////////////////////////////////////////
//
// internal data types
//

#define RW_READ  true
#define RW_WRITE false

typedef enum CPU_6502_CYCLE {
	CYCLE_BEGIN = 0,		// (1) in timing diagram
	CYCLE_MIDDLE = 1,		// (2) in timing diagram
	CYCLE_END = 2			// (3) in timing diagram
} CPU_6502_CYCLE;

typedef enum CPU_6503_STATE {
	CS_INIT = 0,
	CS_RUNNING = 1
} CPU_6502_STATE;

typedef struct Cpu6502_private {
	Cpu6502			intf;
	bool			prev_reset;
	bool			prev_clock;
	uint16_t		internal_ab;			// internal address bus
	bool			internal_rw;			// internal rw latch
	CPU_6502_STATE	state;
	int8_t			decode_cycle;			// instruction decode cycle
	uint8_t			operand;
	union {
		uint16_t	addr;
		struct {
			uint8_t	adl;
			uint8_t adh;
		};
	};
	bool			page_crossed;
} Cpu6502_private;

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(cpu)	((Cpu6502_private *) (cpu))

static void process_end(Cpu6502 *cpu) {

	// always write to the output pins that aren't tristate
	*cpu->bus_address = PRIVATE(cpu)->internal_ab;
	*cpu->pin_rw	  = PRIVATE(cpu)->internal_rw;

	// store state of the clock pin
	PRIVATE(cpu)->prev_clock = *cpu->pin_clock;
}

static void execute_init(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
/* init sequence after reset: should be the same sequence as BRK/IRQ/NMI but always read */

	switch(PRIVATE(cpu)->decode_cycle) {
		case 0 :		// initialize stack pointer to zero
			if (phase == CYCLE_BEGIN) {
				cpu->reg_sp = 0;
				PRIVATE(cpu)->internal_ab = 0x00ff;
			}
			break;
		
		case 1 :		// pull IR to zero
			if (phase == CYCLE_BEGIN) {
				cpu->reg_ir = 0;
			}
			break;

		case 2 :		// "push" high byte of PC
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = 0x100 + cpu->reg_ir;
				--cpu->reg_ir;
			}
			break;

		case 3 :		// "push" low byte of PC
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = 0x100 + cpu->reg_ir;
				--cpu->reg_ir;
			}
			break;

		case 4 :		// "push" status register
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = 0x100 + cpu->reg_ir;
				--cpu->reg_ir;
			}
			break;

		case 5 :		// read low byte of the reset vector
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = 0xfffc;
			} else if (phase == CYCLE_END) {
				cpu->reg_pc = SET_LOBYTE(cpu->reg_pc, *cpu->bus_data);
			}
			break;

		case 6 :		// read high byte of the reset vector
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = 0xfffd;
			} else if (phase == CYCLE_END) {
				cpu->reg_pc = SET_HIBYTE(cpu->reg_pc, *cpu->bus_data);
				PRIVATE(cpu)->state = CS_RUNNING;
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;

		default:
			assert(0 && "invalid decode_cycle");
			break;
	}
}

static inline void fetch_memory(Cpu6502 *cpu, uint16_t addr, uint8_t *dst, CPU_6502_CYCLE phase) {
	assert(cpu);
	assert(dst);

	switch (phase) {
		case CYCLE_BEGIN : 
			PRIVATE(cpu)->internal_ab = addr;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			*dst = *cpu->bus_data;
			break;
	}
}

static inline void fetch_pc_memory(Cpu6502 *cpu, uint8_t *dst, CPU_6502_CYCLE phase) {
	assert(cpu);
	assert(dst);

	switch (phase) {
		case CYCLE_BEGIN : 
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			*dst = *cpu->bus_data;
			++cpu->reg_pc;
			break;
	}
}

static inline bool fetch_operand_immediate(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	assert(cpu);
	assert(PRIVATE(cpu)->decode_cycle == 1);

	fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
	return phase == CYCLE_END;
}

static inline bool fetch_operand_zeropage(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	bool result = false;

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->adl, phase);
			break;
		case 2 :		// fetch operand from memory
			PRIVATE(cpu)->adh = 0;
			fetch_memory(cpu, PRIVATE(cpu)->addr, &PRIVATE(cpu)->operand, phase);
			result = phase == CYCLE_END;
			break;
	};

	return result;
}

static inline bool fetch_operand_zeropage_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {

	bool result = false;

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->adl, phase);
			break;
		case 2 :		// fetch discarded data & add adl + reg_x
			switch (phase) {
				case CYCLE_BEGIN:
					// according to programming manual: cpu fetches data at base adress and discards it
					PRIVATE(cpu)->adh = 0;
					PRIVATE(cpu)->internal_ab = PRIVATE(cpu)->addr;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					// there is no page crossing in indexed zero page, a wrap-around occurs
					PRIVATE(cpu)->adl += index;
					break;
			};
			break;
		case 3 :		// fetch operand from memory
			fetch_memory(cpu, PRIVATE(cpu)->addr, &PRIVATE(cpu)->operand, phase);
			result = phase == CYCLE_END;
			break;
	};

	return result;
}

static inline bool fetch_operand_absolute(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	bool result = false;

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->adl, phase);
			break;
		case 2 :		// fetch address - high byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->adh, phase);
			break;
		case 3 :		// fetch operand from memory
			fetch_memory(cpu, PRIVATE(cpu)->addr, &PRIVATE(cpu)->operand, phase);
			result = phase == CYCLE_END;
			break;
	};

	return result;
}

static inline bool fetch_operand_absolute_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {
// cycle count depends on the value of the base adress and the index:
//	- if a page boundary is crossed (low-byte of address + index > 0xff) then it takes 5 cycles
//	- if no page boundary is crossed only 4 cycles are required

	bool result = false;

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->adl, phase);
			break;
		case 2 :		// fetch address - high byte & add adl + index
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->adh = 0;
					PRIVATE(cpu)->internal_ab = cpu->reg_pc;
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->addr += index;
					PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->adh > 0;
					break;
				case CYCLE_END:
					PRIVATE(cpu)->adh = *cpu->bus_data;
					++cpu->reg_pc;
					break;
			};
			break;
		case 3:			// fetch operand + add BAH + carry
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->internal_ab = PRIVATE(cpu)->addr;
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->adh += PRIVATE(cpu)->page_crossed;
					break;
				case CYCLE_END:
					PRIVATE(cpu)->operand = *cpu->bus_data;
					result = !PRIVATE(cpu)->page_crossed;
					break;
			};
			break;
		case 4 :		// fetch operand from memory (only when page crossing occured)
			fetch_memory(cpu, PRIVATE(cpu)->addr, &PRIVATE(cpu)->operand, phase);
			result = phase == CYCLE_END;
			break;
	};

	return result;
}

static inline bool fetch_operand_indexed_indirect(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	bool result = false;

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch discard memory & add zp + index-x
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->adl, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand += cpu->reg_x;
			}
			break;
		case 3:			// fetch address - low byte & increment zp + index-x
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->adl, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand += 1;
			}
			break;
		case 4:			// fetch address - high byte
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->adh, phase);
			break;
		case 5 :		// fetch operand from memory 
			fetch_memory(cpu, PRIVATE(cpu)->addr, &PRIVATE(cpu)->operand, phase);
			result = phase == CYCLE_END;
			break;
	}
	
	return result;
}

static inline bool fetch_operand_indirect_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	bool result = false;


	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch address - low byte & increment zp
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->adl, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand += 1;
			}
			break;
		case 3 :		// fetch address - high byte & add adl + y
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->adh, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->adl + cpu->reg_y > 255;
				PRIVATE(cpu)->adl += cpu->reg_y;
			}
			break;
		case 4 :
			fetch_memory(cpu, PRIVATE(cpu)->addr, &PRIVATE(cpu)->operand, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->adh += PRIVATE(cpu)->page_crossed;
				result = !PRIVATE(cpu)->page_crossed;
			}
			break;
		case 5 :		// fetch operand from memory 
			fetch_memory(cpu, PRIVATE(cpu)->addr, &PRIVATE(cpu)->operand, phase);
			result = phase == CYCLE_END;
			break;
	}

	return result;
}

static inline bool fetch_operand_g1(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (cpu->reg_ir & ADDR_6502_MASK) {
		case ADDR_6502_G1_INDX: return fetch_operand_indexed_indirect(cpu, phase);
		case ADDR_6502_G1_ZP:	return fetch_operand_zeropage(cpu, phase);
		case ADDR_6502_G1_IMM:	return fetch_operand_immediate(cpu, phase);
		case ADDR_6502_G1_ABS:	return fetch_operand_absolute(cpu, phase);
		case ADDR_6502_G1_INDY: return fetch_operand_indirect_indexed(cpu, phase);
		case ADDR_6502_G1_ZPX:	return fetch_operand_zeropage_indexed(cpu, phase, cpu->reg_x);
		case ADDR_6502_G1_ABSY:	return fetch_operand_absolute_indexed(cpu, phase, cpu->reg_y);
		case ADDR_6502_G1_ABSX:	return fetch_operand_absolute_indexed(cpu, phase, cpu->reg_x);
	};

	return false;
}

static inline void decode_lda(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (fetch_operand_g1(cpu, phase)) {
		cpu->reg_a	= PRIVATE(cpu)->operand;
		cpu->p_zero_result = cpu->reg_a == 0;
		cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_ldx(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
// also implements TAX and TSX

	bool op_ready = false;
	bool op_finished = false;

	switch ((cpu->reg_ir & ADDR_6502_MASK) >> 2) {
		case 0:		// immediate operand
			op_ready = fetch_operand_immediate(cpu, phase);
			break;
		case 1:		// zeropage addressing
			op_ready = fetch_operand_zeropage(cpu, phase);
			break;
		case 2:		// TAX: transfer accumulator to index-x
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			}
			if (phase == CYCLE_END) {
				cpu->reg_x = cpu->reg_a;
				op_finished = true;
			}
			break;
		case 3:		// absolute addressing
			op_ready = fetch_operand_absolute(cpu, phase);
			break;
		// case 4:  not defined
		case 5:		// zeropage, y indexed
			op_ready = fetch_operand_zeropage_indexed(cpu, phase, cpu->reg_y);
			break;
		case 6:		// TSX: transfer stack pointer to index-x
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			}
			if (phase == CYCLE_END) {
				cpu->reg_x = cpu->reg_sp;
				op_finished = true;
			}
		case 7:		// absolute, y-indexed
			op_ready = fetch_operand_absolute_indexed(cpu, phase, cpu->reg_y);
			break;
	}

	if (op_ready) {
		cpu->reg_x = PRIVATE(cpu)->operand;
		op_finished = true;
	}

	if (op_finished) {
		cpu->p_zero_result = cpu->reg_x == 0;
		cpu->p_negative_result = (cpu->reg_x & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_ldy(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
// also implements TAY 

	bool op_ready = false;
	bool op_finished = false;

	switch ((cpu->reg_ir & ADDR_6502_MASK) >> 2) {
		case 0:		// immediate operand
			op_ready = fetch_operand_immediate(cpu, phase);
			break;
		case 1:		// zeropage addressing
			op_ready = fetch_operand_zeropage(cpu, phase);
			break;
		case 2:		// TAY: transfer accumulator to index-y
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			}
			if (phase == CYCLE_END) {
				cpu->reg_y = cpu->reg_a;
				op_finished = true;
			}
			break;
		case 3:		// absolute addressing
			op_ready = fetch_operand_absolute(cpu, phase);
			break;
		// case 4:  BCS - handled elsewhere
		case 5:		// zeropage, x indexed
			op_ready = fetch_operand_zeropage_indexed(cpu, phase, cpu->reg_x);
			break;
		// case 6 :	CLV - handled elsewhere
		case 7:		// absolute, x-indexed
			op_ready = fetch_operand_absolute_indexed(cpu, phase, cpu->reg_x);
			break;
	}

	if (op_ready) {
		cpu->reg_y = PRIVATE(cpu)->operand;
		op_finished = true;
	}

	if (op_finished) {
		cpu->p_zero_result = cpu->reg_y == 0;
		cpu->p_negative_result = (cpu->reg_y & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_instruction(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (cpu->reg_ir & AC_6502_MASK) {
		case AC_6502_LDA :
			decode_lda(cpu, phase);
			break;
		case AC_6502_LDX :
			decode_ldx(cpu, phase);
			break;
		case AC_6502_LDY :
			switch (cpu->reg_ir) {
				case OP_6502_BCS : 
					break;
				case OP_6502_CLV :
					break;
				default : 
					decode_ldy(cpu, phase);
			};
			break;
	};
}

static inline void cpu_6502_execute_phase(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	// initialization is treated seperately
	if (PRIVATE(cpu)->state == CS_INIT) {
		execute_init(cpu, phase);
		return;
	}
	
	// first cycle is always filling the instruction register with the new opcode
	if (PRIVATE(cpu)->decode_cycle == 0) {
		fetch_pc_memory(cpu, &cpu->reg_ir, phase);
	} else {
		decode_instruction(cpu, phase);
	}

}

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

Cpu6502 *cpu_6502_create(uint16_t *addres_bus, uint8_t *data_bus, const bool *clock, const bool *reset, bool *rw) {
	assert(addres_bus);
	assert(data_bus);
	assert(clock);
	assert(reset);
	assert(rw);

	Cpu6502_private *cpu = (Cpu6502_private *) malloc(sizeof(Cpu6502_private));
	memset(cpu, 0, sizeof(Cpu6502_private));

	cpu->intf.bus_address = addres_bus;
	cpu->intf.bus_data = data_bus;
	cpu->intf.pin_clock = clock;
	cpu->intf.pin_reset = reset;
	cpu->intf.pin_rw = rw;
	cpu->decode_cycle = -1;
	cpu->state = CS_INIT;
	
	return &cpu->intf;
}

void cpu_6502_process(Cpu6502 *cpu) {
	assert(cpu);
	Cpu6502_private *priv = (Cpu6502_private *) cpu;

	// check for changes in the reset line
	if (!priv->prev_reset && *cpu->pin_reset) {
		// reset was just asserted
		priv->internal_ab = 0;

		priv->internal_rw = RW_READ;
		priv->prev_reset = cpu->pin_reset;
	} else if (priv->prev_reset && !*cpu->pin_reset) {
		// reset was just deasserted - start initialization sequence
		priv->prev_reset = *cpu->pin_reset;
		priv->state = CS_INIT;
		priv->decode_cycle = -1;
	}

	// do nothing if reset is asserted or if not on the edge of a clock cycle
	if (*cpu->pin_reset || *cpu->pin_clock == priv->prev_clock) {
		process_end(cpu);
		return;
	}

	if (*cpu->pin_clock == false) {
		// a negative going clock ends the previous cycle and starts a new cycle
		if (priv->decode_cycle >= 0) {
			cpu_6502_execute_phase(cpu, CYCLE_END);
		}

		++priv->decode_cycle;
		cpu_6502_execute_phase(cpu, CYCLE_BEGIN);
	} else {
		// a positive going clock marks the halfway point of the cycle
		cpu_6502_execute_phase(cpu, CYCLE_MIDDLE);
	}

	process_end(cpu);
	return;
}
