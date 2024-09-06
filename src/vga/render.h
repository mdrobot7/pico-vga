#ifndef __PV_RENDER_H
#define __PV_RENDER_H

#include "vga.h"

void render_pixel(uint16_t y, uint16_t x, vga_color_t color);
uint8_t * render_get_pixel_ptr(uint16_t y, uint16_t x);

void render2d_fill(vga_color_t color);
void render2d_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color);
void render2d_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color);
void render2d_rectangle_filled(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, vga_color_t color);
void render2d_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color);
void render2d_triangle_filled(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t x3, uint16_t y3, vga_color_t color);
void render2d_circle(uint16_t x, uint16_t y, uint16_t radius, vga_color_t color);
void render2d_circle_filled(uint16_t x, uint16_t y, uint16_t radius, vga_color_t color);
void render2d_polygon(uint16_t points[][2], uint16_t num_points, vga_color_t color);
void render2d_polygon_filled(uint16_t points[][2], const uint16_t num_points, vga_color_t color);
void render2d_string(char * str, uint16_t x1, uint16_t y, uint16_t x2, bool wrap, vga_color_t color);
void render2d_sprite(vga_color_t * sprite, uint16_t x, uint16_t y, uint16_t size_x, uint16_t size_y, vga_color_t null_color);

#endif