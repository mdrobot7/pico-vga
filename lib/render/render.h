#ifndef RENDER_H
#include "lib-internal.h"

void renderLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness);
void renderTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t thickness);
void renderFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
void renderCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
void renderFilledCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
void renderPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness);
void renderFilledPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness);
void renderFillScreen(uint8_t color);

void renderLineAA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness);
void renderCircleAA(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color);

/**
 * @brief Writes the color to the pixel at x, y. Handles out-of-bounds coordinates. Will not write to 
 * interpolated lines, use writePixelInterp() for that instead.
 * 
 * @param y Y coordinate in screen space
 * @param x X coordinate in screen space
 * @param color Color to write
 */
inline void writePixelBuffered(uint16_t y, uint16_t x, uint8_t color) {
    if(x >= frameWidth || y >= frameHeight) return;
    else if(*((uint8_t **)frameReadAddrStart + y) < frameBufferStart) return; //don't write to interpolated lines
    else *((uint8_t *)(*((uint8_t **)frameReadAddrStart + y)) + x) = color; //This grabs the y'th element of frameReadAddr, adds x to it, dereferences, and writes the color to it
}

/**
 * @brief Writes the color to the pixel at x, y. Handles out-of-bounds coordinates. Will not write to 
 * buffered lines, only the interpolated lines, use writePixelBuffered() for that instead.
 * 
 * @param y Y coordinate in screen space
 * @param x X coordinate in screen space
 * @param color Color to write
 */
inline void writePixelInterp(uint16_t y, uint16_t x, uint8_t color) {
    if(x >= frameWidth || y >= frameHeight) return;
    else if(*((uint8_t **)frameReadAddrStart + y) > frameBufferStart) return; //don't write to buffered lines
    else *((uint8_t *)(*((uint8_t **)frameReadAddrStart + y)) + x) = color; //This grabs the y'th element of frameReadAddr, adds x to it, dereferences, and writes the color to it
}

#endif