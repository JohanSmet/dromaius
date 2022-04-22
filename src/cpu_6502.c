// cpu_6502.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Emulation of the MOS 6502

#include "cpu_6502.h"
#include "cpu_6502_opcodes.h"
#include "simulator.h"
#include "crt.h"

#define SIGNAL_OWNER		cpu
#define SIGNAL_PREFIX		PIN_6502_

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
- To allow synchronization between devices processing of (1) and (2) is not done in
  the same cpu_6502_process call. The function is called twice for each clock change,
  the first time with parameter "delayed" set to false (execute (1)) and the second
  time with "delayed" set to true (execute (2)).
*/

//////////////////////////////////////////////////////////////////////////////
//
// internal data types
//

static uint8_t Cpu6502_PinTypes[CHIP_6502_PIN_COUNT] = {
	[PIN_6502_RDY  ] = CHIP_PIN_INPUT,
	[PIN_6502_IRQ_B] = CHIP_PIN_INPUT,
	[PIN_6502_NMI_B] = CHIP_PIN_INPUT,
	[PIN_6502_SYNC ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB0  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB1  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB2  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB3  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB4  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB5  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB6  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB7  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB8  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB9  ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB10 ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB11 ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB12 ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB13 ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB14 ] = CHIP_PIN_OUTPUT,
	[PIN_6502_AB15 ] = CHIP_PIN_OUTPUT,
	[PIN_6502_DB7  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_DB6  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_DB5  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_DB4  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_DB3  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_DB2  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_DB1  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_DB0  ] = CHIP_PIN_INPUT | CHIP_PIN_OUTPUT,
	[PIN_6502_RW   ] = CHIP_PIN_OUTPUT,
	[PIN_6502_CLK  ] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
	[PIN_6502_SO   ] = CHIP_PIN_INPUT,
	[PIN_6502_PHI2O] = CHIP_PIN_OUTPUT,
	[PIN_6502_PHI1O] = CHIP_PIN_OUTPUT,
	[PIN_6502_RES_B] = CHIP_PIN_INPUT | CHIP_PIN_TRIGGER,
};

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

typedef struct output_t {
	bool			drv_data;			// should cpu be driving the databus?
	uint8_t			data;				// data to be written to the databus

	uint16_t		address;			// data to be written to the addressbus
	bool			rw;					// rw pin
	bool			sync;				// sync pin
} output_t;

typedef struct Cpu6502_private {
	Cpu6502			intf;

	uint8_t			in_data;				// incoming data (only valid when signal rw is high)
	output_t		output;
	output_t		last_output;

	CPU_6502_STATE	state;
	int8_t			decode_cycle;			// instruction decode cycle
	uint16_t		override_pc;			// != 0: fetch next instruction from this location, overruling the current pc
	uint8_t			operand;
	addr_t			addr;
	addr_t			i_addr;
	bool			page_crossed;
	bool			nmi_triggered;

	bool			delayed_cycle;

	uint16_t		last_out_address;
} Cpu6502_private;

//////////////////////////////////////////////////////////////////////////////
//
// internal functions
//

#define PRIVATE(cpu)	((Cpu6502_private *) (cpu))

#define OUTPUT_DATA(d)						\
	PRIVATE(cpu)->output.data = (uint8_t) (d);	\
	PRIVATE(cpu)->output.drv_data = true;

