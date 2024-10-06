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

void draw_clear() {
  const vga_config_t * config = vga_get_config();
  for (int i = 0; i < config->render_queue_len; i++) {
    config->render_queue[i].header.flags.shown = false;
  }
  vga_refresh();
}

void draw_set_shown(vga_render_item_t * item, bool shown) {
  assert(item);

  item->header.flags.shown  = shown;
  item->header.flags.update = true;
}