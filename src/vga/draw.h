#ifndef __PV_DRAW_H
#define __PV_DRAW_H

#include "hardware/pio.h"
#include "pico/stdlib.h"

typedef enum {
  RES_800x600,
  RES_640x480,
  RES_1024x768,
} DisplayConfigBaseResolution_t;

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
} DisplayConfigResolutionScale_t;

typedef struct {
  PIO pio; // Which PIO to use for color
  DisplayConfigBaseResolution_t baseResolution;
  DisplayConfigResolutionScale_t resolutionScale;
  bool autoRender;                // Turn on autoRendering (no manual updateDisplay() call required)
  bool antiAliasing;              // Turn antialiasing on or off
  uint32_t frameBufferSizeBytes;  // Cap the size of the frame buffer (if unused, set to 0xFFFF) -- Initializer will write back the amount of memory used. Default: Either the size of one frame or 75% of PICO_VGA_MAX_MEMORY_BYTES, whichever is less.
  uint8_t numInterpolatedLines;   // Override the default number of interpolated frame lines -- used if the frame buffer is not large enough to hold all of the frame data. Default = 2.
  uint32_t renderQueueSizeBytes;  // Cap the size of the render queue (if unused, set to 0).
  bool peripheralMode;            // Enable command input via SPI.
  bool clearRenderQueueOnDeInit;  // Clear the render queue after a call to deInitPicoVGA()
  bool disconnectDisplay;         // Setting to true causes the display to disconnect and go to sleep after a call to deInitPicoVGA().
  bool interpolatedRenderingMode; // Set the rendering mode when interpolating lines. False: begin interpolating lines after rendering the rest of the frame. True: Interpolate lines as they are displayed in real time
  uint16_t colorDelayCycles;
} DisplayConfig_t;

// Default configuration for DisplayConfig_t
#define DISPLAY_CONFIG_DEFAULT {                   \
  .PIO = pio0                                      \
           .baseResolution                         \
  = RES_800x600,                                   \
  .resolutionScale           = RES_SCALED_400x300, \
  .autoRender                = true,               \
  .antiAliasing              = false,              \
  .frameBufferSizeBytes      = 0xFFFFFFFF,         \
  .renderQueueSizeBytes      = 0,                  \
  .numInterpolatedLines      = 0,                  \
  .peripheralMode            = false,              \
  .clearRenderQueueOnDeInit  = false,              \
  .disconnectDisplay         = false,              \
  .interpolatedRenderingMode = false,              \
  .colorDelayCycles          = 0,                  \
}

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

RenderQueueUID_t drawSprite(uint8_t * sprite, uint16_t x, uint16_t y, uint16_t dimX, uint16_t dimY, uint8_t nullColor, int8_t scaleX, int8_t scaleY);

void clearScreen();

void setHidden(RenderQueueUID_t itemUID, bool hidden);
void removeItem(RenderQueueUID_t itemUID);

#endif