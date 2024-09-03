#include "pico-vga.h"
#include "pico/stdlib.h"

vga_config_t displayConf = {
  .base_resolution          = RES_800x600,
  .scaled_resolution        = RES_SCALED_400x300,
  .auto_render              = true,
  .antialiasing             = false,
  .frameBufferSizeBytes     = 0xFFFFFFFF,
  .renderQueueSizeBytes     = 0,
  .num_interpolated_lines   = 0,
  .peripheralMode           = false,
  .clearRenderQueueOnDeInit = false,
  .color_delay_cycles       = 0
};
ControllerConfig_t controllerConf = {
  .numControllers = 4,
};

vga_render_item_t render_queue[100]; // The render queue

int main() {
  stdio_init_all();
  for (uint8_t i = 0; i < 20; i++) { // 5 seconds to open serial communication
    // printf("Waiting for user to open serial...\n");
    // sleep_ms(250);
  }
  // printf("\n");

  initPicoVGA(&displayConf, &controllerConf, &audioConf, &sdConf);

  // Draw lines
  /*for(uint16_t i = 0; i < frame_height; i += 10) {
      drawLine(frame_width/2, frame_height/2, i, 0, COLOR_RED, 0);
      sleep_ms(15);
  }*/
  /*for(uint16_t i = 0; i < frame_height; i += 10) {
      drawLine(NULL, frame_width/2, frame_height/2, frame_width - 1, i, COLOR_GREEN, 0);
      sleep_ms(15);
  }
  for(uint16_t i = frame_width - 1; i > 0; i -= 10) {
      drawLine(NULL, frame_width/2, frame_height/2, i, frame_height - 1, COLOR_BLUE, 0);
      sleep_ms(15);
  }
  for(uint16_t i = frame_height - 1; i > 0; i -= 10) {
      drawLine(NULL, frame_width/2, frame_height/2, 0, i, COLOR_WHITE, 0);
      sleep_ms(15);
  }
  sleep_ms(250);
  clearScreen();
  for(uint16_t i = 0; i < frame_width; i += 10) {
      drawLine(NULL, 0, frame_width - 1, i, 0, COLOR_NAVY, 0);
      sleep_ms(15);
  }
  for(uint16_t i = 0; i < frame_height; i += 10) {
      drawLine(NULL, 0, frame_height, frame_width - 1, i, COLOR_PURPLE, 0);
      sleep_ms(15);
  }
  sleep_ms(250);
  clearScreen();
  for(uint16_t i = frame_width - 1; i > 0; i -= 10) {
      drawLine(NULL, frame_width - 1, frame_height - 1, i, 0, COLOR_YELLOW, 0);
      sleep_ms(15);
  }
  for(uint16_t i = 0; i < frame_height; i += 10) {
      drawLine(NULL, frame_height - 1, frame_height - 1, 0, i, COLOR_BLUE, 0);
      sleep_ms(15);
  }
  sleep_ms(250);
  clearScreen();

  //Draw rectangles
  for(uint8_t i = 0; i < 4; i++) {
      for(uint8_t j = 0; j < 3; j++) {
          drawRectangle(NULL, 25 + i*100, 25 + j*100, 75 + i*100, 75 + j*100, COLOR_MAGENTA);
          sleep_ms(15);
      }
  }
  sleep_ms(250);
  clearScreen();

  //Draw filled rectangles
  for(uint8_t i = 0; i < 16; i++) {
      drawFilledRectangle(NULL, i*25, i*25, frame_width - i*25, frame_height - i*25, 127 + 128*i, 127 + 128*i);
      sleep_ms(15);
  }
  sleep_ms(250);
  clearScreen();

  //Draw circles
  for(uint8_t i = 0; i < 32; i++) {
      drawCircle(NULL, frame_width/2, frame_height/2, i*25, i*8);
      sleep_ms(15);
  }
  sleep_ms(250);
  clearScreen();

  //Draw text
  for(uint8_t i = 0; i < 255; i++) {
      char s[2] = {i, '\0'};
      drawText(NULL, 0, 0, frame_width, s, COLOR_WHITE, COLOR_BLACK, true, 0);
  }
  sleep_ms(500);
  clearScreen();
  */
  while (true) {}
}