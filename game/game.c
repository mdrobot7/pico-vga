#include "../lib/pico-vga.h"
#include "game.h"

//The name of your program. Used to load/store data from the SD card.
const char name[] = "Test";

DisplayConfig_t displayConf = {
    .baseResolution = RES_800x600,
    .resolutionScale = RES_SCALED_400x300,
    .autoRender = true,
    .antiAliasing = false,
    .frameBufferSizeBytes = 0xFFFF,
    .numInterpolatedLines = 0,
    .peripheralMode = false,
    .clearRenderQueueOnDeInit = false,
    .colorDelayCycles = 0
};
ControllerConfig_t controllerConf = {
    .numControllers = 4,
};
AudioConfig_t audioConf = {};
SDConfig_t sdConf = {};
USBHostConfig_t usbConf = {};

void game()
{
    stdio_init_all();
    for(uint8_t i = 0; i < 20; i++) { //5 seconds to open serial communication
        //printf("Waiting for user to open serial...\n");
        //sleep_ms(250);
    }
    //printf("\n");

    initPicoVGA(&displayConf, &controllerConf, &audioConf, &sdConf, &usbConf);

    //Draw lines
    for(uint16_t i = 0; i < frameHeight; i += 10) {
        drawLine(NULL, frameWidth/2, frameHeight/2, i, 0, COLOR_RED, 0);
        sleep_ms(15);
    }
    for(uint16_t i = 0; i < frameHeight; i += 10) {
        drawLine(NULL, frameWidth/2, frameHeight/2, frameWidth - 1, i, COLOR_GREEN, 0);
        sleep_ms(15);
    }
    for(uint16_t i = frameWidth - 1; i > 0; i -= 10) {
        drawLine(NULL, frameWidth/2, frameHeight/2, i, frameHeight - 1, COLOR_BLUE, 0);
        sleep_ms(15);
    }
    for(uint16_t i = frameHeight - 1; i > 0; i -= 10) {
        drawLine(NULL, frameWidth/2, frameHeight/2, 0, i, COLOR_WHITE, 0);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();
    for(uint16_t i = 0; i < frameWidth; i += 10) {
        drawLine(NULL, 0, frameWidth - 1, i, 0, COLOR_NAVY, 0);
        sleep_ms(15);
    }
    for(uint16_t i = 0; i < frameHeight; i += 10) {
        drawLine(NULL, 0, frameHeight, frameWidth - 1, i, COLOR_PURPLE, 0);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();
    for(uint16_t i = frameWidth - 1; i > 0; i -= 10) {
        drawLine(NULL, frameWidth - 1, frameHeight - 1, i, 0, COLOR_YELLOW, 0);
        sleep_ms(15);
    }
    for(uint16_t i = 0; i < frameHeight; i += 10) {
        drawLine(NULL, frameHeight - 1, frameHeight - 1, 0, i, COLOR_BLUE, 0);
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
        drawFilledRectangle(NULL, i*25, i*25, frameWidth - i*25, frameHeight - i*25, 127 + 128*i, 127 + 128*i);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();

    //Draw circles
    for(uint8_t i = 0; i < 32; i++) {
        drawCircle(NULL, frameWidth/2, frameHeight/2, i*25, i*8);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();

    //Draw text
    for(uint8_t i = 0; i < 255; i++) {
        char s[2] = {i, '\0'};
        drawText(NULL, 0, 0, frameWidth, s, COLOR_WHITE, COLOR_BLACK, true, 0);
    }
    sleep_ms(500);
    clearScreen();


}