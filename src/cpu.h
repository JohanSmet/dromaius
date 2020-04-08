// cpu.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Common 'base' for cpu implementations

#ifndef DROMAIUS_CPU_H
#define DROMAIUS_CPU_H

#include "types.h"

typedef void (*CPU_PROCESS_FUNC)(void *cpu, bool delayed);
typedef void (*CPU_OVERRIDE_NEXT_INSTRUCTION_ADDRESS)(void *cpu, uint16_t addr);
typedef bool (*CPU_IS_AT_START_OF_INSTRUCTION)(void *cpu);
typedef int64_t  (*CPU_PROGRAM_COUNTER)(void *cpu);

#define CPU_DECLARE_FUNCTIONS		\
	CPU_PROCESS_FUNC process;		\
	CPU_OVERRIDE_NEXT_INSTRUCTION_ADDRESS override_next_instruction_address;	\
	CPU_IS_AT_START_OF_INSTRUCTION is_at_start_of_instruction;					\
	CPU_PROGRAM_COUNTER program_counter;

typedef struct Cpu {
	CPU_DECLARE_FUNCTIONS
} Cpu;

#endif // DROMAIUS_CPU_H
