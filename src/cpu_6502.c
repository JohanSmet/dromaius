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

typedef enum CPU_6502_STATE {
	CS_INIT = 0,
	CS_RUNNING = 1,
	CS_IN_IRQ = 2,
	CS_IN_NMI = 3
} CPU_6502_STATE;

typedef enum CPU_6502_INTERRUPT_TYPE {
	INTR_RESET = 0,
	INTR_BRK = 1,
	INTR_IRQ = 2,
	INTR_NMI = 3
} CPU_6502_INTERRUPT_TYPE;

typedef union addr_t {
	uint16_t full;
	struct {
		uint8_t lo_byte;
		uint8_t hi_byte;
	};
} addr_t;

typedef struct Cpu6502_private {
	Cpu6502			intf;
	bool			prev_reset;
	bool			prev_clock;
	bool			prev_nmi;
	uint16_t		internal_ab;			// internal address bus
	bool			internal_rw;			// internal rw latch
	CPU_6502_STATE	state;
	int8_t			decode_cycle;			// instruction decode cycle
	uint16_t		override_pc;			// != 0: fetch next instruction from this location, overruling the current pc
	uint8_t			operand;
	addr_t			addr;
	addr_t			i_addr;
	bool			page_crossed;
	bool			nmi_triggered;
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
	*cpu->pin_sync	  = PRIVATE(cpu)->decode_cycle == 0;

	// store state of the clock pin
	PRIVATE(cpu)->prev_clock = *cpu->pin_clock;
}

static void interrupt_sequence(Cpu6502 *cpu, CPU_6502_CYCLE phase, CPU_6502_INTERRUPT_TYPE irq_type) {

	static const bool FORCE_READ[] = {
		true,		// reset
		false,		// BRK-instruction
		false,		// regular IRQ
		false		// non-maskable IRQ
	};

	static const uint16_t VECTOR_LOW[] = {
		0xfffc,		// reset
		0xfffe,		// BRK-instruction
		0xfffe,		// regular IRQ
		0xfffa		// non-maskable IRQ
	};

	static const uint16_t VECTOR_HIGH[] = {
		0xfffd,		// reset
		0xffff,		// BRK-instruction
		0xffff,		// regular IRQ
		0xfffb		// non-maskable IRQ
	};

/*	static const uint16_t PC_INC[] = {
		1,			// reset
		1,			// BRK-instruction
		0,			// regular IRQ
		0			// non-maskable IRQ
	}; */

	switch(PRIVATE(cpu)->decode_cycle) {
		case 0 :		// finish previous operation
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			}
			break;
		case 1 :		// force a BRK instruction
			if (phase == CYCLE_BEGIN) {
				cpu->reg_ir = OP_6502_BRK;
			}
			break;
		case 2 :		// push high byte of PC
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
					PRIVATE(cpu)->internal_rw = RW_WRITE | FORCE_READ[irq_type];
					break;
				case CYCLE_MIDDLE:
					*cpu->bus_data = HI_BYTE(cpu->reg_pc);
					break;
				case CYCLE_END:
					cpu->reg_sp -= 1;
					PRIVATE(cpu)->internal_rw = RW_READ;
					break;
			}
			break;
		case 3 :		// push low byte of PC
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
					PRIVATE(cpu)->internal_rw = RW_WRITE | FORCE_READ[irq_type];
					break;
				case CYCLE_MIDDLE:
					*cpu->bus_data = LO_BYTE(cpu->reg_pc);
					break;
				case CYCLE_END:
					cpu->reg_sp -= 1;
					PRIVATE(cpu)->internal_rw = RW_READ;
					break;
			}
			break;
		case 4 :		// push processor state
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
					PRIVATE(cpu)->internal_rw = RW_WRITE | FORCE_READ[irq_type];
					break;
				case CYCLE_MIDDLE:
					*cpu->bus_data = cpu->reg_p;
					break;
				case CYCLE_END:
					cpu->reg_sp -= 1;
					PRIVATE(cpu)->internal_rw = RW_READ;
					break;
			}
			break;
		case 5 :		// read low byte of the reset vector
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = VECTOR_LOW[irq_type];
			} else if (phase == CYCLE_END) {
				cpu->reg_pc = SET_LO_BYTE(cpu->reg_pc, *cpu->bus_data);
			}
			break;
		case 6 :		// read high byte of the reset vector
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = VECTOR_HIGH[irq_type];
			} else if (phase == CYCLE_END) {
				cpu->reg_pc = SET_HI_BYTE(cpu->reg_pc, *cpu->bus_data);
				cpu->p_interrupt_disable = irq_type != INTR_RESET;
				PRIVATE(cpu)->state = CS_RUNNING;
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}

static void execute_init(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	interrupt_sequence(cpu, phase, INTR_RESET);
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

static inline void store_memory(Cpu6502 *cpu, uint16_t addr, uint8_t value, CPU_6502_CYCLE phase) {
	assert(cpu);

	switch (phase) {
		case CYCLE_BEGIN : 
			PRIVATE(cpu)->internal_ab = addr;
			PRIVATE(cpu)->internal_rw = RW_WRITE;
			break;
		case CYCLE_MIDDLE:
			*cpu->bus_data = value;
			break;
		case CYCLE_END : 
			PRIVATE(cpu)->internal_rw = RW_READ;
			break;
	}
}

static inline void fetch_zp_discard_add(Cpu6502 *cpu, uint8_t index, CPU_6502_CYCLE phase) {
// utility function for indexed zero-page memory accesses
//	- according to programming manual: cpu fetches data at base adress and discards it
//  - there is no page crossing in indexed zero page, a wrap-around occurs

	assert(cpu);

	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->addr.hi_byte = 0;
			PRIVATE(cpu)->internal_ab = PRIVATE(cpu)->addr.full;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			PRIVATE(cpu)->addr.lo_byte += index;
			break;
	};
}

