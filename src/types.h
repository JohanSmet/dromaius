// types.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// Easy include to make sure commonly used basic types are defined

#ifndef DROMAIUS_TYPES_H
#define DROMAIUS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

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
#define BIT_SET_U16(x,b)((x) |= (uint16_t) (1 << (b)))
#define BIT_CLEAR(x,b)	((x) &= (uint8_t) ~(1 << (b)))
#define BIT_IS_SET(x,b) ((bool) (((x) & (1 << (b))) >> (b)))

#ifdef _MSC_VER
	inline int bit_lowest_set(uint64_t x) {
		unsigned long index;
		_BitScanForward64(&index, x);
		return index;
	}
#else
	static inline int bit_lowest_set(uint64_t x) {
		return	__builtin_ctzll(x);
	}
#endif

#define FLAG_SET(x,f)			((x) |= (f))
#define FLAG_CLEAR_U8(x,f)		((x) &= (uint8_t) ~(f))
#define FLAG_CLEAR_U64(x,f)		((x) &= (uint64_t) ~(f))
#define FLAG_IS_SET(x,f)		((bool) ((x) & (f)))
#define FLAG_SET_CLEAR_U8(x,f,c)	((x) = (uint8_t) (((x) & ~(f)) | (-(int)(c) & (f))))
#define FLAG_SET_CLEAR_U16(x,f,c)	((x) = (uint16_t) (((x) & ~(f)) | (-(int)(c) & (f))))
#define FLAG_SET_CLEAR_U64(x,f,c)	((x) = (uint64_t) (((x) & ~(f)) | ((uint64_t) -(int64_t)(c) & (f))))

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

// time
#define S_TO_PS(t)  ((t) * 1000000000000ll)		// second to picosecond
#define MS_TO_PS(t) ((t) * 1000000000ll)		// millisecond to picosecond
#define US_TO_PS(t) ((t) * 1000000ll)			// microsecond to picosecond
#define NS_TO_PS(t) ((t) * 1000ll)				// nanosecond to picosecond

#define PS_TO_NS(t) ((t) / 1000ll)
#define PS_TO_US(t) ((t) / 1000000ll)
#define PS_TO_MS(t) ((t) / 1000000000ll)
#define PS_TO_S(t)  ((t) / 1000000000000ll)

#define FREQUENCY_TO_PS(f)	(1000000000000ll / (f)) // frequency (in hertz) to interval in picoseconds

#endif // DROMAIUS_TYPES_H
