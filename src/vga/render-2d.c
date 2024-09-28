#include "../common.h"
#include "color.h"
#include "hardware/dma.h"
#include "pico/platform.h"
#include "render.h"
#include "vga.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTIONS
 ************************************/

static void render_fast_vert_line(uint16_t x, uint16_t y1, uint16_t y2, vga_color_t color) {
  if (y2 > y1) {
    SWAP(y1, y2);
  }
  for (; y1 <= y2; y1++) {
    render_pixel(y1, x, color);
  }
}

static void render_fast_horiz_line(uint16_t x1, uint16_t x2, uint16_t y, vga_color_t color) {
  if (x2 > x1) {
    SWAP(x1, x2);
  }
  for (; x1 <= x2; x1++) {
    render_pixel(y, x1, color);
  }
}

static void bresenham_low(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
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
    render_pixel(y, x, color);
    if (D > 0) {
      y = y + yi;
      D = D + 2 * (dy - dx);
    } else {
      D = D + 2 * dy;
    }
  }
}

static void bresenham_high(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
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
    render_pixel(y, x, color);
    if (D > 0) {
      x = x + xi;
      D = D + 2 * (dx - dy);
    } else {
      D = D + 2 * dx;
    }
  }
}

static void bresenham_circle(uint16_t x, uint16_t y, uint16_t pixel_x, uint16_t pixel_y, vga_color_t color) {
  render_pixel(y + pixel_y, x + pixel_x, color);
  render_pixel(y + pixel_y, x - pixel_x, color);
  render_pixel(y - pixel_y, x + pixel_x, color);
  render_pixel(y - pixel_y, x - pixel_x, color);
  render_pixel(y + pixel_x, x + pixel_y, color);
  render_pixel(y + pixel_x, x - pixel_y, color);
  render_pixel(y - pixel_x, x + pixel_y, color);
  render_pixel(y - pixel_x, x - pixel_y, color);
}

static void bresenham_circle_filled(uint16_t x, uint16_t y, uint16_t pixel_x, uint16_t pixel_y, vga_color_t color) {
  render_fast_horiz_line(x - pixel_x, x + pixel_x, y + pixel_y, color);
  render_fast_horiz_line(x + pixel_x, x - pixel_x, y - pixel_y, color);
  render_fast_horiz_line(x + pixel_y, x - pixel_y, y + pixel_x, color);
  render_fast_horiz_line(x + pixel_y, x - pixel_y, y - pixel_x, color);
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

void render2d_fill(vga_color_t color) {
  render2d_rectangle_filled(0, 0, vga_get_width() - 1, vga_get_height() - 1, color);
}

void render2d_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
  if (x1 == x2) {
    render_fast_vert_line(x1, y1, y2, color);
  } else if (y1 == y2) {
    render_fast_horiz_line(x1, x2, y1, color);
  } else {
    if (ABS(y2 - y1) < ABS(x2 - x1)) {        // -1 < slope < 1
      if (x1 > x2) {                          // line goes right -> left
        bresenham_low(x2, y2, x1, y1, color); // Coordinate pairs reversed
      } else {
        bresenham_low(x1, y1, x2, y2, color);
      }
    } else {                                   // slope =< -1 || slope >= 1
      if (y1 > y2) {                           // line goes bottom -> top
        bresenham_high(x2, y2, x1, y1, color); // Coordinate pairs reversed
      } else {
        bresenham_high(x1, y1, x2, y2, color);
      }
    }
  }
}

void render2d_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
  render_fast_vert_line(x1, y1, y2, color);
  render_fast_vert_line(x2, y1, y2, color);
  render_fast_horiz_line(x1, x2, y1, color);
  render_fast_horiz_line(x1, x2, y2, color);
}

