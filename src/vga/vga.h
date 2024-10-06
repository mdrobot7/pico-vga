#ifndef __PV_DRAW_H
#define __PV_DRAW_H

#include "color.h"
#include "font.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

/************************************
 * MACROS AND TYPEDEFS
 ************************************/

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
 * RENDER QUEUE
 ************************************/

typedef enum {
  VGA_RENDER_ITEM_FILL,
  VGA_RENDER_ITEM_PIXEL,
  VGA_RENDER_ITEM_LINE,
  VGA_RENDER_ITEM_RECTANGLE,
  VGA_RENDER_ITEM_FILLED_RECTANGLE,
  VGA_RENDER_ITEM_TRIANGLE,
  VGA_RENDER_ITEM_FILLED_TRIANGLE,
  VGA_RENDER_ITEM_CIRCLE,
  VGA_RENDER_ITEM_FILLED_CIRCLE,
  VGA_RENDER_ITEM_STRING,
  VGA_RENDER_ITEM_SPRITE,
  VGA_RENDER_ITEM_BITMAP,
  VGA_RENDER_ITEM_POLYGON,
  VGA_RENDER_ITEM_FILLED_POLYGON,
  VGA_RENDER_ITEM_LIGHT,
  VGA_RENDER_ITEM_SVG,
  VGA_RENDER_ITEM_MAX = 255, // Ensure that a vga_render_item_type_t variable is 8 bits
} vga_render_item_type_t;

enum {
  POINT_X = 0,
  POINT_Y,
  POINT_Z,
};

typedef struct __packed {
  vga_render_item_type_t type : 8;

  // FLAGS
  union {
    struct __packed {
      uint32_t shown      : 1;
      uint32_t update     : 1;
      uint32_t wordwrap   : 1;
      uint32_t __reserved : 5;
    } flags;
    uint8_t flags_byte;
  };
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
  vga_color_t color;

  // Set to true to not shade this triangle.
  uint8_t hidden;
} vga_triangle_t;

struct __packed vga_render_item_t; // Predefinition, takes care of warnings later on from the function pointers
typedef struct __packed {
  vga_render_item_header_t header;
  union {
    struct __packed {
      // Center point (reference point for the entire object, center of rotation)
      uint16_t x, y;

      // Rotation -- negative value means negative rotation
      int8_t theta;

      // Scaling/stretching in the x and y directions
      uint8_t scale_x, scale_y;

      vga_color_t color;

      union {
        struct __packed {
          // Pointer to an array of points that make up polygons in 2D
          uint16_t num_points;
          uint16_t (*points)[2]; // Pointer to a [num_points][2] array
        } points_arr;
        struct __packed {
          // Small points array, covers anything up to triangle (larger polygons, just use points_arr.points above)
          uint16_t x[3], y[3];
        } point;
        struct __packed {
          char * str;
          uint16_t x2;
        } str;
        struct __packed {
          vga_color_t * sprite;
          uint16_t size_x;
          uint16_t size_y;
          vga_color_t null_color;
        } sprite;
      };
    } item_2d;
    struct __packed {
      int16_t x, y, z;
      int8_t theta_x, theta_y, theta_z;
      uint8_t scale_x, scale_y, scale_z;
      // Color is stored in each triangle in the mesh

      // Pointer to triangles that make up the triangle mesh for 3D rendering
      uint16_t num_triangles;
      vga_triangle_t * triangles;
    } item_3d;
  };

  // Pointer to a function that modifies the current RenderQueueItem to animate it. Called every frame (60Hz).
  void (*animate)(struct vga_render_item_t *);
} vga_render_item_t;

/************************************
 * CONFIGURATION
 ************************************/

typedef enum {
  RES_800x600 = 0, // @ 60Hz, 40MHz pixel clock
  RES_640x480,     // @ 60Hz, 25MHz pixel clock
  RES_1024x768,    // @ 60Hz, 65MHz pixel clock
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
  RES_SCALED_512x384  = 2,
} vga_resolution_scaled_t;