static inline void fetch_high_byte_address_indexed(Cpu6502 *cpu, uint8_t index, CPU_6502_CYCLE phase) {
// utility function for indexed absolute addressed memory accesses
// fetch high byte of address while adding the offset to the low byte

	assert(cpu);

	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->addr.hi_byte = 0;
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			PRIVATE(cpu)->addr.full += index;
			PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->addr.hi_byte > 0;
			break;
		case CYCLE_END:
			PRIVATE(cpu)->addr.hi_byte = *cpu->bus_data;
			++cpu->reg_pc;
			break;
	};
}

static inline bool fetch_memory_page_crossed(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	assert(cpu);

	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = PRIVATE(cpu)->addr.full;
			return false;
		case CYCLE_MIDDLE:
			PRIVATE(cpu)->addr.hi_byte += PRIVATE(cpu)->page_crossed;
			return false;
		case CYCLE_END:
			PRIVATE(cpu)->operand = *cpu->bus_data;
			return !PRIVATE(cpu)->page_crossed;
	};
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

static inline int8_t fetch_address_immediate(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	assert(cpu);

	if (PRIVATE(cpu)->decode_cycle == 1) {
		switch (phase) {
			case CYCLE_BEGIN :
				PRIVATE(cpu)->addr.full = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END :
				++cpu->reg_pc;
				break;
		}
	}

	return 1;
}

static inline int8_t fetch_address_zeropage(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			PRIVATE(cpu)->addr.hi_byte = 0;
			break;
	};

	return 2;
}

static inline int8_t fetch_address_zeropage_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// fetch discarded data & add addr.lo_byte + offset
			fetch_zp_discard_add(cpu, index, phase);
			break;
	};

	return 3;
}

static inline int8_t fetch_address_absolute(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	bool result = false;

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// fetch address - high byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.hi_byte, phase);
			break;
	};

	return 3;
}

static inline int8_t fetch_address_absolute_indexed_shortcut(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {
// cycle count depends on the value of the base adress and the index:
//	- if a page boundary is crossed (low-byte of address + index > 0xff) then it takes 5 cycles
//	- if no page boundary is crossed only 4 cycles are required

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// fetch address - high byte & add addr.lo_byte + index
			fetch_high_byte_address_indexed(cpu, index, phase);
			break;
		case 3:			// fetch operand + add BAH + carry
			fetch_memory_page_crossed(cpu, phase);
			break;
	};

	return 3 + PRIVATE(cpu)->page_crossed;
}

static inline int8_t fetch_address_absolute_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {
// no lookup-ahead (or shortcut) when the indexing does not case page crossing
//	used for operations that write to memory

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// fetch address - high byte & add addr.lo_byte + index
			fetch_high_byte_address_indexed(cpu, index, phase);
			break;
		case 3:			// fetch operand + add BAH + carry
			fetch_memory_page_crossed(cpu, phase);
			break;
	};

	return 4;
}


static inline int8_t fetch_address_indirect(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch indirect address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->i_addr.lo_byte, phase);
			break;
		case 2 :		// fetch indirect address - high byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->i_addr.hi_byte, phase);
			break;
		case 3 :		// fetch jump address - low byte
			fetch_memory(cpu, PRIVATE(cpu)->i_addr.full, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 4 :		// fetch jump address - high byte
			fetch_memory(cpu, PRIVATE(cpu)->i_addr.full + 1, &PRIVATE(cpu)->addr.hi_byte, phase);
			break;
	}

	return 5;
}

static inline int8_t fetch_address_indexed_indirect(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch discard memory & add zp + index-x
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand += cpu->reg_x;
			}
			break;
		case 3:			// fetch address - low byte & increment zp + index-x
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand += 1;
			}
			break;
		case 4:			// fetch address - high byte
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.hi_byte, phase);
			break;
	}
	
	return 5;
}

static inline int8_t fetch_address_indirect_indexed_shortcut(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch address - low byte & increment zp
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand += 1;
			}
			break;
		case 3 :		// fetch address - high byte & add addr.lo_byte + y
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.hi_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->addr.lo_byte + cpu->reg_y > 255;
				PRIVATE(cpu)->addr.lo_byte += cpu->reg_y;
			}
			break;
		case 4 :
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->addr.hi_byte += PRIVATE(cpu)->page_crossed;
			}
			break;
	}

	return 4 + PRIVATE(cpu)->page_crossed;
}

static inline int8_t fetch_address_indirect_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch address - low byte & increment zp
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand += 1;
			}
			break;
		case 3 :		// fetch address - high byte & add addr.lo_byte + y
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.hi_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->addr.lo_byte + cpu->reg_y > 255;
				PRIVATE(cpu)->addr.lo_byte += cpu->reg_y;
			}
			break;
		case 4 :
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->addr.hi_byte += PRIVATE(cpu)->page_crossed;
			}
			break;
	}

	return 5;
}

static inline int8_t fetch_address_shortcut(Cpu6502 *cpu, ADDRESSING_MODE_6502 mode, CPU_6502_CYCLE phase) {

	switch (mode) {
		case AM_6502_IMMEDIATE:
			return fetch_address_immediate(cpu, phase);
		case AM_6502_ZEROPAGE:
			return fetch_address_zeropage(cpu, phase);
		case AM_6502_ZEROPAGE_X:
			return fetch_address_zeropage_indexed(cpu, phase, cpu->reg_x);
		case AM_6502_ZEROPAGE_Y:
			return fetch_address_zeropage_indexed(cpu, phase, cpu->reg_y);
		case AM_6502_ABSOLUTE:
			return fetch_address_absolute(cpu, phase);
		case AM_6502_ABSOLUTE_X:
			return fetch_address_absolute_indexed_shortcut(cpu, phase, cpu->reg_x);
		case AM_6502_ABSOLUTE_Y:
			return fetch_address_absolute_indexed_shortcut(cpu, phase, cpu->reg_y);
		case AM_6502_INDIRECT:
			return fetch_address_indirect(cpu, phase);
		case AM_6502_INDIRECT_X:
			return fetch_address_indexed_indirect(cpu, phase);
		case AM_6502_INDIRECT_Y:
			return fetch_address_indirect_indexed_shortcut(cpu, phase);
		default:
			return 0;
	};
}

