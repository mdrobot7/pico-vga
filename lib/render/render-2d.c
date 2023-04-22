#include "render.h"

/**
 * @brief Get whether the bit at coordinates x,y in char c is 1 or 0 (color or no color) in the font
 * 
 * @param c The char to look up in the font (Dims: CHAR_WIDTH * CHAR_HEIGHT)
 * @param x The x position within that char
 * @param y The y position within that char
 * @return uint8_t 1 or 0
 */
static inline uint8_t getFontBit(uint8_t c, uint8_t x, uint8_t y) {
    return *(font + CHAR_HEIGHT*c + y) >> (CHAR_WIDTH - x + 1) & 1u;
}

static inline void bresenhamLow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    //Bresenham's line drawing algorithm, thanks Wikipedia! (https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm)
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;

    int32_t yi = 1;
    if(dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int32_t D = (2*dy) - dx;
    int32_t y = y1;

    for(int32_t x = x1; x < x2; x++) {
        writePixel(y, x, color);
        if(D > 0) {
            y = y + yi;
            D = D + 2*(dy - dx);
        }
        else {
            D = D + 2*dy;
        }
    }
}

static inline void bresenhamHigh(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
    int32_t dx = x2 - x1;
    int32_t dy = y2 - y1;
    
    int32_t xi = 1;
    if(dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int32_t D = (2*dx) - dy;
    int32_t x = x1;

    for(int32_t y = y1; y < y2; y++) {
        writePixel(y, x, color);
        if(D > 0) {
            x = x + xi;
            D = D + 2*(dx - dy);
        }
        else {
            D = D + 2*dx;
        }
    }
}

static inline void renderFastVertLine(uint16_t x, uint16_t y1, uint16_t y2, uint8_t color) {
    for(; y1 < y2; y1++) {
        writePixel(y1, x, color);
    }
}

static inline void renderFastHorizLine(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color) {
    for(; x1 < x2; x1++) {
        writePixel(y, x1, color);
    }
}

static inline void renderLineAA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
    return;
}

static inline void renderTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t thickness) {
    renderLine(x1, y1, x2, y2, color, thickness);
    renderLine(x2, y2, x3, y3, color, thickness);
    renderLine(x1, y1, x3, y3, color, thickness);
}

static inline void renderFillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
    //Bresenham triangle algorithm, thanks to http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
    if(y1 == y2 && y2 == y3) { //weird case where all points are on the same line
        renderFastHorizLine(min(x1, x2, x3), max(x1, x2, x3), y1, color);
    }

    // Sort coordinates by Y order (y3 >= y2 >= y1)
    if (y1 > y2) {
        uint16_t ty = y1; //swap y1, y2
        y1 = y2;
        y2 = ty;

        uint16_t tx = x1;
        x1 = x2;
        x2 = tx;
    }
    if (y2 > y3) {
        uint16_t ty = y2; //swap y2, y3
        y2 = y3;
        y3 = ty;

        uint16_t tx = x2;
        x2 = x3;
        x3 = tx;
    }
    if (y1 > y2) {
        uint16_t ty = y1; //swap y1, y2
        y1 = y2;
        y2 = ty;

        uint16_t tx = x1;
        x1 = x2;
        x2 = tx;
    }

    if(y1 == y2) {} //flat-topped triangle
    else if(y2 == y3) {} //flat-bottomed triangle
    else { //all other triangles
        uint16_t ySplit = y2; //the split between the two flat-sided triangles
    }
}

static inline void renderCircle(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color) {
    return;
}

static inline void renderCircleAA(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color) {
    return;
}

static inline void renderPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness) {
    //Split up for performance
    if(displayConfig->antiAliasing) {
        for(int i = 0; i < numPoints - 2; i += 2) {
            renderLineAA(points[i], points[i + 1], points[i + 2], points[i + 3], color, thickness);
        }
    }
    else {
        for(int i = 0; i < numPoints - 2; i += 2) {
            renderLine(points[i], points[i + 1], points[i + 2], points[i + 3], color, thickness);
        }
    }
}

//If you have any shape that is convex on the top or bottom, the stack overflow polygon fill algorithm isn't
//gonna work (like an upside down U shape or something). It doesn't handle multiple shading sections at all.

static void renderFillPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness) {
    uint16_t minX = 0xFFFF, minY = 0xFFFF, maxX = 0, maxY = 0;
    for(int i = 0; i < numPoints; i += 2) {
        if(points[i] < minX) minX = points[i];
        if(points[i] > maxX) maxX = points[i];
        if(points[i + 1] < minY) minY = points[i + 1];
        if(points[i + 1] < minX) minX = points[i + 1];
    }

    uint16_t x0Buf[maxY - minY];
    uint16_t x1Buf[maxY - minY];

    //TODO

    for(int i = 0; i < maxY - minY; i++) {
        renderFastHorizLine(x0Buf[i], x1Buf[i], minY + i, color);
    }
}

//====================================================================================================//

/**
 * @brief Render a line from (x1, y1) to (x2, y2) with a thickness using Bresenham's line 
 * algorithm (integer only)
 * 
 * @param x1 
 * @param y1 
 * @param x2 
 * @param y2 
 * @param color Color of the line
 * @param thickness Line thickness (default: 1)
 */
void renderLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
    if(x1 == x2) {
        renderFastVertLine(x1, y1, y2, color);
    }
    else if (y1 == y2) {
        renderFastHorizLine(x1, x2, y1, color);
    }
    else {
        if(y2 - y1 < x2 - x1) {
            if(x1 > x2) {
                //if(thickness != 0); //bresenhamLowThick(x2, y2, x1, y1, color, thickness) <- implement later
                bresenhamLow(x2, y2, x1, y1, color); //Coordinate pairs reversed
            }
            else {
                bresenhamLow(x1, y1, x2, y2, color);
            }
        }
        else {
            if(y1 > y2) {
                bresenhamHigh(x2, y2, x1, y1, color); //Coordinate pairs reversed
            }
            else {
                bresenhamHigh(x1, y1, x2, y2, color);
            }
        }
    }
}