typedef struct {
  PIO pio; // Which PIO to use for color
  vga_resolution_base_t base_resolution;
  vga_resolution_scaled_t scaled_resolution;
  vga_render_item_t * render_queue;
  uint16_t render_queue_len;
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
 * EXTERN VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

/**
 * @brief Initialize the VGA module
 *
 * @param config VGA config struct
 * @return int 0 on success, nonzero otherwise
 */
int vga_init(vga_config_t * config);

/**
 * @brief Deinitialize the VGA module
 *
 * @param config VGA config struct
 * @return int 0 on success, nonzero otherwise
 */
int vga_deinit(vga_config_t * config);

/**
 * @brief Get the current VGA config
 *
 * @return const vga_config_t* READ-ONLY VGA config struct
 */
const vga_config_t * vga_get_config();

/**
 * @brief Get the screen width, in pixels (as currently configured)
 *
 * @return uint16_t Width
 */
uint16_t vga_get_width();

/**
 * @brief Get the screen height, in pixels (as currently configured)
 *
 * @return uint16_t Height
 */
uint16_t vga_get_height();

/**
 * @brief Get the full screen width, including blanking time,
 * in pixels (as currently configured)
 *
 * @return uint16_t Width
 */
uint16_t vga_get_width_full();

/**
 * @brief Get the full screen height, including blanking time,
 * in pixels (as currently configured)
 *
 * @return uint16_t Height
 */
uint16_t vga_get_height_full();

/**
 * @brief Force-refresh the display (auto-render mode or manual mode)
 *
 */
void vga_refresh();

/**
 * @brief Get the frame read address buffer
 *
 * @return uint8_t*
 */
uint8_t ** __vga_get_frame_read_addr();

/**
 * @brief Hide all items in the render queue.
 *
 */
void draw_clear();

/**
 * @brief Set an item shown or hidden.
 *
 * @param item The item to show/hide
 * @param shown True to show, false to hide
 */
void draw_set_shown(vga_render_item_t * item, bool shown);

/**
 * @brief Set an item's scale
 *
 * @param item The item to scale
 * @param scale_x
 * @param scale_y
 */
void draw2d_set_scale(vga_render_item_t * item, uint8_t scale_x, uint8_t scale_y);

/**
 * @brief Set an item's rotation
 *
 * @param item The item to rotate
 * @param theta Angle in ????
 */
void draw2d_set_rotation(vga_render_item_t * item, int8_t theta);

/**
 * @brief Set an item's color
 *
 * @param item The item to color
 * @param color
 */
void draw2d_set_color(vga_render_item_t * item, vga_color_t color);

/**
 * @brief Draw a single pixel
 *
 * @param item Render queue item to fill with data
 * @param x
 * @param y
 * @param color
 */
void draw2d_pixel(vga_render_item_t * item, uint16_t x, uint16_t y, vga_color_t color);

/**
 * @brief Draw a line
 *
 * @param item Render queue item to fill with data
 * @param x1
 * @param y1
 * @param x2
 * @param y2
 * @param color
 */
void draw2d_line(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color);

/**
 * @brief Draw a rectangle
 *
 * @param item Render queue item to fill with data
 * @param x1 Top left corner of the rectangle
 * @param y1 Top left corner of the rectangle
 * @param x2 Bottom right corner of the rectangle
 * @param y2 Bottom right corner of the rectangle
 * @param color
 */
void draw2d_rectangle(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color);

/**
 * @brief Draw a triangle
 *
 * @param item Render queue item to fill with data
 * @param x1 Vertex 1 of the triangle
 * @param y1 Vertex 1 of the triangle
 * @param x2 Vertex 2 of the triangle
 * @param y2 Vertex 2 of the triangle
 * @param x3 Vertex 3 of the triangle
 * @param y3 Vertex 3 of the triangle
 * @param color
 */
void draw2d_triangle(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color);

/**
 * @brief Draw a circle
 *
 * @param item Render queue item to fill with data
 * @param x Center of the circle
 * @param y Center of the circle
 * @param radius Radius of the circle, in pixels
 * @param color
 */
void draw2d_circle(vga_render_item_t * item, uint16_t x, uint16_t y, uint16_t radius, uint8_t color);

/**
 * @brief Draw an arbitrary polygon
 *
 * @param item Render queue item to fill with data
 * @param points List of coordinates for each vertex of the polygon (in clockwise order)
 * @param num_points Number of vertices/points passed
 * @param color
 */
void draw2d_polygon(vga_render_item_t * item, uint16_t points[][2], uint16_t num_points, vga_color_t color);

/**
 * @brief Draw a filled rectangle
 *
 * @param item Render queue item to fill with data
 * @param x1 Top left corner of the rectangle
 * @param y1 Top left corner of the rectangle
 * @param x2 Bottom right corner of the rectangle
 * @param y2 Bottom right corner of the rectangle
 * @param color
 */
void draw2d_rectangle_filled(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color);

/**
 * @brief Draw a filled triangle
 *
 * @param item Render queue item to fill with data
 * @param x1 Vertex 1 of the triangle
 * @param y1 Vertex 1 of the triangle
 * @param x2 Vertex 2 of the triangle
 * @param y2 Vertex 2 of the triangle
 * @param x3 Vertex 3 of the triangle
 * @param y3 Vertex 3 of the triangle
 * @param color
 */
void draw2d_triangle_filled(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color);

/**
 * @brief Draw a filled circle
 *
 * @param item Render queue item to fill with data
 * @param x Center of the circle
 * @param y Center of the circle
 * @param radius Radius of the circle, in pixels
 * @param color
 */
void draw2d_circle_filled(vga_render_item_t * item, uint16_t x, uint16_t y, uint16_t radius, vga_color_t color);

/**
 * @brief Draw a filled polygon
 *
 * @param item Render queue item to fill with data
 * @param points List of coordinates for each vertex of the polygon (in clockwise order)
 * @param num_points Number of vertices/points passed
 * @param color
 */
void draw2d_polygon_filled(vga_render_item_t * item, uint16_t points[][2], uint16_t num_points, vga_color_t color);

/**
 * @brief Fill the whole screen with a color
 *
 * @param item Render queue item to fill with data
 * @param color
 */
void draw2d_fill(vga_render_item_t * item, vga_color_t color);

/**
 * @brief Draw a text box using the system font
 *
 * @param item Render queue item to fill with data
 * @param x1 Left side of the text box
 * @param y Top side of the text box
 * @param x2 Right side of the text box
 * @param str String to draw
 * @param color
 * @param wrap True to enable word wrap, false to disable.
 * Will wrap at x2 and use as much vertical height as needed.
 */
void draw2d_text(vga_render_item_t * item, uint16_t x1, uint16_t y, uint16_t x2, char * str, vga_color_t color, bool wrap);

/**
 * @brief Set the system font
 *
 * @param new_font Pointer to the new font
 */
void draw2d_set_font(const uint8_t new_font[256][FONT_HEIGHT]);

/**
 * @brief Get the system font
 *
 * @return uint8_t* Pointer to the current system font, a 256*FONT_HEIGHT array
 */
const uint8_t * draw2d_get_font();

/**
 * @brief Draw a sprite (2d array of pixels)
 *
 * @param item Render queue item to fill with data
 * @param x Top left corner of the sprite
 * @param y Top left corner of the sprite
 * @param sprite Pointer to the sprite data
 * @param size_x Width of the sprite
 * @param size_y Height of the sprite
 * @param null_color "Transparent" color for the sprite.
 * If a pixel in the sprite is null_color, don't change it on
 * the screen (keep the background color).
 */
void draw2d_sprite(vga_render_item_t * item, uint16_t x, uint16_t y, vga_color_t * sprite, uint16_t size_x, uint16_t size_y, vga_color_t null_color);
#endif