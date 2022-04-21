// display_rgba.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A RGBA pixel display implementation

#include "display_rgba.h"
#include "crt.h"

DisplayRGBA *display_rgba_create(size_t width, size_t height) {
	DisplayRGBA *display = dms_malloc(sizeof(DisplayRGBA) + (width * height * sizeof(int32_t)));
	display->width = width;
	display->height = height;
	return display;
}

void display_rgba_destroy(DisplayRGBA *display) {
	assert(display);
	dms_free(display);
}

void display_rgba_clear(DisplayRGBA *display) {
	assert(display);
	dms_zero(display->frame, display->width * display->height * sizeof(int32_t));
}
