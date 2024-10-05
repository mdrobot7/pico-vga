#include "vga.h"

#include "../pinout.h"
#include "color.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "render.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

#define SECOND_CORE_MAGIC (13)

// LSBs for DMA CTRL register bits (pico's SDK constants weren't great)
#define SDK_DMA_CTRL_EN            0
#define SDK_DMA_CTRL_HIGH_PRIORITY 1
#define SDK_DMA_CTRL_DATA_SIZE     2
#define SDK_DMA_CTRL_INCR_READ     4
#define SDK_DMA_CTRL_INCR_WRITE    5
#define SDK_DMA_CTRL_RING_SIZE     6
#define SDK_DMA_CTRL_RING_SEL      10
#define SDK_DMA_CTRL_CHAIN_TO      11
#define SDK_DMA_CTRL_TREQ_SEL      15
#define SDK_DMA_CTRL_IRQ_QUIET     21
#define SDK_DMA_CTRL_BSWAP         22
#define SDK_DMA_CTRL_SNIFF_EN      23
#define SDK_DMA_CTRL_BUSY          24

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/

static volatile vga_config_t * vga_config; // Can't pass params to irq handler, so we're doing this

enum {
  FRAME_WIDTH_IDX = 0,
  FRAME_HEIGHT_IDX,
  FRAME_WIDTH_FULL_IDX,
  FRAME_HEIGHT_FULL_IDX,
};
// Widths and heights for the three base resolutions
static const uint16_t frame_size[3][4] = { { 800, 600, 1056, 628 },
                                           { 640, 480, 800, 525 },
                                           { 1024, 768, 1344, 806 } };
#define LARGEST_FRAME_WIDTH       (768)
#define LARGEST_FRAME_FULL_HEIGHT (1344)

static uint16_t frame_width       = 0;
static uint16_t frame_height      = 0;
static uint16_t frame_width_full  = 0;
static uint16_t frame_height_full = 0;

static uint8_t frame_ctrl_dma = 0;
static uint8_t frame_data_dma = 0;
static uint8_t blank_data_dma = 0;

static uint8_t color_pio_sm = 0;

static volatile uint8_t framebuffer[PV_FRAMEBUFFER_BYTES];
static const volatile uint8_t blank[LARGEST_FRAME_WIDTH]             = { 0 }; // ~0.7kB
static volatile uint8_t * frame_read_addr[LARGEST_FRAME_FULL_HEIGHT] = { 0 }; // ~5.2kB

/************************************
 * STATIC FUNCTIONS
 ************************************/

// DMA Interrupt Callback
static void update_frame_ptr() {
  dma_channel_acknowledge_irq0(frame_ctrl_dma);

  // TODO: Add this back in
  // Grab the element in frameReadAddr that frame_ctrl_dma just wrote (hence the -1) and see if it was an
  // interpolated line. If so, incrememt the interpolated line counter and flag the renderer so it can start
  // re-rendering it.
  // if (config->interpolated_mode && (((uint8_t *) dma_hw->ch[frame_ctrl_dma].read_addr) - 1) < frameBufferStart && (((uint8_t *) dma_hw->ch[frame_ctrl_dma].read_addr) - 1) >= (blankLineStart + frame_width)) {
  //   interpolatedLine = (interpolatedLine + 1) % config->num_interpolated_lines;
  //   irq_set_pending(lineInterpolationIRQ);
  // }

  // If the DMA read "cursor" is past the end of the frame data, reset it to the beginning
  if (dma_hw->ch[frame_ctrl_dma].read_addr >= (io_rw_32) &frame_read_addr[frame_height_full * vga_config->scaled_resolution]) {
    dma_hw->ch[frame_ctrl_dma].read_addr = (io_rw_32) frame_read_addr;
  }
}