void render2d_rectangle_filled(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
  static volatile dma_channel_config config = {
    .ctrl = DMA_CH0_CTRL_TRIG_INCR_WRITE_BITS
          | (DREQ_FORCE << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB)
          | (DMA_SIZE_32 << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB)
          | DMA_CH0_CTRL_TRIG_HIGH_PRIORITY_BITS
          | DMA_CH0_CTRL_TRIG_EN_BITS,
  };
  uint32_t color32 = color | (color << 8) | (color << 16) | (color << 24);

  for (int y = y1; y <= y2; y++) {
    for (int x = x1; x <= x2; x++) {
      render_pixel(y, x, color);
    }
  }
  return;

  // *** Don't care about this for now ***

  static uint8_t dma_chan;
  dma_chan        = !dma_chan ? dma_claim_unused_channel(true) : dma_chan;
  int dma_bytes   = (x2 - x1 + 1) / sizeof(uint32_t); // coordinates are inclusive
  int spare_bytes = (x2 - x1 + 1) % sizeof(uint32_t);

  // DMA one line at a time -- if a rectangle is in the middle of
  // the screen DMA can't jump the gaps on the left and right.
  // Handle line doubling as well: only write to doubled lines once,
  // skip over all of the duplicates (hence the +=)
  for (int y = y1; y < y2; y += vga_get_config()->scaled_resolution) {
    channel_config_set_chain_to(&config, dma_chan); // Make sure chaining is off
    dma_channel_configure(dma_chan, &config, &color32, render_get_pixel_ptr(y, x1), dma_bytes, true);

    for (int x = 0; x < spare_bytes; x++) {
      render_pixel(y, x1 + dma_bytes + x, color);
    }

    dma_channel_wait_for_finish_blocking(dma_chan);
  }

  // dma_channel_unclaim(dma_chan);
  asm("nop");
}

void render2d_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color) {
  render2d_line(x1, y1, x2, y2, color);
  render2d_line(x2, y2, x3, y3, color);
  render2d_line(x1, y1, x3, y3, color);
}

