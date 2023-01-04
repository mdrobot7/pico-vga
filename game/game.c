#include "../sdk/pico-vga.h"
#include "game.h"

void game()
{
    stdio_init_all();
    for(uint8_t i = 0; i < 20; i++) { //5 seconds to open serial communication
        //printf("Waiting for user to open serial...\n");
        //sleep_ms(250);
    }
    //printf("\n");

    initDisplay(true);
}