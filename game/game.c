#include "../sdk/pico-vga.h"
#include "game.h"

Controllers_t controllers = {
    .maxNumControllers = 1,
    .p = null,
};

DisplayConfig_t displayConf = {};
ControllerConfig_t controllerConf = {
    .maxNumControllers = 1,
    .controllers = &controllers,
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

    initDisplay(true);

    //Draw lines
    for(uint16_t i = 0; i < FRAME_WIDTH; i += 10) {
        drawLine(NULL, FRAME_WIDTH/2, FRAME_HEIGHT/2, i, 0, COLOR_RED, 0);
        sleep_ms(15);
    }
    for(uint16_t i = 0; i < FRAME_HEIGHT; i += 10) {
        drawLine(NULL, FRAME_WIDTH/2, FRAME_HEIGHT/2, FRAME_WIDTH - 1, i, COLOR_GREEN, 0);
        sleep_ms(15);
    }
    for(uint16_t i = FRAME_WIDTH - 1; i > 0; i -= 10) {
        drawLine(NULL, FRAME_WIDTH/2, FRAME_HEIGHT/2, i, FRAME_HEIGHT - 1, COLOR_BLUE, 0);
        sleep_ms(15);
    }
    for(uint16_t i = FRAME_HEIGHT - 1; i > 0; i -= 10) {
        drawLine(NULL, FRAME_WIDTH/2, FRAME_HEIGHT/2, 0, i, COLOR_WHITE, 0);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();
    for(uint16_t i = 0; i < FRAME_WIDTH; i += 10) {
        drawLine(NULL, 0, FRAME_HEIGHT - 1, i, 0, COLOR_NAVY, 0);
        sleep_ms(15);
    }
    for(uint16_t i = 0; i < FRAME_HEIGHT; i += 10) {
        drawLine(NULL, 0, FRAME_HEIGHT, FRAME_WIDTH - 1, i, COLOR_PURPLE, 0);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();
    for(uint16_t i = FRAME_WIDTH - 1; i > 0; i -= 10) {
        drawLine(NULL, FRAME_WIDTH - 1, FRAME_HEIGHT - 1, i, 0, COLOR_YELLOW, 0);
        sleep_ms(15);
    }
    for(uint16_t i = 0; i < FRAME_HEIGHT; i += 10) {
        drawLine(NULL, FRAME_WIDTH - 1, FRAME_HEIGHT - 1, 0, i, COLOR_BLUE, 0);
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
        drawFilledRectangle(NULL, i*25, i*25, FRAME_WIDTH - i*25, FRAME_HEIGHT - i*25, 127 + 128*i, 127 + 128*i);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();

    //Draw circles
    for(uint8_t i = 0; i < 32; i++) {
        drawCircle(NULL, FRAME_WIDTH/2, FRAME_HEIGHT/2, i*25, i*8);
        sleep_ms(15);
    }
    sleep_ms(250);
    clearScreen();

    //Draw text
    for(uint8_t i = 0; i < 255; i++) {
        char s[2] = {i, '\0'};
        drawText(NULL, 0, 0, FRAME_WIDTH, s, COLOR_WHITE, COLOR_BLACK, true, 0);
    }
    sleep_ms(500);
    clearScreen();


}