#ifndef __PV_COLOR_H
#define __PV_COLOR_H

#include "pico/stdlib.h"

#define COLOR_WHITE   0b11111111
#define COLOR_SILVER  0b10110110
#define COLOR_GRAY    0b10010010
#define COLOR_BLACK   0b00000000

#define COLOR_RED     0b11100000
#define COLOR_MAROON  0b10000000
#define COLOR_YELLOW  0b11111100
#define COLOR_OLIVE   0b10010000
#define COLOR_LIME    0b00011100
#define COLOR_GREEN   0b00010000
#define COLOR_TEAL    0b00010010
#define COLOR_CYAN    0b00011111
#define COLOR_BLUE    0b00000011
#define COLOR_NAVY    0b00000010
#define COLOR_MAGENTA 0b11100011
#define COLOR_PURPLE  0b10000010

/**
 * @brief Convert a 3 byte HTML color code into an 8 bit color.
 *
 * @param color The HTML color code.
 * @return An 8 bit compressed color.
 */
static inline uint8_t color_html_to_8bit(uint32_t color) {
    return color_rgb_to_8bit((uint8_t)((color >> 16) & 255), (uint8_t)((color >> 8) & 255), (uint8_t)((color & 255)));
}

/**
 * @brief Convert red, green, and blue color values into a single 8 bit color.
 *
 * @param r Red value
 * @param g Blue value
 * @param b Green value
 * @return An 8 bit compressed color.
 */
static inline uint8_t color_rgb_to_8bit(uint8_t r, uint8_t g, uint8_t b) {
    return ((r/32) << 5) | ((g/32) << 2) | (b/64);
}

/**
 * @brief Convert HSV colors into 8 bit compressed RGB.
 * @param hue Hue value, 0-255
 * @param saturation Saturation value, 0-255
 * @param value Brightness, 0-255
 * @return An 8 bit compressed color
 */
static inline uint8_t color_hsb_to_8bit(uint8_t hue, uint8_t saturation, uint8_t value) {
    float max = value;
    float min = max*(255 - saturation);

    // Slow...
    float x = (max - min)*(1 - fabs(fmod(hue/(255*(1.0/6.0)), 2) - 1));

    uint8_t r, g, b;
    if(hue < 255*(1.0/6.0)) {
        r = max;
        g = x + min;
        b = min;
    }
    else if(hue < 255*(2.0/6.0)) {
        r = x + min;
        g = max;
        b = min;
    }
    else if(hue < 255*(3.0/6.0)) {
        r = min;
        g = max;
        b = x + min;
    }
    else if(hue < 255*(4.0/6.0)) {
        r = min;
        g = x + min;
        b = max;
    }
    else if(hue < 255*(5.0/6.0)) {
        r = x + min;
        g = min;
        b = max;
    }
    else if(hue < 255*(6.0/6.0)) {
        r = max;
        g = min;
        b = x + min;
    }
    else return 0;

    return color_rgb_to_8bit(r, g, b);
}

/**
 * @brief Invert a color.
 *
 * @param color A color to invert.
 * @return The inverted version of the supplied color.
 */
static inline uint8_t color_invert(uint8_t color) {
    return 255 - color;
}

#endif