static void dma_init(vga_config_t * config) {
  frame_ctrl_dma = dma_claim_unused_channel(true);
  frame_data_dma = dma_claim_unused_channel(true);
  blank_data_dma = dma_claim_unused_channel(true);

  // Default channel config is **NOT** the same as the register reset values. See pg 106 of C SDK docs for info.

  // &frame_read_addr[i] -> frame_data_dma.read_addr (tell frame_data_dma where to read from)
  dma_channel_config frame_ctrl_config = dma_channel_get_default_config(frame_ctrl_dma);
  channel_config_set_high_priority(&frame_ctrl_config, true);
  channel_config_set_transfer_data_size(&frame_ctrl_config, DMA_SIZE_32);
  dma_channel_configure(frame_ctrl_dma, &frame_ctrl_config, &dma_hw->ch[frame_data_dma].al3_read_addr_trig, frame_read_addr, 1, false);

  // frame_read_addr[i] -> pioX->txfifo (read from the address in frame_read_addr, send to PIO)
  dma_channel_config frame_data_config = dma_channel_get_default_config(frame_data_dma);
  channel_config_set_high_priority(&frame_data_config, true);
  channel_config_set_transfer_data_size(&frame_data_config, DMA_SIZE_32);
  channel_config_set_chain_to(&frame_data_config, blank_data_dma);
  channel_config_set_dreq(&frame_data_config, pio_get_dreq(config->pio, color_pio_sm, true));
  channel_config_set_irq_quiet(&frame_data_config, true);
  dma_channel_configure(frame_data_dma, &frame_data_config, &(config->pio)->txf[color_pio_sm], NULL, frame_width / 4, false);

  // 0x00 -> pioX->txfifo (send 0s to the PIO for blanking time)
  dma_channel_config blank_data_config = dma_channel_get_default_config(blank_data_dma);
  channel_config_set_high_priority(&blank_data_config, true);
  channel_config_set_transfer_data_size(&blank_data_config, DMA_SIZE_32);
  channel_config_set_read_increment(&blank_data_config, false); // Defaults to true
  channel_config_set_chain_to(&blank_data_config, frame_ctrl_dma);
  channel_config_set_dreq(&blank_data_config, pio_get_dreq(config->pio, color_pio_sm, true));
  channel_config_set_irq_quiet(&blank_data_config, true);
  dma_channel_configure(blank_data_dma, &blank_data_config, &(config->pio)->txf[color_pio_sm], blank, (frame_width_full - frame_width) / 4, false);

  dma_channel_set_irq0_enabled(frame_ctrl_dma, true);
  irq_set_priority(DMA_IRQ_0, 0); // Set the DMA interrupt to the highest priority

  irq_set_exclusive_handler(DMA_IRQ_0, (irq_handler_t) update_frame_ptr);
  irq_set_enabled(DMA_IRQ_0, true);

  dma_channel_start(frame_ctrl_dma); // Start!
}

static void second_core_init() {
  multicore_fifo_push_blocking(SECOND_CORE_MAGIC);

  if (vga_config->antialiasing) {
    render(); // TODO: add antialiasing support
  } else {
    render();
  }
}

/************************************
 * GLOBAL FUNCTIONS
 ************************************/

