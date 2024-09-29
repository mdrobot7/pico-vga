#ifndef __PV_PINOUT_H
#define __PV_PINOUT_H

/*
        PINOUT (V1 + Janks)
GPIO0 - Color LSB (BLUE 2)
GPIO 1-6 - Color
GPIO7 - Color MSB (RED 1)
GPIO8 - HSync
GPIO9 - VSync (janked to GPIO10)

GPIO16 - SD Card MISO (RX)
GPIO19 - SD Card MOSI (TX)
GPIO18 - SD Card CLK
GPIO17 - SD Card Chip Select

GPIO27 - Audio L
GPIO28 - Audio R
*/

/*
        PINOUT (V3)
GPIO0  - Color LSB (BLUE 2)
GPIO 1-6 - Color
GPIO7  - Color MSB (RED 1)
GPIO8  - HSync (PWM 4A)
GPIO13 - VSync (PWM 6B)

GPIO14 - Audio L (PWM 7A)
GPIO15 - Audio R (PWM 7B)

GPIO12 - Peripheral Mode MISO (RX) (SPI1)
GPIO11 - Peripheral Mode MOSI (TX)
GPIO10 - Peripheral Mode CLK
GPIO9  - Peripheral Mode Chip Select

GPIO16 - SD Card MISO (RX) (SPI0)
GPIO19 - SD Card MOSI (TX)
GPIO18 - SD Card CLK
GPIO17 - SD Card Chip Select

GPIO20 - Controller SDA (I2C0)
GPIO21 - Controller SCL
*/

#define COLOR_LSB_PIN   0
#define HSYNC_PIN       8
#define HSYNC_PWM_SLICE (HSYNC_PIN / 2) % 8
#define HSYNC_PWM_CHAN  (HSYNC_PIN % 2)
#define VSYNC_PIN       10
#define VSYNC_PWM_SLICE (VSYNC_PIN / 2) % 8
#define VSYNC_PWM_CHAN  (VSYNC_PIN % 2)

#define AUDIO_L_PIN       27
#define AUDIO_L_PWM_SLICE (AUDIO_L_PIN / 2) % 8
#define AUDIO_L_PWM_CHAN  (AUDIO_L_PIN % 2)
#define AUDIO_R_PIN       28
#define AUDIO_R_PWM_SLICE (AUDIO_R_PIN / 2) % 8
#define AUDIO_R_PWM_CHAN  (AUDIO_R_PIN % 2)

#define SD_MISO_PIN 16
#define SD_MOSI_PIN 19
#define SD_CLK_PIN  18
#define SD_CS_PIN   17

#define PERIPHERAL_MISO_PIN 16
#define PERIPHERAL_MOSI_PIN 19
#define PERIPHERAL_CLK_PIN  18
#define PERIPHERAL_CS_PIN   17

#define CONTROLLER_SDA_PIN 20
#define CONTROLLER_SCL_PIN 21
#define CONTROLLER_I2C     i2c0

#endif