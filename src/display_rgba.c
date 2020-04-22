// display_rgba.c - Johan Smet - BSD-3-Clause (see LICENSE)
//
// A RGBA pixel display implementation

#include "display_rgba.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

DisplayRGBA *display_rgba_create(size_t width, size_t height) {
	DisplayRGBA *display = malloc(sizeof(DisplayRGBA) + (width * height * sizeof(int32_t)));
	display->width = width;
	display->height = height;
	return display;
}

void display_rgba_destroy(DisplayRGBA *display) {
	assert(display);
	free(display);
}

void display_rgba_clear(DisplayRGBA *display) {
	assert(display);
	memset(display->frame, 0, display->width * display->height * sizeof(int32_t));
}
