#include "main.h"
#include "sdk/pico-vga.h"
#include "game/game.h"
#include "pico/stdlib.h"

int main() {
    stdio_init_all();
    for(uint8_t i = 0; i < 20; i++) { //5 seconds to open serial communication
        //printf("Waiting for user to open serial...\n");
        //sleep_ms(250);
    }
    //printf("\n");

    initDisplay(true);
    
    game();
}