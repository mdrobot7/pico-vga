#include "lib-internal.h"

uint8_t HTMLTo8Bit(uint32_t color) {
    return rgbTo8Bit((uint8_t)((color >> 16) & 255), (uint8_t)((color >> 8) & 255), (uint8_t)((color & 255)));
}
uint8_t rgbTo8Bit(uint8_t r, uint8_t g, uint8_t b) {
    return ((r/32) << 5) | ((g/32) << 2) | (b/64);
}

//Hue, saturation, and value are all 0-255 integers
uint8_t hsvToRGB(uint8_t hue, uint8_t saturation, uint8_t value) {
    double max = value;
    double min = max*(255 - saturation);

    double x = (max - min)*(1 - fabs(fmod(hue/(255*(1.0/6.0)), 2) - 1));

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

    return rgbTo8Bit(r, g, b);
}

uint8_t invertColor(uint8_t color) {
    return 255 - color;
}