int vga_init(vga_config_t * config) {
  clocks_init();
  vga_config        = config;
  frame_width       = frame_size[config->base_resolution][FRAME_WIDTH_IDX] / config->scaled_resolution;
  frame_height      = frame_size[config->base_resolution][FRAME_HEIGHT_IDX] / config->scaled_resolution;
  frame_width_full  = frame_size[config->base_resolution][FRAME_WIDTH_FULL_IDX] / config->scaled_resolution;
  frame_height_full = frame_size[config->base_resolution][FRAME_HEIGHT_FULL_IDX] / config->scaled_resolution;

  for (int i = 0; i < 120000; i++) {
    framebuffer[i] = COLOR_YELLOW;
  }

  // Override if there are less than 2 interpolated lines
  if (config->num_interpolated_lines < 2) config->num_interpolated_lines = 2;

  color_pio_sm = pio_claim_unused_sm(config->pio, true);

  // Add PIO program to PIO instruction memory. SDK will find location and
  // return with the memory offset of the program.
  // uint offset = pio_add_program(config->pio, &color_program);

  gpio_set_function(HSYNC_PIN, GPIO_FUNC_PWM);
  gpio_set_function(VSYNC_PIN, GPIO_FUNC_PWM);
  pwm_config default_pwm_conf = pwm_get_default_config();
  pwm_init(HSYNC_PWM_SLICE, &default_pwm_conf, false); // reset to known state
  pwm_init(VSYNC_PWM_SLICE, &default_pwm_conf, false);

  if (config->base_resolution == RES_800x600) {
    // 120MHz system clock frequency (multiple of 40MHz pixel clock)
    set_sys_clock_pll(1440000000, 6, 2); // VCO frequency (MHz), PD1, PD2 -- see vcocalc.py

    // Initialize color pio program, but DON'T enable PIO state machine
    // CPU Base clock (after reconfig): 120.0 MHz. 120/3 = 40Mhz pixel clock
    uint offset = pio_add_program(config->pio, &color_program);
    color_program_init(config->pio, color_pio_sm, offset, COLOR_LSB_PIN, 3 * config->scaled_resolution);

    pwm_set_clkdiv(HSYNC_PWM_SLICE, 3.0);                     // pixel clock
    pwm_set_wrap(HSYNC_PWM_SLICE, 1056 - 1);                  // full line width - 1
    pwm_set_counter(HSYNC_PWM_SLICE, 128 + 88);               // sync pulse + back porch
    pwm_set_chan_level(HSYNC_PWM_SLICE, HSYNC_PWM_CHAN, 128); // sync pulse

    pwm_set_clkdiv(VSYNC_PWM_SLICE, 198.0);                      // clkdiv maxes out at /256, so this (pixel clock * full line width)/16 = (3*1056)/16, dividing by 16 to compensate
    pwm_set_wrap(VSYNC_PWM_SLICE, (628 * 16) - 1);               // full frame height, adjusted for clkdiv fix
    pwm_set_counter(VSYNC_PWM_SLICE, (4 + 23) * 16);             // sync pulse + back porch
    pwm_set_chan_level(VSYNC_PWM_SLICE, VSYNC_PWM_CHAN, 4 * 16); // back porch
  } else if (config->base_resolution == RES_640x480) {
    // 125MHz system clock frequency -- possibly bump to 150MHz later
    set_sys_clock_pll(1500000000, 6, 2);

    uint offset = pio_add_program(config->pio, &color_program);
    color_program_init(config->pio, color_pio_sm, offset, COLOR_LSB_PIN, 5 * config->scaled_resolution);

    // Sources for these timings, because they are slightly different (for a 25MHz pixel clock, not 25.175MHz)
    // https://www.eevblog.com/forum/microcontrollers/implementing-a-vga-controller-on-spartan-3-board-with-25-0-mhz-clock/
    // pg 24: https://docs.xilinx.com/v/u/en-US/ug130

    pwm_set_output_polarity(HSYNC_PWM_SLICE, !HSYNC_PWM_CHAN ? true : false, HSYNC_PWM_CHAN ? true : false);
    pwm_set_clkdiv(HSYNC_PWM_SLICE, 5.0);
    pwm_set_wrap(HSYNC_PWM_SLICE, 800 - 1);
    pwm_set_counter(HSYNC_PWM_SLICE, 96 + 48);
    pwm_set_chan_level(HSYNC_PWM_SLICE, HSYNC_PWM_CHAN, 96);

    pwm_set_output_polarity(VSYNC_PWM_SLICE, !VSYNC_PWM_CHAN ? true : false, VSYNC_PWM_CHAN ? true : false);
    pwm_set_clkdiv(VSYNC_PWM_SLICE, 250.0); // see above for reasoning behind this -- (5*800)/16
    pwm_set_wrap(VSYNC_PWM_SLICE, (521 * 16) - 1);
    pwm_set_counter(VSYNC_PWM_SLICE, (2 + 29) * 16);
    pwm_set_chan_level(VSYNC_PWM_SLICE, VSYNC_PWM_CHAN, 2 * 16);
  } else if (config->base_resolution == RES_1024x768) {
    // 150MHz system clock frequency
    set_sys_clock_pll(1500000000, 5, 2);

    uint offset = pio_add_program(config->pio, &color_program);
    color_program_init(config->pio, color_pio_sm, offset, COLOR_LSB_PIN, 2 * config->scaled_resolution);

    pwm_set_output_polarity(HSYNC_PWM_SLICE, !HSYNC_PWM_CHAN ? true : false, HSYNC_PWM_CHAN ? true : false);
    pwm_set_clkdiv(HSYNC_PWM_SLICE, 2.0);
    pwm_set_wrap(HSYNC_PWM_SLICE, 1328 - 1);
    pwm_set_counter(HSYNC_PWM_SLICE, 136 + 144);
    pwm_set_chan_level(HSYNC_PWM_SLICE, HSYNC_PWM_CHAN, 136);

    pwm_set_output_polarity(VSYNC_PWM_SLICE, !VSYNC_PWM_CHAN ? true : false, VSYNC_PWM_CHAN ? true : false);
    pwm_set_clkdiv(VSYNC_PWM_SLICE, 166.0); // see above for reasoning behind this -- (2*800)/16
    pwm_set_wrap(VSYNC_PWM_SLICE, (806 * 16) - 1);
    pwm_set_counter(VSYNC_PWM_SLICE, (6 + 29) * 16);
    pwm_set_chan_level(VSYNC_PWM_SLICE, VSYNC_PWM_CHAN, 6 * 16);
  }

  // Fail if the frame buffer is too small
  if (PV_FRAMEBUFFER_BYTES < frame_width * 5) return 1;

  // uint32_t num_buffered_lines = PV_FRAMEBUFFER_BYTES / frame_width;

  // TODO: Add interpolation back
  /*
  if (num_buffered_lines >= frame_height) {
  } else {
    frameBufferStart      = frameBufferEnd - num_buffered_lines * frame_width;
    interpolatedLineStart = frameBufferStart - config->num_interpolated_lines * frame_width;
    blankLineStart        = interpolatedLineStart - frame_width;
  }

  if (num_buffered_lines != frame_height) {
    /*
        Even distribution algorithm:
    - Finds the distance between each of the interpolated lines (segmentLen)
    - There will always be spares, since the number of interpolated lines will almost never perfectly
      divide into the number of total lines. This is numSpares.
    - Instead of putting an interpolated line every segmentLen and putting all of the spares at the end
      (making the CPU work harder in the beginning of the frame) this adds 1 element to numSpares
      number of segments.
    - This algorithm interpolates a line every (segmentLen + 1) lines until it runs out of spares, then
      interpolates every segmentLen lines.
    - This reddit post explains it: https://www.reddit.com/r/learnprogramming/comments/31ns44/splitting_the_contents_of_an_array_into_as_evenly/
    - i = line in frame, j = buffer line counter, k = interpolated line counter, l = frameReadAddr index counter
    */
  /*
uint16_t segmentLen = frame_height / (frame_height - num_buffered_lines);
uint16_t numSpares  = frame_height % (frame_height - num_buffered_lines);
for (int i = 0, j = 0, k = 0; i < frame_height; i++) {
  if (numSpares != 0 && i % (segmentLen + 1) == 0) {
    for (int l = 0; l < config->scaled_resolution; l++) {
      ((uint8_t **) frameReadAddrStart)[(i * config->scaled_resolution) + l] = interpolatedLineStart + k * frame_width;
    }
    k = (k + 1) % config->num_interpolated_lines;
    numSpares--;
  } else if (numSpares == 0 && i % segmentLen == 0) {
    for (int l = 0; l < config->scaled_resolution; l++) {
      ((uint8_t **) frameReadAddrStart)[(i * config->scaled_resolution) + l] = interpolatedLineStart + k * frame_width;
    }
    k = (k + 1) % config->num_interpolated_lines;
  } else {
    // If this line isn't interpolated, grab the next buffered line from the framebuffer
    for (int l = 0; l < config->scaled_resolution; l++) {
      ((uint8_t **) frameReadAddrStart)[(i * config->scaled_resolution) + l] = (uint8_t *) (frameBufferStart + frame_width * j);
    }
    j++;
  }
}
} else {
// No interpolation
}
*/

  for (int i = 0; i < frame_height * config->scaled_resolution; i++) {
    frame_read_addr[i] = framebuffer + frame_width * (i / config->scaled_resolution);
  }

  // Fill in blanking time at the bottom of the screen
  for (int i = frame_height * config->scaled_resolution; i < frame_height_full * config->scaled_resolution; i++) {
    frame_read_addr[i] = blank;
  }
  // Hacky fix: Since the true height of 640x480 is 525 lines, integer division becomes an issue. This is the fix
  if (config->base_resolution == RES_640x480) {
    frame_read_addr[524] = blank;
  }

  dma_init(vga_config); // Must be run here so the IRQ runs on the second core

  pio_enable_sm_mask_in_sync(vga_config->pio, 1u << color_pio_sm); // start color state machine and clock
  pwm_set_enabled(HSYNC_PWM_SLICE, true);                          // start hsync and vsync signals
  pwm_set_enabled(VSYNC_PWM_SLICE, true);

  multicore_launch_core1(second_core_init);
  while (multicore_fifo_pop_blocking() != SECOND_CORE_MAGIC); // busy wait while the core is initializing
  return 0;
}

