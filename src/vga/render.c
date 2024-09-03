#include "render.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/time.h"

static volatile uint8_t update           = 0;
volatile uint8_t interpolatedLine        = 0;
volatile uint8_t interpolationIncomplete = 0;

/**
 * @brief Force-refresh the display.
 *
 */
void updateDisplay() {
  update = 1;
  if (displayConfig->auto_render) ((vga_render_item_t *) renderQueueStart)->flags |= RQI_UPDATE; // force-update in auto_render mode
}


/**
 * @brief The renderer! Loops over the render queue waiting for something to update (or to be told
 * to update the framebuffer manually) and then draws it out to the display.
 *
 */
void render() {
  multicore_fifo_push_blocking(13); // tell core 0 that everything is ok/it's running

  vga_render_item_t * item;

  while (1) {
    if (displayConfig->auto_render) {
      item = (vga_render_item_t *) renderQueueStart;
      // look for the first item that needs an update, render that item and everything after it
      while (!(item->flags & RQI_UPDATE)) {
        if (item >= lastItem) item = (vga_render_item_t *) renderQueueStart;
      }
      // if the update is to hide or remove an item, rerender the whole thing
      if (item->flags & RQI_HIDDEN || item->uid == RQI_UID_REMOVED) item = (vga_render_item_t *) renderQueueStart;
    } else { // manual rendering
      while (!update);
      item = (vga_render_item_t *) renderQueueStart;
    }

    while (item <= lastItem) {
      if (!(item->flags & RQI_HIDDEN) && item->uid == RQI_UID_REMOVED) {
        switch (item->type) {
          case RQI_T_PIXEL:
            writeBufferedPixel(item->y, item->x, item->thetaZ);
            break;
          case RQI_T_LINE:
            Points_t * pts = (Points_t *) item->points_or_triangles;
            renderLine(pts->points[0], pts->points[1], pts->points[2], pts->points[3], item->thetaZ, item->scale_z);
            break;
          case RQI_T_RECTANGLE:
            break;
          case RQI_T_TRIANGLE:
            Points_t * pts = (Points_t *) item->points_or_triangles;
            renderTriangle(pts->points[0], pts->points[1], pts->points[2], pts->points[3], pts->points[4], pts->points[5], item->thetaZ, item->scale_z);
            break;
          case RQI_T_CIRCLE:
            renderCircle(item->x, item->y, item->z, item->thetaZ);
            break;
          case RQI_T_FILLED_RECTANGLE:
            break;
          case RQI_T_FILLED_TRIANGLE:
            Points_t * pts = (Points_t *) item->points_or_triangles;
            renderFilledTriangle(pts->points[0], pts->points[1], pts->points[2], pts->points[3], pts->points[4], pts->points[5], item->thetaZ);
            break;
          case RQI_T_FILLED_CIRCLE:
            renderFilledCircle(item->x, item->y, item->z, item->thetaZ);
            break;
          case RQI_T_STRING:
            /*//x1, y = top left corner, x2 = right side (for word wrap), thetaZ = color, scale_z = background
            uint16_t x1 = ((Points_t *)lastItem->points_or_triangles)->points[0]; //Fetch points from the memory space right after item
            uint16_t y = ((Points_t *)lastItem->points_or_triangles)->points[1];
            uint16_t x2 = ((Points_t *)lastItem->points_or_triangles)->points[2];
            char * str = &(((Points_t *)lastItem->points_or_triangles)->points[3]);

            for(uint16_t c = 0; str[c] != '\0'; c++) {
                for(uint8_t i = 0; i < CHAR_HEIGHT; i++) {
                    for(uint8_t j = 0; j < CHAR_WIDTH; j++) {
                        writePixel(y + i, x1 + j, getFontBit(str[c], i, j) ? item->thetaZ : item->scale_z);
                    }
                }
                x += CHAR_WIDTH + 1;
                if(item->flags & RQI_WORDWRAP && x + CHAR_WIDTH >= x2) {
                    x = item->x;
                    y += CHAR_HEIGHT;
                }
            }*/
            break;
          case RQI_T_SPRITE:
            /*for(uint16_t i = 0; i < item->y2; i++) {
                for(uint16_t j = 0; j < item->x2; j++) {
                    if(*(item->obj + i*item->x2 + j) != item->color) {
                        writePixel(item->y1 + i, item->x1 + j, *(item->obj + i*item->x2 + j));
                    }
                }
            }*/
            break;
          case RQI_T_FILL:
            for (uint16_t y = 0; y < frame_height; y++) {
              for (uint16_t x = 0; x < frame_width; x++) {
                writePixel(y, x, item->thetaZ);
              }
            }
            /*if(item->obj == NULL) {
                for(uint16_t i = 0; i < FRAME_WIDTH; i++) {
                    frame[l][i] = item->color;
                }
            }
            else {
                for(uint16_t i = 0; i < FRAME_WIDTH; i++) {
                    //Skip changing the pixel if it's set to the COLOR_NULL value
                    if(*(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i) == COLOR_NULL) continue;
                    //*(original mem location + (currentRowInArray * nColumns) + currentColumnInArray)
                    frame[l][i] = *(item->obj + ((currentLine - item->y1)*(item->x2 - item->x1)) + i);
                }
            }*/
            break;
        }
      }

      item->flags &= ~RQI_UPDATE; // Clear the update bit
      item++;
    }

    update = 0;
  }
}