static inline int8_t fetch_address(Cpu6502 *cpu, ADDRESSING_MODE_6502 mode, CPU_6502_CYCLE phase) {

	switch (mode) {
		case AM_6502_IMMEDIATE:	
			return fetch_address_immediate(cpu, phase);
		case AM_6502_ZEROPAGE:	
			return fetch_address_zeropage(cpu, phase);
		case AM_6502_ZEROPAGE_X:	
			return fetch_address_zeropage_indexed(cpu, phase, cpu->reg_x);
		case AM_6502_ZEROPAGE_Y:	
			return fetch_address_zeropage_indexed(cpu, phase, cpu->reg_y);
		case AM_6502_ABSOLUTE:	
			return fetch_address_absolute(cpu, phase);
		case AM_6502_ABSOLUTE_X:	
			return fetch_address_absolute_indexed(cpu, phase, cpu->reg_x);
		case AM_6502_ABSOLUTE_Y:	
			return fetch_address_absolute_indexed(cpu, phase, cpu->reg_y);
		case AM_6502_INDIRECT:
			return fetch_address_indirect(cpu, phase);
		case AM_6502_INDIRECT_X: 
			return fetch_address_indexed_indirect(cpu, phase);
		case AM_6502_INDIRECT_Y: 
			return fetch_address_indirect_indexed(cpu, phase);
		default:
			return 0;
	};
}

static inline bool fetch_operand(Cpu6502 *cpu, ADDRESSING_MODE_6502 mode, CPU_6502_CYCLE phase) {

	bool result = false;

	int8_t memop_cycle = fetch_address_shortcut(cpu, mode, phase);

	if (memop_cycle == PRIVATE(cpu)->decode_cycle) {
		fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
		result = phase == CYCLE_END;
	}

	return result;
}

static inline bool fetch_operand_g1(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	return fetch_operand(cpu, (cpu->reg_ir & ADDR_6502_MASK) >> 2, phase);
}

static inline bool store_to_memory(Cpu6502 *cpu, uint8_t value, ADDRESSING_MODE_6502 mode, CPU_6502_CYCLE phase) {

	bool result = false;

	int8_t memop_cycle = fetch_address(cpu, mode, phase);

	if (memop_cycle == PRIVATE(cpu)->decode_cycle) {
		store_memory(cpu, PRIVATE(cpu)->addr.full, value, phase);
		result = phase == CYCLE_END;
	}

	return result;
}

static inline bool store_to_memory_g1(Cpu6502 *cpu, uint8_t value, CPU_6502_CYCLE phase) {
	return store_to_memory(cpu, value, (cpu->reg_ir & ADDR_6502_MASK) >> 2, phase);
}

static inline void stack_push(Cpu6502 *cpu, uint8_t value, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
			PRIVATE(cpu)->internal_rw = RW_WRITE;
			break;
		case CYCLE_MIDDLE:
			*cpu->bus_data = value;
			break;
		case CYCLE_END:
			cpu->reg_sp -= 1;
			PRIVATE(cpu)->internal_rw = RW_READ;
			break;
	}
}

static inline uint8_t stack_pop(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	uint8_t result = 0;

	switch (phase) {
		case CYCLE_BEGIN:
			cpu->reg_sp += 1;
			PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			result = *cpu->bus_data;
			break;
	}

	return result;
}

