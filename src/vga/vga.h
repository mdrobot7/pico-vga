#ifndef __PV_DRAW_H
#define __PV_DRAW_H

#include "hardware/pio.h"
#include "pico/stdlib.h"

// The maximum size of the frame buffer for the pico-vga renderer. The renderer will use
// either the allocated size or the size of 1 frame, whichever is smaller.
// Allocated at compile-time, so be careful of other memory-intensive parts of your program.
#ifndef PV_FRAMEBUFFER_BYTES
#define PV_FRAMEBUFFER_BYTES 200000
#endif

// Switch to true if running in peripheral mode
#ifndef PV_PERIPHERAL_MODE
#define PV_PERIPHERAL_MODE false
#endif

/************************************
 * CONFIGURATION
 ************************************/

typedef enum {
  RES_800x600 = 0,
  RES_640x480,
  RES_1024x768,
} vga_resolution_base_t;

typedef enum {
  RES_SCALED_800x600  = 1,
  RES_SCALED_400x300  = 2,
  RES_SCALED_200x150  = 4,
  RES_SCALED_100x75   = 8,
  RES_SCALED_640x480  = 1,
  RES_SCALED_320x240  = 2,
  RES_SCALED_160x120  = 4,
  RES_SCALED_80x60    = 8,
  RES_SCALED_1024x768 = 1,
} vga_resolution_scaled_t;

typedef struct {
  PIO pio; // Which PIO to use for color
  vga_resolution_base_t base_resolution;
  vga_resolution_scaled_t scaled_resolution;
  bool auto_render;               // Turn on autoRendering (no manual updateDisplay() call required)
  bool antialiasing;              // Turn antialiasing on or off
  uint8_t num_interpolated_lines; // Override the default number of interpolated frame lines -- used if the frame buffer is not large enough to hold all of the frame data. Default = 2.
  uint16_t color_delay_cycles;
} vga_config_t;

// Default configuration for vga_config_t
#define VGA_CONFIG_DEFAULT {                    \
  .pio                    = pio0,               \
  .baseResolution         = RES_800x600,        \
  .resolutionScale        = RES_SCALED_400x300, \
  .auto_render            = true,               \
  .antialiasing           = false,              \
  .num_interpolated_lines = 2,                  \
  .color_delay_cycles     = 0,                  \
}

/************************************
 * RENDER QUEUE
 ************************************/

typedef enum {
  RQI_T_FILL,
  RQI_T_PIXEL,
  RQI_T_LINE,
  RQI_T_RECTANGLE,
  RQI_T_FILLED_RECTANGLE,
  RQI_T_TRIANGLE,
  RQI_T_FILLED_TRIANGLE,
  RQI_T_CIRCLE,
  RQI_T_FILLED_CIRCLE,
  RQI_T_STRING,
  RQI_T_SPRITE,
  RQI_T_BITMAP,
  RQI_T_POLYGON,
  RQI_T_FILLED_POLYGON,
  RQI_T_LIGHT,
  RQI_T_SVG,
  RQI_T_MAX = 255, // Ensure that a vga_render_item_type_t variable is 8 bits
} vga_render_item_type_t;

typedef struct __packed {
  vga_render_item_type_t type : 8;

  // FLAGS
  uint32_t hidden             : 1;
  uint32_t update             : 1;
  uint32_t wordwrap           : 1;
  uint32_t __reserved         : 5;
} vga_render_item_header_t;

// A single triangle, used for polygon meshes in 3D rendering.
typedef struct __packed {
  // Coordinates of the triangle's vertices.
  int16_t x1, y1, z1;
  int16_t x2, y2, z2;
  int16_t x3, y3, z3;

  // Coordinates of the normal vector to the triangle (indicates which side is "up", where to apply textures).
  int16_t normX, normY, normZ;

  // The color of this triangle.
  uint8_t color;

  // Set to true to not shade this triangle.
  uint8_t hidden;
} vga_triangle_t;

struct __packed vga_render_item_t; // Predefinition, takes care of warnings later on from the function pointers
typedef struct __packed {
  vga_render_item_header_t header;

  // Center point (reference point for the entire object, center of rotation)
  // Treated as a "fixed point" value, with the most significant 9 bits being the integer part and
  // the least significant 6 bits being the fractional part (one bit left because it's signed).
  int16_t x, y, z;

  // Rotation in x, y, z -- negative value means negative rotation
  int8_t theta_x, theta_y, theta_z;

  // Scaling/stretching in the x, y, and z directions
  int8_t scale_x, scale_y, scale_z;

  // Pointer to an array of points that make up polygons in 2D or triangles that make up the triangle
  // mesh for 3D rendering (Either Points_t or Triangle_t)
  union {
    uint16_t num_triangles;
    uint16_t num_points;
  };
  union {
    vga_triangle_t ** triangles;
    int16_t ** points;
  };

  // Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
  void (*animate)(struct vga_render_item_t *);
} vga_render_item_t;

/************************************
 * EXTERN VARIABLES
 ************************************/

// TODO: Find a better place for this so it isn't exposed to the user
extern volatile uint8_t framebuffer[PV_FRAMEBUFFER_BYTES];

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

int vga_init(vga_config_t * config);
int vga_deinit(vga_config_t * config);

uint16_t vga_get_width();
uint16_t vga_get_height();
uint16_t vga_get_width_full();
uint16_t vga_get_height_full();

void updateFullDisplay();

RenderQueueUID_t drawPixel(uint16_t x, uint16_t y, uint8_t color);
RenderQueueUID_t drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color, uint8_t thickness);
RenderQueueUID_t drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t thickness, uint8_t color);
RenderQueueUID_t drawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t thickness, uint8_t color);
RenderQueueUID_t drawCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t thickness, uint8_t color);
RenderQueueUID_t drawPolygon(uint16_t points[][2], uint8_t numPoints, uint8_t thickness, uint8_t color);

RenderQueueUID_t drawFilledRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t color);
RenderQueueUID_t drawFilledTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, uint8_t color);
RenderQueueUID_t drawFilledCircle(uint16_t x, uint16_t y, uint16_t radius, uint8_t color);
RenderQueueUID_t fillPolygon(uint16_t points[][2], uint8_t numPoints, uint8_t color);
RenderQueueUID_t fillScreen(uint8_t * obj, uint8_t color);

RenderQueueUID_t drawText(uint16_t x1, uint16_t y, uint16_t x2, char * str, uint8_t color, uint16_t bgColor, bool wrap, uint8_t strSizeOverrideBytes);
void setTextFont(uint8_t * newFont);

RenderQueueUID_t drawSprite(uint8_t * sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, int8_t scale_x, int8_t scale_y);

void clearScreen();

void setHidden(RenderQueueUID_t itemUID, bool hidden);
void removeItem(RenderQueueUID_t itemUID);

#endif