void render2d_triangle_filled(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color) {
  // Adafruit GFX slope algorithm, https://github.com/adafruit/Adafruit-GFX-Library/blob/master/Adafruit_GFX.cpp
  int32_t a, b, y, last;

  // Sort coordinates by Y order (y3 >= y2 >= y1)
  if (y1 > y2) {
    SWAP(y1, y2);
    SWAP(x1, x2);
  }
  if (y2 > y3) {
    SWAP(y3, y2);
    SWAP(x3, x2);
  }
  if (y1 > y2) {
    SWAP(y1, y2);
    SWAP(x1, x2);
  }

  if (y1 == y3) { // Handle awkward all-on-same-line case as its own thing
    render_fast_horiz_line(MIN(x1, MIN(x2, x3)), MAX(x1, MAX(x2, x3)), y1, color);
    return;
  }

  int32_t dx12 = x2 - x1,
          dy12 = y2 - y1,
          dx13 = x3 - x1,
          dy13 = y3 - y1,
          dx23 = x3 - x2,
          dy23 = y3 - y2;
  int32_t sa = 0, sb = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y2=y3 (flat-bottomed triangle), the scanline y2
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y2 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y1=y2
  // (flat-topped triangle).
  if (y2 == y3) {
    last = y2; // Include y2 scanline
  } else {
    last = y2 - 1; // Skip it
  }

  for (y = y1; y <= last; y++) {
    a = x1 + sa / dy12;
    b = x1 + sb / dy13;
    sa += dx12;
    sb += dx12;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
    */
    if (a > b) {
      SWAP(a, b);
    }
    render_fast_horiz_line(a, b - a + 1, y, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y2=y3.
  sa = (int32_t) dx23 * (y - y2);
  sb = (int32_t) dx13 * (y - y1);
  for (; y <= y3; y++) {
    a = x2 + sa / dy23;
    b = x1 + sb / dy13;
    sa += dx23;
    sb += dx13;
    /* longhand:
    a = x2 + (x3 - x2) * (y - y2) / (y3 - y2);
    b = x1 + (x3 - x1) * (y - y1) / (y3 - y1);
    */
    if (a > b) {
      SWAP(a, b);
    }
    render_fast_horiz_line(a, b - a + 1, y, color);
  }
}

void render2d_circle(uint16_t x, uint16_t y, uint16_t radius, vga_color_t color) {
  // Bresenham circle algorithm, https://www.geeksforgeeks.org/bresenhams-circle-drawing-algorithm/
  int32_t pixel_x = 0, pixel_y = radius;
  int32_t d = 3 - 2 * radius;
  bresenham_circle(x, y, pixel_x, pixel_y, color);
  while (pixel_y >= pixel_x) {
    // check for decision parameter and correspondingly update d, y
    if (d > 0) {
      pixel_y--;
      d = d + 4 * (pixel_x - pixel_y) + 10;
    } else {
      d = d + 4 * pixel_x + 6;
    }

    // Increment x after updating decision parameter
    pixel_x++;

    // Draw the circle using the new coordinates
    bresenham_circle(x, y, pixel_x, pixel_y, color);
  }
}

void render2d_circle_filled(uint16_t x, uint16_t y, uint16_t radius, vga_color_t color) {
  // Bresenham circle algorithm, modified for a filled circle
  int32_t pixel_x = 0, pixel_y = radius;
  int32_t d = 3 - 2 * radius;
  bresenham_circle_filled(x, y, pixel_x, pixel_y, color);
  while (pixel_y >= pixel_x) {
    // check for decision parameter and correspondingly update d, y
    if (d > 0) {
      pixel_y--;
      d = d + 4 * (pixel_x - pixel_y) + 10;
    } else {
      d = d + 4 * pixel_x + 6;
    }

    // Increment x after updating decision parameter
    pixel_x++;

    // Draw the circle using the new coordinates
    bresenham_circle_filled(x, y, pixel_x, pixel_y, color);
  }
}

void render2d_polygon(uint16_t points[][2], uint16_t num_points, vga_color_t color) {
  for (int i = 0; i < num_points; i++) {
    render2d_line(points[i][POINT_X], points[i + 1][POINT_X], points[i][POINT_Y], points[i + 1][POINT_Y], color);
  }
  render2d_line(points[num_points][POINT_X], points[0][POINT_X], points[num_points][POINT_Y], points[0][POINT_Y], color);
}

void render2d_polygon_filled(uint16_t points[][2], const uint16_t num_points, vga_color_t color) {
  // Sunshine's polygon filling algorithm applet, http://www.sunshine2k.de/coding/java/Polygon/Filling/FillPolygon.htm
  enum {
    EDGE_TOP = 0, //"top" point, higher on the screen/smaller y coordinate
    EDGE_BOT,
  };
  // turn points into edges
  uint16_t * edges[num_points][2]; // Pairs of points, !! could be large !!
  uint32_t i;
  for (i = 0; i < num_points; i++) {
    // put the edge in Y-order (top to bottom)
    if (points[i][POINT_Y] < points[i + 1][POINT_Y]) {
      edges[i][EDGE_TOP] = points[i];
      edges[i][EDGE_BOT] = points[(i + 1) % num_points];
    } else {
      edges[i][EDGE_TOP] = points[(i + 1) % num_points];
      edges[i][EDGE_BOT] = points[i];
    }
  }

  // sort edges in y-order (selection sort)
  for (i = 0; i < num_points - 1u; i++) {
    uint32_t j              = i + 1;
    uint32_t smallest_y_idx = j;
    for (; j < num_points; j++) {
      if (edges[j][EDGE_TOP][POINT_Y] < edges[smallest_y_idx][EDGE_TOP][POINT_Y]) {
        smallest_y_idx = j;
      }
    }
    if (smallest_y_idx != i) {
      SWAP(edges[i][EDGE_TOP], edges[smallest_y_idx][EDGE_TOP]);
      SWAP(edges[i][EDGE_BOT], edges[smallest_y_idx][EDGE_BOT]);
    }
  }

  // find the largest y value (NOT guaranteed by edges sorting)
  uint32_t largest_y = 0;
  for (i = 0; i < num_points; i++) {
    if (points[i][POINT_Y] > largest_y) {
      largest_y = points[i][POINT_Y];
    }
  }

  // loop through each scanline
  uint16_t x_coords[num_points]; // !!! can be large !!!
  uint32_t x_coords_idx = 0;
  for (i = edges[0][EDGE_TOP][POINT_Y]; i < largest_y; i++) {
    uint32_t j;
    // find edges cut by the scanline
    for (j = 0; j < num_points; i++) {
      if (edges[j][EDGE_TOP][POINT_Y] == i) { // reached the top of an edge
        if (edges[j][EDGE_BOT][POINT_Y] == i) {
          x_coords[x_coords_idx++] = edges[j][EDGE_TOP][POINT_X]; // horizontal edge, add both ends
          x_coords[x_coords_idx++] = edges[j][EDGE_TOP][POINT_X];
        }
      } else if (edges[j][EDGE_BOT][POINT_Y] == i) { // reached the bottom of an edge
        x_coords[x_coords_idx++] = edges[j][EDGE_BOT][POINT_X];
      } else { // somewhere in the middle
        x_coords[x_coords_idx++] = 1;

        // point slope (y - y1) = m(x - x1) -> (y - y1)/m + x1 = x TODO
      }
    }

    if (x_coords_idx < 2 || x_coords_idx % 2 != 0) {
      // this should never happen with a legal polygon
      continue;
    }

    // sort x coordinates ascending (selection sort)
    for (j = 0; j < num_points - 1u; j++) {
      uint32_t k              = j + 1;
      uint32_t smallest_x_idx = k;
      for (; k < num_points; k++) {
        if (x_coords[k] < x_coords[smallest_x_idx]) {
          smallest_x_idx = k;
        }
      }
      if (smallest_x_idx != j) {
        SWAP(x_coords[j], x_coords[smallest_x_idx]);
      }
    }

    // draw between each pair of points
    for (j = 0; j < x_coords_idx; j += 2) {
      render_fast_horiz_line(x_coords[j], x_coords[j + 1], i, color);
    }
  }
}

void render2d_string(char * str, uint16_t x1, uint16_t y, uint16_t x2, bool wrap, vga_color_t color) {
  uint16_t cursor_x = x1; // in pixels
  uint16_t cursor_y = y;
  for (int i = 0; str[i] != '\0'; i++) {
    for (int bit_y = 0; bit_y < FONT_HEIGHT; bit_y++) {
      for (int bit_x = 0; bit_x < FONT_WIDTH; bit_x++) {
        if (GET_BIT(draw2d_get_font()[i], bit_x)) {
          // bits are grabbed right -> left (0 -> 5), but need to be rendered left -> right
          render_pixel(cursor_y + bit_y, cursor_x + (FONT_WIDTH - bit_x), color);
        }
      }
    }
    cursor_x = (cursor_x + FONT_WIDTH + FONT_SPACING);
    if (wrap) {
      if (cursor_x > x2) {
        cursor_x = x2;
        cursor_y += FONT_HEIGHT;
      }
    } else if (cursor_x > vga_get_width()) { // Make sure we can't run off into random memory
      cursor_x = vga_get_width();
    }
  }
}

void render2d_sprite(vga_color_t * sprite, uint16_t x, uint16_t y, uint16_t size_x, uint16_t size_y, vga_color_t null_color) {
  for (uint32_t i = 0; i < size_y; i++) {
    for (uint32_t j = 0; j < size_x; j++) {
      vga_color_t color = *((sprite + size_x * i) + j);
      if (color != null_color) {
        render_pixel(y + i, x + j, color);
      }
    }
  }
}