static inline void decode_branch_instruction(Cpu6502 *cpu, uint8_t bit, uint8_t value, CPU_6502_CYCLE phase) {
// the branching instructions follow the 6502's philosophy of doing the least amount of work possible
// -> 2 cycles are always needed to fetch the opcode (before this function) and to fetch the offset (relative addressing)
// -> if the branch isn't taken, the work ends there and the program continues with the next instruction (reg_pc)
// -> if the branch is taken: the offset is added in the 3th cycle
// -> only if a page boundary is crossed a 4th cycle is required

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1:		// fetch offset
			fetch_pc_memory(cpu, &PRIVATE(cpu)->i_addr.lo_byte, phase);
			if (phase == CYCLE_END && IS_BIT_SET(cpu->reg_p, bit) != value) {
				// check flags + stop if branch not taken
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
		case 2:		// add relative address + check for page crossing
			switch (phase) {
				case CYCLE_BEGIN: 
					PRIVATE(cpu)->internal_ab = cpu->reg_pc;
					PRIVATE(cpu)->i_addr.hi_byte = HI_BYTE(cpu->reg_pc);
					cpu->reg_pc += (int8_t) PRIVATE(cpu)->i_addr.lo_byte;
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->i_addr.hi_byte != HI_BYTE(cpu->reg_pc);
					break;
				case CYCLE_END:
					if (!PRIVATE(cpu)->page_crossed) {
						PRIVATE(cpu)->decode_cycle = -1;
					}
					break;
			}
			break;
		case 3:		// extra cycle when page crossed to add carry to high address
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->i_addr.lo_byte = LO_BYTE(cpu->reg_pc);
					PRIVATE(cpu)->internal_ab = PRIVATE(cpu)->i_addr.full;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void calculate_adc_decimal(Cpu6502 *cpu) {
/* ADC (and SBC) effect the C/N/V/Z-flags, even in decimal mode. On a 6502 only the carry is supported and valid.
   But the other flags are still affected, we do are best to emulate this behaviour.
   Many thanks go to Bruce Clark, for his excellent explanation of the decimal mode of the 6502 (see: http://www.6502.org/tutorials/decimal_mode.html)
   and the accompanying test program.
*/
	uint16_t bin_result = cpu->reg_a + PRIVATE(cpu)->operand + cpu->p_carry;

	uint16_t al = (cpu->reg_a & 0x0f) + (PRIVATE(cpu)->operand & 0x0f) + cpu->p_carry;
	if (al >= 0x0a) {
		al = ((al + 0x06) & 0x0f) + 0x10;
	}
	uint16_t a_seq1 = (cpu->reg_a & 0xf0) + (PRIVATE(cpu)->operand & 0xf0) + al;
	if (a_seq1 >= 0xa0) {
		a_seq1 += 0x60;
	}

	int16_t a_seq2 = (int8_t) (cpu->reg_a & 0xf0) + (int8_t) (PRIVATE(cpu)->operand & 0xf0) + al;

	cpu->reg_a = a_seq1 & 0x00ff;
	cpu->p_carry = (a_seq1 >= 0x0100);
	cpu->p_negative_result = IS_BIT_SET(a_seq2, 7);
	cpu->p_overflow = (a_seq2 < -128) || (a_seq2 > 127);
	cpu->p_zero_result = (bin_result & 0xff) == 0x00;
}

static inline void decode_adc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (!fetch_operand_g1(cpu, phase)) {
		return;
	}

	if (!cpu->p_decimal_mode) {
		int16_t s_result = (int8_t) cpu->reg_a + (int8_t) PRIVATE(cpu)->operand + cpu->p_carry;
		uint16_t u_result = cpu->reg_a + PRIVATE(cpu)->operand + cpu->p_carry;
		cpu->reg_a = u_result & 0x00ff;
		cpu->p_carry = IS_BIT_SET(u_result, 8);
		cpu->p_overflow = (s_result < -128) || (s_result > 127);
		cpu->p_zero_result = cpu->reg_a == 0;
		cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
	} else {
		calculate_adc_decimal(cpu);
	}

	PRIVATE(cpu)->decode_cycle = -1;
}

static inline void decode_and(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (fetch_operand_g1(cpu, phase)) {
		cpu->reg_a	= cpu->reg_a & PRIVATE(cpu)->operand;
		cpu->p_zero_result = cpu->reg_a == 0;
		cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_asl(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (cpu->reg_ir == OP_6502_ASL_ACC) {
		switch (phase) {
			case CYCLE_BEGIN:
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END:
				cpu->p_carry = (cpu->reg_a & 0b10000000) >> 7;
				cpu->reg_a <<= 1;
				cpu->p_zero_result = cpu->reg_a == 0;
				cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
				PRIVATE(cpu)->decode_cycle = -1;
				break;
		}
		return;
	}

	static const int8_t AM_LUT[] = {
		-1,						// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	uint8_t memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->internal_rw = RW_WRITE;
					break;
				case CYCLE_END:
					cpu->p_carry = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->operand <<= 1;
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					*cpu->bus_data = PRIVATE(cpu)->operand;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					PRIVATE(cpu)->internal_rw = RW_READ;
					cpu->p_zero_result = PRIVATE(cpu)->operand == 0;
					cpu->p_negative_result = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_bcc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 0, false, phase);
}

static inline void decode_bcs(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 0, true, phase);
}

static inline void decode_beq(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 1, true, phase);
}

static inline void decode_bit(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const int8_t AM_LUT[] = {
		-1,						// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		-1,						// 5
		-1,						// 6
		-1						// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		cpu->p_negative_result = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
		cpu->p_overflow = (PRIVATE(cpu)->operand & 0b01000000) >> 6;
		cpu->p_zero_result = (PRIVATE(cpu)->operand & cpu->reg_a) == 0;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_bmi(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 7, true, phase);
}

static inline void decode_bne(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 1, false, phase);
}

static inline void decode_bpl(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 7, false, phase);
}

static inline void decode_brk(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (PRIVATE(cpu)->decode_cycle == 1 && phase == CYCLE_BEGIN) {
		cpu->p_break_command = true;
	}

	interrupt_sequence(cpu, phase, INTR_BRK);
}

static inline void decode_bvc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 6, false, phase);
}

static inline void decode_bvs(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	decode_branch_instruction(cpu, 6, true, phase);
}

