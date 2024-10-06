#include "render.h"

#include "vga.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

// TODO: Add interpolation back in
// volatile uint8_t interpolatedLine        = 0;
// volatile uint8_t interpolationIncomplete = 0;

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

static volatile bool update = 0;

/************************************
 * STATIC FUNCTIONS
 ************************************/

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

/**
 * @brief The renderer! Loops over the render queue waiting for something to update (or to be told
 * to update the framebuffer manually) and then draws it out to the display.
 *
 */
void render() {
  const vga_config_t * config = vga_get_config();
  vga_render_item_t * rq      = config->render_queue;
  uint16_t rq_len             = config->render_queue_len;

  int i = 0;

  while (true) {
    if (config->auto_render) {
      // look for the first item that needs an update, render that item and everything after it
      i = 0;
      while (!rq[i].header.flags.update && !update) {
        i = (i + 1) % rq_len;
      }
      // if the update is to hide an item or a force-refresh, rerender the whole thing
      if (!rq[i].header.flags.shown || update) {
        i = 0;
      }
    } else { // manual rendering
      while (!update);
      i = 0;
    }

    render2d_fill(COLOR_BLACK); // wipe the display
    for (; i < rq_len; i++) {
      if (rq[i].header.flags.shown) {
        switch (rq[i].header.type) {
          case VGA_RENDER_ITEM_FILL:
            render2d_fill(rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_PIXEL:
            render_pixel(rq[i].item_2d.y, rq[i].item_2d.x, rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_LINE:
            render2d_line(rq[i].item_2d.point.x[0], rq[i].item_2d.point.y[0], rq[i].item_2d.point.x[1], rq[i].item_2d.point.y[1], rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_RECTANGLE:
            render2d_rectangle(rq[i].item_2d.point.x[0], rq[i].item_2d.point.y[0], rq[i].item_2d.point.x[1], rq[i].item_2d.point.y[1], rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_FILLED_RECTANGLE:
            render2d_rectangle_filled(rq[i].item_2d.point.x[0], rq[i].item_2d.point.y[0], rq[i].item_2d.point.x[1], rq[i].item_2d.point.y[1], rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_TRIANGLE:
            render2d_triangle(rq[i].item_2d.point.x[0], rq[i].item_2d.point.y[0], rq[i].item_2d.point.x[1], rq[i].item_2d.point.y[1], rq[i].item_2d.point.x[2], rq[i].item_2d.point.y[2], rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_FILLED_TRIANGLE:
            render2d_triangle_filled(rq[i].item_2d.point.x[0], rq[i].item_2d.point.y[0], rq[i].item_2d.point.x[1], rq[i].item_2d.point.y[1], rq[i].item_2d.point.x[2], rq[i].item_2d.point.y[2], rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_CIRCLE:
            render2d_circle(rq[i].item_2d.x, rq[i].item_2d.y, rq[i].item_2d.point.x[0], rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_FILLED_CIRCLE:
            render2d_circle_filled(rq[i].item_2d.x, rq[i].item_2d.y, rq[i].item_2d.point.x[0], rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_STRING:
            render2d_string(rq[i].item_2d.str.str, rq[i].item_2d.x, rq[i].item_2d.y, rq[i].item_2d.str.x2, rq[i].header.flags.wordwrap, rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_SPRITE:
            render2d_sprite(rq[i].item_2d.sprite.sprite, rq[i].item_2d.x, rq[i].item_2d.y, rq[i].item_2d.sprite.size_x, rq[i].item_2d.sprite.size_y, rq[i].item_2d.sprite.null_color);
            break;
          case VGA_RENDER_ITEM_BITMAP:
            break;
          case VGA_RENDER_ITEM_POLYGON:
            render2d_polygon(rq[i].item_2d.points_arr.points, rq[i].item_2d.points_arr.num_points, rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_FILLED_POLYGON:
            render2d_polygon_filled(rq[i].item_2d.points_arr.points, rq[i].item_2d.points_arr.num_points, rq[i].item_2d.color);
            break;
          case VGA_RENDER_ITEM_LIGHT:
            break;
          case VGA_RENDER_ITEM_SVG:
            break;
          case VGA_RENDER_ITEM_MAX:
          default:
            break;
        }
      }

      if (rq[i].animate) {
        rq[i].animate((struct vga_render_item_t *) &rq[i]);
      }

      rq[i].header.flags.update = false;
    }

    update = false;
  }
}

/**
 * @brief Writes the color to the pixel at x, y. Handles out-of-bounds coordinates.
 *
 * @param y Y coordinate in screen space
 * @param x X coordinate in screen space
 * @param color Color to write
 */
void render_pixel(uint16_t y, uint16_t x, vga_color_t color) {
  if (x >= vga_get_width() || y >= vga_get_height())
    return;

  // Write out to the screen, but also handle line doubling.
  // Line doubling (for scaled resolutions) is done by writing the same
  // pointer to frame_read_addr 2 (4, 8) times in a row. This means that any
  // data written to that pointer will automatically be duped to the line(s)
  // below it. Pixel doubling is handled by clocking the color PIO slower.
  // i.e. line 2 on a 400x300 scaled display -> frame_read_addr[4] at base 800x600 resolution
  __vga_get_frame_read_addr()[y * vga_get_config()->scaled_resolution][x] = color;
}

uint8_t * render_get_pixel_ptr(uint16_t y, uint16_t x) {
  return &(__vga_get_frame_read_addr()[y][x]);
}

void vga_refresh() {
  update = true;
}

/*
        Line Interpolation
==================================
*/

// TODO: Add this back in
/*
volatile uint8_t lineInterpolationIRQ = 0;

static irq_handler_t lineInterpolationHandler() {
  irq_clear(lineInterpolationIRQ);
}

void initLineInterpolation() {
  lineInterpolationIRQ = user_irq_claim_unused(true);
  irq_set_priority(lineInterpolationIRQ, 255); // set to lowest priority, might change later based on testing
  irq_set_exclusive_handler(lineInterpolationIRQ, (irq_handler_t) lineInterpolationHandler);
  irq_set_enabled(31, true);
}

void deInitLineInterpolation() {
  irq_set_enabled(31, false);
  user_irq_unclaim(lineInterpolationIRQ);
}*/