#ifndef RENDER_H
#include "lib-internal.h"



//Checks for out-of-bounds coordinates, does pointer math, and writes to the frame
static inline void writePixel(uint16_t y, uint16_t x, uint8_t color) {
    if(x >= frameWidth || y >= frameHeight) return;
    //calculating the row/y off of frameReadAddr handles interpolated lines.
    //This grabs the y'th element of frameReadAddr, adds x to it, dereferences, and writes the color to it
    else *((uint8_t *)(*((uint8_t **)frameReadAddrStart + y)) + x) = color;
}

#endif