static inline void decode_clc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->p_carry = false;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_cld(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->p_decimal_mode = false;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_cli(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->p_interrupt_disable = false;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_clv(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->p_overflow = false;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_cmp(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (fetch_operand_g1(cpu, phase)) {
		int8_t result = cpu->reg_a - PRIVATE(cpu)->operand;
		cpu->p_zero_result = result == 0;
		cpu->p_negative_result = (result & 0b10000000) >> 7;
		cpu->p_carry = result >= 0;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_cpx_cpy(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t reg) {

	static const int8_t AM_LUT[] = {
		AM_6502_IMMEDIATE,		// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		-1,						// 5
		-1,						// 6
		-1						// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		int8_t result = reg - PRIVATE(cpu)->operand;
		cpu->p_zero_result = result == 0;
		cpu->p_negative_result = (result & 0b10000000) >> 7;
		cpu->p_carry = result >= 0;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_dec(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const int8_t AM_LUT[] = {
		-1,						// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	uint8_t memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform decrement / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->internal_rw = RW_WRITE;
					break;
				case CYCLE_END:
					PRIVATE(cpu)->operand -= 1;
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					*cpu->bus_data = PRIVATE(cpu)->operand;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					PRIVATE(cpu)->internal_rw = RW_READ;
					cpu->p_zero_result = PRIVATE(cpu)->operand == 0;
					cpu->p_negative_result = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_dex(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->reg_x -= 1;
			cpu->p_zero_result = cpu->reg_x == 0;
			cpu->p_negative_result = (cpu->reg_x & 0b10000000) >> 7;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_dey(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->reg_y -= 1;
			cpu->p_zero_result = cpu->reg_y == 0;
			cpu->p_negative_result = (cpu->reg_y & 0b10000000) >> 7;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_eor(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (fetch_operand_g1(cpu, phase)) {
		cpu->reg_a	= cpu->reg_a ^ PRIVATE(cpu)->operand;
		cpu->p_zero_result = cpu->reg_a == 0;
		cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_inc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const int8_t AM_LUT[] = {
		-1,						// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	uint8_t memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform increment / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->internal_rw = RW_WRITE;
					break;
				case CYCLE_END:
					PRIVATE(cpu)->operand += 1;
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					*cpu->bus_data = PRIVATE(cpu)->operand;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					PRIVATE(cpu)->internal_rw = RW_READ;
					cpu->p_zero_result = PRIVATE(cpu)->operand == 0;
					cpu->p_negative_result = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_inx(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->reg_x += 1;
			cpu->p_zero_result = cpu->reg_x == 0;
			cpu->p_negative_result = (cpu->reg_x & 0b10000000) >> 7;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_iny(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			cpu->reg_y += 1;
			cpu->p_zero_result = cpu->reg_y == 0;
			cpu->p_negative_result = (cpu->reg_y & 0b10000000) >> 7;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_jmp(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	int8_t memop_cycle = 0;

	if (cpu->reg_ir == OP_6502_JMP_ABS) {
		memop_cycle = fetch_address_absolute(cpu, phase);
	} else {
		memop_cycle = fetch_address_indirect(cpu, phase);
	}

	if (PRIVATE(cpu)->decode_cycle == memop_cycle - 1 && phase == CYCLE_END) {
		cpu->reg_pc = PRIVATE(cpu)->addr.full;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_jsr(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	
	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address low byte (adl)
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// store adl in cpu, put stack pointer on address bus
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// push program_counter - high byte
			stack_push(cpu, HI_BYTE(cpu->reg_pc), phase);
			break;
		case 4 :		// push program_counter - low byte
			stack_push(cpu, LO_BYTE(cpu->reg_pc), phase);
			break;
		case 5 :		// fetch address high byte (adh)
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.hi_byte, phase);
			if (phase == CYCLE_END) {
				cpu->reg_pc = PRIVATE(cpu)->addr.full;
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
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

	static const int8_t AM_LUT[] = {
		AM_6502_IMMEDIATE,		// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_Y,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_Y		// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		cpu->reg_x = PRIVATE(cpu)->operand;
		cpu->p_zero_result = cpu->reg_x == 0;
		cpu->p_negative_result = (cpu->reg_x & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_ldy(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const int8_t AM_LUT[] = {
		AM_6502_IMMEDIATE,		// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		cpu->reg_y = PRIVATE(cpu)->operand;
		cpu->p_zero_result = cpu->reg_y == 0;
		cpu->p_negative_result = (cpu->reg_y & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_lsr(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (cpu->reg_ir == OP_6502_LSR_ACC) {
		switch (phase) {
			case CYCLE_BEGIN:
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END:
				cpu->p_carry = cpu->reg_a & 0b00000001;
				cpu->reg_a >>= 1;
				cpu->p_zero_result = cpu->reg_a == 0;
				cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
				PRIVATE(cpu)->decode_cycle = -1;
				break;
		}
		return;
	}

	static const int8_t AM_LUT[] = {
		-1,						// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	uint8_t memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->internal_rw = RW_WRITE;
					break;
				case CYCLE_END:
					cpu->p_carry = PRIVATE(cpu)->operand & 0b00000001;
					PRIVATE(cpu)->operand >>= 1;
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					*cpu->bus_data = PRIVATE(cpu)->operand;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					PRIVATE(cpu)->internal_rw = RW_READ;
					cpu->p_zero_result = PRIVATE(cpu)->operand == 0;
					cpu->p_negative_result = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_nop(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END : 
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_pha(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch discard data & decode pha
			fetch_memory(cpu, cpu->reg_pc, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 : 
			stack_push(cpu, cpu->reg_a, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}

static inline void decode_php(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch discard data & decode php
			fetch_memory(cpu, cpu->reg_pc, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 : 
			// apparently the break bit is also set by PHP
			// (I didn't see that mentioned in the programming manual, or I missed it, but several only resources mention this.)
			stack_push(cpu, cpu->reg_p | 0b00010000, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}

static inline void decode_pla(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch discard data & decode pla
			fetch_memory(cpu, cpu->reg_pc, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// read stack, 
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// pop value from stack
			cpu->reg_a = stack_pop(cpu, phase);
			if (phase == CYCLE_END) {
				cpu->p_zero_result = cpu->reg_a == 0;
				cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}

static inline void decode_plp(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch discard data & decode plp
			fetch_memory(cpu, cpu->reg_pc, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// read stack
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// pop value from stack
			cpu->reg_p = stack_pop(cpu, phase);
			cpu->p_expension = true;		// reserved bit is always set
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}


static inline void decode_ora(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (fetch_operand_g1(cpu, phase)) {
		cpu->reg_a	= cpu->reg_a | PRIVATE(cpu)->operand;
		cpu->p_zero_result = cpu->reg_a == 0;
		cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_rol(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (cpu->reg_ir == OP_6502_ROL_ACC) {
		switch (phase) {
			case CYCLE_BEGIN:
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END: {
				uint8_t carry = cpu->p_carry;
				cpu->p_carry = (cpu->reg_a & 0b10000000) >> 7;
				cpu->reg_a = (cpu->reg_a << 1) | carry;
				cpu->p_zero_result = cpu->reg_a == 0;
				cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
				PRIVATE(cpu)->decode_cycle = -1;
				break;
			}
		}
		return;
	}

	static const int8_t AM_LUT[] = {
		-1,						// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	uint8_t memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->internal_rw = RW_WRITE;
					break;
				case CYCLE_END: {
					uint8_t carry = cpu->p_carry;
					cpu->p_carry = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->operand = (PRIVATE(cpu)->operand << 1) | carry;
					break;
				}
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					*cpu->bus_data = PRIVATE(cpu)->operand;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					PRIVATE(cpu)->internal_rw = RW_READ;
					cpu->p_zero_result = PRIVATE(cpu)->operand == 0;
					cpu->p_negative_result = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}


static inline void decode_ror(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (cpu->reg_ir == OP_6502_ROR_ACC) {
		switch (phase) {
			case CYCLE_BEGIN:
				PRIVATE(cpu)->internal_ab = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END: {
				uint8_t carry = cpu->p_carry << 7;
				cpu->p_carry = cpu->reg_a & 0b00000001;
				cpu->reg_a = (cpu->reg_a >> 1) | carry;
				cpu->p_zero_result = cpu->reg_a == 0;
				cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
				PRIVATE(cpu)->decode_cycle = -1;
				break;
			}
		}
		return;
	}

	static const int8_t AM_LUT[] = {
		-1,						// 0
		AM_6502_ZEROPAGE,		// 1
		-1,						// 2
		AM_6502_ABSOLUTE,		// 3
		-1,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		-1,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	uint8_t memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->internal_rw = RW_WRITE;
					break;
				case CYCLE_END: {
					uint8_t carry = cpu->p_carry << 7;
					cpu->p_carry = PRIVATE(cpu)->operand & 0b00000001;
					PRIVATE(cpu)->operand = (PRIVATE(cpu)->operand >> 1) | carry;
					break;
				}
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					*cpu->bus_data = PRIVATE(cpu)->operand;
					break;
				case CYCLE_MIDDLE:
					break;
				case CYCLE_END:
					PRIVATE(cpu)->internal_rw = RW_READ;
					cpu->p_zero_result = PRIVATE(cpu)->operand == 0;
					cpu->p_negative_result = (PRIVATE(cpu)->operand & 0b10000000) >> 7;
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_rts(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	
	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch discard data & decode rts
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// "increment stack pointer
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// pop program_counter - low byte
			cpu->reg_pc = SET_LO_BYTE(cpu->reg_pc, stack_pop(cpu, phase));
			break;
		case 4 :		// pop program_counter - low byte
			cpu->reg_pc = SET_HI_BYTE(cpu->reg_pc, stack_pop(cpu, phase));
			break;
		case 5 :		// increment program counter
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}

static inline void decode_rti(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	
	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// decode RTI
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			break;
		case 2 :		// "increment" stack pointer
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->internal_ab = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// pop status register
			cpu->reg_p = stack_pop(cpu, phase);
			if (phase == CYCLE_END) {
				cpu->p_break_command = false;
			}
			break;
		case 4 :		// pop program_counter - low byte
			cpu->reg_pc = SET_LO_BYTE(cpu->reg_pc, stack_pop(cpu, phase));
			break;
		case 5 :		// pop program_counter - low byte
			cpu->reg_pc = SET_HI_BYTE(cpu->reg_pc, stack_pop(cpu, phase));
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}

static inline void decode_sbc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (!fetch_operand_g1(cpu, phase)) {
		return;
	}

	// on a 6502 the C/N/V/Z-flags are set using the binary mode computation.
	uint16_t u_result = cpu->reg_a + (uint8_t) ~PRIVATE(cpu)->operand + cpu->p_carry;
	int16_t s_result = (int8_t) cpu->reg_a - (int8_t) PRIVATE(cpu)->operand - !cpu->p_carry;

	if (!cpu->p_decimal_mode) {
		cpu->reg_a = u_result & 0x00ff;
	} else {
		int16_t al = (cpu->reg_a & 0x0f) - (PRIVATE(cpu)->operand & 0x0f) + cpu->p_carry - 1;
		if (al < 0) {
			al = ((al - 0x06) & 0x0f) - 0x10;
		}
		int16_t a_seq3 = (cpu->reg_a & 0xf0) - (PRIVATE(cpu)->operand & 0xf0) + al;
		if (a_seq3 < 0) {
			a_seq3 -= 0x60;
		}
		cpu->reg_a = a_seq3 & 0x00ff;
	}

	cpu->p_carry = IS_BIT_SET(u_result, 8);
	cpu->p_overflow = (s_result < -128) || (s_result > 127);
	cpu->p_zero_result = (u_result & 0x00ff) == 0;
	cpu->p_negative_result = (u_result & 0b10000000) >> 7;

	PRIVATE(cpu)->decode_cycle = -1;
}

static inline void decode_sec(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			cpu->p_carry = true;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_sed(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			cpu->p_decimal_mode = true;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_sei(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->internal_ab = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			cpu->p_interrupt_disable = true;
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_sta(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (store_to_memory_g1(cpu, cpu->reg_a, phase)) {
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_stx(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	
	bool op_ready = false;

	switch (cpu->reg_ir) {
		case OP_6502_STX_ZP :
			op_ready = store_to_memory(cpu, cpu->reg_x, AM_6502_ZEROPAGE, phase);
			break;
		case OP_6502_STX_ZPY :
			op_ready = store_to_memory(cpu, cpu->reg_x, AM_6502_ZEROPAGE_Y, phase);
			break;
		case OP_6502_STX_ABS :
			op_ready = store_to_memory(cpu, cpu->reg_x, AM_6502_ABSOLUTE, phase);
			break;
	}

	if (op_ready) {
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_sty(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	
	bool op_ready = false;

	switch (cpu->reg_ir) {
		case OP_6502_STY_ZP :
			op_ready = store_to_memory(cpu, cpu->reg_y, AM_6502_ZEROPAGE, phase);
			break;
		case OP_6502_STY_ZPX :
			op_ready = store_to_memory(cpu, cpu->reg_y, AM_6502_ZEROPAGE_X, phase);
			break;
		case OP_6502_STY_ABS :
			op_ready = store_to_memory(cpu, cpu->reg_y, AM_6502_ABSOLUTE, phase);
			break;
	}

	if (op_ready) {
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_tax(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->internal_ab = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_x = cpu->reg_a;
		cpu->p_zero_result = cpu->reg_x == 0;
		cpu->p_negative_result = (cpu->reg_x & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_tay(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->internal_ab = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_y = cpu->reg_a;
		cpu->p_zero_result = cpu->reg_y == 0;
		cpu->p_negative_result = (cpu->reg_y & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_tsx(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->internal_ab = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_x = cpu->reg_sp;
		cpu->p_zero_result = cpu->reg_x == 0;
		cpu->p_negative_result = (cpu->reg_x & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_txa(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->internal_ab = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_a = cpu->reg_x;
		cpu->p_zero_result = cpu->reg_a == 0;
		cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_txs(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->internal_ab = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_sp = cpu->reg_x;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_tya(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->internal_ab = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_a = cpu->reg_y;
		cpu->p_zero_result = cpu->reg_a == 0;
		cpu->p_negative_result = (cpu->reg_a & 0b10000000) >> 7;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_instruction(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	/* some instruction are grouped by addressing mode and can easily be tested at once */
	switch (cpu->reg_ir & AC_6502_MASK) {
		case AC_6502_ADC :
			decode_adc(cpu, phase);
			return;
		case AC_6502_AND :
			decode_and(cpu, phase);
			return;
		case AC_6502_ASL :
			decode_asl(cpu, phase);
			return;
		case AC_6502_CMP :
			decode_cmp(cpu, phase);
			return;
		case AC_6502_DEC :
			if (cpu->reg_ir != OP_6502_DEX) {
				decode_dec(cpu, phase);
			}
			break;
		case AC_6502_EOR :
			decode_eor(cpu, phase);
			return;
		case AC_6502_INC :
			if (cpu->reg_ir != OP_6502_NOP) {
				decode_inc(cpu, phase);
			}
			break;
		case AC_6502_LDA :
			decode_lda(cpu, phase);
			return;
		case AC_6502_LDX :
			if (cpu->reg_ir != OP_6502_TAX && cpu->reg_ir != OP_6502_TSX) {
				decode_ldx(cpu, phase);
				return;
			}
			break;
		case AC_6502_LDY :
			if (cpu->reg_ir != OP_6502_BCS && cpu->reg_ir != OP_6502_CLV && cpu->reg_ir != OP_6502_TAY) {
				decode_ldy(cpu, phase);
				return;
			};
			break;
		case AC_6502_LSR :
			decode_lsr(cpu, phase);
			return;
		case AC_6502_ORA :
			decode_ora(cpu, phase);
			return;
		case AC_6502_ROL :
			decode_rol(cpu, phase);
			return;
		case AC_6502_ROR :
			decode_ror(cpu, phase);
			return;
		case AC_6502_SBC :
			decode_sbc(cpu, phase);
			return;
		case AC_6502_STA :
			decode_sta(cpu, phase);
			return;
	};

	switch (cpu->reg_ir) {
		case OP_6502_BCC :
			decode_bcc(cpu, phase);
			break;
		case OP_6502_BCS :
			decode_bcs(cpu, phase);
			break;
		case OP_6502_BIT_ZP :
		case OP_6502_BIT_ABS:
			decode_bit(cpu, phase);
			break;
		case OP_6502_BEQ :
			decode_beq(cpu, phase);
			break;
		case OP_6502_BMI :
			decode_bmi(cpu, phase);
			break;
		case OP_6502_BNE :
			decode_bne(cpu, phase);
			break;
		case OP_6502_BPL :
			decode_bpl(cpu, phase);
			break;
		case OP_6502_BRK :
			decode_brk(cpu, phase);
			break;
		case OP_6502_BVC :
			decode_bvc(cpu, phase);
			break;
		case OP_6502_BVS :
			decode_bvs(cpu, phase);
			break;
		case OP_6502_CLC :
			decode_clc(cpu, phase);
			break;
		case OP_6502_CLD :
			decode_cld(cpu, phase);
			break;
		case OP_6502_CLI :
			decode_cli(cpu, phase);
			break;
		case OP_6502_CLV :
			decode_clv(cpu, phase);
			break;
		case OP_6502_CPX_IMM:
		case OP_6502_CPX_ZP:
		case OP_6502_CPX_ABS:
			decode_cpx_cpy(cpu, phase, cpu->reg_x);
			break;
		case OP_6502_CPY_IMM:
		case OP_6502_CPY_ZP:
		case OP_6502_CPY_ABS:
			decode_cpx_cpy(cpu, phase, cpu->reg_y);
			break;
		case OP_6502_DEX:
			decode_dex(cpu, phase);
			break;
		case OP_6502_DEY:
			decode_dey(cpu, phase);
			break;
		case OP_6502_INX:
			decode_inx(cpu, phase);
			break;
		case OP_6502_INY:
			decode_iny(cpu, phase);
			break;
		case OP_6502_JMP_ABS:
		case OP_6502_JMP_IND:
			decode_jmp(cpu, phase);
			break;
		case OP_6502_JSR:
			decode_jsr(cpu, phase);
			break;
		case OP_6502_NOP:
			decode_nop(cpu, phase);
			break;
		case OP_6502_PHA:
			decode_pha(cpu, phase);
			break;
		case OP_6502_PHP:
			decode_php(cpu, phase);
			break;
		case OP_6502_PLA:
			decode_pla(cpu, phase);
			break;
		case OP_6502_PLP:
			decode_plp(cpu, phase);
			break;
		case OP_6502_RTI:
			decode_rti(cpu, phase);
			break;
		case OP_6502_RTS:
			decode_rts(cpu, phase);
			break;
		case OP_6502_SEC :
			decode_sec(cpu, phase);
			break;
		case OP_6502_SED :
			decode_sed(cpu, phase);
			break;
		case OP_6502_SEI :
			decode_sei(cpu, phase);
			break;
		case OP_6502_STX_ZP : 
		case OP_6502_STX_ZPY : 
		case OP_6502_STX_ABS : 
			decode_stx(cpu, phase);
			break;
		case OP_6502_STY_ZP : 
		case OP_6502_STY_ZPX : 
		case OP_6502_STY_ABS : 
			decode_sty(cpu, phase);
			break;
		case OP_6502_TAX :
			decode_tax(cpu, phase);
			break;
		case OP_6502_TAY :
			decode_tay(cpu, phase);
			break;
		case OP_6502_TSX :
			decode_tsx(cpu, phase);
			break;
		case OP_6502_TXA :
			decode_txa(cpu, phase);
			break;
		case OP_6502_TXS:
			decode_txs(cpu, phase);
			break;
		case OP_6502_TYA :
			decode_tya(cpu, phase);
			break;
	};
}

static inline void cpu_6502_execute_phase(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	// initialization is treated seperately
	if (PRIVATE(cpu)->state == CS_INIT) {
		execute_init(cpu, phase);
		return;
	}

	// check for interrupts between instructions
	if (PRIVATE(cpu)->decode_cycle == 0 && phase == CYCLE_BEGIN) {
		if (PRIVATE(cpu)->nmi_triggered) {
			PRIVATE(cpu)->state = CS_IN_NMI;
			PRIVATE(cpu)->nmi_triggered = false;
		}
		if (*cpu->pin_irq_b == false && !cpu->p_interrupt_disable) {
			PRIVATE(cpu)->state = CS_IN_IRQ;
		}
		if (PRIVATE(cpu)->override_pc) {
			cpu->reg_pc = PRIVATE(cpu)->override_pc;
			PRIVATE(cpu)->override_pc = 0;
		}
	}

	// irq starting sequence is handled seperately
	if (PRIVATE(cpu)->state == CS_IN_IRQ) {
		interrupt_sequence(cpu, phase, INTR_IRQ);
		return;
	}

	if (PRIVATE(cpu)->state == CS_IN_NMI) {
		interrupt_sequence(cpu, phase, INTR_NMI);
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

Cpu6502 *cpu_6502_create(
		uint16_t *addres_bus,
		uint8_t *data_bus,
		const bool *clock,
		const bool *reset,
		bool *rw,
		const bool *irq,
		const bool *nmi,
		bool *sync,
		const bool *rdy) {
	assert(addres_bus);
	assert(data_bus);
	assert(clock);
	assert(reset);
	assert(rw);
	assert(irq);
	assert(nmi);
	assert(sync);
	assert(rdy);

	Cpu6502_private *cpu = (Cpu6502_private *) malloc(sizeof(Cpu6502_private));
	memset(cpu, 0, sizeof(Cpu6502_private));

	cpu->intf.bus_address = addres_bus;
	cpu->intf.bus_data = data_bus;
	cpu->intf.pin_clock = clock;
	cpu->intf.pin_reset_b = reset;
	cpu->intf.pin_rw = rw;
	cpu->intf.pin_irq_b = irq;
	cpu->intf.pin_nmi_b = nmi;
	cpu->intf.pin_sync = sync;
	cpu->intf.pin_rdy = rdy;
	cpu->decode_cycle = -1;
	cpu->state = CS_INIT;
	cpu->nmi_triggered = false;
	cpu->prev_nmi = true;
	cpu->prev_reset = ACTLO_DEASSERT;
	cpu->override_pc = 0;
	
	return &cpu->intf;
}

void cpu_6502_destroy(Cpu6502 *cpu) {
	assert(cpu);
	free((Cpu6502_private *) cpu);
}

void cpu_6502_process(Cpu6502 *cpu) {
	assert(cpu);
	Cpu6502_private *priv = (Cpu6502_private *) cpu;

	// check for changes in the reset line
	if (!ACTLO_ASSERTED(priv->prev_reset) && ACTLO_ASSERTED(*cpu->pin_reset_b)) {
		// reset was just asserted
		priv->internal_ab = 0;

		priv->internal_rw = RW_READ;
		priv->prev_reset = *cpu->pin_reset_b;
	} else if (ACTLO_ASSERTED(priv->prev_reset) && !ACTLO_ASSERTED(*cpu->pin_reset_b)) {
		// reset was just deasserted - start initialization sequence
		priv->prev_reset = *cpu->pin_reset_b;
		priv->state = CS_INIT;
		priv->decode_cycle = -1;
	}

	// do nothing:
	//  - if reset is asserted or
	//  - if rdy is not asserted or
	//  - if not on the edge of a clock cycle
	if (ACTLO_ASSERTED(*cpu->pin_reset_b) || !ACTHI_ASSERTED(*cpu->pin_rdy) || *cpu->pin_clock == priv->prev_clock) {
		process_end(cpu);
		return;
	}

	// always check for an nmi request
	if (*cpu->pin_nmi_b != priv->prev_nmi) {
		priv->nmi_triggered = priv->nmi_triggered || !*cpu->pin_nmi_b;
		priv->prev_nmi = *cpu->pin_nmi_b;
	}

	// normal processing
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

void cpu_6502_override_next_instruction_address(Cpu6502 *cpu, uint16_t pc) {
	assert(cpu);
	PRIVATE(cpu)->override_pc = pc;
}