#define CPU_CHANGE_FLAG(flag, cond)			\
	FLAG_SET_CLEAR_U8(cpu->reg_p, FLAG_6502_##flag, (cond))

static void process_end(Cpu6502 *cpu) {

	output_t *output = &PRIVATE(cpu)->output;
	output_t *last_output = &PRIVATE(cpu)->last_output;

	// address bus
	if (output->address != last_output->address) {
		SIGNAL_GROUP_WRITE(address, output->address);
		last_output->address = output->address;
	}

	// data bus (tri-state)
	if (output->drv_data) {
		// cpu should be active - write if data changed or if cpu just became active
		if (output->data != last_output->drv_data || !last_output->drv_data) {
			SIGNAL_GROUP_WRITE(data, output->data);
			last_output->data = output->data;
			last_output->drv_data = output->drv_data;
		}
	} else if (last_output->drv_data) {
		// cpu was active last time but not anymore
		SIGNAL_GROUP_NO_WRITE(data);
		last_output->drv_data = false;
	}

	// rw pin
	if (output->rw != last_output->rw) {
		SIGNAL_WRITE(RW, output->rw);
		last_output->rw = output->rw;
	}

	// sync pin
	output->sync = PRIVATE(cpu)->decode_cycle == 0;
	if (output->sync != last_output->sync) {
		SIGNAL_WRITE(SYNC, output->sync);
		last_output->sync = output->sync;
	}
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
				PRIVATE(cpu)->output.address = cpu->reg_pc;
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
					PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
					PRIVATE(cpu)->output.rw = RW_WRITE | FORCE_READ[irq_type];
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(HI_BYTE(cpu->reg_pc));
					break;
				case CYCLE_END:
					cpu->reg_sp--;
					PRIVATE(cpu)->output.rw = RW_READ;
					break;
			}
			break;
		case 3 :		// push low byte of PC
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
					PRIVATE(cpu)->output.rw = RW_WRITE | FORCE_READ[irq_type];
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(LO_BYTE(cpu->reg_pc));
					break;
				case CYCLE_END:
					cpu->reg_sp--;
					PRIVATE(cpu)->output.rw = RW_READ;
					break;
			}
			break;
		case 4 :		// push processor state
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
					PRIVATE(cpu)->output.rw = RW_WRITE | FORCE_READ[irq_type];
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(cpu->reg_p);
					break;
				case CYCLE_END:
					cpu->reg_sp--;
					PRIVATE(cpu)->output.rw = RW_READ;
					break;
			}
			break;
		case 5 :		// read low byte of the reset vector
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->output.address = VECTOR_LOW[irq_type];
			} else if (phase == CYCLE_END) {
				cpu->reg_pc = SET_LO_BYTE(cpu->reg_pc, PRIVATE(cpu)->in_data);
			}
			break;
		case 6 :		// read high byte of the reset vector
			if (phase == CYCLE_BEGIN) {
				PRIVATE(cpu)->output.address = VECTOR_HIGH[irq_type];
			} else if (phase == CYCLE_END) {
				cpu->reg_pc = SET_HI_BYTE(cpu->reg_pc, PRIVATE(cpu)->in_data);
				CPU_CHANGE_FLAG(I, true);
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
			PRIVATE(cpu)->output.address = addr;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			*dst = PRIVATE(cpu)->in_data;
			break;
	}
}

static inline void store_memory(Cpu6502 *cpu, uint16_t addr, uint8_t value, CPU_6502_CYCLE phase) {
	assert(cpu);

	switch (phase) {
		case CYCLE_BEGIN :
			PRIVATE(cpu)->output.address = addr;
			PRIVATE(cpu)->output.rw = RW_WRITE;
			break;
		case CYCLE_MIDDLE:
			OUTPUT_DATA(value);
			break;
		case CYCLE_END :
			PRIVATE(cpu)->output.rw = RW_READ;
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
			PRIVATE(cpu)->output.address = PRIVATE(cpu)->addr.full;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			PRIVATE(cpu)->addr.lo_byte = (uint8_t) (PRIVATE(cpu)->addr.lo_byte + index);
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
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			PRIVATE(cpu)->addr.full = (uint16_t) (PRIVATE(cpu)->addr.full + index);
			PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->addr.hi_byte > 0;
			break;
		case CYCLE_END:
			PRIVATE(cpu)->addr.hi_byte = PRIVATE(cpu)->in_data;
			++cpu->reg_pc;
			break;
	};
}

static inline bool fetch_memory_page_crossed(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	assert(cpu);

	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = PRIVATE(cpu)->addr.full;
			return false;
		case CYCLE_MIDDLE:
			PRIVATE(cpu)->addr.hi_byte = (uint8_t) (PRIVATE(cpu)->addr.hi_byte + PRIVATE(cpu)->page_crossed);
			return false;
		case CYCLE_END:
			PRIVATE(cpu)->operand = PRIVATE(cpu)->in_data;
			return !PRIVATE(cpu)->page_crossed;
	};

	return false;
}

static inline void fetch_pc_memory(Cpu6502 *cpu, uint8_t *dst, CPU_6502_CYCLE phase) {
	assert(cpu);
	assert(dst);

	switch (phase) {
		case CYCLE_BEGIN :
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			*dst = PRIVATE(cpu)->in_data;
			++cpu->reg_pc;
			break;
	}
}

static inline int fetch_address_immediate(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
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

static inline int fetch_address_zeropage(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->addr.lo_byte, phase);
			PRIVATE(cpu)->addr.hi_byte = 0;
			break;
	};

	return 2;
}