/**
 * @brief The renderer, but with antialiasing! Splitting the functions like this saves CPU cycles and
 * makes the code neater, but it's entirely the same code except it calls renderThingAA() instead of the
 * standard render functions.
 *
 */
void renderAA() {
  return;
}

static void interpolate(uint16_t line) {
  return;
}


/*
        Line Interpolation
==================================
*/
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
}


/*
        Garbage Collector
=================================
*/
static volatile bool garbageCollectorActive = false;
static uint8_t garbageCollectorDMA          = 0;
struct repeating_timer garbageCollectorTimer;

static irq_handler_t garbageCollectorHandler() {
  dma_channel_acknowledge_irq1(garbageCollectorDMA);

  vga_render_item_t * item = findRenderQueueItem(RQI_UID_REMOVED);
  if (item >= lastItem) {
    garbageCollectorActive = false;
    return NULL; // No garbage collecting needs to be done
  }

  dma_channel_set_read_addr(garbageCollectorDMA, (item + item->num_points_or_triangles + 1), false);
  dma_channel_set_write_addr(garbageCollectorDMA, item, false);
  dma_channel_set_trans_count(garbageCollectorDMA, ((uint32_t) lastItem - (uint32_t) (dma_hw->ch[garbageCollectorDMA].read_addr)) / 4, true);
}

bool garbageCollector(struct repeating_timer * t) {
  if (garbageCollectorActive) return true; // Don't start it again if it hasn't finished already
  garbageCollectorActive = true;
  garbageCollectorHandler();
  return true;
}

void initGarbageCollector() {
  garbageCollectorDMA = dma_claim_unused_channel(true);

  // Default channel config is **NOT** the same as the register reset values. See pg 106 of C SDK docs for info.
  dma_channel_config garbage_collector_config = dma_channel_get_default_config(garbageCollectorDMA);
  channel_config_set_write_increment(&garbage_collector_config, true);
  dma_channel_set_config(garbageCollectorDMA, &garbage_collector_config, false);

  dma_channel_set_irq1_enabled(garbageCollectorDMA, true);
  irq_set_exclusive_handler(DMA_IRQ_1, (irq_handler_t) garbageCollectorHandler);
  irq_set_priority(DMA_IRQ_1, 32);
  irq_set_enabled(DMA_IRQ_1, true);

  // Set the garbage collector to run every n ms
  add_repeating_timer_ms(5, garbageCollector, NULL, &garbageCollectorTimer);
}

void deInitGarbageCollector() {
  cancel_repeating_timer(&garbageCollectorTimer);

  irq_set_enabled(DMA_IRQ_1, false); // Disable IRQ first, see pg 93 of C SDK docs
  dma_channel_abort(garbageCollectorDMA);

  dma_channel_unclaim(garbageCollectorDMA);
}