int vga_deinit(vga_config_t * config) {
  multicore_reset_core1(); // Stop core 1

  irq_set_enabled(DMA_IRQ_0, false); // Disable IRQ first, see pg 93 of C SDK docs
  dma_channel_abort(frame_ctrl_dma);
  dma_channel_abort(frame_data_dma);
  dma_channel_abort(blank_data_dma);

  dma_unclaim_mask((1 << frame_ctrl_dma) | (1 << frame_data_dma) | (1 << blank_data_dma));

  pio_sm_set_enabled(config->pio, color_pio_sm, false);
  pio_sm_clear_fifos(config->pio, color_pio_sm);

  pwm_set_enabled(HSYNC_PWM_SLICE, false);
  pwm_set_enabled(VSYNC_PWM_SLICE, false);

  return 0;
}

const vga_config_t * vga_get_config() {
  return (const vga_config_t *) vga_config; // Force read-only
}

uint16_t vga_get_width() {
  return frame_width;
}

uint16_t vga_get_height() {
  return frame_height;
}

uint16_t vga_get_width_full() {
  return frame_width_full;
}

uint16_t vga_get_height_full() {
  return frame_height_full;
}

uint8_t ** __vga_get_frame_read_addr() {
  return (uint8_t **) frame_read_addr; // remove volatile qualifier
}