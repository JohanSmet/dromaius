// display_rgba.h - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A RGBA pixel display implementation

#ifndef DROMAIUS_DISPLAY_RGBA_H
#define DROMAIUS_DISPLAY_RGBA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// types
typedef struct DisplayRGBA {
	size_t		width;				// in pixels
	size_t		height;				// in pixels
	uint32_t	frame[];			// RGBA buffer (width * height elements)
} DisplayRGBA;

// functions
DisplayRGBA *display_rgba_create(size_t width, size_t height);
void display_rgba_destroy(DisplayRGBA *display);
void display_rgba_clear(DisplayRGBA *display);

#ifdef __cplusplus
}
#endif

#endif // DROMAIUS_DISPLAY_RGBA_H
