#include "sdk/sdk.h"
#include "main.h"
#include "game/game.h"

int main() {
    stdio_init_all();
    for(uint8_t i = 0; i < 20; i++) { //5 seconds to open serial communication
        //printf("Waiting for user to open serial...\n");
        //sleep_ms(250);
    }
    //printf("\n");

    Controller P1;
    Controller P2;
    Controller P3;
    Controller P4;
    
    initDisplay(&P1, &P2, &P3, &P4, true);
    
    while(1);
    
    game();
}