static inline int fetch_address_zeropage_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {

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

static inline int fetch_address_absolute(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

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

static inline int fetch_address_absolute_indexed_shortcut(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {
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

static inline int fetch_address_absolute_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t index) {
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


static inline int fetch_address_indirect(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch indirect address - low byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->i_addr.lo_byte, phase);
			break;
		case 2 :		// fetch indirect address - high byte
			fetch_pc_memory(cpu, &PRIVATE(cpu)->i_addr.hi_byte, phase);
			break;
		case 3 :		// fetch jump address - low byte
			fetch_memory(cpu, PRIVATE(cpu)->i_addr.full, &PRIVATE(cpu)->addr.lo_byte, phase);
			INC_UINT16(PRIVATE(cpu)->i_addr.full, (phase == CYCLE_END));
			break;
		case 4 :		// fetch jump address - high byte
			fetch_memory(cpu, PRIVATE(cpu)->i_addr.full, &PRIVATE(cpu)->addr.hi_byte, phase);
			break;
	}

	return 5;
}

static inline int fetch_address_indexed_indirect(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch discard memory & add zp + index-x
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				INC_UINT8(PRIVATE(cpu)->operand, cpu->reg_x);
			}
			break;
		case 3:			// fetch address - low byte & increment zp + index-x
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand++;
			}
			break;
		case 4:			// fetch address - high byte
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.hi_byte, phase);
			break;
	}

	return 5;
}

static inline int fetch_address_indirect_indexed_shortcut(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch address - low byte & increment zp
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand++;
			}
			break;
		case 3 :		// fetch address - high byte & add addr.lo_byte + y
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.hi_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->addr.lo_byte + cpu->reg_y > 255;
				INC_UINT8(PRIVATE(cpu)->addr.lo_byte, cpu->reg_y);
			}
			break;
		case 4 :
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			if (phase == CYCLE_END) {
				INC_UINT8(PRIVATE(cpu)->addr.hi_byte, PRIVATE(cpu)->page_crossed);
			}
			break;
	}

	return 4 + PRIVATE(cpu)->page_crossed;
}

static inline int fetch_address_indirect_indexed(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	switch (PRIVATE(cpu)->decode_cycle) {
		case 1 :		// fetch zero page address
			fetch_pc_memory(cpu, &PRIVATE(cpu)->operand, phase);
			break;
		case 2 :		// fetch address - low byte & increment zp
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.lo_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->operand++;
			}
			break;
		case 3 :		// fetch address - high byte & add addr.lo_byte + y
			fetch_memory(cpu, PRIVATE(cpu)->operand, &PRIVATE(cpu)->addr.hi_byte, phase);
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->page_crossed = PRIVATE(cpu)->addr.lo_byte + cpu->reg_y > 255;
				INC_UINT8(PRIVATE(cpu)->addr.lo_byte, cpu->reg_y);
			}
			break;
		case 4 :
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			if (phase == CYCLE_END) {
				INC_UINT8(PRIVATE(cpu)->addr.hi_byte, PRIVATE(cpu)->page_crossed);
			}
			break;
	}

	return 5;
}

static inline int fetch_address_shortcut(Cpu6502 *cpu, ADDRESSING_MODE_6502 mode, CPU_6502_CYCLE phase) {

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

static inline int fetch_address(Cpu6502 *cpu, ADDRESSING_MODE_6502 mode, CPU_6502_CYCLE phase) {

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

	int memop_cycle = fetch_address_shortcut(cpu, mode, phase);

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

	int memop_cycle = fetch_address(cpu, mode, phase);

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
			PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
			PRIVATE(cpu)->output.rw = RW_WRITE;
			break;
		case CYCLE_MIDDLE:
			OUTPUT_DATA(value);
			break;
		case CYCLE_END:
			cpu->reg_sp--;
			PRIVATE(cpu)->output.rw = RW_READ;
			break;
	}
}

static inline uint8_t stack_pop(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	uint8_t result = 0;

	switch (phase) {
		case CYCLE_BEGIN:
			cpu->reg_sp++;
			PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			result = PRIVATE(cpu)->in_data;
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
			if (phase == CYCLE_END && BIT_IS_SET(cpu->reg_p, bit) != value) {
				// check flags + stop if branch not taken
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
		case 2:		// add relative address + check for page crossing
			switch (phase) {
				case CYCLE_BEGIN:
					PRIVATE(cpu)->output.address = cpu->reg_pc;
					PRIVATE(cpu)->i_addr.hi_byte = HI_BYTE(cpu->reg_pc);
					INC_UINT16(cpu->reg_pc, (int8_t) PRIVATE(cpu)->i_addr.lo_byte);
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
					PRIVATE(cpu)->output.address = PRIVATE(cpu)->i_addr.full;
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
	int carry = FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY);
	int bin_result = cpu->reg_a + PRIVATE(cpu)->operand + carry;

	int al = (cpu->reg_a & 0x0f) + (PRIVATE(cpu)->operand & 0x0f) + carry;
	if (al >= 0x0a) {
		al = ((al + 0x06) & 0x0f) + 0x10;
	}
	int a_seq1 = (cpu->reg_a & 0xf0) + (PRIVATE(cpu)->operand & 0xf0) + al;
	if (a_seq1 >= 0xa0) {
		a_seq1 += 0x60;
	}

	int a_seq2 = (int8_t) (cpu->reg_a & 0xf0) + (int8_t) (PRIVATE(cpu)->operand & 0xf0) + al;

	cpu->reg_a = (uint8_t) (a_seq1 & 0x00ff);
	CPU_CHANGE_FLAG(C, a_seq1 >= 0x0100);
	CPU_CHANGE_FLAG(V, (a_seq2 < -128) || (a_seq2 > 127));
	CPU_CHANGE_FLAG(Z, (bin_result & 0xff) == 0x00);
	CPU_CHANGE_FLAG(N, BIT_IS_SET(a_seq2, 7));
}

static inline void decode_adc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (!fetch_operand_g1(cpu, phase)) {
		return;
	}

	if (!FLAG_IS_SET(cpu->reg_p, FLAG_6502_DECIMAL_MODE)) {
		int carry = FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY);
		int s_result = (int8_t) cpu->reg_a + (int8_t) PRIVATE(cpu)->operand + carry;
		int u_result = cpu->reg_a + PRIVATE(cpu)->operand + carry;
		cpu->reg_a = (uint8_t) (u_result & 0x00ff);
		CPU_CHANGE_FLAG(C, BIT_IS_SET(u_result, 8));
		CPU_CHANGE_FLAG(V, (s_result < -128) || (s_result > 127));
		CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
	} else {
		calculate_adc_decimal(cpu);
	}

	PRIVATE(cpu)->decode_cycle = -1;
}

