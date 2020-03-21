// types.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Easy include to make sure commonly used basic types are defined

#ifndef DROMAIUS_TYPES_H
#define DROMAIUS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// some basic operations
#define MIN(a,b)    ((a) <= (b) ? (a) : (b))
#define MAX(a,b)    ((a) >= (b) ? (a) : (b))
#define IS_POW2(x)  ((x) != 0 && ((x) & ((x)-1)) == 0)
#define CLAMP(x, min, max)  (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))

#define LO_BYTE(x)		(uint8_t) ((x) & 0xff)						// low byte of a 16-bit word
#define HI_BYTE(x)		(uint8_t) (((x) >> 8) & 0xff)				// high byte of a 16-bit word

#define SET_LO_BYTE(x,l) ((uint16_t) (((x) & 0xff00) | (l)))		// set low byte of a 16-bit word
#define SET_HI_BYTE(x,h) ((uint16_t) (((x) & 0x00ff) | ((h) << 8)))	// set high byte of a 16-bit word

#define BIT_SET(x,b)	((x) |= (1 << (b)))
#define BIT_CLEAR(x,b)	((x) &= (uint8_t) ~(1 << (b)))
#define BIT_IS_SET(x,b) ((bool) (((x) & (1 << (b))) >> (b)))

#define FLAG_SET(x,f)			((x) |= (f))
#define FLAG_CLEAR(x,f)			((x) &= ~(f))
#define FLAG_IS_SET(x,f)		((bool) (((x) & (f)) == (f)))
#define FLAG_SET_CLEAR(x,f,c)		((x) = ((x) & ~(f)) | (-(c) & (f)))
#define FLAG_SET_CLEAR_U8(x,f,c)	((x) = (uint8_t) (((x) & ~(f)) | (-(c) & (f))))
#define FLAG_SET_CLEAR_U16(x,f,c)	((x) = (uint16_t) (((x) & ~(f)) | (-(c) & (f))))

#define MASK_IS_SET(x,m,v)		((bool) (((x) & (m)) == (v)))

#define MAKE_WORD(h,l)	((uint16_t) (((h) << 8) | (l)))

#define BCD_BYTE(h,l)	((((h) & 0xf) << 4) | ((l) & 0xf))

#define INC_UINT16(v,d) ((v) = (uint16_t) ((v) + (d)))
#define DEC_UINT16(v,d) ((v) = (uint16_t) ((v) - (d)))
#define INC_UINT8(v,d) ((v) = (uint8_t) ((v) + (d)))
#define DEC_UINT8(v,d) (((v) = uint8_t) ((v) - (d)))


#define ACTLO_ASSERTED(x) ((x) == false)
#define ACTHI_ASSERTED(x) ((x) == true)

#define ACTLO_ASSERT false
#define ACTLO_DEASSERT true

#define ACTHI_ASSERT true
#define ACTHI_DEASSERT false

#endif // DROMAIUS_TYPES_H
