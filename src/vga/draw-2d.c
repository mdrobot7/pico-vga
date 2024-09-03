#include "../common.h"
#include "font.h"
#include "pico/assert.h"
#include "vga.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

volatile uint8_t * font = (uint8_t *) cp437; // The current font in use by the system

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

// Reset any flags, trigger it to be rendered
static void clear_and_activate_item(vga_render_item_t * item) {
  item->header.flags_byte = 0;
  clear_and_activate_item(item);
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

void draw2d_set_scale(vga_render_item_t * item, uint8_t scale_x, uint8_t scale_y) {
  invalid_params_if(item, NULL);

  item->item_2d.scale_x     = scale_x;
  item->item_2d.scale_y     = scale_y;
  item->header.flags.update = true;
}

void draw2d_set_rotation(vga_render_item_t * item, int8_t theta) {
  invalid_params_if(item, NULL);

  item->item_2d.theta       = theta;
  item->header.flags.update = true;
}

void draw2d_set_color(vga_render_item_t * item, vga_color_t color) {
  invalid_params_if(item, NULL);

  item->item_2d.color       = color;
  item->header.flags.update = true;
}


void draw2d_pixel(vga_render_item_t * item, uint16_t x, uint16_t y, vga_color_t color) {
  invalid_params_if(item, NULL);

  item->header.type   = VGA_RENDER_ITEM_PIXEL;
  item->item_2d.x     = x;
  item->item_2d.y     = y;
  item->item_2d.color = color;

  clear_and_activate_item(item);
}

// TODO: Add line thickness
void draw2d_line(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
  invalid_params_if(item, NULL);

  item->header.type        = VGA_RENDER_ITEM_LINE;
  item->item_2d.x          = AVG(x1, x2); // Center point
  item->item_2d.y          = AVG(y1, y2);
  item->item_2d.color      = color;
  item->item_2d.point.x[0] = x1;
  item->item_2d.point.y[0] = y1;
  item->item_2d.point.x[1] = x2;
  item->item_2d.point.y[1] = y2;

  clear_and_activate_item(item);
}

// TODO: Add line thickness
void draw2d_rectangle(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
  invalid_params_if(item, NULL);

  item->header.type        = VGA_RENDER_ITEM_RECTANGLE;
  item->item_2d.x          = AVG(x1, x2); // Center point
  item->item_2d.y          = AVG(y1, y2);
  item->item_2d.color      = color;
  item->item_2d.point.x[0] = x1; // Diagonal Corners
  item->item_2d.point.y[0] = y1;
  item->item_2d.point.x[1] = x2;
  item->item_2d.point.y[1] = y2;

  clear_and_activate_item(item);
}

// TODO: Add line thickness
void draw2d_triangle(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color) {
  invalid_params_if(item, NULL);

  item->header.type        = VGA_RENDER_ITEM_TRIANGLE;
  item->item_2d.x          = (x1 + x2 + x3) / 3; // Center point
  item->item_2d.y          = (y1 + y2 + y3) / 3;
  item->item_2d.color      = color;
  item->item_2d.point.x[0] = x1; // Corners
  item->item_2d.point.y[0] = y1;
  item->item_2d.point.x[1] = x2;
  item->item_2d.point.y[1] = y2;
  item->item_2d.point.x[2] = x3;
  item->item_2d.point.y[2] = y3;

  clear_and_activate_item(item);
}

// TODO: Add line thickness
void draw2d_circle(vga_render_item_t * item, uint16_t x, uint16_t y, uint16_t radius, uint8_t color) {
  invalid_params_if(item, NULL);

  item->header.type        = VGA_RENDER_ITEM_CIRCLE;
  item->item_2d.x          = x; // Center point
  item->item_2d.y          = y;
  item->item_2d.color      = color;
  item->item_2d.point.x[0] = radius;
  item->item_2d.point.y[0] = radius;

  clear_and_activate_item(item);
}

// Draws lines between all points in the list. Points must be in clockwise order.
void draw2d_polygon(vga_render_item_t * item, uint16_t points[][2], uint16_t num_points, vga_color_t color) {
  invalid_params_if(item, NULL);

  item->header.type                   = VGA_RENDER_ITEM_POLYGON;
  item->item_2d.x                     = points[0][0]; // Just grab the first point, be lazy
  item->item_2d.y                     = points[0][1];
  item->item_2d.color                 = color;
  item->item_2d.points_arr.points     = points;
  item->item_2d.points_arr.num_points = num_points;

  clear_and_activate_item(item);
}

void draw2d_rectangle_filled(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color) {
  draw2d_rectangle(item, x1, y1, x2, y2, color);
  item->header.type = VGA_RENDER_ITEM_FILLED_RECTANGLE; // Mark it filled
  clear_and_activate_item(item);                        // Reactivate, just in case
}

void draw2d_triangle_filled(vga_render_item_t * item, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color) {
  draw2d_triangle(item, x1, y2, x2, y2, x3, y3, color);
  item->header.type = VGA_RENDER_ITEM_FILLED_TRIANGLE;
  clear_and_activate_item(item);
}

void draw2d_circle_filled(vga_render_item_t * item, uint16_t x, uint16_t y, uint16_t radius, vga_color_t color) {
  draw2d_circle(item, x, y, radius, color);
  item->header.type = VGA_RENDER_ITEM_FILLED_CIRCLE;
  clear_and_activate_item(item);
}

// Draws lines and fills between all points in the list. Points must be in clockwise order.
void draw2d_polygon_filled(vga_render_item_t * item, uint16_t points[][2], uint16_t num_points, vga_color_t color) {
  draw2d_polygon(item, points, num_points, color);
  item->header.type = VGA_RENDER_ITEM_FILLED_POLYGON;
  clear_and_activate_item(item);
}

void draw2d_fill(vga_render_item_t * item, vga_color_t color) {
  invalid_params_if(item, NULL);

  item->header.type   = VGA_RENDER_ITEM_FILL;
  item->item_2d.color = color;
  clear_and_activate_item(item);
}


/*
        Text Functions
==============================
*/

void draw2d_text(vga_render_item_t * item, uint16_t x1, uint16_t y, uint16_t x2, char * str, vga_color_t color, bool wrap) {
  invalid_params_if(item, NULL);

  item->header.type     = VGA_RENDER_ITEM_STRING;
  item->item_2d.x       = x1; // Grab the top left corner, be lazy
  item->item_2d.y       = y;
  item->item_2d.color   = color;
  item->item_2d.str.str = str;
  item->item_2d.str.x2  = x2;

  item->header.flags_byte     = 0;
  item->header.flags.wordwrap = wrap;
  item->header.flags.update   = true;
}

void draw2d_set_font(uint8_t * new_font[256][FONT_HEIGHT]) {
  invalid_params_if(new_font, NULL);

  font = new_font;
}


/*
        Sprite Drawing Functions
========================================
*/
void draw2d_sprite(vga_render_item_t * item, uint16_t x, uint16_t y, vga_color_t * sprite, uint16_t size_x, uint16_t size_y, vga_color_t null_color) {
  invalid_params_if(item, NULL);

  item->header.type               = VGA_RENDER_ITEM_SPRITE;
  item->item_2d.x                 = x; // Grab the top left corner, be lazy
  item->item_2d.y                 = y;
  item->item_2d.sprite.sprite     = sprite;
  item->item_2d.sprite.size_x     = size_x;
  item->item_2d.sprite.size_y     = size_y;
  item->item_2d.sprite.null_color = null_color;

  clear_and_activate_item(item);
}