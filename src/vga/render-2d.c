#include "hardware/dma.h"
#include "pico/float.h"
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
  return *(font + CHAR_HEIGHT * c + y) >> (CHAR_WIDTH - x + 1) & 1u;
}

/**
 * @brief Render a line from (x, y1) to (x, y2) INCLUSIVE with a color.
 *
 * @param x
 * @param y1
 * @param y2
 * @param color
 */
static inline void renderFastVertLine(uint16_t x, uint16_t y1, uint16_t y2, uint8_t color) {
  for (; y1 <= y2; y1++) {
    writePixel(y1, x, color);
  }
}

/**
 * @brief Render a line from (x1, y) to (x2, y) INCLUSIVE with a color.
 *
 * @param x1
 * @param x2
 * @param y
 * @param color
 */
static inline void renderFastHorizLine(uint16_t x1, uint16_t x2, uint16_t y, uint8_t color) {
  for (; x1 <= x2; x1++) {
    writePixel(y, x1, color);
  }
}

// Standard Bresenham low/high functions
static inline void bresenhamLow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
  // Bresenham's line drawing algorithm, thanks Wikipedia! (https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm)
  int32_t dx = x2 - x1;
  int32_t dy = y2 - y1;

  int32_t yi = 1;
  if (dy < 0) {
    yi = -1;
    dy = -dy;
  }
  int32_t D = (2 * dy) - dx;
  int32_t y = y1;

  for (int32_t x = x1; x < x2; x++) {
    writePixel(y, x, color);
    if (D > 0) {
      y = y + yi;
      D = D + 2 * (dy - dx);
    } else {
      D = D + 2 * dy;
    }
  }
}

static inline void bresenhamHigh(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color) {
  int32_t dx = x2 - x1;
  int32_t dy = y2 - y1;

  int32_t xi = 1;
  if (dx < 0) {
    xi = -1;
    dx = -dx;
  }
  int32_t D = (2 * dx) - dy;
  int32_t x = x1;

  for (int32_t y = y1; y < y2; y++) {
    writePixel(y, x, color);
    if (D > 0) {
      x = x + xi;
      D = D + 2 * (dx - dy);
    } else {
      D = D + 2 * dx;
    }
  }
}

/**
 * @brief Render a triangle with a flat bottom using Bresenham's triangle algorithm
 *
 * @param x1 X coordinate of the peak
 * @param y1 Y coordinate of the peak
 * @param x2 The bottom left x coordinate (along the flat edge)
 * @param x3 The bottom right x coordinate (along the flat edge)
 * @param yBot The y coordinate of the bottom edge
 * @param color The color to fill the triangle
 */
static inline void renderFlatBottomedTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t x3, uint16_t yBot, uint8_t color) {
  return;
}

/**
 * @brief Render a triangle with a flat top using Bresenham's triangle algorithm
 *
 * @param x1 X coordinate of the valley
 * @param y1 Y coordinate of the valley
 * @param x2 The top left x coordinate (along the flat edge)
 * @param x3 The top right x coordinate (along the flat edge)
 * @param yTop The y coordinate of the top edge
 * @param color The color to fill the triangle
 */
static inline void renderFlatToppedTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t x3, uint16_t yTop, uint8_t color) {
  return;
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
  if (x1 == x2) {
    renderFastVertLine(x1, y1, y2, color);
  } else if (y1 == y2) {
    renderFastHorizLine(x1, x2, y1, color);
  } else {
    if (abs(y2 - y1) < abs(x2 - x1)) {
      if (x1 > x2) {
        // if(thickness != 0); //bresenhamLowThick(x2, y2, x1, y1, color, thickness) <- implement later
        bresenhamLow(x2, y2, x1, y1, color); // Coordinate pairs reversed
      } else {
        bresenhamLow(x1, y1, x2, y2, color);
      }
    } else {
      if (y1 > y2) {
        bresenhamHigh(x2, y2, x1, y1, color); // Coordinate pairs reversed
      } else {
        bresenhamHigh(x1, y1, x2, y2, color);
      }
    }
  }
}

/**
 * @brief Render a line from (x1, y1) to (x2, y2) with antialiasing.
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param color Color of the line
 * @param thickness Thickness of the line (default: 1)
 */
void renderLineAA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
  return;
}

/**
 * @brief Render a triangle with no fill.
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param x3
 * @param y3
 * @param color Color of the lines.
 * @param thickness Thickness of the lines.
 */
inline void renderTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color, uint8_t thickness) {
  renderLine(x1, y1, x2, y2, color, thickness);
  renderLine(x2, y2, x3, y3, color, thickness);
  renderLine(x1, y1, x3, y3, color, thickness);
}

/**
 * @brief Render a triangle with a fill
 *
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param x3
 * @param y3
 * @param color
 */
