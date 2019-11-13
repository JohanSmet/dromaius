// types.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Easy include to make sure commonly used basic types are defined

#ifndef DROMAIUS_TYPES_H
#define DROMAIUS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// some basic operation
#define MIN(a,b)    ((a) <= (b) ? (a) : (b))
#define MAX(a,b)    ((a) >= (b) ? (a) : (b))
#define IS_POW2(x)  ((x) != 0 && ((x) & ((x)-1)) == 0)
#define CLAMP(x, min, max)  (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))

#define LOBYTE(x)		((x) & 0xff)					// low byte of a 16-bit word
#define HIBYTE(x)		(((x) >> 8) & 0xff)				// high byte of a 16-bit word

#define SET_LOBYTE(x,l)	(((x) & 0xff00) | (l))			// set low byte of a 16-bit word
#define SET_HIBYTE(x,h) (((x) & 0x00ff) | ((h) << 8))	// set high byte of a 16-bit word

#define IS_BIT_SET(x,b) (((x) & (1 << (b))) >> (b))

#define MAKE_WORD(h,l) (((h) << 8) | (l))

#endif // DROMAIUS_TYPES_H
