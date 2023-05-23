#ifndef PINOUT_H
#define PINOUT_H

/*
        PINOUT (V1)
GPIO0 - Color LSB (BLUE 2)
GPIO 1-6 - Color
GPIO7 - Color MSB (RED 1)
GPIO8 - HSync
GPIO9 - VSync (janked to GPIO10)

GPIO16 - SD Card MISO (RX)
GPIO17 - SD Card Chip Select
GPIO18 - SD Card CLK
GPIO19 - SD Card MOSI (TX)

GPIO27 - Audio L
GPIO28 - Audio R
*/

/*
        PINOUT (V2)
GPIO0  - Color LSB (BLUE 2)
GPIO 1-6 - Color
GPIO7  - Color MSB (RED 1)
GPIO8  - HSync (PWM 4A)
GPIO13 - VSync (PWM 6B)

GPIO12 - SD Card MISO (RX) (SPI1)
GPIO11 - SD Card MOSI (TX)
GPIO10 - SD Card CLK
GPIO9  - SD Card Chip Select

GPIO14 - Audio L (PWM 7A)
GPIO15 - Audio R (PWM 7B)

GPIO20 - Controller SDA (I2C0)
GPIO21 - Controller SCL
*/

#define COLOR_LSB_PIN 0
#define HSYNC_PIN 8
#define HSYNC_PWM_SLICE (HSYNC_PIN / 2) % 8
#define HSYNC_PWM_CHAN  (HSYNC_PIN % 2)
#define VSYNC_PIN 13
#define VSYNC_PWM_SLICE (VSYNC_PIN / 2) % 8
#define VSYNC_PWM_CHAN  (VSYNC_PIN % 2)

#define AUDIO_L_PIN 14
#define AUDIO_L_PWM_SLICE (AUDIO_L_PIN / 2) % 8
#define AUDIO_L_PWM_CHAN  (AUDIO_L_PIN % 2)
#define AUDIO_R_PIN 15
#define AUDIO_R_PWM_SLICE (AUDIO_R_PIN / 2) % 8
#define AUDIO_R_PWM_CHAN  (AUDIO_R_PIN % 2)

#define SD_MISO_PIN 12
#define SD_MOSI_PIN 11
#define SD_CLK_PIN 10
#define SD_CS_PIN 9

#endif