inline void renderFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color) {
  // Bresenham triangle algorithm, thanks to http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
  if (y1 == y2 && y2 == y3) { // weird case where all points are on the same line
    renderFastHorizLine(min(x1, x2, x3), max(x1, x2, x3), y1, color);
  }

  // Sort coordinates by Y order (y3 >= y2 >= y1)
  if (y1 > y2) {
    uint16_t ty = y1; // swap y1, y2
    y1          = y2;
    y2          = ty;

    uint16_t tx = x1;
    x1          = x2;
    x2          = tx;
  }
  if (y2 > y3) {
    uint16_t ty = y2; // swap y2, y3
    y2          = y3;
    y3          = ty;

    uint16_t tx = x2;
    x2          = x3;
    x3          = tx;
  }
  if (y1 > y2) {
    uint16_t ty = y1; // swap y1, y2
    y1          = y2;
    y2          = ty;

    uint16_t tx = x1;
    x1          = x2;
    x2          = tx;
  }

  if (y1 == y2) { // flat-topped triangle
    renderFlatToppedTriangle(x3, y3, x1 < x2 ? x1 : x2, x1 < x2 ? x2 : x1, y1, color);
  } else if (y2 == y3) { // flat-bottomed triangle
    renderFlatBottomedTriangle(x1, y1, x2 < x3 ? x2 : x3, x2 < x3 ? x3 : x2, y2, color);
  } else {                                                      // all other triangles
    uint16_t ySplit = y2;                                       // the split between the two flat-sided triangles
    uint16_t xSplit = x1 + ((y2 - y1) * (x3 - x1)) / (y3 - y1); // derived from point-slope form, proof is on the linked website (I verified it)
    renderFlatBottomedTriangle(x1, y1, x2 < xSplit ? x2 : xSplit, x2 < xSplit ? xSplit : x2, ySplit, color);
    renderFlatToppedTriangle(x3, y3, x2 < xSplit ? x2 : xSplit, x2 < xSplit ? xSplit : x2, ySplit, color);
  }
}

/**
 * @brief Render a circle centered at (x, y). SLOW, uses floating point math.
 *
 * @param x
 * @param y
 * @param radius
 * @param color Color of the circle
 */
inline void renderCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
  for (uint16_t Y = y - radius; Y <= y + radius; Y++) {
    uint16_t X = sqrt(pow(radius, 2.0) - pow(Y - y, 2.0));
    writePixel(Y, x + X, color); // handle the +/- from the sqrt (the 2 sides of the circle)
    writePixel(Y, x - X, color);
  }
}

/**
 * @brief Render a filled circle centered at (x, y). SLOW, uses floating point math.
 *
 * @param x
 * @param y
 * @param radius
 * @param color Color of the circle and fill.
 */
inline void renderFilledCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
  // x1, y1 = center, z = radius in x direction
  for (uint16_t Y = y - radius; Y <= y + radius; Y++) {
    uint16_t X = sqrt(pow(radius, 2.0) - pow(Y - y, 2.0));
    renderFastHorizLine(x - X, x + X, Y, color);
  }
}

void renderCircleAA(uint16_t x1, uint16_t y1, uint16_t radius, uint8_t color) {
  return;
}

void renderPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness) {
  // Split up for performance
  if (displayConfig->antiAliasing) {
    for (int i = 0; i < numPoints - 2; i += 2) {
      renderLineAA(points[i], points[i + 1], points[i + 2], points[i + 3], color, thickness);
    }
  } else {
    for (int i = 0; i < numPoints - 2; i += 2) {
      renderLine(points[i], points[i + 1], points[i + 2], points[i + 3], color, thickness);
    }
  }
}

// If you have any shape that is convex on the top or bottom, the stack overflow polygon fill algorithm isn't
// gonna work (like an upside down U shape or something). It doesn't handle multiple shading sections at all.

void renderFilledPolygon(uint16_t points[], uint8_t numPoints, uint8_t dimPoints, uint8_t color, uint8_t thickness) {
  uint16_t minX = 0xFFFF, minY = 0xFFFF, maxX = 0, maxY = 0;
  for (int i = 0; i < numPoints; i += 2) {
    if (points[i] < minX) minX = points[i];
    if (points[i] > maxX) maxX = points[i];
    if (points[i + 1] < minY) minY = points[i + 1];
    if (points[i + 1] < minX) minX = points[i + 1];
  }

  uint16_t x0Buf[maxY - minY];
  uint16_t x1Buf[maxY - minY];

  // TODO

  for (int i = 0; i < maxY - minY; i++) {
    renderFastHorizLine(x0Buf[i], x1Buf[i], minY + i, color);
  }
}

/**
 * @brief Fills the full screen with a color. Uses the DMA for significantly faster writes.
 *
 * @param color
 */
void renderFillScreen(uint8_t color) {
  uint32_t color32 = color | (color << 8) | (color << 16) | (color << 24);

  uint8_t dmaChan    = dma_claim_unused_channel(true);
  uint8_t spareBytes = (frameWidth * frameHeight) % 4;

  for (uint8_t i = 0; i < spareBytes; i++) {
    frameBufferStart[i] = color; // treating the frame buffer as a large 1D array
  }

  dma_channel_config config = dma_channel_get_default_config(dmaChan);
  channel_config_set_high_priority(&config, true); // channels are round-robin arbitrated, this is probably fine
  channel_config_set_read_increment(&config, false);
  channel_config_set_write_increment(&config, true);
  dma_channel_configure(dmaChan, &config, &color32, &frameBufferStart[spareBytes], (frameWidth * frameHeight) / 4, true);

  dma_channel_wait_for_finish_blocking(dmaChan);

  dma_channel_unclaim(dmaChan);
}

void renderInterpLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness) {
  return;
}