static inline void decode_and(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (fetch_operand_g1(cpu, phase)) {
		cpu->reg_a	= cpu->reg_a & PRIVATE(cpu)->operand;
		CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_asl(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (cpu->reg_ir == OP_6502_ASL_ACC) {
		switch (phase) {
			case CYCLE_BEGIN:
				PRIVATE(cpu)->output.address = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END:
				CPU_CHANGE_FLAG(C, BIT_IS_SET(cpu->reg_a, 7));
				cpu->reg_a = (uint8_t) (cpu->reg_a << 1);
				CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
				CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
				PRIVATE(cpu)->decode_cycle = -1;
				break;
		}
		return;
	}

	static const uint8_t AM_LUT[] = {
		AM_6502_UNDEFINED,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_ZEROPAGE_X,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	int memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->output.rw = RW_WRITE;
					break;
				case CYCLE_END:
					CPU_CHANGE_FLAG(C, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
					PRIVATE(cpu)->operand = (uint8_t) (PRIVATE(cpu)->operand << 1);
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(PRIVATE(cpu)->operand);
					break;
				case CYCLE_END:
					PRIVATE(cpu)->output.rw = RW_READ;
					CPU_CHANGE_FLAG(Z, PRIVATE(cpu)->operand == 0);
					CPU_CHANGE_FLAG(N, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
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

	static const uint8_t AM_LUT[] = {
		AM_6502_UNDEFINED,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_UNDEFINED,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_UNDEFINED		// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		CPU_CHANGE_FLAG(N, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
		CPU_CHANGE_FLAG(V, BIT_IS_SET(PRIVATE(cpu)->operand, 6));
		CPU_CHANGE_FLAG(Z, (PRIVATE(cpu)->operand & cpu->reg_a) == 0);
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
		CPU_CHANGE_FLAG(B, true);
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
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			CPU_CHANGE_FLAG(CARRY, false);
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_cld(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			CPU_CHANGE_FLAG(DECIMAL_MODE, false);
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_cli(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			CPU_CHANGE_FLAG(INTERRUPT_DISABLE, false);
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_clv(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			CPU_CHANGE_FLAG(OVERFLOW, false);
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_cmp(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (fetch_operand_g1(cpu, phase)) {
		int8_t result = (int8_t) (cpu->reg_a - PRIVATE(cpu)->operand);
		CPU_CHANGE_FLAG(Z, result == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(result, 7));
		CPU_CHANGE_FLAG(C, cpu->reg_a >= PRIVATE(cpu)->operand);
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_cpx_cpy(Cpu6502 *cpu, CPU_6502_CYCLE phase, uint8_t reg) {

	static const uint8_t AM_LUT[] = {
		AM_6502_IMMEDIATE,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_UNDEFINED,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_UNDEFINED		// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		int8_t result = (int8_t) (reg - PRIVATE(cpu)->operand);
		CPU_CHANGE_FLAG(Z, result == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(result, 7));
		CPU_CHANGE_FLAG(C, reg >= PRIVATE(cpu)->operand);
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_dec(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const uint8_t AM_LUT[] = {
		AM_6502_UNDEFINED,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_ZEROPAGE_X,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	int memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform decrement / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->output.rw = RW_WRITE;
					break;
				case CYCLE_END:
					PRIVATE(cpu)->operand--;
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(PRIVATE(cpu)->operand);
					break;
				case CYCLE_END:
					PRIVATE(cpu)->output.rw = RW_READ;
					CPU_CHANGE_FLAG(Z, PRIVATE(cpu)->operand == 0);
					CPU_CHANGE_FLAG(N, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_dex(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			cpu->reg_x--;
			CPU_CHANGE_FLAG(Z, cpu->reg_x == 0);
			CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_x, 7));
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_dey(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			cpu->reg_y--;
			CPU_CHANGE_FLAG(Z, cpu->reg_y == 0);
			CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_y, 7));
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_eor(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (fetch_operand_g1(cpu, phase)) {
		cpu->reg_a	= cpu->reg_a ^ PRIVATE(cpu)->operand;
		CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_inc(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const uint8_t AM_LUT[] = {
		AM_6502_UNDEFINED,						// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,						// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,						// 4
		AM_6502_ZEROPAGE_X,		// 5
		AM_6502_UNDEFINED,						// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	int memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform increment / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->output.rw = RW_WRITE;
					break;
				case CYCLE_END:
					PRIVATE(cpu)->operand++;
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(PRIVATE(cpu)->operand);
					break;
				case CYCLE_END:
					PRIVATE(cpu)->output.rw = RW_READ;
					CPU_CHANGE_FLAG(Z, PRIVATE(cpu)->operand == 0);
					CPU_CHANGE_FLAG(N, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_inx(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			cpu->reg_x++;
			CPU_CHANGE_FLAG(Z, cpu->reg_x == 0);
			CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_x, 7));
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_iny(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END :
			cpu->reg_y++;
			CPU_CHANGE_FLAG(Z, cpu->reg_y == 0);
			CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_y, 7));
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_jmp(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	int memop_cycle = 0;

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
				PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
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
		CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_ldx(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const uint8_t AM_LUT[] = {
		AM_6502_IMMEDIATE,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_ZEROPAGE_Y,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_ABSOLUTE_Y		// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		cpu->reg_x = PRIVATE(cpu)->operand;
		CPU_CHANGE_FLAG(Z, cpu->reg_x == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_x, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_ldy(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	static const uint8_t AM_LUT[] = {
		AM_6502_IMMEDIATE,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_ZEROPAGE_X,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	if (fetch_operand(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase)) {
		cpu->reg_y = PRIVATE(cpu)->operand;
		CPU_CHANGE_FLAG(Z, cpu->reg_y == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_y, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_lsr(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (cpu->reg_ir == OP_6502_LSR_ACC) {
		switch (phase) {
			case CYCLE_BEGIN:
				PRIVATE(cpu)->output.address = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END:
				CPU_CHANGE_FLAG(C, BIT_IS_SET(cpu->reg_a, 0));
				cpu->reg_a >>= 1;
				CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
				CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
				PRIVATE(cpu)->decode_cycle = -1;
				break;
		}
		return;
	}

	static const uint8_t AM_LUT[] = {
		AM_6502_UNDEFINED,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_ZEROPAGE_X,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	int memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->output.rw = RW_WRITE;
					break;
				case CYCLE_END:
					CPU_CHANGE_FLAG(C, BIT_IS_SET(PRIVATE(cpu)->operand, 0));
					PRIVATE(cpu)->operand >>= 1;
					break;
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(PRIVATE(cpu)->operand);
					break;
				case CYCLE_END:
					PRIVATE(cpu)->output.rw = RW_READ;
					CPU_CHANGE_FLAG(Z, PRIVATE(cpu)->operand == 0);
					CPU_CHANGE_FLAG(N, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
					PRIVATE(cpu)->decode_cycle = -1;
					break;
			}
			break;
	}
}

static inline void decode_nop(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
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
			// (I didn't see that mentioned in the programming manual, or I missed it, but several online resources mention this.)
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
				PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// pop value from stack
			cpu->reg_a = stack_pop(cpu, phase);
			if (phase == CYCLE_END) {
				CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
				CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
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
				PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// pop value from stack
			cpu->reg_p = stack_pop(cpu, phase) | FLAG_6502_EXPANSION;	// reserved bit is always set
			FLAG_CLEAR_U8(cpu->reg_p, FLAG_6502_BREAK_COMMAND);			// ignore the 'B'-flag (it gets set by the PHP-instruction)
			if (phase == CYCLE_END) {
				PRIVATE(cpu)->decode_cycle = -1;
			}
			break;
	}
}


static inline void decode_ora(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (fetch_operand_g1(cpu, phase)) {
		cpu->reg_a	= cpu->reg_a | PRIVATE(cpu)->operand;
		CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_rol(Cpu6502 *cpu, CPU_6502_CYCLE phase) {

	if (cpu->reg_ir == OP_6502_ROL_ACC) {
		switch (phase) {
			case CYCLE_BEGIN:
				PRIVATE(cpu)->output.address = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END: {
				int carry = FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY);
				CPU_CHANGE_FLAG(C, BIT_IS_SET(cpu->reg_a, 7));
				cpu->reg_a = (uint8_t) ((cpu->reg_a << 1) | carry);
				CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
				CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
				PRIVATE(cpu)->decode_cycle = -1;
				break;
			}
		}
		return;
	}

	static const uint8_t AM_LUT[] = {
		AM_6502_UNDEFINED,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_ZEROPAGE_X,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	int memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->output.rw = RW_WRITE;
					break;
				case CYCLE_END: {
					int carry = FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY);
					CPU_CHANGE_FLAG(C, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
					PRIVATE(cpu)->operand = (uint8_t) ((PRIVATE(cpu)->operand << 1) | carry);
					break;
				}
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(PRIVATE(cpu)->operand);
					break;
				case CYCLE_END:
					PRIVATE(cpu)->output.rw = RW_READ;
					CPU_CHANGE_FLAG(Z, PRIVATE(cpu)->operand == 0);
					CPU_CHANGE_FLAG(N, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
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
				PRIVATE(cpu)->output.address = cpu->reg_pc;
				break;
			case CYCLE_MIDDLE:
				break;
			case CYCLE_END: {
				int carry = FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY);
				CPU_CHANGE_FLAG(C, BIT_IS_SET(cpu->reg_a, 0));
				cpu->reg_a = (cpu->reg_a >> 1) | (uint8_t) (carry << 7);
				CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
				CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
				PRIVATE(cpu)->decode_cycle = -1;
				break;
			}
		}
		return;
	}

	static const uint8_t AM_LUT[] = {
		AM_6502_UNDEFINED,		// 0
		AM_6502_ZEROPAGE,		// 1
		AM_6502_UNDEFINED,		// 2
		AM_6502_ABSOLUTE,		// 3
		AM_6502_UNDEFINED,		// 4
		AM_6502_ZEROPAGE_X,		// 5
		AM_6502_UNDEFINED,		// 6
		AM_6502_ABSOLUTE_X		// 7
	};

	int memop_cycle = fetch_address(cpu, AM_LUT[EXTRACT_6502_ADRESSING_MODE(cpu->reg_ir)], phase);

	switch (PRIVATE(cpu)->decode_cycle - memop_cycle) {
		case 0:		// fetch operand
			fetch_memory(cpu, PRIVATE(cpu)->addr.full, &PRIVATE(cpu)->operand, phase);
			break;
		case 1:		// perform rotate / turn on write
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					PRIVATE(cpu)->output.rw = RW_WRITE;
					break;
				case CYCLE_END: {
					int carry = FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY);
					CPU_CHANGE_FLAG(C, BIT_IS_SET(PRIVATE(cpu)->operand, 0));
					PRIVATE(cpu)->operand = (PRIVATE(cpu)->operand >> 1) | (uint8_t) (carry << 7);
					break;
				}
			}
			break;
		case 2:		// store result + set flags
			switch (phase) {
				case CYCLE_BEGIN:
					break;
				case CYCLE_MIDDLE:
					OUTPUT_DATA(PRIVATE(cpu)->operand);
					break;
				case CYCLE_END:
					PRIVATE(cpu)->output.rw = RW_READ;
					CPU_CHANGE_FLAG(Z, PRIVATE(cpu)->operand == 0);
					CPU_CHANGE_FLAG(N, BIT_IS_SET(PRIVATE(cpu)->operand, 7));
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
				PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
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
				PRIVATE(cpu)->output.address = MAKE_WORD(0x01, cpu->reg_sp);
			}
			break;
		case 3 :		// pop status register
			cpu->reg_p = stack_pop(cpu, phase);
			if (phase == CYCLE_END) {
				CPU_CHANGE_FLAG(B, false);
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
	int carry = FLAG_IS_SET(cpu->reg_p, FLAG_6502_CARRY);
	int u_result = cpu->reg_a + (uint8_t) ~PRIVATE(cpu)->operand + carry;
	int s_result = (int8_t) cpu->reg_a - (int8_t) PRIVATE(cpu)->operand - !carry;

	if (!FLAG_IS_SET(cpu->reg_p, FLAG_6502_DECIMAL_MODE)) {
		cpu->reg_a = (uint8_t) (u_result & 0x00ff);
	} else {
		int al = (cpu->reg_a & 0x0f) - (PRIVATE(cpu)->operand & 0x0f) + carry - 1;
		if (al < 0) {
			al = ((al - 0x06) & 0x0f) - 0x10;
		}
		int a_seq3 = (cpu->reg_a & 0xf0) - (PRIVATE(cpu)->operand & 0xf0) + al;
		if (a_seq3 < 0) {
			a_seq3 -= 0x60;
		}
		cpu->reg_a = (uint8_t) (a_seq3 & 0x00ff);
	}

	CPU_CHANGE_FLAG(C, BIT_IS_SET(u_result, 8));
	CPU_CHANGE_FLAG(V, (s_result < -128) || (s_result > 127));
	CPU_CHANGE_FLAG(Z, (u_result & 0x00ff) == 0);
	CPU_CHANGE_FLAG(N, BIT_IS_SET(u_result, 7));

	PRIVATE(cpu)->decode_cycle = -1;
}

static inline void decode_sec(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			CPU_CHANGE_FLAG(C, true);
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_sed(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			CPU_CHANGE_FLAG(D, true);
			PRIVATE(cpu)->decode_cycle = -1;
			break;
	}
}

static inline void decode_sei(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	switch (phase) {
		case CYCLE_BEGIN:
			PRIVATE(cpu)->output.address = cpu->reg_pc;
			break;
		case CYCLE_MIDDLE:
			break;
		case CYCLE_END:
			CPU_CHANGE_FLAG(I, true);
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
		PRIVATE(cpu)->output.address = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_x = cpu->reg_a;
		CPU_CHANGE_FLAG(Z, cpu->reg_x == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_x, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_tay(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->output.address = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_y = cpu->reg_a;
		CPU_CHANGE_FLAG(Z, cpu->reg_y == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_y, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_tsx(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->output.address = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_x = cpu->reg_sp;
		CPU_CHANGE_FLAG(Z, cpu->reg_x == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_x, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_txa(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->output.address = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_a = cpu->reg_x;
		CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_txs(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->output.address = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_sp = cpu->reg_x;
		PRIVATE(cpu)->decode_cycle = -1;
	}
}

static inline void decode_tya(Cpu6502 *cpu, CPU_6502_CYCLE phase) {
	if (phase == CYCLE_BEGIN) {
		PRIVATE(cpu)->output.address = cpu->reg_pc;
	}
	if (phase == CYCLE_END) {
		cpu->reg_a = cpu->reg_y;
		CPU_CHANGE_FLAG(Z, cpu->reg_a == 0);
		CPU_CHANGE_FLAG(N, BIT_IS_SET(cpu->reg_a, 7));
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

	// data-bus
	PRIVATE(cpu)->output.drv_data = false;
	PRIVATE(cpu)->in_data = SIGNAL_GROUP_READ_U8(data);

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
		if (ACTLO_ASSERTED(SIGNAL_READ(IRQ_B)) && !FLAG_IS_SET(cpu->reg_p, FLAG_6502_INTERRUPT_DISABLE)) {
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

void cpu_6502_override_next_instruction_address(Cpu6502 *cpu, uint16_t pc) {
	assert(cpu);
	PRIVATE(cpu)->override_pc = pc;
}

bool cpu_6502_at_start_of_instruction(Cpu6502 *cpu) {
	assert(cpu);
	return SIGNAL_READ_NEXT(SYNC);
}

bool cpu_6502_irq_is_asserted(Cpu6502 *cpu) {
	assert(cpu);
	return ACTLO_ASSERTED(SIGNAL_READ_NEXT(IRQ_B));
}

int64_t cpu_6502_program_counter(Cpu6502 *cpu) {
	assert(cpu);
	return cpu->reg_pc;
}

//////////////////////////////////////////////////////////////////////////////
//
// interface functions
//

static void cpu_6502_destroy(Cpu6502 *cpu);
static void cpu_6502_process(Cpu6502 *cpu);

Cpu6502 *cpu_6502_create(Simulator *sim, Cpu6502Signals signals) {

	Cpu6502_private *priv = (Cpu6502_private *) dms_calloc(1, sizeof(Cpu6502_private));
	Cpu6502 *cpu = &priv->intf;

	CHIP_SET_FUNCTIONS(cpu, cpu_6502_process, cpu_6502_destroy);
	CHIP_SET_VARIABLES(cpu, sim, cpu->signals, Cpu6502_PinTypes, CHIP_6502_PIN_COUNT);

	cpu->signal_pool = sim->signal_pool;
	cpu->override_next_instruction_address = (CPU_OVERRIDE_NEXT_INSTRUCTION_ADDRESS) cpu_6502_override_next_instruction_address;
	cpu->is_at_start_of_instruction = (CPU_IS_AT_START_OF_INSTRUCTION) cpu_6502_at_start_of_instruction;
	cpu->irq_is_asserted = (CPU_IRQ_IS_ASSERTED) cpu_6502_irq_is_asserted;
	cpu->program_counter = (CPU_PROGRAM_COUNTER) cpu_6502_program_counter;

	dms_memcpy(cpu->signals, signals, sizeof(Cpu6502Signals));

	cpu->sg_address = signal_group_create();
	cpu->sg_data = signal_group_create();

	SIGNAL_DEFINE_GROUP(AB0, address);
	SIGNAL_DEFINE_GROUP(AB1, address);
	SIGNAL_DEFINE_GROUP(AB2, address);
	SIGNAL_DEFINE_GROUP(AB3, address);
	SIGNAL_DEFINE_GROUP(AB4, address);
	SIGNAL_DEFINE_GROUP(AB5, address);
	SIGNAL_DEFINE_GROUP(AB6, address);
	SIGNAL_DEFINE_GROUP(AB7, address);
	SIGNAL_DEFINE_GROUP(AB8, address);
	SIGNAL_DEFINE_GROUP(AB9, address);
	SIGNAL_DEFINE_GROUP(AB10, address);
	SIGNAL_DEFINE_GROUP(AB11, address);
	SIGNAL_DEFINE_GROUP(AB12, address);
	SIGNAL_DEFINE_GROUP(AB13, address);
	SIGNAL_DEFINE_GROUP(AB14, address);
	SIGNAL_DEFINE_GROUP(AB15, address);

	SIGNAL_DEFINE_GROUP(DB0, data);
	SIGNAL_DEFINE_GROUP(DB1, data);
	SIGNAL_DEFINE_GROUP(DB2, data);
	SIGNAL_DEFINE_GROUP(DB3, data);
	SIGNAL_DEFINE_GROUP(DB4, data);
	SIGNAL_DEFINE_GROUP(DB5, data);
	SIGNAL_DEFINE_GROUP(DB6, data);
	SIGNAL_DEFINE_GROUP(DB7, data);

	SIGNAL_DEFINE_DEFAULT(RDY, ACTHI_ASSERT);
	SIGNAL_DEFINE(PHI1O);
	SIGNAL_DEFINE_DEFAULT(IRQ_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE_DEFAULT(NMI_B, ACTLO_DEASSERT);
	SIGNAL_DEFINE(SYNC);
	SIGNAL_DEFINE_DEFAULT(RW, true);
	SIGNAL_DEFINE(CLK);
	SIGNAL_DEFINE(SO);
	SIGNAL_DEFINE(PHI2O);
	SIGNAL_DEFINE_DEFAULT(RES_B, ACTLO_DEASSERT);

	priv->decode_cycle = -1;
	priv->state = CS_INIT;
	priv->nmi_triggered = false;
	priv->override_pc = 0;

	return cpu;
}

static void cpu_6502_destroy(Cpu6502 *cpu) {
	assert(cpu);
	signal_group_destroy(cpu->sg_address);
	signal_group_destroy(cpu->sg_data);
	dms_free((Cpu6502_private *) cpu);
}

static void cpu_6502_process(Cpu6502 *cpu) {
	assert(cpu);
	Cpu6502_private *priv = (Cpu6502_private *) cpu;

	// check for changes in the reset line
	bool reset_b = SIGNAL_READ(RES_B);

	if (SIGNAL_CHANGED(RES_B)) {
		if (ACTLO_ASSERTED(reset_b)) {
			// reset was just asserted
			priv->output.address = 0;
			priv->output.rw = RW_READ;
		} else {
			// reset was just deasserted - start initialization sequence
			priv->state = CS_INIT;
			priv->decode_cycle = -1;
			priv->delayed_cycle = false;
		}
	}

	// do nothing:
	//  - if reset is asserted or
	//  - if rdy is not asserted
	if (ACTLO_ASSERTED(reset_b) || !ACTHI_ASSERTED(SIGNAL_READ(RDY))) {
		process_end(cpu);
		return;
	}

	// always check for an nmi request
	if (SIGNAL_CHANGED(NMI_B)) {
		priv->nmi_triggered = priv->nmi_triggered || !SIGNAL_READ(NMI_B);
	}

	// normal processing
	bool clock = SIGNAL_READ(CLK);
	bool clock_changed = SIGNAL_CHANGED(CLK);

	if (priv->delayed_cycle) {
		priv->delayed_cycle = false;
		++priv->decode_cycle;
		cpu_6502_execute_phase(cpu, CYCLE_BEGIN);
	} else if (clock && clock_changed) {
		// a positive going clock marks the halfway point of the cycle
		cpu_6502_execute_phase(cpu, CYCLE_MIDDLE);
	} else if (!clock && clock_changed) {
		// a negative going clock ends the previous cycle and starts a new cycle
		cpu_6502_execute_phase(cpu, CYCLE_END);

		// ask to be woken up next timestep to process the delayed part of the negative edge
		priv->delayed_cycle = true;
		cpu->schedule_timestamp = cpu->simulator->current_tick + 1;
	}

	process_end(cpu);
	return;
}
