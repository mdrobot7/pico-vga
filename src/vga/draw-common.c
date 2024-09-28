#include "pico/assert.h"
#include "pico/stdlib.h"
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

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

// Marks all items as shown, clears the screen to black (2d or 3d)
// vga_render_item_ts aren't actually cleared since they don't have to be. They'll get overwritten
void draw_clear() {
  const vga_config_t * config = vga_get_config();
  for (int i = 0; i < config->render_queue_len; i++) {
    config->render_queue[i].header.flags.shown = false;
  }
  vga_refresh();
}

// Set item to be shown (true = shown, false = hidden)
void draw_set_shown(vga_render_item_t * item, bool shown) {
  assert(item);

  item->header.flags.shown = shown;
  item->header.flags.update = true;
  // update screen
}