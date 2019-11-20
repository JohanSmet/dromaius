// filter_6502_asm.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Dissambler for 6502 assembly

#ifndef DROMAIUS_FILTER_6502_ASM_H
#define DROMAIUS_FILTER_6502_ASM_H

#include "types.h"


int filt_6502_asm_line(const uint8_t *binary, size_t bin_size, size_t bin_index, size_t bin_offset, char **line);


#endif // DROMAIUS_FILTER_6502_ASM_H
